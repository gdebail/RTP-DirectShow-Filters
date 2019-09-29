//------------------------------------------------------------------------------
// File: RTPDest.h
//
// Desc: DirectShow sample code - definitions for RTPDest renderer.
//
// Copyright (c) 1992 - 2000, Microsoft Corporation.  All rights reserved.
//------------------------------------------------------------------------------

#ifndef __RTPDest__
#define __RTPDest__

class CRTPDestInputPin;
class CRTPDestVideo;
class CRTPDestAudio;
class CRTPDest;
class CRTPDestFilter;

#include "iStrmSes.h"

#include "liveMedia.hh"
#include "GroupsockHelper.hh"
#include "BasicUsageEnvironment.hh"
#include <string.h>
#if defined(__WIN32__) || defined(_WIN32)
#else
#include <unistd.h>
#endif
#include <stdlib.h>

class CStreamingSessionStream {
public:
	CStreamingSessionStream(UsageEnvironment* env, struct in_addr destinationAddress, short portBase, unsigned char ttl = 7); 
	  // def ttl low, in case routers don't admin scope

	~CStreamingSessionStream() {
	}

	DSPushRTPSink* GetSink() {return fRTPSink;}

	void Open(DSPushRTPSink* snk, unsigned totalSessionBandwidth, char* trkID);
	void Setup();
	void Play();
	void Pause();
	void Mute();
	void Unmute();
	void Teardown();
	void Close();

public:
	char* trackID() {return (char*)fTrackID;}
	unsigned short portNum() {return ntohs(fRTPSocket->destPort().num());}
	Groupsock* rtpSocket() {return fRTPSocket;}
	RTPSink* rtpSink() {return fRTPSink;}
	RTCPInstance* rtcpInstance() {return fRTCPInstance;}
	UsageEnvironment& envir() {return *m_env;}

private:
	char fTrackID[255];
	Groupsock* fRTPSocket; Groupsock* fRTCPSocket; // works even for unicast
	DSPushRTPSink* fRTPSink; RTCPInstance* fRTCPInstance;
	UsageEnvironment* m_env;
};

class CStreamingSession
{
friend class CRTPDest;
public:

	CStreamingSession() :
	    serverMediaSession(NULL),
		audioStream(NULL),
		videoStream(NULL),
		m_audPortBase(0),
		m_vidPortBase(0),
		m_env(NULL)
		{}

	~CStreamingSession()	{
	}

	UsageEnvironment& envir() {return *m_env;}

	CStreamingSessionStream* audioStream;
	CStreamingSessionStream* videoStream;

protected:
	ServerMediaSession* serverMediaSession;
	struct in_addr m_destinationAddress;
	int m_audPortBase;
	int m_vidPortBase;
	UsageEnvironment* m_env;
};

#define BYTES_PER_LINE 20
#define FIRST_HALF_LINE "   %2x %2x %2x %2x %2x %2x %2x %2x %2x %2x"
#define SECOND_HALF_LINE " %2x %2x %2x %2x %2x %2x %2x %2x %2x %2x"

extern DWORD WINAPI RTPsndThread(LPVOID pv);

// Main filter object

class CRTPDestFilter :	public CBaseFilter
{
    CRTPDest * const m_pRTPDest;

public:

    // Constructor
    CRTPDestFilter(CRTPDest *pRTPDest,
                LPUNKNOWN pUnk,
                CCritSec *pLock,
                HRESULT *phr);

    // Pin enumeration
    CBasePin * GetPin(int n);
    int GetPinCount();

    // Open and close the file as necessary
    STDMETHODIMP Run(REFERENCE_TIME tStart);
    STDMETHODIMP Pause();
    STDMETHODIMP Stop();
};

class CRTPSample : public CMediaSample
{
public:
	CRTPSample(CMediaSample *pSample, AM_MEDIA_TYPE* pmt, HRESULT *phr);
	STDMETHODIMP_(ULONG) Release();
	AM_MEDIA_TYPE* m_pmt;
};

class CRTPSampleElem
{
public:
	CRTPSampleElem(CRTPSample *pSample) :
		m_pSample(pSample),
		m_pNext(NULL) {}

	~CRTPSampleElem() {}

	CRTPSample *m_pSample;
	CRTPSampleElem *m_pNext;
};

typedef CGenericList<CRTPSample> CRTPSampleList;

//  Pin object
class CRTPDestInputPin : public CRenderedInputPin
{

friend class CRTPDestVideo;
friend class CRTPDestAudio;
friend class CRTPDest;

