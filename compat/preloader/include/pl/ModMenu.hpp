#pragma once
#include <cstdint>
#include <functional>
#include <span>
#include <string>
#include <string_view>
#include <vector>
#include "pl/Export.hpp"

namespace pl::modmenu {

enum class ConfigType : int { Toggle, SliderInt, SliderFloat, Radio, Color, Keybind, Text, Button };

enum class DrawCommandType : int { Text, Rect, Line, RectFilled, CircleFilled, TriangleFilled, Image };

struct HudSurfaceSize { float width{}; float height{}; };

struct ConfigEntry {
    std::string key;
    std::string displayName;
    ConfigType type{};
    std::string defaultValue;
    std::string minValue;
    std::string maxValue;
    std::string dependsOn;
};

struct ModuleInfo {
    std::string moduleId;
    std::string displayName;
    std::string description;
    std::string modId;
    bool defaultEnabled{};
    bool hideInHudEditor{};
    std::vector<ConfigEntry> configs;
    std::function<void(std::string_view moduleId, bool enabled)> onToggle;
    std::function<void(std::string_view moduleId, std::string_view key, std::string_view value)> onConfigChanged;
    std::function<void(std::string_view moduleId, std::string_view key, bool isDown)> onKeybind;
};

struct DrawCommand {
    DrawCommandType type{};
    float x{};
    float y{};
    float w{};
    float h{};
    float x3{};
    float y3{};
    uint32_t color{};
    float size{};
    std::string text;
    std::string fontId;
    std::string imageId;
};

PL_EXPORT bool registerModule(const ModuleInfo& info);
PL_EXPORT void unregisterModule(std::string_view moduleId);
PL_EXPORT void submitDrawCommands(std::string_view moduleId, std::span<const DrawCommand> commands);
PL_EXPORT HudSurfaceSize getHudSurfaceSize();
PL_EXPORT bool registerFont(std::string_view fontId, std::span<const unsigned char> ttfData);

}
