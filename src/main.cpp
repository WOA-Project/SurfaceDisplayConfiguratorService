#include "pch.h"

#include <tchar.h>
#include <stdio.h>

#include "DisplayIdentityCache.h"

#include "InputStitching\InputSpaceGlobals.h"
#include "InputStitching\InputSpaceTest.h"

int
_tmain(int argc, TCHAR *argv[])
{
    UNREFERENCED_PARAMETER(argc);
    UNREFERENCED_PARAMETER(argv);

    wprintf(L"Hello");

    HRESULT hr = g_DisplayIdentityCache.Initialize();

    if (FAILED(hr))
    {
        wprintf(
            L"DisplayIdentityCache::Initialize "
            L"failed: 0x%08X\n",
            hr);

        return hr;
    }

    wprintf(L"Hello");

    hr = InputStitching::g_InputSpaceManager.Initialize();

    if (FAILED(hr))
    {
        wprintf(
            L"InputSpaceManager::Initialize "
            L"failed: 0x%08X\n",
            hr);

        return hr;
    }

    wprintf(
        L"\n"
        L"========================================\n"
        L" Input Space Test Harness\n"
        L"========================================\n");

    hr = InputStitching::RunInputSpaceTests();

    if (FAILED(hr))
    {
        wprintf(
            L"RunInputSpaceTests failed: "
            L"0x%08X\n",
            hr);

        return hr;
    }

    wprintf(L"All tests completed.\n");

    wprintf(
        L"\n"
        L"Publishing stitched configuration...\n");

    hr = InputStitching::TestPublish();

    if (FAILED(hr))
    {
        wprintf(
            L"TestPublish failed: "
            L"0x%08X\n",
            hr);

        return hr;
    }

    wprintf(L"Publish succeeded.\n");

    wprintf(
        L"\n"
        L"Press ENTER to remove "
        L"the published input space...\n");

    getchar();

    hr = InputStitching::TestRemove();

    if (FAILED(hr))
    {
        wprintf(
            L"TestRemove failed: "
            L"0x%08X\n",
            hr);

        return hr;
    }

    wprintf(L"Input space removed.\n");

    return 0;
}