#include "pch.h"

#include <InputStitching\InputSpaceDiagnostics.h>

#include <sstream>

#include <InputStitching\Plan.h>

namespace InputStitching {

const wchar_t *
ToString(InputTopologyMode Mode)
{
    switch (Mode)
    {
    case InputTopologyMode::Legacy:
        return L"Legacy";

    case InputTopologyMode::DualIndependent:
        return L"DualIndependent";

    case InputTopologyMode::DualStitched:
        return L"DualStitched";

    case InputTopologyMode::SingleLeft:
        return L"SingleLeft";

    case InputTopologyMode::SingleRight:
        return L"SingleRight";

    case InputTopologyMode::Custom:
        return L"Custom";

    default:
        return L"Unknown";
    }
}

const wchar_t *
ToString(RegionRole Role)
{
    switch (Role)
    {
    case RegionRole::Display:
        return L"Display";

    case RegionRole::Gap:
        return L"Gap";

    case RegionRole::Disabled:
        return L"Disabled";

    case RegionRole::TouchOnly:
        return L"TouchOnly";

    case RegionRole::PenOnly:
        return L"PenOnly";

    case RegionRole::Custom:
        return L"Custom";

    default:
        return L"Unknown";
    }
}

const wchar_t *
ToString(InternalPanelRole Role)
{
    switch (Role)
    {
    case InternalPanelRole::LeftPanel:
        return L"LeftPanel";

    case InternalPanelRole::RightPanel:
        return L"RightPanel";

    case InternalPanelRole::External:
        return L"External";

    case InternalPanelRole::Unknown:
    default:

        return L"Unknown";
    }
}

const wchar_t *
ToString(InternalPanelTopology Topology)
{
    switch (Topology)
    {
    case InternalPanelTopology::None:
        return L"None";

    case InternalPanelTopology::Single:
        return L"Single";

    case InternalPanelTopology::Dual:
        return L"Dual";

    default:
        return L"Unknown";
    }
}

const wchar_t *
ToString(DeviceArrangement Arrangement)
{
    switch (Arrangement)
    {
    case DeviceArrangement::LandscapeDual:
        return L"LandscapeDual";

    case DeviceArrangement::PortraitDual:
        return L"PortraitDual";

    case DeviceArrangement::Tent:
        return L"Tent";

    case DeviceArrangement::SingleScreen:
        return L"SingleScreen";

    case DeviceArrangement::Unknown:
    default:

        return L"Unknown";
    }
}

std::wstring
DescribeSummary(const Plan &PlanData)
{
    std::wstringstream stream;

    stream << L"Mode=" << ToString(PlanData.Mode)

           << L", Topology=" << ToString(PlanData.Topology)

           << L", Arrangement=" << ToString(PlanData.Arrangement)

           << L", Regions=" << PlanData.Regions.size()

           << L", Displays=" << PlanData.DisplayCount

           << L", Stitched=" << (PlanData.Stitched ? L"Yes" : L"No")

           << L", Gap=" << (PlanData.GapEnabled ? L"Yes" : L"No");

    return stream.str();
}

std::wstring
DumpTopology(const Plan &PlanData)
{
    std::wstringstream stream;

    stream << L"Topology\r\n"
           << L"----------------------------------------\r\n"

           << L"Mode: " << ToString(PlanData.Mode) << L"\r\n"

           << L"Topology: " << ToString(PlanData.Topology) << L"\r\n"

           << L"Arrangement: " << ToString(PlanData.Arrangement) << L"\r\n"

           << L"Display Count: " << PlanData.DisplayCount << L"\r\n"

           << L"Internal Display Count: " << PlanData.InternalDisplayCount << L"\r\n";

    return stream.str();
}

std::wstring
DumpStitching(const Plan &PlanData)
{
    std::wstringstream stream;

    stream << L"Stitching\r\n"
           << L"----------------------------------------\r\n"

           << L"Enabled: " << (PlanData.Stitched ? L"Yes" : L"No") << L"\r\n"

           << L"Gap Region: " << (PlanData.GapEnabled ? L"Present" : L"Absent") << L"\r\n"

           << L"Gap Width: " << PlanData.GapPixels << L"\r\n";

    return stream.str();
}

std::wstring
DumpInputSpace(const Plan &PlanData)
{
    std::wstringstream stream;

    stream << L"InputSpace\r\n"
           << L"----------------------------------------\r\n"

           << L"Id.LowPart: " << PlanData.Space.Id.LowPart << L"\r\n"

           << L"Id.HighPart: " << PlanData.Space.Id.HighPart << L"\r\n"

           << L"Region Count: " << PlanData.Regions.size() << L"\r\n"

           << L"Published: " << (PlanData.Debug.Published ? L"Yes" : L"No") << L"\r\n";

    return stream.str();
}

std::wstring
DumpRegions(const Plan &PlanData)
{
    std::wstringstream stream;

    stream << L"Regions\r\n" << L"----------------------------------------\r\n";

    for (size_t i = 0; i < PlanData.Metadata.size(); ++i)
    {
        const auto &metadata = PlanData.Metadata[i];

        stream << L"[" << i << L"]\r\n"

               << L"Role: " << ToString(metadata.Role) << L"\r\n"

               << L"PanelRole: " << ToString(metadata.PanelRole) << L"\r\n"

               << L"Name: " << metadata.Name << L"\r\n";

        if (!metadata.PanelId.empty())
        {
            stream << L"PanelId: " << metadata.PanelId << L"\r\n";
        }

        stream << L"Internal: " << (metadata.IsInternal ? L"Yes" : L"No") << L"\r\n"

               << L"Primary: " << (metadata.IsPrimary ? L"Yes" : L"No") << L"\r\n"

               << L"Rect: (" << metadata.DesktopRect.left << L"," << metadata.DesktopRect.top << L")-("
               << metadata.DesktopRect.right << L"," << metadata.DesktopRect.bottom << L")\r\n\r\n";

        if (i < PlanData.Regions.size())
        {
            const auto &region = PlanData.Regions[i];

            stream << L"DesktopRect: (" << region.Rect.left << L"," << region.Rect.top << L")-(" << region.Rect.right
                   << L"," << region.Rect.bottom << L")\r\n"

                   << L"ModeRect: (" << region.ModeRect.left << L"," << region.ModeRect.top << L")-("
                   << region.ModeRect.right << L"," << region.ModeRect.bottom << L")\r\n"

                   << L"ScalingRect: (" << region.ScalingRect.left << L"," << region.ScalingRect.top << L")-("
                   << region.ScalingRect.right << L"," << region.ScalingRect.bottom << L")\r\n"

                   << L"Rotation: " << region.Rotation << L"\r\n"

                   << L"Dpi: " << region.Dpi << L"\r\n";
        }
    }

    return stream.str();
}

std::wstring
DescribePlan(const Plan &PlanData)
{
    std::wstringstream stream;

    stream << L"Input Space Diagnostic Report\r\n"
           << L"========================================\r\n\r\n"

           << DescribeSummary(PlanData) << L"\r\n\r\n"

           << DumpTopology(PlanData) << L"\r\n"

           << DumpStitching(PlanData) << L"\r\n"

           << DumpInputSpace(PlanData) << L"\r\n"

           << DumpRegions(PlanData);

    if (!PlanData.Debug.Log.empty())
    {
        stream << L"Diagnostics\r\n"
               << L"----------------------------------------\r\n"

               << PlanData.Debug.Log << L"\r\n";
    }

    return stream.str();
}

} // namespace InputStitching