#pragma once

#include <string>

#include <InputStitching\InputSpaceTypes.h>

namespace InputStitching {

struct Plan;

//
// Diagnostic helpers for the Input Space
// subsystem.
//
// These functions generate human-readable
// descriptions of the generated topology,
// regions, stitching state, and publication
// plan. They are intended for:
//
//   Debug logging
//   Test validation
//   HeatCore investigation
//   Field diagnostics
//

//
// Creates a complete diagnostic report
// describing the entire plan.
//
std::wstring
DescribePlan(const Plan &PlanData);

//
// Produces a concise summary suitable for
// logs and telemetry.
//
std::wstring
DescribeSummary(const Plan &PlanData);

//
// Dumps all input-space regions including
// geometry and roles.
//
std::wstring
DumpRegions(const Plan &PlanData);

//
// Dumps topology information.
//
std::wstring
DumpTopology(const Plan &PlanData);

//
// Dumps stitching-related information.
//
std::wstring
DumpStitching(const Plan &PlanData);

//
// Dumps publication metadata.
//
std::wstring
DumpInputSpace(const Plan &PlanData);

//
// Formatting helpers.
//

const wchar_t *
ToString(InputTopologyMode Mode);

const wchar_t *
ToString(RegionRole Role);

const wchar_t *
ToString(InternalPanelRole Role);

const wchar_t *
ToString(InternalPanelTopology Topology);

const wchar_t *
ToString(PanelArrangement Arrangement);

} // namespace InputStitching