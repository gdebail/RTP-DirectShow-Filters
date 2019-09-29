//------------------------------------------------------------------------------
// File: RTPFilter.cpp
//
// Desc: RTP/RTCP with RTSP client Source Filter
//
// Copyright (c) 1990-2002 Morgan Multimedia.  All rights reserved.
//------------------------------------------------------------------------------
#include <streams.h>
#include <olectl.h>
#include <initguid.h>
#include <winbase.h>
#include <stdio.h>
#include <stdiostr.h>
#include "MSVideoRTPSource.hh"
#include "RTPSource.h"

// To use "rtsp://..." or "*.sdp" with DirectShow (WMP 6, 7 & 9)
/*

Windows Registry Editor Version 5.00

[HKEY_CLASSES_ROOT\rtsp]
@="Real-Time Streaming Protocol"
"EditFlags"=hex:02,00,00,00
"URL Protocol"=""
"Source Filter"="{E5B059AC-65A6-400a-A113-06F46EB488DD}"

[HKEY_CLASSES_ROOT\rtsp\shell]

[HKEY_CLASSES_ROOT\rtsp\shell\open]

[HKEY_CLASSES_ROOT\rtsp\shell\open\command]
@="\"C:\\Program Files\\Windows Media Player\\mplayer2.exe\" \"%L\""

[HKEY_CLASSES_ROOT\Media Type\Extensions\.sdp]
"Source Filter"="{E5B059AC-65A6-400a-A113-06F46EB488DD}"

[HKEY_CLASSES_ROOT\.sdp]
@="rtsp"
"Content Type"="application/sdp"

*/

// TODO : IAMPushSource, etc ...

// #pragma warning(disable:4710)  // 'function': function not inlined (optimzation)

// Setup data

const AMOVIESETUP_MEDIATYPE sudOpPinTypes =
{
    &MEDIATYPE_Video,       // Major type
    &MEDIASUBTYPE_NULL      // Minor type
};

const AMOVIESETUP_PIN sudOpPin =
{
    L"Output",              // Pin string name
    FALSE,                  // Is it rendered
    TRUE,                   // Is it an output
    FALSE,                  // Can we have none
    FALSE,                  // Can we have many
    &CLSID_NULL,            // Connects to filter
    NULL,                   // Connects to pin
    1,                      // Number of types
    &sudOpPinTypes };       // Pin details

const AMOVIESETUP_FILTER sudRTPSourceax =
{
    &CLSID_FilterRTPSource,    // Filter CLSID
    L"Morgan RTP Source Filter",  // String name
    MERIT_NORMAL,       // Filter merit
    1,                      // Number pins
    &sudOpPin               // Pin details
};


// COM global table of objects in this dll

CFactoryTemplate g_Templates[] = {
  { L"Morgan RTP Source"
  , &CLSID_FilterRTPSource
  , CFilterRTPSource::CreateInstance
  , NULL
  , &sudRTPSourceax }
};
int g_cTemplates = sizeof(g_Templates) / sizeof(g_Templates[0]);


//
// DllRegisterServer
//
// Exported entry points for registration and unregistration
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


//
// CreateInstance
//
// The only allowed way to create Filter RTPSources!
//
CUnknown * WINAPI CFilterRTPSource::CreateInstance(LPUNKNOWN lpunk, HRESULT *phr)
{
	/*LPTSTR pCmdLine = GetCommandLine();
	MessageBox(NULL, pCmdLine, "", NULL);
	STARTUPINFO StartupInfo;
	GetStartupInfo(&StartupInfo);
	MessageBox(NULL, StartupInfo.lpTitle, StartupInfo.lpDesktop, NULL);*/

    CFilterRTPSource *punk = new CFilterRTPSource(lpunk, phr);
    if (punk == NULL) {
        *phr = E_OUTOFMEMORY;
    }

    return (CSource *)punk;
    
	/*CUnknown *punk = new CFilterRTPSource(lpunk, phr);

	if (*phr != S_OK)
	{
		delete punk;
		punk = NULL;
	}

    if (punk == NULL) 
	{
        *phr = E_OUTOFMEMORY;
    }
    return punk;*/

} // CreateInstance


//
// Constructor
//
// Initialise a CRTPSourceStream object
//
CFilterRTPSource::CFilterRTPSource(LPUNKNOWN lpunk, HRESULT *phr) :
	CSourceSeeking(NAME("Morgan RTP Source Seeking"), lpunk, phr, &m_cStateLock),
    CSource(NAME("Morgan RTP Source Filter"), lpunk, CLSID_FilterRTPSource),
	CSourcePosition(NAME("Morgan RTP Source Position"), lpunk, phr, &m_cStateLock),
	m_pURL(NULL),
	m_url(NULL),
	m_llTotal(0),
	m_llCurrent(0),
	m_scheduler(NULL),
	m_env(NULL),
	m_rtspClient(NULL),
	m_hRTPrcvThread(NULL),
	m_StopRTPThread(0),
	m_verbosityLevel(1),
	m_username(NULL),
	m_password(NULL),
	m_sdpDescription(NULL),
	m_bRTSP(FALSE),
	m_session(NULL),
	m_singleMedium(NULL),
	m_desiredPortNum(0)
{
    CAutoLock cAutoLock(&m_cStateLock);

	gettimeofday(&m_startTime, &m_Idunno);

#ifdef DEBUG
	AllocConsole();
	SetConsoleTitle("Morgan RTP Source : stdout/stderr");

	/*HANDLE OutH = GetStdHandle(STD_OUTPUT_HANDLE);
	COORD buffer_size;
	buffer_size.X = 80;  // 80 columns
	buffer_size.Y = 60;  // 45 rows
	SetConsoleScreenBufferSize(OutH, buffer_size);*/

	//m_stdin = freopen("conin$", "r", stdin);
	m_stdout = freopen("conout$", "w", stdout);
	m_stderr = freopen("conout$", "w", stderr);

	fprintf(stdout, "stdout: ok\n");
	fprintf(stderr, "stderr: ok\n");

	//stdiostream* myStreamIn = new stdiostream(stdin);
	stdiostream* myStreamOut = new stdiostream(stdout);
	stdiostream* myStreamErr = new stdiostream(stderr);
	//cin = *(myStreamIn);
	cout = *(myStreamOut);
	cerr = *(myStreamErr);

	cout << "cout: ok\n";
	cerr << "cerr: ok\n";

	Socket::DebugLevel = 1;

#endif

	m_evtStopRTP = CreateEvent(NULL, TRUE, FALSE, NULL);

	*phr = S_OK;

} // (Constructor)

CFilterRTPSource::~CFilterRTPSource()
{
	Disconnect();

	if (m_pURL)
		delete [] m_pURL;
	if (m_url)
		delete [] m_url;
	
	if (m_username != NULL)
		delete m_username;
	if (m_password != NULL)
		delete m_password;

#ifdef DEBUG
	//fclose(m_stdin);
	fclose(m_stdout);
	fclose(m_stderr);
	FreeConsole();
#endif
}

CRTPSourceStream *CFilterRTPSource::GetStream(int i) 
{
	return m_paStreams ? (CRTPSourceStream *)(m_paStreams[i]) : NULL;
}


DWORD WINAPI CFilterRTPSource::RTPrcvThread(LPVOID pv)
{
  DbgLog((LOG_TRACE, 1, TEXT("RTP RTCP receiving thread Started.")));

  int tp = GetPrivateProfileInt("RTP", "thread_priority", GetThreadPriority(GetCurrentThread()), "MMRTPSrc.ini");
  SetThreadPriority(GetCurrentThread(), tp);
  char sztp[256];
  sprintf(sztp, "%d", tp);
  WritePrivateProfileString("RTP", "thread_priority", sztp, "MMRTPSrc.ini");

  CFilterRTPSource *pDsFilter = (CFilterRTPSource *)pv;

  // Start each subsession sink
  MediaSubsessionIterator iter(*pDsFilter->m_session);
  MediaSubsession* subsession;
  while ((subsession = iter.next()) != NULL) 
  {
		if (subsession->sink)
		  subsession->sink->startPlaying(*(subsession->readSource()),
						 subsessionAfterPlaying,
						 pv);
  }

  // Finally, issue a RTSP "PLAY" command for each subsession,
  // to start the data flow:
  if (pDsFilter->m_bRTSP)
	pDsFilter->startPlayingRTSPStreams();

  pDsFilter->m_env->taskScheduler().blockMyself(&pDsFilter->m_StopRTPThread);

  pDsFilter->m_hRTPrcvThread = NULL;

  SetEvent(pDsFilter->m_evtStopRTP);

  return 0;
}

