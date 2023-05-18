//***************************************************************************
//*                                                                         *
//*  createdelayedfile.cpp                                                  *
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

HRESULT
PathExEscape(
    PCWSTR pszPath,
    PWSTR pszEscaped,
    INT cchEscaped
    )
{
    PCWSTR ps = pszPath;
    PWSTR pd = pszEscaped;
    ULONG cchCopied = 0;

    --cchEscaped; // for terminate null

    if( cchEscaped > 0 )
    {
        while( *ps && cchEscaped )
        {
            if( *ps == L' ' )
            {
                if( (cchEscaped - 3) >= 0 )
                {
                    memcpy(pd,L"%20",sizeof(L"%20"));
                    pd += 3,cchEscaped -= 3;
                    ps += 1;
                }
                else
                {
                    break;
                }
            }
            else
            {
                *pd++ = *ps++;
                cchEscaped--;
            }
        }
        *pd = L'\0';
    }

    return (*ps == L'\0' ? S_OK : S_FALSE);
}

HRESULT 
MakePath(
    PCWSTR pszInput,
    PWSTR pszOutput,
    DWORD cchOutput
    )
{
    HRESULT hr;
    PWSTR pszBuffer  = AllocStringBuffer( MAX_UNICODE_PATH_LENGTH );

    if( pszBuffer )
    {
        ExpandEnvironmentStrings(pszInput,pszBuffer,MAX_UNICODE_PATH_LENGTH);

        if( _wcsnicmp(pszBuffer,L"\\??\\",4) == 0 )
            hr = StringCchPrintf(pszOutput,cchOutput,L"%s",pszBuffer);
        else
            hr = StringCchPrintf(pszOutput,cchOutput,L"\\??\\%s",pszBuffer);
    }
    else
    {
        hr = E_OUTOFMEMORY;
    }

    _SafeMemFree( pszBuffer );

    return hr;
}

HRESULT
FreeStringRecord(
    SRDELAYED_RECORD_STR *pRecordStr
    )
{
    _SafeMemFree( pRecordStr->pszParam1 );
    _SafeMemFree( pRecordStr->pszParam2 );
    _SafeMemFree( pRecordStr->pszParam3 );
    _SafeMemFree( pRecordStr->pszParam4 );
    return S_OK;
}

HRESULT
MakeStringRecord(
    CSRDelayedItem *pItem,	
    SRDELAYED_RECORD_STR *pRecordStr
    )
{
    HRESULT hr = S_OK;
    PWSTR pszPathBuffer = AllocStringBuffer( MAX_UNICODE_PATH_LENGTH );
    if( pszPathBuffer == NULL )
        return E_OUTOFMEMORY;

    pRecordStr->pszParam1 = DupString( pItem->Param1 );

    if( pItem->Param2 )
    {
        if( pItem->Command != SRD_CMD_SETSHORTNAME )
        {
            if( (hr = MakePath(pItem->Param2,pszPathBuffer,MAX_UNICODE_PATH_LENGTH)) == S_OK )
            {
                pRecordStr->pszParam2 = DupString(pszPathBuffer);
            }
        }
        else
        {
            pRecordStr->pszParam2 = DupString(pItem->Param2);
        }
    }
    else
    {
        pRecordStr->pszParam2 = DupString(L"");
    }

    if( pItem->Param3 )
    {
        if( (hr = MakePath(pItem->Param3,pszPathBuffer,MAX_UNICODE_PATH_LENGTH)) == S_OK )
        {
            pRecordStr->pszParam3 = DupString(pszPathBuffer);
        }
    }
    else
    {
        pRecordStr->pszParam3 = DupString(L"");
    }

    pRecordStr->pszParam4 = DupString( WSTR_NOTEXECUTED );

    _SafeMemFree(pszPathBuffer);

    return hr;
}

__forceinline BOOL WriteParam(HANDLE hFile,PCWSTR psz)
{
    DWORD cb;
    return WriteFile(hFile,psz,
            ((DWORD)wcslen(psz) + 1) * sizeof(WCHAR),
            &cb,NULL);
}

HRESULT
WriteRecord(
    HANDLE hFile,
    SRDELAYED_RECORD_STR *pRecordStr
    )
{
    HRESULT hr = E_FAIL;

    __try
    {
        if( !WriteParam(hFile,pRecordStr->pszParam1) )
        {
            __leave;
        }

        if( !WriteParam(hFile,pRecordStr->pszParam2) )
        {
            __leave;
        }

        if( !WriteParam(hFile,pRecordStr->pszParam3) )
        {
            __leave;
        }

        if( !WriteParam(hFile,pRecordStr->pszParam4) )
        {
            __leave;
        }

        SetLastError( ERROR_SUCCESS );
    }
    __finally
    {
        hr = HRESULT_FROM_WIN32( GetLastError() );
    }

    return hr;
}

HRESULT
CreateSRDelayedFile(
    PCWSTR pszFile,
    CSRItemArray *pSRItemArray,
    BOOL bForceOverwrite
    )
{
    HRESULT hr = E_FAIL;

    DWORD dwCreationDisposition = CREATE_NEW;
    if( bForceOverwrite )
        dwCreationDisposition = CREATE_ALWAYS;

    // create output file
    HANDLE hFile;
    hFile = CreateFile(pszFile,GENERIC_WRITE,FILE_SHARE_READ,NULL,dwCreationDisposition,0,NULL);
    if( hFile == INVALID_HANDLE_VALUE )
    {
        if( GetLastError() == ERROR_FILE_EXISTS )
            return SRDF_E_OPERATIONFILE_EXISTS;
        else
            return HRESULT_FROM_WIN32( GetLastError() );
    }

    __try
    {
        int i;
        int cItems = pSRItemArray->GetCount();

        for(i = 0; i < cItems; i++)
        {
            SRDELAYED_RECORD_STR RecordStr = {0};

            CSRDelayedItem *pItem = pSRItemArray->GetPtr(i);

            hr = MakeStringRecord(pItem,&RecordStr);
            if( FAILED(hr) )
                __leave;

            // write record
            hr = WriteRecord(hFile,&RecordStr);
            if( FAILED(hr) )
                __leave;

            FreeStringRecord(&RecordStr);
        }

        // terminate double null
        hr = WriteParam(hFile,L"");
        if( FAILED(hr) )
            __leave;

        hr = S_OK;
    }
    __finally
    {
        // clsoe output file
        CloseHandle(hFile);
    }
    return hr;
}
