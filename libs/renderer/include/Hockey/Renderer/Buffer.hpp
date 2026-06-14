#pragma once

#include <cstddef>
#include <string>

namespace Hockey {

enum class BufferUsage { Vertex, Index, Uniform, Storage, Staging, Readback };

struct BufferDesc {
    size_t size = 0;
    BufferUsage usage = BufferUsage::Vertex;
    bool cpuVisible = false;
    std::string debugName;
};

} // namespace Hockey