HRESULT CFilterRTPSource::StartRTPrcvThread()
{
	if (m_hRTPrcvThread != 0) 
		return S_OK;

    DbgLog((LOG_TRACE, 1, TEXT("Starting RTP RTCP receiving thread.")));

	m_StopRTPThread = 0;
	ResetEvent(m_evtStopRTP);

	DWORD threadid;
	// Create RTP receiving thread
	if (!m_hRTPrcvThread)
		m_hRTPrcvThread = CreateThread(NULL,
			0,
			RTPrcvThread,
			this,
			0,
			&threadid);

	return m_hRTPrcvThread ? S_OK : S_FALSE;
}

HRESULT CFilterRTPSource::StopRTPrcvThread()
{
	if (m_hRTPrcvThread == 0)
		return S_OK;

    DbgLog((LOG_TRACE, 1, TEXT("Stopping RTP RTCP receiving thread.")));

	if (m_bRTSP)
		tearDownRTSPStreams();

	ResetEvent(m_evtStopRTP);

	m_StopRTPThread = 1;

#ifdef SOFT_WAY
	if (WaitForSingleObject(m_evtStopRTP, STOP_RTP_TIMEOUT) == WAIT_TIMEOUT)
	{
	    DbgLog((LOG_ERROR, 1, TEXT("Can't stop RTP RTCP receiving thread.")));
		return S_FALSE;
	}
#else // HARD_WAY
	TerminateThread(m_hRTPrcvThread, 0);

	m_hRTPrcvThread = NULL;
	SetEvent(m_evtStopRTP);
#endif

	return S_OK;
}

STDMETHODIMP CFilterRTPSource::Load(LPCOLESTR lpwszFileName, const AM_MEDIA_TYPE *pmt)
{
    CheckPointer(lpwszFileName, E_POINTER);

	if (m_url)
		delete [] m_url;

    // lstrlenW is one of the few Unicode functions that works on win95
    int cch = lstrlenW(lpwszFileName) + 1;
#ifndef UNICODE
    m_url = new char[cch * 2];
    if (!m_url) {
      	return E_OUTOFMEMORY;
    }
    WideCharToMultiByte(GetACP(), 0, lpwszFileName, -1,
    		m_url, cch, NULL, NULL);
#else
    m_url = lpwszFileName;
#endif

	if (m_pURL)
		delete [] m_pURL;

	// keep URL in a wsz member
    m_pURL = new WCHAR[cch];
    if (m_pURL!=NULL) {
		CopyMemory(m_pURL, lpwszFileName, cch*sizeof(WCHAR));
    }

	return Connect(TRUE);
}

HRESULT CFilterRTPSource::Disconnect()
{
	if (m_env == NULL)
		return S_OK;

    DbgLog((LOG_TRACE, 1, TEXT("Disconnecting ...")));

	HRESULT hr = StopRTPrcvThread();

	if (m_sdpDescription != NULL)
		delete [] m_sdpDescription;
	m_sdpDescription = NULL;

	if (m_session != NULL)
	{
		MediaSubsessionIterator iter(*m_session);
		MediaSubsession *subsession;
		while ((subsession = iter.next()) != NULL) 
		{
			DbgLog((LOG_TRACE, 1, TEXT("Closing \"%s/%s\" subsession"),
				subsession->mediumName(), subsession->codecName()));

			Medium::close(subsession->sink);
			subsession->sink = NULL;
		}
		Medium::close(m_session);
	}
	m_session = NULL;
	DbgLog((LOG_TRACE, 1, TEXT("Media session closed.")));

	if (m_rtspClient != NULL) 
		Medium::close(m_rtspClient);
	m_rtspClient = NULL;
	DbgLog((LOG_TRACE, 1, TEXT("RTSP Client closed.")));

	if (SUCCEEDED(hr))
	{
		if (m_env != NULL)
			delete m_env;
		m_env = NULL;
		DbgLog((LOG_TRACE, 1, TEXT("Environment closed.")));

		if (m_scheduler != NULL)
			delete m_scheduler;
		m_scheduler = NULL;
		DbgLog((LOG_TRACE, 1, TEXT("Task scheduler closed.")));

		DbgLog((LOG_TRACE, 1, TEXT("RTP RTCP receiving thread Stopped.")));
	}

	return hr;
}

HRESULT CFilterRTPSource::Connect(BOOL bStartRTPrcvThread)
{
	if (m_scheduler != NULL || m_env != NULL)
	{
		DbgLog((LOG_ERROR, 1, TEXT("Connect called with existing environment or task scheduler.")));
		return S_FALSE;
	}

	// Begin by setting up our usage environment:
	m_scheduler = BasicTaskScheduler::createNew();
	if (m_scheduler == NULL)
	{
		DbgLog((LOG_ERROR, 1, TEXT("Failed to create Task Scheduler")));
		return S_FALSE;
	}
	m_env = BasicUsageEnvironment::createNew(*m_scheduler);
	if (m_env == NULL) 
	{
		DbgLog((LOG_ERROR, 1, TEXT("Failed to create Usage Environment")));
		return S_FALSE;
	}

	if ((
		(BYTE)(m_url[0]&(BYTE)0xDF) == 'R' &&
		(BYTE)(m_url[1]&(BYTE)0xDF) == 'T' &&
		(BYTE)(m_url[2]&(BYTE)0xDF) == 'S' &&
		(BYTE)(m_url[3]&(BYTE)0xDF) == 'P' &&
		m_url[4] == ':'
		) || 
		(
		(BYTE)(m_url[0]&(BYTE)0xDF) == 'M' &&
		(BYTE)(m_url[1]&(BYTE)0xDF) == 'M' &&
		(BYTE)(m_url[2]&(BYTE)0xDF) == 'S' &&
		(BYTE)(m_url[3]&(BYTE)0xDF) == 'P' &&
		m_url[4] == ':'
		))
	{
		m_bRTSP = True;

		m_url[0] = 'r';
		m_url[1] = 't';

		// Create our RTSP client object:
		m_rtspClient = RTSPClient::createNew(*m_env, m_verbosityLevel, "RTSPClient");
		if (m_rtspClient == NULL)
		{
			DbgLog((LOG_ERROR, 1, TEXT("Failed to create RTSP client: %s"), m_env->getResultMsg()));
			return S_FALSE;
		}

		// Open the URL, to get a SDP description:
		if (m_username != NULL && m_password != NULL) 
			m_sdpDescription = m_rtspClient->describeWithPassword(m_url, m_username, m_password);
		else
			m_sdpDescription = m_rtspClient->describeURL(m_url);
		if (m_sdpDescription == NULL) 
		{
			DbgLog((LOG_ERROR, 1, TEXT("Failed to get a SDP description from URL \"%s\": %s"), m_url, m_env->getResultMsg()));
		    return S_FALSE;
		}

		//MessageBox(0, m_sdpDescription, "SDP", 0);
	}
	else
	{
		m_bRTSP = FALSE;

		FILE* fid = fopen(m_url, "rb");
		if (fid == NULL) 
		{
			DbgLog((LOG_ERROR, 1, TEXT("Failed to open \"%s\""), m_url));
			return S_FALSE;
		}

		unsigned fileSize = 0;
		struct stat sb;
		if (stat(m_url, &sb) == 0) 
		{
			fileSize = sb.st_size;

			m_sdpDescription = new TCHAR[fileSize + 1];
			fileSize = fread(m_sdpDescription, 1, fileSize, fid);
			m_sdpDescription[fileSize] = NULL;
		}

		fclose(fid);
	}

	DbgLog((LOG_TRACE, 2, TEXT("Opened URL \"%s\", returning a SDP description:\"%s\""), m_url, m_sdpDescription));

	// Create a media m_session object from this SDP description:
	m_session = MediaSession::createNew(*m_env, m_sdpDescription);
	if (m_session == NULL) 
	{
		DbgLog((LOG_ERROR, 1, TEXT("Failed to create a MediaSession object from the SDP description: %s"), m_env->getResultMsg()));
		return S_FALSE;
	}
	else if (!m_session->hasSubsessions()) 
	{
		DbgLog((LOG_ERROR, 1, TEXT("This m_session has no media subsessions (i.e., \"m=\" lines)")));
		Medium::close(m_session);
		m_session = NULL;
		return S_FALSE;
	}

	// Then, setup the "RTPSource"s for the m_session:
	MediaSubsessionIterator iter(*m_session);
	MediaSubsession *subsession;
	Boolean madeProgress = False;
	char const* singleMediumToTest = m_singleMedium;
	while ((subsession = iter.next()) != NULL) 
	{
		// If we've asked to receive only a single medium, then check this now:
		if (singleMediumToTest != NULL) 
		{
		  if (strcmp(subsession->mediumName(), singleMediumToTest) != 0) 
		  {
			fprintf(stderr, "Ignoring \"%s/%s\" subsession, because we've asked to receive a single %s m_session only\n",
				subsession->mediumName(), subsession->codecName(),
				m_singleMedium);
			continue;
		  } 
		  else 
		  {
			// Receive this subsession only
			singleMediumToTest = TEXT("xxxxx");
			// this hack ensures that we get only 1 subsession of this type
		  }
		}

		if (m_desiredPortNum != 0) 
		{
		  subsession->setClientPortNum(m_desiredPortNum);
		  m_desiredPortNum += 2;
		}

		Boolean ok = subsession->initiate(-1 /*simpleRTPoffsetArg*/);

		if (!ok)
			ok = subsession->initiate(0);

		if (!ok) 
		{
			fprintf(stderr, "Unable to create receiver for \"%s/%s\" subsession: %s\n",
				subsession->mediumName(), subsession->codecName(),
				m_env->getResultMsg());
		} 
		else 
		{
			fprintf(stderr, "Created receiver for \"%s/%s\" subsession (client ports %d-%d)\n",
				subsession->mediumName(), subsession->codecName(),
				subsession->clientPortNum(), subsession->clientPortNum()+1);
			madeProgress = True;

			if (subsession->rtpSource() != NULL) 
			{
			  //unsigned const thresh = 1000000/2; // 1/2 second 
			  unsigned const thresh = 40000; // 0.04 second = 1 frame @ 25 fps
			  //unsigned const thresh = 100000 // default reordering threshold: 100 ms
			  subsession->rtpSource()->setPacketReorderingThresholdTime(thresh);
			}
		}
	}
	if (!madeProgress) 
	{
		DbgLog((LOG_ERROR, 1, TEXT("Can't setup RTP sources for current session.")));
		Medium::close(m_session);
		m_session = NULL;
		return S_FALSE;
	}

	// Create and start "DShowSink"s for each subsession:
	unsigned streamCounter = 0;
	madeProgress = False;
	iter.reset();
	while ((subsession = iter.next()) != NULL) 
	{
		if (subsession->readSource() == NULL) continue; // was not initiated

		// Create a DShow sink (output pin) for each desired stream:
		char sinkName[1000];
		// name is "<medium_name>-<codec_name>-<counter>"
		_snprintf(sinkName, sizeof sinkName, "%s-%s-%d",
		   subsession->mediumName(), subsession->codecName(),
		   ++streamCounter);

		if (GetStreamCount() < streamCounter)
		{
			HRESULT hr;

			WCHAR achPinName[MAX_PATH];

			const char *achTmp = subsession->readSource()->MIMEtype();
			//char *achTmp = sinkName;

			MultiByteToWideChar(CP_ACP
				, 0L
				, achTmp
				, lstrlenA(achTmp) + 1
				, achPinName
				, sizeof(achPinName)/sizeof(TCHAR));

			CRTPSourceStream *pStream = new CRTPSourceStream(&hr, this, achPinName);
		}

		if (GetStream(streamCounter - 1))
			subsession->sink = DShowSink::createNew(*m_env, subsession, GetStream(streamCounter - 1), sinkName);
		else
			subsession->sink = NULL;

		if (subsession->sink == NULL) 
		{
		  fprintf(stderr, "Failed to create FileSink for \"%s\": %s\n",
			  sinkName, m_env->getResultMsg());
		}
		else
		{
		  // Also set a handler to be called if a RTCP "BYE" arrives
		  // for this subsession:
		  if (subsession->rtcpInstance() != NULL) 
		  {
			subsession->rtcpInstance()->setByeHandler(
								  subsessionByeHandler,
								  this);
		  }

		  madeProgress = True;
		}
	}
	if (!madeProgress) 
	{
		DbgLog((LOG_ERROR, 1, TEXT("Can't create sinks and output pins for current session.")));
		Medium::close(m_session);
		m_session = NULL;
		return S_FALSE;
	}

	// Issue RTSP "SETUP"s on this session:
	if (m_bRTSP)
	{
		HRESULT hr = setupRTSPStreams();
		if (hr != S_OK)
		{
			DbgLog((LOG_ERROR, 1, TEXT("Can't issue RTSP SETUP on this session")));
			Medium::close(m_session);
			m_session = NULL;
			return S_FALSE;
		}
	}

	SetRate(1.0); // update m_rtDuration

	// try to start RTP/RTSP thread
	if (bStartRTPrcvThread)
		return StartRTPrcvThread();
	else
		return S_OK;

}

