/** \file
 * \brief initializes iupdial, iupgauge, iuptabs, iupcb, iupgc and iupval controls.
 *
 * See Copyright Notice in iup.h
 */
 
#ifndef __IUPCONTROLS_H 
#define __IUPCONTROLS_H

#include "iupdial.h"
#include "iupcells.h"
#include "iupcolorbar.h"
#include "iupgauge.h"
#include "iuptabs.h"
#include "iupval.h"
#include "iupmatrix.h"
#include "iuptree.h"
//#include "iupsbox.h"
#include "iupmask.h"
#include "iupcb.h"
#include "iupspin.h"
//#include "iupgetparam.h"

#ifdef __cplusplus
extern "C" {
#endif

int IupControlsOpen(void);
void IupControlsClose(void);   /* for backward compatibility only, does nothing since IUP 3 */

#ifdef __cplusplus
}
#endif

#endif
