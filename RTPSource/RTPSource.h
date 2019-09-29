//------------------------------------------------------------------------------
// File: FRTPSource.h
//
// Desc: DirectShow sample code - main header file for the Filter RTPSource
//       source filter.  For more information refer to RTPSource.cpp
//
// Copyright (c) 1999-2001 Microsoft Corporation.  All rights reserved.
//------------------------------------------------------------------------------

#include <mmreg.h>
#include <qnetwork.h>
#include <mpegdef.h>
#include <mpeg2typ.h>
#include <mpgutil.h>
#include "alloc.h"

#include "liveMedia.hh"
#include "GroupsockHelper.hh"
#include "BasicUsageEnvironment.hh"

#include <string.h>
#if defined(__WIN32__) || defined(_WIN32)
#else
#include <unistd.h>
#endif
#include <sys/stat.h>

#ifndef _MEDIA_SINK_HH
#include "MediaSink.hh"
#endif

//------------------------------------------------------------------------------
// Define GUIDS used in this sample
//------------------------------------------------------------------------------
// {E5B059AC-65A6-400a-A113-06F46EB488DD}
DEFINE_GUID(CLSID_FilterRTPSource, 
0xe5b059ac, 0x65a6, 0x400a, 0xa1, 0x13, 0x6, 0xf4, 0x6e, 0xb4, 0x88, 0xdd);

#define FOURCC( ch0, ch1, ch2, ch3 )				\
		( (DWORD)(BYTE)(ch0) | ( (DWORD)(BYTE)(ch1) << 8 ) |	\
		( (DWORD)(BYTE)(ch2) << 16 ) | ( (DWORD)(BYTE)(ch3) << 24 ) )

inline bool IsMemoryBadFood( DWORD d )
{
    return (    ( d == 0xDDDDDDDD )     // crt: dead land (deleted objects)
             || ( d == 0xCDCDCDCD )     // crt: clean land (new, uninit’d objects)
             || ( d == 0xFDFDFDFD )     // crt: no man's land (off the end)
             || ( d == 0xCCCCCCCC )     // vc++: stack objects init’d with this
             || ( d == 0xFEEEFEEE )     // ? nt internal ?
             || ( d == 0xBAADF00D ) );  // winnt: nt internal "not yours" filler
}

// 00000055-0000-0010-8000-00AA00389B71         MEDIASUBTYPE_MPEG3AudioPayload
DEFINE_GUID(MEDIASUBTYPE_MPEG3AudioPayload,
WAVE_FORMAT_MPEGLAYER3, 0x0000, 0x0010, 0x80, 0x00, 0x00, 0xAA, 0x00, 0x38, 0x9B, 0x71);


//------------------------------------------------------------------------------
// Forward Declarations
//------------------------------------------------------------------------------
// The class managing output pin(s)
class CRTPSourceStream;

// The class managing the filter
class CFilterRTPSource;
class CRTPSource;

