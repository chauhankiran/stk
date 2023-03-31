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
 * File:         gdkcolor.c
 * Author:       Peter Mattis
 * Description:  This module contains the routines for creating
 *               and destroying colormaps and for allocating
 *               and freeing colors.
 */
#include <X11/Xlib.h>
#include "gdk.h"
#include "gdkprivate.h"


static gint gdk_colormap_match_color (GdkColormap *cmap,
				      GdkColor    *color,
				      gchar       *available);


GdkColormap*
gdk_colormap_new (GdkVisual *visual,
		  gint       private_cmap)
{
  GdkColormap *colormap;
  GdkColormapPrivate *private;
  Visual *xvisual;
  XColor default_colors[256];
  int size;
  int i;

  g_function_enter ("gdk_colormap_new");

  if (!visual)
    g_error ("passed NULL visual to gdk_colormap_new");

  private = g_new (GdkColormapPrivate, 1);
  colormap = (GdkColormap*) private;

  private->xdisplay = gdk_display;
  private->visual = visual;
  private->next_color = 0;
  xvisual = ((GdkVisualPrivate*) visual)->xvisual;

  switch (visual->type)
    {
    case GDK_VISUAL_GRAYSCALE:
    case GDK_VISUAL_PSEUDO_COLOR:
      private->private = private_cmap;
      private->xcolormap = XCreateColormap (private->xdisplay, gdk_root_window,
					    xvisual, (private_cmap) ? (AllocAll) : (AllocNone));

      for (i = 0; i < 256; i++)
	default_colors[i].pixel = i;

      XQueryColors (private->xdisplay,
		    DefaultColormap (private->xdisplay, gdk_screen),
		    default_colors, visual->colormap_size);

      for (i = 0; i < visual->colormap_size; i++)
	{
	  colormap->colors[i].pixel = default_colors[i].pixel;
	  colormap->colors[i].red = default_colors[i].red;
	  colormap->colors[i].green = default_colors[i].green;
	  colormap->colors[i].blue = default_colors[i].blue;
	}

      gdk_colormap_change (colormap, visual->colormap_size);
      break;

    case GDK_VISUAL_DIRECT_COLOR:
      private->private = TRUE;
      private->xcolormap = XCreateColormap (private->xdisplay, gdk_root_window,
					    xvisual, AllocAll);

      size = 1 << visual->red_prec;
      for (i = 0; i < size; i++)
	colormap->colors[i].red = i * 65535 / (size - 1);

      size = 1 << visual->green_prec;
      for (i = 0; i < size; i++)
	colormap->colors[i].green = i * 65535 / (size - 1);

      size = 1 << visual->blue_prec;
      for (i = 0; i < size; i++)
	colormap->colors[i].blue = i * 65535 / (size - 1);

      gdk_colormap_change (colormap, visual->colormap_size);
      break;

    case GDK_VISUAL_STATIC_GRAY:
    case GDK_VISUAL_STATIC_COLOR:
    case GDK_VISUAL_TRUE_COLOR:
      private->private = FALSE;
      private->xcolormap = XCreateColormap (private->xdisplay, gdk_root_window,
					    xvisual, AllocNone);
      break;
    }

  g_function_leave ("gdk_colormap_new");
  return colormap;
}

void
gdk_colormap_destroy (GdkColormap *colormap)
{
  GdkColormapPrivate *private;

  g_function_enter ("gdk_colormap_destroy");

  if (!colormap)
    g_error ("passed NULL colormap to gdk_colormap_destroy");

  private = (GdkColormapPrivate*) colormap;
  XFreeColormap (private->xdisplay, private->xcolormap);

  g_free (colormap);

  g_function_leave ("gdk_colormap_destroy");
}

