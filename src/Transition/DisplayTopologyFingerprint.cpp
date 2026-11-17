#include "pch.h"

#include "Transition\DisplayTopologyFingerprint.h"

#include <algorithm>

namespace Transition {

namespace {

void
HashCombine(uint64_t &Seed, uint64_t Value)
{
    Seed ^= Value + 0x9e3779b97f4a7c15ULL + (Seed << 6) + (Seed >> 2);
}

BOOL CALLBACK
EnumMonitorProc(HMONITOR Monitor, HDC, LPRECT, LPARAM Context)
{
    auto *fingerprint = reinterpret_cast<DisplayTopologyFingerprint *>(Context);

    MONITORINFOEXW info{};

    info.cbSize = sizeof(info);

    if (!GetMonitorInfoW(Monitor, &info))
    {
        return TRUE;
    }

    OutputDescriptor output{};

    output.DesktopRect = info.rcMonitor;

    output.Primary = (info.dwFlags & MONITORINFOF_PRIMARY) != 0;

    DEVMODEW mode{};

    mode.dmSize = sizeof(mode);

    if (EnumDisplaySettingsW(info.szDevice, ENUM_CURRENT_SETTINGS, &mode))
    {
        output.Rotation = mode.dmDisplayOrientation;
    }

    fingerprint->Outputs.push_back(std::move(output));

    ++fingerprint->OutputCount;

    return TRUE;
}

} // namespace

HRESULT
DisplayTopologyCapture::Capture(DisplayTopologyFingerprint &Fingerprint)
{
    Fingerprint = {};

    Fingerprint.VirtualDesktop.left = GetSystemMetrics(SM_XVIRTUALSCREEN);

    Fingerprint.VirtualDesktop.top = GetSystemMetrics(SM_YVIRTUALSCREEN);

    Fingerprint.VirtualDesktop.right = Fingerprint.VirtualDesktop.left + GetSystemMetrics(SM_CXVIRTUALSCREEN);

    Fingerprint.VirtualDesktop.bottom = Fingerprint.VirtualDesktop.top + GetSystemMetrics(SM_CYVIRTUALSCREEN);

    EnumDisplayMonitors(nullptr, nullptr, EnumMonitorProc, reinterpret_cast<LPARAM>(&Fingerprint));

    uint64_t hash = 0;

    HashCombine(hash, Fingerprint.OutputCount);

    HashCombine(hash, Fingerprint.VirtualDesktop.left);

    HashCombine(hash, Fingerprint.VirtualDesktop.top);

    HashCombine(hash, Fingerprint.VirtualDesktop.right);

    HashCombine(hash, Fingerprint.VirtualDesktop.bottom);

    for (const auto &output : Fingerprint.Outputs)
    {
        HashCombine(hash, output.DesktopRect.left);

        HashCombine(hash, output.DesktopRect.top);

        HashCombine(hash, output.DesktopRect.right);

        HashCombine(hash, output.DesktopRect.bottom);

        HashCombine(hash, output.Primary ? 1ull : 0ull);

        HashCombine(hash, output.Rotation);
    }

    Fingerprint.Hash = hash;

    return S_OK;
}

} // namespace Transition
