#include "Analyzer.hpp"
#include "Offsets.hpp"
#include "Signatures.hpp"
#include <pl/Memory.hpp>
#include <pl/Mod.hpp>
#include <pl/ModMenu.hpp>
#include <algorithm>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <iterator>
#include <sstream>
#include <string_view>
#include <utility>
#include <vector>

namespace {
using TickFn = void (*)(void*);
using StartDestroyFn = bool (*)(void*, const void*, std::uint8_t, bool*);
using StopDestroyFn = void (*)(void*, const void*);
using GetBlockFn = const void* (*)(void*, const void*, std::int32_t);
using RenderLevelFn = void (*)(void*, void*, void*);

TickFn gTickOriginal = nullptr;
StartDestroyFn gStartDestroyOriginal = nullptr;
StopDestroyFn gStopDestroyOriginal = nullptr;
GetBlockFn gGetBlock = nullptr;
RenderLevelFn gRenderLevelOriginal = nullptr;

void* gTickTarget = nullptr;
void* gStartTarget = nullptr;
void* gStopTarget = nullptr;
void* gRenderTarget = nullptr;

bool plausiblePtr(const void* ptr) {
    return reinterpret_cast<std::uintptr_t>(ptr) >= 0x10000ull;
}

std::string_view blockShortName(std::string_view id) {
    const auto pos = id.find_last_of(':');
    return pos == std::string_view::npos ? id : id.substr(pos + 1);
}

std::string makeDisplayName(std::string_view id) {
    id = blockShortName(id);
    std::string out;
    out.reserve(id.size());
    bool capitalize = true;
    for (const char c : id) {
        if (c == '_') {
            out.push_back(' ');
            capitalize = true;
        } else if (capitalize && c >= 'a' && c <= 'z') {
            out.push_back(static_cast<char>(c - 'a' + 'A'));
            capitalize = false;
        } else {
            out.push_back(c);
            capitalize = false;
        }
    }
    return out.empty() ? "Unknown Block" : out;
}

std::string requiredTool(std::string_view id) {
    const auto n = blockShortName(id);
    const auto has = [&](std::string_view token) { return n.find(token) != std::string_view::npos; };
    if (has("concrete_powder")) return "Shovel";
    if (has("leaves") || has("cobweb") || has("vine") || has("wool")) return "Shears";
    if (has("log") || has("wood") || has("planks") || has("stem") || has("hyphae") || has("bamboo")) return "Axe";
    if (has("sand") || has("gravel") || has("dirt") || has("grass_block") || has("clay") || has("snow") || has("soul_soil") || has("soul_sand") || has("mud") || has("farmland")) return "Shovel";
    if (has("hay_block") || has("target") || has("wart_block") || has("sponge") || has("sculk")) return "Hoe";
    if (has("stone") || has("cobblestone") || has("deepslate") || has("ore") || has("brick") || has("bricks") || has("obsidian") || has("ancient_debris") || has("basalt") || has("andesite") || has("diorite") || has("granite") || has("tuff") || has("terracotta") || has("rail") || has("anvil") || has("iron_block") || has("gold_block") || has("copper_block") || has("redstone_block") || has("quartz_block") || has("prismarine")) return "Pickaxe";
    return "None";
}

bool readBlockName(const void* block, std::string& out) {
    if (!plausiblePtr(block)) return false;
    const auto blockBytes = reinterpret_cast<const std::byte*>(block);
    const auto blockType = *reinterpret_cast<const void* const*>(blockBytes + bba::offset::BlockType);
    if (!plausiblePtr(blockType)) return false;
    const auto nameInfo = reinterpret_cast<const std::byte*>(blockType) + bba::offset::BlockTypeNameInfo;
    const auto fullName = nameInfo + bba::offset::NameInfoFullName;
    const auto* stringObject = reinterpret_cast<const std::string*>(fullName + bba::offset::HashedString);
    if (!plausiblePtr(stringObject)) return false;
    try {
        const std::string& name = *stringObject;
        if (name.empty() || name.size() > 256) return false;
        out = name;
        return true;
    } catch (...) {
        return false;
    }
}

std::string formatEta(float seconds) {
    if (!(seconds > 0.0f) || !std::isfinite(seconds)) return "Calculating...";
    std::ostringstream ss;
    if (seconds < 10.0f) {
        ss << std::fixed << std::setprecision(1) << seconds;
    } else {
        ss << std::fixed << std::setprecision(0) << seconds;
    }
    ss << 's';
    return ss.str();
}

void tickDetour(void* player) {
    gBlockBreakAnalyzer.onTick(player);
    if (gTickOriginal) gTickOriginal(player);
}

bool startDestroyDetour(void* gameMode, const void* position, std::uint8_t face, bool* destroyed) {
    struct BlockPosCopy { int x; int y; int z; } copy{};
    if (position) {
        const auto* p = reinterpret_cast<const int*>(position);
        copy = {p[0], p[1], p[2]};
    }
    const bool result = gStartDestroyOriginal ? gStartDestroyOriginal(gameMode, position, face, destroyed) : false;
    if (position) gBlockBreakAnalyzer.onStartDestroy(gameMode, &copy);
    return result;
}

void stopDestroyDetour(void* gameMode, const void* position) {
    if (gStopDestroyOriginal) gStopDestroyOriginal(gameMode, position);
    gBlockBreakAnalyzer.onStopDestroy(gameMode);
}

void renderLevelDetour(void* renderer, void* screenContext, void* arg3) {
    if (gRenderLevelOriginal) gRenderLevelOriginal(renderer, screenContext, arg3);
    gBlockBreakAnalyzer.onRender(renderer, screenContext, arg3);
}
}

