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
#include "gtkstyle.h"
#include "gtkprivate.h"


#define DEFAULT_FONT     "-Adobe-Helvetica-Medium-R-Normal--*-120-*-*-*-*-*-*"
#define HASH_TABLE_SIZE  127


static void      gtk_styles_init         (void);
static GtkStyle* gtk_style_new_with_vals (GdkColor    *foreground,
					  GdkColor    *background,
					  GdkColor    *highlight,
					  GdkColor    *shadow,
					  GdkFont     *font,
					  gint         shadow_thickness,
					  gint         depth,
					  GdkColormap *colormap);
static void      gtk_style_add           (GtkStyle    *style);
static void      gtk_style_remove        (GtkStyle    *style);
static gint      gtk_style_hash          (GdkColor    *foreground,
					  GdkColor    *background,
					  GdkFont     *font,
					  gint         shadow_thickness);
static GtkStyle* gtk_style_find_by_value (GdkColor    *foreground,
					  GdkColor    *background,
					  GdkColor    *highlight,
					  GdkColor    *shadow,
					  GdkFont     *font,
					  gint         shadow_thickness,
					  gint         depth,
					  GdkColormap *colormap);
static gint      gtk_style_compare       (GtkStyle    *style,
					  GdkColor    *foreground,
					  GdkColor    *background,
					  GdkColor    *highlight,
					  GdkColor    *shadow,
					  GdkFont     *font,
					  gint         shadow_thickness,
					  gint         depth,
					  GdkColormap *colormap);


static GdkFont *default_font = NULL;

static const gint    insensitive_min  = 10000;
static const gdouble insensitive_mult = 1.5;
static const gdouble active_mult      = 0.9;
static const gdouble prelight_mult    = 1.1;
static const gdouble highlight_mult   = 1.3;
static const gdouble shadow_mult      = 0.7;

static GList *val_hash_table[HASH_TABLE_SIZE];
static GList *style_hash_table[HASH_TABLE_SIZE];
static int initialized = 0;


GtkStyle*
gtk_style_new (gint shadow_thickness)
{
  GtkStyle *style;
  GdkColor foreground[5];
  GdkColor background[5];
  GdkFont *font;

  gint r, g, b;
  gint i;

  g_function_enter ("gtk_style_new");

  if (!initialized)
    gtk_styles_init ();

  foreground[GTK_STATE_NORMAL] = gtk_default_foreground;
  foreground[GTK_STATE_ACTIVE] = gtk_default_foreground;
  foreground[GTK_STATE_PRELIGHT] = gtk_default_foreground;
  foreground[GTK_STATE_SELECTED] = gtk_default_selected_foreground;
  background[GTK_STATE_NORMAL] = gtk_default_background;
  background[GTK_STATE_SELECTED] = gtk_default_selected_background;
  background[GTK_STATE_INSENSITIVE] = gtk_default_background;

  shadow_thickness = ((shadow_thickness == -1) ?
		      (gtk_default_shadow_thickness) :
		      (shadow_thickness));

  if (!default_font)
    {
      default_font = gdk_font_load (DEFAULT_FONT);
      if (!default_font)
	g_error ("unable to load default font");
    }
  font = default_font;

  r = (foreground[GTK_STATE_NORMAL].red + insensitive_min) * insensitive_mult;
  g = (foreground[GTK_STATE_NORMAL].green + insensitive_min) * insensitive_mult;
  b = (foreground[GTK_STATE_NORMAL].blue + insensitive_min) * insensitive_mult;
  foreground[GTK_STATE_INSENSITIVE].red = CLAMP (r, 0, 65535);
  foreground[GTK_STATE_INSENSITIVE].green = CLAMP (g, 0, 65535);
  foreground[GTK_STATE_INSENSITIVE].blue = CLAMP (b, 0, 65535);

  r = background[GTK_STATE_NORMAL].red * active_mult;
  g = background[GTK_STATE_NORMAL].green * active_mult;
  b = background[GTK_STATE_NORMAL].blue * active_mult;
  background[GTK_STATE_ACTIVE].red = CLAMP (r, 0, 65535);
  background[GTK_STATE_ACTIVE].green = CLAMP (g, 0, 65535);
  background[GTK_STATE_ACTIVE].blue = CLAMP (b, 0, 65535);

  r = background[GTK_STATE_NORMAL].red * prelight_mult;
  g = background[GTK_STATE_NORMAL].green * prelight_mult;
  b = background[GTK_STATE_NORMAL].blue * prelight_mult;
  background[GTK_STATE_PRELIGHT].red = CLAMP (r, 0, 65535);
  background[GTK_STATE_PRELIGHT].green = CLAMP (g, 0, 65535);
  background[GTK_STATE_PRELIGHT].blue = CLAMP (b, 0, 65535);

  style = gtk_style_new_with_vals (foreground, background, NULL, NULL, font, shadow_thickness, -1, NULL);

  g_function_leave ("gtk_style_new");
  return style;
}

