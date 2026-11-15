#pragma once

#include <windows.h>

namespace InputStitching {

HRESULT
TestDualIndependent();

HRESULT
TestDualStitched();

HRESULT
TestSingleLeft();

HRESULT
TestSingleRight();

HRESULT
TestPublish();

HRESULT
TestRemove();

HRESULT
DumpCurrentPlan();

HRESULT
RunInputSpaceTests();

} // namespace InputStitching