//------------------------------------------------------------------------------
// Class CFilterRTPSource
//
// This is the main class for the Filter RTPSource filter. It inherits from
// CSource, the DirectShow base class for source filters.
//------------------------------------------------------------------------------
class CFilterRTPSource : public CSourceSeeking,
						 public CSource,
						 public CSourcePosition,
						 public IFileSourceFilter,
						 public IAMOpenProgress,
						 public IAMNetworkStatus,
						 public IAMNetShowExProps,
						 public IAMStreamSelect
{
#ifdef DEBUG
	FILE *m_stdin;
	FILE *m_stdout;
	FILE *m_stderr;
#endif
public:
    // It is only allowed to to create these objects with CreateInstance
    CFilterRTPSource(LPUNKNOWN lpunk, HRESULT *phr);

	~CFilterRTPSource();

    // The only allowed way to create Filter RTPSources!
    static CUnknown * WINAPI CreateInstance(LPUNKNOWN lpunk, HRESULT *phr);

	// we replace the DECLARE_IUNKNOWN macro so we can resolve
	// ambiguous references due to multiple CUnknowns
	STDMETHODIMP QueryInterface(REFIID riid, void **ppv) {
        return CBaseFilter::GetOwner()->QueryInterface(riid,ppv);      
    };                                                   
    STDMETHODIMP_(ULONG) AddRef() {                      
        return CBaseFilter::GetOwner()->AddRef();                     
    };                                                   
    STDMETHODIMP_(ULONG) Release() {                     
        return CBaseFilter::GetOwner()->Release();                    
    };

    // IDispatch methods inherited from CSourcePosition::CMediaPosition
    STDMETHODIMP GetTypeInfoCount(UINT * pctinfo) {
        return CMediaPosition::GetTypeInfoCount(pctinfo);
	}

    STDMETHODIMP GetTypeInfo(
      UINT itinfo,
      LCID lcid,
      ITypeInfo ** pptinfo) {
        return CMediaPosition::GetTypeInfo(itinfo, lcid, pptinfo);}

    STDMETHODIMP GetIDsOfNames(
      REFIID riid,
      OLECHAR  ** rgszNames,
      UINT cNames,
      LCID lcid,
      DISPID * rgdispid) {
        return CMediaPosition::GetIDsOfNames(riid, rgszNames, cNames, lcid, rgdispid);}
	  
    STDMETHODIMP Invoke(
      DISPID dispidMember,
      REFIID riid,
      LCID lcid,
      WORD wFlags,
      DISPPARAMS * pdispparams,
      VARIANT * pvarResult,
      EXCEPINFO * pexcepinfo,
      UINT * puArgErr) {
        return CMediaPosition::Invoke(dispidMember,
									  riid,
									  lcid,
									  wFlags,
									  pdispparams,
									  pvarResult,
									  pexcepinfo,
									  puArgErr);}


    STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void **ppv)
    {
        if (riid == IID_IFileSourceFilter) {
            return GetInterface((IFileSourceFilter *)this, ppv);
        } else if (riid == IID_IAMOpenProgress) {
            return GetInterface((IAMOpenProgress *)this, ppv);
        } else if (riid == IID_IAMNetworkStatus) {
            return GetInterface((IAMNetworkStatus *)this, ppv);
        } else if (riid == IID_IAMNetShowExProps) {

			// To avoid WMP7/8/9 to fail trying to handle the streaming
			// Unfortunatly WMP6 query IAMNetShowExProps only once :-(
			BOOL bIAMNetShowExProps = (BOOL)GetPrivateProfileInt("WMP", "statistics", 0, "MMRTPSrc.ini");
			WritePrivateProfileString("WMP", "statistics", bIAMNetShowExProps ? "1" : "0", "MMRTPSrc.ini");
			if (bIAMNetShowExProps)
				return GetInterface((IAMNetShowExProps *)this, ppv);
			else
				return CSource::NonDelegatingQueryInterface(riid, ppv);

		} else if(riid == IID_IAMStreamSelect) {
			return GetInterface((IAMStreamSelect *)this, ppv);
		} else if(riid == IID_IMediaSeeking) {
			return GetInterface((IMediaSeeking *)this, ppv);
		} else if(riid == IID_IMediaPosition) {
			return GetInterface((IMediaPosition *)this, ppv);
        }

        return CSource::NonDelegatingQueryInterface(riid, ppv);
    }

	HRESULT StartRTPrcvThread();
	HRESULT StopRTPrcvThread();
	HANDLE m_evtStopRTP;
