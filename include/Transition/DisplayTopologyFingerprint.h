#pragma once

#include <windows.h>

#include <cstdint>
#include <vector>

namespace Transition {

struct OutputDescriptor
{
    RECT DesktopRect{};

    bool Primary = false;

    UINT Rotation = 0;
};

struct DisplayTopologyFingerprint
{
    uint64_t Hash = 0;

    UINT OutputCount = 0;

    RECT VirtualDesktop{};

    std::vector<OutputDescriptor> Outputs;

    bool operator==(const DisplayTopologyFingerprint &Other) const noexcept { return Hash == Other.Hash; }

    bool operator!=(const DisplayTopologyFingerprint &Other) const noexcept { return !(*this == Other); }
};

class DisplayTopologyCapture
{
public:
    static HRESULT Capture(DisplayTopologyFingerprint &Fingerprint);
};

} // namespace Transition