//==========================================================================;
//
//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
//  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
//  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
//  PURPOSE.
//
//  Copyright (c) 1992 - 1998  Microsoft Corporation.  All Rights Reserved.
//
//--------------------------------------------------------------------------;

//#include <windows.h>
//#include <windowsx.h>
#include <streams.h>
#include <commctrl.h>
//#include <olectl.h>
//#include <memory.h>
#include <stdlib.h>
#include <stdio.h>
#include <tchar.h>
#include "resource.h"
//#include "RTPDestuids.h"
#include "RTPDest.h"
#include "RTPDestProp.h"


//
// CreateInstance
//
// Used by the ActiveMovie base classes to create instances
//
CUnknown *CRTPDestProperties::CreateInstance(LPUNKNOWN lpunk, HRESULT *phr)
{
    CUnknown *punk = new CRTPDestProperties(lpunk, phr);
    if (punk == NULL) {
	*phr = E_OUTOFMEMORY;
    }
    return punk;

} // CreateInstance

//
// Constructor
//
CRTPDestProperties::CRTPDestProperties(LPUNKNOWN pUnk, HRESULT *phr) :
    CBasePropertyPage(NAME("Morgan RTP Dest Property Page"),
                      pUnk,IDD_RTPDESTPROP,IDS_TITLE),
	//m_pVCRTPDest(NULL),
    //m_pIPRTPDest(NULL),
    m_bIsInitialized(FALSE)
{
    ASSERT(phr);
	
	m_max_aud_size = 0;
	m_min_aud_size = 32765; //INT_MAX;

} // (Constructor)

void CRTPDestProperties::DispFileSize()
{
	TCHAR   sz[60];

	// File size (estimation)
	unsigned int file_size = 0; //m_pVfwCD->SendDriverMessage(MJ2M_GETFILESIZE, 0, 0);

	if (file_size < 10000)
		_stprintf(sz, TEXT("%i"), file_size);
	else if (file_size < 1024 * 1024)
		_stprintf(sz, TEXT("%.01fK"), (double)file_size / 1024.0);
	else
		_stprintf(sz, TEXT("%.01fM"), (double)file_size / (1024.0 * 1024.0));
	SetWindowText(GetDlgItem(m_Dlg, IDC_FILESIZE), sz);
}

int CRTPDestProperties::OnVideo(int nFrame, int nFrameSize)
{
	TCHAR   sz[60];
	REFTIME fps;
	unsigned int vid_cur_size;
	unsigned int vid_tot_size;

	_stprintf(sz, TEXT("%i"), nFrame);
	SetWindowText(GetDlgItem(m_Dlg, IDC_FRAMES), sz);

	//if (nFrame == 1)
	{
		REFTIME framerate;
		framerate = 0.0; //m_pVfwCD->SendDriverMessage(MJ2M_GETFRAMERATE, (WPARAM)&framerate, 0);

		_stprintf(sz, TEXT("%.02f"), framerate);
		SetWindowText(GetDlgItem(m_Dlg, IDC_FRAMERATE), sz);
	}

	fps = 0.0; //m_pVfwCD->SendDriverMessage(MJ2M_GETPROCESSINGTIME, (WPARAM)&fps, 0);
	if (fps)
		fps = (double)nFrame / fps;
	_stprintf(sz, TEXT("%.02f"), fps);
	SetWindowText(GetDlgItem(m_Dlg, IDC_FPS), sz);

	vid_cur_size = 0; //m_pVfwCD->SendDriverMessage(MJ2M_GETFRAMESIZE, 0, 0);

	if (vid_cur_size < 10000)
		_stprintf(sz, TEXT("%i (%i)"), nFrame, vid_cur_size);
	else
		_stprintf(sz, TEXT("%i (%.01fK)"), nFrame, (double)vid_cur_size / 1024.0);
	SetWindowText(GetDlgItem(m_Dlg, IDC_FRAMECOUNT), sz);

	vid_tot_size = 0; //m_pVfwCD->SendDriverMessage(MJ2M_GETVIDTOTSIZE, 0, 0);

	if (vid_tot_size < 10000)
		_stprintf(sz, TEXT("%i"), vid_tot_size);
	else if (vid_tot_size < 1024 * 1024)
		_stprintf(sz, TEXT("%.01fK"), (double)vid_tot_size / 1024.0);
	else
		_stprintf(sz, TEXT("%.01fM"), (double)vid_tot_size / (1024.0 * 1024.0));
	SetWindowText(GetDlgItem(m_Dlg, IDC_VTRKSIZE), sz);

	DispFileSize();

    return 1;
}

