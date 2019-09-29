# Microsoft Developer Studio Project File - Name="RTPSource" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=RTPSource - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "RTPSource.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "RTPSource.mak" CFG="RTPSource - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "RTPSource - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "RTPSource - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "RTPSource - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "..\..\build\RTPSource\release"
# PROP Intermediate_Dir "..\..\build\RTPSource\release\obj"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /YX /FD /c
# ADD CPP /nologo /MD /W3 /Gm /GX /Zi /O2 /Ob2 /I ".\mpgparse\include" /I "..\liveMedia\include" /I "..\BasicUsageEnvironment\include" /I "..\groupsock\include" /I "..\UsageEnvironment\include" /D CRTAPI1=_cdecl /D CRTAPI2=_cdecl /D _X86_=1 /D "_WIN95" /D _WIN32_WINDOWS=0x0400 /D _WIN32_IE=0x0300 /D WINVER=0x0400 /D "WIN32" /D "_WIN32" /D "_MT" /D "_DLL" /FR /YX /Oxs /GF /D_WIN32_WINNT=-0x0400 /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /i "..\..\BaseClasses" /d "NDEBUG" /d "WIN32"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /machine:I386
# ADD LINK32 wsock32.lib oldnames.lib msvcrt.lib msvcirt.lib strmbase.lib quartz.lib vfw32.lib winmm.lib kernel32.lib advapi32.lib version.lib largeint.lib user32.lib gdi32.lib comctl32.lib ole32.lib olepro32.lib oleaut32.lib uuid.lib /nologo /stack:0x200000,0x200000 /entry:"DllEntryPoint@12" /dll /pdb:none /machine:I386 /nodefaultlib /def:".\RTPSource.def" /out:"..\..\build\RTPSource\release\RTPSource.ax" /libpath:"..\..\..\..\lib" /subsystem:windows,4.0 /opt:ref /release /debug:none /OPT:NOREF /OPT:ICF
# Begin Special Build Tool
TargetPath=\MMCodecV3.1\RTP\build\RTPSource\release\RTPSource.ax
SOURCE="$(InputPath)"
PostBuild_Desc=Register Filter
PostBuild_Cmds=regsvr32 /s $(TargetPath)
# End Special Build Tool

!ELSEIF  "$(CFG)" == "RTPSource - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "..\..\build\RTPSource\debug"
# PROP Intermediate_Dir "..\..\build\RTPSource\debug\obj"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /YX /FD /c
# ADD CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /I ".\mpgparse\mpeg1" /I ".\mpgparse\include" /I "..\liveMedia\include" /I "..\BasicUsageEnvironment\include" /I "..\groupsock\include" /I "..\UsageEnvironment\include" /D CRTAPI1=_cdecl /D CRTAPI2=_cdecl /D _X86_=1 /D "_WIN95" /D _WIN32_WINDOWS=0x0400 /D _WIN32_IE=0x0300 /D WINVER=0x0400 /D "WIN32" /D "_WIN32" /D "_MT" /D "_DLL" /D DBG=1 /D "DEBUG" /D "_DEBUG" /Fr /YX /Oid /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "_DEBUG" /D __WIN32__=1 /mktyplib203 /o "NUL" /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG" /d "WIN32" /d __WIN32__=1 /d _WINNT=1
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /debug /machine:I386 /pdbtype:sept
# ADD LINK32 wsock32.lib oldnames.lib msvcrtd.lib msvcirtd.lib strmbasd.lib quartz.lib vfw32.lib winmm.lib kernel32.lib advapi32.lib version.lib largeint.lib user32.lib gdi32.lib comctl32.lib ole32.lib olepro32.lib oleaut32.lib uuid.lib /nologo /stack:0x200000,0x200000 /entry:"DllEntryPoint@12" /dll /pdb:none /machine:I386 /nodefaultlib /out:"..\..\build\RTPSource\debug\RTPSource.ax" /debug:mapped,full /subsystem:windows,4.0
# Begin Special Build Tool
TargetPath=\MMCodecV3.1\RTP\build\RTPSource\debug\RTPSource.ax
SOURCE="$(InputPath)"
PostBuild_Desc=Register Filter
PostBuild_Cmds=regsvr32 /s $(TargetPath)
# End Special Build Tool

!ENDIF 

# Begin Target

# Name "RTPSource - Win32 Release"
# Name "RTPSource - Win32 Debug"
# Begin Group "header"

# PROP Default_Filter "*.h"
# Begin Source File

SOURCE=.\alloc.h
# End Source File
# Begin Source File

SOURCE=.\resource.h
# End Source File
# Begin Source File

