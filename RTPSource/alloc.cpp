//------------------------------------------------------------------------------
// File: alloc.cpp
//
// Desc: Main and Sub Allocators sharing same memory buffers
//
// Copyright (c) 1990-2002 Morgan Multimedia.  All rights reserved.
//------------------------------------------------------------------------------
#include <streams.h>
#include "alloc.h"

CMainAllocator::CMainAllocator(
    LPUNKNOWN  pUnk,
    HRESULT   *phr
) : CMemAllocator(TEXT("CMain Allocator"), pUnk, phr),
	m_pFree(NULL),
	m_Free(0),
	m_pReceived(NULL),
	m_pDelivered(NULL),
	m_MaxReceived(0)
{
}

CMainAllocator::~CMainAllocator()
{
}

HRESULT CMainAllocator::Alloc()
{
    CAutoLock lck(this);

    HRESULT hr = CMemAllocator::Alloc();

    if (S_OK == hr) {
        ASSERT(m_lCount == m_lFree.GetCount());

        /* Find the first */
        CMediaSample *pSample = m_lFree.Head();

        m_pBuffer = (PBYTE)(DWORD)-1;
        for (; pSample != NULL; pSample = m_lFree.Next(pSample)) {
            PBYTE pbTemp;
            pSample->GetPointer(&pbTemp);
            if (m_pBuffer > pbTemp) {
                m_pBuffer = pbTemp;
            }
        }
    }
    return hr;
}
