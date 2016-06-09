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
#include "VirtualDesktops.h"
#include "../shared.h"

IServiceProvider* pServiceProvider = nullptr;
IVirtualDesktopManager *pDesktopManager = nullptr;
IVirtualDesktopManagerInternal* pDesktopManagerInternal = nullptr;

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
		return false;
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



	hr = pServiceProvider->QueryService(CLSID_VirtualDesktopAPI_Unknown, &pDesktopManagerInternal);
	if (FAILED(hr))
	{
		Log("> QueryService(DesktopManagerInternal) failed");
		pDesktopManager->Release();
		pDesktopManager = nullptr;
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
		pDesktopManagerInternal->Release();
		pServiceProvider->Release();
		ComStatus = COMSTATUS_UNINITIALIZED;
	}
}

void HandleSysCommand(WPARAM numberOfTheDestinationDesktop, HWND hwnd)
{
	if (numberOfTheDestinationDesktop)
	{
		Log("Getting RootWindow of %X to move to %X", hwnd, numberOfTheDestinationDesktop);
		HWND rootHwnd = GetAncestor(hwnd, GA_ROOTOWNER);
		if (rootHwnd != NULL)
		{
			Log("Root window is null");
			hwnd = rootHwnd;
		}

		Log("Moving %X to %X", hwnd, numberOfTheDestinationDesktop);
		IObjectArray *pObjectArray = nullptr;
		HRESULT hr = pDesktopManagerInternal->GetDesktops(&pObjectArray);
		if (FAILED(hr))
		{
			Log("Failed to get desktops for %X", hwnd);
			return;
		}

		IVirtualDesktop *pDesktop = nullptr;
		if (SUCCEEDED(pObjectArray->GetAt((UINT)numberOfTheDestinationDesktop, __uuidof(IVirtualDesktop), (void**)&pDesktop)))
		{
			GUID id;
			hr = pDesktop->GetID(&id);

			if (SUCCEEDED(hr))
			{
				Log("pDesktopManager->MoveWindowToDesktop(%X, %X)", hwnd, id);
				hr = pDesktopManager->MoveWindowToDesktop(hwnd, id);
				if (! SUCCEEDED(hr))
				{
					Log("Error %X on moving %X to %X", hr, hwnd, id);
				}
			}
			pDesktop->Release();
		}
		pObjectArray->Release();
	}
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