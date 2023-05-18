//***************************************************************************
//*                                                                         *
//*  dumpdelayedfile.cpp                                                    *
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

static wchar_t *__getpart(int , wchar_t *p, DWORD cb)
{
    size_t cch = wcslen(p);

    wprintf(L"%s",*p != NULL ? p : L"-");
    wprintf(L"\n");

    return p + (cch + 1); // next part
}

static HRESULT parse(PVOID ptr,DWORD cb)
{
    HRESULT hr = E_FAIL;
    WCHAR *p = (WCHAR *)ptr;

    __try
    {
        __try
        {
            while( *p )
            {
                p = __getpart(0,p,cb);
                if( p == NULL ) __leave;
                p = __getpart(1,p,cb);
                if( p == NULL ) __leave;
                p = __getpart(2,p,cb);
                if( p == NULL ) __leave;
                p = __getpart(3,p,cb);
                if( p == NULL ) __leave;

                printf("\n",p);
            }

            hr = S_OK;
        }
        __except( GetExceptionCode() == EXCEPTION_ACCESS_VIOLATION )
        { 
            hr = HRESULT_FROM_NT( EXCEPTION_ACCESS_VIOLATION );
        }	
    }
    __finally
    {
        ;
    }

    return hr;
}

static HRESULT SimpleDump(PCWSTR pszFile)
{
    HANDLE hFile;
    hFile = CreateFile(pszFile,GENERIC_READ,FILE_SHARE_READ,NULL,OPEN_EXISTING,0,NULL);
    if( hFile == INVALID_HANDLE_VALUE )
        return HRESULT_FROM_WIN32( GetLastError() );

    HRESULT hr = E_FAIL;
    HANDLE hMap = NULL;
    PVOID ptr = NULL;
    DWORD dwError = 0;
    DWORD cbFileSize = 0;

    __try
    {
        // File size : use lower 4GB only
        cbFileSize = GetFileSize(hFile,NULL);

        hMap = CreateFileMapping(hFile,NULL,PAGE_READONLY,0,0,NULL);
        if( hMap == NULL )
            __leave;

        ptr = MapViewOfFileEx(hMap,FILE_MAP_READ,0,0,cbFileSize,NULL);
        if( ptr == NULL )
            __leave;

        parse(ptr,cbFileSize);

        dwError = ERROR_SUCCESS;
    }
    __finally
    {
        dwError = GetLastError();
        UnmapViewOfFile(ptr);
        CloseHandle(hMap);
        CloseHandle(hFile);
    }
    return HRESULT_FROM_WIN32( dwError );
}

HRESULT
DumpSRDelayedFile(
    PCWSTR pszFile
    )
{
    if(!PathFileExists(pszFile))
        return  HRESULT_FROM_WIN32( ERROR_FILE_NOT_FOUND );

    return SimpleDump(pszFile);
}