void
gtk_style_destroy (GtkStyle *style)
{
  gint i;

  g_function_enter ("gtk_style_destroy");

  g_assert (style != NULL);

  if (!initialized)
    gtk_styles_init ();

  if (style->ref_count == 0)
    {
      for (i = 0; i < 5; i++)
	{
	  if (style->foreground_gc[i])
	    gtk_gc_release (style->foreground_gc[i]);
	  if (style->background_gc[i])
	    gtk_gc_release (style->background_gc[i]);
	  if (style->highlight_gc[i])
	    gtk_gc_release (style->highlight_gc[i]);
	  if (style->shadow_gc[i])
	    gtk_gc_release (style->shadow_gc[i]);
	}

      gtk_style_remove (style);
      g_free (style);
    }

  g_function_leave ("gtk_style_destroy");
}

void
gtk_style_init (GtkStyle *style)
{
  gint r, g, b;
  gint i;

  g_function_enter ("gtk_style_init");

  g_assert (style != NULL);

  if (!initialized)
    gtk_styles_init ();

  if (style->attach_count == -1)
    {
      style->attach_count = 0;

      for (i = 0; i < 5; i++)
	{
	  r = style->background[i].red * highlight_mult;
	  g = style->background[i].green * highlight_mult;
	  b = style->background[i].blue * highlight_mult;
	  style->highlight[i].red = CLAMP (r, 0, 65535);
	  style->highlight[i].green = CLAMP (g, 0, 65535);
	  style->highlight[i].blue = CLAMP (b, 0, 65535);

	  r = style->background[i].red * shadow_mult;
	  g = style->background[i].green * shadow_mult;
	  b = style->background[i].blue * shadow_mult;
	  style->shadow[i].red = CLAMP (r, 0, 65535);
	  style->shadow[i].green = CLAMP (g, 0, 65535);
	  style->shadow[i].blue = CLAMP (b, 0, 65535);
	}
    }

  g_function_leave ("gtk_style_init");
}

GtkStyle*
gtk_style_attach (GtkStyle  *style,
		  GdkWindow *window)
{
  gint i;

  g_function_enter ("gtk_style_attach");

  g_assert (style != NULL);
  g_assert (window != NULL);

  if (!initialized)
    gtk_styles_init ();

  if ((style->attach_count > 0) &&
      ((window->depth != style->depth) ||
       (window->colormap != style->colormap)))
    {
      style = gtk_style_new_with_vals (style->foreground,
				       style->background,
				       style->highlight,
				       style->shadow,
				       style->font,
				       style->shadow_thickness,
				       window->depth,
				       window->colormap);

      if (style->attach_count < 0)
	style->attach_count = 0;
      style->ref_count += 1;
    }

  gtk_style_init (style);

  style->attach_count += 1;
  style->depth = window->depth;
  style->colormap = window->colormap;

  if (style->attach_count == 1)
    {
      for (i = 0; i < 5; i++)
	{
	  if (!gdk_color_alloc (window->colormap, &style->foreground[i]))
	    g_message ("color allocation failed");
	  if (!gdk_color_alloc (window->colormap, &style->background[i]))
	    g_message ("color allocation failed");
	  if (!gdk_color_alloc (window->colormap, &style->highlight[i]))
	    g_message ("color allocation failed");
	  if (!gdk_color_alloc (window->colormap, &style->shadow[i]))
	    g_message ("color allocation failed");

	  style->foreground_gc[i] = gtk_gc_get (window, &style->foreground[i], NULL, style->font,
						GDK_COPY, GDK_SOLID, NULL, NULL,
						GDK_CLIP_BY_CHILDREN, TRUE);
	  style->background_gc[i] = gtk_gc_get (window, &style->background[i], NULL, style->font,
						GDK_COPY, GDK_SOLID, NULL, NULL,
						GDK_CLIP_BY_CHILDREN, TRUE);
	  style->highlight_gc[i] = gtk_gc_get (window, &style->highlight[i], NULL, style->font,
					       GDK_COPY, GDK_SOLID, NULL, NULL,
					       GDK_CLIP_BY_CHILDREN, TRUE);
	  style->shadow_gc[i] = gtk_gc_get (window, &style->shadow[i], NULL, style->font,
					    GDK_COPY, GDK_SOLID, NULL, NULL,
					    GDK_CLIP_BY_CHILDREN, TRUE);
	}
    }

  g_function_leave ("gtk_style_attach");
  return style;
}

