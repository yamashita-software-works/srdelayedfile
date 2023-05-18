#pragma once

#include <winternl.h>
#include <shlwapi.h>
#include <strsafe.h>
#include <winerror.h>
#include <xmllite.h>
#include "mem.h"
#include "dynamicptrarray.h"

#define MAX_UNICODE_VOLUMENAME_LENGTH  (64)
#define MAX_UNICODE_PATH_LENGTH        (MAX_UNICODE_VOLUMENAME_LENGTH + 32768 + MAX_PATH)

__forceinline BOOL IsEqualName(PCWSTR s1,PCWSTR s2)
{
    return ( _wcsicmp( s1, s2 ) == 0 );
}

#define DupString(psz) _MemDupString(psz);
#define FreeString(psz) if( psz ) { _MemFree(psz); psz = NULL; }
#define AllocStringBuffer(cch)  _MemAllocStringBuffer(cch)

#define FACILITY_SRDF                   1024 
#define SRDF_E_XMLSYNTAX_ERROR          MAKE_HRESULT(SEVERITY_ERROR,FACILITY_SRDF,1)
#define SRDF_E_SRDELAYED_EXE_NOT_FOUND  MAKE_HRESULT(SEVERITY_ERROR,FACILITY_SRDF,2)
#define SRDF_E_OPERATIONFILE_EXISTS     MAKE_HRESULT(SEVERITY_ERROR,FACILITY_SRDF,3)

/////////////////////////////////////////////////////////////////////////////

enum {
    SRD_CMD_NONE = 0,
    SRD_CMD_MOVE = 1,
    SRD_CMD_DELETE = 2,
    SRD_CMD_SETSHORTNAME = 3,
};

typedef struct _SRDELAYED_ITEM
{
    ULONG Command;
    LPWSTR Param1;
    LPWSTR Param2;
    LPWSTR Param3;
    LPWSTR Param4;
} SRDELAYED_ITEM,*PSRDELAYED_ITEM;

typedef struct _SRDELAYED_RECORD_STR
{
    PWSTR pszParam1;
    PWSTR pszParam2;
    PWSTR pszParam3;
    PWSTR pszParam4;
} SRDELAYED_RECORD_STR;

#define WSTR_MOVEFILE         L"MoveFile"
#define WSTR_DELETEFILE       L"DeleteFile"
#define WSTR_SETFILESHORTNAME L"SetFileShortName"

#define WSTR_NOTEXECUTED      L"NotExecuted"

/////////////////////////////////////////////////////////////////////////////

enum {
    SR_ELEMENT_NONE = 0,
    SR_ELEMENT_MOVE,
    SR_ELEMENT_DELETE,
    SR_ELEMENT_SETSHORTNAME,
    SR_ELEMENT_PATH,
    SR_ELEMENT_SOURCEPATH,
    SR_ELEMENT_DESTINATIONPATH,
    SR_ELEMENT_SHORTNAME,
};

class CSRDelayedItem : public SRDELAYED_ITEM
{
protected:
    int m_element;

public:
    CSRDelayedItem()
    {
        Command = SRD_CMD_NONE;
        Param1 = NULL;
        Param2 = NULL;
        Param3 = NULL;
        Param4 = NULL;
        m_element = 0;
    }

    virtual ~CSRDelayedItem()
    {
        FreeString(Param1);
        FreeString(Param2);
        FreeString(Param3);
        FreeString(Param4);
    }

    virtual HRESULT SetElement(PCWSTR ElementName)
    {
        return S_FALSE;
    }

    virtual HRESULT SetData(PCWSTR ElementName)
    {
        return S_FALSE;
    }

    void operator=(const CSRDelayedItem& src) throw()
    {
        memcpy(this,&src,sizeof(SRDELAYED_ITEM));
    }

    operator PSRDELAYED_ITEM() throw()
    {
        return (PSRDELAYED_ITEM)this;
    }
};

enum {
    SECTION_NONE = 0,
    SECTION_COMMANDS = 1,
};

