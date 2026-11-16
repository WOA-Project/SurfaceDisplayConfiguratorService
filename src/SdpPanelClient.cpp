#include "pch.h"
#include "SdpPanelClient.h"

#include <Windows.h>
#include <winioctl.h>
#include <SetupAPI.h>
#include <initguid.h>

#pragma comment(lib, "Setupapi.lib")

DEFINE_GUID(GUID_DEVINTERFACE_SDP_PANEL, 0xdbe4373d, 0x3c81, 0x40cb, 0xac, 0xe4, 0xe0, 0xe5, 0xd0, 0x5f, 0x0c, 0x9f);

#define SDP_DEVICE_TYPE 0x8321

#define SDP_IOCTL(fn) CTL_CODE(SDP_DEVICE_TYPE, (fn), METHOD_BUFFERED, FILE_ANY_ACCESS)

#define IOCTL_SDP_SET_BRIGHTNESS SDP_IOCTL(0x802)
#define IOCTL_SDP_GET_BRIGHTNESS SDP_IOCTL(0x807)
#define IOCTL_SDP_SET_NITS SDP_IOCTL(0x81A)
#define IOCTL_SDP_GET_PANEL_STATUS SDP_IOCTL(0x819)
#define IOCTL_SDP_SET_BRIGHTNESS_OVERRIDE SDP_IOCTL(0x81E)

#pragma pack(push, 1)

struct SDP_IOCTL_HEADER
{
    ULONG DisplayId;
    ULONG InputSize;
    ULONG OutputSize;
    ULONG Reserved;
    UCHAR Payload[1];
};

#pragma pack(pop)

namespace {
#pragma pack(push, 1)

struct BrightnessRequest
{
    SDP_IOCTL_HEADER Header;
    UCHAR Value;
};

struct NitsRequest
{
    SDP_IOCTL_HEADER Header;
    ULONG MilliNits;
};

struct OverrideRequest
{
    SDP_IOCTL_HEADER Header;
    UCHAR Enable;
    ULONG Selector;
};

struct EmptyRequest
{
    SDP_IOCTL_HEADER Header;
};

#pragma pack(pop)
} // namespace

SdpPanelClient::SdpPanelClient() : m_device(INVALID_HANDLE_VALUE) {}

SdpPanelClient::~SdpPanelClient()
{
    Close();
}

bool
SdpPanelClient::Open()
{
    if (IsOpen())
    {
        return true;
    }

    m_device = OpenInterface();

    return IsOpen();
}

void
SdpPanelClient::Close()
{
    if (m_device != INVALID_HANDLE_VALUE)
    {
        CloseHandle(m_device);
        m_device = INVALID_HANDLE_VALUE;
    }
}

bool
SdpPanelClient::IsOpen() const
{
    return m_device != INVALID_HANDLE_VALUE;
}

HANDLE
SdpPanelClient::OpenInterface()
{
    HDEVINFO devInfo =
        SetupDiGetClassDevs(&GUID_DEVINTERFACE_SDP_PANEL, nullptr, nullptr, DIGCF_PRESENT | DIGCF_DEVICEINTERFACE);

    if (devInfo == INVALID_HANDLE_VALUE)
    {
        return INVALID_HANDLE_VALUE;
    }

    SP_DEVICE_INTERFACE_DATA interfaceData{};
    interfaceData.cbSize = sizeof(interfaceData);

    if (!SetupDiEnumDeviceInterfaces(devInfo, nullptr, &GUID_DEVINTERFACE_SDP_PANEL, 0, &interfaceData))
    {
        SetupDiDestroyDeviceInfoList(devInfo);
        return INVALID_HANDLE_VALUE;
    }

    DWORD requiredSize = 0;

    SetupDiGetDeviceInterfaceDetail(devInfo, &interfaceData, nullptr, 0, &requiredSize, nullptr);

    auto detail = reinterpret_cast<PSP_DEVICE_INTERFACE_DETAIL_DATA>(malloc(requiredSize));

    if (!detail)
    {
        SetupDiDestroyDeviceInfoList(devInfo);
        return INVALID_HANDLE_VALUE;
    }

    detail->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA);

    HANDLE device = INVALID_HANDLE_VALUE;

    if (SetupDiGetDeviceInterfaceDetail(devInfo, &interfaceData, detail, requiredSize, nullptr, nullptr))
    {
        device = CreateFile(
            detail->DevicePath,
            GENERIC_READ | GENERIC_WRITE,
            FILE_SHARE_READ | FILE_SHARE_WRITE,
            nullptr,
            OPEN_EXISTING,
            FILE_ATTRIBUTE_NORMAL,
            nullptr);
    }

    free(detail);

    SetupDiDestroyDeviceInfoList(devInfo);

    return device;
}

bool
SdpPanelClient::SendIoctl(
    DWORD ioctl,
    void *input,
    DWORD inputSize,
    void *output,
    DWORD outputSize,
    DWORD *bytesReturned)
{
    if (!IsOpen())
    {
        return false;
    }

    DWORD returned = 0;

    BOOL result = DeviceIoControl(m_device, ioctl, input, inputSize, output, outputSize, &returned, nullptr);

    if (bytesReturned)
    {
        *bytesReturned = returned;
    }

    return result != FALSE;
}

bool
SdpPanelClient::SetBrightness(DisplayId display, UCHAR level)
{
    BrightnessRequest request{};

    request.Header.DisplayId = display;
    request.Header.InputSize = sizeof(UCHAR);
    request.Value = level;

    return SendIoctl(IOCTL_SDP_SET_BRIGHTNESS, &request, sizeof(request), nullptr, 0);
}

bool
SdpPanelClient::SetBrightnessNits(DisplayId display, ULONG milliNits)
{
    NitsRequest request{};

    request.Header.DisplayId = display;
    request.Header.InputSize = sizeof(ULONG);
    request.MilliNits = milliNits;

    return SendIoctl(IOCTL_SDP_SET_NITS, &request, sizeof(request), nullptr, 0);
}

bool
SdpPanelClient::GetBrightness(DisplayId display, UCHAR &level)
{
    EmptyRequest request{};

    request.Header.DisplayId = display;
    request.Header.OutputSize = 1;

    DWORD returned = 0;

    return SendIoctl(IOCTL_SDP_GET_BRIGHTNESS, &request, sizeof(request.Header), &level, sizeof(level), &returned);
}

bool
SdpPanelClient::GetPanelStatus(DisplayId display, SDP_PANEL_STATUS &status)
{
    EmptyRequest request{};

    request.Header.DisplayId = display;
    request.Header.OutputSize = sizeof(status);

    DWORD returned = 0;

    return SendIoctl(IOCTL_SDP_GET_PANEL_STATUS, &request, sizeof(request.Header), &status, sizeof(status), &returned);
}

bool
SdpPanelClient::SetBrightnessOverride(bool enabled)
{
    OverrideRequest request{};

    request.Header.DisplayId = Primary;
    request.Header.InputSize = 5;

    request.Enable = enabled ? 1 : 0;

    request.Selector = 1;

    return SendIoctl(IOCTL_SDP_SET_BRIGHTNESS_OVERRIDE, &request, sizeof(request), nullptr, 0);
}