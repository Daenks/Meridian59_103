// Meridian 59, Copyright 1994-2012 Andrew Kirmse and Chris Kirmse.
// All rights reserved.
//
// This software is distributed under a license that is described in
// the LICENSE file that accompanies it.
//
// Meridian is a registered trademark.
/*
* loadrsc.c
*

  This module loads in the saved game dynarsc files and tells blakres.c
  about what it finds.  The .rsc format is described in resource.txt.
  These are stored in a series of files in the save game.
  
*/

#include "blakserv.h"

/* local function prototypes */
bool EachLoadRsc(char *filename, int resource_num, int lang_id, char *string);
Bool LoadDynamicRscName(char *filename);
void LoadRSBHash(char *rsb_path);

/******************************************************************************/
/*
 * LoadRsc: Loads the RESOURCE_RSC_SPEC files (.rsc or .rsb).
 */
void LoadRsc(void)
{
   char file_load_path[MAX_PATH+FILENAME_MAX];
   char *filespec = ConfigStr(RESOURCE_RSC_SPEC);
   char *filepath = ConfigStr(PATH_RSC);

   int files_loaded = 0;
   sprintf(file_load_path, "%s%s", filepath, filespec);
   StringVector files;
   if (FindMatchingFiles(file_load_path, &files))
   {
      for (StringVector::iterator it = files.begin(); it != files.end(); ++it)
      {
         sprintf(file_load_path, "%s%s", filepath, it->c_str());

         if (RscFileLoad(file_load_path, EachLoadRsc))
         {
            if (stricmp(filespec, "*.rsb") == 0)
               LoadRSBHash(file_load_path);
            files_loaded++;
         }
         else
            eprintf("LoadRsc error loading %s\n", it->c_str());
      }
   }

   dprintf("LoadRsc loaded %i of %i found %s files\n",
      files_loaded, files.size(), filespec);

}
/******************************************************************************/
/*
 * LoadRSBHash:  Maps the RSB file and creates a hash of it.
 */
void LoadRSBHash(char *rsb_path)
{
   FILE *fp;
   char *ptr;
   struct stat st;
   unsigned char rsb_hash[ENCRYPT_LEN + 1];
   unsigned long size;

   rsb_hash[0] = 0;

   fp = fopen(rsb_path, "rb");
   if (!fp)
      return;

   // Get file size.
   stat(rsb_path, &st);
   size = st.st_size;

   // Allocate buffer and read file in.
   ptr = (char *)AllocateMemory(MALLOC_ID_RESOURCE, size);
   if (fread(ptr, 1, size, fp) == size)
   {
      MDStringBytes(ptr, rsb_hash, size);
      SaveRsbMD5((char *)rsb_hash);
   }

   FreeMemory(MALLOC_ID_RESOURCE, ptr, size);
   fclose(fp);

   return;
}
/******************************************************************************/
/*
 * EachLoadRsc:  Adds a resource loaded from an rsb/rsc file.
 */
bool EachLoadRsc(char *filename, int resource_num, int lang_id, char *string)
{
   AddResource(resource_num,lang_id,string);
   return true;
}
/******************************************************************************/
/*
 * LoadDynamicRsc:  Calls LoadDynamicRscName to load a dynamic resource file.
 */
void LoadDynamicRsc(char *filename)
{
   if (LoadDynamicRscName(filename) == False)
      eprintf("LoadDynamicRsc error loading %s\n",filename);
}
/******************************************************************************/
/*
 * LoadDynamicRscName:  Loads the dynamic resources from a .rsc file.
 *   These are the server resouces for player names.
 */
Bool LoadDynamicRscName(char *filename)
{
   FILE *fh = fopen(filename,"rb");
   int magic_num;
   int version;
   unsigned int num_resources;
   unsigned int i;

   if (!fh)
      return False;

   fread(&magic_num,4,1,fh);
   fread(&version,4,1,fh);
   fread(&num_resources,4,1,fh);
   if (ferror(fh) || feof(fh))
   {
      fclose(fh);
      return False;
   }

   if (version != 1)
   {
      eprintf("LoadDynamicRscName can't understand rsc version != 1\n");
      fclose(fh);
      return False;
   }

   for (i = 0; i < num_resources; ++i)
   {
      int resource_id;
      int resource_type;
      unsigned int len_data;
      char resource_value[500];

      fread(&resource_id,4,1,fh);
      fread(&resource_type,4,1,fh);
      fread(&len_data,4,1,fh);

      if (ferror(fh) || feof(fh))
      {
         fclose(fh);
         return False;
      }

      if (len_data > sizeof(resource_value)-1)
      {
         eprintf("LoadDynamicRscName got invalid long dynamic resource %u\n",
            len_data);
         return False;
      }

      fread(&resource_value,len_data,1,fh);

      if (ferror(fh) || feof(fh))
      {
         fclose(fh);
         return False;
      }
      resource_value[len_data] = '\0'; // null-terminate string
      //dprintf("got %u %s\n",resource_id,resource_value);
      AddResource(resource_id,0,resource_value);
   }

   fclose(fh);
   return True;
}