GdkColormap*
gdk_colormap_get_system (void)
{
  static GdkColormap *colormap = NULL;
  GdkColormapPrivate *private;
  XColor xpalette[256];
  gint i;

  g_function_enter ("gdk_colormap_get_system");

  if (!colormap)
    {
      private = g_new (GdkColormapPrivate, 1);
      colormap = (GdkColormap*) private;

      private->xdisplay = gdk_display;
      private->xcolormap = DefaultColormap (gdk_display, gdk_screen);
      private->visual = gdk_visual_get_system ();
      private->private = FALSE;
      private->next_color = 0;

      for (i = 0; i < 256; i++)
	{
	  xpalette[i].pixel = i;
	  xpalette[i].red = 0;
	  xpalette[i].green = 0;
	  xpalette[i].blue = 0;
	}

      XQueryColors (gdk_display, private->xcolormap, xpalette, 256);

      for (i = 0; i < 256; i++)
	{
	  colormap->colors[i].pixel = xpalette[i].pixel;
	  colormap->colors[i].red = xpalette[i].red;
	  colormap->colors[i].green = xpalette[i].green;
	  colormap->colors[i].blue = xpalette[i].blue;
	}
    }

  g_function_leave ("gdk_colormap_get_system");
  return colormap;
}

gint
gdk_colormap_get_system_size (void)
{
  return DisplayCells (gdk_display, gdk_screen);
}

void
gdk_colormap_change (GdkColormap *colormap,
		     gint         ncolors)
{
  GdkColormapPrivate *private;
  GdkVisual *visual;
  XColor palette[256];
  gint shift;
  int max_colors;
  int size;
  int i;

  g_function_enter ("gdk_colormap_change");

  if (!colormap)
    g_error ("passed NULL colormap to gdk_colormap_change");

  private = (GdkColormapPrivate*) colormap;
  switch (private->visual->type)
    {
    case GDK_VISUAL_GRAYSCALE:
    case GDK_VISUAL_PSEUDO_COLOR:
      /*      if (private->private)*/
      /*	{*/
	  for (i = 0; i < ncolors; i++)
	    {
	      palette[i].pixel = colormap->colors[i].pixel;
	      palette[i].red = colormap->colors[i].red;
	      palette[i].green = colormap->colors[i].green;
	      palette[i].blue = colormap->colors[i].blue;
	      palette[i].flags = DoRed | DoGreen | DoBlue;
	    }

	  XStoreColors (private->xdisplay, private->xcolormap, palette, ncolors);
	  private->next_color = MAX (private->next_color, ncolors);
	  /*	}*/
      break;

    case GDK_VISUAL_DIRECT_COLOR:
      visual = private->visual;

      shift = visual->red_shift;
      max_colors = 1 << visual->red_prec;
      size = (ncolors < max_colors) ? (ncolors) : (max_colors);

      for (i = 0; i < size; i++)
	{
	  palette[i].pixel = i << shift;
	  palette[i].red = colormap->colors[i].red;
	  palette[i].flags = DoRed;
	}

      XStoreColors (private->xdisplay, private->xcolormap, palette, size);

      shift = visual->green_shift;
      max_colors = 1 << visual->green_prec;
      size = (ncolors < max_colors) ? (ncolors) : (max_colors);

      for (i = 0; i < size; i++)
	{
	  palette[i].pixel = i << shift;
	  palette[i].green = colormap->colors[i].green;
	  palette[i].flags = DoGreen;
	}

      XStoreColors (private->xdisplay, private->xcolormap, palette, size);

      shift = visual->blue_shift;
      max_colors = 1 << visual->blue_prec;
      size = (ncolors < max_colors) ? (ncolors) : (max_colors);

      for (i = 0; i < size; i++)
	{
	  palette[i].pixel = i << shift;
	  palette[i].blue = colormap->colors[i].blue;
	  palette[i].flags = DoBlue;
	}

      XStoreColors (private->xdisplay, private->xcolormap, palette, size);
      break;

    default:
      break;
    }

  g_function_leave ("gdk_colormap_change");
}

void
gdk_colors_store (GdkColormap   *colormap,
		  GdkColor      *colors,
		  gint           ncolors)
{
  gint i;

  for (i = 0; i < ncolors; i++)
    {
      colormap->colors[i].pixel = colors[i].pixel;
      colormap->colors[i].red = colors[i].red;
      colormap->colors[i].green = colors[i].green;
      colormap->colors[i].blue = colors[i].blue;
    }

  gdk_colormap_change (colormap, ncolors);
}

