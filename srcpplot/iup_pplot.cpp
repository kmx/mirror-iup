/*
 * IupPPlot component
 *
 * Description : A component, derived from PPlot and IUP canvas
 *      Remark : Depend on libs IUP, CD, IUPCD
 */


#ifdef _MSC_VER
#pragma warning(disable: 4100)
#pragma warning(disable: 4512)
#endif

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

#include "iup.h"
#include "iupcbs.h"
#include "iup_pplot.h"
#include "iupkey.h"

#include <cd.h>
#include <cdiup.h>
#include <cddbuf.h>
#include <cdirgb.h>
#include <cdgdiplus.h>

#include "iup_class.h"
#include "iup_register.h"
#include "iup_object.h"
#include "iup_attrib.h"
#include "iup_str.h"
#include "iup_drv.h"
#include "iup_stdcontrols.h"
#include "iup_assert.h"

#include "iupPPlot.h"
#include "iupPPlotInteraction.h"
#include "iuppplot.hpp"


#ifndef M_E
#define M_E 2.71828182846
#endif

struct _IcontrolData
{
  iupCanvas canvas;  /* from IupCanvas (must reserve it) */
  PPainterIup* plt;
};


static int iPPlotGetCDFontStyle(const char* value);


/* PPlot function pointer typedefs. */
typedef int (*IFnC)(Ihandle*, cdCanvas*); /* postdraw_cb, predraw_cb */
typedef int (*IFniiff)(Ihandle*, int, int, float, float); /* delete_cb */
typedef int (*IFniiffi)(Ihandle*, int, int, float, float, int); /* select_cb */
typedef int (*IFniiffff)(Ihandle*, int, int, float, float, float*, float*); /* edit_cb */


/* callback: forward paint request to PPlot object */
static int iPPlotPaint_CB(Ihandle* ih)
{
  ih->data->plt->Draw(0);  /* full redraw only if nothing changed */
  return IUP_DEFAULT;
}

/* callback: forward resize request to PPlot object */
static int iPPlotResize_CB(Ihandle* ih)
{
  ih->data->plt->Resize();
  return IUP_DEFAULT;
}

/* callback: forward mouse button events to PPlot object */
static int iPPlotMouseButton_CB(Ihandle* ih, int btn, int stat, int x, int y, char* r)
{
  ih->data->plt->MouseButton(btn, stat, x, y, r);
  return IUP_DEFAULT;
}

/* callback: forward mouse button events to PPlot object */
static int iPPlotMouseMove_CB(Ihandle* ih, int x, int y)
{
  ih->data->plt->MouseMove(x, y);
  return IUP_DEFAULT;
}

/* callback: forward keyboard events to PPlot object */
static int iPPlotKeyPress_CB(Ihandle* ih, int c, int press)
{
  ih->data->plt->KeyPress(c, press);
  return IUP_DEFAULT;
} 

/* user level call: add dataset to plot */
void IupPPlotBegin(Ihandle* ih, int strXdata)
{
  iupASSERT(iupObjectCheck(ih));
  if (!iupObjectCheck(ih))
    return;

  if (ih->iclass->nativetype != IUP_TYPECANVAS || 
      !iupStrEqual(ih->iclass->name, "pplot"))
    return;

  PlotDataBase* inXData = (PlotDataBase*)iupAttribGetStr(ih, "_IUP_PPLOT_XDATA");
  PlotDataBase* inYData = (PlotDataBase*)iupAttribGetStr(ih, "_IUP_PPLOT_YDATA");

  if (inXData) delete inXData;
  if (inYData) delete inYData;

  if (strXdata)
    inXData = (PlotDataBase*)(new StringData());
  else
    inXData = (PlotDataBase*)(new PlotData());

  inYData = (PlotDataBase*)new PlotData();

  iupAttribSetStr(ih, "_IUP_PPLOT_XDATA",    (char*)inXData);
  iupAttribSetStr(ih, "_IUP_PPLOT_YDATA",    (char*)inYData);
  iupAttribSetStr(ih, "_IUP_PPLOT_STRXDATA", (char*)(strXdata? "1": "0"));
}

void IupPPlotAdd(Ihandle* ih, float x, float y)
{
  iupASSERT(iupObjectCheck(ih));
  if (!iupObjectCheck(ih))
    return;

  if (ih->iclass->nativetype != IUP_TYPECANVAS || 
      !iupStrEqual(ih->iclass->name, "pplot"))
    return;

  PlotData* inXData = (PlotData*)iupAttribGetStr(ih, "_IUP_PPLOT_XDATA");
  PlotData* inYData = (PlotData*)iupAttribGetStr(ih, "_IUP_PPLOT_YDATA");
  int strXdata = iupAttribGetInt(ih, "_IUP_PPLOT_STRXDATA");

  if (!inYData || !inXData || strXdata)
    return;

  inXData->push_back(x);
  inYData->push_back(y);
}

void IupPPlotAddStr(Ihandle* ih, const char* x, float y)
{
  iupASSERT(iupObjectCheck(ih));
  if (!iupObjectCheck(ih))
    return;

  if (ih->iclass->nativetype != IUP_TYPECANVAS || 
      !iupStrEqual(ih->iclass->name, "pplot"))
    return;

  StringData *inXData = (StringData*)iupAttribGetStr(ih, "_IUP_PPLOT_XDATA");
  PlotData   *inYData = (PlotData*)iupAttribGetStr(ih, "_IUP_PPLOT_YDATA");
  int strXdata = iupAttribGetInt(ih, "_IUP_PPLOT_STRXDATA");

  if (!inYData || !inXData || !strXdata)
    return;

  inXData->AddItem(x);
  inYData->push_back(y);
}

void IupPPlotInsertStr(Ihandle* ih, int inIndex, int inSampleIndex, const char* inX, float inY)
{
  iupASSERT(iupObjectCheck(ih));
  if (!iupObjectCheck(ih))
    return;

  if (ih->iclass->nativetype != IUP_TYPECANVAS || 
      !iupStrEqual(ih->iclass->name, "pplot"))
    return;

  PlotDataBase* theXDataBase = ih->data->plt->_plot.mPlotDataContainer.GetXData(inIndex);
  PlotDataBase* theYDataBase = ih->data->plt->_plot.mPlotDataContainer.GetYData(inIndex);
  StringData *theXData = (StringData*)theXDataBase;
  PlotData   *theYData = (PlotData*)theYDataBase;
  if (!theYData || !theXData)
    return;

  theXData->InsertItem(inSampleIndex, inX);
  theYData->insert(theYData->begin()+inSampleIndex, inY);
}

void IupPPlotInsert(Ihandle* ih, int inIndex, int inSampleIndex, float inX, float inY)
{
  iupASSERT(iupObjectCheck(ih));
  if (!iupObjectCheck(ih))
    return;

  if (ih->iclass->nativetype != IUP_TYPECANVAS || 
      !iupStrEqual(ih->iclass->name, "pplot"))
    return;

  PlotDataBase* theXDataBase = ih->data->plt->_plot.mPlotDataContainer.GetXData(inIndex);
  PlotDataBase* theYDataBase = ih->data->plt->_plot.mPlotDataContainer.GetYData(inIndex);
  PlotData* theXData = (PlotData*)theXDataBase;
  PlotData* theYData = (PlotData*)theYDataBase;
  if (!theYData || !theXData)
    return;

  theXData->insert(theXData->begin()+inSampleIndex, inX);
  theYData->insert(theYData->begin()+inSampleIndex, inY);
}

int IupPPlotEnd(Ihandle* ih)
{
  iupASSERT(iupObjectCheck(ih));
  if (!iupObjectCheck(ih))
    return -1;

  if (ih->iclass->nativetype != IUP_TYPECANVAS || 
      !iupStrEqual(ih->iclass->name, "pplot"))
    return -1;

  PlotDataBase* inXData = (PlotDataBase*)iupAttribGetStr(ih, "_IUP_PPLOT_XDATA");
  PlotDataBase* inYData = (PlotDataBase*)iupAttribGetStr(ih, "_IUP_PPLOT_YDATA");
  if (!inYData || !inXData)
    return -1;

  /* add to plot */
  ih->data->plt->_currentDataSetIndex = ih->data->plt->_plot.mPlotDataContainer.AddXYPlot(inXData, inYData);

  LegendData* legend = ih->data->plt->_plot.mPlotDataContainer.GetLegendData(ih->data->plt->_currentDataSetIndex);
  legend->mStyle.mFontStyle = iPPlotGetCDFontStyle(IupGetAttribute(ih, "LEGENDFONTSTYLE"));
  legend->mStyle.mFontSize  = IupGetInt(ih, "LEGENDFONTSIZE");

  iupAttribSetStr(ih, "_IUP_PPLOT_XDATA", NULL);
  iupAttribSetStr(ih, "_IUP_PPLOT_YDATA", NULL);

  ih->data->plt->_redraw = 1;
  return ih->data->plt->_currentDataSetIndex;
}

void IupPPlotTransform(Ihandle* ih, float x, float y, int *ix, int *iy)
{
  iupASSERT(iupObjectCheck(ih));
  if (!iupObjectCheck(ih))
    return;

  if (ih->iclass->nativetype != IUP_TYPECANVAS || 
      !iupStrEqual(ih->iclass->name, "pplot"))
    return;

  if (ix) *ix = ih->data->plt->_plot.Round(ih->data->plt->_plot.mXTrafo->Transform(x));
  if (iy) *iy = ih->data->plt->_plot.Round(ih->data->plt->_plot.mYTrafo->Transform(y));
}

/* user level call: plot on the given device */
void IupPPlotPaintTo(Ihandle* ih, void* _cnv)
{
  iupASSERT(iupObjectCheck(ih));
  if (!iupObjectCheck(ih))
    return;

  if (ih->iclass->nativetype != IUP_TYPECANVAS || 
      !iupStrEqual(ih->iclass->name, "pplot"))
    return;

  ih->data->plt->DrawTo((cdCanvas *)_cnv);
}

/* --------------------------------------------------------------------
                      class implementation
   -------------------------------------------------------------------- */

PostPainterCallbackIup::PostPainterCallbackIup (PPlot &inPPlot, Ihandle* inHandle):
  _ih(inHandle)
{
  inPPlot.mPostDrawerList.push_back (this);
}

bool PostPainterCallbackIup::Draw(Painter &inPainter)
{
  IFnC cb = (IFnC)IupGetCallback(_ih, "POSTDRAW_CB");

  if (cb)
  {
    PPainterIup* iupPainter = (PPainterIup*)(&inPainter);
    cb(_ih, iupPainter->_cddbuffer);
  }

  return true;
}

PrePainterCallbackIup::PrePainterCallbackIup (PPlot &inPPlot, Ihandle* inHandle):
  _ih(inHandle)
{
  inPPlot.mPreDrawerList.push_back (this);
}

bool PrePainterCallbackIup::Draw(Painter &inPainter)
{
  IFnC cb = (IFnC)IupGetCallback(_ih, "PREDRAW_CB");
  if (cb)
  {
    PPainterIup* iupPainter = (PPainterIup*)(&inPainter);
    cb(_ih, iupPainter->_cddbuffer);
  }

  return true;
}

bool PDeleteInteractionIup::DeleteNotify(int inIndex, int inSampleIndex, PlotDataBase* inXData, PlotDataBase* inYData)
{
  IFniiff cb = (IFniiff)IupGetCallback(_ih, "DELETE_CB");
  if (cb)
  {
    if (inIndex == -1)
    {
      Icallback cbb = IupGetCallback(_ih, "DELETEBEGIN_CB");
      if (cbb && cbb(_ih) == IUP_IGNORE)
        return false;
    }
    else if (inIndex == -2)
    {
      Icallback cbb = IupGetCallback(_ih, "DELETEEND_CB");
      if (cbb)
        cbb(_ih);
    }
    else
    {
      float theX = inXData->GetValue(inSampleIndex);
      float theY = inYData->GetValue(inSampleIndex);
      int ret = cb(_ih, inIndex, inSampleIndex, theX, theY);
      if (ret == IUP_IGNORE)
        return false;
    }
  }

  return true;
}

bool PSelectionInteractionIup::SelectNotify(int inIndex, int inSampleIndex, PlotDataBase* inXData, PlotDataBase* inYData, bool inSelect)
{
  IFniiffi cb = (IFniiffi)IupGetCallback(_ih, "SELECT_CB");
  if (cb)
  {
    if (inIndex == -1)
    {
      Icallback cbb = IupGetCallback(_ih, "SELECTBEGIN_CB");
      if (cbb && cbb(_ih) == IUP_IGNORE)
        return false;
    }
    else if (inIndex == -2)
    {
      Icallback cbb = IupGetCallback(_ih, "SELECTEND_CB");
      if (cbb)
        cbb(_ih);
    }
    else
    {
      float theX = inXData->GetValue(inSampleIndex);
      float theY = inYData->GetValue(inSampleIndex);
      int ret = cb(_ih, inIndex, inSampleIndex, theX, theY, (int)inSelect);
      if (ret == IUP_IGNORE)
        return false;
    }
  }

  return true;
}

bool PEditInteractionIup::Impl_HandleKeyEvent (const PKeyEvent &inEvent)
{
  if (inEvent.IsArrowDown () || inEvent.IsArrowUp () ||
      inEvent.IsArrowLeft () || inEvent.IsArrowRight ())
    return true;

  return false;
};

bool PEditInteractionIup::Impl_Calculate (Painter &inPainter, PPlot& inPPlot)
{
  PlotDataContainer &theContainer = inPPlot.mPlotDataContainer;
  long thePlotCount = theContainer.GetPlotCount();

  if (!EditNotify(-1, 0, 0, 0, NULL, NULL))
    return false;

  for (long theI=0;theI<thePlotCount;theI++)
  {
    PlotDataBase* theXData = theContainer.GetXData (theI);
    PlotDataBase* theYData = theContainer.GetYData (theI);
    PlotDataSelection *thePlotDataSelection = theContainer.GetPlotDataSelection (theI);

    if (mKeyEvent.IsArrowDown () || mKeyEvent.IsArrowUp () ||
        mKeyEvent.IsArrowLeft () || mKeyEvent.IsArrowRight ())
      HandleCursorKey (thePlotDataSelection, theXData, theYData, theI);
  }

  EditNotify(-2, 0, 0, 0, NULL, NULL);

  return true;
}

