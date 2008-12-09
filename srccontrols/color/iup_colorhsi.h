/** \file
 * \brief HSI Color Manipulation
 * Copied and adapted from IM
 *
 * See Copyright Notice in "iup.h"
 */

#ifndef __IUP_COLORHSI_H
#define __IUP_COLORHSI_H

#if  defined(__cplusplus)
extern "C" {
#endif

/* 0<=H<=359 */
/* 0<=S<=1 */
/* 0<=I<=1 */


/* Converts from RGB to HSI.
 */
void iupColorRGB2HSI(unsigned char r, unsigned char g, unsigned char b, float *h, float *s, float *i);

/* Converts from HSI to RGB.
 */
void iupColorHSI2RGB(float h, float s, float i, unsigned char *r, unsigned char *g, unsigned char *b);

int iupStrToHSI(const char *str, float *h, float *s, float *i);


#if defined(__cplusplus)
}
#endif

#endif
