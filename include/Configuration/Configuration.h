#pragma once

#include <Windows.h>

#include "../WindowPlacement/WindowPlacementService.h"

namespace Configuration {
inline constexpr LONG PreviewMargin = 120;

inline constexpr LONG CommitMargin = 120;

inline constexpr WindowPlacementPolicy PlacementPolicy = WindowPlacementPolicy::FitMonitor;
} // namespace Configuration