void PEditInteractionIup::HandleCursorKey (const PlotDataSelection *inPlotDataSelection, PlotDataBase* inXData, PlotDataBase* inYData, int inIndex)
{
  float theXDelta = 0; // pixels
  if (mKeyEvent.IsArrowLeft () || mKeyEvent.IsArrowRight ())
  {
    theXDelta = 1;

    if (mKeyEvent.IsArrowLeft ())
      theXDelta *= -1;

    if (mKeyEvent.IsOnlyControlKeyDown ())
      theXDelta *= 10;
  }

  float theYDelta = 0; // pixels
  if (mKeyEvent.IsArrowDown () || mKeyEvent.IsArrowUp ())
  {
    theYDelta = 1;

    if (mKeyEvent.IsArrowDown ())
      theYDelta *= -1;

    if (mKeyEvent.IsOnlyControlKeyDown ())
      theYDelta *= 10;
  }

  for (int theI=0;theI<inYData->GetSize ();theI++)
  {
    if (inPlotDataSelection->IsSelected (theI))
    {
      float theX = inXData->GetValue(theI);
      float newX = theX;

      if (theXDelta)
      {
        float theXPixels = mPPlot.mXTrafo->Transform(theX);
        theXPixels += theXDelta;
        newX = mPPlot.mXTrafo->TransformBack(theXPixels);
      }

      float theY = inYData->GetValue(theI);
      float newY = theY;
      if (theYDelta)
      {
        float theYPixels = mPPlot.mYTrafo->Transform(theY);
        theYPixels -= theYDelta;  // in pixels Y is descending
        newY = mPPlot.mYTrafo->TransformBack(theYPixels);
      }

      if (!EditNotify(inIndex, theI, theX, theY, &newX, &newY))
        return;

      if (inXData->IsString())
      {
        StringData *theXData = (StringData*)(inXData);
        PlotData* theYData = (PlotData*)(inYData);
        theXData->mRealPlotData[theI] = newX;
        (*theYData)[theI] = newY;
      }
      else
      {
        PlotData* theXData = (PlotData*)(inXData);
        PlotData* theYData = (PlotData*)(inYData);
        (*theXData)[theI] = newX;
        (*theYData)[theI] = newY;
      }
    }
  }
}

bool PEditInteractionIup::EditNotify(int inIndex, int inSampleIndex, float inX, float inY, float *inNewX, float *inNewY)
{
  IFniiffff cb = (IFniiffff)IupGetCallback(_ih, "EDIT_CB");
  if (cb)
  {
    if (inIndex == -1)
    {
      Icallback cbb = IupGetCallback(_ih, "EDITBEGIN_CB");
      if (cbb && cbb(_ih) == IUP_IGNORE)
        return false;
    }
    else if (inIndex == -2)
    {
      Icallback cbb = IupGetCallback(_ih, "EDITEND_CB");
      if (cbb)
        cbb(_ih);
    }
    else
    {
      int ret = cb(_ih, inIndex, inSampleIndex, inX, inY, inNewX, inNewY);
      if (ret == IUP_IGNORE)
        return false;
    }
  }

  return true;
}

InteractionContainerIup::InteractionContainerIup(PPlot &inPPlot, Ihandle* inHandle):
  mZoomInteraction (inPPlot),
  mSelectionInteraction (inPPlot, inHandle),
  mEditInteraction (inPPlot, inHandle),
  mDeleteInteraction (inPPlot, inHandle),
  mCrosshairInteraction (inPPlot),
  mPostPainterCallback(inPPlot, inHandle),
  mPrePainterCallback(inPPlot, inHandle)
{
  AddInteraction (mZoomInteraction);
  AddInteraction (mSelectionInteraction);
  AddInteraction (mEditInteraction);
  AddInteraction (mDeleteInteraction);
  AddInteraction (mCrosshairInteraction);
}

PPainterIup::PPainterIup(Ihandle *ih) : 
  Painter(),
  _ih(ih),
  _cdcanvas(NULL),
  _cddbuffer(NULL),
  _mouseDown(0),
  _currentDataSetIndex(-1),
  _redraw(1)
{
  _plot.mShowLegend = false; // change default to hidden
  _plot.mPlotBackground.mTransparent = false;  // always draw the background
  _plot.mMargins.mLeft = 15;
  _plot.mMargins.mBottom = 15;
  _plot.mMargins.mTop = 30;
  _plot.mMargins.mRight = 15;
  _plot.mXAxisSetup.mTickInfo.mTickDivision = 5;
  _plot.mYAxisSetup.mTickInfo.mTickDivision = 5;
  _plot.mXAxisSetup.mTickInfo.mMinorTickScreenSize = 5;
  _plot.mYAxisSetup.mTickInfo.mMinorTickScreenSize = 5;
  _plot.mXAxisSetup.mTickInfo.mMajorTickScreenSize = 8;
  _plot.mYAxisSetup.mTickInfo.mMajorTickScreenSize = 8;

  _InteractionContainer = new InteractionContainerIup(_plot, _ih);

} /* c-tor */


PPainterIup::~PPainterIup()
{
  if (_cddbuffer != NULL)
    cdKillCanvas(_cddbuffer);

  delete _InteractionContainer;
} /* d-tor */

class MarkDataDrawer: public LineDataDrawer
{
 public:
   MarkDataDrawer (bool inDrawLine)
   {
     mDrawLine = inDrawLine;
     mDrawPoint = true;
     mMode = inDrawLine ? "MARKLINE" : "MARK";
   };
   virtual bool DrawPoint (int inScreenX, int inScreenY, const PRect &inRect, Painter &inPainter) const;
};

bool MarkDataDrawer::DrawPoint (int inScreenX, int inScreenY, const PRect &inRect, Painter &inPainter) const
{
  PPainterIup* painter = (PPainterIup*)&inPainter;
  cdCanvasMark(painter->_cddbuffer, inScreenX, cdCanvasInvertYAxis(painter->_cddbuffer, inScreenY));

  return true;
}

static void RemoveSample(PPlot& inPPlot, int inIndex, int inSampleIndex)
{
  PlotDataBase* theXDataBase = inPPlot.mPlotDataContainer.GetXData(inIndex);
  PlotDataBase* theYDataBase = inPPlot.mPlotDataContainer.GetYData(inIndex);

  if (theXDataBase->IsString())
  {
    StringData *theXData = (StringData *)theXDataBase;
    PlotData* theYData = (PlotData*)theYDataBase;
    theXData->mRealPlotData.erase(theXData->mRealPlotData.begin()+inSampleIndex);
    theXData->mStringData.erase(theXData->mStringData.begin()+inSampleIndex);
    theYData->erase(theYData->begin()+inSampleIndex);
  }
  else
  {
    PlotData* theXData = (PlotData*)theXDataBase;
    PlotData* theYData = (PlotData*)theYDataBase;
    theXData->erase(theXData->begin()+inSampleIndex);
    theYData->erase(theYData->begin()+inSampleIndex);
  }
}

/* --------------------------------------------------------------------
                      CD Gets - size and style
   -------------------------------------------------------------------- */

static int iPPlotGetCDFontStyle(const char* value)
{
  if (!value)
    return -1;
  if (iupStrEqualNoCase(value, "PLAIN"))
    return CD_PLAIN;
  if (iupStrEqualNoCase(value, "BOLD"))
    return CD_BOLD;
  if (iupStrEqualNoCase(value, "ITALIC"))
    return CD_ITALIC;
  if (iupStrEqualNoCase(value, "BOLDITALIC"))
    return CD_BOLD_ITALIC;
  return -1;
}

static char* iPPlotGetPlotFontSize(int size)
{
  if (size)
  {
    char* buffer = iupStrGetMemory(50);
    sprintf(buffer, "%d", size);
    return buffer;
  }
  else
    return NULL;
}

static char* iPPlotGetPlotFontStyle(int style)
{
  if (style >= CD_PLAIN && style <= CD_BOLD_ITALIC)
  {
    char* style_str[4] = {"PLAIN", "BOLD", "ITALIC", "BOLDITALIC"};
    return style_str[style];
  }
  else
    return NULL;
}

static char* iPPlotGetPlotPenStyle(int style)
{
  if (style >= CD_CONTINUOUS && style <= CD_DASH_DOT_DOT)
  {
    char* style_str[5] = {"CONTINUOUS", "DASHED", "DOTTED", "DASH_DOT", "DASH_DOT_DOT"};
    return style_str[style];
  }
  else
    return NULL;
}

static int iPPlotGetCDPenStyle(const char* value)
{
  if (!value || iupStrEqualNoCase(value, "CONTINUOUS"))
    return CD_CONTINUOUS;
  else if (iupStrEqualNoCase(value, "DASHED"))
    return CD_DASHED;
  else if (iupStrEqualNoCase(value, "DOTTED"))
    return CD_DOTTED;
  else if (iupStrEqualNoCase(value, "DASH_DOT"))
    return CD_DASH_DOT;
  else if (iupStrEqualNoCase(value, "DASH_DOT_DOT"))
    return CD_DASH_DOT_DOT;
  else
    return CD_CONTINUOUS;
}
 
static char* iPPlotGetPlotMarkStyle(int style)
{
  if (style >= CD_PLUS && style <= CD_HOLLOW_DIAMOND)
  {
    char* style_str[9] = {"PLUS", "STAR", "CIRCLE", "X", "BOX", "DIAMOND", "HOLLOW_CIRCLE", "HOLLOW_BOX", "HOLLOW_DIAMOND"};
    return style_str[style];
  }
  else
    return NULL;
}

static int iPPlotGetCDMarkStyle(const char* value)
{
  if (!value || iupStrEqualNoCase(value, "PLUS"))
    return CD_PLUS;
  else if (iupStrEqualNoCase(value, "STAR"))
    return CD_STAR;
  else if (iupStrEqualNoCase(value, "CIRCLE"))
    return CD_CIRCLE;
  else if (iupStrEqualNoCase(value, "X"))
    return CD_X;
  else if (iupStrEqualNoCase(value, "BOX"))
    return CD_BOX;
  else if (iupStrEqualNoCase(value, "DIAMOND"))
    return CD_DIAMOND;
  else if (iupStrEqualNoCase(value, "HOLLOW_CIRCLE"))
    return CD_HOLLOW_CIRCLE;
  else if (iupStrEqualNoCase(value, "HOLLOW_BOX"))
    return CD_HOLLOW_BOX;
  else if (iupStrEqualNoCase(value, "HOLLOW_DIAMOND"))
    return CD_HOLLOW_DIAMOND;
  else
    return CD_PLUS;
}

/*****************************************************************************/
/***** SET AND GET ATTRIBUTES ************************************************/
/*****************************************************************************/

/* refresh plot window (write only) */
static int iPPlotSetRedrawAttrib(Ihandle* ih, const char* value)
{
  (void)value;  /* not used */
  ih->data->plt->Draw(1);   /* force a full redraw here */
  return 0;
}

/* total number of datasets (read only) */
static char* iPPlotGetCountAttrib(Ihandle* ih)
{
  char* att_buffer = iupStrGetMemory(30);
  sprintf(att_buffer, "%d", ih->data->plt->_plot.mPlotDataContainer.GetPlotCount());
  return att_buffer;
}

/* legend box visibility */
static int iPPlotSetLegendShowAttrib(Ihandle* ih, const char* value)
{
  if (iupStrEqualNoCase(value, "YES") || iupStrEqualNoCase(value, "ON"))
    ih->data->plt->_plot.mShowLegend = true;
  else 
    ih->data->plt->_plot.mShowLegend = false;

  ih->data->plt->_redraw = 1;
  return 0;
}

static char* iPPlotGetLegendShowAttrib(Ihandle* ih)
{
  if (ih->data->plt->_plot.mShowLegend)
    return "YES";
  else
    return "NO";
}

/* legend box visibility */
static int iPPlotSetLegendPosAttrib(Ihandle* ih, const char* value)
{
  if (iupStrEqualNoCase(value, "TOPLEFT"))
    ih->data->plt->_plot.mLegendPos = PPLOT_TOPLEFT;
  if (iupStrEqualNoCase(value, "BOTTOMLEFT"))
    ih->data->plt->_plot.mLegendPos = PPLOT_BOTTOMLEFT;
  if (iupStrEqualNoCase(value, "BOTTOMRIGHT"))
    ih->data->plt->_plot.mLegendPos = PPLOT_BOTTOMRIGHT;
  else
    ih->data->plt->_plot.mLegendPos = PPLOT_TOPRIGHT;

  ih->data->plt->_redraw = 1;
  return 0;
}

static char* iPPlotGetLegendPosAttrib(Ihandle* ih)
{
  char* legendpos_str[4] = {"TOPLEFT", "TOPRIGHT", "BOTTOMLEFT", "BOTTOMRIGHT"};

  return legendpos_str[ih->data->plt->_plot.mLegendPos];
}

/* background color */
static int iPPlotSetBGColorAttrib(Ihandle* ih, const char* value)
{
  unsigned char rr, gg, bb;
  if (iupStrToRGB(value, &rr, &gg, &bb))
  {
    ih->data->plt->_plot.mPlotBackground.mPlotRegionBackColor = PColor(rr, gg, bb);
    ih->data->plt->_redraw = 1;
  }
  return 0;
}

static char* iPPlotGetBGColorAttrib(Ihandle* ih)
{
  char* att_buffer = iupStrGetMemory(30);
  sprintf(att_buffer, "%d %d %d",
          ih->data->plt->_plot.mPlotBackground.mPlotRegionBackColor.mR,
          ih->data->plt->_plot.mPlotBackground.mPlotRegionBackColor.mG,
          ih->data->plt->_plot.mPlotBackground.mPlotRegionBackColor.mB);
  return att_buffer;
}


/* title color */
static int iPPlotSetFGColorAttrib(Ihandle* ih, const char* value)
{
  unsigned char rr, gg, bb;
  if (iupStrToRGB(value, &rr, &gg, &bb))
  {
    ih->data->plt->_plot.mPlotBackground.mTitleColor = PColor(rr, gg, bb);
    ih->data->plt->_redraw = 1;
  }
  return 0;
}

static char* iPPlotGetFGColorAttrib(Ihandle* ih)
{
  char* att_buffer = iupStrGetMemory(30);
  sprintf(att_buffer, "%d %d %d",
          ih->data->plt->_plot.mPlotBackground.mTitleColor.mR,
          ih->data->plt->_plot.mPlotBackground.mTitleColor.mG,
          ih->data->plt->_plot.mPlotBackground.mTitleColor.mB);
  return att_buffer;
}


/* plot title */
static int iPPlotSetTitleAttrib(Ihandle* ih, const char* value)
{
  if (value && value[0] != 0)
    ih->data->plt->_plot.mPlotBackground.mTitle = value;
  else
    ih->data->plt->_plot.mPlotBackground.mTitle.resize(0);

  ih->data->plt->_redraw = 1;
  return 0;
}

static char* iPPlotGetTitleAttrib(Ihandle* ih)
{
  char* att_buffer = iupStrGetMemory(256);
  strncpy(att_buffer, ih->data->plt->_plot.mPlotBackground.mTitle.c_str(), 256);
  att_buffer[255]='\0';
  return att_buffer;
}


