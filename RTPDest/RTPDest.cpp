//------------------------------------------------------------------------------
// File: RTPDest.cpp
//
// Desc: DirectShow sample code - implementation of a renderer that RTPDests
//       the samples it receives into a text file.
//
// Copyright (c) 1992 - 2000, Microsoft Corporation.  All rights reserved.
//------------------------------------------------------------------------------


// Summary
//
// We are a generic renderer that can be attached to any data stream that
// uses IMemInputPin data transport. For each sample we receive we write
// its contents including its properties into a RTPDest file. The file we
// will write into is specified when the RTPDest filter is created. GraphEdit
// creates a file open dialog automatically when it sees a filter being
// created that supports the IFileSinkFilter interface.
//
//
// Implementation
//
// Pretty straightforward really, we have our own input pin class so that
// we can override Receive, all that does is to write the properties and
// data into a raw data file (using the Write function). We don't keep
// the file open when we are stopped so the flags to the open function
// ensure that we open a file if already there otherwise we create it.
//
//
// Demonstration instructions
//
// Start GraphEdit, which is available in the SDK DXUtils folder. Drag and drop
// an MPEG, AVI or MOV file into the tool and it will be rendered. Then go to
// the filters in the graph and find the filter (box) titled "Video Renderer"
// This is the filter we will be replacing with the RTPDest renderer. Then click
// on the box and hit DELETE. After that go to the Graph menu and select the
// "Insert Filters", from the dialog box find and select the "RTPDest Filter".
//
// You will be asked to supply a filename where you would like to have the
// data RTPDested, the data we receive in this filter is RTPDested in text form.
// Then dismiss the dialog. Back in the graph layout find the output pin of
// the filter that used to be connected to the input of the video renderer
// you just deleted, right click and do "Render". You should see it being
// connected to the input pin of the RTPDest filter you just inserted.
//
// Click Pause and Run and then a little later stop on the GraphEdit frame and
// the data being passed to the renderer will be RTPDested into a file. Stop the
// graph and RTPDest the filename that you entered when inserting the filter into
// the graph, the data supplied to the renderer will be displayed as raw data
//
//
// Files
//
// RTPDest.cpp             Main implementation of the RTPDest renderer
// RTPDest.def             What APIs the DLL will import and export
// RTPDest.h               Class definition of the derived renderer
// RTPDest.rc              Version information for the sample DLL
// RTPDestuids.h           CLSID for the RTPDest filter
// makefile             How to build it...
//
//
// Base classes used
//
// CBaseFilter          Base filter class supporting IMediaFilter
// CRenderedInputPin    An input pin attached to a renderer
// CUnknown             Handle IUnknown for our IFileSinkFilter
// CPosPassThru         Passes seeking interfaces upstream
// CCritSec             Helper class that wraps a critical section
//
//

#include <windows.h>
#include <commdlg.h>
#include <streams.h>
#include <initguid.h>
#include <mmreg.h>
#include <time.h>
#include "MSVideoRTPSink.hh"

#include "RTPDestuids.h"
#include "RTPDest.h"
#include "RTPDestProp.h"

static unsigned nInstances = 0;
static char StopThread = NULL;
static HANDLE hRTPsndThread = NULL;

#ifdef BSD
static struct timezone Idunno;
#else
static int Idunno;
#endif

#define SAFE_DELETE(p) \
	if (p) delete (p); \
	p = NULL;

#define SAFE_CLOSE(p) \
	if (p) Medium::close(p); \
	p = NULL;


static const short rtpPortNumAudioBase = 6666;
static const short rtpPortNumVideoBase = 8888;
static const short nbPorts = (rtpPortNumVideoBase - rtpPortNumAudioBase) / 2;

static unsigned rtpAudioPortList[nbPorts];
static unsigned rtpVideoPortList[nbPorts];

void initPortLists() {
	ZeroMemory(rtpAudioPortList, nbPorts * sizeof(unsigned));
	ZeroMemory(rtpVideoPortList, nbPorts * sizeof(unsigned));
}

short getFreeAudioPort() {
	unsigned i = 0;
	while(i < nbPorts && rtpAudioPortList[i]) i++;
	rtpAudioPortList[i] = 1;
	return rtpPortNumAudioBase + i * 2;
}

short getFreeVideoPort() {
	unsigned i = 0;
	while(i < nbPorts && rtpVideoPortList[i]) i++;
	rtpVideoPortList[i] = 1;
	return rtpPortNumVideoBase + i * 2;
}

void releaseAudioPort(short port) {
	rtpAudioPortList[(port - rtpPortNumAudioBase) / 2] = 0;
}

void releaseVideoPort(short port) {
	rtpVideoPortList[(port - rtpPortNumVideoBase) / 2] = 0;
}

const AMOVIESETUP_MEDIATYPE sudPinTypes =
{
    &MEDIATYPE_NULL,            // Major type
    &MEDIASUBTYPE_NULL          // Minor type
};

const AMOVIESETUP_PIN sudPins =
{
    L"Input",                   // Pin string name
    FALSE,                      // Is it rendered
    FALSE,                      // Is it an output
    FALSE,                      // Allowed none
    FALSE,                      // Likewise many
    &CLSID_NULL,                // Connects to filter
    L"Output",                  // Connects to pin
    1,                          // Number of types
    &sudPinTypes                // Pin information
};

const AMOVIESETUP_FILTER sudRTPDest =
{
    &CLSID_RTPDest,                // Filter CLSID
    L"Morgan RTP Dest",            // String name
    MERIT_DO_NOT_USE,           // Filter merit
    1,                          // Number pins
    &sudPins                    // Pin details
};