BlockBreakAnalyzer gBlockBreakAnalyzer;

bool BlockBreakAnalyzer::load(pl::mod::ModContext& context) {
    const auto fontPath = context.resourceDir() / "minecraft.ttf";
    std::ifstream fontFile(fontPath, std::ios::binary);
    if (!fontFile) return false;
    std::vector<unsigned char> font((std::istreambuf_iterator<char>(fontFile)), std::istreambuf_iterator<char>());
    if (font.empty() || !pl::modmenu::registerFont("minecraft", font)) return false;

    pl::modmenu::ModuleInfo info{};
    info.moduleId = mModuleId;
    info.displayName = "Block Break Analyzer";
    info.description = "Shows the targeted block, required tool, and a live break-time estimate.";
    info.modId = mModuleId;
    info.defaultEnabled = true;
    info.hideInHudEditor = true;
    info.onToggle = [](std::string_view, bool enabled) { gBlockBreakAnalyzer.onToggle(enabled); };
    if (!pl::modmenu::registerModule(info)) return false;

    if (!resolveAndInstall()) {
        pl::modmenu::unregisterModule(mModuleId);
        return false;
    }
    return true;
}

bool BlockBreakAnalyzer::resolveAndInstall() {
    gTickTarget = reinterpret_cast<void*>(pl::memory::resolveSignature(bba::sig::NormalTick, bba::sig::module));
    gStartTarget = reinterpret_cast<void*>(pl::memory::resolveSignature(bba::sig::GameModeStartDestroyBlock, bba::sig::module));
    gStopTarget = reinterpret_cast<void*>(pl::memory::resolveSignature(bba::sig::GameModeStopDestroyBlock, bba::sig::module));
    gRenderTarget = reinterpret_cast<void*>(pl::memory::resolveSignature(bba::sig::RenderLevel, bba::sig::module));
    const auto block = pl::memory::resolveSignature(bba::sig::BlockSourceGetBlock, bba::sig::module);
    if (!gTickTarget || !gStartTarget || !gStopTarget || !gRenderTarget || !block) return false;
    gGetBlock = reinterpret_cast<GetBlockFn>(block);

    auto rollback = [&]() {
        if (gRenderTarget && gRenderLevelOriginal) pl::memory::unhook(gRenderTarget, reinterpret_cast<void*>(&renderLevelDetour));
        if (gStopTarget && gStopDestroyOriginal) pl::memory::unhook(gStopTarget, reinterpret_cast<void*>(&stopDestroyDetour));
        if (gStartTarget && gStartDestroyOriginal) pl::memory::unhook(gStartTarget, reinterpret_cast<void*>(&startDestroyDetour));
        if (gTickTarget && gTickOriginal) pl::memory::unhook(gTickTarget, reinterpret_cast<void*>(&tickDetour));
        gRenderLevelOriginal = nullptr;
        gStopDestroyOriginal = nullptr;
        gStartDestroyOriginal = nullptr;
        gTickOriginal = nullptr;
    };

    if (pl::memory::hook(gTickTarget, reinterpret_cast<void*>(&tickDetour), reinterpret_cast<void**>(&gTickOriginal)) != 0) return false;
    if (pl::memory::hook(gStartTarget, reinterpret_cast<void*>(&startDestroyDetour), reinterpret_cast<void**>(&gStartDestroyOriginal)) != 0) { rollback(); return false; }
    if (pl::memory::hook(gStopTarget, reinterpret_cast<void*>(&stopDestroyDetour), reinterpret_cast<void**>(&gStopDestroyOriginal)) != 0) { rollback(); return false; }
    if (pl::memory::hook(gRenderTarget, reinterpret_cast<void*>(&renderLevelDetour), reinterpret_cast<void**>(&gRenderLevelOriginal)) != 0) { rollback(); return false; }
    mHooksInstalled.store(true, std::memory_order_release);
    return true;
}

