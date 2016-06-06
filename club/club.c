// Meridian 59, Copyright 1994-2012 Andrew Kirmse and Chris Kirmse.
// All rights reserved.
//
// This software is distributed under a license that is described in
// the LICENSE file that accompanies it.
//
// Meridian is a registered trademark.
/*
 * club.c
 *

 Client update program, using FTP!!!! Yeah!!!!

 */

#include "club.h"
#include <vector>

/* time to wait at program start */
#define INIT_TIME 3000

HINSTANCE hInst;
HWND hwndMain;

Bool success;
Bool get_patchinfo;
static Bool retry = True;   // True when an error occurs and user asks to retry

std::string restart_filename;

int transfer_progress = 0;
int transfer_file_size = 0;
char string[256];
char format[256];
char format_filename[256];
char filename_string[256];
char scanning_string[256];

std::string transfer_machine;
#if VANILLA_UPDATER
std::string transfer_filename;
std::string transfer_local_filename;
#else
std::string transfer_path;
std::string patchinfo_path;
#endif
std::string patchinfo_filename;
std::string dest_path;

/* local function prototypes */
Bool ParseCommandLine(const char *args);
void RestartFilename();
void StartupError();
void RestartClient();
void Interface(int how_show);
long WINAPI InterfaceWindowProc(HWND hwnd,UINT message,UINT wParam,LONG lParam);
void OnTimer(HWND hwnd,int id);
BOOL CALLBACK ErrorDialogProc(HWND hDlg, UINT message, UINT wParam, LONG lParam);

