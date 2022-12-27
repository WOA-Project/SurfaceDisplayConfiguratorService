#pragma once

VOID OnPowerEvent(_In_ GUID SettingGuid, _In_ PVOID Value, _In_ ULONG ValueLength, _Inout_opt_ PVOID Context);
int AutoRotateMain(SERVICE_STATUS_HANDLE g_StatusHandle);