void BlockBreakAnalyzer::removeHooks() {
    if (gRenderTarget && gRenderLevelOriginal) pl::memory::unhook(gRenderTarget, reinterpret_cast<void*>(&renderLevelDetour));
    if (gStopTarget && gStopDestroyOriginal) pl::memory::unhook(gStopTarget, reinterpret_cast<void*>(&stopDestroyDetour));
    if (gStartTarget && gStartDestroyOriginal) pl::memory::unhook(gStartTarget, reinterpret_cast<void*>(&startDestroyDetour));
    if (gTickTarget && gTickOriginal) pl::memory::unhook(gTickTarget, reinterpret_cast<void*>(&tickDetour));
    gRenderLevelOriginal = nullptr;
    gStopDestroyOriginal = nullptr;
    gStartDestroyOriginal = nullptr;
    gTickOriginal = nullptr;
    gGetBlock = nullptr;
    gTickTarget = nullptr;
    gStartTarget = nullptr;
    gStopTarget = nullptr;
    gRenderTarget = nullptr;
    mHooksInstalled.store(false, std::memory_order_release);
}

bool BlockBreakAnalyzer::enable() {
    mEnabled.store(true, std::memory_order_release);
    return mHooksInstalled.load(std::memory_order_acquire);
}

bool BlockBreakAnalyzer::disable() {
    mEnabled.store(false, std::memory_order_release);
    mGameMode.store(nullptr, std::memory_order_release);
    {
        std::scoped_lock lock(mMutex);
        mSnapshot = {};
        mHasTargetPos = false;
    }
    submitEmpty();
    return true;
}

bool BlockBreakAnalyzer::unload() {
    disable();
    removeHooks();
    pl::modmenu::unregisterModule(mModuleId);
    return true;
}

void BlockBreakAnalyzer::onToggle(bool enabled) {
    mEnabled.store(enabled, std::memory_order_release);
    if (!enabled) submitEmpty();
}

void BlockBreakAnalyzer::onStartDestroy(void* gameMode, const void* blockPosition) {
    if (!mEnabled.load(std::memory_order_acquire) || !plausiblePtr(gameMode) || !blockPosition) return;
    std::scoped_lock lock(mMutex);
    const auto* pos = reinterpret_cast<const int*>(blockPosition);
    mTargetPos = BlockPos{pos[0], pos[1], pos[2]};
    mHasTargetPos = true;
    mGameMode.store(gameMode, std::memory_order_release);
    mBreakStarted = std::chrono::steady_clock::now();
    mLastProgressTime = mBreakStarted;
    mLastProgress = 0.0f;
    mRateEma = 0.0f;
}

void BlockBreakAnalyzer::onStopDestroy(void* gameMode) {
    const auto active = mGameMode.load(std::memory_order_acquire);
    if (!gameMode || active == gameMode) {
        mGameMode.store(nullptr, std::memory_order_release);
        std::scoped_lock lock(mMutex);
        mSnapshot = {};
        mHasTargetPos = false;
    }
}

void BlockBreakAnalyzer::onTick(void* player) {
    if (!mEnabled.load(std::memory_order_acquire) || !plausiblePtr(player)) return;
    mPlayer.store(player, std::memory_order_release);
    if (mGameMode.load(std::memory_order_acquire)) updateSnapshot(player);
}

bool BlockBreakAnalyzer::resolveTargetBlockName(void* player, std::string& name) const {
    if (!plausiblePtr(player) || !gGetBlock) return false;
    BlockPos target{};
    {
        std::scoped_lock lock(mMutex);
        if (!mHasTargetPos) return false;
        target = mTargetPos;
    }

    const auto playerBytes = reinterpret_cast<const std::byte*>(player);
    const auto dimension = *reinterpret_cast<void* const*>(playerBytes + bba::offset::ActorDimension);
    if (!plausiblePtr(dimension)) return false;
    const auto blockSource = *reinterpret_cast<void* const*>(reinterpret_cast<const std::byte*>(dimension) + bba::offset::DimensionBlockSource);
    if (!plausiblePtr(blockSource)) return false;
    const auto* block = gGetBlock(blockSource, &target, 0);
    return readBlockName(block, name);
}