//
// subsession Handlers
//
void CFilterRTPSource::subsessionByeHandler(void* clientData) 
{
  subsessionAfterPlaying(clientData);
}

void CFilterRTPSource::subsessionAfterPlaying(void* clientData) 
{
  sessionAfterPlaying(clientData);
}

void CFilterRTPSource::sessionAfterPlaying(void* clientData) 
{
  CFilterRTPSource* filter = (CFilterRTPSource*)clientData;
  // fix me :
  //filter->Stop();
}

// 
// RTSP commands
//
HRESULT CFilterRTPSource::setupRTSPStreams()
{
  MediaSubsessionIterator iter(*m_session);
  MediaSubsession *subsession;
  HRESULT hr = S_FALSE;

  while ((subsession = iter.next()) != NULL) 
  {
    if (subsession->clientPortNum() == 0)
	{
      DbgLog((LOG_TRACE, 2, TEXT("Port on \"%s/%s\" subsession was not set"),
	      subsession->mediumName(), subsession->codecName()));
		continue; // port # was not set
	}

	Boolean streamUsingTCP = (Boolean)GetPrivateProfileInt("RTP", "over_tcp", 0, "MMRTPSrc.ini");
	WritePrivateProfileString("RTP", "over_tcp", streamUsingTCP ? "1" : "0", "MMRTPSrc.ini");

	if (streamUsingTCP)
		subsession->rtpSource()->setPacketReorderingThresholdTime(0);

    if (!m_rtspClient->setupMediaSubsession(*subsession,
											False /* streamOutgoing */,
										    streamUsingTCP)) 
	{
      DbgLog((LOG_ERROR, 1, TEXT("Failed to issue RTSP \"SETUP\" on \"%s/%s\" subsession: %s"),
	      subsession->mediumName(), subsession->codecName(),
	      m_env->getResultMsg()));
    }
	else 
	{
      DbgLog((LOG_TRACE, 1, TEXT("Issued RTSP \"SETUP\" on \"%s/%s\" subsession (client ports %d-%d)"),
	      subsession->mediumName(), subsession->codecName(),
	      subsession->clientPortNum(), subsession->clientPortNum()+1));
	  hr = S_OK;
    }
  }

  return hr;
}

HRESULT CFilterRTPSource::startPlayingRTSPStreams()
{
  MediaSubsessionIterator iter(*m_session);
  MediaSubsession *subsession;

  while ((subsession = iter.next()) != NULL) 
  {
    if (subsession->sessionId == NULL)
	{

		DbgLog((LOG_TRACE, 2, TEXT("No RTSP session in progress on \"%s/%s\" subsession"),
		  subsession->mediumName(), subsession->codecName()));
		continue; //no RTSP sess in progress
	}

    if (!m_rtspClient->playMediaSubsession(*subsession)) 
	{
      DbgLog((LOG_ERROR, 1, TEXT("RTSP \"PLAY\" command on \"%s/%s\" subsession failed: %s"),
	      subsession->mediumName(), subsession->codecName(),
	      m_env->getResultMsg()));
	  return S_FALSE;
    }
	else
	{
      DbgLog((LOG_TRACE, 1, TEXT("Issued RTSP \"PLAY\" command on \"%s/%s\" subsession"),
	      subsession->mediumName(), subsession->codecName()));
    }
  }

  return S_OK;
}

HRESULT CFilterRTPSource::tearDownRTSPStreams() 
{
  MediaSubsessionIterator iter(*m_session);
  MediaSubsession* subsession;
  while ((subsession = iter.next()) != NULL) 
  {
    if (subsession->sessionId == NULL) 
	{
		DbgLog((LOG_TRACE, 2, TEXT("No PLAY in progress on \"%s/%s\" subsession"),
		  subsession->mediumName(), subsession->codecName()));
		continue; // no PLAY in progress
	}
    
    DbgLog((LOG_TRACE, 1, TEXT("Tearing down \"%s/%s\" subsession"),
	    subsession->mediumName(), subsession->codecName()));

    m_rtspClient->teardownMediaSubsession(*subsession);
  }

  return S_OK;
}