SOURCE=.\RTPSource.h
# End Source File
# End Group
# Begin Group "Filter"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\alloc.cpp
DEP_CPP_ALLOC=\
	".\alloc.h"\
	{$(INCLUDE)}"amextra.h"\
	{$(INCLUDE)}"amfilter.h"\
	{$(INCLUDE)}"audevcod.h"\
	{$(INCLUDE)}"cache.h"\
	{$(INCLUDE)}"combase.h"\
	{$(INCLUDE)}"cprop.h"\
	{$(INCLUDE)}"ctlutil.h"\
	{$(INCLUDE)}"dllsetup.h"\
	{$(INCLUDE)}"dsschedule.h"\
	{$(INCLUDE)}"fourcc.h"\
	{$(INCLUDE)}"measure.h"\
	{$(INCLUDE)}"msgthrd.h"\
	{$(INCLUDE)}"mtype.h"\
	{$(INCLUDE)}"outputq.h"\
	{$(INCLUDE)}"pstream.h"\
	{$(INCLUDE)}"refclock.h"\
	{$(INCLUDE)}"reftime.h"\
	{$(INCLUDE)}"renbase.h"\
	{$(INCLUDE)}"source.h"\
	{$(INCLUDE)}"streams.h"\
	{$(INCLUDE)}"strmctl.h"\
	{$(INCLUDE)}"sysclock.h"\
	{$(INCLUDE)}"transfrm.h"\
	{$(INCLUDE)}"transip.h"\
	{$(INCLUDE)}"videoctl.h"\
	{$(INCLUDE)}"vtrans.h"\
	{$(INCLUDE)}"winctrl.h"\
	{$(INCLUDE)}"winutil.h"\
	{$(INCLUDE)}"wxdebug.h"\
	{$(INCLUDE)}"wxlist.h"\
	{$(INCLUDE)}"wxutil.h"\
	
# End Source File
# Begin Source File

SOURCE=.\ProdInfo.h
# End Source File
# Begin Source File

SOURCE=.\RTPFilter.cpp
DEP_CPP_RTPFI=\
	"..\BasicUsageEnvironment\include\BasicUsageEnvironment.hh"\
	"..\BasicUsageEnvironment\include\BasicUsageEnvironment_version.hh"\
	"..\BasicUsageEnvironment\include\DelayQueue.hh"\
	"..\BasicUsageEnvironment\include\Lock.hh"\
	"..\groupsock\include\GroupEId.hh"\
	"..\groupsock\include\Groupsock.hh"\
	"..\groupsock\include\groupsock_version.hh"\
	"..\groupsock\include\GroupsockHelper.hh"\
	"..\groupsock\include\NetAddress.hh"\
	"..\groupsock\include\NetInterface.hh"\
	"..\liveMedia\include\BasicUDPSource.hh"\
	"..\liveMedia\include\ByteStreamFileSource.hh"\
	"..\liveMedia\include\DeviceSource.hh"\
	"..\livemedia\include\dspushrtpsink.hh"\
	"..\liveMedia\include\FileSink.hh"\
	"..\liveMedia\include\FramedFileSource.hh"\
	"..\liveMedia\include\FramedFilter.hh"\
	"..\liveMedia\include\FramedSource.hh"\
	"..\liveMedia\include\GSMAudioRTPSink.hh"\
	"..\liveMedia\include\H263plusVideoRTPSink.hh"\
	"..\liveMedia\include\H263plusVideoRTPSource.hh"\
	"..\liveMedia\include\HTTPSink.hh"\
	"..\liveMedia\include\JPEGVideoRTPSource.hh"\
	"..\liveMedia\include\liveMedia.hh"\
	"..\liveMedia\include\liveMedia_version.hh"\
	"..\liveMedia\include\Media.hh"\
	"..\liveMedia\include\MediaSession.hh"\
	"..\liveMedia\include\MediaSink.hh"\
	"..\liveMedia\include\MediaSource.hh"\
	"..\liveMedia\include\MP3ADU.hh"\
	"..\liveMedia\include\MP3ADUinterleaving.hh"\
	"..\liveMedia\include\MP3ADURTPSink.hh"\
	"..\liveMedia\include\MP3ADURTPSource.hh"\
	"..\liveMedia\include\MP3ADUTranscoder.hh"\
	"..\liveMedia\include\MP3FileSource.hh"\
	"..\liveMedia\include\MP3HTTPSource.hh"\
	"..\liveMedia\include\MP3Transcoder.hh"\
	"..\liveMedia\include\MPEGAudioRTPSink.hh"\
	"..\liveMedia\include\MPEGAudioRTPSource.hh"\
	"..\liveMedia\include\MPEGAudioStreamFramer.hh"\
	"..\liveMedia\include\MPEGDemux.hh"\
	"..\liveMedia\include\MPEGDemuxedElementaryStream.hh"\
	"..\liveMedia\include\MPEGVideoHTTPSink.hh"\
	"..\liveMedia\include\MPEGVideoRTPSink.hh"\
	"..\liveMedia\include\MPEGVideoRTPSource.hh"\
	"..\liveMedia\include\MPEGVideoStreamFramer.hh"\
	"..\livemedia\include\msvideortpsink.hh"\
	"..\livemedia\include\msvideortpsource.hh"\
	"..\liveMedia\include\MultiFramedRTPSink.hh"\
	"..\liveMedia\include\MultiFramedRTPSource.hh"\
	"..\liveMedia\include\PrioritizedRTPStreamSelector.hh"\
	"..\liveMedia\include\QCELPAudioRTPSource.hh"\
	"..\liveMedia\include\QuickTimeFileSink.hh"\
	"..\liveMedia\include\QuickTimeGenericRTPSource.hh"\
	"..\liveMedia\include\RTCP.hh"\
	"..\liveMedia\include\RTPInterface.hh"\
	"..\liveMedia\include\RTPSink.hh"\
	"..\liveMedia\include\RTPSource.hh"\
	"..\liveMedia\include\RTSPClient.hh"\
	"..\liveMedia\include\RTSPServer.hh"\
	"..\liveMedia\include\ServerMediaSession.hh"\
	"..\liveMedia\include\SimpleRTPSink.hh"\
	"..\liveMedia\include\SimpleRTPSource.hh"\
	"..\UsageEnvironment\include\Boolean.hh"\
	"..\UsageEnvironment\include\HashTable.hh"\
	"..\UsageEnvironment\include\UsageEnvironment.hh"\
	"..\UsageEnvironment\include\UsageEnvironment_version.hh"\
	".\alloc.h"\
	".\mpgparse\include\mpeg2typ.h"\
	".\mpgparse\include\mpegdef.h"\
	".\mpgparse\include\mpgutil.h"\
	".\RTPSource.h"\
	{$(INCLUDE)}"amextra.h"\
	{$(INCLUDE)}"amfilter.h"\
	{$(INCLUDE)}"audevcod.h"\
	{$(INCLUDE)}"cache.h"\
	{$(INCLUDE)}"combase.h"\
	{$(INCLUDE)}"cprop.h"\
	{$(INCLUDE)}"ctlutil.h"\
	{$(INCLUDE)}"dllsetup.h"\
	{$(INCLUDE)}"dsschedule.h"\
	{$(INCLUDE)}"fourcc.h"\
	{$(INCLUDE)}"measure.h"\
	{$(INCLUDE)}"msgthrd.h"\
	{$(INCLUDE)}"mtype.h"\
	{$(INCLUDE)}"outputq.h"\
	{$(INCLUDE)}"pstream.h"\
	{$(INCLUDE)}"refclock.h"\
	{$(INCLUDE)}"reftime.h"\
	{$(INCLUDE)}"renbase.h"\
	{$(INCLUDE)}"source.h"\
	{$(INCLUDE)}"streams.h"\
	{$(INCLUDE)}"strmctl.h"\
	{$(INCLUDE)}"sysclock.h"\
	{$(INCLUDE)}"transfrm.h"\
	{$(INCLUDE)}"transip.h"\
	{$(INCLUDE)}"videoctl.h"\
	{$(INCLUDE)}"vtrans.h"\
	{$(INCLUDE)}"winctrl.h"\
	{$(INCLUDE)}"winutil.h"\
	{$(INCLUDE)}"wxdebug.h"\
	{$(INCLUDE)}"wxlist.h"\
	{$(INCLUDE)}"wxutil.h"\
	
