// Meridian 59, Copyright 1994-2012 Andrew Kirmse and Chris Kirmse.
// All rights reserved.
//
// This software is distributed under a license that is described in
// the LICENSE file that accompanies it.
//
// Meridian is a registered trademark.
/*
 * clientpatch.h:  Header file containing client patching routines.
 */

#ifndef _CLIENTPATCH_H
#define _CLIENTPATCH_H

#include <jansson.h>
#include <md5.h>

#define HASHABLE_BYTES 16
#define HEX_HASH 33

#define CLIENTPATCH_FILE_VERSION 3

void ClientPatchGetValidBasepath(json_t **CacheFile, char *retpath);
void ClientPatchGetAbsPath(json_t **CacheFile, char *retpath);
bool CompareCacheToLocalFile(json_t **CacheFile);
json_t * GenerateCacheFile(const char *fullpath, const char *basepath, const char *file);
void GenerateCacheMD5(const char *fullpath, const char *file, json_t **CacheFile);

/*
 * ClientPatchGetValidBasepath: Get the base path including the current
 *   directory '.', put the result into retpath.
 */
void ClientPatchGetValidBasepath(json_t **CacheFile, char *retpath)
{
   const char *path = json_string_value(json_object_get(*CacheFile, "Basepath"));
   strcpy(retpath, ".");
   strcat(retpath, path);
}

/*
 * ClientPatchGetValidFilepath: Get the path (base and file) including
 *   the current directory '.', put the result into retpath.
 */
void ClientPatchGetValidFilepath(json_t **CacheFile, char *retpath)
{
   const char *file = json_string_value(json_object_get(*CacheFile, "Filename"));
   const char *path = json_string_value(json_object_get(*CacheFile, "Basepath"));

   strcpy(retpath, ".");
   strcat(retpath, path);
   strcat(retpath, file);
}

/*
 * CompareCacheToLocalFile: Takes a cachefile, compares it to a local one.
 *                          Returns true if they are the same, false otherwise.
 */
bool CompareCacheToLocalFile(json_t **CacheFile)
{
   HANDLE fp, fileHandle;
   char *buffer;
   unsigned char hash[HASHABLE_BYTES];
   char myhash[HEX_HASH];
   unsigned long size;
   const char *file = json_string_value(json_object_get(*CacheFile, "Filename"));
   const char *path = json_string_value(json_object_get(*CacheFile, "Basepath"));

   char combine[MAX_PATH + FILENAME_MAX];
   strcpy(combine, ".");
   strcat(combine, path);
   strcat(combine, file);

   // Lookup local file.
   fp = CreateFile(combine, GENERIC_READ, FILE_SHARE_READ, NULL,
      OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN, NULL);
   if (!fp)
      return false;

   size = GetFileSize(fp, NULL);

   if (size != json_integer_value(json_object_get(*CacheFile, "Length")))
   {
      CloseHandle(fp);
      return false;
   }

   fileHandle = CreateFileMapping(fp, NULL, PAGE_READONLY, 0, size, NULL);
   if (!fileHandle)
   {
      CloseHandle(fp);
      return false;
   }

   buffer = (char *)MapViewOfFile(fileHandle, FILE_MAP_READ, 0, 0, 0);
   if (!buffer)
   {
      CloseHandle(fp);
      CloseHandle(fileHandle);
      return false;
   }

   MDStringBytes(buffer, hash, size);
   UnmapViewOfFile(buffer);
   CloseHandle(fp);
   CloseHandle(fileHandle);

   // Convert to hex.
   for (int i = 0; i < 16; ++i)
      sprintf(myhash + i * 2, "%02X", hash[i]);

   // Null terminate.
   myhash[32] = 0;

   if (strcmp(myhash, json_string_value(json_object_get(*CacheFile, "MyHash"))) != 0)
      return false;
   return true;
}

/*
 * GenerateCacheFile: Takes a full path to a file, a relative path (from client root)
 *   and a filename, generates a json_t object containing Basepath, Filename, Version,
 *   Length, MyHash and whether to download (zip false, all else true). Returns the
 *   object.
 */
json_t * GenerateCacheFile(const char *fullpath, const char *basepath, const char *file)
{
   json_t *CacheFile;
   CacheFile = json_object();

   json_object_set(CacheFile, "Basepath", json_string(basepath));
   json_object_set(CacheFile, "Filename", json_string(file));
   json_object_set(CacheFile, "Version", json_pack("i", CLIENTPATCH_FILE_VERSION));

   if (stricmp(strrchr(file, '.'), ".zip") == 0)
      json_object_set(CacheFile, "Download", json_pack("b", 0));
   else
      json_object_set(CacheFile, "Download", json_pack("b", 1));

   GenerateCacheMD5(fullpath, file, &CacheFile);

   return CacheFile;
}

/*
 * GenerateCacheMD5: Takes a json_t file containing Filename and Basepath.
 *                   Adds the MD5 hash and file length.
 */
void GenerateCacheMD5(const char *fullpath, const char *file, json_t **CacheFile)
{
   HANDLE fp, fileHandle;
   char *buffer;
   unsigned char hash[HASHABLE_BYTES];
   char myhash[HEX_HASH];
   unsigned long size;
   char combine[MAX_PATH + FILENAME_MAX];

   strcpy(combine, fullpath);
   strcat(combine, file);

   fp = CreateFile(combine, GENERIC_READ, FILE_SHARE_READ, NULL,
      OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN, NULL);
   if (!fp)
      return;

   size = GetFileSize(fp, NULL);
   if (size == INVALID_FILE_SIZE
      || !size)
   {
      json_object_set(*CacheFile, "Length", json_pack("i", 0));
      CloseHandle(fp);

      return;
   }

   json_object_set(*CacheFile, "Length", json_pack("i", size));

   fileHandle = CreateFileMapping(fp, NULL, PAGE_READONLY, 0, size, NULL);
   if (!fileHandle)
   {
      CloseHandle(fp);
      return;
   }

   buffer = (char *)MapViewOfFile(fileHandle, FILE_MAP_READ, 0, 0, 0);
   if (!buffer)
   {
      CloseHandle(fp);
      CloseHandle(fileHandle);
      json_object_set(*CacheFile, "MyHash", json_string(""));
      return;
   }

   MDStringBytes(buffer, hash, size);
   UnmapViewOfFile(buffer);
   CloseHandle(fp);
   CloseHandle(fileHandle);

   // Convert to hex.
   for (int i = 0; i < 16; ++i)
      sprintf(myhash + i * 2, "%02X", hash[i]);

   // Null terminate.
   myhash[32] = 0;
   json_object_set(*CacheFile, "MyHash", json_string(myhash));
}

#endif /* #ifndef _CLIENTPATCH_H */
