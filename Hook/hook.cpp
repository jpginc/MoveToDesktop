/**
* MoveToDesktop
*
* Copyright (C) 2015-2016 by Tobias Salzmann
* Copyright (C) 2008-2011 by Manuel Meitinger
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 2 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#define WIN32_LEAN_AND_MEAN

#include <Windows.h>
#include <stdio.h>
#include <process.h>
#include <Windows.h>
#include <vector>
#include "VirtualDesktops.h"
#include "../shared.h"

IServiceProvider* pServiceProvider = nullptr;
IVirtualDesktopManager *pDesktopManager = nullptr;

enum EComStatus
{
	COMSTATUS_UNINITIALIZED,
	COMSTATUS_INITIALIZED,
	COMSTATUS_ERROR,
};

int ComStatus = COMSTATUS_UNINITIALIZED;

BOOL InitCom()
{
	Log("Initalizing Com");
	if (ComStatus == COMSTATUS_INITIALIZED)
	{
		Log("> Allready Initialized!");
		return true;
	}
	else if (ComStatus == COMSTATUS_ERROR)
	{
		Log("> Allready tried to initialize but it failed.");
	}

	ComStatus = COMSTATUS_ERROR;
	HRESULT hr = ::CoInitialize(NULL);
	if (FAILED(hr))
	{
		Log("> CoInitialize failed: %X", hr);
		return FALSE;
	}

	hr = ::CoCreateInstance(CLSID_ImmersiveShell, NULL, CLSCTX_LOCAL_SERVER, __uuidof(IServiceProvider), (PVOID*)&pServiceProvider);
	if (FAILED(hr))
	{
		Log("> CoCreateInstance failed: %X", hr);
		return FALSE;
	}



	hr = pServiceProvider->QueryService(__uuidof(IVirtualDesktopManager), &pDesktopManager);
	if (FAILED(hr))
	{
		Log("> QueryService(DesktopManager) failed");
		pServiceProvider->Release();
		pServiceProvider = nullptr;
		return FALSE;
	}

	ComStatus = COMSTATUS_INITIALIZED;
	return TRUE;
}

VOID FreeCom()
{
	if (ComStatus == COMSTATUS_INITIALIZED)
	{
		pDesktopManager->Release();
		pServiceProvider->Release();
		ComStatus = COMSTATUS_UNINITIALIZED;
	}
}

UINT GetDesktopNumber(UINT desktopNumberWithMask)
{
	return desktopNumberWithMask & 0xF;
}

bool ContainsMask(UINT desktopNumberWithMask)
{
	//We use 4 bits for the desktop number (0xF)
	//A UINT can be 2 or 4 BYTES so zero off the first 2 bytes if it is 4 bytes
	UINT mask = UINT_MAX & 0xABC0;

	//and remove the first 4 bits (desktop number) from desktopNumberWithMask
	desktopNumberWithMask = desktopNumberWithMask & (UINT_MAX ^ 0xF);
	Log("Mask is %X and passed in is %X", mask, desktopNumberWithMask);

	return (desktopNumberWithMask ^ mask) == 0x0;
}

bool GetGuidOfDesktopFromRegistry(int desktopIndex, GUID &guid)
{
	HKEY hKey;
	LONG result;
	unsigned long type = REG_DWORD, size;
	const LPCSTR virtualDesktopIdsGroup = "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\VirtualDesktops";
	const LPCSTR virtualDesktopIdsKey = "VirtualDesktopIDs";


	result = RegOpenKeyEx(HKEY_CURRENT_USER, virtualDesktopIdsGroup, 0, KEY_READ, &hKey);
	if (result == ERROR_SUCCESS)
	{
		result = RegQueryValueEx(hKey, virtualDesktopIdsKey, NULL, &type, NULL, &size);
		if (result == ERROR_SUCCESS && (size / (sizeof(GUID)) > desktopIndex))
		{
			Log("The size is %d", size);
			std::vector<unsigned char> buffer(size);
			result = RegQueryValueEx(hKey, virtualDesktopIdsKey, NULL, &type, &buffer.front(), &size);
			guid = *(reinterpret_cast<GUID*>(&buffer.front()) + desktopIndex);
			return true;
		}
		else
		{
			Log("Cant get the size");
		}
		RegCloseKey(hKey);
	}
	else
	{
		Log("error %d", result);
	}

	return false;
}

void MoveToDesktop(GUID desktopId, HWND hwnd)
{
	try {
		if (!InitCom())
		{
			Log("InitCom failed");
			return;
		}

		HRESULT hr = pDesktopManager->MoveWindowToDesktop(hwnd, desktopId);
		if (!SUCCEEDED(hr))
		{
			Log("Error %X on moving %X to %X", hr, hwnd, desktopId);
		}
	}
	catch (int e)
	{
		Log("Exception %X", e);
	}
	return;
}

HWND GetRootHwnd(HWND hwnd)
{
	Log("Getting RootWindow of %X", hwnd);
	HWND rootHwnd = GetAncestor(hwnd, GA_ROOTOWNER);
	return rootHwnd == NULL ? hwnd : rootHwnd;
}

void HandleSysCommand(WPARAM desktopNumberWithMask, HWND hwnd)
{
	if (!ContainsMask((UINT)desktopNumberWithMask)) return;

	UINT desktopNumber = GetDesktopNumber((UINT)desktopNumberWithMask);
	GUID desktopId;
	if (!GetGuidOfDesktopFromRegistry(desktopNumber, desktopId)) return;

	MoveToDesktop(desktopId, hwnd);
}

LRESULT CALLBACK GetMsgProc(INT code, WPARAM wParam, LPARAM lParam)
{
#define msg ((PMSG)lParam)
	if (code == HC_ACTION)
	{
		switch (msg->message)
		{
		case WM_SYSCOMMAND:
		{
			Log("WM_SYSCOMMAND");
			HandleSysCommand(msg->wParam, msg->hwnd);
			break;
		}
		}
	}
	return CallNextHookEx(NULL, code, wParam, lParam);
#undef msg
}

BOOL WINAPI DllMain(HINSTANCE handle, DWORD dwReason, LPVOID reserved)
{

	if (dwReason == DLL_PROCESS_ATTACH)
	{
		Log("Startup");
	}
	else if (dwReason == DLL_PROCESS_DETACH)
	{
		FreeCom();
	}
	return TRUE;
}