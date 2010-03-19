/******************************************************************************
 * Automatically generated file (iuplua5). Please don't change anything.                *
 *****************************************************************************/

#include <stdlib.h>

#include <lua.h>
#include <lauxlib.h>

#include "iup.h"
#include "iuplua.h"
#include "il.h"


static int Submenu(lua_State *L)
{
  Ihandle *ih = IupSubmenu((char *) luaL_optstring(L, 1, NULL), iuplua_checkihandleornil(L, 2));
  iuplua_plugstate(L, ih);
  iuplua_pushihandle_raw(L, ih);
  return 1;
}

int iupsubmenulua_open(lua_State * L)
{
  iuplua_register(L, Submenu, "Submenu");


#ifdef IUPLUA_USELOH
#ifdef TEC_BIGENDIAN
#ifdef TEC_64
#include "loh/submenu_be64.loh"
#else
#include "loh/submenu_be32.loh"
#endif
#else
#ifdef TEC_64
#ifdef WIN64
#include "loh/submenu_le64w.loh"
#else
#include "loh/submenu_le64.loh"
#endif
#else
#include "loh/submenu.loh"
#endif
#endif
#else
  iuplua_dofile(L, "submenu.lua");
#endif

  return 0;
}

