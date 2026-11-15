#include "pch.h"

#include <InputStitching\InputSpacePublisher.h>

#include <windows.h>

#include <InputStitching\Plan.h>

namespace InputStitching {

PFN_NtConfigureInputSpace
InputSpacePublisher::Resolve()
{
    if (m_NtConfigureInputSpace)
    {
        return m_NtConfigureInputSpace;
    }

    HMODULE win32u = GetModuleHandleW(L"win32u.dll");

    if (!win32u)
    {
        win32u = LoadLibraryExW(L"win32u.dll", nullptr, LOAD_LIBRARY_SEARCH_SYSTEM32);
    }

    if (!win32u)
    {
        return nullptr;
    }

    m_NtConfigureInputSpace =
        reinterpret_cast<PFN_NtConfigureInputSpace>(GetProcAddress(win32u, "NtConfigureInputSpace"));

    return m_NtConfigureInputSpace;
}

HRESULT
InputSpacePublisher::Publish(const Plan &PlanData)
{
    PFN_NtConfigureInputSpace fn = Resolve();

    if (!fn)
    {
        return HRESULT_FROM_WIN32(ERROR_PROC_NOT_FOUND);
    }

    if (PlanData.Regions.empty())
    {
        return E_INVALIDARG;
    }

    const INPUT_SPACE_REGION *regions = PlanData.Regions.data();

    const UINT32 regionCount = static_cast<UINT32>(PlanData.Regions.size());

    if (!fn(&PlanData.Space, regions, regionCount))
    {
        DWORD error = GetLastError();

        if (!error)
        {
            error = ERROR_GEN_FAILURE;
        }

        return HRESULT_FROM_WIN32(error);
    }

    return S_OK;
}

HRESULT
InputSpacePublisher::Remove(const LUID &SpaceId)
{
    PFN_NtConfigureInputSpace fn = Resolve();

    if (!fn)
    {
        return HRESULT_FROM_WIN32(ERROR_PROC_NOT_FOUND);
    }

    INPUT_SPACE space{};

    space.Id = SpaceId;

    space.Reserved = 0;

    //
    // Reverse-engineered behavior:
    // RegionCount == 0 removes the
    // registered input space.
    //
    if (!fn(&space, nullptr, 0))
    {
        DWORD error = GetLastError();

        if (!error)
        {
            error = ERROR_GEN_FAILURE;
        }

        return HRESULT_FROM_WIN32(error);
    }

    return S_OK;
}

} // namespace InputStitching