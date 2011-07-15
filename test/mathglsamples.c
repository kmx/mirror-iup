/*
MathGL samples w/ IupMglPlot
Based on: MathGL documentation (v. 1.10), Cap. 9
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>


#include "iup.h"
#include "iupcontrols.h"
#include "iup_mglplot.h"
#include "iupkey.h"

#include <cd.h>
#include <cdiup.h>

static Ihandle *plot;

void ResetClear(void)
{
  IupSetAttribute(plot, "RESET", NULL);
  IupSetAttribute(plot, "CLEAR", NULL);

  // Some defaults in MathGL are different in IupMglPlot
  IupSetAttribute(plot, "AXS_X", "NO");
  IupSetAttribute(plot, "AXS_Y", "NO");
  IupSetAttribute(plot, "AXS_Z", "NO");
}

static int postdraw_cb(Ihandle* ih, cdCanvas* cnv)
{
  (void)cnv;
  (void)ih;
  IupMglPlotDrawText(plot, "This is very long string drawn along a curve", 0, (float)sin(2*3.14*1), 0);
  IupMglPlotDrawText(plot, "Another string drawn above a curve", 0, (float)cos(2*3.14*2), 0);
  return IUP_DEFAULT;
}

void SampleOne3D(void)
{
  IupMglPlotNewDataSet(plot, 1);
  IupMglPlotSetFromFormula(plot, 0, "-2*((2*x-1)^2 + (2*y-1)^2 + (2*z-1)^4 - (2*z-1)^2 - 0.1)", 60, 50, 40);

  IupSetAttribute(plot, "ROTATE", "40:0:60");
  IupSetAttribute(plot, "LIGHT", "YES");
  IupSetAttribute(plot, "TRANSPARENT", "YES");
  IupSetAttribute(plot, "BOX", "YES");
  IupSetAttribute(plot, "DS_MODE", "VOLUME_ISOSURFACE");
}

void SampleOne2D(void)
{
  IupMglPlotNewDataSet(plot, 1);
  IupMglPlotSetFromFormula(plot, 0, "0.6*sin(2*pi*x)*sin(3*pi*y) + 0.4*cos(3*pi*(x*y))", 50, 40, 1);

  IupSetAttribute(plot, "ROTATE", "40:0:60");
  IupSetAttribute(plot, "LIGHT", "YES");
  IupSetAttribute(plot, "BOX", "YES");
  IupSetAttribute(plot, "DS_MODE", "PLANAR_SURFACE");
}

void SamplePie1D(void)
{
//   int i;
// 
//   for(i=0; i < 2; i++)
//   {
//     IupMglPlotNewDataSet(plot, 1);
//     IupMglPlotSetFromFormula(plot, i, "rnd+0.1", 7, 1, 1);
//     IupSetAttribute(plot, "DS_MODE", "PIECHART");
//     // gr->Chart(ch,"bgr cmy#");  /* DS_COLOR?? => Blue(0), Green(1), Red(2), Transparent(3), Cyan(4), Magenta(5), Yellow(6) */
//   }

  IupMglPlotNewDataSet(plot, 1);
  IupMglPlotSetFromFormula(plot, 0, "rnd+0.1", 7, 2, 1);
  IupSetAttribute(plot, "DS_MODE", "PIECHART");

  IupSetAttribute(plot, "ROTATE", "40:60:0");
  IupSetAttribute(plot, "LIGHT", "YES");
  IupSetAttribute(plot, "BOX", "YES");

  IupSetAttribute(plot, "BOX", "YES");
}

void SampleChart1D(void)
{
//   int i;
//   
//   for(i=0; i < 2; i++)
//   {
//     IupMglPlotNewDataSet(plot, 1);
//     IupMglPlotSetFromFormula(plot, i, "rnd+0.1", 7, 1, 1);
//     IupSetAttribute(plot, "DS_MODE", "CHART");
//   }

  IupMglPlotNewDataSet(plot, 1);
  IupMglPlotSetFromFormula(plot, 0, "rnd+0.1", 7, 2, 1);
  IupSetAttribute(plot, "DS_MODE", "CHART");

  IupSetAttribute(plot, "ROTATE", "40:60:0");
  IupSetAttribute(plot, "LIGHT", "YES");
  IupSetAttribute(plot, "BOX", "YES");

  IupSetAttribute(plot, "BOX", "YES");
}

