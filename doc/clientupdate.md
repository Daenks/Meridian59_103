Client Updating
--------------

The classic client sends a version number when it connects to the server,
comprised of a major and a minor version. The major version for the client
is currently 50, and the minor version is incremented with each update.
If the minor version number is detected as outdated, the server will send
an AP_CLIENT_PATCH protocol message telling the client that it needs to update.
This protocol includes details on the download location for the client, the
update program and a cache file containing info on the client files.

### AP_CLIENT_PATCH protocol
The protocol ID for this message is 13.
The details sent are:

   * host machine to connect to (e.g. www.patch.com)
   * the client directory on the host, without backslash (e.g. /105/client)
   * the patch cachefile path, with backslash (e.g. /105/)
   * the patch cachefile containing info on the client files (e.g. patchinfo.txt)
   * the updater executable name (e.g. club.exe)
   * the reason for downloading (e.g. "Your client is out of date.")

### Blakserv config values
These strings should be placed in blakserv.cfg before starting up the server.
Currently Classic and Ogre clients are supported by this protocol.

Example config, placed under the [Update] section:

```
[Update]
DownloadReason          <An update of Meridian files is available and required.>
ClassicPatchHost        www.patch.com
ClassicPatchPath        /105/client
ClassicPatchCachePath   /105/
ClassicPatchTxt         clientpatch.txt
ClassicClubExe          club.exe
OgrePatchHost           www.patch.com
OgrePatchPath           /NET105/client
OgrePatchCachePath      /NET105/
OgrePatchTxt            patchinfo.txt
OgreClubExe             Meridian59.Patcher.exe
```

### Update program
The client will first download the update executable, then run it. The updater
will update all the client files followed by running the same client that
launched it. The updater should download and load the patch cache file and then
download any missing or outdated files in the order listed in the cache file.
The updater program for the classic Meridian 59 client is club.exe, which can
be found in the Club project.

### Patch cache file/Clientpatch
The patch cache file contains information about each of the client files to
download. This is stored as a JSON array of objects for each file. Each object
has six fields:

   * Basepath (string, the relative path from the client root directory)
   * Filename (string, filename of the client file to download)
   * Version (int, used internally by the patch cache generator to version files)
   * Download (boolean, whether the file is downloaded or not)
   * Length (int, the length in bytes of the file)
   * MyHash (string, a 16-byte MD5 hash of the full file contents)

This file is generated using /bin/clientpatch.exe, which is generated from the
Clientpatch project. To generate the file, run as follows:
```
clientpatch.exe c:\105\clientpatch.txt c:\105\clientpatch
```
