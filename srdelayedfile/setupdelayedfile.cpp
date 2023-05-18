//***************************************************************************
//*                                                                         *
//*  setupdelayedfile.cpp                                                   *
//*                                                                         *
//*  Create: 2023-05-08                                                     *
//*                                                                         *
//*  Author: YAMASHITA Katsuhiro                                            *
//*                                                                         *
//*  Copyright (C) YAMASHITA Katsuhiro. All rights reserved.                *
//*  Licensed under the MIT License.                                        *
//*                                                                         *
//***************************************************************************
#include <windows.h>
#include "srdelayedfile.h"
#include "misc.h"

/*++

Quote:

Spaces in the path and nameshould be hexadecimal encoded.For example, 
for Program Files,encodethe path as "\??\C:Program%20Files\a.dll".

When theregistry or system is being restored upon restart,your application
needs to ensurethat SetupExecute gets written in the correct registry hive. 

Recovery of theregistry is performed before Srdelayed.exe is run.
Theapplication needs to writeSetupExecute into therecovered version of 
theregistry becausethis is theversion that is read.

--*/

#define _REG_VALUE_SETUPEXECUTE    L"SetupExecute"
#define _REG_KEY_SESSION_MANAGER   L"SYSTEM\\CurrentControlSet\\Control\\Session Manager"

#define _LPWSTR_SRDELAYED_PATH     L"%SystemRoot%\\System32\\srdelayed.exe"

DWORD AppendCommand(PCWSTR pszDelayedFile,DWORD cbDelayedFile)
{
	HKEY hKey;
	DWORD samDesired = KEY_QUERY_VALUE|KEY_SET_VALUE;
	PCWSTR pszKey = _REG_KEY_SESSION_MANAGER;
	PWSTR pBuffer = NULL;
	DWORD dwType = 0;
	DWORD dwError = 0;
	DWORD cbDataSize = 0;
	PWSTR pExists = NULL;
	DWORD cbExists = 0;
	LSTATUS rc;

	// only succeeds in admin mode.
	if( (rc = RegOpenKeyEx(HKEY_LOCAL_MACHINE,pszKey,0,samDesired,&hKey)) != ERROR_SUCCESS )
	{
		return rc;
	}

	__try
	{
		if( (rc = RegQueryValueEx(hKey,_REG_VALUE_SETUPEXECUTE,NULL,&dwType,NULL,&cbExists)) == ERROR_SUCCESS )
		{
			// value exists
			if( REG_MULTI_SZ != dwType )
			{
				dwError = ERROR_DATATYPE_MISMATCH; // type missmatch
				__leave;
			}

			pExists = (PWSTR)_MemAlloc( cbExists );
			if( pExists == NULL )
			{
				dwError = ERROR_NOT_ENOUGH_MEMORY;
				__leave;
			}

			rc = RegQueryValueEx(hKey,_REG_VALUE_SETUPEXECUTE,NULL,&dwType,(LPBYTE)pExists,&cbExists);
		}
		else
		{
			// value not exists
			if( ERROR_FILE_NOT_FOUND != rc )
			{
				dwError = rc;
				__leave; // fatal error
			}

			dwType = REG_MULTI_SZ;
			cbExists = sizeof(WCHAR);
		}

		cbDataSize = cbExists + cbDelayedFile;

		pBuffer = (PWSTR)_MemAllocZero(cbDataSize);
		if( pBuffer == NULL )
		{
			dwError = ERROR_NOT_ENOUGH_MEMORY;
			__leave;
		}

		if( pExists )
			memcpy(pBuffer,pExists,cbExists);
		memcpy(&pBuffer[ (cbExists/sizeof(WCHAR))-1 ],pszDelayedFile,cbDelayedFile);

		pBuffer[ (cbDataSize/sizeof(WCHAR))-1 ] = UNICODE_NULL;

		rc = RegSetValueEx(hKey,_REG_VALUE_SETUPEXECUTE,0,dwType,(LPBYTE)pBuffer,cbDataSize);

		dwError = rc;
	}
	__finally
	{
		_SafeMemFree(pBuffer);
		_SafeMemFree(pExists);

		RegCloseKey(hKey);
	}
	return dwError;
}

HRESULT
SetupSRDelayedFile(
    PCWSTR pszDelayedFile
	)
{
	HRESULT hr;
	DWORD cbDelayedFile;
	DWORD dwError = ERROR_SUCCESS;
	WCHAR szCmdLine[MAX_PATH];
	WCHAR szFullFilePath[MAX_PATH];
	WCHAR szEscapedFilePath[MAX_PATH];
	WCHAR szSRDelayedExe[MAX_PATH];
	szCmdLine[0] = 0;
	szFullFilePath[0] = 0;
	szEscapedFilePath[0] = 0;

	ExpandEnvironmentStrings(_LPWSTR_SRDELAYED_PATH,szSRDelayedExe,MAX_PATH);

	if( !PathFileExists(szSRDelayedExe) )
	{
		return SRDF_E_SRDELAYED_EXE_NOT_FOUND;
	}

	if( wcsncmp(L"\\??\\",pszDelayedFile,4) == 0 )
	{
		StringCchCopy(szFullFilePath,MAX_PATH,pszDelayedFile);
	}
	else 
	{
		if( PathIsFileSpec(pszDelayedFile) )
		{
			GetCurrentDirectory(MAX_PATH,szFullFilePath);
			PathCombine(szFullFilePath,szFullFilePath,pszDelayedFile);
		}
		else if( PathIsRelative(pszDelayedFile) )
		{
			PathSearchAndQualify(pszDelayedFile,szFullFilePath,MAX_PATH);
		}
		else
		{
			StringCchCopy(szFullFilePath,MAX_PATH,pszDelayedFile);
		}
	}

	if( !PathFileExists(szFullFilePath) )
	{
		dwError = ERROR_FILE_NOT_FOUND;
	}

	if( dwError == ERROR_SUCCESS && szFullFilePath[0] != 0 )
	{
		StringCchCopy(szEscapedFilePath,MAX_PATH,L"\\??\\");
		PathExEscape(szFullFilePath,&szEscapedFilePath[4],MAX_PATH-4);

		hr = StringCchPrintf(szCmdLine,MAX_PATH,L"%s %s",szSRDelayedExe,szEscapedFilePath);
		if( hr != S_OK )
		{
			return hr;
		}

		cbDelayedFile = (DWORD)((wcslen(szCmdLine) + 1) * sizeof(WCHAR));

#if 0 // debug
		printf("\n%S\n",szCmdLine);
		dwError = 0;
#else
		dwError = AppendCommand(szCmdLine,cbDelayedFile);
#endif
	}

	return HRESULT_FROM_WIN32(dwError);
}
