#pragma once

#include <windows.h>
#include <evntprov.h>

namespace Transition {

class TransitionEtwProvider
{
public:
    static HRESULT Register();

    static void Unregister();

    static void TransitionBegin(LPCWSTR ScenarioName);

    static void TransitionEnd(LPCWSTR ScenarioName, UINT DurationMs);

    static void CaptureBegin();

    static void CaptureEnd(UINT DurationMs);

    static void AnimationBegin(LPCWSTR ScenarioName);

    static void AnimationCommitPoint(LPCWSTR ScenarioName);

    static void AnimationEnd(LPCWSTR ScenarioName);

    static void TopologyActionBegin();

    static void TopologyActionEnd(HRESULT Hr);

    static void RebuildBegin();

    static void RebuildEnd(UINT DurationMs);

    static void AccessLost();

    static void DeviceRemoved(HRESULT Reason);

    static void TopologyChanged(UINT OutputCount);
    static REGHANDLE s_ProviderHandle;

private:
};

} // namespace Transition