int CRTPDestProperties::OnAudio(int nBlock, int nBlockSize)
{
	TCHAR   sz[60];
	unsigned int aud_blocks;
	unsigned int aud_cur_size;
	unsigned int aud_tot_size;

	aud_tot_size = 0; // m_pVfwCD->SendDriverMessage(MJ2M_GETAUDTOTSIZE, 0, 0);
	aud_cur_size = 0; // m_pVfwCD->SendDriverMessage(MJ2M_GETAUDBLKSIZE, 0, 0);
	aud_blocks = 0; // m_pVfwCD->SendDriverMessage(MJ2M_GETAUDBLKCOUNT, 0, 0);

	if ((unsigned int)nBlockSize > m_max_aud_size)
		m_max_aud_size = nBlockSize;
	if ((unsigned int)nBlockSize < m_min_aud_size)
		m_min_aud_size = nBlockSize;

	if (nBlockSize < 10000)
		_stprintf(sz, TEXT("%i (%i) [%i, %i]"), nBlock, nBlockSize, m_min_aud_size, m_max_aud_size);
	else
		_stprintf(sz, TEXT("%i (%.01fK) [%i, %i]"), nBlock, (double)nBlockSize / 1024.0, m_min_aud_size, m_max_aud_size);
	SetWindowText(GetDlgItem(m_Dlg, IDC_AUDIOBLKS), sz);

	if (aud_cur_size < 10000)
		_stprintf(sz, TEXT("%i (%i)"), aud_blocks, aud_cur_size);
	else
		_stprintf(sz, TEXT("%i (%.01fK)"), aud_blocks, (double)aud_cur_size / 1024.0);
	SetWindowText(GetDlgItem(m_Dlg, IDC_BLOCKCOUNT), sz);

	if (aud_tot_size < 10000)
		_stprintf(sz, TEXT("%i"), aud_tot_size);
	else if (aud_tot_size < 1024 * 1024)
		_stprintf(sz, TEXT("%.01fK"), (double)aud_tot_size / 1024.0);
	else
		_stprintf(sz, TEXT("%.01fM"), (double)aud_tot_size / (1024.0 * 1024.0));
	SetWindowText(GetDlgItem(m_Dlg, IDC_ATRKSIZE), sz);

	DispFileSize();

    return 1;
}

int OnVideo(int nFrame, int nFrameSize, DWORD dwUser)
{

	CRTPDestProperties *pProp = (CRTPDestProperties *)dwUser;

	if (pProp)
		return pProp->OnVideo(nFrame, nFrameSize);
	else
		return 0;
}

int OnAudio(int nBlock, int nBlockSize, DWORD dwUser)
{

	CRTPDestProperties *pProp = (CRTPDestProperties *)dwUser;

	if (pProp)
		return pProp->OnAudio(nBlock, nBlockSize);
	else
		return 0;
}

//
// OnReceiveMessage
//
// Handles the messages for our property window
//
BOOL CRTPDestProperties::OnReceiveMessage(HWND hwnd,
                                          UINT uMsg,
                                          WPARAM wParam,
                                          LPARAM lParam)
{
    switch (uMsg)
    {
		case WM_CLOSE:
		//KillTimer(hwnd, m_timerID);
		break;

        case WM_COMMAND:
        {
            if (m_bIsInitialized)
            {
                m_bDirty = TRUE;
                if (m_pPageSite)
                {
                    m_pPageSite->OnStatusChange(PROPPAGESTATUS_DIRTY);
                }
            }
			if (m_pVfwCD) switch (GET_WM_COMMAND_ID(wParam, lParam))
			{
				case IDC_CONFIG:
					m_pVfwCD->ShowDialog(VfwCompressDialog_Config, hwnd);
					break;
				case IDABOUT:
					m_pVfwCD->ShowDialog(VfwCompressDialog_About, hwnd);
					break;
			}
            return (LRESULT) 1;
        }


    }
    return CBasePropertyPage::OnReceiveMessage(hwnd,uMsg,wParam,lParam);

} // OnReceiveMessage