HRESULT GetVideoFCCMediaType(DWORD fcc, unsigned w, unsigned h, unsigned b, unsigned f, CMediaType *pmt)
{
#define DEFINE_LOCAL_GUID(name, l, w1, w2, b1, b2, b3, b4, b5, b6, b7, b8) \
        const GUID name \
                = { l, w1, w2, { b1, b2,  b3,  b4,  b5,  b6,  b7,  b8 } }

	DEFINE_LOCAL_GUID(MEDIASUBTYPE_XXXX,
	fcc, 0x0000, 0x0010, 0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71);

	//pmt->InitMediaType();

	long lFmtLen = sizeof(VIDEOINFOHEADER); // + uVidFmtLen - sizeof(BITMAPINFOHEADER);

	/*if (fcc == FOURCC('M','J','P','G'))
		lFmtLen += sizeof(JPEGINFOHEADER);*/

    VIDEOINFOHEADER *pvi = (VIDEOINFOHEADER *) pmt->AllocFormatBuffer(lFmtLen);
    if (!pvi)
		return(E_OUTOFMEMORY);

    ZeroMemory(pvi, lFmtLen);

	pvi->bmiHeader.biSize			= sizeof(BITMAPINFOHEADER);

	/*if (fcc == FOURCC('M','J','P','G'))
		pvi->bmiHeader.biSize += sizeof(JPEGINFOHEADER);*/

	pvi->bmiHeader.biWidth			= w;
	pvi->bmiHeader.biHeight			= h;
	pvi->bmiHeader.biPlanes			= 1;
	pvi->bmiHeader.biBitCount		= b;
	pvi->bmiHeader.biCompression	= fcc;
	pvi->bmiHeader.biSizeImage		= 0; //GetBitmapSize(&pvi->bmiHeader);
	/*pvi->bmiHeader.biXPelsPerMeter	= ;
	pvi->bmiHeader.biYPelsPerMeter	= ;
	pvi->bmiHeader.biClrUsed		= ;
	pvi->bmiHeader.biClrImportant	= ;*/

    SetRectEmpty(&(pvi->rcSource));	// we want the whole image area rendered.
    SetRectEmpty(&(pvi->rcTarget));	// no particular destination rectangle

	/*pvi->rcSource.left = 0;
	pvi->rcSource.top = 0;
	pvi->rcSource.right = m_Parent->m_width;
	pvi->rcSource.bottom = m_Parent->m_height;

	pvi->rcTarget.left = 0;
	pvi->rcTarget.top = 0;
	pvi->rcTarget.right = m_Parent->m_presentation_width;
	pvi->rcTarget.bottom = m_Parent->m_presentation_height;*/

	pvi->AvgTimePerFrame = f ? (LONGLONG)(10000000 / f) : 0;

    pmt->SetType(&MEDIATYPE_Video);
	pmt->SetSubtype(&MEDIASUBTYPE_XXXX);
    pmt->SetFormatType(&FORMAT_VideoInfo);
    pmt->SetSampleSize(pvi->bmiHeader.biSizeImage);
    pmt->SetTemporalCompression(FALSE);

    // Work out the GUID for the subtype from the header info.
    /*const GUID SubTypeGUID = GetBitmapSubtype(&pvi->bmiHeader);
    pmt->SetSubtype(&SubTypeGUID);*/

    return NOERROR;

}

//-----------------------------------------------------------------
// ACM MP3
//-----------------------------------------------------------------
LRESULT ParseMP3Header(LPBYTE src,DWORD size, MPEGLAYER3WAVEFORMAT *pFormat)
{
	// WAVEFORMATEX
	if (size <= 128) return S_FALSE;

	// MP3
	if (src[0] !=0xff)	return S_FALSE;
	if (src[1]&0xf8 !=0xf8) return S_FALSE;

	int	anBitrate[2][3][16] =
	{
		{
		// MPEG-1
			{ 0,32,64,96,128,160,192,224,256,288,320,352,384,416,448,0 },	// 32000Hz(layer1)
			{ 0,32,48,56, 64, 80, 96,112,128,160,192,224,256,320,384,0 },	// 44100Hz(layer2)
			{ 0,32,40,48, 56, 64, 80, 96,112,128,160,192,224,256,320,0 },	// 48000Hz(layer3)
		},
		{
		// MPEG-2
			{ 0,32,48,56, 64, 80, 96,112,128,144,160,176,192,224,256,0 },	// 32000Hz(layer1)
			{ 0, 8,16,24, 32, 40, 48, 56, 64, 80, 96,112,128,144,160,0 },	// 44100Hz(layer2)
			{ 0, 8,16,24, 32, 40, 48, 56, 64, 80, 96,112,128,144,160,0 },	// 48000Hz(layer3)
		},
	};

	int anFreq[2][4] =
	{
		{ 44100,48000,32000,0 },
		{ 22050,24000,16000,0 },
	};

	int nLayer = 4-((src[1] >> 1) & 3);
	if(nLayer == 4)
	{
		// MP3
		return S_FALSE;
	}
	
	int nMpeg		= ((src[1] & 8) == 0) ? 1 : 0;
	int nBitrate	= anBitrate[nMpeg][nLayer-1][ src[2]>>4 ];
	int nFreq		= anFreq[nMpeg][(src[2] >> 2) & 3];
	int nChannel	= ((src[3] >> 6) == 3) ? 1 : 2;
	int nFrameSize	= 144000 * nBitrate / nFreq;

	//	MP3
   	ZeroMemory(pFormat, sizeof(MPEGLAYER3WAVEFORMAT));
	pFormat->wfx.cbSize			= MPEGLAYER3_WFX_EXTRA_BYTES;
	pFormat->wfx.wFormatTag		= WAVE_FORMAT_MPEGLAYER3;
	pFormat->wfx.nChannels		= nChannel;
	pFormat->wfx.nSamplesPerSec	= nFreq;
	pFormat->wfx.nAvgBytesPerSec= nBitrate * 1000 / 8;
	pFormat->wfx.nBlockAlign	= 1;
	pFormat->wfx.wBitsPerSample	= 0;
	pFormat->wID				= MPEGLAYER3_ID_MPEG;
	pFormat->fdwFlags			= MPEGLAYER3_FLAG_PADDING_OFF;
	pFormat->nBlockSize			= nFrameSize;
	pFormat->nFramesPerBlock	= 1;
	pFormat->nCodecDelay		= 0x0571;

	// ID3 Tag : Bug ???
	if ((src[size-128] == 'T') && (src[size-127] == 'A') && (src[size-126] == 'G'))
	{
		size-= 128;
	}
	return S_OK;

}

////////// DShowSink //////////

DShowSink::DShowSink(UsageEnvironment& env, MediaSubsession* subsession, CRTPSourceStream* pin, char *pName)
  : MediaSink(env), 
  m_subsession(subsession),
  pin(pin),
  sampleOffset(0),
  received(0),
  m_detected(False),
  m_BufferSize(0),
  m_Discontinuity(TRUE)
{
	if (!pin->IsConnected())
	{
	    // Create our allocators
		HRESULT hr;
		m_pAllocator = new CMainAllocator(
							   NULL, // No owner - allocators are separate objects
							   &hr);
		if (m_pAllocator == NULL) {
			return;
		}
		if (FAILED(hr)) {
			return;
		}

		//  The base classes expect the allocator to be AddRef'd
		m_pAllocator->AddRef();

		ALLOCATOR_PROPERTIES Props, Actual;

		char sz[256];
		int cbBuffer = GetPrivateProfileInt("FILTER", "buff_size", 64 * 1024, "MMRTPSrc.ini");
		sprintf(sz, "%d", cbBuffer);
		WritePrivateProfileString("FILTER", "buff_size", sz, "MMRTPSrc.ini");

		int cAud = GetPrivateProfileInt("FILTER", "nb_aud_buff", 4, "MMRTPSrc.ini");
		sprintf(sz, "%d", cAud);
		WritePrivateProfileString("FILTER", "nb_aud_buff", sz, "MMRTPSrc.ini");

		int cVid = GetPrivateProfileInt("FILTER", "nb_vid_buff", 32, "MMRTPSrc.ini");
		sprintf(sz, "%d", cVid);
		WritePrivateProfileString("FILTER", "nb_vid_buff", sz, "MMRTPSrc.ini");

		Props.cbBuffer = cbBuffer;
		Props.cBuffers = pName[0] == 'v' ? cVid : cAud;
		Props.cbAlign = 1;
		Props.cbPrefix = 0;
		((IMemAllocator *)m_pAllocator)->SetProperties(&Props, &Actual);

		m_pAllocator->Commit();

		m_pSubAllocator = new CSubAllocator(NULL, &hr, m_pAllocator);
		if (m_pSubAllocator == NULL) {
			return;
		}
		if (FAILED(hr)) {
			return;
		}
	}
	else
	{
	    //  Bind to our allocators (created at first connection)
		m_pSubAllocator = pin->Allocator();
		m_pAllocator = m_pSubAllocator->Allocator();
	}

	// To inform IAMOpenProgress of filter
	pin->GetFilter()->m_llTotal += m_pAllocator->GetSize();

	// Flush all pending samples
	IMediaSample * pSample;
	while (pSample = pin->m_SampleList->RemoveHead())
		pSample->Release();

	pin->m_sink = this;

	strcpy(m_pName, pName);
}

DShowSink::~DShowSink() 
{
	// Inform binded pin (CRTPSourceStream) we're dead
	pin->m_sink = NULL;
}

DShowSink* DShowSink::createNew(UsageEnvironment& env, MediaSubsession* subsession, CRTPSourceStream *pin, char *pName) {
  
  if (pin == NULL)
	  return NULL;

  return new DShowSink(env, subsession, pin, pName);
}

