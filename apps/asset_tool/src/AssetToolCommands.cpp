#include "AssetToolCommands.hpp"

#include <cstdio>
#include <stdexcept>
#include <string>
#include <vector>

#include "Hockey/Assets/AssetDatabase.hpp"
#include "Hockey/Assets/AssetManager.hpp"
#include "Hockey/Assets/AssetType.hpp"

namespace Hockey {

namespace {

// Parses a decimal AssetID string. Returns an invalid (0) id on parse failure.
AssetID ParseAssetID(const std::string& text) {
    try {
        return AssetID(static_cast<std::uint64_t>(std::stoull(text)));
    } catch (const std::exception&) {
        return AssetID{};
    }
}

const char* StatusLabel(const AssetMetadata& meta) {
    if (meta.missing) {
        return "missing";
    }
    if (meta.dirty) {
        return "dirty";
    }
    if (meta.cooked) {
        return "cooked";
    }
    return "imported";
}

void PrintMetadata(const AssetMetadata& meta) {
    std::printf("  id:        %s\n", meta.id.ToString().c_str());
    std::printf("  type:      %s\n", AssetTypeToString(meta.type).c_str());
    std::printf("  status:    %s\n", StatusLabel(meta));
    std::printf("  name:      %s\n", meta.name.c_str());
    std::printf("  raw:       %s\n", meta.rawPath.generic_string().c_str());
    std::printf("  cooked:    %s\n", meta.cookedPath.generic_string().c_str());
    std::printf("  rawHash:   %llu\n", static_cast<unsigned long long>(meta.rawFileHash));
    if (!meta.dependencies.empty()) {
        std::printf("  deps:      ");
        for (const AssetID dep : meta.dependencies) {
            std::printf("%s ", dep.ToString().c_str());
        }
        std::printf("\n");
    }
    if (!meta.dependents.empty()) {
        std::printf("  dependents:");
        for (const AssetID dep : meta.dependents) {
            std::printf(" %s", dep.ToString().c_str());
        }
        std::printf("\n");
    }
}

int ReportStatus(const Status& status) {
    if (!status) {
        std::fprintf(stderr, "error: %s\n", status.error.c_str());
        return 1;
    }
    return 0;
}

} // namespace

void PrintAssetToolUsage() {
    std::printf("HockeyAssetTool --root <dir> <command> [args]\n");
    std::printf("Commands:\n");
    std::printf("  discover                 scan data/raw, create/update metadata, save db\n");
    std::printf("  import <raw-path>         import one raw file\n");
    std::printf("  import-all               import all supported raw files\n");
    std::printf("  cook <asset-id>          cook one asset\n");
    std::printf("  cook-all-dirty           cook dirty assets and dependents\n");
    std::printf("  recook-all               force recook all supported assets\n");
    std::printf("  validate                 check files/cooked outputs/dependencies\n");
    std::printf("  list                     print asset id, type, status, path\n");
    std::printf("  print <asset-id>         print metadata for one asset\n");
}

int RunAssetToolCommand(const std::filesystem::path& root, const std::string& command,
                        const std::vector<std::string>& args) {
    AssetManager manager;
    if (const Status status = manager.Init(AssetManager::DefaultCreateInfo(root)); !status) {
        std::fprintf(stderr, "failed to init asset manager: %s\n", status.error.c_str());
        return 1;
    }

    if (command == "discover") {
        const Status status = manager.DiscoverRawAssets();
        manager.SaveDatabase();
        std::printf("discovered %zu assets\n", manager.Database().Count());
        return ReportStatus(status);
    }

    if (command == "import") {
        if (args.empty()) {
            std::fprintf(stderr, "import requires a raw path\n");
            return 1;
        }
        const Status status = manager.ImportAsset(args.front());
        manager.SaveDatabase();
        return ReportStatus(status);
    }

    if (command == "import-all") {
        const Status status = manager.ImportAll();
        manager.SaveDatabase();
        std::printf("imported; database holds %zu assets\n", manager.Database().Count());
        return ReportStatus(status);
    }

    if (command == "cook") {
        if (args.empty()) {
            std::fprintf(stderr, "cook requires an asset id\n");
            return 1;
        }
        const AssetID id = ParseAssetID(args.front());
        if (!id.IsValid()) {
            std::fprintf(stderr, "invalid asset id: %s\n", args.front().c_str());
            return 1;
        }
        return ReportStatus(manager.CookAsset(id));
    }

    if (command == "cook-all-dirty") {
        manager.DiscoverRawAssets();
        manager.ImportAll();
        const Status status = manager.CookAllDirty();
        manager.SaveDatabase();
        return ReportStatus(status);
    }

    if (command == "recook-all") {
        manager.DiscoverRawAssets();
        manager.ImportAll();
        const Status status = manager.RecookAll();
        manager.SaveDatabase();
        return ReportStatus(status);
    }

    if (command == "validate") {
        std::vector<AssetValidationIssue> issues;
        const Status status = manager.ValidateReferences(&issues);
        for (const AssetValidationIssue& issue : issues) {
            std::printf("  [%s] %s (%s): %s\n", issue.error ? "error" : "warn", issue.path.generic_string().c_str(),
                        issue.id.ToString().c_str(), issue.message.c_str());
        }
        std::printf("validation %s (%zu issues)\n", status ? "passed" : "failed", issues.size());
        return status ? 0 : 1;
    }

    if (command == "list") {
        std::vector<AssetMetadata*> all = manager.Database().All();
        std::printf("%zu assets:\n", all.size());
        for (const AssetMetadata* meta : all) {
            std::printf("  %-21s %-9s %-8s %s\n", meta->id.ToString().c_str(), AssetTypeToString(meta->type).c_str(),
                        StatusLabel(*meta), meta->rawPath.generic_string().c_str());
        }
        return 0;
    }

    if (command == "print") {
        if (args.empty()) {
            std::fprintf(stderr, "print requires an asset id\n");
            return 1;
        }
        const AssetID id = ParseAssetID(args.front());
        const AssetMetadata* meta = manager.Database().Find(id);
        if (meta == nullptr) {
            std::fprintf(stderr, "no asset with id: %s\n", args.front().c_str());
            return 1;
        }
        PrintMetadata(*meta);
        return 0;
    }

    std::fprintf(stderr, "unknown command: %s\n", command.c_str());
    PrintAssetToolUsage();
    return 1;
}

} // namespace Hockey
