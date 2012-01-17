PROJNAME = iup
APPNAME := iuplua
APPTYPE = CONSOLE

STRIP = 
OPT = YES      
NO_SCRIPTS = Yes
# IM and IupPPlot uses C++
LINKER = $(CPPC)

NO_LUAOBJECT = Yes
USE_BIN2C_LUA = Yes

ifdef USE_LUA52
  LUASFX = 52
else
  USE_LUA51 = Yes
  LUASFX = 51
endif

APPNAME := $(APPNAME)$(LUASFX)
SRC = iup_lua$(LUASFX).c

ifdef NO_LUAOBJECT
  DEFINES += IUPLUA_USELH
  USE_LH_SUBDIR = Yes
  LHDIR = lh
else
  DEFINES += IUPLUA_USELOH
  USE_LOH_SUBDIR = Yes
  LOHDIR = loh$(LUASFX)
endif

ifdef GTK_DEFAULT
  ifdef USE_MOTIF
    # Build Motif version in Linux and BSD
    APPNAME := $(APPNAME)mot
  endif
else  
  ifdef USE_GTK
    # Build GTK version in IRIX,SunOS,AIX,Win32
    APPNAME := $(APPNAME)gtk
  endif
endif

SRCLUA = console5.lua indent.lua

ifdef DBG
  ALL_STATIC=Yes
endif

ifdef ALL_STATIC
  # Statically link everything only when debugging
  IUP := ..
  USE_IUPLUA = Yes
  USE_IUP3 = Yes
  USE_STATIC = Yes
  
  ifdef DBG_DIR
    IUPLIB = $(IUP)/lib/$(TEC_UNAME)d
    CDLIB = $(CD)/lib/$(TEC_UNAME)d
    IMLIB = $(IM)/lib/$(TEC_UNAME)d
  else
    IUPLIB = $(IUP)/lib/$(TEC_UNAME)
    CDLIB = $(CD)/lib/$(TEC_UNAME)
    IMLIB = $(IM)/lib/$(TEC_UNAME)
  endif  
  
  DEFINES += USE_STATIC

  ifeq "$(TEC_UNAME)" "SunOS510x86"
    IUPLUA_NO_GL = Yes
  endif
    
  #IUPLUA_NO_GL = Yes
  ifndef IUPLUA_NO_GL 
    USE_OPENGL = Yes
  else
    DEFINES += IUPLUA_NO_GL
  endif

  #IUPLUA_NO_CD = Yes
  ifndef IUPLUA_NO_CD 
    USE_CDLUA = Yes
    USE_IUPCONTROLS = Yes
    ifneq ($(findstring Win, $(TEC_SYSNAME)), )
      LIBS += iuplua_pplot$(LIBLUASUFX) iup_pplot
    else
      SLIB += $(IUPLIB)/libiuplua_pplot$(LIBLUASUFX).a $(IUPLIB)/libiup_pplot.a
    endif
      
    ifndef IUPLUA_NO_IM
      ifneq ($(findstring Win, $(TEC_SYSNAME)), )
        LIBS += cdluaim$(LIBLUASUFX)
      else
        SLIB += $(CDLIB)/libcdluaim$(LIBLUASUFX).a
      endif
    endif
    ifneq ($(findstring Win, $(TEC_SYSNAME)), )
      ifndef USE_GTK
        USE_GDIPLUS=Yes
      endif
    else
      ifdef USE_MOTIF
        USE_XRENDER=Yes
      endif
    endif
  else
    DEFINES += IUPLUA_NO_CD
  endif

  #IUPLUA_NO_IM = Yes
  ifndef IUPLUA_NO_IM
    USE_IMLUA = Yes
    
    ifneq ($(findstring Win, $(TEC_SYSNAME)), )
      LIBS += imlua_process$(LIBLUASUFX) iupluaim$(LIBLUASUFX) im_process iupim
    else
      SLIB += $(IMLIB)/libimlua_process$(LIBLUASUFX).a $(IUPLIB)/libiupluaim$(LIBLUASUFX).a $(IMLIB)/libim_process.a $(IUPLIB)/libiupim.a
    endif
    
  else
    DEFINES += IUPLUA_NO_IM
  endif

  IUPLUA_IMGLIB = Yes
  ifdef IUPLUA_IMGLIB
    DEFINES += IUPLUA_IMGLIB
    ifneq ($(findstring Win, $(TEC_SYSNAME)), )
      LIBS += iupluaimglib$(LIBLUASUFX) iupimglib
    else
      SLIB += $(IUPLIB)/libiupluaimglib$(LIBLUASUFX).a $(IUPLIB)/libiupimglib.a
    endif
  endif
  
  IUPLUA_TUIO = Yes
  ifdef IUPLUA_TUIO
    DEFINES += IUPLUA_TUIO
    ifneq ($(findstring Win, $(TEC_SYSNAME)), )
      LIBS += iupluatuio$(LIBLUASUFX) iuptuio
      LIBS += ws2_32 winmm
    else
      SLIB += $(IUPLIB)/libiupluatuio$(LIBLUASUFX).a $(IUPLIB)/libiuptuio.a
    endif
  endif
else
  ifneq ($(findstring Win, $(TEC_SYSNAME)), )
    # Dinamically link in Windows, when not debugging
    # Must call "tecmake dll8" so USE_* will use the correct TEC_UNAME
    USE_DLL = Yes
    GEN_MANIFEST = No
  else
    # In UNIX Lua is always statically linked, late binding is used.
    USE_STATIC = Yes
    
    # Except in Cygwin
    ifneq ($(findstring cygw, $(TEC_UNAME)), )
      USE_STATIC:=
    endif
  endif
endif


ifneq ($(findstring Win, $(TEC_SYSNAME)), )
  SLIB += setargv.obj
  SRC += iuplua5.rc
endif

ifneq ($(findstring cygw, $(TEC_UNAME)), )
  LIBS += readline history
endif

ifneq ($(findstring MacOS, $(TEC_UNAME)), )
  LIBS += readline
endif

ifneq ($(findstring Linux, $(TEC_UNAME)), )
  LIBS += dl 
  #To allow late binding
  LFLAGS = -Wl,-E
  LIBS += readline history curses ncurses
endif

ifneq ($(findstring BSD, $(TEC_UNAME)), )
  #To allow late binding
  LFLAGS = -Wl,-E
  LIBS += readline history curses ncurses
endif

ifneq ($(findstring SunOS, $(TEC_UNAME)), )
  LIBS += dl
endif

ifneq ($(findstring AIX, $(TEC_UNAME)), )
  FLAGS  += -mminimal-toc
  OPTFLAGS = -mminimal-toc -ansi -pedantic 
  LFLAGS = -Xlinker "-bbigtoc"
endif

