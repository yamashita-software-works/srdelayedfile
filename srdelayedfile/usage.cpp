//***************************************************************************
//*                                                                         *
//*  usage.cpp                                                              *
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
#include <shlwapi.h>
#include <stdio.h>
#include <strsafe.h>
#include "resource.h"

void PrintHelp()
{
    HINSTANCE hInst = GetModuleHandle(NULL);

    HRSRC h = FindResource(hInst,MAKEINTRESOURCE(IDR_USAGE),MAKEINTRESOURCE(RT_ANSI_TEXT));
    if ( h == NULL )
        return ;

    HGLOBAL hgbl;
    hgbl = LoadResource(hInst,h);
    if( hgbl )
    {
        PCHAR pText = (PCHAR)LockResource(hgbl);
        if( pText )
        {
            printf(pText);
        }
    }
}
