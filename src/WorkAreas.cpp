/*
 * Copyright (c) 2022-2023 The DuoWOA authors
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */
#include "pch.h"
#include "WorkAreas.h"

constexpr inline int DEFAULT_DPI = 96;

BOOL WINAPI
EnumDisplayMonitorCallback(HMONITOR monitor, HDC dc, LPRECT rect, LPARAM param)
{
    UNREFERENCED_PARAMETER(dc);
    UNREFERENCED_PARAMETER(rect);

    if (monitor == NULL)
    {
        return FALSE;
    }

    if (param == NULL)
    {
        return FALSE;
    }

    DOUBLE mainBottomOffset = *reinterpret_cast<DOUBLE *>(param);
    MONITORINFOEX monitorInfo{sizeof(MONITORINFOEX)};
    BOOL result = GetMonitorInfo(monitor, &monitorInfo);
    if (!result)
    {
        return TRUE;
    }

    // The bottom offset represents the area taken by the taskbar,
    // with no error when shytaskbar is enabled on the primary monitor
    DOUBLE bottomOffset = monitorInfo.rcMonitor.bottom - monitorInfo.rcWork.bottom;

    UINT monitorDpiX;
    UINT monitorDpiY;

    if (S_OK != GetDpiForMonitor(monitor, MDT_EFFECTIVE_DPI, &monitorDpiX, &monitorDpiY))
    {
        return TRUE;
    }

    DOUBLE monitorScaling = ((DOUBLE)monitorDpiY / (DOUBLE)DEFAULT_DPI);
    bottomOffset /= monitorScaling;

    DOUBLE bottomOffsetDifference = bottomOffset - mainBottomOffset;

    // When shytaskbar is active, the real offset is meant to be smaller than the
    // active one due to a bug in the shell, making the are active bigger than
    // it actually is (matching the expanded taskbar)
    if (bottomOffsetDifference > 0)
    {
        RECT workArea = monitorInfo.rcWork;

        // Fix the wrong bottom offset on this monitor
        workArea.bottom += (LONG)(bottomOffsetDifference * monitorScaling);

        // Apply new work area
        result = SystemParametersInfo(SPI_SETWORKAREA, 0, &workArea, 0);
        if (!result)
        {
            return TRUE;
        }
    }

    return TRUE;
}

//
// Subject: Update secondary monitors work area (bottom portion) to work around a design flaw
//          in Windows Shy Taskbar feature with multiple displays
//
// Returns: TRUE if succeeded, FALSE if failed
//
BOOL WINAPI
UpdateMonitorWorkAreas()
{
    POINT point = {0, 0};
    HMONITOR mainMonitor = MonitorFromPoint(point, MONITOR_DEFAULTTOPRIMARY);
    MONITORINFOEX mainMonitorInfo{sizeof(MONITORINFOEX)};
    BOOL result = GetMonitorInfo(mainMonitor, &mainMonitorInfo);
    if (!result)
    {
        return FALSE;
    }

    // The bottom offset represents the area taken by the taskbar,
    // with no error when shytaskbar is enabled on the primary monitor
    DOUBLE mainBottomOffset = mainMonitorInfo.rcMonitor.bottom - mainMonitorInfo.rcWork.bottom;

    UINT mainMonitorDpiX;
    UINT mainMonitorDpiY;

    if (S_OK != GetDpiForMonitor(mainMonitor, MDT_EFFECTIVE_DPI, &mainMonitorDpiX, &mainMonitorDpiY))
    {
        return FALSE;
    }

    // Divide by the DPI Y value to compare with other displays
    DOUBLE mainMonitorScaling = ((DOUBLE)mainMonitorDpiY / (DOUBLE)DEFAULT_DPI);
    mainBottomOffset /= mainMonitorScaling;

    return EnumDisplayMonitors(NULL, NULL, EnumDisplayMonitorCallback, reinterpret_cast<LPARAM>(&mainBottomOffset));
}