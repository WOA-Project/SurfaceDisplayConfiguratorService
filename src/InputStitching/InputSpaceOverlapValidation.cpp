#include "pch.h"

#include "InputStitching\InputSpaceOverlapValidation.h"

#include <algorithm>

#include "InputStitching\Plan.h"

namespace InputStitching {

bool
GetRegionIntersection(const RECT &Left, const RECT &Right, RECT *Intersection)
{
    RECT intersection{};

    intersection.left = std::max(Left.left, Right.left);

    intersection.top = std::max(Left.top, Right.top);

    intersection.right = std::min(Left.right, Right.right);

    intersection.bottom = std::min(Left.bottom, Right.bottom);

    //
    // Touching edges are not overlap.
    //
    if (intersection.right <= intersection.left || intersection.bottom <= intersection.top)
    {
        return false;
    }

    if (Intersection)
    {
        *Intersection = intersection;
    }

    return true;
}

bool
RegionsOverlap(const RECT &Left, const RECT &Right)
{
    return GetRegionIntersection(Left, Right, nullptr);
}

bool
RegionHasOverlap(const Plan &PlanData, size_t RegionIndex, std::wstring *Reason)
{
    if (RegionIndex >= PlanData.Regions.size())
    {
        return false;
    }

    const auto &current = PlanData.Regions[RegionIndex];

    for (size_t i = 0; i < PlanData.Regions.size(); ++i)
    {
        if (i == RegionIndex)
        {
            continue;
        }

        const auto &other = PlanData.Regions[i];

        RECT intersection{};

        if (!GetRegionIntersection(current.Rect, other.Rect, &intersection))
        {
            continue;
        }

        if (Reason)
        {
            *Reason = L"Region overlap detected.";
        }

        return true;
    }

    return false;
}

bool
ValidateDisplayRegions(const Plan &PlanData, std::wstring *Reason)
{
    for (size_t i = 0; i < PlanData.Metadata.size(); ++i)
    {
        const auto &leftMeta = PlanData.Metadata[i];

        if (leftMeta.Role != RegionRole::Display)
        {
            continue;
        }

        for (size_t j = i + 1; j < PlanData.Metadata.size(); ++j)
        {
            const auto &rightMeta = PlanData.Metadata[j];

            if (rightMeta.Role != RegionRole::Display)
            {
                continue;
            }

            if (!RegionsOverlap(leftMeta.DesktopRect, rightMeta.DesktopRect))
            {
                continue;
            }

            if (Reason)
            {
                *Reason = L"Display regions overlap.";
            }

            return false;
        }
    }

    return true;
}

bool
ValidateCustomRegions(const Plan &PlanData, std::wstring *Reason)
{
    for (size_t i = 0; i < PlanData.Metadata.size(); ++i)
    {
        const auto &custom = PlanData.Metadata[i];

        if (custom.Role != RegionRole::Gap)
        {
            continue;
        }

        for (size_t j = 0; j < PlanData.Metadata.size(); ++j)
        {
            if (i == j)
            {
                continue;
            }

            const auto &other = PlanData.Metadata[j];

            if (RegionsOverlap(custom.DesktopRect, other.DesktopRect))
            {
                if (Reason)
                {
                    *Reason = L"Custom region overlaps another region.";
                }

                return false;
            }
        }
    }

    return true;
}

bool
ValidateTopologyContinuity(const Plan &PlanData, std::wstring *Reason)
{
    UNREFERENCED_PARAMETER(Reason);

    //
    // Future:
    //
    // Detect disconnected islands,
    // invalid stitched layouts,
    // unsupported arrangements, etc.
    //
    return true;
}

bool
ValidateUniqueRegionGeometry(const Plan &PlanData, std::wstring *Reason)
{
    for (size_t i = 0; i < PlanData.Metadata.size(); ++i)
    {
        const auto &left = PlanData.Metadata[i];

        for (size_t j = i + 1; j < PlanData.Metadata.size(); ++j)
        {
            const auto &right = PlanData.Metadata[j];

            if (left.DesktopRect.left == right.DesktopRect.left && left.DesktopRect.top == right.DesktopRect.top &&
                left.DesktopRect.right == right.DesktopRect.right &&
                left.DesktopRect.bottom == right.DesktopRect.bottom)
            {
                if (Reason)
                {
                    *Reason = L"Duplicate region geometry detected.";
                }

                return false;
            }
        }
    }

    return true;
}

bool
ValidateNoRegionOverlap(const Plan &PlanData, std::wstring *Reason)
{
    if (!ValidateUniqueRegionGeometry(PlanData, Reason))
    {
        return false;
    }

    if (!ValidateDisplayRegions(PlanData, Reason))
    {
        return false;
    }

    if (!ValidateCustomRegions(PlanData, Reason))
    {
        return false;
    }

    if (!ValidateTopologyContinuity(PlanData, Reason))
    {
        return false;
    }

    return true;
}

} // namespace InputStitching