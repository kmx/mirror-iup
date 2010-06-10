PROJNAME = iup
APPNAME := iupview
OPT = YES

SRC = iup_view.c

IUP := ..

LINKER = $(CPPC)

USE_CD = Yes
USE_IUPCONTROLS = Yes
USE_IUP3 = Yes

ifdef GTK_DEFAULT
  ifdef USE_MOTIF
    # Build Motif version in Linux,Darwin,FreeBSD
    APPNAME := $(APPNAME)mot
  endif
else  
  ifdef USE_GTK
    # Build GTK version in IRIX,SunOS,AIX,Win32
    APPNAME := $(APPNAME)gtk
  endif
endif

ifeq "$(TEC_UNAME)" "SunOS510x86"
  DEFINES = USE_NO_OPENGL
else  
  USE_OPENGL = Yes
endif

USE_IM = Yes
ifdef USE_IM
  DEFINES += USE_IM  
  ifneq ($(findstring Win, $(TEC_SYSNAME)), )
    LIBS = iupim iupimglib
  else
    ifdef DBG_DIR
      IUPLIB = $(IUP)/lib/$(TEC_UNAME)d
    else
      IUPLIB = $(IUP)/lib/$(TEC_UNAME)
    endif  
    SLIB = $(IUPLIB)/libiupim.a $(IUPLIB)/libiupimglib.a
  endif             
endif 

USE_STATIC = Yes

ifneq ($(findstring Win, $(TEC_SYSNAME)), )
  SRC += ../etc/iup.rc
endif

INCLUDES = ../src

ifeq ($(TEC_UNAME), vc8)
  ifdef DBG
    #debug info not working for vc8 linker
    define DBG
    endef
  endif
endif         