void
gtk_style_detach (GtkStyle *style)
{
  g_function_enter ("gtk_style_dettach");

  g_assert (style != NULL);

  if (!initialized)
    gtk_styles_init ();

  style->attach_count -= 1;

  g_function_leave ("gtk_style_detach");
}

void
gtk_style_ref (GtkStyle *style)
{
  g_function_enter ("gtk_style_ref");

  g_assert (style != NULL);

  if (!initialized)
    gtk_styles_init ();

  style->ref_count += 1;
  if (style->ref_count < 0)
    g_error ("bad style ref_count: %d", style->ref_count);

  g_function_leave ("gtk_style_ref");
}

void
gtk_style_unref (GtkStyle *style)
{
  g_function_enter ("gtk_style_unref");

  g_assert (style != NULL);

  if (!initialized)
    gtk_styles_init ();

  if (style->ref_count == 1)
    gtk_style_destroy (style);
  else
    {
      style->ref_count -= 1;
      if (style->ref_count < 0)
	g_error ("bad style ref_count: %d", style->ref_count);
    }

  g_function_leave ("gtk_style_unref");
}


static void
gtk_styles_init ()
{
  gint i;

  g_function_enter ("gtk_styles_init");

  for (i = 0; i < HASH_TABLE_SIZE; i++)
    {
      val_hash_table[i] = NULL;
      style_hash_table[i] = NULL;
    }
  initialized = 1;

  g_function_leave ("gtk_styles_init");
}

static GtkStyle*
gtk_style_new_with_vals (GdkColor    *foreground,
			 GdkColor    *background,
			 GdkColor    *highlight,
			 GdkColor    *shadow,
			 GdkFont     *font,
			 gint         shadow_thickness,
			 gint         depth,
			 GdkColormap *colormap)
{
  GtkStyle *style;
  gint i;

  g_function_enter ("gtk_style_new_with_vals");

  style = gtk_style_find_by_value (foreground, background, highlight,
				   shadow, font, shadow_thickness,
				   depth, colormap);

  if (!style)
    {
      style = g_new (GtkStyle, 1);
      /*      g_message ("allocating a new style"); */

      style->depth = -1;
      style->colormap = NULL;
      style->ref_count = 0;
      style->attach_count = -1;

      for (i = 0; i < 5; i++)
	{
	  style->foreground_gc[i] = NULL;
	  style->background_gc[i] = NULL;
	  style->highlight_gc[i] = NULL;
	  style->shadow_gc[i] = NULL;
	}

      if (foreground)
	for (i = 0; i < 5; i++)
	  style->foreground[i] = foreground[i];

      if (background)
	for (i = 0; i < 5; i++)
	  style->background[i] = background[i];

      if (highlight)
	for (i = 0; i < 5; i++)
	  style->highlight[i] = highlight[i];

      if (shadow)
	for (i = 0; i < 5; i++)
	  style->shadow[i] = shadow[i];

      style->font = font;
      style->shadow_thickness = shadow_thickness;

      gtk_style_add (style);
    }

  g_function_leave ("gtk_style_new_with_vals");
  return style;
}

