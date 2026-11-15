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
        PlanData->Arrangement = DetermineArrangement(*leftPanel, *rightPanel);
    }

    return PlanData->Regions.empty() ? HRESULT_FROM_WIN32(ERROR_NOT_FOUND) : S_OK;
}

static void
UpdateRegionRect(Plan *plan, const RECT &original, const RECT &replacement)
{
    for (size_t i = 0; i < plan->Regions.size(); ++i)
    {
        auto &region = plan->Regions[i];

        if (EqualRect(&region.Rect, &original))
        {
            region.Rect = replacement;

            if (i < plan->Metadata.size())
            {
                plan->Metadata[i].DesktopRect = replacement;
            }

            return;
        }
    }
}

HRESULT
InputSpaceBuilder::BuildGapRegion(const InputSpacePolicy &Policy, Plan *PlanData)
{
    if (!PlanData)
    {
        return E_POINTER;
    }

    auto leftPanel = PlanData->FindPanel(InternalPanelRole::LeftPanel);
    auto rightPanel = PlanData->FindPanel(InternalPanelRole::RightPanel);
    if (!leftPanel || !rightPanel)
    {
        return S_FALSE;
    }

    const RECT a = leftPanel->DesktopRect;
    const RECT b = rightPanel->DesktopRect;
    const LONG centerAX = (a.left + a.right) / 2;
    const LONG centerAY = (a.top + a.bottom) / 2;
    const LONG centerBX = (b.left + b.right) / 2;
    const LONG centerBY = (b.top + b.bottom) / 2;
    const LONG dx = centerBX - centerAX;
    const LONG dy = centerBY - centerAY;

    //
    // Determine dominant arrangement axis.
    //
    const bool horizontalArrangement = std::abs(dx) >= std::abs(dy);

    LONG gapWidth = 0;

    INPUT_SPACE_REGION gap{};
    gap.Flags = CustomRegion();

    if (horizontalArrangement)
    {
        const RECT &visualLeft = (a.left <= b.left) ? a : b;
        const RECT &visualRight = (a.left <= b.left) ? b : a;
        const LONG overlapTop = std::max(visualLeft.top, visualRight.top);
        const LONG overlapBottom = std::min(visualLeft.bottom, visualRight.bottom);
        if (overlapBottom <= overlapTop)
        {
            return S_FALSE;
        }

        const LONG desktopGap = visualRight.left - visualLeft.right;
        if (desktopGap < 0)
        {
            PlanData->Debug.Log += L"Panels overlap horizontally.\n";
            return S_FALSE;
        }

        gapWidth = Policy.GapPixels > 0 ? Policy.GapPixels : desktopGap;
        gap.Rect = {visualLeft.right, overlapTop, visualLeft.right + gapWidth, overlapBottom};

        //
        // Shift visually-right panel.
        //
        RECT shifted = visualRight;
        shifted.left += gapWidth;
        shifted.right += gapWidth;

        UpdateRegionRect(PlanData, visualRight, shifted);
    }
    else
    {
        const RECT &visualTop = (a.top <= b.top) ? a : b;
        const RECT &visualBottom = (a.top <= b.top) ? b : a;
        const LONG overlapLeft = std::max(visualTop.left, visualBottom.left);
        const LONG overlapRight = std::min(visualTop.right, visualBottom.right);
        if (overlapRight <= overlapLeft)
        {
            return S_FALSE;
        }

        const LONG desktopGap = visualBottom.top - visualTop.bottom;
        if (desktopGap < 0)
        {
            PlanData->Debug.Log += L"Panels overlap vertically.\n";

            return S_FALSE;
        }

        gapWidth = Policy.GapPixels > 0 ? Policy.GapPixels : desktopGap;
        gap.Rect = {overlapLeft, visualTop.bottom, overlapRight, visualTop.bottom + gapWidth};

        //
        // Shift visually-bottom panel.
        //
        RECT shifted = visualBottom;

        shifted.top += gapWidth;
        shifted.bottom += gapWidth;

        UpdateRegionRect(PlanData, visualBottom, shifted);
    }

    PlanData->Regions.push_back(gap);

    RegionMetadata metadata{};
    metadata.Role = RegionRole::Gap;
    metadata.Name = L"Hinge Gap";
    metadata.DesktopRect = gap.Rect;

    PlanData->Metadata.push_back(metadata);

    PlanData->GapEnabled = true;
    PlanData->GapPixels = gapWidth;
    PlanData->Stitched = true;

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

    if (PlanData.Arrangement != DeviceArrangement::LandscapeDual)
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

DeviceArrangement
InputSpaceBuilder::DetermineArrangement(const DisplayEntry &Left, const DisplayEntry &Right)
{
    //
    // Tent:
    // 90° / 270° opposing rotations.
    //
    if ((Left.Region.Rotation == DISPLAYCONFIG_ROTATION_ROTATE90 &&
         Right.Region.Rotation == DISPLAYCONFIG_ROTATION_ROTATE270) ||

        (Left.Region.Rotation == DISPLAYCONFIG_ROTATION_ROTATE270 &&
         Right.Region.Rotation == DISPLAYCONFIG_ROTATION_ROTATE90))
    {
        return DeviceArrangement::Tent;
    }

    LONG width =
        abs(((Left.Region.Rect.left + Left.Region.Rect.right) / 2) -
            ((Right.Region.Rect.left + Right.Region.Rect.right) / 2));

    LONG height =
        abs(((Left.Region.Rect.top + Left.Region.Rect.bottom) / 2) -
            ((Right.Region.Rect.top + Right.Region.Rect.bottom) / 2));

    if (width > height)
    {
        return DeviceArrangement::LandscapeDual;
    }

    return DeviceArrangement::PortraitDual;
}

} // namespace InputStitching