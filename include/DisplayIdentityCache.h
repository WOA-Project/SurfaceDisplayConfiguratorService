#pragma once

#include <windows.h>
#include <string>

#include "DeviceProperties.h"

struct DisplayIdentity
{
    std::wstring PanelId;

    ULONG PanelGroup = 0;
    ULONG PanelSide = 0;

    ACPI_PLD_V2_BUFFER Pld = {};

    DISPLAY_DEVICE DisplayDevice = {};

    bool IsLeft = false;
    bool IsRight = false;
};

class DisplayIdentityCache
{
public:
    HRESULT Initialize();

    const DisplayIdentity *FindByPanelId(PCWSTR PanelId) const;

    const DisplayIdentity &LeftPanel() const;
    const DisplayIdentity &RightPanel() const;

private:
    HRESULT BuildIdentity(PCWSTR PanelId, DisplayIdentity &Identity);

private:
    DisplayIdentity m_LeftPanel;
    DisplayIdentity m_RightPanel;

    bool m_Initialized = false;
};

extern DisplayIdentityCache g_DisplayIdentityCache;