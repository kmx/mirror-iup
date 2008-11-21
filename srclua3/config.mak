PROJNAME = iup
LIBNAME = iuplua3
OPT = YES

LOHDIR = loh
SRCLUA = iuplua.lua iuplua_widgets.lua constants.lua
SRC    = iuplua.c iuplua_api.c iuplua_widgets.c 

USE_LUA = Yes

DEFINES = IUPLUA_USELOH
INCLUDES = ../include ../src
LDIR = ../lib/$(TEC_UNAME)  
LIBS = iup

ifeq ($(findstring Win, $(TEC_SYSNAME)), )
  USE_MOTIF = Yes
endif

