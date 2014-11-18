/**************************************************************************
 *
 *  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
 *  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
 *  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
 *  PURPOSE.
 *
 *  Copyright (c) 1993, 1994  Microsoft Corporation.  All Rights Reserved.
 *
 ***************************************************************************/

#ifndef _WAVEMIX_H
#define _WAVEMIX_H

#ifdef __cplusplus
extern "C"
{
#endif

/* set to byte packing so Win16 and Win32 structures will be the same */
#pragma pack(1)

#ifndef NOWMIXSTR
typedef LPVOID LPMIXWAVE;
#endif

/* flag values for play params */
#define WMIX_QUEUEWAVE          0x00
#define WMIX_CLEARQUEUE         0x01
#define WMIX_USELRUCHANNEL      0x02
#define WMIX_HIPRIORITY         0x04
#define WMIX_HIGHPRIORITY       WMIX_HIPRIORITY /* alternate spelling */
#define WMIX_WAIT               0x08


typedef struct
{
  WORD wSize;
  HANDLE hMixSession;
  int iChannel;
  LPMIXWAVE lpMixWave;
  HWND hWndNotify;
  DWORD dwFlags;
  WORD wLoops;  /* 0xFFFF means loop forever */
}
MIXPLAYPARAMS, * PMIXPLAYPARAM, FAR * LPMIXPLAYPARAMS;

typedef struct
{
  WORD wSize;
  BYTE bVersionMajor;
  BYTE bVersionMinor;
  char szDate[12]; /* Mmm dd yyyy */
  DWORD dwFormats; /* see waveOutGetDevCaps (wavemix requires synchronous device) */
}
WAVEMIXINFO, *PWAVEMIXINFO, FAR * LPWAVEMIXINFO;

#define WMIX_CONFIG_CHANNELS     0x1
#define WMIX_CONFIG_SAMPLINGRATE 0x2
#define WMIX_CONFIG_RESOLUTION   0x4
typedef struct
{
  WORD wSize;
  DWORD dwFlags;
  WORD wChannels;       /* 1 = MONO, 2 = STEREO */
  WORD wSamplingRate;   /* 11,22,44  (11=11025, 22=22050, 44=44100 Hz) */
}
MIXCONFIG, *PMIXCONFIG, FAR * LPMIXCONFIG;

WORD    WINAPI WaveMixGetInfo(LPWAVEMIXINFO lpWaveMixInfo);

/* return same errors as waveOutOpen, waveOutWrite, and waveOutClose */
HANDLE  WINAPI WaveMixInit(void); /* returns hMixSession - you should keep it and use for subsequent API calls */
HANDLE  WINAPI WaveMixConfigureInit(LPMIXCONFIG lpConfig);
UINT    WINAPI WaveMixActivate(HANDLE hMixSession, BOOL fActivate);

#define WMIX_FILE     0x00000001L
#define WMIX_RESOURCE 0x00000002L
#define WMIX_MEMORY   0x00000004L
#define WMIX_RAWDATA  0x80000000Lu
LPMIXWAVE WINAPI WaveMixOpenWave(HANDLE hMixSession, LPSTR szWaveFilename, HINSTANCE hInst, DWORD dwFlags);

#define WMIX_OPENSINGLE 0 /* open the single channel specified by iChannel */
#define WMIX_OPENALL    1 /* opens all the channels, iChannel ignored */
#define WMIX_OPENCOUNT  2 /* open iChannel Channels (eg. if iChannel = 4 will create channels 0-3) */
UINT    WINAPI WaveMixOpenChannel(HANDLE hMixSession, int iChannel, DWORD dwFlags);

UINT    WINAPI WaveMixPlay(LPMIXPLAYPARAMS lpMixPlayParams);

#define WMIX_ALL      0x0001 /* stops sound on all the channels, iChannel ignored */
#define WMIX_NOREMIX  0x0002 /* prevents the currently submited blocks from being remixed to exclude new channel */
UINT    WINAPI WaveMixFlushChannel(HANDLE hMixSession, int iChannel, DWORD dwFlags);
UINT    WINAPI WaveMixCloseChannel(HANDLE hMixSession, int iChannel, DWORD dwFlags);

UINT    WINAPI WaveMixFreeWave(HANDLE HMixSession, LPMIXWAVE lpMixWave);
UINT    WINAPI WaveMixCloseSession(HANDLE hMixSession);
void    WINAPI WaveMixPump(void);

int     WINAPI WaveMixGetProfileInt(LPSTR lpszKey, int iDefValue);
int     WINAPI WaveMixSetProfileInt(LPSTR lpszKey, int iValue);
UINT    WINAPI WaveMixConvert(HANDLE hMixSession, LPMIXWAVE lpMixWave, LPWAVEFORMAT lpFmt);

#define WMIX_SAMPLES    0x0000
#define WMIX_TIME       0x0001
DWORD   WINAPI WaveMixWaveLength(HANDLE hMixSession, LPMIXWAVE lpMixWave, DWORD dwFlags);

UINT    WINAPI WaveMixVolume(HANDLE hMixSession, LPMIXWAVE lpMixWave, int iLeft, int iRight);

/* return to default packing */
#pragma pack()

/* these constants are copied from mmsystem.h in case user
** doesn't have mmsystem.h
*/
#if !defined(_INC_MMSYSTEM) && !defined(__MMSYSTEM_H)

#define MMSYSERR_BASE          0
#define WAVERR_BASE            32

#define MMSYSERR_NOERROR      0                    // no error
#define MMSYSERR_ERROR        (MMSYSERR_BASE + 1)  // unspecified error
#define MMSYSERR_BADDEVICEID  (MMSYSERR_BASE + 2)  // device ID out of range
#define MMSYSERR_NOTENABLED   (MMSYSERR_BASE + 3)  // driver failed enable
#define MMSYSERR_ALLOCATED    (MMSYSERR_BASE + 4)  // device already allocated
#define MMSYSERR_INVALHANDLE  (MMSYSERR_BASE + 5)  // device handle is invalid
#define MMSYSERR_NODRIVER     (MMSYSERR_BASE + 6)  // no device driver present
#define MMSYSERR_NOMEM        (MMSYSERR_BASE + 7)  // memory allocation error
#define MMSYSERR_NOTSUPPORTED (MMSYSERR_BASE + 8)  // function isn't supported
#define MMSYSERR_BADERRNUM    (MMSYSERR_BASE + 9)  // error value out of range
#define MMSYSERR_INVALFLAG    (MMSYSERR_BASE + 10) // invalid flag passed
#define MMSYSERR_INVALPARAM   (MMSYSERR_BASE + 11) // invalid parameter passed
#define MMSYSERR_HANDLEBUSY   (MMSYSERR_BASE + 12) // handle being used
                                                   // simultaneously on another
                                                   // thread (eg callback)
#define MMSYSERR_INVALIDALIAS (MMSYSERR_BASE + 13) // "Specified alias not found in WIN.INI
#define MMSYSERR_LASTERROR    (MMSYSERR_BASE + 13) // last error in range

#define MM_WOM_OPEN         0x3BB                  // waveform output
#define MM_WOM_CLOSE        0x3BC
#define MM_WOM_DONE         0x3BD

#endif // _NC_MMSYSTEM

#ifdef __cplusplus
}
#endif

// Meridian 59 stuff copied from old wavemix.hInst

// Constants for volume added 8/95 ARK
#define MAX_VOLUME 64
#define LOG_MAX_VOLUME 6

/* Windowsx.h style message handlers (copied from mixtest 6/95 ARK) */

/* void Cls_OnMM_WOM_DONE(HWND hwnd, int iChannel, LPMIXWAVE lpMixWave); */
#define HANDLE_MM_WOM_DONE(hwnd, wParam, lParam, fn) \
((fn)((hwnd), (int)(wParam), (LPMIXWAVE)(lParam)), 0L)
#define FORWARD_MM_WOM_DONE(hwnd, bCommand, lpNCB, fn) \
    (void)(fn)((hwnd), WM_MM_WOM_DONE, (WPARAM)(iChannel),(LPARAM) lpMixWave)

#endif // _WAVEMIX_H
