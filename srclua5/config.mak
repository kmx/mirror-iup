BUILD_IUP3 = Yes

PROJNAME = iup
LIBNAME  = iuplua51
OPT = YES
DEF_FILE = iuplua5.def

DEFINES = IUPLUA_USELOH

USE_LUA51 = Yes

INCLUDES = ../include ../src
LDIR = ../lib/$(TEC_UNAME)  
LIBS = iup

CTRLUA = button.lua canvas.lua dialog.lua \
       filedlg.lua fill.lua frame.lua hbox.lua \
       item.lua image.lua label.lua menu.lua multiline.lua \
       list.lua separator.lua radio.lua \
       submenu.lua text.lua toggle.lua vbox.lua zbox.lua timer.lua \
       sbox.lua spin.lua spinbox.lua cbox.lua
       
ifdef BUILD_IUP3
  CTRLUA += val.lua tabs.lua
endif

GC := $(addsuffix .c, $(basename $(CTRLUA)))
GC := $(addprefix il_, $(GC))
SRCLUA = iuplua.lua constants.lua $(CTRLUA)
LOHDIR = loh

$(GC) : il_%.c : %.lua generator.lua
	$(LUABIN) generator.lua $<

SRC = iuplua.c iuplua_api.c il_scanf.c il_getparam.c il_getcolor.c $(GC)

ifeq ($(findstring Win, $(TEC_SYSNAME)), )
  USE_MOTIF = Yes
endif