/* plot title font size */
static int iPPlotSetTitleFontSizeAttrib(Ihandle* ih, const char* value)
{
  int ii;
  if (iupStrToInt(value, &ii))
  {
    ih->data->plt->_plot.mPlotBackground.mStyle.mFontSize = ii;
    ih->data->plt->_redraw = 1;
  }
  return 0;
}

static char* iPPlotGetTitleFontSizeAttrib(Ihandle* ih)
{
  return iPPlotGetPlotFontSize(ih->data->plt->_plot.mPlotBackground.mStyle.mFontSize);
}


/* plot title font style */
static int iPPlotSetTitleFontStyleAttrib(Ihandle* ih, const char* value)
{
  int style = iPPlotGetCDFontStyle(value);
  if (style != -1)
  {
    ih->data->plt->_plot.mPlotBackground.mStyle.mFontStyle = style;
    ih->data->plt->_redraw = 1;
  }
  return 0;
}

/* legend font size */
static int iPPlotSetLegendFontSizeAttrib(Ihandle* ih, const char* value)
{
  int ii, xx;
  if (!iupStrToInt(value, &xx))
    return 0;

  for (ii = 0; ii < ih->data->plt->_plot.mPlotDataContainer.GetPlotCount(); ii++)
  {
    LegendData* legend = ih->data->plt->_plot.mPlotDataContainer.GetLegendData(ii);
    legend->mStyle.mFontSize = xx;
  }

  ih->data->plt->_redraw = 1;
  return 1;
}

/* legend font style */
static int iPPlotSetLegendFontStyleAttrib(Ihandle* ih, const char* value)
{
  int ii;
  int style = iPPlotGetCDFontStyle(value);
  if (style == -1)
    return 0;

  for (ii = 0; ii < ih->data->plt->_plot.mPlotDataContainer.GetPlotCount(); ii++)
  {
    LegendData* legend = ih->data->plt->_plot.mPlotDataContainer.GetLegendData(ii);
    legend->mStyle.mFontStyle = style;
  }

  ih->data->plt->_redraw = 1;
  return 1;
}

/* plot margins */
static int iPPlotSetMarginLeftAttrib(Ihandle* ih, const char* value)
{
  int ii;
  if (iupStrToInt(value, &ii))
  {
    ih->data->plt->_plot.mMargins.mLeft = ii;
    ih->data->plt->_redraw = 1;
  }
  return 0;
}

static int iPPlotSetMarginRightAttrib(Ihandle* ih, const char* value)
{
  int ii;
  if (iupStrToInt(value, &ii))
  {
    ih->data->plt->_plot.mMargins.mRight = ii;
    ih->data->plt->_redraw = 1;
  }
  return 0;
}

static int iPPlotSetMarginTopAttrib(Ihandle* ih, const char* value)
{
  int ii;
  if (iupStrToInt(value, &ii))
  {
    ih->data->plt->_plot.mMargins.mTop = ii;
    ih->data->plt->_redraw = 1;
  }
  return 0;
}

static int iPPlotSetMarginBottomAttrib(Ihandle* ih, const char* value)
{
  int ii;
  if (iupStrToInt(value, &ii))
  {
    ih->data->plt->_plot.mMargins.mBottom = ii;
    ih->data->plt->_redraw = 1;
  }
  return 0;
}

static char* iPPlotGetMarginLeftAttrib(Ihandle* ih)
{
  char* att_buffer = iupStrGetMemory(30);
  sprintf(att_buffer, "%d", ih->data->plt->_plot.mMargins.mLeft);
  return att_buffer;
}

static char* iPPlotGetMarginRightAttrib(Ihandle* ih)
{
  char* att_buffer = iupStrGetMemory(30);
  sprintf(att_buffer, "%d", ih->data->plt->_plot.mMargins.mRight);
  return att_buffer;
}

static char* iPPlotGetMarginTopAttrib(Ihandle* ih)
{
  char* att_buffer = iupStrGetMemory(30);
  sprintf(att_buffer, "%d", ih->data->plt->_plot.mMargins.mTop);
  return att_buffer;
}

static char* iPPlotGetMarginBottomAttrib(Ihandle* ih)
{
  char* att_buffer = iupStrGetMemory(30);
  sprintf(att_buffer, "%d", ih->data->plt->_plot.mMargins.mBottom);
  return att_buffer;
}

/* plot grid color */
static int iPPlotSetGridColorAttrib(Ihandle* ih, const char* value)
{
  unsigned char rr, gg, bb;
  if (iupStrToRGB(value, &rr, &gg, &bb))
  {
    ih->data->plt->_plot.mGridInfo.mGridColor = PColor(rr, gg, bb);
    ih->data->plt->_redraw = 1;
  }
  return 0;
}

static char* iPPlotGetGridColorAttrib(Ihandle* ih)
{
  char* att_buffer = iupStrGetMemory(30);
  sprintf(att_buffer, "%d %d %d",
          ih->data->plt->_plot.mGridInfo.mGridColor.mR,
          ih->data->plt->_plot.mGridInfo.mGridColor.mG,
          ih->data->plt->_plot.mGridInfo.mGridColor.mB);
  return att_buffer;
}

/* plot grid line style */
static int iPPlotSetGridLineStyleAttrib(Ihandle* ih, const char* value)
{
  ih->data->plt->_plot.mGridInfo.mStyle.mPenStyle = iPPlotGetCDPenStyle(value);
  ih->data->plt->_redraw = 1;
  return 0;
}

static char* iPPlotGetGridLineStyleAttrib(Ihandle* ih)
{
  return iPPlotGetPlotPenStyle(ih->data->plt->_plot.mGridInfo.mStyle.mPenStyle);
}

/* grid */
static int iPPlotSetGridAttrib(Ihandle* ih, const char* value)
{
  if (iupStrEqualNoCase(value, "VERTICAL"))  /* vertical grid - X axis  */
  {
    ih->data->plt->_plot.mGridInfo.mXGridOn = true;
    ih->data->plt->_plot.mGridInfo.mYGridOn = false;
  }
  else if (iupStrEqualNoCase(value, "HORIZONTAL")) /* horizontal grid - Y axis */
  {
    ih->data->plt->_plot.mGridInfo.mYGridOn = true;
    ih->data->plt->_plot.mGridInfo.mXGridOn = false;
  }
  else if (iupStrEqualNoCase(value, "YES"))
  {
    ih->data->plt->_plot.mGridInfo.mXGridOn = true;
    ih->data->plt->_plot.mGridInfo.mYGridOn = true;
  }
  else
  {
    ih->data->plt->_plot.mGridInfo.mYGridOn = false;
    ih->data->plt->_plot.mGridInfo.mXGridOn = false;
  }

  ih->data->plt->_redraw = 1;
  return 0;
}

static char* iPPlotGetGridAttrib(Ihandle* ih)
{
  if (ih->data->plt->_plot.mGridInfo.mXGridOn && ih->data->plt->_plot.mGridInfo.mYGridOn)
    return "YES";
  else if (ih->data->plt->_plot.mGridInfo.mYGridOn)
    return "HORIZONTAL";
  else if (ih->data->plt->_plot.mGridInfo.mXGridOn)
    return "VERTICAL";
  else
    return "NO";
}

/* current dataset index */
static int iPPlotSetCurrentAttrib(Ihandle* ih, const char* value)
{
  int ii;
  if (iupStrToInt(value, &ii))
  {
    int imax = ih->data->plt->_plot.mPlotDataContainer.GetPlotCount();
    ih->data->plt->_currentDataSetIndex = ( (ii>=0) && (ii<imax) ? ii : -1);
    ih->data->plt->_redraw = 1;
  }
  return 0;
}

static char* iPPlotGetCurrentAttrib(Ihandle* ih)
{
  char* att_buffer = iupStrGetMemory(30);
  sprintf(att_buffer, "%d", ih->data->plt->_currentDataSetIndex);
  return att_buffer;
}

/* remove a dataset */
static int iPPlotSetRemoveAttrib(Ihandle* ih, const char* value)
{
  int ii;
  if (iupStrToInt(value, &ii))
  {
    ih->data->plt->_plot.mPlotDataContainer.RemoveElement(ii);
    ih->data->plt->_redraw = 1;
  }
  return 0;
}

/* clear all datasets */
static int iPPlotSetClearAttrib(Ihandle* ih, const char* value)
{
  ih->data->plt->_plot.mPlotDataContainer.ClearData();
  ih->data->plt->_redraw = 1;
  return 0;
}

/* =============================== */
/* current plot dataset attributes */
/* =============================== */

/* current plot line style */
static int iPPlotSetDSLineStyleAttrib(Ihandle* ih, const char* value)
{
  if (ih->data->plt->_currentDataSetIndex <  0 ||
      ih->data->plt->_currentDataSetIndex >= ih->data->plt->_plot.mPlotDataContainer.GetPlotCount())
    return 0;
  
  DataDrawerBase* drawer = ih->data->plt->_plot.mPlotDataContainer.GetDataDrawer(ih->data->plt->_currentDataSetIndex);
  drawer->mStyle.mPenStyle = iPPlotGetCDPenStyle(value);

  ih->data->plt->_redraw = 1;
  return 0;
}

static char* iPPlotGetDSLineStyleAttrib(Ihandle* ih)
{
  if (ih->data->plt->_currentDataSetIndex < 0 ||
      ih->data->plt->_currentDataSetIndex >= ih->data->plt->_plot.mPlotDataContainer.GetPlotCount())
    return NULL;

  DataDrawerBase* drawer = ih->data->plt->_plot.mPlotDataContainer.GetDataDrawer(ih->data->plt->_currentDataSetIndex);

  return iPPlotGetPlotPenStyle(drawer->mStyle.mPenStyle);
}

/* current plot line width */
static int iPPlotSetDSLineWidthAttrib(Ihandle* ih, const char* value)
{
  int ii;

  if (ih->data->plt->_currentDataSetIndex <  0 ||
      ih->data->plt->_currentDataSetIndex >= ih->data->plt->_plot.mPlotDataContainer.GetPlotCount())
    return 0;

  if (iupStrToInt(value, &ii))
  {
    DataDrawerBase* drawer = ih->data->plt->_plot.mPlotDataContainer.GetDataDrawer(ih->data->plt->_currentDataSetIndex);
    drawer->mStyle.mPenWidth = ii;
    ih->data->plt->_redraw = 1;
  }
  return 0;
}

static char* iPPlotGetDSLineWidthAttrib(Ihandle* ih)
{
  if (ih->data->plt->_currentDataSetIndex < 0 ||
      ih->data->plt->_currentDataSetIndex >= ih->data->plt->_plot.mPlotDataContainer.GetPlotCount())
    return NULL;

  DataDrawerBase* drawer = ih->data->plt->_plot.mPlotDataContainer.GetDataDrawer(ih->data->plt->_currentDataSetIndex);
  char* att_buffer = iupStrGetMemory(30);
  sprintf(att_buffer, "%d", drawer->mStyle.mPenWidth);
  return att_buffer;
}

/* current plot mark style */
static int iPPlotSetDSMarkStyleAttrib(Ihandle* ih, const char* value)
{
  if (ih->data->plt->_currentDataSetIndex <  0 ||
      ih->data->plt->_currentDataSetIndex >= ih->data->plt->_plot.mPlotDataContainer.GetPlotCount())
    return 0;
  
  DataDrawerBase* drawer = ih->data->plt->_plot.mPlotDataContainer.GetDataDrawer(ih->data->plt->_currentDataSetIndex);
  drawer->mStyle.mMarkStyle = iPPlotGetCDMarkStyle(value);
  ih->data->plt->_redraw = 1;
  return 0;
}

static char* iPPlotGetDSMarkStyleAttrib(Ihandle* ih)
{
  if (ih->data->plt->_currentDataSetIndex < 0 ||
      ih->data->plt->_currentDataSetIndex >= ih->data->plt->_plot.mPlotDataContainer.GetPlotCount())
    return NULL;

  DataDrawerBase* drawer = ih->data->plt->_plot.mPlotDataContainer.GetDataDrawer(ih->data->plt->_currentDataSetIndex);

  return iPPlotGetPlotMarkStyle(drawer->mStyle.mMarkStyle);
}

/* current plot mark size */
static int iPPlotSetDSMarkSizeAttrib(Ihandle* ih, const char* value)
{
  int ii;

  if (ih->data->plt->_currentDataSetIndex <  0 ||
      ih->data->plt->_currentDataSetIndex >= ih->data->plt->_plot.mPlotDataContainer.GetPlotCount())
    return 0;
  
  if (iupStrToInt(value, &ii))
  {
    DataDrawerBase* drawer = ih->data->plt->_plot.mPlotDataContainer.GetDataDrawer(ih->data->plt->_currentDataSetIndex);
    drawer->mStyle.mMarkSize = ii;
    ih->data->plt->_redraw = 1;
  }
  return 0;
}

static char* iPPlotGetDSMarkSizeAttrib(Ihandle* ih)
{
  if (ih->data->plt->_currentDataSetIndex < 0 ||
      ih->data->plt->_currentDataSetIndex >= ih->data->plt->_plot.mPlotDataContainer.GetPlotCount())
    return NULL;

  DataDrawerBase* drawer = ih->data->plt->_plot.mPlotDataContainer.GetDataDrawer(ih->data->plt->_currentDataSetIndex);
  char* att_buffer = iupStrGetMemory(30);
  sprintf(att_buffer, "%d", drawer->mStyle.mMarkSize);
  return att_buffer;
}

/* current dataset legend */
static int iPPlotSetDSLegendAttrib(Ihandle* ih, const char* value)
{
  if (ih->data->plt->_currentDataSetIndex <  0 ||
      ih->data->plt->_currentDataSetIndex >= ih->data->plt->_plot.mPlotDataContainer.GetPlotCount())
    return 0;
  
  LegendData* legend = ih->data->plt->_plot.mPlotDataContainer.GetLegendData(ih->data->plt->_currentDataSetIndex);
    
  if (value)
    legend->mName = value;
  else
    legend->mName.resize(0);

  ih->data->plt->_redraw = 1;
  return 0;
}

static char* iPPlotGetDSLegendAttrib(Ihandle* ih)
{
  if (ih->data->plt->_currentDataSetIndex < 0 ||
      ih->data->plt->_currentDataSetIndex >= ih->data->plt->_plot.mPlotDataContainer.GetPlotCount())
    return NULL;

  LegendData* legend = ih->data->plt->_plot.mPlotDataContainer.GetLegendData(ih->data->plt->_currentDataSetIndex);
  char* att_buffer = iupStrGetMemory(256);
  strncpy(att_buffer, legend->mName.c_str(), 255);
  att_buffer[255]='\0';
  return att_buffer;
}