Boolean DShowSink::continuePlaying() 
{

	if (fSource == NULL) return False;

getbuf:
	// Get a buffer
	m_pBuffer = m_pAllocator->GetBuffer(&m_BufferSize);

	// Bufer overrun ?
	if (m_pBuffer == NULL)
	{
		// Have we a pending sample to skip ?
		IMediaSample *pSample = pin->m_SampleList->RemoveHead();
		if (pSample != NULL)
		{
			// forward discontinuity
			if (pSample->IsDiscontinuity())
			{
				IMediaSample *pNext = pin->m_SampleList->GetHead();
				if (pNext)
				{
					pNext->SetDiscontinuity(TRUE);
				}
				else
					m_Discontinuity = TRUE;
			}

			// skip it
			pSample->Release(); // will call CMainAllocator::Delivered(pSample)

			// retry getting a buffer
			goto getbuf;
		}

		stopPlaying(); // stop playing this stream, it's useless
		return False; // buffer overrun and no more sample to skip ... weird !!!
	}

	//ASSERT(m_BufferSize != 0);

	// JPEG
	if (m_pName[0] == 'v' && m_pName[6] == 'J')
		sampleOffset = 193; //193; // M-JPEG header size

	fSource->getNextFrame(m_pBuffer + sampleOffset, m_BufferSize - sampleOffset,
			afterGettingFrame, this,
			onSourceClosure, this);

	/*int socket = pin->GetFilter()->m_rtspClient->socketNum();
	send(socket, "$\0\0\0", 4, 0);*/

	return True;
}

/*  Helper to set MPEG timestamps */
REFERENCE_TIME TimeStamp(LONGLONG llPts, LONGLONG llFirstPts)
{
    LARGE_INTEGER liPtsOffset;
	llPts <<= 1;
	llFirstPts <<= 1;
    liPtsOffset.QuadPart = llPts - llFirstPts;
    liPtsOffset.HighPart &= 1;
    liPtsOffset.HighPart = -liPtsOffset.HighPart;
    return llMulDiv(liPtsOffset.QuadPart,
                    UNITS,
                    MPEG_TIME_DIVISOR,
                    0);
}

/*unsigned DShowSink::convertToRTPTimestamp(struct timeval timestamp) const {
  unsigned rtpTimestamp = fTimestampBase;
  rtpTimestamp += (timestampFrequency()*timestamp.tv_sec);
  rtpTimestamp += (unsigned)
    ((2.0*timestampFrequency()*timestamp.tv_usec + 1000000.0)/2000000);
       // note: rounding

#ifdef DEBUG_TIMESTAMPS
  fprintf(stderr, "timestamp base: %u, presentationTime: %u.%06u,\n\tRTP timestamp: %u\n",
	  fTimestampBase, timestamp.tv_sec,
	  timestamp.tv_usec, rtpTimestamp);
#endif

  return rtpTimestamp;
}*/

void DShowSink::afterGettingFrame(void* clientData, unsigned frameSize,
				 struct timeval presentationTime) 
{

  DShowSink* sink = (DShowSink*)clientData;

  sink->received += frameSize;

  // To inform IAMOpenProgress of filter
  sink->pin->GetFilter()->m_llCurrent += frameSize;

  if (!sink->m_detected && !sink->pin->IsMediaTypeFound())
  {
	if (strncmp(&sink->m_pName[6], "X-MS-DSHOW", 10) == 0)
	{
		/*char *Codec = &sink->m_pName[6 + 5];
		GetVideoFCCMediaType(FOURCC(Codec[0],Codec[1],Codec[2],Codec[3]),
			sink->m_subsession->videoWidth(),
			sink->m_subsession->videoHeight(),
			24,
			sink->m_subsession->videoFPS(),
			&sink->pin->m_sinkmt);*/

		sink->pin->m_sinkmt = *((MSVideoRTPSource *)sink->fSource)->fpmt;
		sink->m_detected = True;
		SetEvent(sink->pin->m_evtMediaTypeFound);

	}
	else if (sink->m_pName[0] == 'v')
	{
		// JPEG
		if (sink->m_pName[6] == 'J')
		{
			JPEGVideoRTPSource *pJpeg = (JPEGVideoRTPSource *)sink->fSource;
			GetVideoFCCMediaType(FOURCC('M','J','P','G'), 
				pJpeg->width, 
				pJpeg->height,
				24,
				0,
				&sink->pin->m_sinkmt);
			sink->m_detected = True;
			SetEvent(sink->pin->m_evtMediaTypeFound);
		}
		// MPEG1 sequence header detection
		else if (*(UNALIGNED DWORD *)(sink->m_pBuffer + sink->sampleOffset)== DWORD_SWAP(SEQUENCE_HEADER_CODE))
		{
			SEQHDR_INFO SeqHdrInfo;
			if (ParseSequenceHeader(sink->m_pBuffer + sink->sampleOffset, frameSize, &SeqHdrInfo))
			{
				GetVideoMediaType(&sink->pin->m_sinkmt, TRUE, &SeqHdrInfo);
				sink->m_detected = True;
				SetEvent(sink->pin->m_evtMediaTypeFound);
			}
		}
	}
	else if (sink->m_pName[0] == 'a')
	{
		// Raw audio
		if (sink->m_pName[6] == 'L')
		{
			// Parse SDP rtpmap
			// Something like : "rtpmap:96 L8/11025/1", "rtpmap:96 L16/22050", , "rtpmap:96 L16/44100/2", ...
			const char szTag[] = "rtpmap:96 L";
			char *pFormat = strstr(sink->pin->GetFilter()->m_sdpDescription, szTag);
			if (pFormat)
			{
				pFormat += strlen(szTag);
				char *pBits = NULL;
				char *pFreq = NULL;
				char *pChannel = NULL;
				char *p = pBits = strdup(pFormat);

				while (*p != '\0')
				{
					if (*p == '/')
					{
						*p = '\0';
						if (!pFreq)
							pFreq = p + 1;
						else
							pChannel = p + 1;
					}

					if (*p == '\r' || *p == '\n')
					{
						*p = '\0';
						break;
					}
					p++;
				}

				// Create format description (MediaType)
				if (pBits && pFreq)
				{
					int nBits		= atoi(pBits);
					int nFreq		= atoi(pFreq);
					int nChannel	= pChannel ? atoi(pChannel) : 1;

					static WAVEFORMATEX wf;
   					ZeroMemory(&wf, sizeof(WAVEFORMATEX));
					wf.wFormatTag = WAVE_FORMAT_PCM;
					wf.nSamplesPerSec = nFreq;
					wf.wBitsPerSample = nBits;
					wf.nChannels = nChannel;
					wf.nBlockAlign = wf.nChannels * (wf.wBitsPerSample / 8);
					wf.nAvgBytesPerSec = wf.nSamplesPerSec * wf.nBlockAlign;
					wf.cbSize = 0;
					CreateAudioMediaType(&wf, &sink->pin->m_sinkmt, TRUE);

					sink->m_detected = True;
					SetEvent(sink->pin->m_evtMediaTypeFound);
				}

				free(pBits);
			}
		}
		else
		{
			static MPEG1WAVEFORMAT wf;
			if (ParseAudioHeader(sink->m_pBuffer + sink->sampleOffset, &wf)) 
			{
				if (wf.fwHeadLayer == ACM_MPEG_LAYER3)
				{
					// MP3 audio header detection
					static MPEGLAYER3WAVEFORMAT wf;
					if (ParseMP3Header(sink->m_pBuffer + sink->sampleOffset, frameSize, &wf) == S_OK)
					{
						sink->pin->m_sinkmt.majortype = MEDIATYPE_Audio;
						sink->pin->m_sinkmt.subtype = MEDIASUBTYPE_MPEG3AudioPayload;
						sink->pin->m_sinkmt.SetFormat((PBYTE)&wf, sizeof(wf));
						sink->pin->m_sinkmt.SetFormatType(&FORMAT_WaveFormatEx);

						sink->m_detected = True;
						SetEvent(sink->pin->m_evtMediaTypeFound);
					}
				}
				else
				{
					sink->pin->m_sinkmt.majortype = MEDIATYPE_Audio;
					sink->pin->m_sinkmt.subtype = MEDIASUBTYPE_MPEG1AudioPayload;
					sink->pin->m_sinkmt.SetFormat((PBYTE)&wf, sizeof(wf));
					sink->pin->m_sinkmt.SetFormatType(&FORMAT_WaveFormatEx);

					sink->m_detected = True;
					SetEvent(sink->pin->m_evtMediaTypeFound);
				}

			}
		}
	}
  }

  // Create the sample
  CMediaSample *pSample = sink->m_pSubAllocator->GetSample(frameSize + sink->sampleOffset);

#define PT_TO_DS_TIME(p) MILLISECONDS_TO_100NS_UNITS(p.tv_sec * 1000) + (LONGLONG)p.tv_usec * 10

