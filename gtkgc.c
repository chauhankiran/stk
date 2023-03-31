/* GTK - The General Toolkit (written for the GIMP)
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
#include "gtkgc.h"
#include "gtkprivate.h"


#define HASH_TABLE_SIZE 127


typedef struct _GtkGC  GtkGC;
struct _GtkGC
{
  GdkGC *gc;
  gint depth;
  gint ref_count;
};


static void gtk_gc_init (void);
static void gtk_gc_add (GtkGC *);
static void gtk_gc_remove (GtkGC *);
static GtkGC* gtk_gc_find_by_gc (GdkGC *);
static GtkGC* gtk_gc_find_by_val (gint              depth,
				  GdkColor         *foreground,
				  GdkColor         *background,
				  GdkFont          *font,
				  GdkFunction       function,
				  GdkFill           fill,
				  GdkPixmap        *tile,
				  GdkPixmap        *stipple,
				  GdkSubwindowMode  subwindow_mode,
				  gint              graphics_exposures);
static gint gtk_gc_hash (gint              depth,
			 GdkColor         *foreground,
			 GdkColor         *background,
			 GdkFont          *font,
			 GdkFunction       function,
			 GdkFill           fill,
			 GdkPixmap        *tile,
			 GdkPixmap        *stipple,
			 GdkSubwindowMode  subwindow_mode,
			 gint              graphics_exposures);


static GList *val_hash_table[HASH_TABLE_SIZE];
static GList *gc_hash_table[HASH_TABLE_SIZE];
static int initialized = 0;


GdkGC*
gtk_gc_get (GdkWindow        *window,
	    GdkColor         *foreground,
	    GdkColor         *background,
	    GdkFont          *font,
	    GdkFunction       function,
	    GdkFill           fill,
	    GdkPixmap        *tile,
	    GdkPixmap        *stipple,
	    GdkSubwindowMode  subwindow_mode,
	    gint              graphics_exposures)
{
  GtkGC *gtk_gc;
  GdkGC *gdk_gc;

  g_function_enter ("gtk_gc_get");

  if (!window)
    g_error ("passed NULL window to gtk_gc_get");

  gtk_gc = gtk_gc_find_by_val (window->depth, foreground, background, font,
			       function, fill, tile, stipple, subwindow_mode,
			       graphics_exposures);

  if (gtk_gc)
    {
      gtk_gc->ref_count += 1;
      gdk_gc = gtk_gc->gc;
    }
  else
    {
      gdk_gc = gdk_gc_new (window);

      gtk_gc = g_new (GtkGC, 1);
      gtk_gc->gc = gdk_gc;
      gtk_gc->depth = window->depth;
      gtk_gc->ref_count = 1;

      if (foreground)
	gdk_gc_set_foreground (gdk_gc, foreground);

      if (background)
	gdk_gc_set_background (gdk_gc, background);

      if (font)
	gdk_gc_set_font (gdk_gc, font);

      if (tile)
	gdk_gc_set_tile (gdk_gc, tile);

      if (stipple)
	gdk_gc_set_stipple (gdk_gc, stipple);

      gdk_gc_set_function (gdk_gc, function);
      gdk_gc_set_fill (gdk_gc, fill);
      gdk_gc_set_subwindow (gdk_gc, subwindow_mode);
      gdk_gc_set_exposures (gdk_gc, graphics_exposures);

      gtk_gc_add (gtk_gc);
    }

  g_function_leave ("gtk_gc_get");
  return gdk_gc;
}

void
gtk_gc_release (gc)
     GdkGC *gc;
{
  GtkGC *gtk_gc;

  g_function_enter ("gtk_gc_release");

  if (!gc)
    g_error ("passed NULL gc to gtk_gc_release");

  if (!initialized)
    gtk_gc_init ();

  gtk_gc = gtk_gc_find_by_gc (gc);

  if (gtk_gc)
    {
      gtk_gc->ref_count -= 1;
      if (gtk_gc->ref_count <= 0)
	{
	  gtk_gc_remove (gtk_gc);
	  gdk_gc_destroy (gtk_gc->gc);
	  g_free (gtk_gc);
	}
    }

  g_function_leave ("gtk_gc_release");
}


static void
gtk_gc_init ()
{
  gint i;

  g_function_enter ("gtk_gc_init");

  for (i = 0; i < HASH_TABLE_SIZE; i++)
    {
      val_hash_table[i] = NULL;
      gc_hash_table[i] = NULL;
    }
  initialized = 1;

  g_function_leave ("gtk_gc_init");
}

static void
gtk_gc_add (gc)
     GtkGC *gc;
{
  gint hash_val;

  g_function_enter ("gtk_gc_add");

  if (!gc)
    g_error ("passed NULL gc to gtk_gc_add");

  hash_val = gtk_gc_hash (gc->depth, &gc->gc->foreground, &gc->gc->background,
			  gc->gc->font, gc->gc->function, gc->gc->fill, gc->gc->tile,
			  gc->gc->stipple, gc->gc->subwindow_mode,
			  gc->gc->graphics_exposures) % HASH_TABLE_SIZE;
  val_hash_table[hash_val] = g_list_prepend (val_hash_table[hash_val], gc);

  hash_val = ((gint) gc->gc) % HASH_TABLE_SIZE;
  gc_hash_table[hash_val] = g_list_prepend (gc_hash_table[hash_val], gc);

  g_function_leave ("gtk_gc_add");
}

static void
gtk_gc_remove (gc)
     GtkGC *gc;
{
  gint hash_val;

  g_function_enter ("gtk_gc_remove");

  if (!gc)
    g_error ("passed NULL gc to gtk_gc_remove");

  hash_val = gtk_gc_hash (gc->depth, &gc->gc->foreground, &gc->gc->background,
			  gc->gc->font, gc->gc->function, gc->gc->fill, gc->gc->tile,
			  gc->gc->stipple, gc->gc->subwindow_mode,
			  gc->gc->graphics_exposures) % HASH_TABLE_SIZE;
  val_hash_table[hash_val] = g_list_remove (val_hash_table[hash_val], gc);

  hash_val = ((gint) gc->gc) % HASH_TABLE_SIZE;
  gc_hash_table[hash_val] = g_list_remove (gc_hash_table[hash_val], gc);

  g_function_leave ("gtk_gc_remove");
}

static GtkGC*
gtk_gc_find_by_gc (gc)
     GdkGC *gc;
{
  GList *temp_list;
  gint hash_val;
  GtkGC *gtk_gc;
  gint got_it;

  g_function_enter ("gtk_gc_find_by_gc");

  if (!gc)
    g_error ("passed NULL gc to gtk_gc_find_by_gc");

  hash_val = ((gint) gc) % HASH_TABLE_SIZE;
  temp_list = gc_hash_table[hash_val];

  got_it = FALSE;
  gtk_gc = NULL;

  while (temp_list)
    {
      gtk_gc = temp_list->data;
      temp_list = temp_list->next;

      if (gtk_gc->gc == gc)
	{
	  got_it = TRUE;
	  break;
	}
    }

  if (!got_it)
    gtk_gc = NULL;

  g_function_leave ("gtk_gc_find_by_gc");
  return gtk_gc;
}

static GtkGC*
gtk_gc_find_by_val (gint              depth,
		    GdkColor         *foreground,
		    GdkColor         *background,
		    GdkFont          *font,
		    GdkFunction       function,
		    GdkFill           fill,
		    GdkPixmap        *tile,
		    GdkPixmap        *stipple,
		    GdkSubwindowMode  subwindow_mode,
		    gint              graphics_exposures)
{
  GList *temp_list;
  GtkGC *gtk_gc;
  GdkGC *gdk_gc;
  guint32 fg_pixel;
  guint32 bg_pixel;
  gint hash_val;
  gint got_it;

  g_function_enter ("gtk_gc_find_by_val");

  hash_val = gtk_gc_hash (depth, foreground, background, font,
			  function, fill, tile, stipple, subwindow_mode,
			  graphics_exposures) % HASH_TABLE_SIZE;
  temp_list = val_hash_table[hash_val];

  fg_pixel = (foreground) ? (foreground->pixel) : (0);
  bg_pixel = (background) ? (background->pixel) : (0);

  got_it = FALSE;
  gtk_gc = NULL;

  while (temp_list)
    {
      gtk_gc = temp_list->data;
      temp_list = temp_list->next;

      gdk_gc = gtk_gc->gc;
      if ((gtk_gc->depth == depth) &&
	  (gdk_gc->foreground.pixel == fg_pixel) &&
	  (gdk_gc->background.pixel == bg_pixel) &&
	  (gdk_gc->font == font) &&
	  (gdk_gc->function == function) &&
	  (gdk_gc->fill == fill) &&
	  (gdk_gc->tile == tile) &&
	  (gdk_gc->stipple == stipple) &&
	  (gdk_gc->subwindow_mode == subwindow_mode) &&
	  (gdk_gc->graphics_exposures == graphics_exposures))
	{
	  got_it = TRUE;
	  break;
	}
    }

  if (!got_it)
    gtk_gc = NULL;

  g_function_leave ("gtk_gc_find_by_val");
  return gtk_gc;
}

static gint
gtk_gc_hash (gint              depth,
	     GdkColor         *foreground,
	     GdkColor         *background,
	     GdkFont          *font,
	     GdkFunction       function,
	     GdkFill           fill,
	     GdkPixmap        *tile,
	     GdkPixmap        *stipple,
	     GdkSubwindowMode  subwindow_mode,
	     gint              graphics_exposures)
{
  gint hash_value;

  g_function_enter ("gtk_gc_hash");

  hash_value = depth;

  if (foreground)
    hash_value += (foreground->red +
		   foreground->green +
		   foreground->blue);

  if (background)
    hash_value += (background->red +
		   background->green +
		   background->blue);

  hash_value += (gint) font;
  hash_value += function;
  hash_value += fill;
  hash_value += (gint) tile;
  hash_value += (gint) stipple;
  hash_value += subwindow_mode;
  hash_value += graphics_exposures;

  g_function_leave ("gtk_gc_hash");
  return hash_value;
}
