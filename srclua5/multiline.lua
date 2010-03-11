------------------------------------------------------------------------------
-- Multiline class 
------------------------------------------------------------------------------
local ctrl = {
  nick = "multiline",
  parent = WIDGET,
  creation = "-",
  callback = {
    action = "ns", 
  },
  funcname = "MultiLine",
} 

function ctrl.createElement(class, param)
   return MultiLine()
end
   
iupRegisterWidget(ctrl)
iupSetClass(ctrl, "iup widget")