//
//  Object creation stuff
//
CFactoryTemplate g_Templates[] = {
    {L"Morgan RTP Dest Filter"
	, &CLSID_RTPDest
	, CRTPDest::CreateInstance
	, NULL
	, &sudRTPDest}
	,
    {L"Morgan RTP Dest Properties"
    , &CLSID_RTPDestProp
    , CRTPDestProperties::CreateInstance}
};
int g_cTemplates = sizeof(g_Templates) / sizeof(g_Templates[0]);

// Constructor

CRTPDestFilter::CRTPDestFilter(CRTPDest *pRTPDest,
                         LPUNKNOWN pUnk,
                         CCritSec *pLock,
                         HRESULT *phr) :
    CBaseFilter(NAME("CRTPDestFilter"), pUnk, pLock, CLSID_RTPDest),
    m_pRTPDest(pRTPDest)
{
}

CRTPSample::CRTPSample(CMediaSample *pSample, AM_MEDIA_TYPE* pmt, HRESULT *phr) :
	CMediaSample(NULL, (CBaseAllocator *)1, NULL, NULL, 0)
{

    m_pBuffer =NULL;				// Initialise the buffer
    m_cbBuffer = 0;					// And it's length
    m_lActual = NULL;				// By default, actual = length
    m_pMediaType = NULL;             // No media type change
    m_dwFlags = 0;                   // Nothing set
    m_cRef = 0;                      // 0 ref count
    m_dwTypeSpecificFlags = 0;       // Type specific flags
    m_dwStreamId = AM_STREAM_MEDIA;  // Stream id
    m_pAllocator = NULL;        // Allocator*/

	m_lActual = m_cbBuffer = pSample->GetActualDataLength();

	if (!m_cbBuffer)
		*phr = S_FALSE;
	else
	{
		m_pBuffer = new BYTE[m_cbBuffer];
		LPBYTE pBuffer;
		pSample->GetPointer(&pBuffer);
		memcpy(m_pBuffer, pBuffer, m_cbBuffer);
	}

    REFERENCE_TIME   Start;           /* Start sample time */
    REFERENCE_TIME   End;             /* End sample time */
    LONGLONG         MediaStart;      /* Real media start position */
    LONGLONG         MediaEnd;        /* A difference to get the end */

	if (SUCCEEDED(pSample->GetTime(&Start, &End)))
		SetTime(&Start, &End);

	if (SUCCEEDED(pSample->GetMediaTime(&MediaStart, &MediaEnd)))
		SetMediaTime(&MediaStart, &MediaEnd);

	SetDiscontinuity(pSample->IsDiscontinuity());
	SetPreroll(pSample->IsPreroll());
	SetSyncPoint(pSample->IsSyncPoint());

	m_pmt = pmt;

	AddRef();

	*phr = S_OK;
}

STDMETHODIMP_(ULONG)
CRTPSample::Release()
{
    /* Decrement our own private reference count */
    LONG lRef;
    if (m_cRef == 1) {
        lRef = 0;
        m_cRef = 0;
    } else {
        lRef = InterlockedDecrement(&m_cRef);
    }
    ASSERT(lRef >= 0);

    DbgLog((LOG_MEMORY,3,TEXT("    CRTPSample %X ref-- = %d"),
        this, m_cRef));

    /* Did we release our final reference count */
    if (lRef == 0) {
        /* Free all resources */
        if (m_dwFlags & Sample_TypeChanged) {
            SetMediaType(NULL);
        }
        ASSERT(m_pMediaType == NULL);
        m_dwFlags = 0;
        m_dwTypeSpecificFlags = 0;
        m_dwStreamId = AM_STREAM_MEDIA;

        /* This may cause us to be deleted */
        // Our refcount is reliably 0 thus no-one will mess with us
        //m_pAllocator->ReleaseBuffer(this);
		delete [] m_pBuffer;
		delete this;
    }
    return (ULONG)lRef;
}

//
// GetPages
//
// Returns the clsid's of the property pages we support
//
STDMETHODIMP CRTPDest::GetPages(CAUUID *pPages)
{
    pPages->cElems = 1;
    pPages->pElems = (GUID *) CoTaskMemAlloc(sizeof(GUID) * pPages->cElems);
    if (pPages->pElems == NULL) {
        return E_OUTOFMEMORY;
    }
    pPages->pElems[0] = CLSID_RTPDestProp;    
    return NOERROR;

} // GetPages

//
// GetPin
//
CBasePin * CRTPDestFilter::GetPin(int n)
{
    if (n == 0) {
        return m_pRTPDest->m_pVideoPin;
    } else if (n == 1) {
        return m_pRTPDest->m_pAudioPin;
    } else {
        return NULL;
    }
}


//
// GetPinCount
//
int CRTPDestFilter::GetPinCount()
{
    return 2;
}


//
// Stop
//
// Overriden to close the RTPDest file
//
STDMETHODIMP CRTPDestFilter::Stop()
{
    CAutoLock cObjectLock(m_pLock);
    return CBaseFilter::Stop();
}


//
// Pause
//
// Overriden to open the RTPDest file
//
STDMETHODIMP CRTPDestFilter::Pause()
{
    CAutoLock cObjectLock(m_pLock);
    // m_pRTPDest->OpenFile();
    return CBaseFilter::Pause();
}


//
// Run
//
// Overriden to open the RTPDest file
//
STDMETHODIMP CRTPDestFilter::Run(REFERENCE_TIME tStart)
{
    CAutoLock cObjectLock(m_pLock);
    return CBaseFilter::Run(tStart);
}


