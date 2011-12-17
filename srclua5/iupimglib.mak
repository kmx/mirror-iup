PROJNAME = iup
LIBNAME = iupluaimglib

IUP := ..

OPT = YES
NO_LUALINK = Yes

USE_IUP3 = Yes
USE_IUPLUA = Yes

SRC = iuplua_imglib.c
DEF_FILE = iupluaimglib.def
LIBS = iupimglib

ifdef USE_LUA52
  LUASFX = 52
else
  USE_LUA51 = Yes
  LUASFX = 51
endif

LIBNAME := $(LIBNAME)$(LUASFX)

ifneq ($(findstring MacOS, $(TEC_UNAME)), )
  USE_IUPLUA:=
endif
