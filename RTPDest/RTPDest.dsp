# Microsoft Developer Studio Project File - Name="RTPDest" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=RTPDest - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "RTPDest.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "RTPDest.mak" CFG="RTPDest - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "RTPDest - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "RTPDest - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "RTPDest - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "..\..\build\RTPDest\release"
# PROP Intermediate_Dir "..\..\build\RTPDest\release\obj"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "RTPDest_EXPORTS" /YX /FD /c
# ADD CPP /nologo /G6 /MD /W3 /GX /O2 /Ob2 /I "..\liveMedia\include" /I "..\BasicUsageEnvironment\include" /I "..\groupsock\include" /I "..\UsageEnvironment\include" /D CRTAPI1=_cdecl /D CRTAPI2=_cdecl /D _X86_=1 /D "_WIN95" /D _WIN32_WINDOWS=0x0400 /D _WIN32_IE=0x0300 /D WINVER=0x0400 /D "WIN32" /D "_WIN32" /D "_MT" /D "_DLL" /Fr /YX /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /i "..\..\BaseClasses" /d "NDEBUG" /d "WIN32"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /machine:I386
# ADD LINK32 wsock32.lib oldnames.lib msvcrt.lib msvcirt.lib strmbase.lib quartz.lib vfw32.lib winmm.lib kernel32.lib advapi32.lib version.lib largeint.lib user32.lib gdi32.lib comctl32.lib ole32.lib olepro32.lib oleaut32.lib uuid.lib /nologo /stack:0x200000,0x200000 /entry:"DllEntryPoint@12" /dll /machine:I386 /nodefaultlib /out:"..\..\build\RTPDest\release\RTPDest.ax"
# SUBTRACT LINK32 /incremental:yes
# Begin Special Build Tool
TargetPath=\MMCodecV3.1\RTP\build\RTPDest\release\RTPDest.ax
SOURCE="$(InputPath)"
PostBuild_Desc=Register Filter
PostBuild_Cmds=regsvr32 /s $(TargetPath)
# End Special Build Tool

!ELSEIF  "$(CFG)" == "RTPDest - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "..\..\build\RTPDest\debug"
# PROP Intermediate_Dir "..\..\build\RTPDest\debug\obj"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "RTPDest_EXPORTS" /YX /FD /GZ /c
# ADD CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /I "..\liveMedia\include" /I "..\BasicUsageEnvironment\include" /I "..\groupsock\include" /I "..\UsageEnvironment\include" /D CRTAPI1=_cdecl /D CRTAPI2=_cdecl /D _X86_=1 /D "_WIN95" /D _WIN32_WINDOWS=0x0400 /D _WIN32_IE=0x0300 /D WINVER=0x0400 /D "WIN32" /D "_WIN32" /D "_MT" /D "_DLL" /D DBG=1 /D "DEBUG" /D "_DEBUG" /Fr /YX /FD /GZ /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /D __WIN32__=1 /mktyplib203 /o "NUL" /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG" /d "WIN32" /d __WIN32__=1 /d _WINNT=1
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /debug /machine:I386 /pdbtype:sept
# ADD LINK32 wsock32.lib oldnames.lib msvcrtd.lib msvcirtd.lib strmbasd.lib quartz.lib vfw32.lib winmm.lib kernel32.lib advapi32.lib version.lib largeint.lib user32.lib gdi32.lib comctl32.lib ole32.lib olepro32.lib oleaut32.lib uuid.lib /nologo /stack:0x200000,0x200000 /entry:"DllEntryPoint@12" /dll /pdb:none /machine:I386 /nodefaultlib /out:"..\..\build\RTPDest\debug\RTPDest.ax" /debug:mapped,full /subsystem:windows,4.0
# Begin Special Build Tool
TargetPath=\MMCodecV3.1\RTP\build\RTPDest\debug\RTPDest.ax
SOURCE="$(InputPath)"
PostBuild_Desc=Register Filter
PostBuild_Cmds=regsvr32 /s $(TargetPath)
# End Special Build Tool

!ENDIF 

# Begin Target

# Name "RTPDest - Win32 Release"
# Name "RTPDest - Win32 Debug"
# Begin Group "DShow"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\iStrmSes.h
# End Source File
# Begin Source File

SOURCE=.\RTPDest.cpp
# End Source File
# Begin Source File

SOURCE=.\RTPDest.def
# End Source File
# Begin Source File

SOURCE=.\RTPDest.h
# End Source File
# Begin Source File

SOURCE=.\RTPDestProp.cpp
# End Source File
# Begin Source File

SOURCE=.\RTPDestProp.h
# End Source File
# Begin Source File

SOURCE=.\RTPDestProp.rc
# End Source File
# Begin Source File

SOURCE=.\RTPDestuids.h
# End Source File
# End Group
# Begin Source File

SOURCE=.\Morgan.bmp
# End Source File
# End Target
# End Project