  if (sink->m_Discontinuity)
  {
	  sink->m_DiscontinuityTime = PT_TO_DS_TIME(presentationTime);

	  sink->m_CurrentTime = 0;

	  pSample->SetDiscontinuity(TRUE);

	  CRefTime rtStart = (LONG)0;
	  CRefTime rtEnd = rtStart + (LONG)1;
	  pSample->SetTime((REFERENCE_TIME *) &rtStart,(REFERENCE_TIME *) &rtEnd);

	  sink->m_Discontinuity = FALSE;
  }

  // video
  if (sink->m_pName[0] == 'v')
  {
	  // JPEG
	  if (sink->m_pName[6] == 'J')
	  {
		JPEGVideoRTPSource *pJpeg = (JPEGVideoRTPSource *)sink->fSource;
		if (pJpeg)
		{
			ASSERT(sink->sampleOffset == pJpeg->hdrlen);
			//ASSERT(frameSize == pJpeg->framesize);

			memcpy(sink->m_pBuffer, pJpeg->header, pJpeg->hdrlen);
		}

		// All JPEG frames are key-frames
		pSample->SetSyncPoint(TRUE);

#define JPEG_TIME
#ifdef JPEG_TIME
		// presentation time to DShow time
		if (PT_TO_DS_TIME(presentationTime) - sink->m_DiscontinuityTime < 0)
			MessageBox(0, "", "", 0);
		REFERENCE_TIME rtTime = PT_TO_DS_TIME(presentationTime) - sink->m_DiscontinuityTime;
		REFERENCE_TIME rtStart = sink->m_CurrentTime;
		REFERENCE_TIME rtEnd = rtTime;

		if (rtEnd > rtStart && sink->pin->IsMediaTypeFound())
		{
			VIDEOINFOHEADER *pvi = (VIDEOINFOHEADER *)sink->pin->CurrentMediaType()->Format();
			if (pvi)
			{
				pvi->AvgTimePerFrame = rtEnd - rtStart;
			}
		}

		if (rtEnd == rtStart)
			rtEnd += 1;

		if (rtEnd > rtStart)
		{
			sink->m_CurrentTime = rtEnd;

			//rtStart += UNITS / 2;
			//rtEnd += UNITS / 2;

			pSample->SetTime(&rtStart, &rtEnd);
		}
#endif

	  }
#define MPEG_TIME
#ifdef MPEG_TIME
	  else if (sink->m_pName[6] == 'M')
	  {
		// MPEG presentation time to DShow time ???
	  }
#endif
  }
  // audio
  else if (sink->m_pName[0] == 'a')
  {
		// presentation time to DShow time
		/*presentationTime.tv_sec -= sink->m_DiscontinuityTime.tv_sec;
		presentationTime.tv_usec -= sink->m_DiscontinuityTime.tv_usec;
		REFERENCE_TIME rtStart= PT_TO_DS_TIME(sink->m_CurrentTime);
		REFERENCE_TIME rtEnd = PT_TO_DS_TIME(presentationTime) + 1;//rtStart + MILLISECONDS_TO_100NS_UNITS(1000 / 15); //(LONG)1;
		pSample->SetTime(&rtStart, &rtEnd);
		sink->m_CurrentTime = presentationTime;*/
  }

  DbgLog((LOG_TRACE, 3, "Sample received Bytes(%d)", frameSize + sink->sampleOffset));

  if (strncmp(&sink->m_pName[6], "X-MS-DSHOW", 10) == 0)
  {
	  MSVideoRTPSource *pMsVideo = (MSVideoRTPSource *)(sink ? sink->fSource : NULL);
	if (pMsVideo && pMsVideo->fsProps)
	{
		sink->pin->m_sinkmt = *pMsVideo->fpmt;

		// pSample->SetMediaType(&sink->pin->m_sinkmt);
		if (pMsVideo->fsProps->Start != (REFERENCE_TIME)-1)
		{
			//pMsVideo->fsProps->Start += UNITS / 2;
			//pMsVideo->fsProps->End += UNITS / 2;
			/*if (pMsVideo->fsProps->Start <= UNITS / 2)
				pSample->SetPreroll(TRUE);*/
			pSample->SetTime(&pMsVideo->fsProps->Start, &pMsVideo->fsProps->End);
		}
		if (pMsVideo->fsProps->MediaStart != (LONGLONG)-1)
		{
			pSample->SetMediaTime(&pMsVideo->fsProps->MediaStart, &pMsVideo->fsProps->MediaEnd);
		}
		pSample->SetDiscontinuity(pMsVideo->fsProps->Discontinuity);
		pSample->SetPreroll(pMsVideo->fsProps->Preroll);
		pSample->SetSyncPoint(pMsVideo->fsProps->SyncPoint);

		DbgLog((LOG_TRACE, 3, "Start(%I64d 100ns) End(%I64d 100ns) Diff(%d ms) HeaderSize(%d) FrameSize(%d) MediaStart(%I64d) MediaEnd(%I64d) Diff(%d)", 
			pMsVideo->fsProps->Start,
			pMsVideo->fsProps->End,
			(LONG)((pMsVideo->fsProps->End - pMsVideo->fsProps->Start) / 10000),
			pMsVideo->fsProps->HeaderSize,
			pMsVideo->fsProps->FrameSize,
			pMsVideo->fsProps->MediaStart,
			pMsVideo->fsProps->MediaEnd,
			(LONG)(pMsVideo->fsProps->MediaEnd - pMsVideo->fsProps->MediaStart)
			));
		if (pMsVideo->fsProps->Discontinuity) DbgLog((LOG_TRACE, 3, "Discontinuity"));
		if (pMsVideo->fsProps->Preroll) DbgLog((LOG_TRACE, 3, "Preroll"));
		if (pMsVideo->fsProps->SyncPoint) DbgLog((LOG_TRACE, 3, "SyncPoint"));
	}
  }

  sink->pin->m_SampleList->AddTail(pSample);

  SetEvent(sink->pin->m_evtDataAvailable);

  // Then try getting the next frame:
  sink->continuePlaying();
}

//
// Constructor
//
CRTPSourceStream::CRTPSourceStream(HRESULT *phr,
                         CFilterRTPSource *pParent,
                         LPCWSTR pPinName) :
    CSourceStream(NAME("Morgan RTP Source Pin"),phr, pParent, pPinName),
	m_sink(NULL)
{
    CAutoLock cAutoLock(&m_cSharedState);

	char sz[256];
	DATA_AVAILABLE_TIMEOUT = GetPrivateProfileInt("FILTER", "data_available_timeout", 2000, "MMRTPSrc.ini");
	sprintf(sz, "%d", DATA_AVAILABLE_TIMEOUT);
	WritePrivateProfileString("FILTER", "data_available_timeout", sz, "MMRTPSrc.ini");

	MEDIA_TYPE_FOUND_TIMEOUT = GetPrivateProfileInt("FILTER", "media_type_found_timeout", 15000, "MMRTPSrc.ini");
	sprintf(sz, "%d", MEDIA_TYPE_FOUND_TIMEOUT);
	WritePrivateProfileString("FILTER", "media_type_found_timeout", sz, "MMRTPSrc.ini");

	m_evtDataAvailable = CreateEvent(NULL, TRUE, FALSE, NULL);
	m_evtsProcessLoop[0] = m_evtDataAvailable; // New data is available in incoming buffer
	m_evtsProcessLoop[1] = GetFilter()->m_evtStopRTP; // RTP receiving thread has been stopped

	m_evtMediaTypeFound = CreateEvent(NULL, TRUE, FALSE, NULL);
	m_evtsGetMediaType[0] = m_evtMediaTypeFound; // MediaType has been found in data of incoming buffer
	m_evtsGetMediaType[1] = GetFilter()->m_evtStopRTP; // RTP receiving thread has been stopped

	m_SampleList = new CSampleList(NAME("Sample Queue List"), TRUE, 256);

} // (Constructor)


//
// Destructor
//
CRTPSourceStream::~CRTPSourceStream()
{
    CAutoLock cAutoLock(&m_cSharedState);

	if (m_SampleList != NULL)
	{
		// Flush pending samples
		IMediaSample * pSample;
		while (pSample = m_SampleList->RemoveHead())
			pSample->Release();

		delete m_SampleList;
	}

	if (m_evtMediaTypeFound)
		CloseHandle(m_evtMediaTypeFound);

	if (m_evtDataAvailable)
		CloseHandle(m_evtDataAvailable);

} // (Destructor)


