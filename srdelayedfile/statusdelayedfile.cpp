//***************************************************************************
//*                                                                         *
//*  statusdelayedfile.cpp                                                  *
//*                                                                         *
//*  Create: 2022-05-17                                                     *
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

#define _REG_KEY_SESSION_MANAGER   L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\SystemRestore"

HRESULT
StatusSRDelayedFile(
    VOID
    )
{
    HKEY hKey;
    DWORD samDesired = KEY_QUERY_VALUE;
    PCWSTR pszKey = _REG_KEY_SESSION_MANAGER;
    DWORD dwType = 0;
    PWSTR pExists = NULL;
    DWORD cb = 0;
    LSTATUS rc;
    WCHAR *psz = NULL;
    int cch = 32768;

    if( (rc = RegOpenKeyEx(HKEY_LOCAL_MACHINE,pszKey,0,samDesired,&hKey)) != ERROR_SUCCESS )
    {
        return rc;
    }

    __try
    {
        int tab = 4;
        wchar_t ch = L' ';

        psz = new WCHAR[cch];

        wprintf(L"\nLast detected error\n");

        LARGE_INTEGER liTime;
        cb = sizeof(LARGE_INTEGER);
        if( (rc = RegQueryValueEx(hKey,L"LastMainenanceTaskRunTimeStamp",NULL,&dwType,(LPBYTE)&liTime,&cb)) == ERROR_SUCCESS && dwType == REG_QWORD )
        {
            _GetDateTimeStringEx2(liTime.QuadPart,psz,cch,NULL,NULL,FALSE,TRUE);
            wprintf(L"%*cTime      : %s\n",tab,ch,psz);
        }

        DWORD status;
        cb = sizeof(DWORD);
        if( (rc = RegQueryValueEx(hKey,L"RestoreStatusResult",NULL,&dwType,(LPBYTE)&status,&cb)) == ERROR_SUCCESS && dwType == REG_DWORD )
        {
            wprintf(L"%*cResult    : 0x%08X\n",tab,ch,status);
        }

        cb = cch;
        if( (rc = RegQueryValueEx(hKey,L"RestoreStatusDetails",NULL,&dwType,(LPBYTE)psz,&cb)) == ERROR_SUCCESS && dwType == REG_SZ )
        {
            wprintf(L"%*cDetails   : %s\n",tab,ch,psz);
        }

        cb = cch;
        if( (rc = RegQueryValueEx(hKey,L"SrDelayedOperation",NULL,&dwType,(LPBYTE)psz,&cb)) == ERROR_SUCCESS && dwType == REG_SZ )
        {
            wprintf(L"%*cOperation : %s\n",tab,ch,psz);
        }

        cb = cch;
        if( (rc = RegQueryValueEx(hKey,L"SrDelayedArg0",NULL,&dwType,(LPBYTE)psz,&cb)) == ERROR_SUCCESS && dwType == REG_SZ )
        {
            wprintf(L"%*cArg0      : %s\n",tab,ch,psz);
        }

        cb = cch;
        if( (rc = RegQueryValueEx(hKey,L"SrDelayedArg1",NULL,&dwType,(LPBYTE)psz,&cb)) == ERROR_SUCCESS && dwType == REG_SZ )
        {
            wprintf(L"%*cArg1      : %s\n",tab,ch,psz);
        }
    }
    __finally
    {
        delete[] psz;

        RegCloseKey(hKey);
    }

    return S_OK;
}