/* current dataset line and legend color */
static int iPPlotSetDSColorAttrib(Ihandle* ih, const char* value)
{
  unsigned char rr, gg, bb;

  if (ih->data->plt->_currentDataSetIndex <  0 ||
      ih->data->plt->_currentDataSetIndex >= ih->data->plt->_plot.mPlotDataContainer.GetPlotCount())
    return 0;

  if (iupStrToRGB(value, &rr, &gg, &bb))
  {
    LegendData* legend = ih->data->plt->_plot.mPlotDataContainer.GetLegendData(ih->data->plt->_currentDataSetIndex);
    legend->mColor = PColor(rr, gg, bb);
    ih->data->plt->_redraw = 1;
  }
  return 0;
}

static char* iPPlotGetDSColorAttrib(Ihandle* ih)
{
  if (ih->data->plt->_currentDataSetIndex < 0 ||
      ih->data->plt->_currentDataSetIndex >= ih->data->plt->_plot.mPlotDataContainer.GetPlotCount())
    return NULL;

  LegendData* legend = ih->data->plt->_plot.mPlotDataContainer.GetLegendData(ih->data->plt->_currentDataSetIndex);
  char* att_buffer = iupStrGetMemory(30);
  sprintf(att_buffer, "%d %d %d", legend->mColor.mR, legend->mColor.mG, legend->mColor.mB);
  return att_buffer;
}

/* show values */
static int iPPlotSetDSShowValuesAttrib(Ihandle* ih, const char* value)
{
  if (ih->data->plt->_currentDataSetIndex <  0 ||
      ih->data->plt->_currentDataSetIndex >= ih->data->plt->_plot.mPlotDataContainer.GetPlotCount())
    return 0;
 
  DataDrawerBase* drawer = ih->data->plt->_plot.mPlotDataContainer.GetDataDrawer(ih->data->plt->_currentDataSetIndex);
    
  if (iupStrEqualNoCase(value, "YES") || iupStrEqualNoCase(value, "ON"))
    drawer->mShowValues = true;
  else 
    drawer->mShowValues = false;

  ih->data->plt->_redraw = 1;
  return 0;
}

static char* iPPlotGetDSShowValuesAttrib(Ihandle* ih)
{
  if (ih->data->plt->_currentDataSetIndex < 0 ||
      ih->data->plt->_currentDataSetIndex >= ih->data->plt->_plot.mPlotDataContainer.GetPlotCount())
    return NULL;

  DataDrawerBase* drawer = ih->data->plt->_plot.mPlotDataContainer.GetDataDrawer(ih->data->plt->_currentDataSetIndex);
  if (drawer->mShowValues)
    return "YES";
  else
    return "NO";
}

/* current dataset drawing mode */
static int iPPlotSetDSModeAttrib(Ihandle* ih, const char* value)
{
  if (ih->data->plt->_currentDataSetIndex <  0 ||
      ih->data->plt->_currentDataSetIndex >= ih->data->plt->_plot.mPlotDataContainer.GetPlotCount())
    return 0;
  
  DataDrawerBase *theDataDrawer = NULL;
  ih->data->plt->_plot.mXAxisSetup.mDiscrete = false;
    
  if(iupStrEqualNoCase(value, "BAR"))
  {
    theDataDrawer = new BarDataDrawer();
    ih->data->plt->_plot.mXAxisSetup.mDiscrete = true;
  }
  else if(iupStrEqualNoCase(value, "MARK"))
    theDataDrawer = new MarkDataDrawer(0);
  else if(iupStrEqualNoCase(value, "MARKLINE"))
    theDataDrawer = new MarkDataDrawer(1);
  else  /* LINE */
    theDataDrawer = new LineDataDrawer();

  ih->data->plt->_plot.mPlotDataContainer.SetDataDrawer(ih->data->plt->_currentDataSetIndex, theDataDrawer);

  ih->data->plt->_redraw = 1;
  return 0;
}

static char* iPPlotGetDSModeAttrib(Ihandle* ih)
{
  if (ih->data->plt->_currentDataSetIndex < 0 ||
      ih->data->plt->_currentDataSetIndex >= ih->data->plt->_plot.mPlotDataContainer.GetPlotCount())
    return NULL;

  DataDrawerBase* drawer = ih->data->plt->_plot.mPlotDataContainer.GetDataDrawer(ih->data->plt->_currentDataSetIndex);

  return (char*)drawer->mMode;
}

/* allows selection and editing */
static int iPPlotSetDSEditAttrib(Ihandle* ih, const char* value)
{
  if (ih->data->plt->_currentDataSetIndex <  0 ||
      ih->data->plt->_currentDataSetIndex >= ih->data->plt->_plot.mPlotDataContainer.GetPlotCount())
    return 0;
  
  PlotDataSelection* dataselect = ih->data->plt->_plot.mPlotDataContainer.GetPlotDataSelection(ih->data->plt->_currentDataSetIndex);
    
  if (iupStrEqualNoCase(value, "YES") || iupStrEqualNoCase(value, "ON"))
    dataselect->resize(ih->data->plt->_plot.mPlotDataContainer.GetConstYData(ih->data->plt->_currentDataSetIndex)->GetSize());
  else
    dataselect->clear();

  ih->data->plt->_redraw = 1;
  return 0;
}

static char* iPPlotGetDSEditAttrib(Ihandle* ih)
{
  if (ih->data->plt->_currentDataSetIndex < 0 ||
      ih->data->plt->_currentDataSetIndex >= ih->data->plt->_plot.mPlotDataContainer.GetPlotCount())
    return NULL;

  PlotDataSelection* dataselect = ih->data->plt->_plot.mPlotDataContainer.GetPlotDataSelection(ih->data->plt->_currentDataSetIndex);
  if (dataselect->empty())
    return "NO";
  else
    return "YES";
}

/* remove a sample */
static int iPPlotSetDSRemoveAttrib(Ihandle* ih, const char* value)
{
  int ii;

  if (ih->data->plt->_currentDataSetIndex <  0 ||
      ih->data->plt->_currentDataSetIndex >= ih->data->plt->_plot.mPlotDataContainer.GetPlotCount())
    return 0;
  
  if (iupStrToInt(value, &ii))
  {
    RemoveSample(ih->data->plt->_plot, ih->data->plt->_currentDataSetIndex, ii);
    ih->data->plt->_redraw = 1;
  }
  return 0;
}

/* ========== */
/* axis props */
/* ========== */

/* ========== */
/* axis props */
/* ========== */

/* axis title */
static int iPPlotSetAXSXLabelAttrib(Ihandle* ih, const char* value)
{
  AxisSetup* axis = &ih->data->plt->_plot.mXAxisSetup;

  if (value)
    axis->mLabel = value;
  else
    axis->mLabel = "";

  ih->data->plt->_redraw = 1;
  return 0;
}

static int iPPlotSetAXSYLabelAttrib(Ihandle* ih, const char* value)
{
  AxisSetup* axis = &ih->data->plt->_plot.mYAxisSetup;

  if (value)
    axis->mLabel = value;
  else
    axis->mLabel = "";

  ih->data->plt->_redraw = 1;
  return 0;
}

static char* iPPlotGetAXSXLabelAttrib(Ihandle* ih)
{
  AxisSetup* axis = &ih->data->plt->_plot.mXAxisSetup;
  char* att_buffer = iupStrGetMemory(256);
  strncpy(att_buffer, axis->mLabel.c_str(), 255);
  att_buffer[255]='\0';
  return att_buffer;
}

static char* iPPlotGetAXSYLabelAttrib(Ihandle* ih)
{
  AxisSetup* axis = &ih->data->plt->_plot.mYAxisSetup;
  char* att_buffer = iupStrGetMemory(256);
  strncpy(att_buffer, axis->mLabel.c_str(), 255);
  att_buffer[255]='\0';
  return att_buffer;
}

/* axis title position */
static int iPPlotSetAXSXLabelCenteredAttrib(Ihandle* ih, const char* value)
{
  AxisSetup* axis = &ih->data->plt->_plot.mXAxisSetup;

  if (iupStrEqualNoCase(value, "YES") || iupStrEqualNoCase(value, "ON"))
    axis->mLabelCentered = true;
  else 
   axis->mLabelCentered = false;

  ih->data->plt->_redraw = 1;
  return 0;
}

static int iPPlotSetAXSYLabelCenteredAttrib(Ihandle* ih, const char* value)
{
  AxisSetup* axis = &ih->data->plt->_plot.mYAxisSetup;

  if (iupStrEqualNoCase(value, "YES") || iupStrEqualNoCase(value, "ON"))
    axis->mLabelCentered = true;
  else 
   axis->mLabelCentered = false;

  ih->data->plt->_redraw = 1;
  return 0;
}

static char* iPPlotGetAXSXLabelCenteredAttrib(Ihandle* ih)
{
  AxisSetup* axis = &ih->data->plt->_plot.mXAxisSetup;

  if (axis->mLabelCentered)
    return "YES";
  else
    return "NO";
}

static char* iPPlotGetAXSYLabelCenteredAttrib(Ihandle* ih)
{
  AxisSetup* axis = &ih->data->plt->_plot.mYAxisSetup;

  if (axis->mLabelCentered)
    return "YES";
  else
    return "NO";
}

/* axis, ticks and label color */
static int iPPlotSetAXSXColorAttrib(Ihandle* ih, const char* value)
{
  unsigned char rr, gg, bb;
  if (iupStrToRGB(value, &rr, &gg, &bb))
  {
    AxisSetup* axis = &ih->data->plt->_plot.mXAxisSetup;
    axis->mColor = PColor(rr, gg, bb);
    ih->data->plt->_redraw = 1;
  }
  return 0;
}

static int iPPlotSetAXSYColorAttrib(Ihandle* ih, const char* value)
{
  unsigned char rr, gg, bb;
  if (iupStrToRGB(value, &rr, &gg, &bb))
  {
    AxisSetup* axis = &ih->data->plt->_plot.mYAxisSetup;
    axis->mColor = PColor(rr, gg, bb);
    ih->data->plt->_redraw = 1;
  }
  return 0;
}

static char* iPPlotGetAXSXColorAttrib(Ihandle* ih)
{
  AxisSetup* axis = &ih->data->plt->_plot.mXAxisSetup;
  char* att_buffer = iupStrGetMemory(30);
  sprintf(att_buffer, "%d %d %d",
          axis->mColor.mR,
          axis->mColor.mG,
          axis->mColor.mB);
  return att_buffer;
}

static char* iPPlotGetAXSYColorAttrib(Ihandle* ih)
{
  AxisSetup* axis = &ih->data->plt->_plot.mYAxisSetup;
  char* att_buffer = iupStrGetMemory(30);
  sprintf(att_buffer, "%d %d %d",
          axis->mColor.mR,
          axis->mColor.mG,
          axis->mColor.mB);
  return att_buffer;
}

/* autoscaling */
static int iPPlotSetAXSXAutoMinAttrib(Ihandle* ih, const char* value)
{
  AxisSetup* axis = &ih->data->plt->_plot.mXAxisSetup;

  if(iupStrEqualNoCase(value, "YES") || iupStrEqualNoCase(value, "ON"))
    axis->mAutoScaleMin = true;
  else 
    axis->mAutoScaleMin = false;

  ih->data->plt->_redraw = 1;
  return 0;
}

static int iPPlotSetAXSYAutoMinAttrib(Ihandle* ih, const char* value)
{
  AxisSetup* axis = &ih->data->plt->_plot.mYAxisSetup;

  if(iupStrEqualNoCase(value, "YES") || iupStrEqualNoCase(value, "ON"))
    axis->mAutoScaleMin = true;
  else 
    axis->mAutoScaleMin = false;

  ih->data->plt->_redraw = 1;
  return 0;
}

static char* iPPlotGetAXSXAutoMinAttrib(Ihandle* ih)
{
  AxisSetup* axis = &ih->data->plt->_plot.mXAxisSetup;

  if (axis->mAutoScaleMin)
    return "YES";
  else
    return "NO";
}

static char* iPPlotGetAXSYAutoMinAttrib(Ihandle* ih)
{
  AxisSetup* axis = &ih->data->plt->_plot.mYAxisSetup;

  if (axis->mAutoScaleMin)
    return "YES";
  else
    return "NO";
}

/* autoscaling */
static int iPPlotSetAXSXAutoMaxAttrib(Ihandle* ih, const char* value)
{
  AxisSetup* axis = &ih->data->plt->_plot.mXAxisSetup;

  if(iupStrEqualNoCase(value, "YES") || iupStrEqualNoCase(value, "ON"))
    axis->mAutoScaleMax = true;
  else 
    axis->mAutoScaleMax = false;

  ih->data->plt->_redraw = 1;
  return 0;
}

static int iPPlotSetAXSYAutoMaxAttrib(Ihandle* ih, const char* value)
{
  AxisSetup* axis = &ih->data->plt->_plot.mYAxisSetup;

  if(iupStrEqualNoCase(value, "YES") || iupStrEqualNoCase(value, "ON"))
    axis->mAutoScaleMax = true;
  else 
    axis->mAutoScaleMax = false;

  ih->data->plt->_redraw = 1;
  return 0;
}

static char* iPPlotGetAXSXAutoMaxAttrib(Ihandle* ih)
{
  AxisSetup* axis = &ih->data->plt->_plot.mXAxisSetup;

  if (axis->mAutoScaleMax)
    return "YES";
  else
    return "NO";
}

static char* iPPlotGetAXSYAutoMaxAttrib(Ihandle* ih)
{
  AxisSetup* axis = &ih->data->plt->_plot.mYAxisSetup;

  if (axis->mAutoScaleMax)
    return "YES";
  else
    return "NO";
}

/* min visible val */
static int iPPlotSetAXSXMinAttrib(Ihandle* ih, const char* value)
{
  float xx;
  if (iupStrToFloat(value, &xx))
  {
    AxisSetup* axis = &ih->data->plt->_plot.mXAxisSetup;
    axis->mMin = xx;
    ih->data->plt->_redraw = 1;
  }
  return 0;
}

static int iPPlotSetAXSYMinAttrib(Ihandle* ih, const char* value)
{
  float xx;
  if (iupStrToFloat(value, &xx))
  {
    AxisSetup* axis = &ih->data->plt->_plot.mYAxisSetup;
    axis->mMin = xx;
    ih->data->plt->_redraw = 1;
  }
  return 0;
}

static char* iPPlotGetAXSXMinAttrib(Ihandle* ih)
{
  AxisSetup* axis = &ih->data->plt->_plot.mXAxisSetup;
  char* att_buffer = iupStrGetMemory(30);
  sprintf(att_buffer, "%g", axis->mMin);
  return att_buffer;
}

static char* iPPlotGetAXSYMinAttrib(Ihandle* ih)
{
  AxisSetup* axis = &ih->data->plt->_plot.mYAxisSetup;
  char* att_buffer = iupStrGetMemory(30);
  sprintf(att_buffer, "%g", axis->mMin);
  return att_buffer;
}

