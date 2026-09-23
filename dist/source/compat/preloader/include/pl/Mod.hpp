#pragma once
#include <filesystem>
#include <string>
#include <utility>
#include "pl/Export.hpp"

namespace pl::mod {

struct ModInfo {
    std::string id;
    std::string displayName;
    std::string author;
    std::string version;
    std::filesystem::path entryPath;
    std::string entryFileName;
    std::filesystem::path libraryPath;
    std::filesystem::path iconPath;
    std::filesystem::path manifestPath;
    std::filesystem::path modRootPath;
};

class ModContext {
public:
    [[nodiscard]] std::filesystem::path resourceDir() const { return mInfo.modRootPath / "resources"; }
private:
    void* mJavaVm{};
    ModInfo mInfo;
    void* mLogger{};
};

using LifecycleFunction = bool (*)(void* instance, ModContext& context);

struct ModRegistration {
    void* instance{};
    LifecycleFunction load{};
    LifecycleFunction enable{};
    LifecycleFunction disable{};
    LifecycleFunction unload{};
};

}

extern "C" PL_EXPORT pl::mod::ModRegistration* PLGetModRegistration();
