#include "pch.h"

#include <InputStitching\InputSpaceValidation.h>

#include <InputStitching\InputSpaceFlags.h>
#include <InputStitching\Plan.h>

namespace InputStitching {

HRESULT
ValidatePlan(const Plan &PlanData, std::wstring *Reason)
{
    if (!ValidateLikeWin32k(PlanData.Space, PlanData.Regions, Reason))
    {
        return E_INVALIDARG;
    }

    return S_OK;
}

bool
ValidateLikeWin32k(const INPUT_SPACE &Space, const std::vector<INPUT_SPACE_REGION> &Regions, std::wstring *Reason)
{
    //
    // Reserved legacy input space.
    //
    if (Space.Id.LowPart == 0 && Space.Id.HighPart == -2)
    {
        if (Reason)
        {
            *Reason = L"Reserved legacy input space identifier.";
        }

        return false;
    }

    if (Space.Reserved != 0)
    {
        if (Reason)
        {
            *Reason = L"INPUT_SPACE.Reserved must be zero.";
        }

        return false;
    }

    if (Regions.empty())
    {
        if (Reason)
        {
            *Reason = L"No regions supplied.";
        }

        return false;
    }

    for (const auto &region : Regions)
    {
        if (!ValidateRegion(region, Reason))
        {
            return false;
        }
    }

    return true;
}

bool
ValidateRegion(const INPUT_SPACE_REGION &Region, std::wstring *Reason)
{
    if (!IsValidRegionFlags(Region.Flags))
    {
        if (Reason)
        {
            *Reason = L"Invalid region flag combination.";
        }

        return false;
    }

    if (!ValidateRect(Region.Rect))
    {
        if (Reason)
        {
            *Reason = L"Invalid region rectangle.";
        }

        return false;
    }

    //
    // Dispatch based on region type.
    //
    // Display regions require DPI,
    // rotation, mode rects, etc.
    //
    // Custom regions (hinge/gap regions)
    // only require valid geometry.
    //
    if (IsDisplayRegion(Region.Flags))
    {
        return ValidateDisplayRegion(Region, Reason);
    }

    return ValidateCustomRegion(Region, Reason);
}

bool
ValidateDisplayRegion(const INPUT_SPACE_REGION &Region, std::wstring *Reason)
{
    if (!ValidateRotation(Region.Rotation))
    {
        if (Reason)
        {
            *Reason = L"Invalid rotation value.";
        }

        return false;
    }

    if (!ValidateDpi(Region))
    {
        if (Reason)
        {
            *Reason = L"Invalid DPI value.";
        }

        return false;
    }

    if (!ValidateModeRect(Region))
    {
        if (Reason)
        {
            *Reason = L"Invalid ModeRect.";
        }

        return false;
    }

    if (!ValidateScalingRect(Region))
    {
        if (Reason)
        {
            *Reason = L"Invalid ScalingRect.";
        }

        return false;
    }

    return true;
}

bool
ValidateCustomRegion(const INPUT_SPACE_REGION &Region, std::wstring *Reason)
{
    UNREFERENCED_PARAMETER(Reason);

    //
    // Custom regions only require
    // valid geometry.
    //
    return ValidateRect(Region.Rect);
}

bool
ValidateRect(const RECT &Rect)
{
    const LONG width = Rect.right - Rect.left;

    const LONG height = Rect.bottom - Rect.top;

    return width > 0 && height > 0;
}

bool
ValidateModeRect(const INPUT_SPACE_REGION &Region)
{
    const RECT &rect = Region.ModeRect;

    const LONG width = rect.right - rect.left;

    const LONG height = rect.bottom - rect.top;

    return rect.left == 0 && rect.top == 0 && width > 0 && height > 0;
}

bool
ValidateScalingRect(const INPUT_SPACE_REGION &Region)
{
    const RECT &mode = Region.ModeRect;

    const RECT &scale = Region.ScalingRect;

    if (!ValidateRect(scale))
    {
        return false;
    }

    if (scale.left < mode.left)
    {
        return false;
    }

    if (scale.top < mode.top)
    {
        return false;
    }

    if (scale.right > mode.right)
    {
        return false;
    }

    if (scale.bottom > mode.bottom)
    {
        return false;
    }

    return true;
}

bool
ValidateRotation(UINT32 Rotation)
{
    return Rotation >= DISPLAYCONFIG_ROTATION_IDENTITY && Rotation <= DISPLAYCONFIG_ROTATION_ROTATE270;
}

bool
ValidateDpi(const INPUT_SPACE_REGION &Region)
{
    //
    // Empirically:
    //
    //   0     = unspecified
    //   96+   = valid monitor DPI
    //
    if (Region.Dpi == 0 && Region.DpiAlt == 0)
    {
        return true;
    }

    return Region.Dpi >= 96 && Region.DpiAlt >= 96;
}

} // namespace InputStitching