//
// DoBufferProcessingLoop
//
// Grabs a buffer and calls the users processing function.
HRESULT CRTPSourceStream::DoBufferProcessingLoop(void) 
{

#define ON_RETURN \
	if (m_sink) m_sink->stopPlaying();

    Command com;

	// get end time
	GetFilter()->ChangeRate();
	GetFilter()->GetDuration(&m_rtEnd); // 0x7F00000000000000 if unknown end time or live stream

    OnThreadStartPlay();

	int tp = GetPrivateProfileInt("FILTER", "thread_priority", GetThreadPriority(GetCurrentThread()), "MMRTPSrc.ini");
	SetThreadPriority(GetCurrentThread(), tp);
	char sztp[256];
	sprintf(sztp, "%d", tp);
	WritePrivateProfileString("FILTER", "thread_priority", sztp, "MMRTPSrc.ini");
/*	
THREAD_PRIORITY_IDLE            -15
THREAD_PRIORITY_LOWEST          -2
THREAD_PRIORITY_BELOW_NORMAL    -1
THREAD_PRIORITY_NORMAL          0
THREAD_PRIORITY_ABOVE_NORMAL    1
THREAD_PRIORITY_HIGHEST         2
THREAD_PRIORITY_TIME_CRITICAL   15
*/


	// init cur sample time
	m_rtCurStart = m_rtCurEnd = 0;

    do {
        while(!CheckRequest(&com)) {

			HRESULT hr = S_OK;

			// Get a sample
			IMediaSample *pSample = m_SampleList->RemoveHead();
			if (pSample == NULL)
			{
				// No more sample available ...

				// Is this stream finished ?
				if (m_rtCurEnd < m_rtEnd) // No
				{
					// be ready to wait for new sample
					ResetEvent(m_evtDataAvailable);
					//SwitchToThread();

					// estimate timeout
					DWORD dwTimeOut = (DWORD)((m_rtEnd - m_rtCurEnd) / 10000);
					if (dwTimeOut > DATA_AVAILABLE_TIMEOUT)
						dwTimeOut = DATA_AVAILABLE_TIMEOUT;

					// block until sample is available or RTP receiving thread stopped or timeout
					DWORD dwReason = WaitForMultipleObjects(2, m_evtsProcessLoop, FALSE, dwTimeOut);
					if (dwReason == WAIT_TIMEOUT ||
						dwReason == WAIT_OBJECT_0 + 1)
					{
						pSample = m_SampleList->RemoveHead();
						if (pSample == NULL)
							hr = S_FALSE; // timeout occured, may be end of stream or RTP receiving thread stopped
						else
							hr = S_OK; // got a sample before calling ResetEvent
					}
					else
						continue;   // go round again sample is available ...
				}
				else // Yes
				{
					// End of stream
					hr = S_FALSE;
				}
			}

			// Get current sample time
			if (pSample != NULL)		
				pSample->GetTime(&m_rtCurStart, &m_rtCurEnd);

            DbgLog((LOG_TRACE, 3, TEXT("Deliver Sample (%d)"), m_SampleList->GetCount()));

            if(hr == S_OK) 
			{
				if (pSample->IsDiscontinuity())
					NewSegment(0, m_rtEnd, 1.0);

				// deliver sample to downstream filter
                hr = Deliver(pSample);
				pSample->Release();

                // downstream filter returns S_FALSE if it wants us to
                // stop or an error if it's reporting an error.
                if(hr != S_OK) 
				{
                    DbgLog((LOG_TRACE, 2, TEXT("Deliver() returned %08x; stopping"), hr));

					ON_RETURN

                    return S_OK;
                }

            }
            else if(hr == S_FALSE) 
			{
                // No more sample available, end of stream or timeout

				ON_RETURN

				DeliverEndOfStream();

                return S_OK;
            }
            else 
			{
                // error
				pSample->Release();
                DbgLog((LOG_ERROR, 1, TEXT("Error %08lX from DoBufferProcessingLoop"), hr));
 				
				ON_RETURN

                DeliverEndOfStream();
				m_pFilter->NotifyEvent(EC_ERRORABORT, hr, 0);

                return hr;
            }
        }

        // For all commands sent to us there must be a Reply call!

        if(com == CMD_RUN || com == CMD_PAUSE) {
            Reply(NOERROR);
        }
        else if(com != CMD_STOP) {
            Reply((DWORD) E_UNEXPECTED);
            DbgLog((LOG_ERROR, 1, TEXT("Unexpected command!!!")));
        }
    } while(com != CMD_STOP);

	ON_RETURN

    return S_FALSE;
}

//
// GetMediaType
//
HRESULT CRTPSourceStream::GetMediaType(CMediaType *pmt)
{

    CAutoLock cAutoLock(m_pFilter->pStateLock());

	if (!m_mt.IsValid())
	{
		// block until media type is found or RTP receiving thread has been stopped or timeout
		DWORD dwReason = WaitForMultipleObjects(2, m_evtsGetMediaType, FALSE, MEDIA_TYPE_FOUND_TIMEOUT);
		if (dwReason == WAIT_TIMEOUT ||
			dwReason == WAIT_OBJECT_0 + 1)
		{
			// Stop receiving data, it's useless ...
			if (m_sink)
				m_sink->stopPlaying();

			//MessageBox(NULL, "VFW_E_TIMEOUT", "", NULL);
			return VFW_E_TIMEOUT;
		}

		if (!m_sink->m_detected)
		{
			//MessageBox(NULL, "E_UNEXPECTED", "", NULL);
			return E_UNEXPECTED;
		}

		//MessageBox(NULL, "GetMediaType", "", NULL);
		SetMediaType(&m_sinkmt);
	}

    CopyMediaType(pmt, &m_mt);

    return NOERROR;

} // GetMediaType

//
// SetMediaType
//
// Called when a media type is agreed between filters
//
HRESULT CRTPSourceStream::SetMediaType(const CMediaType *pMediaType)
{
    CAutoLock cAutoLock(m_pFilter->pStateLock());

    // Pass the call up to my base class

    HRESULT hr = CSourceStream::SetMediaType(pMediaType);
    if (SUCCEEDED(hr)) {

         return NOERROR;
    } 

    return hr;

} // SetMediaType

//
// OnThreadCreate
//
HRESULT CRTPSourceStream::OnThreadCreate()
{
    CAutoLock cAutoLockShared(&m_cSharedState);

    return NOERROR;

} // OnThreadCreate

// override this to set the buffer size and count. Return an error
// if the size/count is not to your liking.
HRESULT CRTPSourceStream::DecideBufferSize(
    IMemAllocator * pAlloc,
    ALLOCATOR_PROPERTIES * pProp
)
{
    pProp->cBuffers = 1; // Don't care about size,
    pProp->cbBuffer = 1; // we insist on our own allocator
    pProp->cbAlign = 1;  // (see DecideAllocator, InitAllocator)
    pProp->cbPrefix = 0;
    ALLOCATOR_PROPERTIES propActual;
    return pAlloc->SetProperties(pProp, &propActual);
}

//
//  Override DecideAllocator because we insist on our own allocator since
//  it's 0 cost in terms of bytes
//
HRESULT CRTPSourceStream::DecideAllocator(
    IMemInputPin *pPin,
    IMemAllocator **ppAlloc
)
{
    HRESULT hr = InitAllocator(ppAlloc);
    if (SUCCEEDED(hr)) {
        ALLOCATOR_PROPERTIES propRequest;
        ZeroMemory(&propRequest, sizeof(propRequest));
        hr = DecideBufferSize(*ppAlloc, &propRequest);
        if (SUCCEEDED(hr)) {
            // tell downstream pins that modification
            // in-place is not permitted
            hr = pPin->NotifyAllocator(*ppAlloc, TRUE);
            if (SUCCEEDED(hr)) {
                return NOERROR;
            }
        }
    }

    /* Likewise we may not have an interface to release */

    if (*ppAlloc) {
        (*ppAlloc)->Release();
        *ppAlloc = NULL;
    }
    return hr;
}

// override this to control the connection
// We use the subsample allocator derived from 
// the main allocator of DShowSink
HRESULT CRTPSourceStream::InitAllocator(IMemAllocator **ppAlloc)
{
    ASSERT(m_pAllocator == NULL);
    HRESULT hr = NOERROR;

	// Bind to the DShowSink's Sub Allocator
    *ppAlloc = m_sink->m_pSubAllocator;
    if (*ppAlloc == NULL) {
        return E_OUTOFMEMORY;
    }

    if (FAILED(hr)) {
        delete *ppAlloc;
        *ppAlloc = NULL;
        return hr;
    }
    /* Get a reference counted IID_IMemAllocator interface */
    (*ppAlloc)->AddRef();
    return NOERROR;
}

STDMETHODIMP CFilterRTPSource::get_ReceivedPackets (long* pReceivedPackets)
{

  if (m_session == NULL)
	  return S_FALSE;

  *pReceivedPackets = 0;

  // Check each subsession's RTPReceptionStatsDB
  MediaSubsessionIterator iter(*m_session);
  MediaSubsession* subsession;
  while ((subsession = iter.next()) != NULL) 
  {
    RTPSource* src = subsession->rtpSource();
    if (src != NULL) 
	{
		RTPReceptionStatsDB& allReceptionStats
		  = src->receptionStatsDB();

	  // Check each RTPReceptionStats
		RTPReceptionStatsDB::Iterator iterator(allReceptionStats);
		while (1) 
		{
		  RTPReceptionStats* receptionStats = iterator.next();
		  if (receptionStats == NULL) break;

		  *pReceivedPackets += receptionStats->totNumPacketsReceived();

		}
    }
  }

  return S_OK;
}

