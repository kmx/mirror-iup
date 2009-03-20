APPNAME = iuptest
APPTYPE = CONSOLE

INCLUDES = ../include

ifdef USE_GTK
  ifndef GTK_DEFAULT
    # Build GTK version in IRIX,SunOS,AIX,Win32
    APPNAME = iuptestgtk
  endif
else  
  ifdef GTK_DEFAULT
    # Build Motif version in Linux,Darwin,FreeBSD
    USE_MOTIF = Yes
    APPNAME = iuptestmot
  endif
endif

USE_IUP3 = Yes
USE_STATIC = Yes
IUP = ..

DBG = Yes

# Must uncomment all SRC lines
DEFINES = BIG_TEST
SRC += bigtest.c  

SRC += tray.c
SRC += dialog.c
SRC += predialogs.c
SRC += timer.c
SRC += label.c
SRC += canvas.c
SRC += frame.c
SRC += idle.c
SRC += button.c
SRC += toggle.c
SRC += vbox.c
SRC += hbox.c
SRC += progressbar.c
SRC += text.c
SRC += val.c
SRC += tabs.c
SRC += sample.c
SRC += menu.c
SRC += spin.c
SRC += text_spin.c
SRC += list.c
SRC += sysinfo.c
SRC += mdi.c
SRC += getparam.c
SRC += getcolor.c

#ifneq ($(findstring Win, $(TEC_SYSNAME)), )
#  LIBS += iupimglib
#else
#  SLIB += $(IUP)/lib/$(TEC_UNAME)/libiupimglib.a
#endif

USE_CD = Yes
SRC += canvas_scrollbar.c
SRC += canvas_cddbuffer.c

USE_OPENGL = Yes
SRC += glcanvas.c
SRC += glcanvas_cube.c

USE_IUPCONTROLS = Yes
SRC += colorbrowser.c
SRC += dial.c
SRC += colorbar.c
SRC += cells_numbering.c
SRC += cells_degrade.c
SRC += cells_checkboard.c
SRC += gauge.c
SRC += matrix.c
SRC += tree.c

LINKER = g++
SRC += pplot.c
ifneq ($(findstring Win, $(TEC_SYSNAME)), )
  LIBS += iup_pplot
#  LIBS += cdpdflib
else
  SLIB += $(IUP)/lib/$(TEC_UNAME)/libiup_pplot.a
#  SLIB += $(CD)/lib/$(TEC_UNAME)/libcdpdflib.a
endif

ifneq ($(findstring Win, $(TEC_SYSNAME)), )
  SRC += iuptest.rc 
else
  ifneq ($(findstring cygw, $(TEC_UNAME)), )
    SRC += iuptest.rc
  endif
endif
 
#ifneq ($(findstring Win, $(TEC_SYSNAME)), )
#  USE_GDIPLUS=Yes
#else
#  USE_XRENDER=Yes
#endif