# End Source File
# Begin Source File

SOURCE=.\RTPSource.def

!IF  "$(CFG)" == "RTPSource - Win32 Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "RTPSource - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\RTPSource.rc
# End Source File
# Begin Source File

SOURCE=.\VersionNo.h
# End Source File
# End Group
# Begin Group "mpegparse"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\mpgparse\mpgutil\mpgutil.cpp
DEP_CPP_MPGUT=\
	".\mpgparse\include\mpeg2typ.h"\
	".\mpgparse\include\mpegdef.h"\
	".\mpgparse\include\mpgutil.h"\
	{$(INCLUDE)}"amextra.h"\
	{$(INCLUDE)}"amfilter.h"\
	{$(INCLUDE)}"audevcod.h"\
	{$(INCLUDE)}"cache.h"\
	{$(INCLUDE)}"combase.h"\
	{$(INCLUDE)}"cprop.h"\
	{$(INCLUDE)}"ctlutil.h"\
	{$(INCLUDE)}"dllsetup.h"\
	{$(INCLUDE)}"dsschedule.h"\
	{$(INCLUDE)}"fourcc.h"\
	{$(INCLUDE)}"measure.h"\
	{$(INCLUDE)}"msgthrd.h"\
	{$(INCLUDE)}"mtype.h"\
	{$(INCLUDE)}"outputq.h"\
	{$(INCLUDE)}"pstream.h"\
	{$(INCLUDE)}"refclock.h"\
	{$(INCLUDE)}"reftime.h"\
	{$(INCLUDE)}"renbase.h"\
	{$(INCLUDE)}"source.h"\
	{$(INCLUDE)}"streams.h"\
	{$(INCLUDE)}"strmctl.h"\
	{$(INCLUDE)}"sysclock.h"\
	{$(INCLUDE)}"transfrm.h"\
	{$(INCLUDE)}"transip.h"\
	{$(INCLUDE)}"videoctl.h"\
	{$(INCLUDE)}"vtrans.h"\
	{$(INCLUDE)}"winctrl.h"\
	{$(INCLUDE)}"winutil.h"\
	{$(INCLUDE)}"wxdebug.h"\
	{$(INCLUDE)}"wxlist.h"\
	{$(INCLUDE)}"wxutil.h"\
	
# End Source File
# End Group
# End Target
# End Project
