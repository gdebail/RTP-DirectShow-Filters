//----------------------------------------------------------------------------
// iStrmSes.h
//----------------------------------------------------------------------------

// A custom interface to allow the user to manage Streaming Sessions

#ifndef __IStreamingSession__
#define __IStreamingSession__

#ifdef __cplusplus
extern "C" {
#endif


//----------------------------------------------------------------------------
// IStreamingSession's GUID
//
DEFINE_GUID(IID_IStreamingSession, 
0xe5b059b2, 0x65a6, 0x400a, 0xa1, 0x13, 0x6, 0xf4, 0x6e, 0xb4, 0x88, 0xdd);

//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// IStreamingSession
//----------------------------------------------------------------------------
DECLARE_INTERFACE_(IStreamingSession, IUnknown)
{
	STDMETHOD(CreateMediaSession) (void *env, const char *destinationAddress, int audPortBase = 0, int vidPortBase = 0) PURE;
	// env type = (UsageEnvironment *)
	// audPortBase = 0, vidPortBase = 0 => automatically choosen by RTP dest filter
	STDMETHOD(Open) (THIS_ const char *urlSuffix) PURE;
	STDMETHOD(Setup) (THIS_ const char *trackID) PURE;
	STDMETHOD(Play) (THIS_ const char *trackID) PURE;
	STDMETHOD(Pause) (THIS_ const char *trackID) PURE;
	STDMETHOD(Mute) (THIS_ const char *trackID) PURE;
	STDMETHOD(Unmute) (THIS_ const char *trackID) PURE;
	STDMETHOD(Teardown) (THIS_ const char *trackID) PURE;
	STDMETHOD(Close) (THIS_) PURE;

};
//----------------------------------------------------------------------------

#ifdef __cplusplus
}
#endif

#endif // __IStreamingSession__
