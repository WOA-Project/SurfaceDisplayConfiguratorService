#pragma once

#include <Windows.h>

#include <vector>

#include "../Models/SeamInfo.h"

class MonitorTopology
{
public:
    void Refresh();

    const SeamInfo &GetSeam() const;

    const RECT &GetCombinedWorkArea() const;

    const std::vector<HMONITOR> &GetMonitors() const;

private:
    void RebuildSeam();

private:
    std::vector<HMONITOR> m_monitors;

    SeamInfo m_seam{};

    RECT m_combinedWorkArea{};
};