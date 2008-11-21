------------------------------------------------------------------------------
-- SpinBox class 
------------------------------------------------------------------------------
local ctrl = {
  nick = "spinbox",
  parent = WIDGET,
  creation = "i",
  callback = {
    spin_cb = "n",
  },
  include = "iupspin.h",
}

function ctrl.createElement(class, arg)
   return Spinbox(arg[1])
end

iupRegisterWidget(ctrl)
iupSetClass(ctrl, "iup widget")
