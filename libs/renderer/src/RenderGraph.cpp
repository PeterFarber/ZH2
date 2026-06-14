#include "Hockey/Renderer/RenderGraph.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <set>

#include "Hockey/Renderer/RenderContext.hpp"

namespace Hockey {

namespace {
constexpr uint32_t kInvalidIndex = std::numeric_limits<uint32_t>::max();
} // namespace

uint32_t RenderGraph::ResourceIndex(TextureHandle handle) const {
    if (!handle.IsValid() || handle.id > m_Resources.size()) {
        return kInvalidIndex;
    }
    return handle.id - 1;
}

bool RenderGraph::IsValidResource(TextureHandle handle) const {
    return ResourceIndex(handle) != kInvalidIndex;
}

const RenderGraph::Resource* RenderGraph::GetResource(TextureHandle handle) const {
    const uint32_t index = ResourceIndex(handle);
    if (index == kInvalidIndex) {
        return nullptr;
    }
    return &m_Resources[index];
}

void RenderGraph::RecomputeSizing(Resource& resource) const {
    if (resource.sizing.mode != RenderGraphSizing::Mode::RenderAreaRelative) {
        return;
    }
    if (m_RenderAreaWidth == 0 || m_RenderAreaHeight == 0) {
        return; // no render area yet; keep the desc's placeholder dimensions
    }
    const float w = static_cast<float>(m_RenderAreaWidth) * resource.sizing.scale;
    const float h = static_cast<float>(m_RenderAreaHeight) * resource.sizing.scale;
    resource.desc.width = std::max(1u, static_cast<uint32_t>(std::lround(w)));
    resource.desc.height = std::max(1u, static_cast<uint32_t>(std::lround(h)));
}

void RenderGraph::Invalidate() {
    m_Compiled = false;
    m_ExecutionOrder.clear();
}

TextureHandle RenderGraph::ImportTexture(const TextureDesc& desc, TextureHandle external) {
    Resource resource;
    resource.desc = desc;
    resource.sizing = RenderGraphSizing::Absolute();
    resource.imported = true;
    resource.external = external;
    resource.dirty = false; // imported resources are owned/realized externally
    m_Resources.push_back(resource);
    Invalidate();
    return TextureHandle{static_cast<uint32_t>(m_Resources.size())};
}

TextureHandle RenderGraph::CreateTransientTexture(const TextureDesc& desc, RenderGraphSizing sizing) {
    Resource resource;
    resource.desc = desc;
    resource.sizing = sizing;
    resource.imported = false;
    resource.dirty = true;
    RecomputeSizing(resource);
    m_Resources.push_back(resource);
    Invalidate();
    return TextureHandle{static_cast<uint32_t>(m_Resources.size())};
}

RenderPassHandle RenderGraph::AddPass(const RenderPassDesc& desc) {
    PassNode node;
    node.desc = desc;

    for (const TextureHandle read : desc.reads) {
        node.reads.push_back(ResourceIndex(read));
    }
    for (const ColorAttachment& color : desc.colorAttachments) {
        node.writes.push_back(ResourceIndex(color.texture));
    }
    if (desc.depth.Enabled()) {
        node.writes.push_back(ResourceIndex(desc.depth.texture));
    }

    m_Passes.push_back(std::move(node));
    Invalidate();
    return RenderPassHandle{static_cast<uint32_t>(m_Passes.size())};
}

Status RenderGraph::Compile() {
    m_LastError.clear();
    m_ExecutionOrder.clear();
    m_Compiled = false;

    const uint32_t passCount = static_cast<uint32_t>(m_Passes.size());

    for (Resource& resource : m_Resources) {
        resource.firstUse = -1;
        resource.lastUse = -1;
    }

    // Gather writers/readers per resource, validating that every referenced
    // handle belongs to this graph.
    std::vector<std::vector<uint32_t>> writers(m_Resources.size());
    std::vector<std::vector<uint32_t>> readers(m_Resources.size());

    for (uint32_t p = 0; p < passCount; ++p) {
        const PassNode& node = m_Passes[p];
        for (const uint32_t r : node.reads) {
            if (r == kInvalidIndex) {
                m_LastError = "Pass '" + node.desc.name + "' reads an unknown resource";
                return Status::Fail(m_LastError);
            }
            readers[r].push_back(p);
        }
        for (const uint32_t w : node.writes) {
            if (w == kInvalidIndex) {
                m_LastError = "Pass '" + node.desc.name + "' writes an unknown resource";
                return Status::Fail(m_LastError);
            }
            writers[w].push_back(p);
        }
    }

    // Missing dependency: a transient resource is read but never produced.
    for (uint32_t idx = 0; idx < m_Resources.size(); ++idx) {
        if (!readers[idx].empty() && writers[idx].empty() && !m_Resources[idx].imported) {
            const std::string& readerName = m_Passes[readers[idx].front()].desc.name;
            m_LastError = "Pass '" + readerName + "' reads resource " + std::to_string(idx + 1) +
                          " which is never written (missing dependency)";
            return Status::Fail(m_LastError);
        }
    }

    // Build dependency edges: every writer must run before every reader (RAW),
    // and writers to the same resource keep their declaration order (WAW).
    std::vector<std::set<uint32_t>> adjacency(passCount);
    std::vector<int> indegree(passCount, 0);
    const auto addEdge = [&](uint32_t from, uint32_t to) {
        if (from == to) {
            return;
        }
        if (adjacency[from].insert(to).second) {
            ++indegree[to];
        }
    };

    for (uint32_t idx = 0; idx < m_Resources.size(); ++idx) {
        for (const uint32_t w : writers[idx]) {
            for (const uint32_t rd : readers[idx]) {
                addEdge(w, rd);
            }
        }
        for (size_t i = 1; i < writers[idx].size(); ++i) {
            addEdge(writers[idx][i - 1], writers[idx][i]);
        }
    }

    // Kahn topological sort. Ready passes are processed in ascending index order
    // so the compiled order is deterministic.
    std::set<uint32_t> ready;
    for (uint32_t p = 0; p < passCount; ++p) {
        if (indegree[p] == 0) {
            ready.insert(p);
        }
    }

    m_ExecutionOrder.reserve(passCount);
    while (!ready.empty()) {
        const uint32_t node = *ready.begin();
        ready.erase(ready.begin());
        m_ExecutionOrder.push_back(node);
        for (const uint32_t next : adjacency[node]) {
            if (--indegree[next] == 0) {
                ready.insert(next);
            }
        }
    }

    if (m_ExecutionOrder.size() != passCount) {
        m_LastError = "Render graph contains a cycle";
        m_ExecutionOrder.clear();
        return Status::Fail(m_LastError);
    }

    // Resource lifetimes: first/last position in the execution order that uses
    // each resource.
    for (uint32_t orderPos = 0; orderPos < m_ExecutionOrder.size(); ++orderPos) {
        const PassNode& node = m_Passes[m_ExecutionOrder[orderPos]];
        const auto touch = [&](uint32_t resIdx) {
            Resource& res = m_Resources[resIdx];
            if (res.firstUse < 0) {
                res.firstUse = static_cast<int>(orderPos);
            }
            res.lastUse = static_cast<int>(orderPos);
        };
        for (const uint32_t r : node.reads) {
            touch(r);
        }
        for (const uint32_t w : node.writes) {
            touch(w);
        }
    }

    m_Compiled = true;
    return Status::Ok();
}

void RenderGraph::Execute(RenderContext& context) {
    if (!m_Compiled) {
        return;
    }
    for (const uint32_t passIndex : m_ExecutionOrder) {
        PassNode& node = m_Passes[passIndex];
        if (context.beginPass) {
            context.beginPass(context, node.desc);
        }
        if (node.desc.execute) {
            node.desc.execute(context);
        }
        if (context.endPass) {
            context.endPass(context, node.desc);
        }
    }
}

void RenderGraph::Resize(uint32_t width, uint32_t height) {
    m_RenderAreaWidth = width;
    m_RenderAreaHeight = height;
    for (Resource& resource : m_Resources) {
        if (resource.imported || resource.sizing.mode != RenderGraphSizing::Mode::RenderAreaRelative) {
            continue;
        }
        const uint32_t oldWidth = resource.desc.width;
        const uint32_t oldHeight = resource.desc.height;
        RecomputeSizing(resource);
        if (resource.desc.width != oldWidth || resource.desc.height != oldHeight) {
            resource.dirty = true;
        }
    }
}

void RenderGraph::MarkResourcesRealized() {
    for (Resource& resource : m_Resources) {
        resource.dirty = false;
    }
}

void RenderGraph::Clear() {
    m_Resources.clear();
    m_Passes.clear();
    m_ExecutionOrder.clear();
    m_Compiled = false;
    m_LastError.clear();
}

const std::string& RenderGraph::PassNameAtOrder(size_t orderIndex) const {
    if (orderIndex >= m_ExecutionOrder.size()) {
        return m_EmptyName;
    }
    return m_Passes[m_ExecutionOrder[orderIndex]].desc.name;
}

} // namespace Hockey
