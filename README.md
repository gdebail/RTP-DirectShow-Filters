# RTP-DirectShow-Filters

What is this ?

It is a set of DirectShow filters that allows you to perform media-streaming on your Windows PC :

Morgan RTP Source Filter (to receive media content over a network).
Morgan RTP Destination Filter (to send media content over a network).
DirectShow is a sub-system of Microsoft DirectX dedicated to media content on Windows platforms. A DirectShow filter is a software component (a kind of Plug-in) that adds features to DirectShow. When you install a new DirectShow filter on your system, every applications based on DirectShow (like Windows Media Player) can use it.

Morgan RTP DirectShow Filters are using RTP, RTCP, RTSP and SDP standard Internet protocols.

Screenshots


Windows Media Player automatically uses the
Morgan RTP Source Filter to play media content
delivered by a streaming server.


Windows Media Player 6 can display network
statistics of Morgan RTP Source Filter.


Morgan Streaming Server uses the Morgan RTP Destination Filter
to deliver streaming media content over a network.


 

Morgan RTP Source Filter :


Graph using the Morgan RTP Source Filter to play a 'JPEG' mov file
streamed by a Darwin Streaming Server.


Output window of the above graph.

Implements an RTSP client.
Receives RTP packets from an RTP server.
Supports unicast, mulcicast and RTP over TCP.
Supports RTCP protocol.
Can parse SDP files describing a streaming session.
Supports 'specific' payload type (96, X-MS-DSHOW) of Morgan RTP Destination filter.
Supports MPEG 1 Video RTP payload type.
Supports MPEG 1 Audio RTP payload type  (including MP3).
Supports a subset of JPEG RTP payload type (*).
Will support more RTP payload types like JPEG2000.
(*) With "Q field > 127" (see RFC 2435 : "3.1.4. - Q: 8 bits") and "Quantization Table Length = 128".


 

Morgan RTP Destination Filter :


Graph created by Morgan Streaming Server showing
Morgan RTP Destination Filter streaming a mj2 file.

Implements an RTSP session server.
Sends RTP packets to an RTP client.
Supports unicast, mulcicast and RTP over TCP.
Supports RTCP protocol.
Only supports a 'specific' RTP payload type (96, X-MS-DSHOW).
Will supports standards RTP payload types like MPEG, JPEG and JPEG2000.

 

Morgan Streaming Server :


Morgan Streaming Server using the Morgan RTP Destination Filter
to stream an mj2 file, window content describe the RTSP session.

Implements an RTSP server.Uses Morgan RTP Destination Filter to stream media content.
Can stream any media format supported by DirectShow including MPEG, MP3, AVI, MOV, DivX, via X-MS-DSHOW payload type.
Dynamically compress uncompressed PCM audio to MP3.
Will support live audio and video sources and will dynamically compress them before sending.

 

Server side :

Installation

Download the server side (62K), unzip its content in an empty directory (C:\MMRTPServer for instance) and run the install.bat file.

You can download a sample clip too :

Sample clip (806K)
(QuickTime - hinted - mov - 320x240 - 15 fps - JPEG - PCM audio).

These clips can be streamed by both Morgan Streaming Server and Darwin Streaming Server and can be played (on client side) by Windows Media Player 6 using Morgan RTP Source Filter and Morgan M-JPEG codec.

Content

Install.bat : Installation script.
RTPDest.ax : Morgan RTP Destination Filter.
MStrmSrv.exe : Morgan Streaming Server.

Usage

Copy media files to be served in the same directory (C:\MMRTPServer for instance). It can be any media format supported by DirectShow including MPEG, MP3, AVI, MOV, DivX.

Run Morgan Streaming Server (MStrmSrv.exe).

Media files are ready to be served and streamed to the client side. Note that RTSP server implemented in Morgan Streaming Server uses port 554.

Limitations

Current version only supports the  'specific' RTP payload type (96, X-MS-DSHOW). Next versions will supports standards RTP payload types like MPEG, JPEG and JPEG2000.

To stream MP3 files with Morgan Streaming Server these files need to have a .mpg or .mpa extension, not .mp3 ...

Configuration (mss.ini)

[RTSP]
port=554
thread_priority=0
[Session]
multicast=0
mcast_addr=239.255.42.42
[RTP]
thread_priority=0
[RTCP]
thread_priority=0

multicast can be 0 or 1 (multicast=0 means unicast)

thread_priority can be :

-15 (IDLE)
-2 (LOWEST)
-1 (BELOW_NORMAL)
0 (NORMAL)
1 (ABOVE_NORMAL)
2 (HIGHEST)
15 (TIME_CRITICAL)


 

Client side :

Installation

Download the client side (103K), unzip its content in an empty directory (C:\MMRTPClient for instance) and run the install.bat file.

Content

Install.bat : Installation script.
RTPSource.ax : Morgan RTP Source Filter.
RTPSource.reg : Registry script.

Usage

Run Windows Media Player 6 (mplayer2.exe).
Click on File menu, choose Open ...
Type the location of your media (something like rtsp://www.morganmultimedia.com/m2.mov or rtsp://192.168.0.1/m2.mov).
or

Click on Start button.
Click on Run.
Type the "rtsp://..." location of your media.
or

Double-click on an SDP file (with .sdp extension).
Limitations

Current version supports Windows Media Player versions 7, 8 and 9 only if statistics is set to 0 in [WMP] section of MMRTPSrc.ini configuration file. If statistics is set to 1, then only WMP 6 is supported.

Configuration (MMRTPSrc.ini)

[FILTER]
data_available_timeout=2000
media_type_found_timeout=15000
buff_size=65536
nb_aud_buff=4
nb_vid_buff=32
thread_priority=0
[RTP]
over_tcp=0
thread_priority=0
[WMP]
statistics=0

xxx_timeout are in milliseconds.

buff_size is in bytes.

thread_priority can be :

-15 (IDLE)
-2 (LOWEST)
-1 (BELOW_NORMAL)
0 (NORMAL)
1 (ABOVE_NORMAL)
2 (HIGHEST)
15 (TIME_CRITICAL)


 

Requirements :

Morgan RTP DirectShow Filters requires 32-bits Windows PC platform with support for DirectShow (now part of DirectX) :

Windows 98, Me, 2000, XP support DirectShow by default.

Windows 95 requires an extra installation.

Windows NT can poorly support DirectShow (aka DirectX Media).

Morgan RTP DirectShow Filters have been compiled using DirectX 8.1 SDK but they should work with previous and future versions of DirectX runtime.

 

Source code :

Source code of Morgan Streaming Server is available here (505K).

Note that it contains a modified version of liveMedia library.

You need MS VC++ 6 and DirectX 8.x SDK installed on your computer to build it.

Morgan Streaming Server project is :
\MSS_v1_0\RTP\src\MStrmSrv\MStrmSrv.dsw

RTP Destination Filter COM Interface (IStreamingSession) is defined in :
\MSS_v1_0\RTP\src\RTPDest\iStrmSes.h

RTP Destination Filter GUID is defined in :
\MSS_v1_0\RTP\src\RTPDest\RTPDestuids.h

All calls to IStreamingSession (RTP Destination Filter) are done by CStreamingServerSession::onRTSPcmd in \MSS_v1_0\RTP\src\MStrmSrv\MStrmSrv.cpp

Note that source code of Morgan RTP Source and Destination filters is not publically available.
