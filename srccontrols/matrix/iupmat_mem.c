/** \file
 * \brief iupmatrix control memory allocation
 *
 * See Copyright Notice in "iup.h"
 */

/**************************************************************************/
/* Functions to allocate memory                                           */
/**************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <string.h>

#include "iup.h"

#include <cd.h>

#include "iup_object.h"
#include "iup_attrib.h"
#include "iup_str.h"
#include "iup_stdcontrols.h"

#include "iupmat_def.h"
#include "iupmat_mem.h"


static void iMatrixGetInitialValues(Ihandle* ih)
{
  int lin, col;
  char* value;
  char attr[100];

  for (lin=0; lin<ih->data->lines.num; lin++)
  {
    for (col=0; col<ih->data->columns.num; col++)
    {
      sprintf(attr, "%d:%d", lin, col);
      value = iupAttribGet(ih, attr);
      if (value)
      {
        /* get the initial value and remove it from the hash table */

        if (*value)
          ih->data->cells[lin][col].value = iupStrDup(value);

        iupAttribSetStr(ih, attr, NULL);
      }
    }
  }
}

void iupMatrixMemAlloc(Ihandle* ih)
{
  if (!ih->data->callback_mode)
  {
    int lin;

    ih->data->lines.num_alloc = ih->data->lines.num;
    if (ih->data->lines.num_alloc == 1)
      ih->data->lines.num_alloc = 5;

    ih->data->columns.num_alloc = ih->data->columns.num;
    if (ih->data->columns.num_alloc == 1)
      ih->data->columns.num_alloc = 5;

    ih->data->cells = (ImatCell**)calloc(ih->data->lines.num_alloc, sizeof(ImatCell*));
    for (lin = 0; lin < ih->data->lines.num_alloc; lin++)
      ih->data->cells[lin] = (ImatCell*)calloc(ih->data->columns.num_alloc, sizeof(ImatCell));

    iMatrixGetInitialValues(ih);
  }

  ih->data->lines.marks = (unsigned char*)calloc(ih->data->lines.num_alloc, sizeof(unsigned char));
  ih->data->columns.marks = (unsigned char*)calloc(ih->data->columns.num_alloc, sizeof(unsigned char));
  ih->data->lines.sizes = (int*)calloc(ih->data->lines.num_alloc, sizeof(int));
  ih->data->columns.sizes = (int*)calloc(ih->data->columns.num_alloc, sizeof(int));
}

void iupMatrixMemRelease(Ihandle* ih)
{
  if (ih->data->cells)
  {
    int lin, col;
    for (lin = 0; lin < ih->data->lines.num_alloc; lin++)
    {
      for (col = 0; col < ih->data->columns.num_alloc; col++)
      {
        ImatCell* cell = &(ih->data->cells[lin][col]);
        if (cell->value)
        {
          free(cell->value);
          cell->value = NULL;
        }
      }
      free(ih->data->cells[lin]);
      ih->data->cells[lin] = NULL;
    }
    free(ih->data->cells);
    ih->data->cells = NULL;
  }

  if (ih->data->columns.marks)
  {
    free(ih->data->columns.marks);
    ih->data->columns.marks = NULL;
  }

  if (ih->data->lines.marks)
  {
    free(ih->data->lines.marks);
    ih->data->lines.marks = NULL;
  }

  if (ih->data->columns.sizes)
  {
    free(ih->data->columns.sizes);
    ih->data->columns.sizes = NULL;
  }

  if (ih->data->lines.sizes)
  {
    free(ih->data->lines.sizes);
    ih->data->lines.sizes = NULL;
  }
}