/************************************************************************/
int WINAPI WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrev_instance,
                   _In_ char *command_line, _In_ int how_show)
{
   hInst = hInstance;

   InitCommonControls();

   get_patchinfo = False;
   success = False; /* whether the copy compeletely succeeded */

   if (ParseCommandLine(command_line))
      Interface(how_show);

   if (success)
      RestartClient();

   if (success == False)
      return 1;

   return 0;
}
/************************************************************************/
std::vector<std::string> split(const std::string &s, char delim) {
   std::vector<std::string> args;
   bool in_quotes = false;
   std::string::size_type start = 0;
   for (std::string::size_type i = 0; i < s.size(); ++i)
   {
      // Hit end of argument
      if (s[i] == delim && !in_quotes)
      {
         // Leave out delimiter
         std::string arg = s.substr(start, i - start);

         args.push_back(arg);
         start = i + 1;
      }

      // Hit quotes
      if (s[i] == '"')
      {
         in_quotes = !in_quotes;
      }
   }

   // Add the last argument
   if (start < s.size())
   {
      args.push_back(s.substr(start, s.size() - start + 1));
   }

   // The client puts quotes around some of our arguments, which we
   // need to undo here
   for (std::vector<std::string>::size_type i = 0; i < args.size(); ++i)
   {
      if (args[i][0] == '"')
         args[i].erase(0, 1);
      std::string::size_type size = args[i].size();
      if (size > 0 && args[i][size - 1] == '"')
         args[i].resize(size - 1);
   }
   
   return args;
}
/************************************************************************/
Bool ParseCommandLine(const char *args)
{
#if !VANILLA_UPDATER
   char buf[MAX_CMDLINE + 1];
   if (strcmp(args, "") == 0)
   {
      FILE *fp;
      fp = fopen("dlinfo.txt", "rb");
      if (!fp)
      {
         StartupError();
         return False;
      }
      fgets(buf, MAX_CMDLINE, fp);
      fclose(fp);
      args = buf;
   }
#endif
   std::string argstring(args);
   std::vector<std::string> arguments = split(argstring, ' ');

   if (arguments.size() == CLUB_NUM_ARGUMENTS)
   {
      if (arguments[1] != "UPDATE")
      {
         StartupError();
         return False;
      }

      restart_filename = arguments[0];
      transfer_machine = arguments[2]; //patchhost
      dest_path = arguments[5];

#if VANILLA_UPDATER
      transfer_filename = arguments[3];
      transfer_local_filename = arguments[4];
#else
      transfer_path = arguments[3];    // patchpath
      patchinfo_path = arguments[4];   // patchinfo_path
   }
   else if (arguments.size() == CLUB_NEW_NUM_ARGUMENTS)
   {
      if (arguments[1] != "UPDATE")
      {
         StartupError();
         return False;
      }
      // Set get_patchinfo to True, so we know to download patchinfo file.
      get_patchinfo = True;

      restart_filename = arguments[0];
      transfer_machine = arguments[2]; //patchhost
      transfer_path = arguments[3];    // patchpath
      patchinfo_path = arguments[4];   // patchinfo_path
      patchinfo_filename = arguments[5];
      dest_path = arguments[6];
#endif
   }
   else
   {
      StartupError();

      return False;
   }

   return True;
}
/************************************************************************/
void StartupError()
{
   MessageBox(NULL,GetString(hInst, IDS_BADSTARTUP),GetString(hInst, IDS_APPNAME),
	      MB_ICONSTOP | MB_OK);
}
/************************************************************************/
void RestartClient()
{
   char s[100+MAX_PATH+FILENAME_MAX];
   STARTUPINFO si;
   PROCESS_INFORMATION pi;

   memset(&si, 0, sizeof(si));
   si.cb = sizeof(si);
   GetStartupInfo(&si); /* shouldn't need to do this.  very weird */
   memset(&pi, 0, sizeof(pi));

   // Get the working directory we want to restart in.
   // Could be different from where club was launched.
   size_t found = restart_filename.find_last_of("/\\");
   std::string dirPath;
   dirPath.assign(restart_filename.substr(0, found));

   if (!CreateProcess(restart_filename.c_str(), "", NULL, NULL, FALSE, 0, NULL, dirPath.c_str(), &si, &pi))
   {
      sprintf(s,GetString(hInst, IDS_CANTRESTART),GetLastError(),restart_filename.c_str());
      MessageBox(NULL,s,GetString(hInst, IDS_APPNAME),MB_ICONSTOP);
   }
}
/************************************************************************/
void Interface(int how_show)
{
   WNDCLASS main_class;
   MSG msg;

   main_class.style = CS_HREDRAW | CS_VREDRAW;
   main_class.lpfnWndProc = InterfaceWindowProc;
   main_class.cbClsExtra = 0;
   main_class.cbWndExtra = DLGWINDOWEXTRA;
   main_class.hInstance = hInst;
   main_class.hIcon = LoadIcon(hInst,MAKEINTRESOURCE(IDI_ICON));
   main_class.hCursor = NULL;
   main_class.hbrBackground = GetSysColorBrush(COLOR_3DFACE);
   main_class.lpszMenuName = NULL;
   main_class.lpszClassName = "club (Andrew & Christopher Kirmse)";

   RegisterClass(&main_class);

   hwndMain = CreateDialog(hInst,MAKEINTRESOURCE(IDD_MAIN_DLG),0,
                           (DLGPROC) InterfaceWindowProc);

   ShowWindow(hwndMain,how_show);

   Status(GetString(hInst, IDS_STARTING));
   
   while (GetMessage(&msg,NULL,0,0))
   {
      TranslateMessage(&msg);
      DispatchMessage(&msg);
   }
}
/************************************************************************/
long WINAPI InterfaceWindowProc(HWND hwnd,UINT message,UINT wParam,LONG lParam)
{
   HWND hCtrl;

   switch (message)
   {
   case WM_INITDIALOG :
      SetTimer(hwnd,TIMER_START_TRANSFER,INIT_TIME,NULL);
      CenterWindow(hwnd,NULL);
      SetFocus(GetDlgItem(hwnd, IDCANCEL));

      GetDlgItemText(hwnd, IDC_BYTES1, format, sizeof(format));
      sprintf(string, format, transfer_progress, transfer_file_size);
      SetDlgItemText(hwnd, IDC_BYTES1, string);

      GetDlgItemText(hwnd, IDS_RETRIEVING, format_filename, sizeof(format_filename));
      sprintf(scanning_string, GetString(hInst, IDS_SCANNING));
      SetDlgItemText(hwnd, IDS_RETRIEVING, scanning_string);
      

      hCtrl = GetDlgItem(hwnd, IDC_ANIMATE1);
      Animate_Open(hCtrl, MAKEINTRESOURCE(IDA_DOWNLOAD));
      break;

   case WM_NCDESTROY:
      PostQuitMessage(0);
      return 0;

   HANDLE_MSG(hwnd, WM_TIMER, OnTimer);

   case CM_RETRYABORT:
      if (retry)
      {
         Status(GetString(hInst, IDS_CONNECTING), transfer_machine.c_str());
        _beginthread((void(*)(void*))TransferStart, 0, NULL);
      }
      else
      {
        PostMessage(hwndMain, WM_CLOSE, 0, 0);
      }
      break;
      
   case CM_DEARCHIVE :
#if VANILLA_UPDATER
      Dearchive(dest_path.c_str(), transfer_local_filename.c_str());
#endif
      break;

   case CM_FILESIZE:
      // Set max value
     transfer_file_size = lParam;
      sprintf(string, format, transfer_progress, transfer_file_size);
      SetDlgItemText(hwndMain, IDC_BYTES1, string);
      SendDlgItemMessage(hwnd, IDC_PROGRESS, PBM_SETRANGE, 0,
                         MAKELPARAM(0, transfer_file_size / 100));
      break;

   case CM_PROGRESS:
      // Set current value
      transfer_progress = lParam;
      sprintf(string, format, transfer_progress, transfer_file_size);
      SetDlgItemText(hwndMain, IDC_BYTES1, string);
      SendDlgItemMessage(hwnd, IDC_PROGRESS, PBM_SETPOS, transfer_progress / 100, 0);
      break;

   case CM_FILENAME:
      sprintf(filename_string, format_filename, (LPCSTR)lParam);
      SetDlgItemText(hwndMain, IDS_RETRIEVING, filename_string);
      break;

   case CM_SCANNING:
      SetDlgItemText(hwndMain, IDS_RETRIEVING, scanning_string);
      break;

   case WM_COMMAND:
      switch(GET_WM_COMMAND_ID(wParam, lParam))
      {
      case IDCANCEL:
	 PostMessage(hwnd, WM_CLOSE, 0, 0);
	 break;
      }
      break;

   }
   return DefWindowProc(hwnd,message,wParam,lParam);    
}
/************************************************************************/
void OnTimer(HWND hwnd,int id)
{
   switch (id)
   {
   case TIMER_START_TRANSFER:
      KillTimer(hwndMain, TIMER_START_TRANSFER);
      PostMessage(hwndMain, CM_RETRYABORT, 0, 0);
      break;
   }
}
/************************************************************************/
/*
 * Status:  Display given status message.
 */