gint
gdk_colors_alloc (GdkColormap   *colormap,
		  gint           contiguous,
		  gulong        *planes,
		  gint           nplanes,
		  gulong        *pixels,
		  gint           npixels)
{
  GdkColormapPrivate *private;
  gint return_val;

  g_function_enter ("gdk_colors_alloc");

  if (!colormap)
    g_error ("passed NULL colormap to gdk_colors_alloc");

  private = (GdkColormapPrivate*) colormap;

  return_val = XAllocColorCells (private->xdisplay, private->xcolormap,
				 contiguous, planes, nplanes, pixels, npixels);

  g_function_leave ("gdk_colors_alloc");
  return return_val;
}

gint
gdk_color_white (GdkColormap *colormap,
		 GdkColor    *color)
{
  gint return_val;

  g_function_enter ("gdk_color_white");

  if (!colormap)
    g_error ("passed NULL colormap to gdk_color_white");

  if (color)
    {
      color->pixel = WhitePixel (gdk_display, gdk_screen);
      color->red = 65535;
      color->green = 65535;
      color->blue = 65535;

      return_val = gdk_color_alloc (colormap, color);
    }
  else
    return_val = FALSE;

  g_function_leave ("gdk_color_white");
  return return_val;
}

gint
gdk_color_black (GdkColormap *colormap,
		 GdkColor    *color)
{
  gint return_val;

  g_function_enter ("gdk_color_black");

  if (!colormap)
    g_error ("passed NULL colormap to gdk_color_black");

  if (color)
    {
      color->pixel = BlackPixel (gdk_display, gdk_screen);
      color->red = 0;
      color->green = 0;
      color->blue = 0;

      return_val = gdk_color_alloc (colormap, color);
    }
  else
    return_val = FALSE;

  g_function_leave ("gdk_color_black");
  return return_val;
}

gint
gdk_color_parse (gchar    *spec,
		 GdkColor *color)
{
  Colormap xcolormap;
  XColor xcolor;
  gint return_val;

  g_function_enter ("gdk_color_parse");

  if (!spec)
    g_error ("passed NULL spec to gdk_color_parse");

  if (!color)
    g_error ("passed NULL color to gdk_color_parse");

  xcolormap = DefaultColormap (gdk_display, gdk_screen);

  if (XParseColor (gdk_display, xcolormap, spec, &xcolor))
    {
      return_val = TRUE;
      color->red = xcolor.red;
      color->green = xcolor.green;
      color->blue = xcolor.blue;
    }
  else
    return_val = FALSE;

  g_function_leave ("gdk_color_parse");
  return return_val;
}

