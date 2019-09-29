//==========================================================================;
//
//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
//  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
//  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
//  PURPOSE.
//
//  Copyright (c) 1996 - 1998  Microsoft Corporation.  All Rights Reserved.
//
//--------------------------------------------------------------------------;
/*  Allocator for sequential buffers
    Like CBaseAllocator BUT always allocates the next buffer
*/

typedef CMediaSample *LPCMEDIASAMPLE;

class CSequentialAllocator : public CMemAllocator
{
public:
    CSequentialAllocator(
        LPUNKNOWN  pUnk,
        HRESULT   *phr
    );

    ~CSequentialAllocator();

    STDMETHODIMP GetBuffer(
        IMediaSample **ppBuffer,
        REFERENCE_TIME * pStartTime,
        REFERENCE_TIME * pEndTime,
        DWORD dwFlags);

    HRESULT Alloc();

    /*  Get buffer index */
    int BufferIndex(PBYTE pbBuffer);

    /*  Given an address get the IMediaSample pointer -
        NB needs optimizing
    */
    CMediaSample *SampleFromBuffer(PBYTE pBuffer);

    /*  Add a buffer to the valid list */
    void AddBuffer(CMediaSample *pSample);

    /*  Step through valid data */
    HRESULT Advance(LONG lAdvance);

    /*  Get the valid part */
    PBYTE GetValid(LONG *plValid);

    /*  Wrap end to go back to start */
    HRESULT Wrap(void);

    /*  Flush the allocator - just discard all the data in it */
    void Flush();

    /*  Get the Free part */
    PBYTE GetBuffer(LONG *plFree)
	{
		if (m_pFree == NULL)
		{
			m_pFree = m_pBuffer;
			m_pReceived = m_pBuffer;
			m_pDelivered = m_pBuffer;
			m_Free = m_lSize * m_lCount;
		}

		ASSERT(m_Free > m_MaxReceived);

		*plFree = m_Free;
		return m_pFree;
	}

	// Advance Received part
    PBYTE Received(LONG Received)
	{
		m_pReceived = m_pFree;
		m_Received += Received;
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
		pSample->GetPointer(&m_pDelivered);

		if (m_pFree < m_pDelivered)
			m_Free = m_pDelivered - m_pFree;

		return m_pDelivered;
	}

private:
    PBYTE           m_pbNext;
    LPCMEDIASAMPLE *m_parSamples;

    /*  Simple wrap around buffer stuff */
    LONG            m_lValid;
    PBYTE           m_pbStartValid;
    PBYTE           m_pBuffer;  /* Copy of CMemAllocator's which is private */

    PBYTE           m_pFree;
	LONG			m_Free;
    PBYTE           m_pReceived;
	LONG			m_Received;
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
        CSequentialAllocator *pAlloc
    ) : CBaseAllocator(NAME("CSubAllocator"), pUnk, phr),
        m_pAlloc(pAlloc)
    {
    }

    ~CSubAllocator()
	{
		m_pAlloc->Decommit();
		m_pAlloc->Release();
	}


    CMediaSample *GetSample(PBYTE pbData, DWORD dwLen)
    {
        HRESULT hr = S_OK;
        CMediaSample *pSample = new CMediaSample(
                                        NAME("CMediaSample"),
                                        this,
                                        &hr,
                                        pbData,
                                        dwLen);
        if (pSample != NULL) {

            /*  We only need to lock the first buffer because
                the super allocator allocates samples sequentially
                so it can't allocate subsequent samples until
                the first one has been freed
            */
            //m_pAlloc->SampleFromBuffer(pbData)->AddRef();

            /*  AddRef() ourselves too to conform to the rules */
            pSample->AddRef();

            /*  Make sure WE don't go away too ! */
            AddRef();
        }
        return pSample;
    }

    STDMETHODIMP ReleaseBuffer(IMediaSample * pSample)
    {
        /*  Free the superallocator's buffer */
        CMediaSample *pMediaSample = (CMediaSample *)pSample;
        PBYTE pBuffer;
        pSample->GetPointer(&pBuffer);
        //m_pAlloc->SampleFromBuffer(pBuffer)->Release();
        delete pMediaSample;

        Release();
        return NOERROR;
    }


    /*  Must override Free() */
    void Free() {}

	CSequentialAllocator *Allocator() {return m_pAlloc;}

private:
    CSequentialAllocator *const m_pAlloc;
};


/*  Track samples in buffer */
