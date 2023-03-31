/* GDK - The General Drawing Kit (written for the GIMP)
 * Copyright (C) 1995 Peter Mattis
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

/*
 * File:         gdkrectangle.c
 * Author:       Peter Mattis
 * Description:  This module contains the routine for determining
 *               the intersection of 2 rectangles.
 */
#include "gdk.h"


gint 
gdk_rectangle_intersect (GdkRectangle *src1, 
			 GdkRectangle *src2, 
			 GdkRectangle *dest)
{
  GdkRectangle *temp;
  gint src1_x2, src1_y2;
  gint src2_x2, src2_y2;
  gint return_val;
  
  g_function_enter ("gdk_rectangle_intersect");

  if (!src1)
    g_error ("passed NULL src1 to gdk_rectangle_intersect");
  
  if (!src2)
    g_error ("passed NULL src2 to gdk_rectangle_intersect");
  
  if (!dest)
    g_error ("passed NULL dest to gdk_rectangle_intersect");

  return_val = FALSE;

  src1_x2 = src1->x + src1->width;
  src1_y2 = src1->y + src1->height;
  src2_x2 = src2->x + src2->width;
  src2_y2 = src2->y + src2->height;

  if (src2->x < src1->x)
    {
      temp = src1;
      src1 = src2;
      src2 = temp;
    }
  dest->x = src2->x;

  if (src2->x < src1_x2)
    {
      if (src1_x2 < src2_x2)
	dest->width = src1_x2 - dest->x;
      else
	dest->width = src2_x2 - dest->x;

      if (src2->y < src1->y)
	{
	  temp = src1;
	  src1 = src2;
	  src2 = temp;
	}
      dest->y = src2->y;
  
      if (src2->y < src1_y2)
	{
	  return_val = TRUE;
	  
	  if (src1_y2 < src2_y2)
	    dest->height = src1_y2 - dest->y;
	  else
	    dest->height = src2_y2 - dest->y;
	}
    }

  g_function_leave ("gdk_rectangle_intersect");
  return return_val;
}