void SampleText1D(void)
{
  IupMglPlotNewDataSet(plot, 1);
  IupMglPlotSetFromFormula(plot, 0, "0.7*sin(2*pi*x) + 0.5*cos(3*pi*x) + 0.2*sin(pi*x)", 50, 1, 1);

  IupMglPlotNewDataSet(plot, 1);
  IupMglPlotSetFromFormula(plot, 1, "sin(2*pi*x)", 50, 1, 1);

  IupMglPlotNewDataSet(plot, 1);
  IupMglPlotSetFromFormula(plot, 2, "cos(2*pi*x)", 50, 1, 1);

//  IupSetCallback(plot, "POSTDRAW_CB", (Icallback)postdraw_cb);

  IupSetAttribute(plot, "BOX", "YES");

  /* how can I do this? */
  // gr->Plot(y.SubData(-1,0));
  // gr->Text(y,"This is very long string drawn along a curve",":k");
  // gr->Text(y,"Another string drawn above a curve","T:r");
}

void SampleTextMark1D(void)
{
  IupMglPlotNewDataSet(plot, 1);
  IupMglPlotSetFromFormula(plot, 0, "0.7*sin(2*pi*x) + 0.5*cos(3*pi*x) + 0.2*sin(pi*x)", 50, 1, 1);
  IupSetAttribute(plot, "DS_MODE", "MARK");
  IupSetAttribute(plot, "DS_COLOR", "0 0 0");

  IupMglPlotNewDataSet(plot, 1);
  IupMglPlotSetFromFormula(plot, 1, "sin(2*pi*x)", 50, 1, 1);
  IupSetAttribute(plot, "DS_MODE", "MARK");
  IupSetAttribute(plot, "DS_COLOR", "0 0 0");

  IupMglPlotNewDataSet(plot, 1);
  IupMglPlotSetFromFormula(plot, 2, "cos(2*pi*x)", 50, 1, 1);
  IupSetAttribute(plot, "DS_MODE", "MARK");
  IupSetAttribute(plot, "DS_COLOR", "0 0 0");

  // y1.Modify("0.5+0.3*cos(2*pi*x)");  /* Pelo que entendi, essa conta tem rela��o com o tamanho do markstyle */
  // gr->TextMark(y,y1,"\\gamma");  /* IupMglPlotDrawMark ??? */

  IupSetAttribute(plot, "BOX", "YES");
}

void SampleMark1D(void)
{
  IupMglPlotNewDataSet(plot, 1);
  IupMglPlotSetFromFormula(plot, 0, "0.7*sin(2*pi*x) + 0.5*cos(3*pi*x) + 0.2*sin(pi*x)", 50, 1, 1);
  IupSetAttribute(plot, "DS_MODE", "MARK");
  IupSetAttribute(plot, "DS_MARKSTYLE", "HOLLOW_BOX");
  IupSetAttribute(plot, "DS_COLOR", "0 0 255");

  IupMglPlotNewDataSet(plot, 1);
  IupMglPlotSetFromFormula(plot, 1, "sin(2*pi*x)", 50, 1, 1);
  IupSetAttribute(plot, "DS_MODE", "MARK");
  IupSetAttribute(plot, "DS_MARKSTYLE", "HOLLOW_BOX");
  IupSetAttribute(plot, "DS_COLOR", "0 0 255");

  IupMglPlotNewDataSet(plot, 1);
  IupMglPlotSetFromFormula(plot, 2, "cos(2*pi*x)", 50, 1, 1);
  IupSetAttribute(plot, "DS_MODE", "MARK");
  IupSetAttribute(plot, "DS_MARKSTYLE", "HOLLOW_BOX");
  IupSetAttribute(plot, "DS_COLOR", "0 0 255");
 
  // y1.Modify("0.5+0.3*cos(2*pi*x)");  /* Pelo que entendi, essa conta tem rela��o com o tamanho do markstyle */
  // gr->Mark(y,y1,"bs");  /* IupMglPlotDrawMark ??? */

  IupSetAttribute(plot, "BOX", "YES");
}

void SampleBoxPlot1D(void)
{
//   int i;
// 
//   for(i=0; i < 7; i++)
//   {
//     IupMglPlotNewDataSet(plot, 1);
//     IupMglPlotSetFromFormula(plot, i, "(2*rnd-1)^3/2", 10, 1, 1);
//     IupSetAttribute(plot, "DS_MODE", "BOXPLOT");
//   }

  IupMglPlotNewDataSet(plot, 1);
  IupMglPlotSetFromFormula(plot, 0, "(2*rnd-1)^3/2", 10, 7, 1);
  IupSetAttribute(plot, "DS_MODE", "BOXPLOT");

  IupSetAttribute(plot, "BOX", "YES");
}

