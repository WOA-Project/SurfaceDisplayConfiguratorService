#pragma once

#include "Transition\\CapturedDesktop.h"
#include "Transition\\CaptureState.h"
#include "Transition\\DesktopDuplicator.h"
#include "Transition\\DisplayTopologyFingerprint.h"

#include <d3d11.h>
#include <wrl/client.h>

#include <vector>

namespace Transition {

class DesktopCaptureManager
{
public:
    DesktopCaptureManager() = default;

    HRESULT Initialize(ID3D11Device *Device);

    void Shutdown();

    HRESULT Capture(CapturedDesktop &Desktop);

    HRESULT Rebuild();

    void Pause();

    void Resume();

    UINT GetOutputCount() const noexcept { return static_cast<UINT>(m_Duplicators.size()); }

    CaptureState GetState() const noexcept { return m_State; }

private:
    HRESULT EnumerateOutputs();

private:
    Microsoft::WRL::ComPtr<ID3D11Device> m_Device;

    std::vector<std::unique_ptr<DesktopDuplicator>> m_Duplicators;

    CaptureState m_State = CaptureState::Active;

    DisplayTopologyFingerprint m_LastTopology;
};

} // namespace Transition