    CRTPDest    * const m_pRTPDest;           // Main renderer object
	CCritSec * const m_pReceiveLock;		  // Receive Sample Lock
	CCritSec * const m_pLock;
    REFERENCE_TIME m_tLast;					  // Last sync sample receive time
    REFERENCE_TIME m_tStart;				  // First sync sample receive time
    REFERENCE_TIME m_tSegStart;				  // Segment Start time
    REFERENCE_TIME m_tSegStop;				  // Segment Stop time
	double m_dSegRate;						  // Segment Rate
    CPosPassThru *m_pPosition;				  // Renderer position controls

    // Overriden to say what interfaces we support where
    STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void ** ppv);

public:

    CRTPDestInputPin(TCHAR *pObjectName,
			      LPCWSTR pName,
				  CRTPDest *pRTPDest,
                  LPUNKNOWN pUnk,
                  CBaseFilter *pFilter,
                  CCritSec *pLock,
                  CCritSec *pReceiveLock,
                  HRESULT *phr);

    virtual ~CRTPDestInputPin();

    // Do something with this media sample
    STDMETHODIMP Receive(IMediaSample *pSample);
    STDMETHODIMP EndOfStream(void);
    STDMETHODIMP ReceiveCanBlock();

#ifdef DEBUG
    HRESULT WriteStringInfo(IMediaSample *pSample);
#endif

    // Check if the pin can support this specific proposed type and format
    HRESULT CheckMediaType(const CMediaType *);

    // Break connection
    HRESULT BreakConnect();

    // Track NewSegment
    STDMETHODIMP NewSegment(REFERENCE_TIME tStart,
                            REFERENCE_TIME tStop,
                            double dRate);
	
	virtual BOOL IsAudio() {return FALSE;}
	virtual BOOL IsVideo() {return FALSE;}

	virtual DSPushRTPSink* GetSink() = 0;

	BOOLEAN IsEndOfStream() 
	{
		if (m_bAtEndOfStream || m_bCompleteNotified || m_pFilter->IsStopped())
			return TRUE;
		else
			return FALSE;
	}

    // return the filter graph we belong to
    IFilterGraph *GetFilterGraph() {
        return m_pFilter->GetFilterGraph();
    }

	// Insist on a special allocator
	STDMETHODIMP GetAllocatorRequirements(ALLOCATOR_PROPERTIES*pProps)
	{
		return E_NOTIMPL;

		//pProps->cbAlign = 0;
		//pProps->cbBuffer = 0;
		//pProps->cBuffers = 0;
		pProps->cbPrefix = 1024; //12 + GetSink()->specialHeaderSize();
		return S_OK;
	}

    // return the allocator interface that this input pin
    // would like the output pin to use
	STDMETHODIMP GetAllocator(
		IMemAllocator **ppAllocator)
	{
		CheckPointer(ppAllocator,E_POINTER);
		ValidateReadWritePtr(ppAllocator,sizeof(IMemAllocator *));
		CAutoLock cObjectLock(m_pLock);

		if (m_pAllocator == NULL) {
			HRESULT hr = CreateMemoryAllocator(&m_pAllocator);
			if (FAILED(hr)) {
				return hr;
			}
		}
		ASSERT(m_pAllocator != NULL);
		*ppAllocator = m_pAllocator;
		m_pAllocator->AddRef();
		return NOERROR;
	}

    // tell the input pin which allocator the output pin is actually
    // going to use.
    STDMETHODIMP NotifyAllocator(
                    IMemAllocator * pAllocator,
                    BOOL bReadOnly)
	{
		CheckPointer(pAllocator,E_POINTER);
		ValidateReadPtr(pAllocator,sizeof(IMemAllocator));
		CAutoLock cObjectLock(m_pLock);

		IMemAllocator *pOldAllocator = m_pAllocator;
		pAllocator->AddRef();
		m_pAllocator = pAllocator;

		if (pOldAllocator != NULL) {
			pOldAllocator->Release();
		}

		ALLOCATOR_PROPERTIES Props;
		m_pAllocator->GetProperties(&Props);

		// the readonly flag indicates whether samples from this allocator should
		// be regarded as readonly - if true, then inplace transforms will not be
		// allowed.
		m_bReadOnly = (BYTE)bReadOnly;
		return NOERROR;
	}

	// List of received samples
	CRTPSampleList *m_SampleList;
	LONGLONG m_Buffered;
	DWORD m_FirstTime;
	DWORD m_CurTime;
};

