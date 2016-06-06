// Meridian 59, Copyright 1994-2012 Andrew Kirmse and Chris Kirmse.
// All rights reserved.
//
// This software is distributed under a license that is described in
// the LICENSE file that accompanies it.
//
// Meridian is a registered trademark.
/*
 * transfer.c:  Do actual file transfer for club.
 */

#include "club.h"
#include <clientpatch.h>
#include <algorithm>

// Handles to Internet connection, session, and file to transfer
static HINTERNET hConnection, hSession;

#if !VANILLA_UPDATER
const char *mime_types[8] = { "application/octet-stream", "application/x-msdownload",
                              "video/ogg", "image/png", "text/xml", "video/avi",
                              "text/plain", NULL };
#endif

static Bool DirectoryExists(LPCTSTR szPath);
static void CreateDirectoryTree(std::string dirPath);
static void inline TransferCleanup();
static Bool DownloadOneFile(std::string basepath, std::string req_file,
                            std::string filename, int file_size);

#define BUFSIZE 4096
static char buf[BUFSIZE];

static int outfile;                   // Handle to output file
static DWORD size;                    // Size of block we're reading
static int bytes_read;                // Total # of bytes we've read

/************************************************************************/
#define debug(x) (dprintf x)
void _cdecl dprintf(char *fmt, ...)
{
   char s[1024];
   va_list marker;
   FILE *debug_file;

   debug_file = fopen("club.txt", "a+");

   if (!debug_file)
      return;

   va_start(marker,fmt);
   vsprintf(s,fmt,marker);
   va_end(marker);

   fputs(s, debug_file);
   fclose(debug_file);
}
/************************************************************************/
static Bool DirectoryExists(LPCTSTR szPath)
{
   DWORD dwAttrib = GetFileAttributes(szPath);

   return (dwAttrib != INVALID_FILE_ATTRIBUTES && 
      (dwAttrib & FILE_ATTRIBUTE_DIRECTORY));
}
/************************************************************************/
static void CreateDirectoryTree(std::string dirPath)
{
   std::string createPath;

   // Start from 3rd char in string - skips the .\ at beginning.
   size_t found = dirPath.find_first_of("/\\", 2);

   while (found != std::string::npos)
   {
      // Assign from char 0, i.e. .\dirname
      createPath.assign(dirPath.substr(0, found));
      CreateDirectory(createPath.c_str(), NULL);
      // Look from found + 1
      found = dirPath.find_first_of("/\\", found + 1);
   }
}
/************************************************************************/
#if VANILLA_UPDATER
Bool TransferStart(void)
{
   HINTERNET hFile;
   Bool done;
   const char *mime_types[2] = { "application/x-zip-compressed", NULL };
   char file_size_buf[20];
   DWORD file_size_buf_len = sizeof(file_size_buf);
   DWORD index = 0;
   int file_size;

   hConnection = InternetOpen(GetString(hInst, IDS_APPNAME), INTERNET_OPEN_TYPE_PRECONFIG, 
                              NULL, NULL, INTERNET_FLAG_RELOAD);
   
   if (hConnection == NULL)
   {
     Error(GetString(hInst, IDS_CANTINIT), GetLastError(), GetLastErrorStr());
     PostMessage(hwndMain, CM_RETRYABORT, 0, 0);
     return False;
   }

   hSession = InternetConnect(hConnection, transfer_machine.c_str(), 
                              INTERNET_INVALID_PORT_NUMBER, 
                              NULL, NULL, INTERNET_SERVICE_HTTP, 
                              0, 0);

   if (hSession == NULL)
   {
     if (GetLastError() != ERROR_IO_PENDING)
     {
        Error(GetString(hInst, IDS_NOCONNECTION), transfer_machine.c_str(),
              GetLastError(), GetLastErrorStr());
        InternetCloseHandle(hConnection);
        PostMessage(hwndMain, CM_RETRYABORT, 0, 0);
        return False;
      }
   }

   hFile = HttpOpenRequest(hSession, NULL, transfer_filename.c_str(), NULL, NULL,
                           mime_types, INTERNET_FLAG_NO_UI, 0);
   if (hFile == NULL)
   {
      if (GetLastError() != ERROR_IO_PENDING)
      {
         Error(GetString(hInst, IDS_CANTFINDFILE), transfer_filename.c_str(), transfer_machine.c_str(), 
               GetLastError(), GetLastErrorStr());
         InternetCloseHandle(hSession);
         InternetCloseHandle(hConnection);
         PostMessage(hwndMain, CM_RETRYABORT, 0, 0);
         return False;
      }
   }
   
   if (!HttpSendRequest(hFile, NULL, 0, NULL, 0)) {
      Error(GetString(hInst, IDS_CANTSENDREQUEST), transfer_filename.c_str(), transfer_machine.c_str(), 
            GetLastError(), GetLastErrorStr());
      InternetCloseHandle(hFile);
      InternetCloseHandle(hSession);
      InternetCloseHandle(hConnection);
      PostMessage(hwndMain, CM_RETRYABORT, 0, 0);
      return False;
   }
   
   // Get file size
   if (!HttpQueryInfo(hFile, HTTP_QUERY_CONTENT_LENGTH,
                      file_size_buf, &file_size_buf_len, &index)) {
      Error(GetString(hInst, IDS_CANTGETFILESIZE), transfer_filename.c_str(), transfer_machine.c_str(), 
            GetLastError(), GetLastErrorStr());
      InternetCloseHandle(hFile);
      InternetCloseHandle(hSession);
      InternetCloseHandle(hConnection);
      PostMessage(hwndMain, CM_RETRYABORT, 0, 0);
      return False;
   }
   
   sscanf(file_size_buf, "%d", &file_size);
   PostMessage(hwndMain, CM_FILESIZE, 0, file_size);
   
   outfile = open(transfer_local_filename.c_str(), O_BINARY | O_RDWR | O_CREAT | O_TRUNC,
                  S_IWRITE | S_IREAD);
   if (outfile < 0)
   {
      Error(GetString(hInst, IDS_CANTWRITELOCALFILE), transfer_local_filename.c_str(), 
           GetLastError(), GetLastErrorStr());
     InternetCloseHandle(hFile);
     InternetCloseHandle(hSession);
     InternetCloseHandle(hConnection);
     PostMessage(hwndMain, CM_RETRYABORT, 0, 0);
     return False;
   }

   // Read first block
   done = False;
   bytes_read = 0;
   while (!done)
   {
     if (!InternetReadFile(hFile, buf, BUFSIZE, &size))
     {
       if (GetLastError() != ERROR_IO_PENDING)
       {
          Error(GetString(hInst, IDS_CANTREADFTPFILE), transfer_filename.c_str(), 
                GetLastError(), GetLastErrorStr());
          PostMessage(hwndMain, CM_RETRYABORT, 0, 0);
       }
     }
     
     Status(GetString(hInst, IDS_READBLOCK));
     
     if (size > 0)
     {
       if (write(outfile, buf, size) != size)
       {
          Error(GetString(hInst, IDS_CANTWRITELOCALFILE), transfer_local_filename.c_str(), 
                GetLastError(), GetLastErrorStr());
          close(outfile);
          InternetCloseHandle(hFile);
          InternetCloseHandle(hSession);
          InternetCloseHandle(hConnection);
          PostMessage(hwndMain, CM_RETRYABORT, 0, 0);
          return False;
       }
     }
     
     // Update graph position
     bytes_read += size;
     PostMessage(hwndMain, CM_PROGRESS, 0, bytes_read);
     
     // See if done with file
     if (size == 0)
     {
       close(outfile);
       
       InternetCloseHandle(hFile);
       InternetCloseHandle(hSession);
       InternetCloseHandle(hConnection);   
       done = True;
     }
   }
   
   PostMessage(hwndMain, CM_DEARCHIVE, 0, 0);
   return True;
}
#else
/************************************************************************/
Bool TransferStart(void)
{
   std::string req_file, filename, basepath;
   json_error_t JsonError;
   json_t *PatchInfo;
   json_t *it;
   size_t array_index = 0;
   int file_size;

   hConnection = InternetOpen(GetString(hInst, IDS_APPNAME), INTERNET_OPEN_TYPE_PRECONFIG,
      NULL, NULL, INTERNET_FLAG_RELOAD);

   if (!hConnection)
   {
      Error(GetString(hInst, IDS_CANTINIT), GetLastError(), GetLastErrorStr());
      PostMessage(hwndMain, CM_RETRYABORT, 0, 0);
      return False;
   }

   hSession = InternetConnect(hConnection, transfer_machine.c_str(),
      INTERNET_INVALID_PORT_NUMBER, NULL, NULL, INTERNET_SERVICE_HTTP, 0, 0);

   if (!hSession)
   {
      if (GetLastError() != ERROR_IO_PENDING)
      {
         Error(GetString(hInst, IDS_NOCONNECTION), transfer_machine.c_str(),
            GetLastError(), GetLastErrorStr());
         InternetCloseHandle(hConnection);
         PostMessage(hwndMain, CM_RETRYABORT, 0, 0);
         return False;
      }
   }

   // If get_patchinfo is True, the first file we need is patchinfo (patch cache).
   if (get_patchinfo)
   {
      // This is the path (including filename) that we request from the host.
      req_file.assign(patchinfo_path);
      req_file.append(patchinfo_filename);
      // Have to estimate file size.
      if (!DownloadOneFile("\\", req_file, patchinfo_filename, 600000))
      {
         // DownloadOneFile handles error messages/cleanup.
         return False;
      }

      // If we successfully downloaded the patch file, we can read it in to
      // a JSON array and compare our club.exe. File downloads to download_dir.
      PatchInfo = json_load_file(patchinfo_filename.c_str(), 0, &JsonError);
      if (!PatchInfo)
      {
         Error(GetString(hInst, IDS_JSONERROR), patchinfo_filename.c_str(), JsonError.text);
         TransferCleanup();
         PostMessage(hwndMain, CM_RETRYABORT, 0, 0);
         return False;
      }
   }
   else
   {
      // Loading pre-downloaded patchcache - patchinfo_path is the local path
      // of the file.
      PatchInfo = json_load_file(patchinfo_path.c_str(), 0, &JsonError);
      if (!PatchInfo)
      {
         Error(GetString(hInst, IDS_JSONERROR), patchinfo_path.c_str(), JsonError.text);
         TransferCleanup();
         PostMessage(hwndMain, CM_RETRYABORT, 0, 0);
         return False;
      }
   }
   //debug(("machine name %s, path name %s\n", transfer_machine.c_str(), transfer_path.c_str()));

   // Successfully loaded patch file, check the update program (club.exe).
   json_array_foreach(PatchInfo, array_index, it)
   {
      // Don't download if patchinfo has file download set to false.
      if (!(json_boolean_value(json_object_get(it, "Download"))))
         continue;

      // Skip this file if it matches the local one.
      if (CompareCacheToLocalFile(&it))
         continue;

      filename.assign(json_string_value(json_object_get(it, "Filename")));

      // Also skip the updater itself - this is checked by the client.
      if (filename == "club.exe")
         continue;

      basepath.assign(json_string_value(json_object_get(it, "Basepath")));

      // This is the path (including filename) that we request from the host.
      req_file.assign(transfer_path);
      req_file.append(basepath);
      req_file.append(filename);
      std::replace(req_file.begin(), req_file.end(), '\\', '/');

      // Use file size from patchinfo.
      file_size = (int)json_integer_value(json_object_get(it, "Length"));

      if (!DownloadOneFile(basepath, req_file, filename, file_size))
      {
         // DownloadOneFile handles error messages/cleanup.
         return False;
      }

      // Update main window to show scanning state.
      SendMessage(hwndMain, CM_SCANNING, 0, 0);
   }

   TransferCleanup();
   Status(GetString(hInst, IDS_RESTARTING));
   success = true;
   PostMessage(hwndMain, WM_CLOSE, 0, 0);

   return True;
}

