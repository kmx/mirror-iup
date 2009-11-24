APPNAME = iupsample
APPTYPE = CONSOLE

INCLUDES = ../include

ifdef USE_GTK
  ifndef GTK_DEFAULT
    # Build GTK version in IRIX,SunOS,AIX,Win32
    APPNAME = iupsamplegtk
  endif
else  
  ifdef GTK_DEFAULT
    # Build Motif version in Linux,Darwin,FreeBSD
    USE_MOTIF = Yes
    APPNAME = iupsamplemot
  endif
endif

USE_IUP3 = Yes
USE_STATIC = Yes
IUP = ../../..

DBG = Yes

#SRC = alarm.c
#SRC = button.c
#SRC = dialog1.c
#SRC = dialog2.c
#SRC = filedlg.c
#SRC = fill.c
#SRC = frame.c
#SRC = getattribute.c
#SRC = getfile.c
#SRC = hbox.c
#SRC = idle.c
#SRC = image.c
#SRC = item.c
#SRC = label.c
#SRC = list1.c
#SRC = list2.c
#SRC = listdialog.c
#SRC = lua_init.c
#SRC = mask.c
#SRC = menu.c
#SRC = message.c
#SRC = multiline1.c
#SRC = multiline2.c
#SRC = progressbar.c
#SRC = radio.c
#SRC = sample.c
#SRC = sbox1.c
#SRC = sbox2.c
#SRC = scanf.c
#SRC = separator.c
#SRC = submenu.c
#SRC = tabs.c
#SRC = text.c
#SRC = timer.c
#SRC = toggle.c
SRC = tree.c
#SRC = val.c
#SRC = vbox.c
#SRC = zbox.c

#ifneq ($(findstring Win, $(TEC_SYSNAME)), )
#  LIBS += iupimglib
#else
#  SLIB += $(IUP)/lib/$(TEC_UNAME)/libiupimglib.a
#endif

#USE_CD = Yes
#SRC = canvas1.c
#SRC = canvas2.c
#SRC = canvas3.c
#SRC = scrollbar.c

#USE_OPENGL = Yes
#SRC = glcanvas.c

#USE_IUPCONTROLS = Yes
#SRC = canvas3.c
#SRC = cbox.c
#SRC = cells_checkboard.c
#SRC = cells_degrade.c
#SRC = cells_numbering.c
#SRC = colorbar.c
#SRC = colorbrowser.c
#SRC = gauge.c
#SRC = getcolor.c
#SRC = getparam.c
#SRC = matrix.c
#SRC = mdi.c

#USE_IUPCONTROLS = Yes
#LINKER = g++
#SRC = pplot.cpp
#ifneq ($(findstring Win, $(TEC_SYSNAME)), )
#  LIBS += iup_pplot cdpdflib
#else
#  SLIB += $(IUP)/lib/$(TEC_UNAME)/libiup_pplot.a $(CD)/lib/$(TEC_UNAME)/libcdpdflib.a
#endif

#USE_LUA51=Yes
#USE_IUPLUA=Yes
#SRC = lua_init.c