/* max visible val */
static int iPPlotSetAXSXMaxAttrib(Ihandle* ih, const char* value)
{
  float xx;
  if (iupStrToFloat(value, &xx))
  {
    AxisSetup* axis = &ih->data->plt->_plot.mXAxisSetup;
    axis->mMax = xx;
    ih->data->plt->_redraw = 1;
  }
  return 0;
}

static int iPPlotSetAXSYMaxAttrib(Ihandle* ih, const char* value)
{
  float xx;
  if (iupStrToFloat(value, &xx))
  {
    AxisSetup* axis = &ih->data->plt->_plot.mYAxisSetup;
    axis->mMax = xx;
    ih->data->plt->_redraw = 1;
  }
  return 0;
}

static char* iPPlotGetAXSXMaxAttrib(Ihandle* ih)
{
  AxisSetup* axis = &ih->data->plt->_plot.mXAxisSetup;
  char* att_buffer = iupStrGetMemory(30);
  sprintf(att_buffer, "%g", axis->mMax);
  return att_buffer;
}

static char* iPPlotGetAXSYMaxAttrib(Ihandle* ih)
{
  AxisSetup* axis = &ih->data->plt->_plot.mYAxisSetup;
  char* att_buffer = iupStrGetMemory(30);
  sprintf(att_buffer, "%g", axis->mMax);
  return att_buffer;
}

/* values from left/top to right/bottom */
static int iPPlotSetAXSXReverseAttrib(Ihandle* ih, const char* value)
{
  AxisSetup* axis = &ih->data->plt->_plot.mXAxisSetup;

  if(iupStrEqualNoCase(value, "YES") || iupStrEqualNoCase(value, "ON"))
    axis->mAscending = false; // inverted
  else 
    axis->mAscending = true;

  ih->data->plt->_redraw = 1;
  return 0;
}

static int iPPlotSetAXSYReverseAttrib(Ihandle* ih, const char* value)
{
  AxisSetup* axis = &ih->data->plt->_plot.mYAxisSetup;

  if(iupStrEqualNoCase(value, "YES") || iupStrEqualNoCase(value, "ON"))
    axis->mAscending = false; // inverted
  else 
    axis->mAscending = true;

  ih->data->plt->_redraw = 1;
  return 0;
}

static char* iPPlotGetAXSXReverseAttrib(Ihandle* ih)
{
  AxisSetup* axis = &ih->data->plt->_plot.mXAxisSetup;

  if (axis->mAscending)
    return "NO";    /* inverted */
  else
    return "YES";
}

static char* iPPlotGetAXSYReverseAttrib(Ihandle* ih)
{
  AxisSetup* axis = &ih->data->plt->_plot.mYAxisSetup;

  if (axis->mAscending)
    return "NO";    /* inverted */
  else
    return "YES";
}

/* axis mode */
static int iPPlotSetAXSXCrossOriginAttrib(Ihandle* ih, const char* value)
{
  AxisSetup* axis = &ih->data->plt->_plot.mXAxisSetup;

  if(iupStrEqualNoCase(value, "YES") || iupStrEqualNoCase(value, "ON"))
    axis->mCrossOrigin = true;
  else 
    axis->mCrossOrigin = false;

  ih->data->plt->_redraw = 1;
  return 0;
}

static int iPPlotSetAXSYCrossOriginAttrib(Ihandle* ih, const char* value)
{
  AxisSetup* axis = &ih->data->plt->_plot.mYAxisSetup;

  if(iupStrEqualNoCase(value, "YES") || iupStrEqualNoCase(value, "ON"))
    axis->mCrossOrigin = true;
  else 
    axis->mCrossOrigin = false;

  ih->data->plt->_redraw = 1;
  return 0;
}

static char* iPPlotGetAXSXCrossOriginAttrib(Ihandle* ih)
{
  AxisSetup* axis = &ih->data->plt->_plot.mXAxisSetup;

  if (axis->mCrossOrigin)
    return "YES";
  else
    return "NO";
}

static char* iPPlotGetAXSYCrossOriginAttrib(Ihandle* ih)
{
  AxisSetup* axis = &ih->data->plt->_plot.mYAxisSetup;

  if (axis->mCrossOrigin)
    return "YES";
  else
    return "NO";
}

/* log/lin scale */
static int iPPlotSetAXSXScaleAttrib(Ihandle* ih, const char* value)
{
  AxisSetup* axis = &ih->data->plt->_plot.mXAxisSetup;

  if(iupStrEqualNoCase(value, "LIN"))
  {
    axis->mLogScale = false;
  }
  else if(iupStrEqualNoCase(value, "LOG10"))
  {
    axis->mLogScale = true;
    axis->mLogBase  = 10.0;
  }
  else if(iupStrEqualNoCase(value, "LOG2"))
  {
    axis->mLogScale = true;
    axis->mLogBase  = 2.0;
  }
  else
  {
    axis->mLogScale = true;
    axis->mLogBase  = (float)M_E;
  }

  ih->data->plt->_redraw = 1;
  return 0;
}

static int iPPlotSetAXSYScaleAttrib(Ihandle* ih, const char* value)
{
  AxisSetup* axis = &ih->data->plt->_plot.mYAxisSetup;

  if(iupStrEqualNoCase(value, "LIN"))
  {
    axis->mLogScale = false;
  }
  else if(iupStrEqualNoCase(value, "LOG10"))
  {
    axis->mLogScale = true;
    axis->mLogBase  = 10.0;
  }
  else if(iupStrEqualNoCase(value, "LOG2"))
  {
    axis->mLogScale = true;
    axis->mLogBase  = 2.0;
  }
  else
  {
    axis->mLogScale = true;
    axis->mLogBase  = (float)M_E;
  }

  ih->data->plt->_redraw = 1;
  return 0;
}

static char* iPPlotGetAXSXScaleAttrib(Ihandle* ih)
{
  AxisSetup* axis = &ih->data->plt->_plot.mXAxisSetup;
  char* att_buffer = iupStrGetMemory(30);

  if (axis->mLogScale)
  {
    if (axis->mLogBase == 10.0)
      strcpy(att_buffer, "LOG10");
    else if (axis->mLogBase == 2.0)
      strcpy(att_buffer, "LOG2");
    else
      strcpy(att_buffer, "LOGN");
  }
  else
    strcpy(att_buffer, "LIN");

  return att_buffer;
}

static char* iPPlotGetAXSYScaleAttrib(Ihandle* ih)
{
  AxisSetup* axis = &ih->data->plt->_plot.mYAxisSetup;
  char* att_buffer = iupStrGetMemory(30);

  if (axis->mLogScale)
  {
    if (axis->mLogBase == 10.0)
      strcpy(att_buffer, "LOG10");
    else if (axis->mLogBase == 2.0)
      strcpy(att_buffer, "LOG2");
    else
      strcpy(att_buffer, "LOGN");
  }
  else
    strcpy(att_buffer, "LIN");

  return att_buffer;
}

/* axis label font size */
static int iPPlotSetAXSXFontSizeAttrib(Ihandle* ih, const char* value)
{
  int ii;
  if (iupStrToInt(value, &ii))
  {
    AxisSetup* axis = &ih->data->plt->_plot.mXAxisSetup;
    axis->mStyle.mFontSize = ii;
    ih->data->plt->_redraw = 1;
  }
  return 0;
}

static int iPPlotSetAXSYFontSizeAttrib(Ihandle* ih, const char* value)
{
  int ii;
  if (iupStrToInt(value, &ii))
  {
    AxisSetup* axis = &ih->data->plt->_plot.mYAxisSetup;
    axis->mStyle.mFontSize = ii;
    ih->data->plt->_redraw = 1;
  }
  return 0;
}

static char* iPPlotGetAXSXFontSizeAttrib(Ihandle* ih)
{
  AxisSetup* axis = &ih->data->plt->_plot.mXAxisSetup;
  return iPPlotGetPlotFontSize(axis->mStyle.mFontSize);
}

static char* iPPlotGetAXSYFontSizeAttrib(Ihandle* ih)
{
  AxisSetup* axis = &ih->data->plt->_plot.mYAxisSetup;
  return iPPlotGetPlotFontSize(axis->mStyle.mFontSize);
}

/* axis label font style */
static int iPPlotSetAXSXFontStyleAttrib(Ihandle* ih, const char* value)
{
  int style = iPPlotGetCDFontStyle(value);
  if (style != -1)
  {
    AxisSetup* axis = &ih->data->plt->_plot.mXAxisSetup;
    axis->mStyle.mFontStyle = style;
    ih->data->plt->_redraw = 1;
  }
  return 0;
}

static int iPPlotSetAXSYFontStyleAttrib(Ihandle* ih, const char* value)
{
  int style = iPPlotGetCDFontStyle(value);
  if (style != -1)
  {
    AxisSetup* axis = &ih->data->plt->_plot.mYAxisSetup;
    axis->mStyle.mFontStyle = style;
    ih->data->plt->_redraw = 1;
  }
  return 0;
}

static char* iPPlotGetAXSXFontStyleAttrib(Ihandle* ih)
{
  AxisSetup* axis = &ih->data->plt->_plot.mXAxisSetup;
  return iPPlotGetPlotFontStyle(axis->mStyle.mFontStyle);
}

static char* iPPlotGetAXSYFontStyleAttrib(Ihandle* ih)
{
  AxisSetup* axis = &ih->data->plt->_plot.mYAxisSetup;
  return iPPlotGetPlotFontStyle(axis->mStyle.mFontStyle);
}

/* automatic tick size */
static int iPPlotSetAXSXAutoTickSizeAttrib(Ihandle* ih, const char* value)
{
  AxisSetup* axis = &ih->data->plt->_plot.mXAxisSetup;

  if(iupStrEqualNoCase(value, "YES") || iupStrEqualNoCase(value, "ON"))
    axis->mTickInfo.mAutoTickSize = true;
  else 
    axis->mTickInfo.mAutoTickSize = false;

  ih->data->plt->_redraw = 1;
  return 0;
}

static int iPPlotSetAXSYAutoTickSizeAttrib(Ihandle* ih, const char* value)
{
  AxisSetup* axis = &ih->data->plt->_plot.mYAxisSetup;

  if(iupStrEqualNoCase(value, "YES") || iupStrEqualNoCase(value, "ON"))
    axis->mTickInfo.mAutoTickSize = true;
  else 
    axis->mTickInfo.mAutoTickSize = false;

  ih->data->plt->_redraw = 1;
  return 0;
}

static char* iPPlotGetAXSXAutoTickSizeAttrib(Ihandle* ih)
{
  AxisSetup* axis = &ih->data->plt->_plot.mXAxisSetup;

  if (axis->mTickInfo.mAutoTickSize)
    return "YES";
  else
    return "NO";
}

static char* iPPlotGetAXSYAutoTickSizeAttrib(Ihandle* ih)
{
  AxisSetup* axis = &ih->data->plt->_plot.mYAxisSetup;

  if (axis->mTickInfo.mAutoTickSize)
    return "YES";
  else
    return "NO";
}

/* size of ticks (in pixels) */
static int iPPlotSetAXSXTickSizeAttrib(Ihandle* ih, const char* value)
{
  int ii;
  if (iupStrToInt(value, &ii))
  {
    AxisSetup* axis = &ih->data->plt->_plot.mXAxisSetup;
    axis->mTickInfo.mMinorTickScreenSize = ii;
    ih->data->plt->_redraw = 1;
  }
  return 0;
}

static int iPPlotSetAXSYTickSizeAttrib(Ihandle* ih, const char* value)
{
  int ii;
  if (iupStrToInt(value, &ii))
  {
    AxisSetup* axis = &ih->data->plt->_plot.mYAxisSetup;
    axis->mTickInfo.mMinorTickScreenSize = ii;
    ih->data->plt->_redraw = 1;
  }
  return 0;
}

static char* iPPlotGetAXSXTickSizeAttrib(Ihandle* ih)
{
  AxisSetup* axis = &ih->data->plt->_plot.mXAxisSetup;
  char* att_buffer = iupStrGetMemory(30);
  sprintf(att_buffer, "%d", axis->mTickInfo.mMinorTickScreenSize);
  return att_buffer;
}

static char* iPPlotGetAXSYTickSizeAttrib(Ihandle* ih)
{
  AxisSetup* axis = &ih->data->plt->_plot.mYAxisSetup;
  char* att_buffer = iupStrGetMemory(30);
  sprintf(att_buffer, "%d", axis->mTickInfo.mMinorTickScreenSize);
  return att_buffer;
}

/* size of major ticks (in pixels) */
static int iPPlotSetAXSXTickMajorSizeAttrib(Ihandle* ih, const char* value)
{
  int ii;
  if (iupStrToInt(value, &ii))
  {
    AxisSetup* axis = &ih->data->plt->_plot.mXAxisSetup;
    axis->mTickInfo.mMajorTickScreenSize = ii;
    ih->data->plt->_redraw = 1;
  }
  return 0;
}

static int iPPlotSetAXSYTickMajorSizeAttrib(Ihandle* ih, const char* value)
{
  int ii;
  if (iupStrToInt(value, &ii))
  {
    AxisSetup* axis = &ih->data->plt->_plot.mYAxisSetup;
    axis->mTickInfo.mMajorTickScreenSize = ii;
    ih->data->plt->_redraw = 1;
  }
  return 0;
}

static char* iPPlotGetAXSXTickMajorSizeAttrib(Ihandle* ih)
{
  AxisSetup* axis = &ih->data->plt->_plot.mXAxisSetup;
  char* att_buffer = iupStrGetMemory(30);
  sprintf(att_buffer, "%d", axis->mTickInfo.mMajorTickScreenSize);
  return att_buffer;
}

static char* iPPlotGetAXSYTickMajorSizeAttrib(Ihandle* ih)
{
  AxisSetup* axis = &ih->data->plt->_plot.mYAxisSetup;
  char* att_buffer = iupStrGetMemory(30);
  sprintf(att_buffer, "%d", axis->mTickInfo.mMajorTickScreenSize);
  return att_buffer;
}

/* axis ticks font size */
static int iPPlotSetAXSXTickFontSizeAttrib(Ihandle* ih, const char* value)
{
  int ii;
  if (iupStrToInt(value, &ii))
  {
    AxisSetup* axis = &ih->data->plt->_plot.mXAxisSetup;
    axis->mTickInfo.mStyle.mFontSize = ii;
    ih->data->plt->_redraw = 1;
  }
  return 0;
}