class CSRDelayedMoveItem : public CSRDelayedItem
{
public:
    CSRDelayedMoveItem()
    {
        Command = SRD_CMD_MOVE;
        Param1 = DupString( L"MoveFile" );
    }

    virtual HRESULT SetElement(PCWSTR ElementName)
    {
        if( IsEqualName( L"SourcePath", ElementName ) )
            m_element = SR_ELEMENT_SOURCEPATH;
        else if( IsEqualName( L"DestinationPath", ElementName ) )
            m_element = SR_ELEMENT_DESTINATIONPATH;
        else
            return S_FALSE;
        return S_OK;
    }

    virtual HRESULT SetData(PCWSTR pwszValue)
    {
        switch( m_element )
        {
            case SR_ELEMENT_SOURCEPATH:
                Param2 = DupString(pwszValue);
                break;
            case SR_ELEMENT_DESTINATIONPATH:
                Param3 = DupString(pwszValue);
                break;
            default:
                return S_FALSE;
        }
        return S_OK;
    }
};

class CSRDelayedDeleteItem : public CSRDelayedItem
{
public:
    CSRDelayedDeleteItem()
    {
        Command = SRD_CMD_DELETE;
        Param1 = DupString( L"DeleteFile" );
    }

    virtual HRESULT SetElement(PCWSTR ElementName)
    {
        if( IsEqualName( L"Path", ElementName ) )
            m_element = SR_ELEMENT_PATH;
        else
            return S_FALSE;
        return S_OK;
    }

    virtual HRESULT SetData(PCWSTR pwszValue)
    {
        switch( m_element )
        {
            case SR_ELEMENT_PATH:
                Param3 = DupString(pwszValue);
                break;
            default:
                return S_FALSE;
        }
        return S_OK;
    }
};

class CSRDelayedSetShortNameItem : public CSRDelayedItem
{
public:
    CSRDelayedSetShortNameItem()
    {
        Command = SRD_CMD_SETSHORTNAME;
        Param1 = DupString( L"SetFileShortName" );
    }

    virtual HRESULT SetElement(PCWSTR ElementName)
    {
        if( IsEqualName( L"ShortName", ElementName ) )
            m_element = SR_ELEMENT_SHORTNAME;
        else if( IsEqualName( L"Path", ElementName ) )
            m_element = SR_ELEMENT_PATH;
        else
            return S_FALSE;
        return S_OK;
    }

    virtual HRESULT SetData(PCWSTR pwszValue)
    {
        switch( m_element )
        {
            case SR_ELEMENT_SHORTNAME:
                Param2 = DupString(pwszValue);
                break;
            case SR_ELEMENT_PATH:
                Param3 = DupString(pwszValue);
                break;
            default:
                return S_FALSE;
        }
        return S_OK;
    }
};

class CSRItemArray : public CDynamicPointerArray<CSRDelayedItem>
{
public:
    CSRItemArray()
    {
    }

    CSRItemArray(CDynamicPointerArray<CSRDelayedItem>& s) : CDynamicPointerArray<CSRDelayedItem>(s)
    {
    }

    virtual ~CSRItemArray()
    {
        deleteAll();
    }

private:
    void deleteAll()
    {
        int i;
        int c = GetCount();
        for(i = 0; i < c; i++)
        {
            CSRDelayedItem *pItem = GetPtr(i);
            delete pItem;
        }
        DeleteAll();
    }
};

/////////////////////////////////////////////////////////////////////////////

HRESULT
XMLParseFile(
    PCWSTR pszFile,
    CSRItemArray **pSRItemArray
    );

HRESULT
CreateSRDelayedFile(
    PCWSTR pszFile,
    CSRItemArray *pSRItemArray,
    BOOL bForceOverwrite
    );

HRESULT
DumpSRDelayedFile(
    PCWSTR pszFile
    );

HRESULT
SetupSRDelayedFile(
    PCWSTR pszDelayedFile
    );

HRESULT
StatusSRDelayedFile(
    VOID
    );

HRESULT
PathExEscape(
    PCWSTR pszPath,
    PWSTR pszEscaped,
    INT cchEscaped
    );

VOID
PrintHelp(
    VOID
    );
