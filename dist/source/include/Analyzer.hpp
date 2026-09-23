#pragma once
#include <atomic>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <mutex>
#include <string>

namespace pl::mod { class ModContext; }

class BlockBreakAnalyzer final {
public:
    bool load(pl::mod::ModContext& context);
    bool enable();
    bool disable();
    bool unload();

    void onTick(void* player);
    void onStartDestroy(void* gameMode, const void* blockPosition);
    void onStopDestroy(void* gameMode);
    void onRender(void* renderer, void* screenContext, void* arg3);
    void onToggle(bool enabled);

private:
    struct BlockPos { int x{}; int y{}; int z{}; };
    struct Snapshot {
        bool valid{};
        std::string displayName;
        std::string requiredTool;
        float estimatedSeconds{-1.0f};
    };

    bool resolveAndInstall();
    void removeHooks();
    void updateSnapshot(void* player);
    bool resolveTargetBlockName(void* player, std::string& name) const;
    void submitEmpty() const;
    void draw(const Snapshot& snapshot) const;

    std::atomic<bool> mEnabled{false};
    std::atomic<bool> mHooksInstalled{false};
    std::atomic<void*> mPlayer{nullptr};
    std::atomic<void*> mGameMode{nullptr};

    mutable std::mutex mMutex;
    Snapshot mSnapshot{};
    BlockPos mTargetPos{};
    bool mHasTargetPos{false};
    std::chrono::steady_clock::time_point mBreakStarted{};
    std::chrono::steady_clock::time_point mLastProgressTime{};
    float mLastProgress{0.0f};
    float mRateEma{0.0f};
    std::string mModuleId{"blockbreakanalyzer"};
};

extern BlockBreakAnalyzer gBlockBreakAnalyzer;