gint
gdk_color_alloc (GdkColormap *colormap,
		 GdkColor    *color)
{
  GdkColormapPrivate *private;
  GdkVisual *visual;
  XColor xcolor;
  gchar available[256];
  gint available_init;
  gint return_val;
  gint i, index;

  g_function_enter ("gdk_color_alloc");

  if (!colormap)
    g_error ("passed NULL colormap to gdk_color_alloc");

  if (!color)
    g_error ("passed NULL color to gdk_color_alloc");

  xcolor.red = color->red;
  xcolor.green = color->green;
  xcolor.blue = color->blue;
  xcolor.pixel = color->pixel;
  xcolor.flags = DoRed | DoGreen | DoBlue;

  return_val = FALSE;
  private = (GdkColormapPrivate*) colormap;

  switch (private->visual->type)
    {
    case GDK_VISUAL_GRAYSCALE:
    case GDK_VISUAL_PSEUDO_COLOR:
      if (private->private)
	{
	  if (private->next_color > 255)
	    {
	      for (i = 0; i < 256; i++)
		available[i] = TRUE;

	      index = gdk_colormap_match_color (colormap, color, available);
	      if (index != -1)
		{
		  available[index] = FALSE;
		  *color = colormap->colors[index];
		  return_val = TRUE;
		}
	      else
		{
		  return_val = FALSE;
		}
	    }
	  else
	    {
	      xcolor.pixel = 255 - private->next_color;
	      color->pixel = xcolor.pixel;
	      private->next_color += 1;

	      XStoreColor (private->xdisplay, private->xcolormap, &xcolor);
	      return_val = TRUE;
	    }
	}
      else
	{
	  available_init = 1;

	  while (1)
	    {
	      if (XAllocColor (private->xdisplay, private->xcolormap, &xcolor))
		{
		  color->pixel = xcolor.pixel;
		  color->red = xcolor.red;
		  color->green = xcolor.green;
		  color->blue = xcolor.blue;

		  colormap->colors[color->pixel] = *color;

		  return_val = TRUE;
		  break;
		}
	      else
		{
		  if (available_init)
		    {
		      available_init = 0;
		      for (i = 0; i < 256; i++)
			available[i] = TRUE;
		    }

		  index = gdk_colormap_match_color (colormap, color, available);
		  if (index != -1)
		    {
		      available[index] = FALSE;
		      xcolor.red = colormap->colors[index].red;
		      xcolor.green = colormap->colors[index].green;
		      xcolor.blue = colormap->colors[index].blue;
		    }
		  else
		    {
		      return_val = FALSE;
		      break;
		    }
		}
	    }
	}
      break;

    case GDK_VISUAL_DIRECT_COLOR:
      visual = private->visual;
      xcolor.pixel = (((xcolor.red >> (16 - visual->red_prec)) << visual->red_shift) +
		      ((xcolor.green >> (16 - visual->green_prec)) << visual->green_shift) +
		      ((xcolor.blue >> (16 - visual->blue_prec)) << visual->blue_shift));
      color->pixel = xcolor.pixel;
      return_val = TRUE;
      break;

    case GDK_VISUAL_STATIC_GRAY:
    case GDK_VISUAL_STATIC_COLOR:
    case GDK_VISUAL_TRUE_COLOR:
      if (XAllocColor (private->xdisplay, private->xcolormap, &xcolor))
	{
	  color->pixel = xcolor.pixel;
	  return_val = TRUE;
	}
      else
	return_val = FALSE;
      break;
    }

  g_function_leave ("gdk_color_alloc");
  return return_val;
}

gint
gdk_color_change (GdkColormap *colormap,
		  GdkColor    *color)
{
  GdkColormapPrivate *private;
  XColor xcolor;

  g_function_enter ("gdk_color_change");

  if (!colormap)
    g_error ("passed NULL colormap to gdk_color_alloc");

  if (!color)
    g_error ("passed NULL color to gdk_color_alloc");

  xcolor.pixel = color->pixel;
  xcolor.red = color->red;
  xcolor.green = color->green;
  xcolor.blue = color->blue;
  xcolor.flags = DoRed | DoGreen | DoBlue;

  private = (GdkColormapPrivate*) colormap;
  XStoreColor (private->xdisplay, private->xcolormap, &xcolor);

  g_function_leave ("gdk_color_change");
  return TRUE;
}


static gint
gdk_colormap_match_color (GdkColormap *cmap,
			  GdkColor    *color,
			  gchar       *available)
{
  GdkColor *colors;
  guint sum, max;
  gint rdiff, gdiff, bdiff;
  gint i, index;

  g_function_enter ("gdk_colormap_match_color");

  g_assert (cmap != NULL);
  g_assert (color != NULL);

  colors = cmap->colors;
  max = 3 * (65536);
  index = -1;

  for (i = 0; i < 256; i++)
    {
      if ((!available) || (available && available[i]))
	{
	  rdiff = (color->red - colors[i].red);
	  gdiff = (color->green - colors[i].green);
	  bdiff = (color->blue - colors[i].blue);

	  sum = ABS (rdiff) + ABS (gdiff) + ABS (bdiff);

	  if (sum < max)
	    {
	      index = i;
	      max = sum;
	    }
	}
    }

  g_function_leave ("gdk_colormap_match_color");
  return index;
}
