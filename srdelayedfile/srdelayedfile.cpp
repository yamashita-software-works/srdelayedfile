//***************************************************************************
//*                                                                         *
//*  srdelayedfile.cpp                                                      *
//*                                                                         *
//*  Create: 2022-05-16                                                     *
//*                                                                         *
//*  Author: YAMASHITA Katsuhiro                                            *
//*                                                                         *
//*  Copyright (C) YAMASHITA Katsuhiro. All rights reserved.                *
//*  Licensed under the MIT License.                                        *
//*                                                                         *
//***************************************************************************
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <strsafe.h>
#include <locale.h>
#include <conio.h>
#include "srdelayedfile.h"
#include "mem.h"
#include "misc.h"

BOOL SetStringOnce( PWSTR *pszSet, PWSTR psz )
{
    if( *pszSet != NULL )
        return FALSE;
    *pszSet = DupString(psz);
    return TRUE;
}

BOOL SetString( PWSTR *pszSet, PWSTR psz )
{
    _SafeMemFree( *pszSet );
    *pszSet = DupString(psz);
    return TRUE;
}

typedef enum {
    SRDUnknown = 0,
    SRDMakeDelayedOperationFile,
    SRDDumpDelayedOperationFile,
    SRDSetupDelayedOperationFile,
    SRDLastErrorDelayedOperationFile,
} SRDCMDTYPE;

typedef struct _COMMANDLINE_PARAMETERS
{
    SRDCMDTYPE cmd;
    PWSTR DelayedOperationFile;
    PWSTR LayoutFile;
    PWSTR OutputFile;
    BOOL ForceOverwrite;  // Force overwrite if exist output file.
} COMMANDLINE_PARAMETERS;

VOID ClearComamndLineParams(COMMANDLINE_PARAMETERS *pcp)
{
    _SafeMemFree(pcp->DelayedOperationFile);
    _SafeMemFree(pcp->LayoutFile);
    _SafeMemFree(pcp->OutputFile);
}

BOOL ParseComamndLine(int argc, WCHAR* argv[],COMMANDLINE_PARAMETERS *pcp)
{
    int i;

    pcp->cmd = SRDUnknown;

    for(i = 1; i < argc; )
    {
        if( argv[i][0] == L'-' || argv[i][0] == L'/' )
        {
            if( _wcsicmp(&argv[i][1], L"i" ) == 0 )
            {
                if( (i + 1) < argc )
                {
                    i++;
                    // input XML file
                    if( !SetStringOnce( &pcp->LayoutFile, argv[i] ) )
                        return FALSE;
                }
            }
            else if( _wcsicmp(&argv[i][1], L"o" ) == 0 )
            {
                if( (i + 1) < argc )
                {
                    i++;
                    // output delayed operations file 
                    if( !SetStringOnce( &pcp->OutputFile, argv[i] ) )
                        return FALSE;
                }
            }
            else if( _wcsicmp(&argv[i][1], L"d" ) == 0 )
            {
                if( pcp->cmd != SRDUnknown )
                    return FALSE;
                pcp->cmd = SRDDumpDelayedOperationFile;
                if( (i + 1) < argc )
                {
                    i++;
                    // dump delayed operations file 
                    if( !SetStringOnce( &pcp->DelayedOperationFile, argv[i] ) )
                        return FALSE;
                }
            }
            else if( _wcsicmp(&argv[i][1], L"f" ) == 0 )
            {
                // create mode option
                pcp->ForceOverwrite = TRUE;
            }
            else if( _wcsicmp(argv[i],L"--setup" ) == 0 )
            {
                if( pcp->cmd != SRDUnknown )
                    return FALSE;
                pcp->cmd = SRDSetupDelayedOperationFile;
                if( (i + 1) < argc )
                {
                    i++;
                    // dump delayed operations file 
                    if( !SetStringOnce( &pcp->DelayedOperationFile, argv[i] ) )
                        return FALSE;
                }
            }
            else if( _wcsicmp(argv[i],L"--lasterror" ) == 0 )
            {
                if( pcp->cmd != SRDUnknown )
                    return FALSE;
                pcp->cmd = SRDLastErrorDelayedOperationFile;
            }
            else
            {
                return FALSE;
            }
        }

        i++;
    }

    if( pcp->cmd == SRDUnknown )
    {
        if( pcp->DelayedOperationFile == NULL )
        {
            if( pcp->OutputFile == NULL || pcp->LayoutFile == NULL )
                return FALSE;
        }
        else
        {
            if( pcp->OutputFile != NULL || pcp->LayoutFile != NULL )
                return FALSE;
        }

        if( pcp->LayoutFile && pcp->OutputFile )
        {
            pcp->cmd = SRDMakeDelayedOperationFile;
        }
    }

    return TRUE;
}

//----------------------------------------------------------------------------
//
//  wmain()
//
//----------------------------------------------------------------------------
int __cdecl wmain(int argc, WCHAR* argv[])
{
    HRESULT hr = S_OK;

    _wsetlocale(LC_ALL,L"");

    if( argc < 2 )
    {
        PrintHelp();
        return 0;
    }

    _MemInit();

    COMMANDLINE_PARAMETERS cp;

    memset(&cp,0,sizeof(cp));

    ParseComamndLine(argc,argv,&cp);

    switch( cp.cmd )
    {
        case SRDLastErrorDelayedOperationFile:
            hr = StatusSRDelayedFile();
            break;
        case SRDDumpDelayedOperationFile:
            hr = DumpSRDelayedFile(cp.DelayedOperationFile);
            break;
        case SRDSetupDelayedOperationFile:
            hr = SetupSRDelayedFile(cp.DelayedOperationFile);
            break;
        case SRDMakeDelayedOperationFile:
        {
            CSRItemArray *pSRRecordArray = NULL;
            hr = XMLParseFile(cp.LayoutFile,&pSRRecordArray);
            if( hr == S_OK )
            {
                hr = CreateSRDelayedFile(cp.OutputFile,pSRRecordArray,cp.ForceOverwrite);
                if( hr == S_OK )
                {
                    printf("\n'%S' create succeeded.\n",cp.OutputFile);
                }
                delete pSRRecordArray;
            }
            break;
        }
        default:
            hr = E_INVALIDARG;
            break;
    }

    if( 0xC00CE000 <= hr && hr <= 0xC00CEFFF )
    {
        // XML Lite error
        printf("XML Syntax or Contents error.\nerror code : 0x%04X (See XML Lite document)\n",hr);
    }
    else if( FAILED(hr) && (HRESULT_FACILITY(hr) == FACILITY_SRDF) )
    {
        CHAR *p;
        switch( hr )
        {
            case SRDF_E_XMLSYNTAX_ERROR:
                p = "Input XML syntax error.";
                break;
            case SRDF_E_SRDELAYED_EXE_NOT_FOUND:
                p = "SRDelayed.exe not found.";
                break;
            case SRDF_E_OPERATIONFILE_EXISTS:
                p = "Already exist operation file.\nIf you want overwrite exists file run with -f option.";
                break;
            default:
                p = "error.";
                break;
        }
        printf("\n%s\n",p);
    }
    else if( FAILED(hr) )
    {
        PWSTR pMessage = NULL;
        if( WinGetErrorMessage(hr,&pMessage) )
        {
            printf("%S\n",pMessage);
            WinFreeErrorMessage(pMessage);
        }
        else
        {
            printf("error: 0x%04X\n",hr);
        }
    }

    ClearComamndLineParams(&cp);

    _MemEnd();

    return HRESULT_CODE(hr);
}