static int iPPlotSetAXSYTickFontSizeAttrib(Ihandle* ih, const char* value)
{
  int ii;
  if (iupStrToInt(value, &ii))
  {
    AxisSetup* axis = &ih->data->plt->_plot.mYAxisSetup;
    axis->mTickInfo.mStyle.mFontSize = ii;
    ih->data->plt->_redraw = 1;
  }
  return 0;
}

static char* iPPlotGetAXSXTickFontSizeAttrib(Ihandle* ih)
{
  AxisSetup* axis = &ih->data->plt->_plot.mXAxisSetup;

  return iPPlotGetPlotFontSize(axis->mTickInfo.mStyle.mFontSize);
}

static char* iPPlotGetAXSYTickFontSizeAttrib(Ihandle* ih)
{
  AxisSetup* axis = &ih->data->plt->_plot.mYAxisSetup;

  return iPPlotGetPlotFontSize(axis->mTickInfo.mStyle.mFontSize);
}

/* axis ticks number font style */
static int iPPlotSetAXSXTickFontStyleAttrib(Ihandle* ih, const char* value)
{
  int style = iPPlotGetCDFontStyle(value);
  if (style != -1)
  {
    AxisSetup* axis = &ih->data->plt->_plot.mXAxisSetup;
    axis->mTickInfo.mStyle.mFontStyle = style;
    ih->data->plt->_redraw = 1;
  }
  return 0;
}

static int iPPlotSetAXSYTickFontStyleAttrib(Ihandle* ih, const char* value)
{
  int style = iPPlotGetCDFontStyle(value);
  if (style != -1)
  {
    AxisSetup* axis = &ih->data->plt->_plot.mYAxisSetup;
    axis->mTickInfo.mStyle.mFontStyle = style;
    ih->data->plt->_redraw = 1;
  }
  return 0;
}

static char* iPPlotGetAXSXTickFontStyleAttrib(Ihandle* ih)
{
  AxisSetup* axis = &ih->data->plt->_plot.mXAxisSetup;

  return iPPlotGetPlotFontSize(axis->mTickInfo.mStyle.mFontStyle);
}

static char* iPPlotGetAXSYTickFontStyleAttrib(Ihandle* ih)
{
  AxisSetup* axis = &ih->data->plt->_plot.mYAxisSetup;

  return iPPlotGetPlotFontSize(axis->mTickInfo.mStyle.mFontStyle);
}

/* axis ticks number format */
static int iPPlotSetAXSXTickFormatAttrib(Ihandle* ih, const char* value)
{
  AxisSetup* axis = &ih->data->plt->_plot.mXAxisSetup;

  if (value && value[0]!=0)
    axis->mTickInfo.mFormatString = value;
  else
    axis->mTickInfo.mFormatString = "%.0f";

  ih->data->plt->_redraw = 1;
  return 0;
}

static int iPPlotSetAXSYTickFormatAttrib(Ihandle* ih, const char* value)
{
  AxisSetup* axis = &ih->data->plt->_plot.mYAxisSetup;

  if (value && value[0]!=0)
    axis->mTickInfo.mFormatString = value;
  else
    axis->mTickInfo.mFormatString = "%.0f";

  ih->data->plt->_redraw = 1;
  return 0;
}

static char* iPPlotGetAXSXTickFormatAttrib(Ihandle* ih)
{
  AxisSetup* axis = &ih->data->plt->_plot.mXAxisSetup;
  char* att_buffer = iupStrGetMemory(256);
  strncpy(att_buffer, axis->mTickInfo.mFormatString.c_str(), 255);
  att_buffer[255]='\0';
  return att_buffer;
}

static char* iPPlotGetAXSYTickFormatAttrib(Ihandle* ih)
{
  AxisSetup* axis = &ih->data->plt->_plot.mYAxisSetup;
  char* att_buffer = iupStrGetMemory(256);
  strncpy(att_buffer, axis->mTickInfo.mFormatString.c_str(), 255);
  att_buffer[255]='\0';
  return att_buffer;
}

/* axis ticks */
static int iPPlotSetAXSXTickAttrib(Ihandle* ih, const char* value)
{
  AxisSetup* axis = &ih->data->plt->_plot.mXAxisSetup;

  if(iupStrEqualNoCase(value, "YES") || iupStrEqualNoCase(value, "ON"))
    axis->mTickInfo.mTicksOn = true;
  else 
    axis->mTickInfo.mTicksOn = false;

  ih->data->plt->_redraw = 1;
  return 0;
}

static int iPPlotSetAXSYTickAttrib(Ihandle* ih, const char* value)
{
  AxisSetup* axis = &ih->data->plt->_plot.mYAxisSetup;

  if(iupStrEqualNoCase(value, "YES") || iupStrEqualNoCase(value, "ON"))
    axis->mTickInfo.mTicksOn = true;
  else 
    axis->mTickInfo.mTicksOn = false;

  ih->data->plt->_redraw = 1;
  return 0;
}

static char* iPPlotGetAXSXTickAttrib(Ihandle* ih)
{
  AxisSetup* axis = &ih->data->plt->_plot.mXAxisSetup;

  if (axis->mTickInfo.mTicksOn)
    return "YES";
  else
    return "NO";
}

static char* iPPlotGetAXSYTickAttrib(Ihandle* ih)
{
  AxisSetup* axis = &ih->data->plt->_plot.mYAxisSetup;

  if (axis->mTickInfo.mTicksOn)
    return "YES";
  else
    return "NO";
}

/* major tick spacing */
static int iPPlotSetAXSXTickMajorSpanAttrib(Ihandle* ih, const char* value)
{
  float xx;
  if (iupStrToFloat(value, &xx))
  {
    AxisSetup* axis = &ih->data->plt->_plot.mXAxisSetup;
    axis->mTickInfo.mMajorTickSpan = xx;
    ih->data->plt->_redraw = 1;
  }
  return 0;
}

static int iPPlotSetAXSYTickMajorSpanAttrib(Ihandle* ih, const char* value)
{
  float xx;
  if (iupStrToFloat(value, &xx))
  {
    AxisSetup* axis = &ih->data->plt->_plot.mYAxisSetup;
    axis->mTickInfo.mMajorTickSpan = xx;
    ih->data->plt->_redraw = 1;
  }
  return 0;
}

static char* iPPlotGetAXSXTickMajorSpanAttrib(Ihandle* ih)
{
  AxisSetup* axis = &ih->data->plt->_plot.mXAxisSetup;
  char* att_buffer = iupStrGetMemory(30);
  sprintf(att_buffer, "%g", axis->mTickInfo.mMajorTickSpan);
  return att_buffer;
}

static char* iPPlotGetAXSYTickMajorSpanAttrib(Ihandle* ih)
{
  AxisSetup* axis = &ih->data->plt->_plot.mYAxisSetup;
  char* att_buffer = iupStrGetMemory(30);
  sprintf(att_buffer, "%g", axis->mTickInfo.mMajorTickSpan);
  return att_buffer;
}

/* number of ticks between major ticks */
static int iPPlotSetAXSXTickDivisionAttrib(Ihandle* ih, const char* value)
{
  int ii;
  if (iupStrToInt(value, &ii))
  {
    AxisSetup* axis = &ih->data->plt->_plot.mXAxisSetup;
    axis->mTickInfo.mTickDivision = ii;
    ih->data->plt->_redraw = 1;
  }
  return 0;
}

static int iPPlotSetAXSYTickDivisionAttrib(Ihandle* ih, const char* value)
{
  int ii;
  if (iupStrToInt(value, &ii))
  {
    AxisSetup* axis = &ih->data->plt->_plot.mYAxisSetup;
    axis->mTickInfo.mTickDivision = ii;
    ih->data->plt->_redraw = 1;
  }
  return 0;
}

static char* iPPlotGetAXSXTickDivisionAttrib(Ihandle* ih)
{
  AxisSetup* axis = &ih->data->plt->_plot.mXAxisSetup;
  char* att_buffer = iupStrGetMemory(30);
  sprintf(att_buffer, "%d", axis->mTickInfo.mTickDivision);
  return att_buffer;
}

static char* iPPlotGetAXSYTickDivisionAttrib(Ihandle* ih)
{
  AxisSetup* axis = &ih->data->plt->_plot.mYAxisSetup;
  char* att_buffer = iupStrGetMemory(30);
  sprintf(att_buffer, "%d", axis->mTickInfo.mTickDivision);
  return att_buffer;
}

/* auto tick spacing */
static int iPPlotSetAXSXAutoTickAttrib(Ihandle* ih, const char* value)
{
  AxisSetup* axis = &ih->data->plt->_plot.mXAxisSetup;

  if(iupStrEqualNoCase(value, "YES") || iupStrEqualNoCase(value, "ON"))
    axis->mTickInfo.mAutoTick = true;
  else 
    axis->mTickInfo.mAutoTick = false;

  ih->data->plt->_redraw = 1;
  return 0;
}

static int iPPlotSetAXSYAutoTickAttrib(Ihandle* ih, const char* value)
{
  AxisSetup* axis = &ih->data->plt->_plot.mYAxisSetup;

  if(iupStrEqualNoCase(value, "YES") || iupStrEqualNoCase(value, "ON"))
    axis->mTickInfo.mAutoTick = true;
  else 
    axis->mTickInfo.mAutoTick = false;

  ih->data->plt->_redraw = 1;
  return 0;
}

static char* iPPlotGetAXSXAutoTickAttrib(Ihandle* ih)
{
  AxisSetup* axis = &ih->data->plt->_plot.mXAxisSetup;

  if (axis->mTickInfo.mAutoTick)
    return "YES";
  else
    return "NO";
}

static char* iPPlotGetAXSYAutoTickAttrib(Ihandle* ih)
{
  AxisSetup* axis = &ih->data->plt->_plot.mYAxisSetup;

  if (axis->mTickInfo.mAutoTick)
    return "YES";
  else
    return "NO";
}

/* MouseButton */
void PPainterIup::MouseButton(int btn, int stat, int x, int y, char *r)
{
  PMouseEvent theEvent;
  int theModifierKeys = 0;

  theEvent.mX = x;
  theEvent.mY = y;
  
  if(btn == IUP_BUTTON1)
  {
    theEvent.mType = ( stat!=0 ? (PMouseEvent::kDown) : (PMouseEvent::kUp) );
    _mouseDown = ( stat!=0 ? 1 : 0 );
  }
  else return;

  _mouse_ALT = 0;
  _mouse_SHIFT = 0;
  _mouse_CTRL = 0;

  if (isalt(r))  /* signal Alt */
  {
    theModifierKeys = (theModifierKeys | PMouseEvent::kAlt);
    _mouse_ALT = 1;
  }
  if (iscontrol(r))  /* signal Ctrl */
  {
    theModifierKeys = (theModifierKeys | PMouseEvent::kControl);
    _mouse_SHIFT = 1;
  }
  if (isshift(r))  /* signal Shift */
  {
    theModifierKeys = (theModifierKeys | PMouseEvent::kShift);
    _mouse_CTRL = 1;
  }
  theEvent.SetModifierKeys (theModifierKeys);

  if( _InteractionContainer->HandleMouseEvent(theEvent))
  {
    this->Draw(1);
  } 
  else
  {
    /* ignore the event */
  }
}

/* MouseMove */
void PPainterIup::MouseMove(int x, int y)
{
  PMouseEvent theEvent;
  int theModifierKeys = 0;

  if(!_mouseDown ) return;

  theEvent.mX = x;
  theEvent.mY = y;

  theEvent.mType = PMouseEvent::kMove;
  if(_mouse_ALT)  /* signal Alt */
  {
    theModifierKeys = (theModifierKeys | PMouseEvent::kAlt);
  }
  if(_mouse_SHIFT)  /* signal Shift */
  {
    theModifierKeys = (theModifierKeys | PMouseEvent::kControl);
  }
  if(_mouse_CTRL)  /* signal Ctrl */
  {
    theModifierKeys = (theModifierKeys | PMouseEvent::kShift);
  }
  theEvent.SetModifierKeys (theModifierKeys);

  if(_InteractionContainer->HandleMouseEvent(theEvent))
  {
    this->Draw(1);
  } 
  else
  {
    /* ignore the event */
  }
}

/* KeyPress */
void PPainterIup::KeyPress(int c, int press)
{
  int theModifierKeys = 0;
  int theRepeatCount = 0;
  PKeyEvent::EKey theKeyCode = PKeyEvent::kNone;
  char theChar = 0;

  if(!press) return;

  switch(c)
  {
    case K_cX:  /* CTRL + X */
      theModifierKeys = PMouseEvent::kControl;
      theKeyCode = PKeyEvent::kChar;
      theChar = 'x';
    break;
    case K_cY:  /* CTRL + Y */
      theModifierKeys = PMouseEvent::kControl;
      theKeyCode = PKeyEvent::kChar;
      theChar = 'y';
    break;
    case K_cR:  /* CTRL + R */
      theModifierKeys = PMouseEvent::kControl;
      theKeyCode = PKeyEvent::kChar;
      theChar = 'r';
    break;
    case K_cUP:  /* CTRL + Arrow */
      theModifierKeys = PMouseEvent::kControl;
    case K_UP:   /* Arrow */
      theKeyCode = PKeyEvent::kArrowUp;
    break;
    case K_cDOWN:  /* CTRL + Arrow */
      theModifierKeys = PMouseEvent::kControl;
    case K_DOWN:   /* Arrow */
      theKeyCode = PKeyEvent::kArrowDown;
    break;
    case K_cLEFT:  /* CTRL + Arrow */
      theModifierKeys = PMouseEvent::kControl;
    case K_LEFT:   /* Arrow */
      theKeyCode = PKeyEvent::kArrowLeft;
    break;
    case K_cRIGHT:  /* CTRL + Arrow */
      theModifierKeys = PMouseEvent::kControl;
    case K_RIGHT:   /* Arrow */
      theKeyCode = PKeyEvent::kArrowRight;
    break;
    case K_cDEL:  /* CTRL + Arrow */
      theModifierKeys = PMouseEvent::kControl;
    case K_DEL:   /* Arrow */
      theKeyCode = PKeyEvent::kDelete;
    break;
  }

  PKeyEvent theEvent (theKeyCode, theRepeatCount, theModifierKeys, theChar);

  if(_InteractionContainer->HandleKeyEvent(theEvent))
  {
    this->Draw(1);
  } 
  else
  {
    /* ignore the event */
  }
}

/* Draw */
void PPainterIup::Draw(int force)
{
  if (!_cddbuffer)
    return;

  cdCanvasActivate(_cddbuffer);

  if (force || _redraw)
  {
    cdCanvasClear(_cddbuffer);
    _plot.Draw(*this);
    _redraw = 0;
  }

  cdCanvasFlush(_cddbuffer);
}

