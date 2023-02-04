#include "pch.h"
#include "DisplayRotationManager.h"

DWORD WINAPI SetPanelOrientation(DWORD PanelId, INT Orientation)
{
	DISPLAY_DEVICE device = { 0 };
	DEVMODE deviceMode = { 0 };
	BOOLEAN IsPrimaryDisplay = FALSE;

	RtlZeroMemory(&device, sizeof(DISPLAY_DEVICE));
	RtlZeroMemory(&deviceMode, sizeof(DEVMODE));

	device.cb = sizeof(DISPLAY_DEVICE);

	if (!EnumDisplayDevices(NULL, PanelId, &device, 0))
	{
		return GetLastError();
	}

	if (!EnumDisplaySettings(device.DeviceName, ENUM_CURRENT_SETTINGS, &deviceMode))
	{
		return GetLastError();
	}

	//
	// In order to switch from portrait to landscape and vice versa we need to swap the resolution width and height
	// So we check for that
	//
	if ((deviceMode.dmDisplayOrientation + Orientation) % 2 == 1)
	{
		int temp = deviceMode.dmPelsHeight;
		deviceMode.dmPelsHeight = deviceMode.dmPelsWidth;
		deviceMode.dmPelsWidth = temp;
	}

	switch (Orientation)
	{
	case DMDO_DEFAULT:
	{
		deviceMode.dmPosition.x = (PanelId - 1) * deviceMode.dmPelsWidth;
		deviceMode.dmPosition.y = 0;
		IsPrimaryDisplay = PanelId == 1;
		break;
	}
	case DMDO_180:
	{
		deviceMode.dmPosition.x = -PanelId * deviceMode.dmPelsWidth;
		deviceMode.dmPosition.y = 0;
		IsPrimaryDisplay = PanelId == 0;
		break;
	}
	case DMDO_90:
	{
		deviceMode.dmPosition.x = 0;
		deviceMode.dmPosition.y = (PanelId - 1) * deviceMode.dmPelsHeight;
		IsPrimaryDisplay = PanelId == 1;
		break;
	}
	case DMDO_270:
	{
		deviceMode.dmPosition.x = 0;
		deviceMode.dmPosition.y = -PanelId * deviceMode.dmPelsHeight;
		IsPrimaryDisplay = PanelId == 0;
		break;
	}
	}

	//
	// Change the display orientation and save to the registry the changes (1 parameter for ChangeDisplaySettings)
	//
	if (deviceMode.dmFields | DM_DISPLAYORIENTATION)
	{
		deviceMode.dmDisplayOrientation = Orientation;

		DWORD flags = CDS_UPDATEREGISTRY | CDS_GLOBAL | CDS_NORESET;
		if (IsPrimaryDisplay)
		{
			printf("Display=%d is now primary\n", PanelId);
			flags |= CDS_SET_PRIMARY;
		}

		if (ChangeDisplaySettingsEx(
			device.DeviceName,
			&deviceMode,
			NULL,
			flags,
			NULL) != DISP_CHANGE_SUCCESSFUL)
		{
			return GetLastError();
		}
	}

	return ERROR_SUCCESS;
}

DWORD WINAPI CommitDisplaySettings()
{
	if (ChangeDisplaySettingsEx(NULL, NULL, NULL, 0, NULL) != DISP_CHANGE_SUCCESSFUL)
	{
		return GetLastError();
	}
	return ERROR_SUCCESS;
}

INT WINAPI ConvertSimpleOrientationToDMDO(SimpleOrientation orientation)
{
	switch (orientation)
	{
	case SimpleOrientation::NotRotated:
	{
		return DMDO_DEFAULT;
	}
	case SimpleOrientation::Rotated90DegreesCounterclockwise:
	{
		return DMDO_270;
	}
	case SimpleOrientation::Rotated180DegreesCounterclockwise:
	{
		return DMDO_180;
	}
	case SimpleOrientation::Rotated270DegreesCounterclockwise:
	{
		return DMDO_90;
	}
	}

	return DMDO_DEFAULT;
}

DWORD WINAPI SetPanelsOrientationState(SimpleOrientation Panel1SimpleOrientation, SimpleOrientation Panel2SimpleOrientation)
{
	DWORD error = ERROR_SUCCESS;

	BOOLEAN Panel1UnknownOrientation = Panel1SimpleOrientation == SimpleOrientation::Faceup || Panel1SimpleOrientation == SimpleOrientation::Facedown;
	BOOLEAN Panel2UnknownOrientation = Panel2SimpleOrientation == SimpleOrientation::Faceup || Panel2SimpleOrientation == SimpleOrientation::Facedown;

	if (Panel1UnknownOrientation && !Panel2UnknownOrientation)
	{
		Panel1SimpleOrientation = Panel2SimpleOrientation;
	}
	else if (Panel2UnknownOrientation && !Panel1UnknownOrientation)
	{
		Panel2SimpleOrientation = Panel1SimpleOrientation;
	}

	INT Panel1Orientation = ConvertSimpleOrientationToDMDO(Panel1SimpleOrientation);
	INT Panel2Orientation = ConvertSimpleOrientationToDMDO(Panel2SimpleOrientation);

	if ((error = SetPanelOrientation(0, Panel1Orientation)) != ERROR_SUCCESS)
	{
		return error;
	}

	if ((error = SetPanelOrientation(1, Panel2Orientation)) != ERROR_SUCCESS)
	{
		return error;
	}

	if ((error = CommitDisplaySettings()) != ERROR_SUCCESS)
	{
		return error;
	}

	return ERROR_SUCCESS;
}

DWORD WINAPI SetExtendedDisplayConfiguration()
{
	if (SetDisplayConfig(
		0,
		NULL,
		0,
		NULL,
		SDC_APPLY | SDC_TOPOLOGY_EXTEND | SDC_PATH_PERSIST_IF_REQUIRED
	) != ERROR_SUCCESS)
	{
		return GetLastError();
	}
	return ERROR_SUCCESS;
}

DWORD WINAPI SetCorrectDisplayConfiguration()
{
	DWORD error = ERROR_SUCCESS;

	if ((error = SetExtendedDisplayConfiguration()) != ERROR_SUCCESS)
	{
		return error;
	}

	if ((error = SetPanelsOrientationState(SimpleOrientation::NotRotated, SimpleOrientation::NotRotated)) != ERROR_SUCCESS)
	{
		return error;
	}

	return ERROR_SUCCESS;
}