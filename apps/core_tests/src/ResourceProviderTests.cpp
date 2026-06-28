#include "Test.hpp"

#include "Hockey/Core/FileSystem.hpp"
#include "Hockey/Core/Paths.hpp"
#include "Hockey/Core/ResourceProvider.hpp"

#include <cstddef>
#include <filesystem>
#include <string>
#include <string_view>
#include <vector>

namespace {

std::vector<std::byte> Bytes(std::string_view text) {
    std::vector<std::byte> bytes;
    bytes.reserve(text.size());
    for (const char ch : text) {
        bytes.push_back(static_cast<std::byte>(ch));
    }
    return bytes;
}

} // namespace

void RunResourceProviderTests() {
    HockeyTest::BeginSuite("ResourceProviderTests");

    const std::filesystem::path workspace = Hockey::Paths::TempFile("resource_provider_tests");
    Hockey::FileSystem::Remove(workspace);
    HK_CHECK(Hockey::FileSystem::WriteTextFile(workspace / "data" / "config" / "editor.toml",
                                                "[app]\nname = \"Editor\"\n"));
    HK_CHECK(Hockey::FileSystem::WriteTextFile(workspace / "data" / "bin" / "payload.bin", "payload"));

    Hockey::FileResourceProvider files(workspace);
    const Hockey::Result<std::string> editorConfig = files.ReadText("data/config/editor.toml");
    HK_CHECK(editorConfig);
    HK_CHECK(editorConfig && editorConfig.value.find("Editor") != std::string::npos);
    const Hockey::Result<std::vector<std::byte>> binary = files.ReadBinary("data/bin/payload.bin");
    HK_CHECK(binary);
    HK_CHECK_EQ(binary ? static_cast<int>(binary.value.size()) : 0, 7);
    HK_CHECK(!files.ReadText("data/config/missing.toml"));
    HK_CHECK(!files.ReadText("/absolute/not_allowed.toml"));
    HK_CHECK(!files.ReadText("data/../outside.toml"));

    std::vector<Hockey::ResourcePackageEntry> entries{
        {"data/ui/menu.rml", Bytes("<rml />")},
        {"data/gameplay/tuning.default.yaml", Bytes("skater:\n  max_speed: 7\n")},
    };
    Hockey::EmbeddedResourceProvider embedded(entries);
    const Hockey::Result<std::string> ui = embedded.ReadText("data/ui/menu.rml");
    HK_CHECK(ui);
    HK_CHECK(ui && ui.value == "<rml />");
    HK_CHECK(embedded.Contains("data/gameplay/tuning.default.yaml"));
    HK_CHECK(!embedded.ReadBinary("data/ui/missing.rml"));
    HK_CHECK(!embedded.ReadBinary("C:/absolute/not_allowed.rml"));

    const Hockey::Result<std::vector<std::byte>> package = Hockey::EncodeResourcePackage(entries);
    HK_CHECK(package);
    const Hockey::Result<std::vector<Hockey::ResourcePackageEntry>> decoded =
        package ? Hockey::DecodeResourcePackage(package.value) :
                  Hockey::Result<std::vector<Hockey::ResourcePackageEntry>>::Fail("package encode failed");
    HK_CHECK(decoded);
    HK_CHECK_EQ(decoded ? static_cast<int>(decoded.value.size()) : 0, 2);
    HK_CHECK(decoded && decoded.value[0].path == "data/gameplay/tuning.default.yaml");

    const Hockey::Result<Hockey::EmbeddedResourceProvider> fromPackage =
        package ? Hockey::EmbeddedResourceProvider::FromPackage(package.value) :
                  Hockey::Result<Hockey::EmbeddedResourceProvider>::Fail("package encode failed");
    HK_CHECK(fromPackage);
    const Hockey::Result<std::string> packageTuning =
        fromPackage ? fromPackage.value.ReadText("data/gameplay/tuning.default.yaml") :
                      Hockey::Result<std::string>::Fail("package decode failed");
    HK_CHECK(packageTuning);
    HK_CHECK(packageTuning && packageTuning.value.find("max_speed") != std::string::npos);

    const std::filesystem::path packagePath = workspace / "out" / "runtime.hkpack";
    HK_CHECK(Hockey::WriteResourcePackageFile(packagePath, entries));
    const Hockey::Result<std::vector<Hockey::ResourcePackageEntry>> fromFile =
        Hockey::ReadResourcePackageFile(packagePath);
    HK_CHECK(fromFile);
    HK_CHECK_EQ(fromFile ? static_cast<int>(fromFile.value.size()) : 0, 2);

    Hockey::FileSystem::Remove(workspace);
}