/* Resize */
void PPainterIup::Resize()
{
  if (!_cddbuffer)
  {
    /* update canvas size */
    cdCanvasActivate(_cdcanvas);

    /* this can fail if canvas size is zero */
    if (IupGetInt(_ih, "USE_IMAGERGB"))
      _cddbuffer = cdCreateCanvas(CD_DBUFFERRGB, _cdcanvas);
    else
      _cddbuffer = cdCreateCanvas(CD_DBUFFER, _cdcanvas);
  }

  if (!_cddbuffer)
    return;

  /* update canvas size */
  cdCanvasActivate(_cddbuffer);

  _redraw = 1;

  return;
}

/* send plot to some other device */ 
void PPainterIup::DrawTo(cdCanvas *usrCnv)
{
  cdCanvas *old_cddbuffer  = _cddbuffer;
  cdCanvas *old_cdcanvas   = _cdcanvas;
  
  _cdcanvas = _cddbuffer = usrCnv;

  if(!_cddbuffer)
    return;

  Draw(1);

  _cddbuffer = old_cddbuffer;
  _cdcanvas  = old_cdcanvas;
}

void PPainterIup::FillArrow(int inX1, int inY1, int inX2, int inY2, int inX3, int inY3)
{
  if (!_cddbuffer)
    return;

  cdCanvasBegin(_cddbuffer, CD_FILL);
  cdCanvasVertex(_cddbuffer, inX1, cdCanvasInvertYAxis(_cddbuffer, inY1));
  cdCanvasVertex(_cddbuffer, inX2, cdCanvasInvertYAxis(_cddbuffer, inY2));
  cdCanvasVertex(_cddbuffer, inX3, cdCanvasInvertYAxis(_cddbuffer, inY3));
  cdCanvasEnd(_cddbuffer);
}

/* DrawLine */
void PPainterIup::DrawLine(float inX1, float inY1, float inX2, float inY2)
{
  if (!_cddbuffer)
    return;

  cdfCanvasLine(_cddbuffer, inX1, cdfCanvasInvertYAxis(_cddbuffer, inY1),
                                           inX2, cdfCanvasInvertYAxis(_cddbuffer, inY2));
}

/* FillRect */
void PPainterIup::FillRect(int inX, int inY, int inW, int inH)
{
  if (!_cddbuffer)
    return;

  cdCanvasBox(_cddbuffer, inX, inX+inW, 
              cdCanvasInvertYAxis(_cddbuffer, inY),
              cdCanvasInvertYAxis(_cddbuffer, inY + inH - 1));
}

/* InvertRect */
void PPainterIup::InvertRect(int inX, int inY, int inW, int inH)
{
  long cprev;

  if (!_cddbuffer)
    return;

  cdCanvasWriteMode(_cddbuffer, CD_XOR);
  cprev = cdCanvasForeground(_cddbuffer, CD_WHITE);
  cdCanvasRect(_cddbuffer, inX, inX + inW - 1,
               cdCanvasInvertYAxis(_cddbuffer, inY),
               cdCanvasInvertYAxis(_cddbuffer, inY + inH - 1));
  cdCanvasWriteMode(_cddbuffer, CD_REPLACE);
  cdCanvasForeground(_cddbuffer, cprev);
}

/* SetClipRect */
void PPainterIup::SetClipRect(int inX, int inY, int inW, int inH)
{
  if (!_cddbuffer)
    return;

  cdCanvasClipArea(_cddbuffer, inX, inX + inW - 1,
                   cdCanvasInvertYAxis(_cddbuffer, inY),
                   cdCanvasInvertYAxis(_cddbuffer, inY + inH - 1));
  cdCanvasClip(_cddbuffer, CD_CLIPAREA);
}

/* GetWidth */
long PPainterIup::GetWidth() const
{
  int iret;

  if (!_cddbuffer)
    return IUP_DEFAULT;

  cdCanvasGetSize(_cddbuffer, &iret, NULL, NULL, NULL);

  return (long)iret;
}

/* GetHeight */
long PPainterIup::GetHeight() const
{
  int iret;

  if (!_cddbuffer)
    return IUP_NOERROR;

  cdCanvasGetSize(_cddbuffer, NULL, &iret, NULL, NULL);

  return (long)iret;
}

/* SetLineColor */
void PPainterIup::SetLineColor(int inR, int inG, int inB)
{
  if (!_cddbuffer)
    return;

  cdCanvasForeground(_cddbuffer, cdEncodeColor((unsigned char)inR,
                                               (unsigned char)inG,
                                               (unsigned char)inB));
}

/* SetFillColor */
void PPainterIup::SetFillColor(int inR, int inG, int inB)
{
  if (!_cddbuffer)
    return;

  cdCanvasForeground(_cddbuffer, cdEncodeColor((unsigned char)inR,
                                               (unsigned char)inG,
                                               (unsigned char)inB));
}

/* CalculateTextDrawSize */
long PPainterIup::CalculateTextDrawSize(const char *inString)
{
  int iw;

  if (!_cddbuffer)
    return IUP_NOERROR;

  cdCanvasGetTextSize(_cddbuffer, const_cast<char *>(inString), &iw, NULL);

  return iw;
}

/* GetFontHeight */
long PPainterIup::GetFontHeight() const
{
  int ih;

  if (!_cddbuffer)
    return IUP_NOERROR;

  cdCanvasGetFontDim(_cddbuffer, NULL, &ih, NULL, NULL);

  return ih;
}

/* DrawText */
/* this call leave all the hard job of alignment on painter side */
void PPainterIup::DrawText(int inX, int inY, short align, const char *inString)
{
  if (!_cddbuffer)
    return;

  cdCanvasTextAlignment(_cddbuffer, align);
  cdCanvasText(_cddbuffer, inX, cdCanvasInvertYAxis(_cddbuffer, inY), const_cast<char *>(inString));
}

/* DrawRotatedText */
void PPainterIup::DrawRotatedText(int inX, int inY, float inDegrees, short align, const char *inString)
{
  double aprev;

  if (!_cddbuffer)
    return;

  cdCanvasTextAlignment(_cddbuffer, align);
  aprev = cdCanvasTextOrientation(_cddbuffer, -inDegrees);
  cdCanvasText(_cddbuffer, inX, cdCanvasInvertYAxis(_cddbuffer, inY), const_cast<char *>(inString));
  cdCanvasTextOrientation(_cddbuffer, aprev);
}

void PPainterIup::SetStyle(const PStyle &inStyle)
{
  if (!_cddbuffer)
    return;

  cdCanvasLineWidth(_cddbuffer, inStyle.mPenWidth);
  cdCanvasLineStyle(_cddbuffer, inStyle.mPenStyle);

  cdCanvasNativeFont(_cddbuffer, IupGetAttribute(_ih, "FONT"));

  if (inStyle.mFontStyle != -1 || inStyle.mFontSize != 0)
    cdCanvasFont(_cddbuffer, NULL, inStyle.mFontStyle, inStyle.mFontSize);

  cdCanvasMarkType(_cddbuffer, inStyle.mMarkStyle);
  cdCanvasMarkSize(_cddbuffer, inStyle.mMarkSize);
}

int iPPlotMapMethod(Ihandle* ih)
{
  int old_gdi = 0;

  if (IupGetInt(ih, "USE_GDI+"))
    old_gdi = cdUseContextPlus(1);

  ih->data->plt->_cdcanvas = cdCreateCanvas(CD_IUP, ih);
  if (!ih->data->plt->_cdcanvas)
    return IUP_ERROR;

  /* this can fail if canvas size is zero */
  if (IupGetInt(ih, "USE_IMAGERGB"))
    ih->data->plt->_cddbuffer = cdCreateCanvas(CD_DBUFFERRGB, ih->data->plt->_cdcanvas);
  else
    ih->data->plt->_cddbuffer = cdCreateCanvas(CD_DBUFFER, ih->data->plt->_cdcanvas);

  if (IupGetInt(ih, "USE_GDI+"))
    cdUseContextPlus(old_gdi);

  ih->data->plt->_redraw = 1;

  return IUP_NOERROR;
}

void iPPlotDestroyMethod(Ihandle* ih)
{
  delete ih->data->plt;
}

int iPPlotCreateMethod(Ihandle* ih, void **params)
{
  (void)params;

  /* free the data alocated by IupCanvas */
  if (ih->data) free(ih->data);
  ih->data = iupALLOCCTRLDATA();

  /* Initializing object with no cd canvases */
  ih->data->plt = new PPainterIup(ih);

  /* IupCanvas callbacks */
  IupSetCallback(ih, "ACTION",      (Icallback)iPPlotPaint_CB);
  IupSetCallback(ih, "RESIZE_CB",   (Icallback)iPPlotResize_CB);
  IupSetCallback(ih, "BUTTON_CB",   (Icallback)iPPlotMouseButton_CB);
  IupSetCallback(ih, "MOTION_CB",   (Icallback)iPPlotMouseMove_CB);
  IupSetCallback(ih, "KEYPRESS_CB", (Icallback)iPPlotKeyPress_CB);

  return IUP_NOERROR;
}