#define STOP_RTP_TIMEOUT 5000

    //  IMediaFilter methods - override these for RTP thread start/stop
    STDMETHODIMP Stop() 
	{
		Disconnect();

		m_llTotal = 0;
		m_llCurrent = 0;

		return CSource::Stop();
	}

    STDMETHODIMP Pause() 
	{
		if (m_hRTPrcvThread == NULL && m_StopRTPThread == 1)
		{
			Connect(TRUE);
		}
		return CSource::Pause();
	}

    STDMETHODIMP Run(REFERENCE_TIME tStart)
	{
		return CSource::Run(tStart);
	}

    /*  IFileSourceFilter methods */
    //  Load a (new) file

    STDMETHODIMP Load(LPCOLESTR lpwszFileName, const AM_MEDIA_TYPE *pmt);

    // Modelled on IPersistFile::Load
    // Caller needs to CoTaskMemFree or equivalent.

    STDMETHODIMP GetCurFile(LPOLESTR * ppszFileName, AM_MEDIA_TYPE *pmt)
    {
        CheckPointer(ppszFileName, E_POINTER);
        *ppszFileName = NULL;
        if (m_pURL!=NULL) {
    	DWORD n = sizeof(WCHAR)*(1+lstrlenW(m_pURL));

            *ppszFileName = (LPOLESTR) CoTaskMemAlloc( n );
            if (*ppszFileName!=NULL) {
                  CopyMemory(*ppszFileName, m_pURL, n);
            }
        }

        /*if (pmt!=NULL) {
            CopyMediaType(pmt, &m_mt);
        }*/

        return NOERROR;
    }

	// IAMOpenProgress implementation
	LONGLONG m_llTotal;
	LONGLONG m_llCurrent;
    HANDLE m_hRTPrcvThread;
	char m_StopRTPThread;

	STDMETHODIMP QueryProgress(
	  LONGLONG *pllTotal,
	  LONGLONG *pllCurrent
	)
	{
		*pllTotal = m_llTotal;
		*pllCurrent = m_llCurrent;

		return S_OK; //VFW_S_ESTIMATED;
	}

	STDMETHODIMP AbortOperation(void)
	{
		StopRTPrcvThread();
		return S_OK;
	}

	// CSourceSeeking
	STDMETHODIMP GetCurrentPosition(LONGLONG *pCurrent);
	HRESULT ChangeStart();
	HRESULT ChangeStop();
	HRESULT ChangeRate();

    // IAMNetworkStatus implementation
    STDMETHODIMP get_ReceivedPackets (long* pReceivedPackets);

    STDMETHODIMP get_RecoveredPackets (long* pRecoveredPackets)
		{*pRecoveredPackets = 0; return S_OK;}

    STDMETHODIMP get_LostPackets (long* pLostPackets);

    STDMETHODIMP get_ReceptionQuality (long* pReceptionQuality);

    STDMETHODIMP get_BufferingCount (long* pBufferingCount)
		{*pBufferingCount = 1; return S_OK;}

    STDMETHODIMP get_IsBroadcast (VARIANT_BOOL* pIsBroadcast)
		{*pIsBroadcast = TRUE; return S_OK;}

    STDMETHODIMP get_BufferingProgress (long* pBufferingProgress)
		{*pBufferingProgress = (long)(m_llCurrent * 100L / m_llTotal); return S_OK;}

	// IAMNetShowExProps
    STDMETHODIMP get_SourceProtocol (long* pSourceProtocol);

    STDMETHODIMP get_Bandwidth (long* pBandwidth);

    STDMETHODIMP get_ErrorCorrection (BSTR* pbstrErrorCorrection) 
	{
		TCHAR String[] = "RTCP";
		WCHAR WideString[1024];
	    MultiByteToWideChar(CP_ACP,0,String,-1,WideString,CAPTION);
		WriteBSTR(pbstrErrorCorrection, WideString);

		return S_OK;
	}

    STDMETHODIMP get_CodecCount (long* pCodecCount)
		{*pCodecCount = 1; return S_OK;}

    STDMETHODIMP GetCodecInstalled (long CodecNum, VARIANT_BOOL* pCodecInstalled)
		{*pCodecInstalled = TRUE; return S_OK;}

    STDMETHODIMP GetCodecDescription (long CodecNum, BSTR* pbstrCodecDescription)
	{
		TCHAR String[] = "Morgan RTP Source";
		WCHAR WideString[1024];
	    MultiByteToWideChar(CP_ACP,0,String,-1,WideString,CAPTION);
		WriteBSTR(pbstrCodecDescription, WideString);

		return S_OK;
	}

    STDMETHODIMP GetCodecURL (long CodecNum, BSTR* pbstrCodecURL)
	{
		TCHAR String[] = "http://www.morgan-multimedia.com/RTP";
		WCHAR WideString[1024];
	    MultiByteToWideChar(CP_ACP,0,String,-1,WideString,CAPTION);
		WriteBSTR(pbstrCodecURL, WideString);

		return S_OK;
	}

	STDMETHODIMP get_CreationDate (DATE* pCreationDate)
		{*pCreationDate = 0; return S_OK;}

    STDMETHODIMP get_SourceLink (BSTR* pbstrSourceLink) 
	{
		/*TCHAR String[] = "c:/DivX/Final_fantasy.avi";
		WCHAR WideString[1024];
	    MultiByteToWideChar(CP_ACP,0,String,-1,WideString,CAPTION);
		WriteBSTR(pbstrSourceLink, WideString);*/

		WriteBSTR(pbstrSourceLink, m_pURL);

		return S_OK;
	}

    //
	// --- IAMStreamSelect ---
    //

	STDMETHODIMP Count(DWORD* pcStreams); 
	STDMETHODIMP Enable(long lIndex, DWORD dwFlags); 
	STDMETHODIMP Info(long lIndex, AM_MEDIA_TYPE** ppmt, DWORD* pdwFlags, LCID* plcid, DWORD* pdwGroup, WCHAR** ppszName, IUnknown** ppObject, IUnknown** ppUnk);  

private:
    LPWSTR m_pURL;

public:
    TCHAR* m_url;

	CRTPSourceStream *GetStream(int i);
    unsigned GetStreamCount() {return m_iPins;}

	int m_verbosityLevel;
	int m_Idunno;
	struct timeval m_startTime;

	TaskScheduler* m_scheduler;
	BasicUsageEnvironment* m_env;
	RTSPClient* m_rtspClient;
	TCHAR* m_username;
	TCHAR* m_password;
	TCHAR* m_sdpDescription;
	BOOL m_bRTSP;
	MediaSession* m_session;
	TCHAR const* m_singleMedium;
	unsigned short m_desiredPortNum;

	HRESULT Connect(BOOL bStartRTPrcvThread);
	HRESULT Disconnect();
	HRESULT setupRTSPStreams();
	HRESULT startPlayingRTSPStreams();
	HRESULT tearDownRTSPStreams();
	static void subsessionByeHandler(void* clientData);
	static void subsessionAfterPlaying(void* clientData);
	static void sessionAfterPlaying(void* clientData);
	static DWORD WINAPI RTPrcvThread(LPVOID pv);


}; // CFilterRTPSource

