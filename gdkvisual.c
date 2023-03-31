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
 * File:         gdkvisual.c
 * Author:       Peter Mattis
 * Description:  This module contains the routines for querying
 *               information about the visuals available.
 */
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include "gdk.h"
#include "gdkprivate.h"


static void gdk_visual_decompose_mask (gulong  mask,
				       gint   *shift,
				       gint   *prec);

static GdkVisualPrivate system_visual;
static GdkVisualPrivate *visuals;
static gint nvisuals;

static gint available_depths[4];
static gint navailable_depths;

static GdkVisualType available_types[6];
static gint navailable_types;

static char* visual_names[] =
{
  "static gray",
  "grayscale",
  "static color",
  "pseudo color",
  "true color",
  "direct color",
};

void
gdk_visual_init ()
{
  static gint possible_depths[4] = { 24, 16, 15, 8 };
  static GdkVisualType possible_types[6] =
    {
      GDK_VISUAL_DIRECT_COLOR,
      GDK_VISUAL_TRUE_COLOR,
      GDK_VISUAL_PSEUDO_COLOR,
      GDK_VISUAL_STATIC_COLOR,
      GDK_VISUAL_GRAYSCALE,
      GDK_VISUAL_STATIC_GRAY
    };

  static gint npossible_depths = 4;
  static gint npossible_types = 6;

  XVisualInfo *visual_list;
  XVisualInfo visual_template;
  GdkVisualPrivate temp_visual;
  Visual *default_xvisual;
  int nxvisuals;
  int i, j;

  g_function_enter ("gdk_visual_init");

  visual_template.screen = gdk_screen;
  visual_list = XGetVisualInfo (gdk_display, VisualScreenMask, &visual_template, &nxvisuals);
  visuals = g_new (GdkVisualPrivate, nxvisuals);

  default_xvisual = DefaultVisual (gdk_display, gdk_screen);

  nvisuals = 0;
  for (i = 0; i < nxvisuals; i++)
    {
      if (visual_list[i].depth >= 8)
	{
#ifdef __cplusplus
	  switch (visual_list[i].c_class)
#else /* __cplusplus */
	  switch (visual_list[i].class)
#endif /* __cplusplus */
	    {
	    case StaticGray:
	      visuals[nvisuals].visual.type = GDK_VISUAL_STATIC_GRAY;
	      break;
	    case GrayScale:
	      visuals[nvisuals].visual.type = GDK_VISUAL_GRAYSCALE;
	      break;
	    case StaticColor:
	      visuals[nvisuals].visual.type = GDK_VISUAL_STATIC_COLOR;
	      break;
	    case PseudoColor:
	      visuals[nvisuals].visual.type = GDK_VISUAL_PSEUDO_COLOR;
	      break;
	    case TrueColor:
	      visuals[nvisuals].visual.type = GDK_VISUAL_TRUE_COLOR;
	      break;
	    case DirectColor:
	      visuals[nvisuals].visual.type = GDK_VISUAL_DIRECT_COLOR;
	      break;
	    }

	  visuals[nvisuals].visual.depth = visual_list[i].depth;
	  visuals[nvisuals].visual.red_mask = visual_list[i].red_mask;
	  visuals[nvisuals].visual.green_mask = visual_list[i].green_mask;
	  visuals[nvisuals].visual.blue_mask = visual_list[i].blue_mask;
	  visuals[nvisuals].visual.colormap_size = visual_list[i].colormap_size;
	  visuals[nvisuals].visual.bits_per_rgb = visual_list[i].bits_per_rgb;
	  visuals[nvisuals].xvisual = visual_list[i].visual;

	  if ((visuals[nvisuals].visual.type == GDK_VISUAL_TRUE_COLOR) ||
	      (visuals[nvisuals].visual.type == GDK_VISUAL_DIRECT_COLOR))
	    {
	      gdk_visual_decompose_mask (visuals[nvisuals].visual.red_mask,
					 &visuals[nvisuals].visual.red_shift,
					 &visuals[nvisuals].visual.red_prec);

	      gdk_visual_decompose_mask (visuals[nvisuals].visual.green_mask,
					 &visuals[nvisuals].visual.green_shift,
					 &visuals[nvisuals].visual.green_prec);

	      gdk_visual_decompose_mask (visuals[nvisuals].visual.blue_mask,
					 &visuals[nvisuals].visual.blue_shift,
					 &visuals[nvisuals].visual.blue_prec);
	    }
	  else
	    {
	      visuals[nvisuals].visual.red_mask = 0;
	      visuals[nvisuals].visual.red_shift = 0;
	      visuals[nvisuals].visual.red_prec = 0;

	      visuals[nvisuals].visual.green_mask = 0;
	      visuals[nvisuals].visual.green_shift = 0;
	      visuals[nvisuals].visual.green_prec = 0;

	      visuals[nvisuals].visual.blue_mask = 0;
	      visuals[nvisuals].visual.blue_shift = 0;
	      visuals[nvisuals].visual.blue_prec = 0;
	    }

	  if (default_xvisual->visualid == visual_list[i].visualid)
	    system_visual = visuals[nvisuals];

	  nvisuals += 1;
	}
    }

  for (i = 0; i < nvisuals; i++)
    {
      for (j = i+1; j < nvisuals; j++)
	{
	  if (visuals[j].visual.depth >= visuals[i].visual.depth)
	    {
	      if ((visuals[j].visual.depth == 8) && (visuals[i].visual.depth == 8))
		{
		  if (visuals[j].visual.type == GDK_VISUAL_PSEUDO_COLOR)
		    {
		      temp_visual = visuals[j];
		      visuals[j] = visuals[i];
		      visuals[i] = temp_visual;
		    }
		  else if ((visuals[i].visual.type != GDK_VISUAL_PSEUDO_COLOR) &&
			   visuals[j].visual.type > visuals[i].visual.type)
		    {
		      temp_visual = visuals[j];
		      visuals[j] = visuals[i];
		      visuals[i] = temp_visual;
		    }
		}
	      else if ((visuals[j].visual.depth > visuals[i].visual.depth) ||
		       ((visuals[j].visual.depth == visuals[i].visual.depth) &&
			(visuals[j].visual.type > visuals[i].visual.type)))
		{
		  temp_visual = visuals[j];
		  visuals[j] = visuals[i];
		  visuals[i] = temp_visual;
		}
	    }
	}
    }

  if (gdk_debug_level >= 1)
    for (i = 0; i < nvisuals; i++)
      g_message ("visual: %s: %d",
		   visual_names[visuals[i].visual.type],
		   visuals[i].visual.depth);

  navailable_depths = 0;
  for (i = 0; i < npossible_depths; i++)
    {
      for (j = 0; j < nvisuals; j++)
	{
	  if (visuals[j].visual.depth == possible_depths[i])
	    {
	      available_depths[navailable_depths++] = visuals[j].visual.depth;
	      break;
	    }
	}
    }

  if (navailable_depths == 0)
    g_error ("unable to find a usable depth");

  navailable_types = 0;
  for (i = 0; i < npossible_types; i++)
    {
      for (j = 0; j < nvisuals; j++)
	{
	  if (visuals[j].visual.type == possible_types[i])
	    {
	      available_types[navailable_types++] = visuals[j].visual.type;
	      break;
	    }
	}
    }

  if (npossible_types == 0)
    g_error ("unable to find a usable visual type");

  g_function_leave ("gdk_visual_init");
}

