#pragma once

#include <Windows.h>
#include <string>

struct SDP_PANEL_STATUS
{
    UCHAR PanelAsleep;
    ULONG Brightness;
    ULONG LastNitsScaled;
    ULONG ThermalPercent;
    ULONG ThermalMaxNits;
    UCHAR VsyncEnabled;
    ULONG VsyncPeriodUs;
    ULONG EsdCount;
    UCHAR DisplayOn;
    UCHAR HasPanelBin;
    UCHAR PanelBin[0x80];
    ULONG Temperature;
    ULONG ThermalIndex;
};

class SdpPanelClient
{
public:
    enum DisplayId : ULONG
    {
        Primary = 1,
        Secondary = 2
    };

public:
    SdpPanelClient();
    ~SdpPanelClient();

    SdpPanelClient(const SdpPanelClient &) = delete;
    SdpPanelClient &operator=(const SdpPanelClient &) = delete;

    bool Open();
    void Close();

    bool IsOpen() const;

    bool SetBrightness(DisplayId display, UCHAR level);
    bool GetBrightness(DisplayId display, UCHAR &level);
    bool SetBrightnessNits(DisplayId display, ULONG milliNits);
    bool GetPanelStatus(DisplayId display, SDP_PANEL_STATUS &status);
    bool SetBrightnessOverride(bool enabled);

private:
    bool SendIoctl(
        DWORD ioctl,
        void *input,
        DWORD inputSize,
        void *output,
        DWORD outputSize,
        DWORD *bytesReturned = nullptr);

    static HANDLE OpenInterface();

private:
    HANDLE m_device;
};