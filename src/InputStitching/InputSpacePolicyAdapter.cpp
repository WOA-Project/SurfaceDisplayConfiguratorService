#include "pch.h"

#include "InputStitching\InputSpacePolicyAdapter.h"
#include <DisplayIdentityCache.h>

namespace InputStitching {

InputSpacePolicy
BuildInputSpacePolicy(
    CONST WCHAR *DisplayPanelId1,
    CONST WCHAR *DisplayPanelId2,
    const DisplayPolicy &Display,
    const InputSpaceConfiguration &Configuration)
{
    const DisplayIdentity *Panel1 = g_DisplayIdentityCache.FindByPanelId(DisplayPanelId1);
    const DisplayIdentity *Panel2 = g_DisplayIdentityCache.FindByPanelId(DisplayPanelId2);

    DisplayPolicy InputDisplayPolicy = {};
    BOOLEAN Panel1IsLeft = FALSE;

    if (!Panel1 && !Panel2)
    {
        return BuildInputSpacePolicyInternal(Display, Configuration);
    }
    else if (Panel1 && !Panel2)
    {
        Panel1IsLeft = Panel1->IsLeft;
    }
    else if (!Panel1 && Panel2)
    {
        Panel1IsLeft = Panel2->IsRight;
    }
    // Both are valid
    else
    {
        Panel1IsLeft = Panel1->IsLeft;
    }

    if (Panel1IsLeft)
    {
        InputDisplayPolicy.Display1Enabled = Display.Display1Enabled;
        InputDisplayPolicy.Display2Enabled = Display.Display2Enabled;
        InputDisplayPolicy.Panel1Orientation = Display.Panel1Orientation;
        InputDisplayPolicy.Panel2Orientation = Display.Panel2Orientation;
    }
    else
    {
        InputDisplayPolicy.Display1Enabled = Display.Display2Enabled;
        InputDisplayPolicy.Display2Enabled = Display.Display1Enabled;
        InputDisplayPolicy.Panel1Orientation = Display.Panel2Orientation;
        InputDisplayPolicy.Panel2Orientation = Display.Panel1Orientation;
    }

    return BuildInputSpacePolicyInternal(InputDisplayPolicy, Configuration);
}

InputSpacePolicy
BuildInputSpacePolicyInternal(
    const DisplayPolicy &Display,
    const InputSpaceConfiguration &Configuration)
{
    InputSpacePolicy policy{};

    //
    // Single-screen detection.
    //
    const bool singleScreen = (Display.Display1Enabled != Display.Display2Enabled);

    if (singleScreen)
    {
        if (Display.Display1Enabled)
        {
            policy.Mode = InputTopologyMode::SingleLeft;
        }
        else
        {
            policy.Mode = InputTopologyMode::SingleRight;
        }
    }
    else
    {
        if (Configuration.EnableStitching)
        {
            policy.Mode = InputTopologyMode::DualStitched;
        }
        else
        {
            policy.Mode = InputTopologyMode::DualIndependent;
        }
    }

    policy.EnableStitching = Configuration.EnableStitching;
    policy.EnableGapRegion = Configuration.EnableGapRegion;
    policy.GapPixels = Configuration.GapPixels;
    policy.PublishToSystem = Configuration.AutoPublish;

    //
    // Currently input follows display
    // participation state.
    //
    policy.EnableDisplay1Input = Display.Display1Enabled;
    policy.EnableDisplay2Input = Display.Display2Enabled;
    policy.EnableDisplay1Pen = Display.Display1Enabled;
    policy.EnableDisplay2Pen = Display.Display2Enabled;

    return policy;
}

} // namespace InputStitching