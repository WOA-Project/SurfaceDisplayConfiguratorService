#include "pch.h"

#include <InputStitching\InputSpaceBuilder.h>

#include <algorithm>

#include <InputStitching\InputSpaceDisplayEnumerator.h>
#include <InputStitching\InputSpaceFlags.h>
#include <InputStitching\Plan.h>

namespace InputStitching {

HRESULT
InputSpaceBuilder::BuildPlan(const InputSpacePolicy &Policy, Plan *PlanData)
{
    if (!PlanData)
    {
        return E_POINTER;
    }

    PlanData->Reset();

    static LONG g_InputSpaceId = 0;

    PlanData->Space.Id.LowPart = InterlockedIncrement(&g_InputSpaceId);

    PlanData->Space.Id.HighPart = GetCurrentProcessId();

    PlanData->Space.Reserved = 0;

    PlanData->Mode = Policy.Mode;

    switch (Policy.Mode)
    {
    case InputTopologyMode::Legacy:

        return GenerateLegacyPlan(Policy, PlanData);

    case InputTopologyMode::DualIndependent:

        return GenerateDualIndependentPlan(Policy, PlanData);

    case InputTopologyMode::DualStitched:

        return GenerateDualStitchedPlan(Policy, PlanData);

    case InputTopologyMode::SingleLeft:

        return GenerateSingleLeftPlan(Policy, PlanData);

    case InputTopologyMode::SingleRight:

        return GenerateSingleRightPlan(Policy, PlanData);

    case InputTopologyMode::Custom:
    default:

        return E_NOTIMPL;
    }
}

HRESULT
InputSpaceBuilder::GenerateLegacyPlan(const InputSpacePolicy &Policy, Plan *PlanData)
{
    return GenerateDualIndependentPlan(Policy, PlanData);
}

HRESULT
InputSpaceBuilder::GenerateDualIndependentPlan(const InputSpacePolicy &Policy, Plan *PlanData)
{
    HRESULT hr = BuildDisplayRegions(Policy, PlanData);

    if (FAILED(hr))
    {
        return hr;
    }

    PlanData->Stitched = false;

    return S_OK;
}

HRESULT
InputSpaceBuilder::GenerateDualStitchedPlan(const InputSpacePolicy &Policy, Plan *PlanData)
{
    HRESULT hr = BuildDisplayRegions(Policy, PlanData);

    if (FAILED(hr))
    {
        return hr;
    }

    if (!ShouldEnableStitching(Policy, *PlanData))
    {
        //
        // Graceful degradation.
        //
        PlanData->Debug.Log += L"Stitching unavailable; "
                               L"falling back to dual-independent mode.\n";

        PlanData->Stitched = false;

        return S_OK;
    }

    hr = BuildGapRegion(Policy, PlanData);

    return hr;
}

HRESULT
InputSpaceBuilder::GenerateSingleLeftPlan(const InputSpacePolicy &Policy, Plan *PlanData)
{
    std::vector<DisplayEntry> displays;

    HRESULT hr = EnumerateDisplays(&displays);

    if (FAILED(hr))
    {
        return hr;
    }

    for (auto const &display : displays)
    {
        if (display.Role != InternalPanelRole::LeftPanel)
        {
            continue;
        }

        PlanData->Regions.push_back(display.Region);

        RegionMetadata metadata{};

        metadata.Role = RegionRole::Display;

        metadata.PanelRole = display.Role;

        metadata.Name = display.Name;

        metadata.PanelId = display.PanelId;

        metadata.IsInternal = display.Internal;

        metadata.IsPrimary = IsPrimaryRegion(display.Region.Flags);

        metadata.DesktopRect = display.Region.Rect;

        PlanData->Metadata.push_back(metadata);

        PlanData->DisplayCount = 1;
        PlanData->InternalDisplayCount = 1;
        PlanData->Topology = InternalPanelTopology::Single;

        return S_OK;
    }

    return HRESULT_FROM_WIN32(ERROR_NOT_FOUND);
}

HRESULT
InputSpaceBuilder::GenerateSingleRightPlan(const InputSpacePolicy &Policy, Plan *PlanData)
{
    std::vector<DisplayEntry> displays;

    UNREFERENCED_PARAMETER(Policy);

    HRESULT hr = EnumerateDisplays(&displays);

    if (FAILED(hr))
    {
        return hr;
    }

    for (auto const &display : displays)
    {
        if (display.Role != InternalPanelRole::RightPanel)
        {
            continue;
        }

        PlanData->Regions.push_back(display.Region);

        RegionMetadata metadata{};

        metadata.Role = RegionRole::Display;

        metadata.PanelRole = display.Role;

        metadata.Name = display.Name;

        metadata.PanelId = display.PanelId;

        metadata.IsInternal = display.Internal;

        metadata.IsPrimary = IsPrimaryRegion(display.Region.Flags);

        metadata.DesktopRect = display.Region.Rect;

        PlanData->Metadata.push_back(metadata);

        PlanData->DisplayCount = 1;
        PlanData->InternalDisplayCount = 1;
        PlanData->Topology = InternalPanelTopology::Single;

        return S_OK;
    }

    return HRESULT_FROM_WIN32(ERROR_NOT_FOUND);
}

HRESULT
InputSpaceBuilder::BuildDisplayRegions(const InputSpacePolicy &Policy, Plan *PlanData)
{
    std::vector<DisplayEntry> displays;

    HRESULT hr = EnumerateDisplays(&displays);

    if (FAILED(hr))
    {
        return hr;
    }

    const DisplayEntry *leftPanel = nullptr;
    const DisplayEntry *rightPanel = nullptr;

    for (auto const &display : displays)
    {
        switch (display.Role)
        {
        case InternalPanelRole::LeftPanel:

            leftPanel = &display;
            break;

        case InternalPanelRole::RightPanel:

            rightPanel = &display;
            break;

        default:

            break;
        }

        PlanData->Regions.push_back(display.Region);

        RegionMetadata metadata{};

        metadata.Role = RegionRole::Display;

        metadata.PanelRole = display.Role;

        metadata.Name = display.Name;

        metadata.PanelId = display.PanelId;

        metadata.IsInternal = display.Internal;

        metadata.IsPrimary = IsPrimaryRegion(display.Region.Flags);

        metadata.DesktopRect = display.Region.Rect;

        PlanData->Metadata.push_back(metadata);

        ++PlanData->DisplayCount;

        if (display.Internal)
        {
            ++PlanData->InternalDisplayCount;
        }
    }

    PlanData->Topology = DetermineTopology(*PlanData);

    if (leftPanel && rightPanel)
    {
        PlanData->Arrangement = DetermineArrangement(leftPanel->Region.Rect, rightPanel->Region.Rect);
    }

    return PlanData->Regions.empty() ? HRESULT_FROM_WIN32(ERROR_NOT_FOUND) : S_OK;
}

/*HRESULT
InputSpaceBuilder::BuildGapRegion(const InputSpacePolicy &Policy, Plan *PlanData)
{
    if (!PlanData)
    {
        return E_POINTER;
    }

    auto left = PlanData->FindPanel(InternalPanelRole::LeftPanel);

    auto right = PlanData->FindPanel(InternalPanelRole::RightPanel);

    if (!left)
    {
        PlanData->Debug.Log += L"Left panel unavailable.\n";

        return S_FALSE;
    }

    if (!right)
    {
        PlanData->Debug.Log += L"Right panel unavailable.\n";

        return S_FALSE;
    }

    const LONG overlapTop = std::max(left->DesktopRect.top, right->DesktopRect.top);

    const LONG overlapBottom = std::min(left->DesktopRect.bottom, right->DesktopRect.bottom);

    if (overlapBottom <= overlapTop)
    {
        return S_FALSE;
    }

    LONG desktopGap = right->DesktopRect.left - left->DesktopRect.right;

    if (desktopGap < 0)
    {
        PlanData->Debug.Log += L"Panels overlap; gap disabled.\n";

        return S_FALSE;
    }

    LONG gapWidth = Policy.GapPixels > 0 ? Policy.GapPixels : desktopGap;

    INPUT_SPACE_REGION gap{};

    gap.Flags = CustomRegion();

    gap.Rect = {left->DesktopRect.right, overlapTop, left->DesktopRect.right + gapWidth, overlapBottom};

    PlanData->Regions.push_back(gap);

    RegionMetadata gapMetadata{};

    gapMetadata.Role = RegionRole::Gap;

    gapMetadata.Name = L"Hinge Gap";

    gapMetadata.IsInternal = false;

    gapMetadata.DesktopRect = gap.Rect;

    PlanData->Metadata.push_back(gapMetadata);

    PlanData->GapEnabled = true;
    PlanData->GapPixels = gapWidth;
    PlanData->Stitched = true;

    return S_OK;
}*/

HRESULT
InputSpaceBuilder::BuildGapRegion(const InputSpacePolicy &Policy, Plan *PlanData)
{
    if (!PlanData)
    {
        return E_POINTER;
    }

    auto left = PlanData->FindPanel(InternalPanelRole::LeftPanel);

    auto right = PlanData->FindPanel(InternalPanelRole::RightPanel);

    if (!left)
    {
        PlanData->Debug.Log += L"Left panel unavailable.\n";

        return S_FALSE;
    }

    if (!right)
    {
        PlanData->Debug.Log += L"Right panel unavailable.\n";

        return S_FALSE;
    }

    //
    // Capture original geometry.
    //
    const RECT leftRect = left->DesktopRect;

    const RECT rightRect = right->DesktopRect;

    const LONG overlapTop = std::max(leftRect.top, rightRect.top);

    const LONG overlapBottom = std::min(leftRect.bottom, rightRect.bottom);

    if (overlapBottom <= overlapTop)
    {
        return S_FALSE;
    }

    LONG desktopGap = rightRect.left - leftRect.right;

    if (desktopGap < 0)
    {
        PlanData->Debug.Log += L"Panels overlap; gap disabled.\n";

        return S_FALSE;
    }

    LONG gapWidth = Policy.GapPixels > 0 ? Policy.GapPixels : desktopGap;

    //
    // Gap geometry is based on ORIGINAL
    // panel coordinates.
    //
    const LONG gapLeft = leftRect.right;

    const LONG gapRight = gapLeft + gapWidth;

    //
    // Shift the right display region to
    // make room for the hinge gap.
    //
    for (auto &region : PlanData->Regions)
    {
        if (!IsDisplayRegion(region.Flags))
        {
            continue;
        }

        if (region.Rect.left == rightRect.left && region.Rect.top == rightRect.top &&
            region.Rect.right == rightRect.right && region.Rect.bottom == rightRect.bottom)
        {
            region.Rect.left += gapWidth;
            region.Rect.right += gapWidth;

            break;
        }
    }

    for (auto &metadata : PlanData->Metadata)
    {
        if (metadata.PanelRole == InternalPanelRole::RightPanel)
        {
            metadata.DesktopRect.left += gapWidth;
            metadata.DesktopRect.right += gapWidth;

            break;
        }
    }

    //
    // Create the hinge gap region using
    // the original geometry.
    //
    INPUT_SPACE_REGION gap{};

    gap.Flags = CustomRegion();

    gap.Rect = {gapLeft, overlapTop, gapRight, overlapBottom};

    PlanData->Regions.push_back(gap);

    RegionMetadata gapMetadata{};

    gapMetadata.Role = RegionRole::Gap;

    gapMetadata.Name = L"Hinge Gap";

    gapMetadata.IsInternal = false;

    gapMetadata.DesktopRect = gap.Rect;

    PlanData->Metadata.push_back(gapMetadata);

    PlanData->GapEnabled = true;
    PlanData->GapPixels = gapWidth;
    PlanData->Stitched = true;

    PlanData->Debug.Log += L"Inserted hinge gap and shifted right panel.\n";

    return S_OK;
}

bool
InputSpaceBuilder::ShouldEnableStitching(const InputSpacePolicy &Policy, const Plan &PlanData) const
{
    if (!Policy.EnableStitching)
    {
        return false;
    }

    if (!Policy.EnableGapRegion)
    {
        return false;
    }

    if (PlanData.Topology != InternalPanelTopology::Dual)
    {
        return false;
    }

    if (PlanData.Arrangement != PanelArrangement::Horizontal)
    {
        return false;
    }

    return true;
}

InternalPanelTopology
InputSpaceBuilder::DetermineTopology(const Plan &PlanData) const
{
    if (PlanData.InternalDisplayCount >= 2)
    {
        return InternalPanelTopology::Dual;
    }

    if (PlanData.InternalDisplayCount == 1)
    {
        return InternalPanelTopology::Single;
    }

    return InternalPanelTopology::None;
}

PanelArrangement
InputSpaceBuilder::DetermineArrangement(const RECT &Left, const RECT &Right) const
{
    LONG overlapX = std::min(Left.right, Right.right) - std::max(Left.left, Right.left);

    LONG overlapY = std::min(Left.bottom, Right.bottom) - std::max(Left.top, Right.top);

    if (overlapY > 0)
    {
        return PanelArrangement::Horizontal;
    }

    if (overlapX > 0)
    {
        return PanelArrangement::Vertical;
    }

    return PanelArrangement::Unknown;
}

} // namespace InputStitching