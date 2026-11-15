#include "pch.h"

#include "InputStitching\InputSpacePolicyAdapter.h"

namespace InputStitching {

InputSpacePolicy
BuildInputSpacePolicy(const DisplayPolicy &Display, const InputSpaceConfiguration &Configuration)
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