//------------------------------------------------------------------------------
// Class DShowSink
//
// This class implements sink from liveMedia lib to DirectShow.
//------------------------------------------------------------------------------
class DShowSink: public MediaSink {
public:
  static DShowSink* createNew(UsageEnvironment& env, MediaSubsession* subsession, CRTPSourceStream* strm, char *pName);

protected:
  DShowSink(UsageEnvironment& env, MediaSubsession* subsession, CRTPSourceStream* pin, char *pName); // called only by createNew()
  virtual ~DShowSink();

protected:
  static void afterGettingFrame(void* clientData, unsigned frameSize,
				struct timeval presentationTime);
  friend void afterGettingFrame(void*, unsigned, struct timeval);

  unsigned received;

//private: // redefined virtual functions:
public:
  MediaSubsession* m_subsession;
  CRTPSourceStream* pin;
  virtual Boolean continuePlaying();

  char m_pName[1024];

  Boolean isPlaying() {return fSource != NULL;}

  unsigned sampleOffset;

  CMainAllocator *m_pAllocator;
  CSubAllocator *m_pSubAllocator;

  unsigned char *m_pBuffer;
  LONG m_BufferSize;

  Boolean m_detected;
  HRESULT m_Discontinuity;
  LONGLONG m_DiscontinuityTime;
  LONGLONG m_CurrentTime;
};

typedef CGenericList<IMediaSample> CSampleList;

//------------------------------------------------------------------------------
// Class CRTPSourceStream
//
// This class implements the stream which is used to output the Filter RTPSource
// data from the source filter. It inherits from DirectShows's base
// CSourceStream class.
//------------------------------------------------------------------------------
class CRTPSourceStream : public CSourceStream
{

public:

    CRTPSourceStream(HRESULT *phr, CFilterRTPSource *pParent, LPCWSTR pPinName);
    ~CRTPSourceStream();

	// The loop executed whilst running
    HRESULT DoBufferProcessingLoop(void);

	// To make the parent class happy (FillBuffer is pure virtual), 
	// we do nothing here, all the job is done in DoBufferProcessingLoop
    HRESULT FillBuffer(IMediaSample *pSamp) {return S_OK;}

    // Override these to insist on our own allocator
    HRESULT DecideBufferSize(IMemAllocator *pIMemAlloc,
                             ALLOCATOR_PROPERTIES *pProperties);
	HRESULT DecideAllocator(IMemInputPin *pPin,
							IMemAllocator **ppAlloc);
	HRESULT InitAllocator(IMemAllocator **ppAlloc);

    // Set the agreed media type
    HRESULT SetMediaType(const CMediaType *pMediaType);

    // Only one MediaType
    HRESULT GetMediaType(CMediaType *pmt);

    // Resets the stream time to zero
    HRESULT OnThreadCreate(void);

	BOOL IsMediaTypeFound() {return m_mt.IsValid();}
	CMediaType * CurrentMediaType() {return &m_mt;}

public:
	CCritSec m_cSharedState;	        // Lock on m_rtSampleTime

	// The parent of this stream
    CFilterRTPSource *GetFilter() {return (CFilterRTPSource *)m_pFilter;}
	
	// The main allocator of this stream
	CMainAllocator *MainAllocator() {return ((CSubAllocator *)m_pAllocator)->Allocator();}

	// The sub allocator of this stream
	CSubAllocator *Allocator() {return ((CSubAllocator *)m_pAllocator);}

	// List of pending samples
	CSampleList *m_SampleList;

	// The MediaSink object that receives RTP data, create samples and 
	// add them to m_SampleList
	DShowSink *m_sink;

	// Media type of this stream (set by DShowSink when detected)
	CMediaType m_sinkmt;

	// End time of this stream
	REFERENCE_TIME m_rtEnd;

	// Start/End time of current sample of this stream
	REFERENCE_TIME m_rtCurStart;
	REFERENCE_TIME m_rtCurEnd;

	// Some events to synchronize on ...
	HANDLE m_evtDataAvailable;
	int DATA_AVAILABLE_TIMEOUT; // 2000
	HANDLE m_evtsProcessLoop[2];

	HANDLE m_evtMediaTypeFound;
	int MEDIA_TYPE_FOUND_TIMEOUT; // 15000
	HANDLE m_evtsGetMediaType[2];

}; // CRTPSourceStream

