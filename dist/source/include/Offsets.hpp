#pragma once
#include <cstddef>

namespace bba::offset {
inline constexpr std::size_t ActorLevel = 0x1D0;
inline constexpr std::size_t ActorDimension = 0x1C0;
inline constexpr std::size_t DimensionBlockSource = 0xD0;
inline constexpr std::size_t LevelHitResultWrapper = 0x1C8;
inline constexpr std::size_t GameModeDestroyProgress = 0x24;
inline constexpr std::size_t HitResultType = 0x18;
inline constexpr std::size_t HitResultPos = 0x2C;
inline constexpr std::size_t BlockType = 0x68;
inline constexpr std::size_t BlockTypeNameInfo = 0x88;
inline constexpr std::size_t NameInfoFullName = 0x40;
inline constexpr std::size_t HashedString = 0x8;
}