//
//  Definition of CRTPDestInputPin
//
CRTPDestInputPin::CRTPDestInputPin(TCHAR *pObjectName,
			                 LPCWSTR pName,
							 CRTPDest *pRTPDest,
                             LPUNKNOWN pUnk,
                             CBaseFilter *pFilter,
                             CCritSec *pLock,
                             CCritSec *pReceiveLock,
                             HRESULT *phr) :

    CRenderedInputPin(pObjectName,
                  pFilter,                   // Filter
                  pLock,                     // Locking
                  phr,                       // Return code
                  pName),                    // Pin name
    m_pPosition(NULL),
	m_pLock(pLock),
    m_pReceiveLock(pReceiveLock),
    m_pRTPDest(pRTPDest),
    m_tLast(0),
	m_tStart(0),
	m_SampleList(NULL),
	m_Buffered(0),
	m_FirstTime(0)
{
	m_SampleList = new CRTPSampleList(NAME("Received Sample List"), IsAudio() ? 128 : 256 , TRUE);
}

CRTPDestInputPin::~CRTPDestInputPin()
{
	if (m_SampleList != NULL)
	{
		IMediaSample* pSample = m_SampleList->RemoveHead();
		while (pSample != NULL)
		{
			pSample->Release();
			pSample = m_SampleList->RemoveHead();
		}
	}
	SAFE_DELETE(m_SampleList);
	SAFE_DELETE(m_pPosition);
}

//
// NonDelegatingQueryInterface
//
// Override this to say what interfaces we support where
//
STDMETHODIMP CRTPDestInputPin::NonDelegatingQueryInterface(REFIID riid, void ** ppv)
{
    CheckPointer(ppv,E_POINTER);
    CAutoLock lock(m_pLock);

    // Do we have this interface
	 if (riid == IID_IMediaPosition || riid == IID_IMediaSeeking) 
	 {
        if (m_pPosition == NULL) 
		{

            HRESULT hr = S_OK;
            m_pPosition = new CPosPassThru(NAME("RTPDest Pass Through"),
                                           (IUnknown *) GetOwner(),
                                           (HRESULT *) &hr, this);
            if (m_pPosition == NULL) 
			{
                return E_OUTOFMEMORY;
            }

            if (FAILED(hr)) 
			{
                delete m_pPosition;
                m_pPosition = NULL;
                return hr;
            }
        }
        return m_pPosition->NonDelegatingQueryInterface(riid, ppv);
    } else {
		return CRenderedInputPin::NonDelegatingQueryInterface(riid, ppv);
    }

} // NonDelegatingQueryInterface

//
// CheckMediaType
//
// Check if the pin can support this specific proposed type and format
//
HRESULT CRTPDestInputPin::CheckMediaType(const CMediaType *pMediaType)
{

	if ((IsAudio() && pMediaType->majortype == MEDIATYPE_Audio) ||
		(IsVideo() && pMediaType->majortype == MEDIATYPE_Video) )
		CopyMediaType(&m_mt, pMediaType);
	else
	    return S_FALSE;  // This format is not acceptable.

    return S_OK;  // This format is acceptable.
}


//
// BreakConnect
//
// Break a connection
//
HRESULT CRTPDestInputPin::BreakConnect()
{
    if (m_pPosition != NULL) {
        m_pPosition->ForceRefresh();
    }
    return CRenderedInputPin::BreakConnect();
}


//
// ReceiveCanBlock
//
//
STDMETHODIMP CRTPDestInputPin::ReceiveCanBlock()
{
    //return S_FALSE; // We don't hold up source threads on Receive
    return S_OK; // We might up source threads on Receive
}

//
// Receive
//
// Do something with this media sample
//
STDMETHODIMP CRTPDestInputPin::Receive(IMediaSample *pSample)
{
    CAutoLock lock(m_pReceiveLock);
 
    REFERENCE_TIME tStart, tStop;
    pSample->GetTime(&tStart, &tStop);
    DbgLog((LOG_TRACE, 1, TEXT("tStart(%s), tStop(%s), Diff(%d ms), Bytes(%d)"),
           (LPCTSTR) CDisp(tStart),
           (LPCTSTR) CDisp(tStop),
           (LONG)((tStart - m_tLast) / 10000),
           pSample->GetActualDataLength()));

    m_tLast = tStart;

    return S_OK;
}

DSPushRTPSink* CRTPDestVideo::GetSink() 
{
	return m_pRTPDest && m_pRTPDest->videoStream ? m_pRTPDest->videoStream->GetSink() : NULL;
}