gint
gdk_visual_get_best_depth ()
{
  return available_depths[0];
}

GdkVisualType
gdk_visual_get_best_type ()
{
  return available_types[0];
}

GdkVisual*
gdk_visual_get_system ()
{
  return ((GdkVisual*) &system_visual);
}

GdkVisual*
gdk_visual_get_best ()
{
  return ((GdkVisual*) &(visuals[0]));
}

GdkVisual*
gdk_visual_get_best_with_depth (gint depth)
{
  GdkVisual *return_val;
  int i;

  g_function_enter ("gdk_visual_get_best_depth");

  return_val = NULL;
  for (i = 0; i < nvisuals; i++)
    if (depth == visuals[i].visual.depth)
      {
	return_val = (GdkVisual*) &(visuals[i]);
	break;
      }

  g_function_leave ("gdk_visual_get_best_depth");
  return return_val;
}

GdkVisual*
gdk_visual_get_best_with_type (GdkVisualType visual_type)
{
  GdkVisual *return_val;
  int i;

  g_function_enter ("gdk_visual_get_best_with_type");

  return_val = NULL;
  for (i = 0; i < nvisuals; i++)
    if (visual_type == visuals[i].visual.type)
      {
	return_val = (GdkVisual*) &(visuals[i]);
	break;
      }

  g_function_leave ("gdk_visual_get_best_with_type");
  return return_val;
}

GdkVisual*
gdk_visual_get_best_with_both (gint          depth,
			       GdkVisualType visual_type)
{
  GdkVisual *return_val;
  int i;

  g_function_enter ("gdk_visual_get_best_with_both");

  return_val = NULL;
  for (i = 0; i < nvisuals; i++)
    if ((depth == visuals[i].visual.depth) &&
	(visual_type == visuals[i].visual.type))
      {
	return_val = (GdkVisual*) &(visuals[i]);
	break;
      }

  g_function_leave ("gdk_visual_get_best_with_both");
  return return_val;
}

void
gdk_query_depths  (gint **depths,
		   gint  *count)
{
  *count = navailable_depths;
  *depths = available_depths;
}

void
gdk_query_visual_types (GdkVisualType **visual_types,
			gint           *count)
{
  *count = navailable_types;
  *visual_types = available_types;
}

void
gdk_query_visuals (GdkVisual **visual_return,
		   gint       *count)
{
  *count = nvisuals;
  *visual_return = (GdkVisual*) visuals;
}


static void
gdk_visual_decompose_mask (gulong  mask,
			   gint   *shift,
			   gint   *prec)
{
  g_function_enter ("gdk_visual_decompose_mask");

  *shift = 0;
  *prec = 0;

  while (!(mask & 0x1))
    {
      (*shift)++;
      mask >>= 1;
    }

  while (mask & 0x1)
    {
      (*prec)++;
      mask >>= 1;
    }

  g_function_leave ("gdk_visual_decompose_mask");
}