class CRTPDestVideo : public CRTPDestInputPin
{

public:

    CRTPDestVideo(CRTPDest *pRTPDest,
                  LPUNKNOWN pUnk,
                  CBaseFilter *pFilter,
                  CCritSec *pLock,
                  CCritSec *pReceiveLock,
                  HRESULT *phr) :
		CRTPDestInputPin(NAME("CRTPDestVideo"),
				      L"Video Input",
				      pRTPDest,
					  pUnk,
					  pFilter,
					  pLock,
					  pReceiveLock,
					  phr) {};

    // Do something with this media sample
    STDMETHODIMP Receive(IMediaSample *pSample);

    STDMETHODIMP EndOfStream(void);

	virtual BOOL IsAudio() {return FALSE;}
	virtual BOOL IsVideo() {return TRUE;}
	virtual DSPushRTPSink* GetSink();
};

class CRTPDestAudio : public CRTPDestInputPin
{

public:

    CRTPDestAudio(CRTPDest *pRTPDest,
                  LPUNKNOWN pUnk,
                  CBaseFilter *pFilter,
                  CCritSec *pLock,
                  CCritSec *pReceiveLock,
                  HRESULT *phr) :
		CRTPDestInputPin(NAME("CRTPDestAudio"),
				      L"Audio Input",
					  pRTPDest,
					  pUnk,
					  pFilter,
					  pLock,
					  pReceiveLock,
					  phr) {};

    // Do something with this media sample
    STDMETHODIMP Receive(IMediaSample *pSample);

    STDMETHODIMP EndOfStream(void);

	virtual BOOL IsAudio() {return TRUE;}
	virtual BOOL IsVideo() {return FALSE;}
	virtual DSPushRTPSink* GetSink();
};

class CRTPDest : public CUnknown, 
				 public CStreamingSession,
				 public IStreamingSession,
				 public ISpecifyPropertyPages
{
	friend class CRTPDestVideo;
	friend class CRTPDestAudio;
    friend class CRTPDestFilter;
    friend class CRTPDestInputPin;

    CRTPDestFilter *m_pFilter;      // Methods for filter interfaces
    CRTPDestVideo *m_pVideoPin;     // A simple rendered video input pin
    CRTPDestAudio *m_pAudioPin;     // A simple rendered video input pin
    CCritSec m_Lock;                // Main renderer critical section
    CCritSec m_ReceiveLock;         // Sublock for received samples
    CPosPassThru *m_pPosition;		// Renderer position controls

public:

    DECLARE_IUNKNOWN

    CRTPDest(LPUNKNOWN pUnk, HRESULT *phr);
    ~CRTPDest();

    static CUnknown * WINAPI CreateInstance(LPUNKNOWN punk, HRESULT *phr);
    
	HRESULT SetTimescaleFramePeriod(REFERENCE_TIME AvgTimePerFrame);

	// IStreamingSession (COM interface)
	STDMETHODIMP CreateMediaSession(void *clientData, const char *destinationAddressStr, int audPortBase = 0, int vidPortBase = 0);
	// env type = (UsageEnvironment *)
	// audPortBase = 0, vidPortBase = 0 => automatically choosen by RTP dest filter
	STDMETHODIMP Open(const char *urlSuffix);
	STDMETHODIMP Setup(const char *trackID);
	STDMETHODIMP Play(const char *trackID);
	STDMETHODIMP Pause(const char *trackID);
	STDMETHODIMP Mute(const char *trackID);
	STDMETHODIMP Unmute(const char *trackID);
	STDMETHODIMP Teardown(const char *trackID);
	STDMETHODIMP Close();

    // ISpecifyPropertyPages interface
    STDMETHODIMP GetPages(CAUUID *pPages);

    IFilterGraph *GetFilterGraph() {
        return m_pFilter->GetFilterGraph();
    }

private:

    // Overriden to say what interfaces we support where
    STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void ** ppv);

public:

	double m_frame_rate;
	UINT32 m_timescale, m_frame_period;
	unsigned int m_frame_count;
	double m_processing_time;
	unsigned int m_rcvd_block_count;
	unsigned int m_rcvd_block_size;
	unsigned int m_block_count;
	unsigned int m_block_size;
	unsigned int m_frame_size;

	unsigned int m_vid_total_size;
	unsigned int m_aud_total_size;
};

#endif