static Iclass* iupPPlotGetClass(void)
{
  Iclass* ic = iupClassNew(iupCanvasGetClass());

  ic->name = "pplot";
  ic->format = NULL;  /* none */
  ic->nativetype = IUP_TYPECANVAS;
  ic->childtype = IUP_CHILDNONE;
  ic->is_interactive = 1;
  ic->has_attrib_id = 1;  /* has attributes with IDs that must be parsed */

  /* Class functions */
  ic->Create  = iPPlotCreateMethod;
  ic->Destroy = iPPlotDestroyMethod;
  ic->Map     = iPPlotMapMethod;

  /* IupPPlot Callbacks */
  iupClassRegisterCallback(ic, "POSTDRAW_CB", "v");
  iupClassRegisterCallback(ic, "PREDRAW_CB", "v");
  iupClassRegisterCallback(ic, "DELETE_CB", "iiff");
  iupClassRegisterCallback(ic, "DELETEBEGIN_CB", "");
  iupClassRegisterCallback(ic, "DELETEEND_CB", "");
  iupClassRegisterCallback(ic, "SELECT_CB", "iiffi");
  iupClassRegisterCallback(ic, "SELECTBEGIN_CB", "");
  iupClassRegisterCallback(ic, "SELECTEND_CB", "");
  iupClassRegisterCallback(ic, "EDIT_CB", "iiffvv");
  iupClassRegisterCallback(ic, "EDITBEGIN_CB", "");
  iupClassRegisterCallback(ic, "EDITEND_CB", "");

  /* Visual */
  iupClassRegisterAttribute(ic, "BGCOLOR", iPPlotGetBGColorAttrib, iPPlotSetBGColorAttrib, "255 255 255", IUP_NOT_MAPPED, IUP_INHERIT);
  iupClassRegisterAttribute(ic, "FGCOLOR", iPPlotGetFGColorAttrib, iPPlotSetFGColorAttrib, "0 0 0", IUP_NOT_MAPPED, IUP_INHERIT);

  /* IupPPlot only */

  iupClassRegisterAttribute(ic, "REDRAW", iupBaseNoGetAttrib, iPPlotSetRedrawAttrib, NULL, IUP_MAPPED, IUP_NO_INHERIT);
  iupClassRegisterAttribute(ic, "TITLE", iPPlotGetTitleAttrib, iPPlotSetTitleAttrib, NULL, IUP_NOT_MAPPED, IUP_NO_INHERIT);
  iupClassRegisterAttribute(ic, "TITLEFONTSIZE", iPPlotGetTitleFontSizeAttrib, iPPlotSetTitleFontSizeAttrib, NULL, IUP_NOT_MAPPED, IUP_NO_INHERIT);
  iupClassRegisterAttribute(ic, "TITLEFONTSTYLE", NULL, iPPlotSetTitleFontStyleAttrib, NULL, IUP_NOT_MAPPED, IUP_NO_INHERIT);
  iupClassRegisterAttribute(ic, "LEGENDSHOW", iPPlotGetLegendShowAttrib, iPPlotSetLegendShowAttrib, "NO", IUP_NOT_MAPPED, IUP_NO_INHERIT);
  iupClassRegisterAttribute(ic, "LEGENDPOS", iPPlotGetLegendPosAttrib, iPPlotSetLegendPosAttrib, "TOPRIGHT", IUP_NOT_MAPPED, IUP_NO_INHERIT);
  iupClassRegisterAttribute(ic, "LEGENDFONTSIZE", NULL, iPPlotSetLegendFontSizeAttrib, NULL, IUP_NOT_MAPPED, IUP_NO_INHERIT);
  iupClassRegisterAttribute(ic, "LEGENDFONTSTYLE", NULL, iPPlotSetLegendFontStyleAttrib, NULL, IUP_NOT_MAPPED, IUP_NO_INHERIT);
  iupClassRegisterAttribute(ic, "MARGINLEFT", iPPlotGetMarginLeftAttrib, iPPlotSetMarginLeftAttrib, NULL, IUP_NOT_MAPPED, IUP_NO_INHERIT);
  iupClassRegisterAttribute(ic, "MARGINRIGHT", iPPlotGetMarginRightAttrib, iPPlotSetMarginRightAttrib, NULL, IUP_NOT_MAPPED, IUP_NO_INHERIT);
  iupClassRegisterAttribute(ic, "MARGINTOP", iPPlotGetMarginTopAttrib, iPPlotSetMarginTopAttrib, NULL, IUP_NOT_MAPPED, IUP_NO_INHERIT);
  iupClassRegisterAttribute(ic, "MARGINBOTTOM", iPPlotGetMarginBottomAttrib, iPPlotSetMarginBottomAttrib, NULL, IUP_NOT_MAPPED, IUP_NO_INHERIT);
  iupClassRegisterAttribute(ic, "GRIDLINESTYLE", iPPlotGetGridLineStyleAttrib, iPPlotSetGridLineStyleAttrib, "CONTINUOUS", IUP_NOT_MAPPED, IUP_NO_INHERIT);
  iupClassRegisterAttribute(ic, "GRIDCOLOR", iPPlotGetGridColorAttrib, iPPlotSetGridColorAttrib, "200 200 200", IUP_NOT_MAPPED, IUP_NO_INHERIT);
  iupClassRegisterAttribute(ic, "GRID", iPPlotGetGridAttrib, iPPlotSetGridAttrib, "NO", IUP_NOT_MAPPED, IUP_NO_INHERIT);

  iupClassRegisterAttribute(ic, "DS_LINESTYLE", iPPlotGetDSLineStyleAttrib, iPPlotSetDSLineStyleAttrib, "CONTINUOUS", IUP_NOT_MAPPED, IUP_NO_INHERIT);
  iupClassRegisterAttribute(ic, "DS_LINEWIDTH", iPPlotGetDSLineWidthAttrib, iPPlotSetDSLineWidthAttrib, "1", IUP_NOT_MAPPED, IUP_NO_INHERIT);
  iupClassRegisterAttribute(ic, "DS_MARKSTYLE", iPPlotGetDSMarkStyleAttrib, iPPlotSetDSMarkStyleAttrib, "X", IUP_NOT_MAPPED, IUP_NO_INHERIT);
  iupClassRegisterAttribute(ic, "DS_MARKSIZE", iPPlotGetDSMarkSizeAttrib, iPPlotSetDSMarkSizeAttrib, "7", IUP_NOT_MAPPED, IUP_NO_INHERIT);
  iupClassRegisterAttribute(ic, "DS_LEGEND", iPPlotGetDSLegendAttrib, iPPlotSetDSLegendAttrib, NULL, IUP_NOT_MAPPED, IUP_NO_INHERIT);
  iupClassRegisterAttribute(ic, "DS_COLOR", iPPlotGetDSColorAttrib, iPPlotSetDSColorAttrib, NULL, IUP_NOT_MAPPED, IUP_NO_INHERIT);
  iupClassRegisterAttribute(ic, "DS_SHOWVALUES", iPPlotGetDSShowValuesAttrib, iPPlotSetDSShowValuesAttrib, "NO", IUP_NOT_MAPPED, IUP_NO_INHERIT);
  iupClassRegisterAttribute(ic, "DS_MODE", iPPlotGetDSModeAttrib, iPPlotSetDSModeAttrib, "LINE", IUP_NOT_MAPPED, IUP_NO_INHERIT);
  iupClassRegisterAttribute(ic, "DS_EDIT", iPPlotGetDSEditAttrib, iPPlotSetDSEditAttrib, "NO", IUP_NOT_MAPPED, IUP_NO_INHERIT);
  iupClassRegisterAttribute(ic, "DS_REMOVE", NULL, iPPlotSetDSRemoveAttrib, NULL, IUP_NOT_MAPPED, IUP_NO_INHERIT);

  iupClassRegisterAttribute(ic, "AXS_XLABEL", iPPlotGetAXSXLabelAttrib, iPPlotSetAXSXLabelAttrib, NULL, IUP_NOT_MAPPED, IUP_NO_INHERIT);
  iupClassRegisterAttribute(ic, "AXS_YLABEL", iPPlotGetAXSYLabelAttrib, iPPlotSetAXSYLabelAttrib, NULL, IUP_NOT_MAPPED, IUP_NO_INHERIT);
  iupClassRegisterAttribute(ic, "AXS_XLABELCENTERED", iPPlotGetAXSXLabelCenteredAttrib, iPPlotSetAXSXLabelCenteredAttrib, "YES", IUP_NOT_MAPPED, IUP_NO_INHERIT);
  iupClassRegisterAttribute(ic, "AXS_YLABELCENTERED", iPPlotGetAXSYLabelCenteredAttrib, iPPlotSetAXSYLabelCenteredAttrib, "YES", IUP_NOT_MAPPED, IUP_NO_INHERIT);
  iupClassRegisterAttribute(ic, "AXS_XCOLOR", iPPlotGetAXSXColorAttrib, iPPlotSetAXSXColorAttrib, "0 0 0", IUP_NOT_MAPPED, IUP_NO_INHERIT);
  iupClassRegisterAttribute(ic, "AXS_YCOLOR", iPPlotGetAXSYColorAttrib, iPPlotSetAXSYColorAttrib, "0 0 0", IUP_NOT_MAPPED, IUP_NO_INHERIT);
  iupClassRegisterAttribute(ic, "AXS_XAUTOMIN", iPPlotGetAXSXAutoMinAttrib, iPPlotSetAXSXAutoMinAttrib, "YES", IUP_NOT_MAPPED, IUP_NO_INHERIT);
  iupClassRegisterAttribute(ic, "AXS_YAUTOMIN", iPPlotGetAXSYAutoMinAttrib, iPPlotSetAXSYAutoMinAttrib, "YES", IUP_NOT_MAPPED, IUP_NO_INHERIT);
  iupClassRegisterAttribute(ic, "AXS_XAUTOMAX", iPPlotGetAXSXAutoMaxAttrib, iPPlotSetAXSXAutoMaxAttrib, "YES", IUP_NOT_MAPPED, IUP_NO_INHERIT);
  iupClassRegisterAttribute(ic, "AXS_YAUTOMAX", iPPlotGetAXSYAutoMaxAttrib, iPPlotSetAXSYAutoMaxAttrib, "YES", IUP_NOT_MAPPED, IUP_NO_INHERIT);
  iupClassRegisterAttribute(ic, "AXS_XMIN", iPPlotGetAXSXMinAttrib, iPPlotSetAXSXMinAttrib, NULL, IUP_NOT_MAPPED, IUP_NO_INHERIT);
  iupClassRegisterAttribute(ic, "AXS_YMIN", iPPlotGetAXSYMinAttrib, iPPlotSetAXSYMinAttrib, NULL, IUP_NOT_MAPPED, IUP_NO_INHERIT);
  iupClassRegisterAttribute(ic, "AXS_XMAX", iPPlotGetAXSXMaxAttrib, iPPlotSetAXSXMaxAttrib, NULL, IUP_NOT_MAPPED, IUP_NO_INHERIT);
  iupClassRegisterAttribute(ic, "AXS_YMAX", iPPlotGetAXSYMaxAttrib, iPPlotSetAXSYMaxAttrib, NULL, IUP_NOT_MAPPED, IUP_NO_INHERIT);
  iupClassRegisterAttribute(ic, "AXS_XREVERSE", iPPlotGetAXSXReverseAttrib, iPPlotSetAXSXReverseAttrib, "NO", IUP_NOT_MAPPED, IUP_NO_INHERIT);
  iupClassRegisterAttribute(ic, "AXS_YREVERSE", iPPlotGetAXSYReverseAttrib, iPPlotSetAXSYReverseAttrib, "NO", IUP_NOT_MAPPED, IUP_NO_INHERIT);
  iupClassRegisterAttribute(ic, "AXS_XCROSSORIGIN", iPPlotGetAXSXCrossOriginAttrib, iPPlotSetAXSXCrossOriginAttrib, "YES", IUP_NOT_MAPPED, IUP_NO_INHERIT);
  iupClassRegisterAttribute(ic, "AXS_YCROSSORIGIN", iPPlotGetAXSYCrossOriginAttrib, iPPlotSetAXSYCrossOriginAttrib, "YES", IUP_NOT_MAPPED, IUP_NO_INHERIT);
  iupClassRegisterAttribute(ic, "AXS_XSCALE", iPPlotGetAXSXScaleAttrib, iPPlotSetAXSXScaleAttrib, "LIN", IUP_NOT_MAPPED, IUP_NO_INHERIT);
  iupClassRegisterAttribute(ic, "AXS_YSCALE", iPPlotGetAXSYScaleAttrib, iPPlotSetAXSYScaleAttrib, "LIN", IUP_NOT_MAPPED, IUP_NO_INHERIT);
  iupClassRegisterAttribute(ic, "AXS_XFONTSIZE", iPPlotGetAXSXFontSizeAttrib, iPPlotSetAXSXFontSizeAttrib, NULL, IUP_NOT_MAPPED, IUP_NO_INHERIT);
  iupClassRegisterAttribute(ic, "AXS_YFONTSIZE", iPPlotGetAXSYFontSizeAttrib, iPPlotSetAXSYFontSizeAttrib, NULL, IUP_NOT_MAPPED, IUP_NO_INHERIT);
  iupClassRegisterAttribute(ic, "AXS_XFONTSTYLE", iPPlotGetAXSXFontStyleAttrib, iPPlotSetAXSXFontStyleAttrib, NULL, IUP_NOT_MAPPED, IUP_NO_INHERIT);
  iupClassRegisterAttribute(ic, "AXS_YFONTSTYLE", iPPlotGetAXSYFontStyleAttrib, iPPlotSetAXSYFontStyleAttrib, NULL, IUP_NOT_MAPPED, IUP_NO_INHERIT);
  iupClassRegisterAttribute(ic, "AXS_XTICK", iPPlotGetAXSXTickAttrib, iPPlotSetAXSXTickAttrib, "YES", IUP_NOT_MAPPED, IUP_NO_INHERIT);
  iupClassRegisterAttribute(ic, "AXS_YTICK", iPPlotGetAXSYTickAttrib, iPPlotSetAXSYTickAttrib, "YES", IUP_NOT_MAPPED, IUP_NO_INHERIT);
  iupClassRegisterAttribute(ic, "AXS_XTICKSIZE", iPPlotGetAXSXTickSizeAttrib, iPPlotSetAXSXTickSizeAttrib, "5", IUP_NOT_MAPPED, IUP_NO_INHERIT);
  iupClassRegisterAttribute(ic, "AXS_YTICKSIZE", iPPlotGetAXSYTickSizeAttrib, iPPlotSetAXSYTickSizeAttrib, "5", IUP_NOT_MAPPED, IUP_NO_INHERIT);
  iupClassRegisterAttribute(ic, "AXS_XTICKFORMAT", iPPlotGetAXSXTickFormatAttrib, iPPlotSetAXSXTickFormatAttrib, "%.0f", IUP_NOT_MAPPED, IUP_NO_INHERIT);
  iupClassRegisterAttribute(ic, "AXS_YTICKFORMAT", iPPlotGetAXSYTickFormatAttrib, iPPlotSetAXSYTickFormatAttrib, "%.0f", IUP_NOT_MAPPED, IUP_NO_INHERIT);
  iupClassRegisterAttribute(ic, "AXS_XTICKFONTSIZE", iPPlotGetAXSXTickFontSizeAttrib, iPPlotSetAXSXTickFontSizeAttrib, NULL, IUP_NOT_MAPPED, IUP_NO_INHERIT);
  iupClassRegisterAttribute(ic, "AXS_YTICKFONTSIZE", iPPlotGetAXSYTickFontSizeAttrib, iPPlotSetAXSYTickFontSizeAttrib, NULL, IUP_NOT_MAPPED, IUP_NO_INHERIT);
  iupClassRegisterAttribute(ic, "AXS_XTICKFONTSTYLE", iPPlotGetAXSXTickFontStyleAttrib, iPPlotSetAXSXTickFontStyleAttrib, NULL, IUP_NOT_MAPPED, IUP_NO_INHERIT);
  iupClassRegisterAttribute(ic, "AXS_YTICKFONTSTYLE", iPPlotGetAXSYTickFontStyleAttrib, iPPlotSetAXSYTickFontStyleAttrib, NULL, IUP_NOT_MAPPED, IUP_NO_INHERIT);
  iupClassRegisterAttribute(ic, "AXS_XAUTOTICK", iPPlotGetAXSXAutoTickAttrib, iPPlotSetAXSXAutoTickAttrib, "YES", IUP_NOT_MAPPED, IUP_NO_INHERIT);
  iupClassRegisterAttribute(ic, "AXS_YAUTOTICK", iPPlotGetAXSYAutoTickAttrib, iPPlotSetAXSYAutoTickAttrib, "YES", IUP_NOT_MAPPED, IUP_NO_INHERIT);
  iupClassRegisterAttribute(ic, "AXS_XAUTOTICKSIZE", iPPlotGetAXSXAutoTickSizeAttrib, iPPlotSetAXSXAutoTickSizeAttrib, "5", IUP_NOT_MAPPED, IUP_NO_INHERIT);
  iupClassRegisterAttribute(ic, "AXS_YAUTOTICKSIZE", iPPlotGetAXSYAutoTickSizeAttrib, iPPlotSetAXSYAutoTickSizeAttrib, "5", IUP_NOT_MAPPED, IUP_NO_INHERIT);
  iupClassRegisterAttribute(ic, "AXS_XTICKMAJORSPAN", iPPlotGetAXSXTickMajorSpanAttrib, iPPlotSetAXSXTickMajorSpanAttrib, "1", IUP_NOT_MAPPED, IUP_NO_INHERIT);
  iupClassRegisterAttribute(ic, "AXS_YTICKMAJORSPAN", iPPlotGetAXSYTickMajorSpanAttrib, iPPlotSetAXSYTickMajorSpanAttrib, "1", IUP_NOT_MAPPED, IUP_NO_INHERIT);
  iupClassRegisterAttribute(ic, "AXS_XTICKDIVISION", iPPlotGetAXSXTickDivisionAttrib, iPPlotSetAXSXTickDivisionAttrib, "5", IUP_NOT_MAPPED, IUP_NO_INHERIT);
  iupClassRegisterAttribute(ic, "AXS_YTICKDIVISION", iPPlotGetAXSYTickDivisionAttrib, iPPlotSetAXSYTickDivisionAttrib, "5", IUP_NOT_MAPPED, IUP_NO_INHERIT);
  iupClassRegisterAttribute(ic, "AXS_XAUTOTICKSIZE", iPPlotGetAXSXAutoTickSizeAttrib, iPPlotSetAXSXAutoTickSizeAttrib, "YES", IUP_NOT_MAPPED, IUP_NO_INHERIT);
  iupClassRegisterAttribute(ic, "AXS_YAUTOTICKSIZE", iPPlotGetAXSYAutoTickSizeAttrib, iPPlotSetAXSYAutoTickSizeAttrib, "YES", IUP_NOT_MAPPED, IUP_NO_INHERIT);
  iupClassRegisterAttribute(ic, "AXS_XTICKMAJORSIZE", iPPlotGetAXSXTickMajorSizeAttrib, iPPlotSetAXSXTickMajorSizeAttrib, "8", IUP_NOT_MAPPED, IUP_NO_INHERIT);
  iupClassRegisterAttribute(ic, "AXS_YTICKMAJORSIZE", iPPlotGetAXSYTickMajorSizeAttrib, iPPlotSetAXSYTickMajorSizeAttrib, "8", IUP_NOT_MAPPED, IUP_NO_INHERIT);

  iupClassRegisterAttribute(ic, "REMOVE", iupBaseNoGetAttrib, iPPlotSetRemoveAttrib, NULL, IUP_NOT_MAPPED, IUP_NO_INHERIT);
  iupClassRegisterAttribute(ic, "CLEAR", iupBaseNoGetAttrib, iPPlotSetClearAttrib, NULL, IUP_NOT_MAPPED, IUP_NO_INHERIT);
  iupClassRegisterAttribute(ic, "COUNT", iPPlotGetCountAttrib, iupBaseNoSetAttrib, NULL, IUP_NOT_MAPPED, IUP_NO_INHERIT);
  iupClassRegisterAttribute(ic, "CURRENT", iPPlotGetCurrentAttrib, iPPlotSetCurrentAttrib, "-1", IUP_NOT_MAPPED, IUP_NO_INHERIT);

  return ic;
}

/* user level call: create control */
Ihandle* IupPPlot(void)
{
  return IupCreate("pplot");
}

void IupPPlotOpen(void)
{
  iupRegisterClass(iupPPlotGetClass());
}