void SampleStem1D(void)
{
  IupMglPlotNewDataSet(plot, 1);
  IupMglPlotSetFromFormula(plot, 0, "0.7*sin(2*pi*x) + 0.5*cos(3*pi*x) + 0.2*sin(pi*x)", 50, 1, 1);
  IupSetAttribute(plot, "DS_MODE", "STEM");

  IupMglPlotNewDataSet(plot, 1);
  IupMglPlotSetFromFormula(plot, 1, "sin(2*pi*x)", 50, 1, 1);
  IupSetAttribute(plot, "DS_MODE", "STEM");

  IupMglPlotNewDataSet(plot, 1);
  IupMglPlotSetFromFormula(plot, 2, "cos(2*pi*x)", 50, 1, 1);
  IupSetAttribute(plot, "DS_MODE", "STEM");

  IupSetAttribute(plot, "BOX", "YES");

  IupSetAttribute(plot, "AXS_XORIGIN", "0.0");
  IupSetAttribute(plot, "AXS_YORIGIN", "0.0");
  IupSetAttribute(plot, "AXS_ZORIGIN", "0.0");

  IupSetAttribute(plot, "BOX", "YES");
}

void SampleStep1D(void)
{
  IupMglPlotNewDataSet(plot, 1);
  IupMglPlotSetFromFormula(plot, 0, "0.7*sin(2*pi*x) + 0.5*cos(3*pi*x) + 0.2*sin(pi*x)", 50, 1, 1);
  IupSetAttribute(plot, "DS_MODE", "STEP");

  IupMglPlotNewDataSet(plot, 1);
  IupMglPlotSetFromFormula(plot, 1, "sin(2*pi*x)", 50, 1, 1);
  IupSetAttribute(plot, "DS_MODE", "STEP");

  IupMglPlotNewDataSet(plot, 1);
  IupMglPlotSetFromFormula(plot, 2, "cos(2*pi*x)", 50, 1, 1);
  IupSetAttribute(plot, "DS_MODE", "STEP");

  IupSetAttribute(plot, "BOX", "YES");
}

void SampleBarh1D(void)
{
//   int i;
//   
//   for(i=0; i < 3; i++)
//   {
//     IupMglPlotNewDataSet(plot, 1);
//     IupMglPlotSetFromFormula(plot, i, "0.8*sin(pi*(2*x+y/2))+0.2*rnd", 10, 1, 1);
//     IupSetAttribute(plot, "DS_MODE", "BARHORIZONTAL");
//   }

  IupMglPlotNewDataSet(plot, 1);
  IupMglPlotSetFromFormula(plot, 0, "0.8*sin(pi*(2*x+y/2))+0.2*rnd", 10, 3, 1);
  IupSetAttribute(plot, "DS_MODE", "BARHORIZONTAL");

  IupSetAttribute(plot, "AXS_XORIGIN", "0.0");
  IupSetAttribute(plot, "AXS_YORIGIN", "0.0");
  IupSetAttribute(plot, "AXS_ZORIGIN", "0.0");

  IupSetAttribute(plot, "BOX", "YES");
}

void SampleBars1D(void)
{
//   int i;
//   
//   for(i=0; i < 3; i++)
//   {
//     IupMglPlotNewDataSet(plot, 1);
//     IupMglPlotSetFromFormula(plot, i, "0.8*sin(pi*(2*x+y/2))+0.2*rnd", 10, 1, 1);
//     IupSetAttribute(plot, "DS_MODE", "BAR");
//   }

  IupMglPlotNewDataSet(plot, 1);
  IupMglPlotSetFromFormula(plot, 0, "0.8*sin(pi*(2*x+y/2))+0.2*rnd", 10, 3, 1);
  IupSetAttribute(plot, "DS_MODE", "BAR");

  IupSetAttribute(plot, "AXS_XORIGIN", "0.0");
  IupSetAttribute(plot, "AXS_YORIGIN", "0.0");
  IupSetAttribute(plot, "AXS_ZORIGIN", "0.0");

  IupSetAttribute(plot, "BOX", "YES");
}

void SampleArea1D(void)
{
  IupMglPlotNewDataSet(plot, 1);
  IupMglPlotSetFromFormula(plot, 0, "0.7*sin(2*pi*x) + 0.5*cos(3*pi*x) + 0.2*sin(pi*x)", 50, 1, 1);
  IupSetAttribute(plot, "DS_MODE", "AREA");

  IupMglPlotNewDataSet(plot, 1);
  IupMglPlotSetFromFormula(plot, 1, "sin(2*pi*x)", 50, 1, 1);
  IupSetAttribute(plot, "DS_MODE", "AREA");

  IupMglPlotNewDataSet(plot, 1);
  IupMglPlotSetFromFormula(plot, 2, "cos(2*pi*x)", 50, 1, 1);
  IupSetAttribute(plot, "DS_MODE", "AREA");
  
  IupSetAttribute(plot, "AXS_XORIGIN", "0.0");
  IupSetAttribute(plot, "AXS_YORIGIN", "0.0");
  IupSetAttribute(plot, "AXS_ZORIGIN", "0.0");
  
  IupSetAttribute(plot, "BOX", "YES");
}

