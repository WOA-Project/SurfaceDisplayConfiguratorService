#include "pch.h"
#include "Detection/WindowClassifier.h"
#include "VirtualDesktop.h"

#include <array>
#include <vector>

namespace {
constexpr wchar_t SplashClassName[] = L"MsoSplash";

constexpr wchar_t SystemAppsFolder[] = L"SYSTEMAPPS";

constexpr wchar_t SearchUI[] = L"SEARCHUI.EXE";

bool
FindFolderInPath(const std::wstring &path, const std::vector<std::wstring> &folders)
{
    for (const auto &folder : folders)
    {
        if (path.find(folder) != std::wstring::npos)
        {
            return true;
        }
    }

    return false;
}

bool
FindAppNameInPath(const std::wstring &path, const std::vector<std::wstring> &apps)
{
    const auto lastSlash = path.find_last_of(L'\\');

    if (lastSlash == std::wstring::npos)
    {
        return false;
    }

    for (const auto &app : apps)
    {
        const auto pos = path.rfind(app);

        if (pos != std::wstring::npos && pos <= lastSlash + 1 && pos + app.length() > lastSlash)
        {
            return true;
        }
    }

    return false;
}

bool
IsSystemWindow(HWND hwnd, const char *className)
{
    constexpr std::array systemClasses = {
        "SysListView32", "WorkerW", "Shell_TrayWnd", "Shell_SecondaryTrayWnd", "Progman"};

    const std::array systemWindows = {GetDesktopWindow(), GetShellWindow()};

    for (auto systemWindow : systemWindows)
    {
        if (hwnd == systemWindow)
        {
            return true;
        }
    }

    for (const auto systemClass : systemClasses)
    {
        if (!_stricmp(className, systemClass))
        {
            return true;
        }
    }

    return false;
}
} // namespace

bool
WindowClassifier::IsSplashScreen(HWND window)
{
    wchar_t className[MAX_PATH]{};

    if (!GetClassNameW(window, className, ARRAYSIZE(className)))
    {
        return false;
    }

    return wcscmp(className, SplashClassName) == 0;
}

bool
WindowClassifier::IsPopupWindow(HWND window)
{
    const auto style = GetWindowLongPtr(window, GWL_STYLE);

    return (style & WS_POPUP) == WS_POPUP;
}

bool
WindowClassifier::HasThickFrame(HWND window)
{
    const auto style = GetWindowLongPtr(window, GWL_STYLE);

    return (style & WS_THICKFRAME) == WS_THICKFRAME;
}

bool
WindowClassifier::HasVisibleOwner(HWND window)
{
    auto owner = GetWindow(window, GW_OWNER);

    if (!owner)
    {
        return false;
    }

    if (!IsWindowVisible(owner))
    {
        return false;
    }

    RECT rect{};

    if (!GetWindowRect(owner, &rect))
    {
        return true;
    }

    return rect.left != rect.right && rect.top != rect.bottom;
}

bool
WindowClassifier::IsStandardWindow(HWND window)
{
    if (GetAncestor(window, GA_ROOT) != window)
    {
        return false;
    }

    const auto style = GetWindowLongPtr(window, GWL_STYLE);

    const auto exStyle = GetWindowLongPtr(window, GWL_EXSTYLE);

    const bool visible = (style & WS_VISIBLE) == WS_VISIBLE;

    const bool toolWindow = (exStyle & WS_EX_TOOLWINDOW) == WS_EX_TOOLWINDOW;

    return visible && !toolWindow;
}

std::wstring
WindowClassifier::GetProcessPath(DWORD pid)
{
    HANDLE process = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, pid);

    if (!process)
    {
        return {};
    }

    std::wstring path;
    path.resize(MAX_PATH);

    DWORD length = static_cast<DWORD>(path.size());

    const bool success = QueryFullProcessImageNameW(process, 0, path.data(), &length) != FALSE;

    CloseHandle(process);

    if (!success)
    {
        return {};
    }

    path.resize(length);

    return path;
}

std::wstring
WindowClassifier::GetProcessPath(HWND window)
{
    constexpr wchar_t ApplicationFrameHost[] = L"ApplicationFrameHost.exe";

    DWORD pid{};

    GetWindowThreadProcessId(window, &pid);

    if (!pid)
    {
        return {};
    }

    auto path = GetProcessPath(pid);

    if (path.length() < wcslen(ApplicationFrameHost))
    {
        return path;
    }

    if (path.compare(
            path.length() - wcslen(ApplicationFrameHost), wcslen(ApplicationFrameHost), ApplicationFrameHost) != 0)
    {
        return path;
    }

    DWORD childPid = pid;

    EnumChildWindows(
        window,
        [](HWND hwnd, LPARAM param) -> BOOL {
            auto *pidPtr = reinterpret_cast<DWORD *>(param);

            DWORD currentPid{};

            GetWindowThreadProcessId(hwnd, &currentPid);

            if (currentPid != 0 && currentPid != *pidPtr)
            {
                *pidPtr = currentPid;

                return FALSE;
            }

            return TRUE;
        },
        reinterpret_cast<LPARAM>(&childPid));

    if (childPid != pid)
    {
        return GetProcessPath(childPid);
    }

    return path;
}

bool
WindowClassifier::IsExcludedByDefault(HWND hwnd, const std::wstring &processPath)
{
    static const std::vector<std::wstring> excludedFolders = {SystemAppsFolder};

    if (FindFolderInPath(processPath, excludedFolders))
    {
        return true;
    }

    char className[256]{};

    GetClassNameA(hwnd, className, ARRAYSIZE(className));

    if (IsSystemWindow(hwnd, className))
    {
        return true;
    }

    static const std::vector<std::wstring> excludedApps = {SearchUI};

    return FindAppNameInPath(processPath, excludedApps);
}

bool
WindowClassifier::IsExcluded(HWND window)
{
    auto processPath = GetProcessPath(window);

    if (processPath.empty())
    {
        return false;
    }

    CharUpperBuffW(processPath.data(), static_cast<DWORD>(processPath.length()));

    return IsExcludedByDefault(window, processPath);
}

bool
WindowClassifier::IsProcessable(HWND window)
{
    if (!IsWindow(window))
    {
        return false;
    }

    char className[256]{};

    GetClassNameA(window, className, sizeof(className));

    if (!_stricmp(className, "XamlExplorerHostIslandWindow"))
    {
        return false;
    }

    if (IsSplashScreen(window))
    {
        return false;
    }

    if (IsIconic(window))
    {
        return false;
    }

    if (!IsStandardWindow(window))
    {
        return false;
    }

    const bool popup = IsPopupWindow(window);

    const bool thickFrame = HasThickFrame(window);

    if (popup && !thickFrame && GetWindow(window, GW_OWNER) != nullptr)
    {
        return false;
    }

    if (HasVisibleOwner(window))
    {
        return false;
    }

    if (IsExcluded(window))
    {
        return false;
    }

    if (!VirtualDesktop::instance().IsWindowOnCurrentDesktop(window))
    {
        return false;
    }

    return true;
}