void iupMatrixMemReAllocLines(Ihandle* ih, int old_num, int num, int base)
{
  int lin, col, end, diff_num, shift_num;

  /* If it doesn't have enough lines allocated, then allocate more space */
  if (num > ih->data->lines.num_alloc)  /* this also implicates that num>old_num */
  {
    int old_alloc = ih->data->lines.num_alloc;

    ih->data->lines.num_alloc = num;

    if (!ih->data->callback_mode)
    {
      ih->data->cells = (ImatCell**)realloc(ih->data->cells, ih->data->lines.num_alloc*sizeof(ImatCell*));
      for(lin = old_alloc; lin < num; lin++)
        ih->data->cells[lin] = (ImatCell*)calloc(ih->data->columns.num_alloc, sizeof(ImatCell));
    }

    ih->data->lines.sizes = (int*)realloc(ih->data->lines.sizes, ih->data->lines.num_alloc*sizeof(int));
    ih->data->lines.marks = (unsigned char*)realloc(ih->data->lines.marks, ih->data->lines.num_alloc*sizeof(unsigned char));
  }

  if (old_num==num)
    return;

  if (num>old_num) /* ADD */
  {
    /* shift the old data, opening space for new data, from base to old_num */
    /*   do it in reverse order to avoid overlapping */
    /* then clear the new space starting at base */

    diff_num = num-old_num;      /* size of the openned space */
    shift_num = old_num-base;    /* size of the data to be moved */
    end = base+diff_num;

    for (lin = old_num-1; lin >= base; lin--)
      memmove(ih->data->cells[lin+diff_num], ih->data->cells[lin], ih->data->columns.num_alloc*sizeof(ImatCell));

    for (lin = base; lin < end; lin++)
      memset(ih->data->cells[lin], 0, ih->data->columns.num_alloc*sizeof(ImatCell));

    memmove(ih->data->lines.sizes+old_num, ih->data->lines.sizes+base, shift_num*sizeof(int));
    memmove(ih->data->lines.marks+old_num, ih->data->lines.marks+base, shift_num*sizeof(unsigned char));

    memset(ih->data->lines.sizes+base, 0, diff_num*sizeof(int));
    memset(ih->data->lines.marks+base, 0, diff_num*sizeof(unsigned char));
  }
  else /* DEL */
  {
    /* release memory from the opened space */
    /* move the old data to opened space from end to base */
    /* then clear the remaining space starting at num */

    diff_num = old_num-num;  /* size of the openned space */
    shift_num = num-base;    /* size of the data to be moved */
    end = base+diff_num;

    for(lin = base; lin < end; lin++)
    {
      for (col = 0; col < ih->data->columns.num_alloc; col++)
      {
        ImatCell* cell = &(ih->data->cells[lin][col]);
        if (cell->value)
        {
          free(cell->value);
          cell->value = NULL;
          cell->mark = 0;
        }
      }
    }

    for (lin = end; lin < old_num; lin++)
      memmove(ih->data->cells[lin-diff_num], ih->data->cells[lin], ih->data->columns.num_alloc*sizeof(ImatCell));

    for (lin = num; lin < old_num; lin++)
      memset(ih->data->cells[lin], 0, ih->data->columns.num_alloc*sizeof(ImatCell));

    memmove(ih->data->lines.sizes+base, ih->data->lines.sizes+end, shift_num*sizeof(int));
    memmove(ih->data->lines.marks+base, ih->data->lines.marks+end, shift_num*sizeof(unsigned char));

    memset(ih->data->lines.sizes+num, 0, diff_num*sizeof(int));
    memset(ih->data->lines.marks+num, 0, diff_num*sizeof(unsigned char));
  }
}

void iupMatrixMemReAllocColumns(Ihandle* ih, int old_num, int num, int base)
{
  int lin, col, end, diff_num, shift_num;

  /* If it doesn't have enough columns allocated, then allocate more space */
  if (num > ih->data->columns.num_alloc)  /* this also implicates that also num>old_num */
  {
    ih->data->columns.num_alloc = num;

    if (!ih->data->callback_mode)
    {
      for(lin = 0; lin < ih->data->lines.num_alloc; lin++)
      {
        ih->data->cells[lin] = (ImatCell*)realloc(ih->data->cells[lin], ih->data->columns.num_alloc*sizeof(ImatCell));
      }
    }

    ih->data->columns.sizes = (int*)realloc(ih->data->columns.sizes, ih->data->columns.num_alloc*sizeof(int));
    ih->data->columns.marks = (unsigned char*)realloc(ih->data->columns.marks, ih->data->columns.num_alloc*sizeof(unsigned char));
  }

  if (old_num==num)
    return;

  if (num>old_num) /* ADD */
  {
    /* shift the old data, opening space for new data, from base to old_num */
    /*   even if (old_num-base)>(num-old_num) memmove will correctly copy the memory */
    /* then clear the openned space starting at base */

    diff_num = num-old_num;     /* size of the openned space */
    shift_num = old_num-base;   /* size of the data to be moved */

    for (lin = 0; lin < ih->data->lines.num_alloc; lin++)
    {
      memmove(ih->data->cells[lin]+old_num, ih->data->cells[lin]+base, shift_num*sizeof(ImatCell));
      memset(ih->data->cells[lin]+base, 0, diff_num*sizeof(ImatCell));
    }

    memmove(ih->data->columns.sizes+old_num, ih->data->columns.sizes+base, shift_num*sizeof(int));
    memmove(ih->data->columns.marks+old_num, ih->data->columns.marks+base, shift_num*sizeof(unsigned char));

    memset(ih->data->columns.sizes+base, 0, diff_num*sizeof(int));
    memset(ih->data->columns.marks+base, 0, diff_num*sizeof(unsigned char));
  }
  else /* DEL */
  {
    /* release memory from the opened space */
    /* move the old data to opened space from end to base */
    /*   even if (num-base)>(old_num-num) memmove will correctly copy the memory */
    /* then clear the remaining space starting at num */

    diff_num = old_num-num;    /* size of the openned space */
    shift_num = num-base;      /* size of the data to be moved */
    end = base+diff_num;

    for (lin = 0; lin < ih->data->lines.num_alloc; lin++)
    {
      for(col = base; col < end; col++)
      {
        ImatCell* cell = &(ih->data->cells[lin][col]);
        if (cell->value)
        {
          free(cell->value);
          cell->value = NULL;
          cell->mark = 0;
        }
      }
    }

    for (lin = 0; lin < ih->data->lines.num_alloc; lin++)
    {
      memmove(ih->data->cells[lin]+base, ih->data->cells[lin]+end, shift_num*sizeof(ImatCell));
      memset(ih->data->cells[lin]+num, 0, diff_num*sizeof(ImatCell));
    }

    memmove(ih->data->columns.sizes+base, ih->data->columns.sizes+end, shift_num*sizeof(int));
    memmove(ih->data->columns.marks+base, ih->data->columns.marks+end, shift_num*sizeof(unsigned char));

    memset(ih->data->columns.sizes+num, 0, diff_num*sizeof(int));
    memset(ih->data->columns.marks+num, 0, diff_num*sizeof(unsigned char));
  }
}
