#include "Test.hpp"
#include "Hockey/Core/FileSystem.hpp"
#include "Hockey/Core/Paths.hpp"
#include <cstddef>
using namespace Hockey;
void RunFileSystemTests() {
    HockeyTest::BeginSuite("FileSystemTests");

    const auto dir = Paths::TempFile("fs_test_dir");
    FileSystem::Remove(dir);

    HK_CHECK_MSG(static_cast<bool>(FileSystem::CreateDirectories(dir)), "create directory");
    HK_CHECK(FileSystem::IsDirectory(dir));

    const auto file = dir / "hello.txt";
    HK_CHECK_MSG(static_cast<bool>(FileSystem::WriteTextFile(file, "hello world")),
                 "write text file");
    HK_CHECK(FileSystem::IsFile(file));

    const auto read = FileSystem::ReadTextFile(file);
    HK_CHECK_MSG(static_cast<bool>(read), "read text file");
    HK_CHECK_EQ(read.value, std::string("hello world"));

    const auto files = FileSystem::ListFiles(dir);
    HK_CHECK_EQ(files.size(), static_cast<size_t>(1));

    const auto missing = FileSystem::ReadTextFile(dir / "nope.txt");
    HK_CHECK_MSG(!missing, "missing file returns failure");

    FileSystem::Remove(dir);
    HK_CHECK(!FileSystem::Exists(dir));
}
