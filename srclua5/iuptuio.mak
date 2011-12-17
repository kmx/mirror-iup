PROJNAME = iup
LIBNAME  = iupluatuio

IUP := ..

OPT = YES
NO_LUALINK = Yes
USE_BIN2C_LUA = Yes
NO_LUAOBJECT = Yes

USE_IUP3 = Yes
USE_IUPLUA = Yes

DEF_FILE = iupluatuio.def
INCLUDES = ../src
LIBS = iuptuio

ifdef USE_LUA52
  LUASFX = 52
else
  USE_LUA51 = Yes
  LUASFX = 51
endif

LIBNAME := $(LIBNAME)$(LUASFX)
ifdef NO_LUAOBJECT
  DEFINES += IUPLUA_USELH
  USE_LH_SUBDIR = Yes
  LHDIR = lh
else
  DEFINES += IUPLUA_USELOH
  USE_LOH_SUBDIR = Yes
  LOHDIR = loh$(LUASFX)
endif

SRCLUA = tuioclient.lua

GC = $(addsuffix .c, $(basename $(SRCLUA)))
GC := $(addprefix il_, $(GC))

$(GC) : il_%.c : %.lua generator.lua
	$(LUABIN) generator.lua $<

SRC	= $(GC)

ifneq ($(findstring MacOS, $(TEC_UNAME)), )
  USE_IUPLUA:=
endif
