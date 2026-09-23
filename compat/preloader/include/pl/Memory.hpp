#pragma once
#include <cstdint>
#include <string_view>
#include "pl/Export.hpp"

namespace pl::memory {
enum class HookPriority : int { Highest = 0, High = 100, Normal = 200, Low = 300, Lowest = 400 };
PL_EXPORT uintptr_t resolveSignature(std::string_view signature, std::string_view moduleName);
PL_EXPORT int hook(void* target, void* detour, void** originalFunc, HookPriority priority = HookPriority::Normal);
PL_EXPORT bool unhook(void* target, void* detour);
}
