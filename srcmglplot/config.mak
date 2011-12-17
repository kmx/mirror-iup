PROJNAME = iup
LIBNAME  = iup_mglplot
OPT = YES

USE_OPENGL = Yes

ifdef DBG
  DEFINES += IUP_ASSERT
endif  

FTGL_INC = $(CD)/src/ftgl
FTGL_LIB = $(CD)/lib/$(TEC_UNAME)
FREETYPE_INC = $(CD)/src/freetype2

INCLUDES = ../include ../src . $(FTGL_INC) $(FREETYPE_INC)
LDIR = ../lib/$(TEC_UNAME) $(FTGL_LIB)
LIBS = iup iupgl ftgl

ifneq ($(findstring Win, $(TEC_SYSNAME)), )
  ifeq ($(findstring dll, $(TEC_UNAME)), )
    DEFINES += FTGL_LIBRARY_STATIC
  endif
endif

DEFINES = NO_PNG NO_GSL

SRCMGLPLOT = mgl_1d.cpp mgl_crust.cpp mgl_evalc.cpp \
  mgl_2d.cpp mgl_data.cpp mgl_evalp.cpp mgl_main.cpp \
  mgl_3d.cpp mgl_data_cf.cpp mgl_exec.cpp mgl_parse.cpp \
  mgl_ab.cpp mgl_data_io.cpp mgl_export.cpp mgl_pde.cpp \
  mgl_addon.cpp mgl_data_png.cpp mgl_fit.cpp mgl_tex_table.cpp \
  mgl_axis.cpp mgl_def_font.cpp mgl_flow.cpp mgl_vect.cpp \
  mgl_combi.cpp mgl_eps.cpp mgl_font.cpp mgl_zb.cpp \
  mgl_cont.cpp mgl_eval.cpp mgl_gl.cpp mgl_zb2.cpp
SRCMGLPLOT := $(addprefix mgl/, $(SRCMGLPLOT))

SRC = iup_mglplot.cpp mgl_makefont.cpp $(SRCMGLPLOT)

ifneq ($(findstring MacOS, $(TEC_UNAME)), )
  ifdef USE_MACOS_OPENGL
    LFLAGS = -framework OpenGL
    USE_OPENGL :=
  endif
  ifneq ($(TEC_SYSMINOR), 4)
    BUILD_DYLIB=Yes
  endif
endif
