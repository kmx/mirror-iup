PROJNAME = iup
LIBNAME = iuplua_mglplot

IUP := ..

OPT = YES
NO_LUAOBJECT = Yes
NO_LUALINK = Yes
USE_BIN2C_LUA = Yes

USE_IUP3 = Yes
USE_IUPLUA = Yes

DEF_FILE = iuplua_mglplot.def
LIBS = iup_mglplot

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

SRCLUA = mglplot.lua

GC := $(addsuffix .c, $(basename $(SRCLUA)))
GC := $(addprefix il_, $(GC))

$(GC) : il_%.c : %.lua generator.lua
	$(LUABIN) generator.lua $<

SRC := iuplua_mglplot.c $(GC)

ifneq ($(findstring MacOS, $(TEC_UNAME)), )
  USE_IUPLUA:=
endif
