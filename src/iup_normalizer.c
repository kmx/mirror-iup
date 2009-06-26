/** \file
 * \brief Normalizer Element.
 *
 * See Copyright Notice in "iup.h"
 */

#include <stdio.h>
#include <stdlib.h>

#include "iup.h"

#include "iup_object.h"
#include "iup_attrib.h"
#include "iup_str.h"
#include "iup_array.h"
#include "iup_stdcontrols.h"


enum {NORMALIZE_NONE, NORMALIZE_WIDTH, NORMALIZE_HEIGHT};

struct _IcontrolData 
{
  Iarray* ih_array;
};

static int iNormalizeGetNormalizeSize(const char* value)
{
  if (!value)
    return NORMALIZE_NONE;
  if (iupStrEqualNoCase(value, "HORIZONTAL"))
    return NORMALIZE_WIDTH;
  if (iupStrEqualNoCase(value, "VERTICAL"))
    return NORMALIZE_HEIGHT;
  if (iupStrEqualNoCase(value, "BOTH"))
    return NORMALIZE_WIDTH|NORMALIZE_HEIGHT;
  return NORMALIZE_NONE;
}

void iupNormalizeSizeBoxChild(Ihandle *ih, int children_natural_maxwidth, int children_natural_maxheight)
{
  Ihandle* child;
  int normalize = iNormalizeGetNormalizeSize(iupAttribGetStr(ih, "NORMALIZESIZE"));
  if (!normalize)
    return;

  /* It is called from Vbox and Hbox ComputeNaturalSizeMethod after the natural size is calculated */

  /* reset the natural width and/or height */
  for (child = ih->firstchild; child; child = child->brother)
  {
    if (!child->floating && (child->iclass->nativetype != IUP_TYPEVOID || !iupStrEqual(child->iclass->name, "fill")))
    {
      if (normalize & NORMALIZE_WIDTH) 
        child->naturalwidth = children_natural_maxwidth;
      if (normalize & NORMALIZE_HEIGHT)
        child->naturalheight = children_natural_maxheight;
    }
  }
}

static int iNormalizerSetNormalizeAttrib(Ihandle* ih, const char* value)
{
  int i, count;
  Ihandle** ih_list;
  Ihandle* ih_control;
  int natural_maxwidth = 0, natural_maxheight = 0;
  int normalize = iNormalizeGetNormalizeSize(value);
  if (!normalize)
    return 1;

  count = iupArrayCount(ih->data->ih_array);
  ih_list = (Ihandle**)iupArrayGetData(ih->data->ih_array);

  for (i = 0; i < count; i++)
  {
    ih_control = ih_list[i];
    iupClassObjectComputeNaturalSize(ih_control);
    natural_maxwidth = iupMAX(natural_maxwidth, ih_control->naturalwidth);
    natural_maxheight = iupMAX(natural_maxheight, ih_control->naturalheight);
  }

  for (i = 0; i < count; i++)
  {
    ih_control = ih_list[i];
    if (!ih_control->floating && (ih_control->iclass->nativetype != IUP_TYPEVOID || !iupStrEqual(ih_control->iclass->name, "fill")))
    {
      if (normalize & NORMALIZE_WIDTH)
        ih_control->userwidth = natural_maxwidth;
      if (normalize & NORMALIZE_HEIGHT)
        ih_control->userheight = natural_maxheight;
    }
  }
  return 1;
}

static int iNormalizerSetAddControlHandleAttrib(Ihandle* ih, const char* value)
{
  Ihandle* ih_control = (Ihandle*)value;
  Ihandle** ih_list = (Ihandle**)iupArrayInc(ih->data->ih_array);
  int count = iupArrayCount(ih->data->ih_array);
  ih_list[count-1] = ih_control;
  return 0;
}

static int iNormalizerSetAddControlAttrib(Ihandle* ih, const char* value)
{
  return iNormalizerSetAddControlHandleAttrib(ih, (char*)IupGetHandle(value));
}

static void iNormalizerComputeNaturalSizeMethod(Ihandle* ih)
{
  ih->naturalwidth = 0;
  ih->naturalheight = 0;

  iNormalizerSetNormalizeAttrib(ih, iupAttribGetStr(ih, "NORMALIZE"));
}

static int iNormalizerCreateMethod(Ihandle* ih, void** params)
{
  ih->data = iupALLOCCTRLDATA();
  ih->data->ih_array = iupArrayCreate(10, sizeof(Ihandle*));

  if (params)
  {
    Ihandle** iparams = (Ihandle**)params;
    Ihandle** ih_list;
    int i = 0;
    while (*iparams) 
    {
      ih_list = (Ihandle**)iupArrayInc(ih->data->ih_array);
      ih_list[i] = *iparams;
      i++;
      iparams++;
    }
  }

  return IUP_NOERROR;
}

static void iNormalizerDestroy(Ihandle* ih)
{
  iupArrayDestroy(ih->data->ih_array);
}

static int iNormalizerMapMethod(Ihandle* ih)
{
  ih->handle = (InativeHandle*)-1; /* fake value just to indicate that it is already mapped */
  return IUP_NOERROR;
}

Iclass* iupNormalizerGetClass(void)
{
  Iclass* ic = iupClassNew(NULL);

  ic->name = "normalizer";
  ic->format = "g"; /* array of Ihandle */
  ic->nativetype = IUP_TYPEVOID;
  ic->childtype = IUP_CHILDNONE;
  ic->is_interactive = 0;

  /* Class functions */
  ic->Create = iNormalizerCreateMethod;
  ic->Map = iNormalizerMapMethod;
  ic->ComputeNaturalSize = iNormalizerComputeNaturalSizeMethod;
  ic->Destroy = iNormalizerDestroy;

  iupClassRegisterAttribute(ic, "NORMALIZE", NULL, iNormalizerSetNormalizeAttrib, IUPAF_SAMEASSYSTEM, "HORIZONTAL", IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "ADDCONTROL_HANDLE", NULL, iNormalizerSetAddControlHandleAttrib, NULL, NULL, IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "ADDCONTROL", NULL, iNormalizerSetAddControlAttrib, NULL, NULL, IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);

  return ic;
}

Ihandle *IupNormalizerv(Ihandle **ih_list)
{
  return IupCreatev("normalizer", (void**)ih_list);
}

Ihandle *IupNormalizer(Ihandle* ih_first, ...)
{
  Ihandle **ih_list;
  Ihandle *ih;

  va_list arglist;
  va_start(arglist, ih_first);
  ih_list = (Ihandle **)iupObjectGetParamList(ih_first, arglist);
  va_end(arglist);

  ih = IupCreatev("normalizer", (void**)ih_list);
  free(ih_list);

  return ih;
}
