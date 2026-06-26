#include "Hockey/UI/RmlUiFileInterface.hpp"

#include <cstdio>
#include <fstream>
#include <memory>

namespace Hockey {
namespace {

std::ifstream* ToStream(Rml::FileHandle file) {
    return reinterpret_cast<std::ifstream*>(file);
}

bool HasParentTraversal(const std::filesystem::path& path) {
    for (const auto& part : path) {
        if (part == "..") {
            return true;
        }
    }
    return false;
}

} // namespace

RmlUiFileInterface::RmlUiFileInterface(std::filesystem::path root) : m_Root(std::move(root)) {}

Rml::FileHandle RmlUiFileInterface::Open(const Rml::String& path) {
    const std::filesystem::path resolved = ResolveUiPath(path);
    if (resolved.empty()) {
        return {};
    }

    auto stream = std::make_unique<std::ifstream>(resolved, std::ios::binary);
    if (!stream->is_open()) {
        return {};
    }

    return reinterpret_cast<Rml::FileHandle>(stream.release());
}

void RmlUiFileInterface::Close(Rml::FileHandle file) {
    delete ToStream(file);
}

size_t RmlUiFileInterface::Read(void* buffer, size_t size, Rml::FileHandle file) {
    std::ifstream* stream = ToStream(file);
    if (stream == nullptr || buffer == nullptr || size == 0) {
        return 0;
    }

    stream->read(static_cast<char*>(buffer), static_cast<std::streamsize>(size));
    return static_cast<size_t>(stream->gcount());
}

bool RmlUiFileInterface::Seek(Rml::FileHandle file, long offset, int origin) {
    std::ifstream* stream = ToStream(file);
    if (stream == nullptr) {
        return false;
    }

    std::ios_base::seekdir direction = std::ios::beg;
    if (origin == SEEK_CUR) {
        direction = std::ios::cur;
    } else if (origin == SEEK_END) {
        direction = std::ios::end;
    } else if (origin != SEEK_SET) {
        return false;
    }

    stream->clear();
    stream->seekg(offset, direction);
    return !stream->fail();
}

size_t RmlUiFileInterface::Tell(Rml::FileHandle file) {
    std::ifstream* stream = ToStream(file);
    if (stream == nullptr) {
        return 0;
    }

    const std::ifstream::pos_type position = stream->tellg();
    if (position < 0) {
        return 0;
    }
    return static_cast<size_t>(position);
}

std::filesystem::path RmlUiFileInterface::ResolveUiPath(const Rml::String& path) const {
    std::filesystem::path requested(path);
    if (requested.empty() || requested.is_absolute() || requested.has_root_name() || requested.has_root_directory() ||
        HasParentTraversal(requested)) {
        return {};
    }

    requested = requested.lexically_normal();
    std::filesystem::path relativeToRoot;

    auto it = requested.begin();
    if (it != requested.end() && *it == "data") {
        auto next = it;
        ++next;
        if (next == requested.end() || *next != "ui") {
            return {};
        }
        relativeToRoot = requested;
    } else {
        relativeToRoot = std::filesystem::path("data") / "ui" / requested;
    }

    if (HasParentTraversal(relativeToRoot)) {
        return {};
    }
    return (m_Root / relativeToRoot).lexically_normal();
}

} // namespace Hockey
