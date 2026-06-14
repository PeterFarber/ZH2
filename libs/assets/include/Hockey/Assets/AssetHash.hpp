#pragma once
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <string_view>

namespace Hockey {

// Stable, platform-independent 64-bit hashing used for dirty detection and the
// source hash recorded in cooked outputs. Uses FNV-1a so results are identical
// on Windows and Linux for the same bytes.
namespace AssetHash {

uint64_t HashBytes(const void* data, size_t size);
uint64_t HashString(std::string_view text);

// Hashes the full contents of a file. Returns 0 if the file cannot be read.
uint64_t HashFile(const std::filesystem::path& path);

} // namespace AssetHash
} // namespace Hockey
