//***************************************************************************
//*                                                                         *
//*  misc.cpp                                                               *
//*                                                                         *
//*  Create: 2022-05-20                                                     *
//*                                                                         *
//*  Author: YAMASHITA Katsuhiro                                            *
//*                                                                         *
//*  Copyright (C) YAMASHITA Katsuhiro. All rights reserved.                *
//*  Licensed under the MIT License.                                        *
//*                                                                         *
//***************************************************************************
#include <windows.h>
#include "misc.h"

VOID WINAPI _GetDateTimeStringEx2(ULONG64 DateTime,LPWSTR pszText,int cchTextMax,LPWSTR DateFormat,LPWSTR TimeFormat,BOOL bDisplayAsUTC,BOOL bMilliseconds)
{
    SYSTEMTIME st;
    FILETIME ftLocal;
    LARGE_INTEGER liLts;
    LONGLONG ns = 10000000; // 100ns

    if( bDisplayAsUTC )
    {
        FileTimeToSystemTime((FILETIME*)&DateTime,&st);

        liLts.QuadPart = DateTime % 10000000; // Get a less than 1 second.
    }
    else
    {
        FileTimeToLocalFileTime((FILETIME*)&DateTime,&ftLocal);
        FileTimeToSystemTime(&ftLocal,&st);

        liLts.HighPart = ftLocal.dwHighDateTime;
        liLts.LowPart  = ftLocal.dwLowDateTime;
        liLts.QuadPart = liLts.QuadPart % 10000000; // Get a less than 1 second.
    }

    int cch;
    cch = GetDateFormat(LOCALE_USER_DEFAULT,
                0,
                &st, 
                DateFormat,
                pszText,cchTextMax);

    pszText[cch-1] = L' ';

    cch += GetTimeFormat(LOCALE_USER_DEFAULT,
                0,
                &st, 
                TimeFormat,
                &pszText[cch],cchTextMax-cch);

    if( bMilliseconds )
    {
        WCHAR szMilliseconds[16];
        int cchMilliseconds;

        WCHAR *p = wcschr(pszText,L'n');
        if( p )
        {
            WCHAR *pMilliseconds = p;
            // count 'n' characters
            while( *p == L'n' ) p++;
            int cch100ns = (int)(p - pMilliseconds);

            if( cch100ns > 0 && ((3 <= cch100ns) && (cch100ns <= 7)))
            {
#if 0
                cchMilliseconds = wsprintf(szMilliseconds,L"%03u",st.wMilliseconds);  // display  1ms "000"
#else
                cchMilliseconds = wsprintf(szMilliseconds,L"%07u",(UINT)liLts.QuadPart); // display 100ns "0000000"
#endif
                int i;
                for(i = 0; i < cch100ns; i++)
                {
                    pMilliseconds[i] = szMilliseconds[i];
                }
            }
        }
        else
        {
            if( (cch + 3) < cchTextMax )
            {
                cch += wsprintf(szMilliseconds,L".%03u",st.wMilliseconds);
                wcscat_s(pszText,cchTextMax,szMilliseconds);
            }
        }
    }
}

int WinGetSystemErrorMessageEx(ULONG ErrorCode,PWSTR *ppMessage,ULONG dwLanguageId)
{
    HMODULE hModule = NULL;
    DWORD f = 0;

    PWSTR pMessageBuf;
    DWORD cch;
    cch = FormatMessage(
                FORMAT_MESSAGE_ALLOCATE_BUFFER | 
                FORMAT_MESSAGE_FROM_SYSTEM |
                f,
                hModule,
                ErrorCode,
                dwLanguageId,
                (LPWSTR)&pMessageBuf,
                0,
                NULL
                );

    if( cch != 0 )
    {
        *ppMessage = (PWSTR)pMessageBuf;
    }

    return cch;
}

int WinGetErrorMessage(ULONG ErrorCode,PWSTR *ppMessage)
{
    return WinGetSystemErrorMessageEx(ErrorCode,ppMessage,GetThreadLocale());
}

int WinGetSystemErrorMessage(ULONG ErrorCode,PWSTR *ppMessage)
{
    return WinGetSystemErrorMessageEx(ErrorCode,ppMessage,MAKELANGID(LANG_ENGLISH,SUBLANG_DEFAULT));
}

void WinFreeErrorMessage(PWSTR pMessage)
{
    if( pMessage )
        LocalFree(pMessage);
}