void BlockBreakAnalyzer::updateSnapshot(void* player) {
    std::string fullName;
    if (!resolveTargetBlockName(player, fullName)) {
        std::scoped_lock lock(mMutex);
        mSnapshot = {};
        return;
    }

    Snapshot next{};
    next.valid = true;
    next.displayName = makeDisplayName(fullName);
    next.requiredTool = requiredTool(fullName);

    const auto gm = mGameMode.load(std::memory_order_acquire);
    {
        std::scoped_lock lock(mMutex);
        if (gm && mBreakStarted.time_since_epoch().count() != 0) {
            const float progressRaw = *reinterpret_cast<const float*>(reinterpret_cast<const std::byte*>(gm) + bba::offset::GameModeDestroyProgress);
            if (std::isfinite(progressRaw)) {
                const float progress = std::clamp(progressRaw, 0.0f, 1.0f);
                const auto now = std::chrono::steady_clock::now();
                const float dt = std::chrono::duration<float>(now - mLastProgressTime).count();
                const float dp = progress - mLastProgress;
                if (dt > 0.0f && dp > 0.0005f) {
                    const float rate = dp / dt;
                    mRateEma = mRateEma <= 0.0f ? rate : (mRateEma * 0.75f + rate * 0.25f);
                }
                mLastProgress = progress;
                mLastProgressTime = now;
                if (progress >= 0.08f && mRateEma > 0.0001f) {
                    next.estimatedSeconds = std::clamp((1.0f - progress) / mRateEma, 0.05f, 120.0f);
                }
            }
        }
        mSnapshot = std::move(next);
    }
}

void BlockBreakAnalyzer::submitEmpty() const {
    pl::modmenu::submitDrawCommands(mModuleId, std::span<const pl::modmenu::DrawCommand>{});
}

void BlockBreakAnalyzer::draw(const Snapshot& snapshot) const {
    if (!snapshot.valid) { submitEmpty(); return; }
    const auto surface = pl::modmenu::getHudSurfaceSize();
    if (!(surface.width > 0.0f && surface.height > 0.0f)) { submitEmpty(); return; }

    const float minSide = std::min(surface.width, surface.height);
    const float scale = std::clamp(minSide / 820.0f, 0.72f, 1.25f);
    const float margin = 12.0f * scale;
    const float width = std::min(310.0f * scale, surface.width - 2.0f * margin);
    const float height = 78.0f * scale;
    const float x = std::max(margin, (surface.width - width) * 0.5f);
    const float y = std::clamp(surface.height * 0.70f, margin, surface.height - height - margin);

    std::vector<pl::modmenu::DrawCommand> commands;
    commands.reserve(4);

    pl::modmenu::DrawCommand bg{};
    bg.type = pl::modmenu::DrawCommandType::RectFilled;
    bg.x = x; bg.y = y; bg.w = width; bg.h = height; bg.x3 = 7.0f * scale; bg.color = 0xB0101010u;
    commands.push_back(std::move(bg));

    auto addText = [&](float py, float size, uint32_t color, std::string text) {
        pl::modmenu::DrawCommand c{};
        c.type = pl::modmenu::DrawCommandType::Text;
        c.x = x + 12.0f * scale;
        c.y = py;
        c.w = width - 24.0f * scale;
        c.h = size * 1.5f;
        c.size = size;
        c.color = color;
        c.text = std::move(text);
        c.fontId = "minecraft";
        commands.push_back(std::move(c));
    };

    addText(y + 9.0f * scale, 18.0f * scale, 0xFFFFFFFFu, snapshot.displayName);
    addText(y + 35.0f * scale, 13.0f * scale, 0xFFF2F2F2u, "Required tool: " + snapshot.requiredTool);
    addText(y + 55.0f * scale, 13.0f * scale, 0xFFE0E0E0u, "Estimated break time: " + formatEta(snapshot.estimatedSeconds));

    pl::modmenu::submitDrawCommands(mModuleId, commands);
}

void BlockBreakAnalyzer::onRender(void*, void*, void*) {
    if (!mEnabled.load(std::memory_order_acquire)) { submitEmpty(); return; }
    Snapshot current;
    {
        std::scoped_lock lock(mMutex);
        current = mSnapshot;
    }
    draw(current);
}

namespace {
bool lifecycleLoad(void* instance, pl::mod::ModContext& context) { return static_cast<BlockBreakAnalyzer*>(instance)->load(context); }
bool lifecycleEnable(void* instance, pl::mod::ModContext& context) { (void)context; return static_cast<BlockBreakAnalyzer*>(instance)->enable(); }
bool lifecycleDisable(void* instance, pl::mod::ModContext& context) { (void)context; return static_cast<BlockBreakAnalyzer*>(instance)->disable(); }
bool lifecycleUnload(void* instance, pl::mod::ModContext& context) { (void)context; return static_cast<BlockBreakAnalyzer*>(instance)->unload(); }
}

extern "C" PL_EXPORT pl::mod::ModRegistration* PLGetModRegistration() {
    static pl::mod::ModRegistration registration{
        &gBlockBreakAnalyzer, lifecycleLoad, lifecycleEnable, lifecycleDisable, lifecycleUnload
    };
    return &registration;
}