void Status(char *fmt, ...)
{
   char s[200];
   va_list marker;
    
   va_start(marker,fmt);
   vsprintf(s,fmt,marker);
   va_end(marker);

   SetDlgItemText(hwndMain, IDC_STATUS, s);
}
/************************************************************************/
/*
 * Error:  Display given error message, and ask for retry.
 */
void Error(char *fmt, ...)
{
   int retval;
   char s[500];
   va_list marker;
    
   va_start(marker,fmt);
   vsprintf(s,fmt,marker);
   va_end(marker);

   retval = DialogBoxParam(hInst, MAKEINTRESOURCE(IDD_ERROR), hwndMain, ErrorDialogProc, 
			   (LPARAM) s);

   if (retval == IDOK)
      retry = True;
   else retry = False;
}
/*****************************************************************************/
/*
 * ErrorDialogProc:  Dialog procedure for displaying error and asking for retry.
 */
BOOL CALLBACK ErrorDialogProc(HWND hDlg, UINT message, UINT wParam, LONG lParam)
{
   switch (message)
   {
   case WM_INITDIALOG:
      SetDlgItemText(hDlg, IDC_MESSAGE, (char *) lParam);
      CenterWindow(hDlg, GetParent(hDlg));
      return FALSE;

   case WM_COMMAND:
      switch(GET_WM_COMMAND_ID(wParam, lParam))
      {
      case IDOK:
	 EndDialog(hDlg, IDOK);
	 return TRUE;
      case IDCANCEL:
	 EndDialog(hDlg, IDCANCEL);
	 return TRUE;
      }
   }
   return FALSE;
}
/*****************************************************************************/
char *GetLastErrorStr()
{ 
   char *error_str;
   
   error_str = "No error string"; /* in case the call  fails */

   // This works only for wininet.dll errors
   FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_HMODULE,
                 GetModuleHandle("wininet.dll"), GetLastError(), 0,
                 (LPTSTR) &error_str, 0, NULL);
   return error_str;
}