void SampleRadar1D(void)
{
//   int i;
//   
//   for(i=0; i < 3; i++)
//   {
//     IupMglPlotNewDataSet(plot, 1);
//     IupMglPlotSetFromFormula(plot, i, "0.4*sin(pi*(2*x+y/2))+0.1*rnd", 10, 1, 1);
//     IupSetAttribute(plot, "DS_MODE", "RADAR");
//   }

  IupMglPlotNewDataSet(plot, 1);
  IupMglPlotSetFromFormula(plot, 0, "0.4*sin(pi*(2*x+y/2))+0.1*rnd", 10, 3, 1);
  IupSetAttribute(plot, "DS_MODE", "RADAR");
}

void SamplePlot1D(void)
{
  IupMglPlotNewDataSet(plot, 1);
  IupMglPlotSetFromFormula(plot, 0, "0.7*sin(2*pi*x)+0.5*cos(3*pi*x)+0.2*sin(pi*x)", 50, 1, 1);

  IupMglPlotNewDataSet(plot, 1);
  IupMglPlotSetFromFormula(plot, 1, "sin(2*pi*x)", 50, 1, 1);
  
  IupMglPlotNewDataSet(plot, 1);
  IupMglPlotSetFromFormula(plot, 2, "cos(2*pi*x)", 50, 1, 1);

  IupSetAttribute(plot, "BOX", "YES");
}

typedef struct _TestItems{
  char* title;
  void (*func)(void);
}TestItems;

static TestItems test_list[] = {
  {"Plot 1D", SamplePlot1D},
  {"Radar 1D", SampleRadar1D},
  {"Area 1D", SampleArea1D},
  {"Bars 1D", SampleBars1D}, 
  {"Barh 1D", SampleBarh1D}, 
  {"Step 1D", SampleStep1D}, 
  {"Stem 1D", SampleStem1D}, 
  {"BoxPlot 1D", SampleBoxPlot1D}, 
  {"Mark 1D", SampleMark1D}, 
  {"TextMark 1D", SampleTextMark1D}, 
  {"Text 1D", SampleText1D}, 
  {"Chart 1D", SampleChart1D}, 
  {"Pie 1D", SamplePie1D}, 
  {"One 2D", SampleOne2D},
  {"One 3D", SampleOne3D},
};

static int k_enter_cb(Ihandle*ih)
{
  int pos = IupGetInt(ih, "VALUE");
  if (pos > 0)
  {
    ResetClear();
    test_list[pos-1].func();
    IupSetAttribute(plot, "REDRAW", NULL);
  }
  return IUP_DEFAULT;
}

static int action_cb(Ihandle *ih, char *text, int item, int state)
{
  (void)text;
  (void)state;
  (void)ih;

  ResetClear();
  test_list[item-1].func();
  IupSetAttribute(plot, "REDRAW", NULL);

  return IUP_DEFAULT;
}

static int close_cb(Ihandle *ih)
{
  (void)ih;
  return IUP_CLOSE;
}

int main(int argc, char* argv[])
{
  int i, count = sizeof(test_list)/sizeof(TestItems);
  char str[50];
  Ihandle *dlg, *list;

  IupOpen(&argc, &argv);
  IupControlsOpen();
  IupMglPlotOpen();     /* init IupMGLPlot library */

  IupSetGlobal("MGLFONTS", "../etc/mglfonts");

  list = IupList(NULL);
  plot = IupMglPlot();

  dlg = IupDialog(IupHbox(list, plot, NULL));
  IupSetAttribute(dlg, "MARGIN", "10x10");
  IupSetAttribute(dlg, "GAP", "10");
  IupSetAttribute(dlg, "TITLE", "MathGL samples w/ IupMglPlot");
  IupSetCallback(dlg, "CLOSE_CB", close_cb);

  IupSetAttribute(plot, "RASTERSIZE", "400x400");
//  IupSetAttribute(plot, "OPENGL", "NO");

  IupSetAttribute(list, "RASTERSIZE", "100x");
  IupSetAttribute(list, "EXPAND", "VERTICAL");
  IupSetAttribute(list, "VISIBLELINES", "15");
  IupSetCallback(list, "ACTION", (Icallback)action_cb);

  for (i=0; i<count; i++)
  {
    sprintf(str, "%d", i+1);
    IupSetAttribute(list, str, test_list[i].title);
  }

  IupShowXY(dlg, 100, IUP_CENTER);

  IupSetAttribute(plot, "RASTERSIZE", NULL);

  ResetClear();
  SamplePlot1D();
  IupSetAttribute(plot, "REDRAW", NULL);

  IupMainLoop();

  IupClose();

  return EXIT_SUCCESS;
}