STDMETHODIMP CRTPDestVideo::Receive(IMediaSample *pSample)
{
try {
	DWORD inTime = timeGetTime();
	if (m_FirstTime == 0)
	{
		m_FirstTime = inTime;
		m_CurTime = inTime;
	}

    //CAutoLock lock(m_pReceiveLock);

	long lData = pSample->GetActualDataLength();
	DbgLog((LOG_TRACE, 3, "Sending vid at %d ms Bytes(%d)...", 
		inTime,
		lData));
	HRESULT hr;
    REFERENCE_TIME tStart, tStop;

	ASSERT(pSample != NULL);

#ifdef DEBUG
	//WriteStringInfo(pSample);
#endif

	VIDEOINFOHEADER *pvi = (VIDEOINFOHEADER *)m_mt.Format();

	hr = pSample->GetTime(&tStart, &tStop);
    if (SUCCEEDED(hr))
	{
		DbgLog((LOG_TRACE, 1, TEXT("vid tStart(%I64d 탎), tStop(%I64d 탎), Diff(%d ms), Bytes(%d), Buffered(%I64d)"),
			   tStart / 10,
			   tStop / 10,
			   (LONG)((tStart - m_tLast) / 10000),
			   lData,
			   m_Buffered));
	    m_tLast = tStart;
	}

	if (pvi->dwBitRate != 0) // based on bitrate
	{
		m_CurTime += (lData * 8000) / pvi->dwBitRate;
	}
	else // based on framerate
	{
		m_CurTime += (DWORD)(tStop - tStart) / 10000;
	}

	MSVideoRTPSink::onReceive(GetSink(), (AM_MEDIA_TYPE*)&m_mt, pSample);
	// Hack
	//((BasicTaskScheduler&)GetSink()->envir().taskScheduler()).SingleStep();

	DWORD sendTime = timeGetTime() - inTime;
	
	DWORD wait = 0;
	DWORD now = timeGetTime();
	if (m_CurTime > now)
		wait = m_CurTime - now;

	if (wait > 0)
		Sleep(wait);

	DbgLog((LOG_TRACE, 3, "...vid sent at %d ms Processing(%d ms) Wait(%d ms)", 
		timeGetTime(), sendTime, wait));

    //return m_pRTPDest->WriteVideo(pSample);
	return S_OK;
}
catch (...)
{
	return S_FALSE;
}
}

DSPushRTPSink* CRTPDestAudio::GetSink() 
{
	return m_pRTPDest && m_pRTPDest->audioStream ? m_pRTPDest->audioStream->GetSink() : NULL;
}

STDMETHODIMP CRTPDestAudio::Receive(IMediaSample *pSample)
{
try {
	ASSERT(pSample != NULL);
	DWORD inTime = timeGetTime();
	if (m_FirstTime == 0)
	{
		m_FirstTime = inTime;
		m_CurTime = inTime;
	}

    //CAutoLock lock(m_pReceiveLock);

	long lData = pSample->GetActualDataLength();
	DbgLog((LOG_TRACE, 3, "Sending aud at %d ms Bytes(%d)...", 
		inTime,
		lData));

#ifdef DEBUG
	//WriteStringInfo(pSample);
#endif

	WAVEFORMATEX *pwf = (WAVEFORMATEX *) m_mt.Format();
	MPEGLAYER3WAVEFORMAT *pMp3 = NULL;
	if (pwf->wFormatTag == WAVE_FORMAT_MPEGLAYER3)
	{
		pMp3 = (MPEGLAYER3WAVEFORMAT *)pwf;
	}

	HRESULT hr;
    REFERENCE_TIME tStart, tStop;
	hr = pSample->GetTime(&tStart, &tStop);
    if (SUCCEEDED(hr))
	{
		DbgLog((LOG_TRACE, 1, TEXT("aud tStart(%I64d 탎), tStop(%I64d 탎), Diff(%d ms), Bytes(%d), Buffered(%I64d)"),
			   tStart / 10,
			   tStop / 10,
			   (LONG)((tStart - m_tLast) / 10000),
			   lData,
			   m_Buffered));
	    m_tLast = tStart;
		if (m_tStart == 0)
			m_tStart = m_tLast;

		if (pMp3 != NULL)
		{
			// printf
			DbgLog((LOG_TRACE, 1, TEXT("aud MP3 Flags(0x%X), BlockSize(%hd), FramesPerBlock(%hd), CodecDelay(%hd)"),
					pMp3->fdwFlags,
					pMp3->nBlockSize,
					pMp3->nFramesPerBlock,
					pMp3->nCodecDelay));
		}

	}

	m_CurTime += (lData * 1000) / pwf->nAvgBytesPerSec;

	MSVideoRTPSink::onReceive(GetSink(), (AM_MEDIA_TYPE*)&m_mt, pSample);	
	// Hack
	//((BasicTaskScheduler&)GetSink()->envir().taskScheduler()).SingleStep();

	DWORD sendTime = timeGetTime() - inTime;
	
	DWORD wait = 0;
	DWORD now = timeGetTime();
	if (m_CurTime > now)
		wait = m_CurTime - now;

	if (wait > 0)
		Sleep(wait);

	DbgLog((LOG_TRACE, 3, "...aud sent at %d ms Processing(%d ms) Wait(%d ms)", 
		timeGetTime(), sendTime, wait));

	return S_OK;
	//return m_pRTPDest->WriteAudio(pSample);
}
catch (...)
{
	return S_FALSE;
}
}