static Bool DownloadOneFile(std::string basepath, std::string req_file,
                            std::string filename, int file_size)
{
   HINTERNET hFile;
   std::string local_file_path;
   Bool done;

   // Update main window with filename.
   SendMessage(hwndMain, CM_FILENAME, 0, (LPARAM)filename.c_str());
   // Update main window with file size.
   SendMessage(hwndMain, CM_FILESIZE, 0, file_size);

   //debug(("downloading file %s\n", req_file.c_str()));

   // This is the path the file should be saved to locally.
   local_file_path.assign(".");
   local_file_path.append(basepath);

   if (!DirectoryExists(local_file_path.c_str()))
      CreateDirectoryTree(local_file_path);

   local_file_path.append(filename);

   // Request file.
   if (!hSession)
   {
      TransferCleanup();
      PostMessage(hwndMain, CM_RETRYABORT, 0, 0);
      return False;
   }
   hFile = HttpOpenRequest(hSession, NULL, req_file.c_str(), NULL, NULL,
      mime_types, INTERNET_FLAG_NO_UI, 0);
   if (!hFile)
   {
      if (GetLastError() != ERROR_IO_PENDING)
      {
         Error(GetString(hInst, IDS_CANTFINDFILE), req_file.c_str(), transfer_machine.c_str(),
            GetLastError(), GetLastErrorStr());
         TransferCleanup();
         PostMessage(hwndMain, CM_RETRYABORT, 0, 0);
         return False;
      }
   }
   if (!HttpSendRequest(hFile, NULL, 0, NULL, 0))
   {
      Error(GetString(hInst, IDS_CANTSENDREQUEST), req_file.c_str(), transfer_machine.c_str(),
         GetLastError(), GetLastErrorStr());
      InternetCloseHandle(hFile);
      TransferCleanup();
      PostMessage(hwndMain, CM_RETRYABORT, 0, 0);
      return False;
   }

   // Create a local file.
   outfile = open(local_file_path.c_str(), O_BINARY | O_RDWR | O_CREAT | O_TRUNC,
      S_IWRITE | S_IREAD);
   if (outfile < 0)
   {
      Error(GetString(hInst, IDS_CANTWRITELOCALFILE), local_file_path.c_str(),
         GetLastError(), GetLastErrorStr());
      InternetCloseHandle(hFile);
      TransferCleanup();
      PostMessage(hwndMain, CM_RETRYABORT, 0, 0);
      return False;
   }

   // Read first block.
   done = False;
   bytes_read = 0;
   while (!done)
   {
      if (!InternetReadFile(hFile, buf, BUFSIZE, &size))
      {
         if (GetLastError() != ERROR_IO_PENDING)
         {
            Error(GetString(hInst, IDS_CANTREADFTPFILE), req_file.c_str(),
               GetLastError(), GetLastErrorStr());
            close(outfile);
            InternetCloseHandle(hFile);
            TransferCleanup();
            PostMessage(hwndMain, CM_RETRYABORT, 0, 0);
            return False;
         }
      }

      Status(GetString(hInst, IDS_READBLOCK));

      if (size > 0)
      {
         if (write(outfile, buf, size) != size)
         {
            Error(GetString(hInst, IDS_CANTWRITELOCALFILE), local_file_path.c_str(),
               GetLastError(), GetLastErrorStr());
            close(outfile);
            InternetCloseHandle(hFile);
            TransferCleanup();
            PostMessage(hwndMain, CM_RETRYABORT, 0, 0);
            return False;
         }
      }

      // Update graph position.
      bytes_read += size;
      SendMessage(hwndMain, CM_PROGRESS, 0, bytes_read);

      // See if done with file.
      if (size == 0)
      {
         close(outfile);

         if (hFile)
            InternetCloseHandle(hFile);
         done = True;
      }
   }

   return True;
}
#endif

static void inline TransferCleanup()
{
   InternetCloseHandle(hSession);
   InternetCloseHandle(hConnection);
}
