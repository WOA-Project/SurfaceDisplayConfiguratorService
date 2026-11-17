#pragma once

#include "Transition\CaptureGenerationTracker.h"

#include <windows.h>

#include <d3d11.h>
#include <dxgi1_2.h>

#include <mutex>

#include <wrl/client.h>

namespace Transition {

enum class DuplicationState
{
    Uninitialized,

    Active,

    AccessLost,

    DeviceRemoved,

    Reinitializing
};

class DesktopDuplicator
{
public:
    DesktopDuplicator() = default;

    DesktopDuplicator(const DesktopDuplicator &) = delete;

    DesktopDuplicator &operator=(const DesktopDuplicator &) = delete;

    DesktopDuplicator(DesktopDuplicator &&) = delete;

    DesktopDuplicator &operator=(DesktopDuplicator &&) = delete;

    HRESULT Initialize(ID3D11Device *Device, UINT OutputIndex);

    HRESULT Recreate(ID3D11Device *Device, UINT OutputIndex);

    void Shutdown();

    HRESULT CaptureSnapshot(Microsoft::WRL::ComPtr<ID3D11Texture2D> &Snapshot);

    RECT GetDesktopRect() const noexcept { return m_DesktopRect; }

    UINT GetOutputIndex() const noexcept { return m_OutputIndex; }

private:
    HRESULT CreateDuplication();

    HRESULT EnsureValid();

private:
    UINT m_OutputIndex = 0;

    RECT m_DesktopRect{};

    DuplicationState m_State = DuplicationState::Uninitialized;

    CaptureGenerationTracker m_GenerationTracker;

    std::mutex m_RebuildLock;

    Microsoft::WRL::ComPtr<ID3D11Device> m_Device;

    Microsoft::WRL::ComPtr<IDXGIOutputDuplication> m_Duplication;
};

} // namespace Transition