//
// OnConnect
//
// Called when we connect to a Compress filter
//
HRESULT CRTPDestProperties::OnConnect(IUnknown *pUnknown)
{
	//ASSERT(m_pVCRTPDest == NULL);
    //ASSERT(m_pIPRTPDest == NULL);

    /*HRESULT hr = pUnknown->QueryInterface(IID_IAMVideoCompression, (void **) &m_pVCRTPDest);
    if (FAILED(hr)) {
        return E_NOINTERFACE;
    }
    ASSERT(m_pVCRTPDest);*/

    HRESULT hr = pUnknown->QueryInterface(IID_IAMVfwCompressDialogs, (void **) &m_pVfwCD);
    if (FAILED(hr)) {
		m_pVfwCD = NULL;
        return E_NOINTERFACE;
    }

	ASSERT(m_pVfwCD);
	//m_pVfwCD->SendDriverMessage(MJ2M_SETVIDEOCALLBACK, (WPARAM)::OnVideo, (LPARAM)this);
	//m_pVfwCD->SendDriverMessage(MJ2M_SETAUDIOCALLBACK, (WPARAM)::OnAudio, (LPARAM)this);

    /*hr = pUnknown->QueryInterface(IID_IIPRTPDest, (void **) &m_pIPRTPDest);
    if (FAILED(hr)) {
        return E_NOINTERFACE;
    }
    ASSERT(m_pIPRTPDest);*/

    // Get the initial image FX property
    //m_pIPRTPDest->get_IPRTPDest(&m_effect, &m_start, &m_length);
    m_bIsInitialized = FALSE ;
    return NOERROR;

} // OnConnect


//
// OnDisconnect
//
// Likewise called when we disconnect from a filter
//
HRESULT CRTPDestProperties::OnDisconnect()
{
    // Release of Interface after setting the appropriate old effect value

    /*if ((m_pIPRTPDest == NULL) || (m_pVCRTPDest == NULL)) {
        return E_UNEXPECTED;
    }*/
	
	//if (m_pVCRTPDest) m_pVCRTPDest->Release();
    //m_pIPRTPDest->Release();
	//m_pVCRTPDest = NULL;
    //m_pIPRTPDest = NULL;


	if (m_pVfwCD) 
	{
		//m_pVfwCD->SendDriverMessage(MJ2M_SETVIDEOCALLBACK, (WPARAM)NULL, (LPARAM)this);
		//m_pVfwCD->SendDriverMessage(MJ2M_SETAUDIOCALLBACK, (WPARAM)NULL, (LPARAM)this);
		m_pVfwCD->Release();
	}
	m_pVfwCD = NULL;

    return NOERROR;

} // OnDisconnect


BOOL CheckReggedV3()
{
	return TRUE;
}

int UpdateLogo(HWND hdlg)
{
	char szOEMLogo[256];
	BOOL bOEM = FALSE;
	HANDLE hBitmap, hOldBitmap;
	BOOL bRegged = CheckReggedV3();

	GetPrivateProfileString("Register", "OEMLogo", "", szOEMLogo, sizeof(szOEMLogo), "M3JP2K.INI");

	hBitmap = NULL;

	if (szOEMLogo[0])
		hBitmap = LoadImage(NULL, szOEMLogo, IMAGE_BITMAP,
							0, 0, LR_LOADMAP3DCOLORS | LR_DEFAULTSIZE | LR_LOADFROMFILE);

	if (hBitmap)
		bOEM = TRUE;
	else
	{
		if (bRegged)
			hBitmap = LoadImage((HINSTANCE__ *)GetWindowLong(hdlg, GWL_HINSTANCE), MAKEINTRESOURCE(IDB_BITMAP2), IMAGE_BITMAP,
								0, 0, LR_LOADMAP3DCOLORS | LR_DEFAULTSIZE);
		else
			hBitmap = LoadImage((HINSTANCE__ *)GetWindowLong(hdlg, GWL_HINSTANCE), MAKEINTRESOURCE(IDB_BITMAP1), IMAGE_BITMAP,
								0, 0, LR_LOADMAP3DCOLORS | LR_DEFAULTSIZE);
	}

	if (hBitmap)
	{
		hOldBitmap = (HANDLE)SendDlgItemMessage(hdlg, IDB_BITMAPMORGAN, STM_GETIMAGE, IMAGE_BITMAP, 0);
		SendDlgItemMessage(hdlg, IDB_BITMAPMORGAN, STM_SETIMAGE, IMAGE_BITMAP, (long)hBitmap);
		DeleteObject(hOldBitmap);
	}

	return bOEM;
}

