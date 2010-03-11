require"iuplua"

-- Utilities
iup_console = {}

function iup_console.printtable(t)
  local n,v = next(t, nil)
  print("--printtable Start--")
  while n do
    print(tostring(n).."="..tostring(v))
    n,v = next(t, n)
  end
  print("--printtable End--")
end

function iup_console.print_version_info()
  if (im) then print("IM " .. im._VERSION .. "  " .. im._COPYRIGHT) end

  if (cd) then print("CD " .. cd._VERSION .. "  " .. cd._COPYRIGHT) end

  print("IUP " .. iup._VERSION .. "  " .. iup._COPYRIGHT)
  print("")
  print("IUP Info")
  print("  System: " .. iup.GetGlobal("SYSTEM"))
  print("  System Version: " .. iup.GetGlobal("SYSTEMVERSION"))

  local mot = iup.GetGlobal("MOTIFVERSION")
  if (mot) then print("  Motif Version: ", mot) end

  print("  Screen Size: " .. iup.GetGlobal("SCREENSIZE"))
  print("  Screen Depth: " .. iup.GetGlobal("SCREENDEPTH"))

  if (iup.GL_VENDOR) then print("  OpenGL Vendor: " .. iup.GL_VENDOR) end
  if (iup.GL_RENDERER) then print("  OpenGL Renderer: " .. iup.GL_RENDERER) end
  if (iup.GL_VERSION) then print("  OpenGL Version: " .. iup.GL_VERSION) end
end

-- Console Dialog

iup_console.lastfilename = nil -- Last file open
iup_console.mlCode = iup.multiline{expand="YES", size="200x120", font="COURIER_NORMAL_10"}
iup_console.lblPosition = iup.label{title="0:0", size="50x"}
iup_console.lblFileName = iup.label{title="", size="50x", expand="HORIZONTAL"}

function iup_console.mlCode:caret_cb(lin, col)
  iup_console.lblPosition.title = lin..":"..col
end

iup_console.butExecute = iup.button{size="50x15", title="Execute",
                                    action="iup.dostring(iup_console.mlCode.value)"}
iup_console.butClearCommands = iup.button{size="50x15", title="Clear",
                                          action="iup_console.mlCode.value=''  iup_console.lblFileName.title = ''  iup_console.lastfilename = nil"}
iup_console.butLoadFile = iup.button{size="50x15", title="Load..."}
iup_console.butSaveasFile = iup.button{size="50x15", title="Save As..."}
iup_console.butSaveFile = iup.button{size="50x15", title="Save"}

function iup_console.butSaveFile:action()
  if (iup_console.lastfilename == nil) then
    iup_console.butSaveasFile:action()
  else
    newfile = io.open(iup_console.lastfilename, "w+")
    if (newfile) then
      newfile:write(iup_console.mlCode.value)
      newfile:close()
    else
      error ("Cannot Save file "..filename)
    end
  end
end

function iup_console.butSaveasFile:action()
  local fd = iup.filedlg{dialogtype="SAVE", title="Save File", 
                         nochangedir="NO", directory=iup_console.last_directory,
                         filter="*.*", filterinfo="All files",allownew=yes}
                         
  fd:popup(iup.LEFT, iup.LEFT)
  
  local status = fd.status
  iup_console.lastfilename = fd.value
  iup_console.lblFileName.title = fd.value
  iup_console.last_directory = fd.directory
  fd:destroy()
  
  if status ~= "-1" then
    if (iup_console.lastfilename == nil) then
      error ("Cannot Save file "..filename)
    end
    local newfile=io.open(iup_console.lastfilename, "w+")
    if (newfile) then
      newfile:write(iup_console.mlCode.value)
      newfile:close(newfile)
    else
      error ("Cannot Save file")
    end
   end
end

function iup_console.LoadFile(filename)
  local newfile = io.open (filename, "r")
  if (newfile == nil) then
    error ("Cannot load file "..filename)
  else
    iup_console.mlCode.value=newfile:read("*a")
    newfile:close (newfile)
    iup_console.lastfilename = filename
    iup_console.lblFileName.title = iup_console.lastfilename
  end
end

function iup_console.butLoadFile:action()
  local fd=iup.filedlg{dialogtype="OPEN", title="Load File", 
                       nochangedir="NO", directory=iup_console.last_directory,
                       filter="*.*", filterinfo="All Files", allownew="NO"}
  fd:popup(iup.CENTER, iup.CENTER)
  local status = fd.status
  local filename = fd.value
  iup_console.last_directory = fd.directory
  fd:destroy()
  
  if (status == "-1") or (status == "1") then
    if (status == "1") then
      error ("Cannot load file "..filename)
    end
  else
    iup_console.LoadFile(filename)
  end
end

iup_console.vbxConsole = iup.vbox
{
  iup.frame{iup.hbox{iup.vbox{iup_console.butLoadFile,
                              iup_console.butSaveFile,
                              iup_console.butSaveasFile,
                              iup_console.butClearCommands,
                              iup_console.butExecute;
                              margin="0x0", gap="10"},
                     iup.vbox{iup_console.lblFileName,
                              iup_console.mlCode,
                              iup_console.lblPosition;
                              alignment = "ARIGHT"};
                     alignment="ATOP"}; title="Commands"}
   ;alignment="ACENTER", margin="5x5", gap="5"
}

-- Main Menu Definition.

iup_console.mnuMain = iup.menu
{
  iup.submenu
  {
    iup.menu
    {
      iup.item{title="Exit", action="return iup.CLOSE"}
    }; title="File"
  },
  iup.submenu{iup.menu
  {
    iup.item{title="Print Version Info...", action=iup_console.print_version_info},
    iup.item{title="About...", action="iup_console.dlgAbout:popup(iup.CENTER, iup.CENTER)"}
  };title="Help"}
}

-- Main Dialog Definition.

iup_console.dlgMain = iup.dialog{iup_console.vbxConsole;
                                 title="IupLua Console",
                                 menu=iup_console.mnuMain,
                                 dragdrop = "YES",
                                 defaultenter=iup_console.butExecute,
                                 close_cb = "return iup.CLOSE"}

function iup_console.dlgMain:dropfiles_cb(filename, num, x, y)
  if (num == 0) then
    iup_console.LoadFile(filename)
  end
end

-- About Dialog Definition.

iup_console.dlgAbout = iup.dialog
{
  iup.vbox
  {
      iup.label{title="IupLua Console"},
      iup.fill{size="5"},
      iup.fill{size="5"},
      iup.frame
      {
          iup.vbox
          {
              iup.label{title="Tecgraf/PUC-Rio"},
              iup.label{title="iup@tecgraf.puc-rio.br"}
          }
      },
      iup.fill{size="5"},
      iup.button{title="OK", action="return iup.CLOSE", size="50X20"}
      ;margin="10x10", alignment="ACENTER"
   }
   ;maxbox="NO", minbox="NO", resize="NO", title="About"
}

-- Displays the Main Dialog

iup_console.dlgMain:show()
iup.SetFocus(iup_console.mlCode)

if (not iup.MainLoopLevel or iup.MainLoopLevel()==0) then
  iup.MainLoop()
end

iup_console.dlgMain:destroy()
iup_console.dlgAbout:destroy()
