#pragma once

#include <windows.h>

#include <string>

namespace InputStitching {

struct Plan;

//
// Detects geometric overlap between
// regions inside a generated plan.
//
bool
ValidateNoRegionOverlap(const Plan &PlanData, std::wstring *Reason = nullptr);

//
// Validates that all regions occupy
// unique desktop space.
//
bool
ValidateUniqueRegionGeometry(const Plan &PlanData, std::wstring *Reason = nullptr);

//
// Determines whether two rectangles
// overlap with positive area.
//
bool
RegionsOverlap(const RECT &Left, const RECT &Right);

//
// Computes the intersection of two
// rectangles.
//
bool
GetRegionIntersection(const RECT &Left, const RECT &Right, RECT *Intersection);

//
// Returns true if the specified region
// overlaps any other region.
//
bool
RegionHasOverlap(const Plan &PlanData, size_t RegionIndex, std::wstring *Reason = nullptr);

//
// Validates that all display regions
// are disjoint.
//
bool
ValidateDisplayRegions(const Plan &PlanData, std::wstring *Reason = nullptr);

//
// Validates that custom regions
// (hinge gaps, future bezels, etc.)
// do not overlap display regions.
//
bool
ValidateCustomRegions(const Plan &PlanData, std::wstring *Reason = nullptr);

//
// Validates that a stitched topology
// forms a contiguous layout.
//
bool
ValidateTopologyContinuity(const Plan &PlanData, std::wstring *Reason = nullptr);

} // namespace InputStitching