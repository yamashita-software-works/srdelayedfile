#pragma once

#include <commctrl.h>

template<class T>
class CDynamicPointerArray
{
    HDPA m_hdpa;

public:
    CDynamicPointerArray(int Grow=8,HANDLE hHeap=NULL)
    {
        m_hdpa = DPA_CreateEx(Grow,hHeap);
    }

    ~CDynamicPointerArray()
    {
        DPA_Destroy(m_hdpa);
    }

    int AddPtr(T* ptr)
    {
        return DPA_AppendPtr(m_hdpa,ptr);
    }

    T* GetPtr(int iIndex)
    {
        return (T *)DPA_GetPtr(m_hdpa,iIndex);
    }

    int GetCount()
    {
        return DPA_GetPtrCount(m_hdpa);
    }

    BOOL DeleteAll()
    {
        return DPA_DeleteAllPtrs(m_hdpa);
    }

    BOOL RemoveAll(int iIndex)
    {
        return DPA_DeletePtr(m_hdpa,iIndex);
    }

#if 0 // If you no problem that only supports HDPA.
    CDynamicPointerArray(const CDynamicPointerArray<T>& dpa)
    {
        m_hdpa = DPA_Clone(dpa,NULL);
    }

    operator HDPA() const throw()
    {
        return (const HDPA)m_hdpa;
    }
#endif
};