//
// OnActivate
//
// We are being activated
//
HRESULT CRTPDestProperties::OnActivate()
{
	TCHAR   sz[255];

	//ASSERT(m_pVCRTPDest);
    //ASSERT(m_pIPRTPDest);

	/*m_pIPRTPDest->getFps(&fps);
	m_pIPRTPDest->getFrames(&frames);
	m_pIPRTPDest->getFormats(szIn, szOut, 255);*/

	//m_pVCRTPDest->get_Quality(&m_Quality);

	UpdateLogo(m_Dlg);

	VIDEOINFOHEADER *pvi = NULL; // (VIDEOINFOHEADER *)m_pVfwCD->SendDriverMessage(MJ2M_GETVIDEOFORMAT, 0, 0);
	WAVEFORMATEX *pwf = NULL; // (WAVEFORMATEX *)m_pVfwCD->SendDriverMessage(MJ2M_GETAUDIOFORMAT, 0, 0);

	if (pvi)
	{
		TCHAR FourCC[5];
		if (pvi->bmiHeader.biCompression == BI_RGB)
			if (pvi->bmiHeader.biBitCount == 16)
				lstrcpy(FourCC, "RGB555");
			else
				lstrcpy(FourCC, "RGB");
		else if (pvi->bmiHeader.biCompression == BI_BITFIELDS)
			if (pvi->bmiHeader.biBitCount == 16)
				lstrcpy(FourCC, "RGB565");
			else
				lstrcpy(FourCC, "RGB");
		else
		{
				 FourCC[0] = ((char *)&pvi->bmiHeader.biCompression)[0];
				 FourCC[1] = ((char *)&pvi->bmiHeader.biCompression)[1];
				 FourCC[2] = ((char *)&pvi->bmiHeader.biCompression)[2];
				 FourCC[3] = ((char *)&pvi->bmiHeader.biCompression)[3];
				 FourCC[4] = 0;
		}

		_stprintf((PCHAR)sz, "%s %ldx%ld, %ld-bits",
				 FourCC,
				 pvi->bmiHeader.biWidth,
				 pvi->bmiHeader.biHeight,
				 pvi->bmiHeader.biBitCount
				 );
	}
    SetWindowText(GetDlgItem(m_Dlg, IDC_IN), sz);

	if (pwf)
	{
		if (pwf->nChannels == 1)
			_stprintf(sz, TEXT("%i-Hz, %i-bits mono, AvgBytesPerSec:%i, BlockAlign:%i"), pwf->nSamplesPerSec, pwf->wBitsPerSample, pwf->nAvgBytesPerSec, pwf->nBlockAlign);
		else
			_stprintf(sz, TEXT("%i-Hz, %i-bits stereo, AvgBytesPerSec:%i, BlockAlign:%i"), pwf->nSamplesPerSec, pwf->wBitsPerSample, pwf->nAvgBytesPerSec, pwf->nBlockAlign);

		//CheckRadioButton(m_Dlg, IDC_1SECAUDIO, IDC_1FRAMEAUDIO, (unsigned int)m_pVfwCD->SendDriverMessage(MJ2M_GETAUDBLKSIZE, 0, 0) == pwf->nAvgBytesPerSec ? IDC_1SECAUDIO : IDC_1FRAMEAUDIO);
		CheckRadioButton(m_Dlg, IDC_1SECAUDIO, IDC_1FRAMEAUDIO, TRUE ? IDC_1SECAUDIO : IDC_1FRAMEAUDIO);

	}
	SetWindowText(GetDlgItem(m_Dlg, IDC_OUT), sz);

    // CheckRadioButton(m_Dlg, IDC_EMBOSS, IDC_NONE, m_effect);
    m_bIsInitialized = TRUE;

	//m_timerID = SetTimer(m_Dlg, 1, 100, NULL);

    return NOERROR;

} // OnActivate


//
// OnDeactivate
//
// We are being deactivated
//
HRESULT CRTPDestProperties::OnDeactivate(void)
{
	//ASSERT(m_pVCRTPDest);
    //ASSERT(m_pIPRTPDest);

    m_bIsInitialized = FALSE;
    GetControlValues();
    return NOERROR;

} // OnDeactivate


//
// OnApplyChanges
//
// Apply any changes so far made
//
HRESULT CRTPDestProperties::OnApplyChanges()
{
    GetControlValues();
    //m_pIPRTPDest->put_IPRTPDest(m_effect, m_start, m_length);
	//m_pVCRTPDest->put_Quality(m_Quality);

    return NOERROR;
} // OnApplyChanges


void CRTPDestProperties::GetControlValues()
{
	WAVEFORMATEX *pwf = NULL; // (WAVEFORMATEX *)m_pVfwCD->SendDriverMessage(MJ2M_GETAUDIOFORMAT, 0, 0);
	if (pwf)
	{
		/*if (IsDlgButtonChecked(m_Dlg, IDC_1SECAUDIO))
			m_pVfwCD->SendDriverMessage(MJ2M_SETAUDBLKSIZE, (WPARAM)pwf->nAvgBytesPerSec, 0);
		else
			m_pVfwCD->SendDriverMessage(MJ2M_SETAUDBLKSIZE, 0, 0);*/
	}
}
