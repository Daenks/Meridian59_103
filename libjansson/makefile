# makefile for jansson

TOPDIR=..
!include $(TOPDIR)\common.mak

# ----------------------------------------------------------------------
# Additional compiler flags (see common.mak)
# /TC           Compile as C code
# /arch:SSE     Use SSE instructions (VS2013+ default to SSE2)
# /wd4005       Ignore macro redefinition warning
CFLAGS = $(CFLAGS) /arch:SSE /TC /D_USING_V110_SDK71_ /wd4005

# ----------------------------------------------------------------------
# Additional static linker flags (see common.mak)
# /SUBSYSTEM:CONSOLE",5.01" NO-UI Windows XP (5.01)
LINKSTFLAGS = $(LINKSTFLAGS) /SUBSYSTEM:CONSOLE",5.01"

OBJS = \
    $(OUTDIR)\dump.obj \
    $(OUTDIR)\error.obj \
    $(OUTDIR)\hashtable.obj \
    $(OUTDIR)\hashtable_seed.obj \
    $(OUTDIR)\load.obj \
    $(OUTDIR)\memory.obj \
    $(OUTDIR)\pack_unpack.obj \
    $(OUTDIR)\strbuffer.obj \
    $(OUTDIR)\strconv.obj \
    $(OUTDIR)\utf.obj \
    $(OUTDIR)\value.obj

all: makedirs $(BLAKLIBDIR)\jansson.lib

$(BLAKLIBDIR)\jansson.lib: $(OBJS)
    $(LINKST) $(LINKSTFLAGS) $** $(LIBS) -out:$@
		
!include $(TOPDIR)\rules.mak