#ifdef DEBUG
//
// RTPDestStringInfo
//
// Write to the file as text form
//
HRESULT CRTPDestInputPin::WriteStringInfo(IMediaSample *pSample)
{
    TCHAR DbgString[256];
    PBYTE pbData;
	HRESULT hr;

	wsprintf(DbgString,"Renderer received %s sample (%d ms)",
		IsAudio() ? "audio" : "video", timeGetTime() - m_FirstTime);
	DbgLog((LOG_TRACE,3,DbgString));

    // Retrieve the time stamps from this sample

    REFERENCE_TIME tStart, tStop;
    hr = pSample->GetTime(&tStart, &tStop);
	if (hr == NOERROR) { 

		// Write the sample time stamps out
		wsprintf(DbgString,"   Start time (%I64d 탎)", tStart / 10);
		DbgLog((LOG_TRACE,3,DbgString));
		wsprintf(DbgString,"   End time (%I64d 탎)", tStop / 10);
		DbgLog((LOG_TRACE,3,DbgString));
	}

    // Display the media times for this sample

    hr = pSample->GetMediaTime(&tStart, &tStop);
    if (hr == NOERROR) {
        wsprintf(DbgString,"   Start media time (%I64d)", tStart);
        DbgLog((LOG_TRACE,3,DbgString));
        wsprintf(DbgString,"   End media time (%I64d)", tStop);
        DbgLog((LOG_TRACE,3,DbgString));
    }

    // Is this a sync point sample

    hr = pSample->IsSyncPoint();
    wsprintf(DbgString,"   Sync point (%d)",(hr == S_OK));
	if (hr == S_OK) DbgLog((LOG_TRACE,3,DbgString));

    // Is this a preroll sample

    hr = pSample->IsPreroll();
    wsprintf(DbgString,"   Preroll (%d)",(hr == S_OK));
	if (hr == S_OK) DbgLog((LOG_TRACE,3,DbgString));

    // Is this a discontinuity sample

    hr = pSample->IsDiscontinuity();
    wsprintf(DbgString,"   Discontinuity (%d)",(hr == S_OK));
	if (hr == S_OK) DbgLog((LOG_TRACE,3,DbgString));

    // Write the actual data length

    LONG DataLength = pSample->GetActualDataLength();
    wsprintf(DbgString,"   Actual data length (%d)",DataLength);
    DbgLog((LOG_TRACE,3,DbgString));

    // Does the sample have a type change aboard

    AM_MEDIA_TYPE *pMediaType;
    pSample->GetMediaType(&pMediaType);
    wsprintf(DbgString,"   Type changed (%d)",
        (pMediaType ? TRUE : FALSE));
    if (pMediaType) DbgLog((LOG_TRACE,3,DbgString));
    DeleteMediaType(pMediaType);

    // Copy the data to the file

    hr = pSample->GetPointer(&pbData);
    if (FAILED(hr)) {
        return hr;
    }

#ifdef DUMP
    // Write each complete line out in BYTES_PER_LINES groups
    TCHAR TempString[256];

    for (int Loop = 0;Loop < (DataLength / BYTES_PER_LINE);Loop++) {
        wsprintf(DbgString,FIRST_HALF_LINE,pbData[0],pbData[1],pbData[2],
                 pbData[3],pbData[4],pbData[5],pbData[6],
                    pbData[7],pbData[8],pbData[9]);
        wsprintf(TempString,SECOND_HALF_LINE,pbData[10],pbData[11],pbData[12],
                 pbData[13],pbData[14],pbData[15],pbData[16],
                    pbData[17],pbData[18],pbData[19]);
        lstrcat(DbgString,TempString);
        DbgLog((LOG_TRACE,3,DbgString));
        pbData += BYTES_PER_LINE;
    }

    // Write the last few bytes out afterwards

    wsprintf(DbgString,"   ");
    for (Loop = 0;Loop < (DataLength % BYTES_PER_LINE);Loop++) {
        wsprintf(TempString,"%x ",pbData[Loop]);
        lstrcat(DbgString,TempString);
    }
    DbgLog((LOG_TRACE,3,DbgString));
#endif

    return NOERROR;
}
#endif

//
// EndOfStream
//
STDMETHODIMP CRTPDestInputPin::EndOfStream(void)
{
    CAutoLock lock(m_pReceiveLock);

	return CRenderedInputPin::EndOfStream();
} 

STDMETHODIMP CRTPDestVideo::EndOfStream(void)
{
    CAutoLock lock(m_pReceiveLock);

	if (!m_pRTPDest->m_pAudioPin->IsConnected() || m_pRTPDest->m_pAudioPin->m_bAtEndOfStream)
	{
		return CRenderedInputPin::EndOfStream();
	}

	m_bAtEndOfStream = TRUE;

	return S_FALSE;
}

STDMETHODIMP CRTPDestAudio::EndOfStream(void)
{
    CAutoLock lock(m_pReceiveLock);

	if (!m_pRTPDest->m_pVideoPin->IsConnected() || m_pRTPDest->m_pVideoPin->m_bAtEndOfStream)
	{
		return CRenderedInputPin::EndOfStream();
	}

	m_bAtEndOfStream = TRUE;

	return S_FALSE;
}
// EndOfStream

//
// NewSegment
//
// Called when we are seeked
//
STDMETHODIMP CRTPDestInputPin::NewSegment(REFERENCE_TIME tStart,
                                       REFERENCE_TIME tStop,
                                       double dRate)
{
    m_tLast = 0;
	m_tStart = 0;
    m_tSegStart = tStart;
    m_tSegStop = tStop;
	m_dSegRate = dRate;
    return S_OK;

} // NewSegment


//
//  CRTPDest class
//

CRTPDest::CRTPDest(LPUNKNOWN pUnk, HRESULT *phr) :
    CUnknown(NAME("CRTPDest"), pUnk),
    CStreamingSession(),
    m_pFilter(NULL),
    m_pVideoPin(NULL),
    m_pAudioPin(NULL),
	m_pPosition(NULL),
	m_block_size(0)
{

	m_frame_rate = 0.0;
	m_frame_count = 0;
	m_rcvd_block_count = 0;
	m_rcvd_block_size = 0;
	m_block_count = 0;
	m_vid_total_size = 0;
	m_aud_total_size = 0;
	m_frame_size = 0;
	m_processing_time = 0.0;

    timeBeginPeriod(1);

    m_pFilter = new CRTPDestFilter(this, GetOwner(), &m_Lock, phr);
    if (m_pFilter == NULL) {
        *phr = E_OUTOFMEMORY;
        return;
    }

    m_pVideoPin = new CRTPDestVideo(this, GetOwner(),
                               m_pFilter,
                               &m_Lock,
                               &m_ReceiveLock,
                               phr);
    if (m_pVideoPin == NULL) {
        *phr = E_OUTOFMEMORY;
        return;
    }

    m_pAudioPin = new CRTPDestAudio(this, GetOwner(),
                               m_pFilter,
                               &m_Lock,
                               &m_ReceiveLock,
                               phr);
    if (m_pAudioPin == NULL) {
        *phr = E_OUTOFMEMORY;
        return;
    }
}