static void
gtk_style_add (GtkStyle *style)
{
  gint hash_val;

  g_function_enter ("gtk_style_add");

  g_assert (style != NULL);

  hash_val = gtk_style_hash (style->foreground, style->background,
			     style->font, style->shadow_thickness) % HASH_TABLE_SIZE;
  val_hash_table[hash_val] = g_list_prepend (val_hash_table[hash_val], style);

  hash_val = ((gint) style) % HASH_TABLE_SIZE;
  style_hash_table[hash_val] = g_list_prepend (style_hash_table[hash_val], style);

  g_function_leave ("gtk_style_add");
}

static void
gtk_style_remove (GtkStyle *style)
{
  gint hash_val;

  g_function_enter ("gtk_style_add");

  g_assert (style != NULL);

  hash_val = gtk_style_hash (style->foreground, style->background,
			     style->font, style->shadow_thickness) % HASH_TABLE_SIZE;
  val_hash_table[hash_val] = g_list_remove (val_hash_table[hash_val], style);

  hash_val = ((gint) style) % HASH_TABLE_SIZE;
  style_hash_table[hash_val] = g_list_remove (style_hash_table[hash_val], style);

  g_function_leave ("gtk_style_add");
}

static gint
gtk_style_hash (GdkColor    *foreground,
		GdkColor    *background,
		GdkFont     *font,
		gint         shadow_thickness)
{
  gint hash_value;
  gint i;

  g_function_enter ("gtk_style_hash");

  hash_value = 0;

  if (foreground)
    for (i = 0; i < 5; i++)
      hash_value += foreground[i].red + foreground[i].green + foreground[i].blue;

  if (background)
    for (i = 0; i < 5; i++)
      hash_value += background[i].red + background[i].green + background[i].blue;

  hash_value += (gint) font;
  hash_value += shadow_thickness;

  g_function_leave ("gtk_style_hash");
  return hash_value;
}

static GtkStyle*
gtk_style_find_by_value (GdkColor    *foreground,
			 GdkColor    *background,
			 GdkColor    *highlight,
			 GdkColor    *shadow,
			 GdkFont     *font,
			 gint         shadow_thickness,
			 gint         depth,
			 GdkColormap *colormap)
{
  GList *temp_list;
  GtkStyle *style;
  gint hash_val;

  g_function_enter ("gtk_style_find_by_value");

  hash_val = gtk_style_hash (foreground, background, font, shadow_thickness) % HASH_TABLE_SIZE;
  temp_list = val_hash_table[hash_val];
  style = NULL;

  while (temp_list)
    {
      style = temp_list->data;
      temp_list = temp_list->next;

      if (gtk_style_compare (style, foreground, background, highlight,
			     shadow, font, shadow_thickness, depth, colormap))
	break;

      style = NULL;
    }

  g_function_leave ("gtk_style_find_by_value");
  return style;
}

static gint
gtk_style_compare (GtkStyle    *style,
		   GdkColor    *foreground,
		   GdkColor    *background,
		   GdkColor    *highlight,
		   GdkColor    *shadow,
		   GdkFont     *font,
		   gint         shadow_thickness,
		   gint         depth,
		   GdkColormap *colormap)
{
  gint return_val;
  gint i;

  g_function_enter ("gtk_style_compare");

  return_val = TRUE;

  if (foreground)
    for (i = 0; i < 5; i++)
      if ((style->foreground[i].red != foreground[i].red) ||
	  (style->foreground[i].green != foreground[i].green) ||
          (style->foreground[i].blue != foreground[i].blue))
	return_val = FALSE;

  if (return_val)
    if (background)
      for (i = 0; i < 5; i++)
	if ((style->background[i].red != background[i].red) ||
	    (style->background[i].green != background[i].green) ||
	    (style->background[i].blue != background[i].blue))
	  return_val = FALSE;

  if (return_val)
    if (font)
      if (style->font != font)
	return_val = FALSE;

  if (return_val)
    if (style->shadow_thickness != shadow_thickness)
      return_val = FALSE;

  if (return_val)
    if (depth != -1)
      if (style->depth != depth)
	return_val = FALSE;

  if (return_val)
    if (colormap)
      if (style->colormap != colormap)
	return_val = FALSE;

  g_function_leave ("gtk_style_compare");
  return return_val;
}
