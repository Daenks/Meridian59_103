#
# Installer makefile, adapted from https://github.com/Meridian59/Meridian59
# Not called during normal build process. The idea behind this installer is
# only a few files are needed (meridian.exe and the few files needed to
# connect to the server) and the rest will be downloaded with the instructions
# given on connection. The meridian.exe used should be a lower version than
# the current server version, so that an update is triggered on connection.
#

TOPDIR=..
MAKENSIS = "c:\program files (x86)\nsis\makensis.exe"

SOURCEDIR = $(TOPDIR)\install

all: meridian59_server105_install.exe

meridian59_server105_install.exe: $(SOURCEDIR)\install.nsi
	$(MAKENSIS) $**
