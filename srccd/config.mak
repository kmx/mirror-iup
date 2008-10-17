PROJNAME = iup
LIBNAME = iupcd
#OPT = YES

IUP_ASSERT=Yes
DBG=YES
NO_DYNAMIC=Yes

DEFINES = CD_NO_OLD_INTERFACE
SRC = iupcd.c
DEF_FILE = iupcd.def

USE_CD=YES
# Can not use USE_IUP because Tecmake will include "iupcd" in linker

INCLUDES = ../include
LIBS = iup
LDIR = ../lib/$(TEC_UNAME)