// Destructor
CRTPDest::~CRTPDest()
{
    SAFE_DELETE(m_pPosition);
    SAFE_DELETE(m_pAudioPin);
    SAFE_DELETE(m_pVideoPin);
    SAFE_DELETE(m_pFilter);

    timeEndPeriod(1);
}


//
// CreateInstance
//
// Provide the way for COM to create a RTPDest filter
//
CUnknown * WINAPI CRTPDest::CreateInstance(LPUNKNOWN punk, HRESULT *phr)
{
#ifdef DEBUG
	Socket::DebugLevel = 1;
#endif
    CRTPDest *pNewObject = new CRTPDest(punk, phr);
    if (pNewObject == NULL) {
        *phr = E_OUTOFMEMORY;
    }
    return pNewObject;

} // CreateInstance

//
// NonDelegatingQueryInterface
//
// Override this to say what interfaces we support where
//
STDMETHODIMP CRTPDest::NonDelegatingQueryInterface(REFIID riid, void ** ppv)
{
    CheckPointer(ppv,E_POINTER);
    CAutoLock lock(&m_Lock);

    // Do we have this interface

    if (riid == IID_ISpecifyPropertyPages) {
        return GetInterface((ISpecifyPropertyPages *) this, ppv);
    } else if (riid == IID_IStreamingSession) {
        return GetInterface((IStreamingSession *) this, ppv);
    } else if (riid == IID_IBaseFilter || riid == IID_IMediaFilter || riid == IID_IPersist) {
		return m_pFilter->NonDelegatingQueryInterface(riid, ppv);
	} else if (riid == IID_IMediaPosition || riid == IID_IMediaSeeking) 
	 {
		CRTPDestInputPin *pPin = NULL;
		if (m_pVideoPin->IsConnected())
			pPin = m_pVideoPin;
		else if (m_pAudioPin->IsConnected())
			pPin = m_pAudioPin;

        if (!pPin) 
    		return CUnknown::NonDelegatingQueryInterface(riid, ppv);

		if (m_pPosition == NULL) 
		{

			HRESULT hr = S_OK;
			m_pPosition = new CPosPassThru(NAME("RTPDest Pass Through"),
										   (IUnknown *) GetOwner(),
										   (HRESULT *) &hr, pPin);
			if (m_pPosition == NULL) 
			{
				return E_OUTOFMEMORY;
			}

			if (FAILED(hr)) 
			{
				delete m_pPosition;
				m_pPosition = NULL;
				return hr;
			}
		}
		return m_pPosition->NonDelegatingQueryInterface(riid, ppv);

	} else {
		return CUnknown::NonDelegatingQueryInterface(riid, ppv);
    }

} // NonDelegatingQueryInterface


HRESULT CRTPDest::SetTimescaleFramePeriod(REFERENCE_TIME AvgTimePerFrame)
{
		m_frame_rate = (double)(10000000.0 / AvgTimePerFrame);

		/*m_frame_rate += 0.005;
		m_frame_rate = ((double)(int)(m_frame_rate * 100.0)) / 100.0;*/

		// 30000 / 1001 = 29.97
		if (m_frame_rate >= 29.97 && m_frame_rate < 29.98)
		{
			m_timescale = 30000;
			m_frame_period = 1001;
		}
		// 1000 / 40 = 25 fps
		else if (m_frame_rate > 24.9 && m_frame_rate <= 25.0)
		{
			m_timescale = 1000;
			m_frame_period = 40;
		}
		// 600 / 25 = 24 fps
		else if (m_frame_rate > 23.9 && m_frame_rate <= 24.0)
		{
			m_timescale = 600;
			m_frame_period = 25;
		}
		// 1000000 / 66776 = 14.97 fps
		else if (m_frame_rate >= 14.97 && m_frame_rate < 14.98)
		{
			m_timescale = 1000000;
			m_frame_period = 66776;
		}
		// 1000000 / AvgTimePerFrame = xx.xx fps
		else
		{
			m_timescale = 10000000;
			m_frame_period = (UINT32)AvgTimePerFrame;
		}

		return 1;
}

//
// DllRegisterSever
//
// Handle the registration of this filter
//
STDAPI DllRegisterServer()
{
    return AMovieDllRegisterServer2( TRUE );

} // DllRegisterServer


//
// DllUnregisterServer
//
STDAPI DllUnregisterServer()
{
    return AMovieDllRegisterServer2( FALSE );

} // DllUnregisterServer

/*DWORD WINAPI RTPsndThread(LPVOID pv)
{
  StopThread = 0;

  env->taskScheduler().blockMyself(&StopThread); // does not return til StopThread != 0

  hRTPsndThread = NULL;
  StopThread = 0;

  SAFE_DELETE(scheduler);
  SAFE_DELETE(env);

  return 0;
}*/

