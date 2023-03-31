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
 * File:         gdkfont.c
 * Author:       Peter Mattis
 * Description:  This module contains the routines for loading
 *               and freeing fonts and querying for the width
 *               of a string in a given font.
 */
#include <X11/Xlib.h>
#include <X11/Xos.h>
#include "gdk.h"
#include "gdkprivate.h"


GdkFont*
gdk_font_load (gchar *font_name)
{
  GdkFont *font;
  GdkFontPrivate *private;

  g_function_enter ("gdk_font_load");

  private = g_new (GdkFontPrivate, 1);
  font = (GdkFont*) private;

  private->xdisplay = gdk_display;
  private->xfont = XLoadQueryFont (private->xdisplay, font_name);
  
  if (!private->xfont)
    {
      g_free (font);
      font = NULL;
    }
  else
    {
      font->ascent = private->xfont->ascent;
      font->descent = private->xfont->descent;
    }
  
  g_function_leave ("gdk_font_load");
  return font;
}

void
gdk_font_free (GdkFont *font)
{
  GdkFontPrivate *private;
  
  g_function_enter ("gdk_font_free");

  if (!font)
    g_error ("passed NULL font to gdk_font_free");

  private = (GdkFontPrivate*) font;
  XFreeFont (private->xdisplay, private->xfont);
  g_free (font);
  
  g_function_leave ("gdk_font_free");
}

gint
gdk_string_width (GdkFont *font, 
		  gchar   *string)
{
  GdkFontPrivate *private;
  gint width;
  
  g_function_enter ("gdk_string_width");

  if (!font)
    g_error ("passed NULL font to gdk_string_width");

  if (!string)
    g_error ("passed NULL string to gdk_string_width");

  private = (GdkFontPrivate*) font;
  width = XTextWidth (private->xfont, string, strlen (string));
    
  g_function_leave ("gdk_string_width");
  return width;
}

gint 
gdk_text_width (GdkFont  *font,
		gchar    *text,
		gint      text_length)
{
  GdkFontPrivate *private;
  gint width;
  
  g_function_enter ("gdk_text_width");

  if (!font)
    g_error ("passed NULL font to gdk_text_width");

  if (!text)
    g_error ("passed NULL text to gdk_text_width");

  private = (GdkFontPrivate*) font;
  width = XTextWidth (private->xfont, text, text_length);
    
  g_function_leave ("gdk_text_width");
  return width;
}

gint
gdk_char_width (GdkFont *font,
		gchar    character)
{
  GdkFontPrivate *private;
  XCharStruct *chars;
  gint width;
  
  g_function_enter ("gdk_char_width");

  if (!font)
    g_error ("passed NULL font to gdk_char_width");

  private = (GdkFontPrivate*) font;
  
  if ((private->xfont->min_byte1 == 0) &&
      (private->xfont->max_byte1 == 0) &&
      (character >= private->xfont->min_char_or_byte2) &&
      (character <= private->xfont->max_char_or_byte2))
    {
      chars = private->xfont->per_char;
      if (chars)
	width = chars[character - private->xfont->min_char_or_byte2].width;
      else
	width = private->xfont->min_bounds.width;
    }
  else
    {
      width = XTextWidth (private->xfont, &character, 1);
    }
  
  g_function_leave ("gdk_char_width");
  return width;
}
