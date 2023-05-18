//***************************************************************************
//*                                                                         *
//*  xmlparse.cpp                                                           *
//*                                                                         *
//*  Create: 2021-09-08                                                     *
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
#include <xmllite.h>
#include "srdelayedfile.h"
#include "resource.h"
#include "filestream.h"
#include "dynamicptrarray.h"

#define LPWSTR_ELEMENT_COMMANDS     L"Commands"
#define LPWSTR_ELEMENT_MOVEFILE     L"MoveFile"
#define LPWSTR_ELEMENT_DELETEFILE   L"DeleteFile"
#define LPWSTR_ELEMENT_SETSHORTNAME L"SetShortName"

struct SRDELAYED_RECORDS
{
    CSRItemArray *m_pItemArray;

    CSRDelayedItem *m_pCurItem;
    PWSTR m_pszCurElement;
    int m_section;

    SRDELAYED_RECORDS(CSRItemArray *pItemArray)
    {
        m_pItemArray = pItemArray;

        m_pCurItem = NULL;
        m_pszCurElement = NULL;
        m_section = SECTION_NONE;
    }

    ~SRDELAYED_RECORDS()
    {
        if( m_pCurItem )
            delete m_pCurItem;
        if( m_pszCurElement )
            FreeString( m_pszCurElement );
    }

    HRESULT BeginElement( PCWSTR ElementName )
    {
        if( IsEqualName( LPWSTR_ELEMENT_COMMANDS, ElementName )  )
        {
            m_section = SECTION_COMMANDS;
        }
        else 
        {
            if( m_section == SECTION_COMMANDS )
            {
                if( IsEqualName( LPWSTR_ELEMENT_MOVEFILE, ElementName ) )
                {
                    if( m_pCurItem != NULL  )
                        return E_FAIL;
                    m_pCurItem = new CSRDelayedMoveItem;
                    m_pszCurElement = DupString(ElementName);
                }
                else if( IsEqualName( LPWSTR_ELEMENT_DELETEFILE, ElementName ) )
                {
                    if( m_pCurItem != NULL  )
                        return E_FAIL;
                    m_pCurItem = new CSRDelayedDeleteItem;
                    m_pszCurElement = DupString(ElementName);
                }
                else if( IsEqualName( LPWSTR_ELEMENT_SETSHORTNAME, ElementName ) )
                {
                    if( m_pCurItem != NULL  )
                        return E_FAIL;
                    m_pCurItem = new CSRDelayedSetShortNameItem;
                    m_pszCurElement = DupString(ElementName);
                }
                else
                {
                    if( m_pCurItem )
                        m_pCurItem->SetElement(ElementName);
                    else
                        return SRDF_E_XMLSYNTAX_ERROR;
                }
            }
        }
        return S_OK;
    }

    HRESULT EndElement( PCWSTR ElementName )
    {
        if( IsEqualName( LPWSTR_ELEMENT_COMMANDS, ElementName ) )
        {
            m_section = SECTION_NONE;
        }
        else
        {
            if( m_section == SECTION_COMMANDS )
            {
                if(	m_pszCurElement && IsEqualName(ElementName,m_pszCurElement) )
                {
                    m_pItemArray->AddPtr( m_pCurItem );

                    m_pCurItem = NULL;

                    FreeString( m_pszCurElement );
                }
            }
        }
        return S_OK;
    }

    HRESULT SetData( PCWSTR pwszValue )
    {
        if(	m_pCurItem )
            m_pCurItem->SetData( pwszValue );
        return S_OK;
    }
};

#if 0
// Reserved 
HRESULT GetAttributes(IXmlReader* pReader)
{
    const WCHAR* pwszLocalName;
    const WCHAR* pwszValue;
    HRESULT hr;
    hr = pReader->MoveToAttributeByName ( L"Option", NULL );
    if( hr == S_OK )
    {
        pReader->GetLocalName(&pwszLocalName, NULL);
        pReader->GetValue(&pwszValue, NULL);
   }
   return hr;
}
#endif

static
HRESULT
ParseXMLData(
    IXmlReader *pReader,
    IStream *pStream,
    SRDELAYED_RECORDS *psrd
    )
{
    HRESULT hr;
    XmlNodeType nodeType;
    const WCHAR* pwszLocalName;
    const WCHAR* pwszValue;

    while( S_OK == (hr = pReader->Read(&nodeType)) )
    {
        switch( nodeType )
        {
            case XmlNodeType_Element:
                hr = pReader->GetLocalName(&pwszLocalName, NULL);
                if( FAILED(hr) )
                    break;

                // Note that when writing elements, an XmlNodeType_EndElement node 
                // is not generated for empty element start tags.
                //
                // If the method is not applicable, it will return FALSE.
                // 
                // You should save the value of IsEmptyElement before processing attributes,
                // or call MoveToElement to make IsEmptyElement valid after processing attributes.
                // 
                // IsEmptyElement returns FALSE when XmlReader is positioned on an attribute node,
                // even if attribute's parent element is empty.
#if 0
                if( FAILED(hr = GetAttributes(pReader)) )
                    break;

                if( FAILED(hr = pReader->MoveToElement()) )
                    break;
#endif
                hr = psrd->BeginElement( pwszLocalName );

                if( FAILED(hr) )
                    break;
                break;

            case XmlNodeType_EndElement:
                hr = pReader->GetLocalName(&pwszLocalName, NULL);
                if( FAILED(hr) )
                    break;

                hr = psrd->EndElement( pwszLocalName );
                if( FAILED(hr) )
                    break;
                break;

            case XmlNodeType_Text:
                hr = pReader->GetValue(&pwszValue, NULL);
                if( FAILED(hr) )
                    break;

                hr = psrd->SetData( pwszValue );
                if( FAILED(hr) )
                    break;
                break;
        }

        if( FAILED(hr) )
            break;
    }

    if( SUCCEEDED(hr) )
        hr = S_OK;

    return hr;
}

HRESULT
XMLParseFile(
    PCWSTR pszFile,
    CSRItemArray **pSRItemArray
    )
{
    HRESULT hr = E_FAIL;
    IXmlReader *pReader = NULL;
    IStream *pStream = NULL;

    try
    {
        hr = FileStream::OpenFile(pszFile,&pStream,FALSE);
        if( FAILED(hr) )
            throw hr;

        hr = CreateXmlReader(__uuidof(IXmlReader),(void**)&pReader,NULL);
        if( FAILED(hr) )
            throw hr;

        hr = pReader->SetInput(pStream);
        if( FAILED(hr) )
            throw hr;

        CSRItemArray *pItemArray = new CSRItemArray();
        if( pItemArray == NULL )
            throw E_OUTOFMEMORY;

        SRDELAYED_RECORDS records(pItemArray);

        hr = ParseXMLData(pReader,pStream,&records);
        if( FAILED(hr) )
        {
            delete pItemArray;
            throw hr;
        }

        *pSRItemArray = pItemArray;
    }	
    catch( HRESULT _hr )
    {
        hr = _hr;
    }

    if( pReader )
        pReader->Release(); // release with IStream

    if( pStream )
        pStream->Release();

    return hr;
}
