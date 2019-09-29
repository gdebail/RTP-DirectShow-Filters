//------------------------------------------------------------------------------
// File: alloc.h
//
// Desc: Main and Sub Allocators sharing same memory buffers
//
// Copyright (c) 1990-2002 Morgan Multimedia.  All rights reserved.
//------------------------------------------------------------------------------
typedef CMediaSample *LPCMEDIASAMPLE;

class CMainAllocator : public CMemAllocator
{
public:
    CMainAllocator(
        LPUNKNOWN  pUnk,
        HRESULT   *phr
    );

    ~CMainAllocator();

    HRESULT Alloc();

	LONG GetSize() {return m_lSize * m_lCount;}
  
	/*  Get the Free part */
    PBYTE GetBuffer(LONG *plFree)
	{
		CAutoLock cAutoLock(&m_cLock);

		if (m_pFree == NULL)
		{
			m_pFree = m_pBuffer;
			m_pReceived = m_pBuffer;
			m_pDelivered = m_pBuffer;
			m_Free = GetSize();
		}

		if (m_Free < m_MaxReceived)
		{
			*plFree = 0;
			return NULL;
		}

		ASSERT(m_Free >= m_MaxReceived);

		*plFree = m_Free;
		return m_pFree;
	}

	// Advance Received part
    PBYTE Received(LONG Received)
	{
		CAutoLock cAutoLock(&m_cLock);

		m_pReceived = m_pFree;
		m_pFree += Received;
		m_Free -= Received;

		// Keep the size of the largest block received
		if (Received > m_MaxReceived)
			m_MaxReceived = Received;

		// Should we wrap around ?
		if (m_Free < m_MaxReceived)
		{
			m_pFree = m_pBuffer;
			m_Free = m_pDelivered - m_pFree;
		}

		return m_pReceived;
	}

	// Advance Delivered part
    PBYTE Delivered(IMediaSample *pSample)
	{
		CAutoLock cAutoLock(&m_cLock);

		pSample->GetPointer(&m_pDelivered);
		m_pDelivered += pSample->GetSize();

		if (m_pFree < m_pDelivered)
			m_Free = m_pDelivered - m_pFree;
		else
			m_Free = m_pBuffer + GetSize() - m_pFree;

		return m_pDelivered;
	}

private:
    /*  Simple wrap around buffer stuff */
	CCritSec		m_cLock;
    PBYTE           m_pFree;
	LONG			m_Free;
    PBYTE           m_pReceived;
    PBYTE           m_pDelivered;
	LONG			m_MaxReceived;
};

/*  Allocator for subsamples */
class CSubAllocator : public CBaseAllocator
{
public:
    CSubAllocator(
        LPUNKNOWN  pUnk,
        HRESULT   *phr,
        CMainAllocator *pAlloc
    ) : CBaseAllocator(NAME("CSubAllocator"), pUnk, phr),
        m_pAlloc(pAlloc)
    {
    }

    ~CSubAllocator()
	{
		m_pAlloc->Decommit();
		m_pAlloc->Release();
	}


    CMediaSample *GetSample(DWORD dwLen)
    {
		PBYTE pbData = m_pAlloc->Received(dwLen);

        HRESULT hr = S_OK;
        CMediaSample *pSample = new CMediaSample(
                                        NAME("CMediaSample"),
                                        this,
                                        &hr,
                                        pbData,
                                        dwLen);
        if (pSample != NULL) 
		{
            /*  AddRef() ourselves too to conform to the rules */
            pSample->AddRef();

            /*  Make sure WE don't go away too ! */
            AddRef();
        }
        return pSample;
    }

    STDMETHODIMP ReleaseBuffer(IMediaSample * pSample)
    {
		/* Release main allocator buffer space for this sample */
		m_pAlloc->Delivered(pSample);

        /*  Free the sample */
        CMediaSample *pMediaSample = (CMediaSample *)pSample;
        delete pMediaSample;

        Release();
        return NOERROR;
    }


    /*  Must override Free() */
    void Free() {}

	CMainAllocator *Allocator() {return m_pAlloc;}

private:
    CMainAllocator *const m_pAlloc;
};