STDMETHODIMP CRTPDest::CreateMediaSession(void *clientData, const char* destinationAddressStr, int audPortBase, int vidPortBase)
{
  if (++nInstances == 1)
  {
	  /*StopThread = 1;

	  while (hRTPsndThread != NULL);

	  // Begin by setting up our usage environment:
	  if (scheduler == NULL)
		scheduler = BasicTaskScheduler::createNew();
	  if (env == NULL)
		env = BasicUsageEnvironment::createNew(*scheduler);*/

	  initPortLists();
  }

  m_env = (BasicUsageEnvironment*)clientData;

  serverMediaSession = ServerMediaSession
   ::createNew(envir(), "Session streamed by \"Morgan Streaming Server\"", "http://www.morgan-multimedia.com");

  /*DWORD threadid;
  // Create 'RTP/RTCP sending' Working Thread
  if (hRTPsndThread == NULL)
  {
 	hRTPsndThread = CreateThread(NULL,
				0,
				RTPsndThread,
				this,
				0,
				&threadid);

	//SetThreadPriority(hRTPsndThread,
	//		THREAD_PRIORITY_IDLE);
  }*/

  m_destinationAddress.s_addr = our_inet_addr((char*)destinationAddressStr);  
  m_audPortBase = audPortBase;
  m_vidPortBase = vidPortBase;


  // HACK
  return (HRESULT)serverMediaSession;
}

CStreamingSessionStream::CStreamingSessionStream(UsageEnvironment* env, struct in_addr destinationAddress, short portBase, unsigned char ttl) :
	fRTCPInstance(NULL),
	fRTPSink(NULL)
{

  Port rtpPort(portBase);
  Port rtcpPort(portBase + 1);

  m_env = env;

  fRTPSocket = new Groupsock(envir(), destinationAddress, rtpPort, ttl);
  fRTCPSocket = new Groupsock(envir(), destinationAddress, rtcpPort, ttl);
}

void CStreamingSessionStream::Open(DSPushRTPSink* snk, unsigned totalSessionBandwidth, char* trkID) {

  fRTPSink = snk;
  strcpy(fTrackID, trkID);

  // Create (and start) a 'RTCP instance' for this RTP fRTPSink:
  const unsigned maxCNAMElen = 100;
  unsigned char CNAME[maxCNAMElen+1];
  gethostname((char*)CNAME, maxCNAMElen);
  CNAME[maxCNAMElen] = '\0'; // just in case
  fRTCPInstance = new RTCPInstance(envir(), fRTCPSocket,
		  totalSessionBandwidth, CNAME,
		  fRTPSink, NULL /* we're a server */, 
		  False /* This won't starts RTCP running automatically */);
  // Note: 
}

STDMETHODIMP CRTPDest::Open(const char *urlSuffix) {

  // TODO : need to convert url suffix to local file name (maybe using a config file)
  const char *fileName = urlSuffix;

  char *stream = (char *)&urlSuffix[strlen(urlSuffix)];
  while (*--stream != '/' && stream > urlSuffix);
  stream++;

  // video only
  if (stricmp(stream, "video") == 0)
  {
	  if (m_pAudioPin->IsConnected())
	  {
		  m_pAudioPin->GetFilterGraph()->Disconnect(m_pAudioPin->GetConnected());
		  m_pAudioPin->GetFilterGraph()->Disconnect(m_pAudioPin);
	  }
	  stream--;
	  stream[0] = 0;
	  stream++;
  }

  // audio only
  if (stricmp(stream, "audio") == 0)
  {
	  if (m_pVideoPin->IsConnected())
	  {
		  m_pVideoPin->GetFilterGraph()->Disconnect(m_pVideoPin->GetConnected());
		  m_pVideoPin->GetFilterGraph()->Disconnect(m_pVideoPin);
	  }
	  stream--;
	  stream[0] = 0;
	  stream++;
  }

  IFilterGraph* pGraph = GetFilterGraph();
  IMediaSeeking* pMS = NULL;
  {
	pGraph->QueryInterface(IID_IMediaSeeking, (void **)&pMS);
  }

  char trackID[255];
  unsigned track = 1;
  if (m_pVideoPin->IsConnected())
  {

	  // Create a Streaming Session Stream object, setup 'groupsocks' for RTP and RTCP:
	  videoStream = new CStreamingSessionStream(m_env, m_destinationAddress, m_vidPortBase ? m_vidPortBase : getFreeVideoPort(), 2 /*ttl*/);
	  
	  DSPushRTPSink* videoSink = NULL;
	  /*if (m_pVideoPin->m_mt.subtype == MEDIASUBTYPE_MPEG1Payload)
	  {
		  // Create a framer for this Source Stream:
		  videoSource
			= MPEGVideoStreamFramer::createNew(*env, videoStream->dsource, False);

		  // Create a 'MPEG Video RTP' fRTPSink from the RTP 'groupsock':
		  videoSink 
			  = MPEGVideoRTPSink::createNew(*env, videoStream->fRTPSocket);
	  }
	  else*/
	  {
		  AM_MEDIA_TYPE mt = (AM_MEDIA_TYPE)m_pVideoPin->m_mt;

		  // Create a Video RTPSink from the RTP 'groupsock':
		  videoSink 
			  = MSVideoRTPSink::createNew(videoStream->envir(), videoStream->rtpSocket(), 
				&mt);
	  }

	  // Create (and start) a 'RTCP instance' for this RTP fRTPSink:
	  sprintf(trackID, "track%d", track++);
	  videoStream->Open(videoSink, 4500 /* in kbps; for RTCP b/w share */,
						trackID);

	  // Get duration
	  m_pVideoPin->m_tSegStart = 0;
	  m_pVideoPin->m_tSegStop = 0;
	  if (pMS)
		pMS->GetDuration(&m_pVideoPin->m_tSegStop);

	  // Add to the subsession list
	  serverMediaSession->addSubsession(
		  videoStream->rtpSink(), videoStream->rtcpInstance(),
		  (float)m_pVideoPin->m_tSegStart / (float)UNITS,
		  (float)m_pVideoPin->m_tSegStop / (float)UNITS);
  }

  if (m_pAudioPin->IsConnected())
  {
	  // Create a Streaming Session Stream object, setup 'groupsocks' for RTP and RTCP:
	  audioStream = new CStreamingSessionStream(m_env, m_destinationAddress, m_audPortBase ? m_audPortBase : getFreeAudioPort(), 2 /*ttl*/);

	  DSPushRTPSink* audioSink = NULL;
	  // MEDIASUBTYPE_MPEG3AudioPayload
	  /*if (m_pAudioPin->m_mt.subtype == MEDIASUBTYPE_MPEG1AudioPayload)
	  {
		  // Create a framer for this Source Stream:
		  audioSource
			= MPEGAudioStreamFramer::createNew(*env, audioStream->dsource);

		  // Create a 'MPEG Audio RTP' fRTPSink from the RTP 'groupsock':
		  audioSink 
			  = MPEGAudioRTPSink::createNew(*env, audioStream->fRTPSocket);
	  }
	  else*/
	  {
		  AM_MEDIA_TYPE mt = (AM_MEDIA_TYPE)m_pAudioPin->m_mt;

		  // Create an Audio RTPSink from the RTP 'groupsock':
		  audioSink 
			  = MSVideoRTPSink::createNew(audioStream->envir(), audioStream->rtpSocket(),
				&mt);
	  }

	  // Create (and start) a 'RTCP instance' for this RTP fRTPSink:
	  sprintf(trackID, "track%d", track++);
	  audioStream->Open(audioSink, 160 /* in kbps; for RTCP b/w share */,
						trackID);

	  // Get duration
	  m_pAudioPin->m_tSegStart = 0;
	  m_pAudioPin->m_tSegStop = 0;
	  if (pMS)
		pMS->GetDuration(&m_pAudioPin->m_tSegStop);

	  // Add to the subsession list
	  //serverMediaSession->addSubsession(*audioSink);
	  serverMediaSession->addSubsession(
		  audioStream->rtpSink(), audioStream->rtcpInstance(),
		  (float)m_pAudioPin->m_tSegStart / (float)UNITS,
		  (float)m_pAudioPin->m_tSegStop / (float)UNITS);
	  
  }

  if (pMS)
	pMS->Release();

  return True;
}