STDMETHODIMP CFilterRTPSource::get_LostPackets (long* pLostPackets)
{
  if (m_session == NULL)
	  return S_FALSE;

  *pLostPackets = 0;

 // Check each subsession's RTPReceptionStatsDB
  MediaSubsessionIterator iter(*m_session);
  MediaSubsession* subsession;
  while ((subsession = iter.next()) != NULL) 
  {
    RTPSource* src = subsession->rtpSource();
    if (src != NULL) 
	{
		RTPReceptionStatsDB& allReceptionStats
		  = src->receptionStatsDB();

	  // Check each RTPReceptionStats
		RTPReceptionStatsDB::Iterator iterator(allReceptionStats);
		while (1) 
		{
		  RTPReceptionStats* receptionStats = iterator.next();
		  if (receptionStats == NULL) break;

		  unsigned highestExtSeqNumReceived = receptionStats->highestExtSeqNumReceived();
		  unsigned totNumExpected  = highestExtSeqNumReceived - receptionStats->baseExtSeqNumReceived();
		  *pLostPackets += totNumExpected - receptionStats->totNumPacketsReceived();
		}
    }
  }

  return S_OK;
}

STDMETHODIMP CFilterRTPSource::get_ReceptionQuality (long* pReceptionQuality)
{
  if (m_session == NULL)
	  return S_FALSE;

  long nJitter = 0;
  long nStats = 0;

  // Check each subsession's RTPReceptionStatsDB
  MediaSubsessionIterator iter(*m_session);
  MediaSubsession* subsession;
  while ((subsession = iter.next()) != NULL) 
  {
    RTPSource* src = subsession->rtpSource();
    if (src != NULL) 
	{
		RTPReceptionStatsDB& allReceptionStats
		  = src->receptionStatsDB();

	  // Check each RTPReceptionStats
		RTPReceptionStatsDB::Iterator iterator(allReceptionStats);
		while (1) 
		{
		  RTPReceptionStats* receptionStats = iterator.next();
		  if (receptionStats == NULL) break;

		  nJitter += receptionStats->jitter();
		  nStats += 1000;
		}
    }
  }

  *pReceptionQuality = 1000 - (nStats ? (nJitter * 100 / nStats) : 0);
  *pReceptionQuality /= 10;

  return S_OK;
}

STDMETHODIMP CFilterRTPSource::get_SourceProtocol (long* pSourceProtocol)
/*
		0 Unknown protocol 
		1 Multicast 
		2 Multisession bridge 
		3 UDP 
		4 TCP 
		5 Media Streaming Broadcast Distribution (MSBD) 
		6 HTTP 
		7 File	
*/
{
  if (m_session == NULL)
	return S_FALSE;

  *pSourceProtocol = 0;

  // Check first subsession
  MediaSubsessionIterator iter(*m_session);
  MediaSubsession* subsession;
  if ((subsession = iter.next()) != NULL) 
  {
	if (IsMulticastAddress(subsession->connectionEndpointAddress()))
		*pSourceProtocol = 1; 
	else
	{
		if (subsession->rtcpChannelId > 0)
			*pSourceProtocol = 4;
		else
			*pSourceProtocol = 3;
	}
  }

  return S_OK;
}

		
STDMETHODIMP CFilterRTPSource::get_Bandwidth (long* pBandwidth)
{
	static DWORD timeOfLastCall = timeGetTime();
	static LONGLONG totBytesOfLastCall = m_llCurrent;

	DWORD now = timeGetTime();
	LONGLONG totBytes = m_llCurrent;

	// in k-bits/s
	if (now - timeOfLastCall > 0)
		*pBandwidth = (long)((totBytes - totBytesOfLastCall) * 8) / (now - timeOfLastCall);
	else
		*pBandwidth = 0;

	totBytesOfLastCall = totBytes;
	timeOfLastCall = now;
	return S_OK;
}

STDMETHODIMP CFilterRTPSource::Count(DWORD* pcStreams)
{
	*pcStreams = GetStreamCount();

	return S_OK;
}

STDMETHODIMP CFilterRTPSource::Enable(long lIndex, DWORD dwFlags)
{

	if (lIndex < 0) 
		return E_INVALIDARG;

	if (dwFlags & AMSTREAMSELECTENABLE_ENABLE)
	{
	}
	else if (dwFlags & AMSTREAMSELECTENABLE_ENABLEALL)
	{
	}
	else // Disable All
	{
	}

	return S_OK;
}


STDMETHODIMP CFilterRTPSource::Info(long lIndex, AM_MEDIA_TYPE** ppmt, DWORD* pdwFlags, LCID* plcid, DWORD* pdwGroup, WCHAR** ppszName, IUnknown** ppObject, IUnknown** ppUnk)
{
	/*
Locale, Language, and Sublanguage IDs
The following macro is defined for creating LCIDs (Winnt.h for 32-bit systems; Olenls.h for 16-bit systems and Macintosh):

//*
// LCID creation/extraction macros:
// 
// MAKELCID - construct locale ID from language ID and
// country code.
// 
#define MAKELCID(l)    ((DWORD)(((WORD)(l)) | (((DWORD)((WORD)(0))) << 16)))
 
There are two predefined LCID values. The system default locale is LOCALE_SYSTEM_DEFAULT, and the current user's locale is LOCALE_USER_DEFAULT.

Another macro constructs a LANGID:

// 
// Language ID creation/extraction macros:
// 
// MAKELANGID - Construct language ID from primary language ID and
// sublanguage ID
// 
#define MAKELANGID(p, s)            ((((USHORT)(s)) << 10) | (USHORT)(p))
 
The following three combinations of primary LANGID and sublanguage identifier (SUBLANGID) have special meanings:

Primary LANGID SUBLANGID Result 
LANG_NEUTRAL SUBLANG_NEUTRAL Language neutral 
LANG_NEUTRAL SUBLANG_SYS_DEFAULT System default language 
LANG_NEUTRAL SUBLANG_DEFAULT User default language 


For primary LANGIDs, the range 0x200 to 0x3ff is user definable. The range 0x000 to 0x1ff is reserved for system use. For SUBLANGIDs, the range 0x20 to 0x3f is user definable. The range 0x00 to 0x1f is reserved for system use.
	*/

	if (lIndex < 0) 
		return S_FALSE;

	CRTPSourceStream *pPin = GetStream(lIndex);

	if (ppmt)
	{
		*ppmt = CreateMediaType((AM_MEDIA_TYPE*)&pPin->m_sinkmt);
	}

	if (pdwFlags) *pdwFlags = AMSTREAMSELECTINFO_EXCLUSIVE | AMSTREAMSELECTINFO_ENABLED;

	if (plcid) *plcid = 50 - lIndex;
	
	if (pdwGroup) *pdwGroup = pPin->m_sinkmt.majortype.Data1;

	if (ppszName) 
	{
		*ppszName = (unsigned short*)CoTaskMemAlloc((wcslen(pPin->Name())+1)*sizeof(unsigned short));
		if (*ppszName != NULL)
			wcscpy(*ppszName, pPin->Name());
	}

	if (ppObject) *ppObject = NULL;

	if (ppUnk) *ppUnk = NULL;

	return S_OK;
}

//
// Source Seeking
//
STDMETHODIMP CFilterRTPSource::GetCurrentPosition(LONGLONG *pCurrent)
{
	*pCurrent = m_rtStart;

	return S_OK;
}

//
// ChangeStart
//
// Source Seeking
//
HRESULT CFilterRTPSource::ChangeStart() 
{
	m_rtStart += UNITS / 60;

	if (m_rtStop > m_rtDuration)
		m_rtStop = m_rtDuration;

	if (m_rtStart > m_rtStop)
		m_rtStart = m_rtStop;


	return NOERROR;
} // ChangeStart


//
// ChangeStop
//
// Source Seeking
//
HRESULT CFilterRTPSource::ChangeStop()
{
	if (m_rtStop > m_rtDuration)
		m_rtStop = m_rtDuration;

	return NOERROR;
} // ChangeStop


//
// ChangeRate
//
// Source Seeking
//
HRESULT CFilterRTPSource::ChangeRate()
{

	double dRate;
	GetRate(&dRate);

	// get end time
	float endTime = 0;
	if (m_session)
		endTime = m_session->playEndTime() * (float)10000000.0;
	m_rtDuration = (REFERENCE_TIME)endTime;
	if (m_rtDuration == 0)
		m_rtDuration = 0x7F00000000000000; // unknown end time or live stream

	m_rtStart = 0;
	m_rtStop = m_rtDuration;

	return NOERROR;
} // ChangeRate