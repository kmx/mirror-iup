------------------------------------------------------------------------------
-- TuioClient class 
------------------------------------------------------------------------------
local ctrl = {
  nick = "tuioclient",
  parent = iup.WIDGET,
  creation = "N",  -- optional integer
  funcname = "TuioClient",
  callback = {
    cursor_cb = "nnnns",
  },
  include = "iuptuio.h",
  extracode = [[ 
static int tuio_multitouch_cb(Ihandle *ih, int count, int* px, int* py, int* id)
{
  int i;
  lua_State *L = iuplua_call_start(ih, "multitouch_cb");
  lua_pushinteger(L, count);
  lua_createtable(L, count, 0);
  for (i = 0; i < count; i++)
  {
    lua_pushinteger(L,i+1);
    lua_pushinteger(L,px[i]);
    lua_settable(L,-3);
  }
  lua_createtable(L, count, 0);
  for (i = 0; i < count; i++)
  {
    lua_pushinteger(L,i+1);
    lua_pushinteger(L,py[i]);
    lua_settable(L,-3);
  }
  lua_createtable(L, count, 0);
  for (i = 0; i < count; i++)
  {
    lua_pushinteger(L,i+1);
    lua_pushinteger(L,id[i]);
    lua_settable(L,-3);
  }
  return iuplua_call(L, 2);
}

int iuptuiolua_open(lua_State* L)
{
  if (iuplua_opencall_internal(L))
    IupTuioOpen();
    
  iuplua_get_env(L);
  iuptuioclientlua_open(L);
  
  iuplua_register_cb(L, "MULTITOUCH_CB", (lua_CFunction)tuio_multitouch_cb, NULL);
  
  return 0;
}

/* obligatory to use require"iupluatuio" */
int luaopen_iupluatuio(lua_State* L)
{
  return iuptuiolua_open(L);
}

]]
}

function ctrl.createElement(class, param)
  return iup.TuioClient(param[1])
end

iup.RegisterWidget(ctrl)
iup.SetClass(ctrl, "iup widget")