#define DO_STREAM(cmd) \
  if (audioStream && strcmp(audioStream->trackID(), trackID) == 0) \
	  audioStream->cmd(); \
  else if (videoStream && strcmp(videoStream->trackID(), trackID) == 0) \
	  videoStream->cmd(); \
  else { \
	  if (audioStream) audioStream->cmd(); \
	  if (videoStream) videoStream->cmd(); } \
  return S_OK;

void CStreamingSessionStream::Setup() {

  /*if (fRTCPInstance)
	  fRTCPInstance->startNetworkReading();*/
}

STDMETHODIMP CRTPDest::Setup(const char *trackID) {

  DO_STREAM(Setup)
}

void afterPlaying(void* clientData) {

  CStreamingSessionStream* ssess = (CStreamingSessionStream*)clientData;
  
  DbgLog((LOG_TRACE, 3, "RTSP[%.08X] : End Of Stream\n", ssess));

}

void CStreamingSessionStream::Play() {

  if (fRTCPInstance)
	  fRTCPInstance->startNetworkReading();

  if (fRTPSink)
	  fRTPSink->startPlaying(afterPlaying, this);
}

STDMETHODIMP CRTPDest::Play(const char *trackID) {

  DO_STREAM(Play)
}

STDMETHODIMP CRTPDest::Pause(const char *trackID) {

  //DO_STREAM(Pause)
  return S_OK;
}

STDMETHODIMP CRTPDest::Mute(const char *trackID) {

  //DO_STREAM(Mute)
  return S_OK;
}
STDMETHODIMP CRTPDest::Unmute(const char *trackID) {

  //DO_STREAM(Unmute)
  return S_OK;
}

void CStreamingSessionStream::Teardown() {

  if (fRTPSink)
	  fRTPSink->stopPlaying();

  if (!fRTCPInstance)
	  return;

  // Disable RTP over TCP before deleting fRTCPSocket
  fRTCPInstance->stopNetworkReading();
  //fRTCPInstance->setStreamSocket(-1, 0);
}

STDMETHODIMP CRTPDest::Teardown(const char *trackID) {

  DO_STREAM(Teardown)
}

void CStreamingSessionStream::Close() {

  SAFE_CLOSE(fRTPSink);
  SAFE_DELETE(fRTPSocket);
  SAFE_CLOSE(fRTCPInstance);
  SAFE_DELETE(fRTCPSocket);

  delete this;
}

STDMETHODIMP CRTPDest::Close() {

  if (videoStream)
	videoStream->Teardown();
  if (audioStream)
	audioStream->Teardown();

  if (videoStream)
  {
    releaseVideoPort(videoStream->portNum());
	videoStream->Close();
	videoStream = NULL;
  }
  if (audioStream)
  {
    releaseAudioPort(audioStream->portNum());
	audioStream->Close();
	audioStream = NULL;
  }

  nInstances--;
  return S_OK;
}
