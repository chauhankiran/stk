/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
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
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include "gtk.h"
#include "gdk.h"

#define AREA_WIDTH  250
#define AREA_HEIGHT 64
#define BLACK       10
#define WHITE       11
#define RIGHT_ERROR 0.4375
#define BELOW_LEFT_ERROR 0.1875
#define BELOW_ERROR 0.3125
#define BELOW_RIGHT_ERROR 0.0625
#define INTENSITY(i) ((i) * 12 + 32)

#define EVENT_MASK  GDK_EXPOSURE_MASK

static void app_init (void);
static int  gamma_correct (int, double);
static void set_colors (double);
static void create_dialog (void);
static void intensity_draw (GtkWidget *area);
static void dither_draw (GtkWidget *area);
static gint intensity_events (GtkWidget *area, GdkEvent *event);
static gint dither_events (GtkWidget *area, GdkEvent *event);
static gint gamma_update (GtkObserver *observer, GtkData *data);
static void gamma_disconnect (GtkObserver *observer, GtkData *data);
static void quit_callback (GtkWidget *w, gpointer client_data, gpointer call_data);

static GtkDataAdjustment *gamma_adj;
static GtkObserver gamma_observer;
static GdkGC *gc = NULL;
static GdkImage *image = NULL;
static GdkVisual *visual;
static GdkColormap *cmap;
static GdkColor shades[12];
static double gamma_value = 1.0;

void
main (int argc, char **argv)
{
  /* Initialize G toolkit */
  gdk_set_debug_level (0);
  gdk_set_show_events (0);
  gtk_init (&argc, &argv);

  /* Initialize the colors */
  app_init ();

  /* Create the display */
  create_dialog ();

  /* Main application loop */
  gtk_main ();
}


/****  Local Functions  ****/

static void
app_init ()
{
  guint32 pixels[12];
  int success;
  int i;

  visual = gdk_visual_get_best ();
  cmap = gdk_colormap_new (visual, TRUE);

  gtk_push_visual (visual);
  gtk_push_colormap (cmap);

  if (visual->type == GDK_VISUAL_PSEUDO_COLOR)
    {
      success = gdk_colors_alloc (cmap, 0, NULL, 0, pixels, 12);

      if (success)
	{
	  for (i = 0; i < 10; i++)
	    {
	      shades[i].red = INTENSITY (i) << 8;
	      shades[i].green = shades[i].blue = shades[i].red;
	      shades[i].pixel = pixels[i];
	    }
	  shades[10].red = 0;
	  shades[10].green = shades[10].blue = shades[10].red;
	  shades[10].pixel = pixels[10];
	  shades[11].red = (255 << 8);
	  shades[11].green = shades[11].blue = shades[11].red;
	  shades[11].pixel = pixels[11];
	  gdk_colors_store (cmap, shades, 12);
	}
    }
  else if ((visual->type == GDK_VISUAL_TRUE_COLOR) ||
	   (visual->type == GDK_VISUAL_DIRECT_COLOR))
    {
      for (i = 0; i < 10; i++)
	{
	  shades[i].red = INTENSITY (i) << 8;
	  shades[i].green = shades[i].blue = shades[i].red;
	  gdk_color_alloc (cmap, &shades[i]);
	}
      shades[10].red = 0;
      shades[10].green = shades[10].blue = shades[10].red;
      gdk_color_alloc (cmap, &shades[10]);
      shades[11].red = (255 << 8);
      shades[11].green = shades[11].blue = shades[11].red;
      gdk_color_alloc (cmap, &shades[11]);

      success = 1;
    }
  else
    g_error ("Unsupported visual.\n");

  if (!success)
    g_error ("Unable to allocate sufficient colors from system colormap.\n");
}

static int
gamma_correct (int intensity, double gamma)
{
  int val;
  double ind;
  double one_over_gamma;

  if (gamma != 0.0)
    one_over_gamma = 1.0 / gamma;

  ind = (double) intensity / 256.0;
  val = (int) (256 * pow (ind, one_over_gamma));

  return val;
}

static void
set_colors (double gamma)
{
  int gray;
  int i;

  if (visual->type == GDK_VISUAL_PSEUDO_COLOR)
    for (i = 0; i < 10; i++)
      {
	gray = INTENSITY (i);
	gray = gamma_correct (gray, gamma);
	shades[i].red = gray << 8;
	shades[i].green = shades[i].blue = shades[i].red;
	gdk_color_change (cmap, &shades[i]);
      }
  else if ((visual->type == GDK_VISUAL_TRUE_COLOR) ||
	   (visual->type == GDK_VISUAL_DIRECT_COLOR))
    for (i = 0; i < 10; i++)
      {
	gray = INTENSITY (i);
	gray = gamma_correct (gray, gamma);
	shades[i].red = gray << 8;
	shades[i].green = shades[i].blue = shades[i].red;
	gdk_color_alloc (cmap, &shades[i]);
      }
}

static void
create_dialog ()
{
  GtkWidget *shell;
  GtkWidget *frame;
  GtkWidget *vbox;
  GtkWidget *hbox;
  GtkWidget *intensity_frame;
  GtkWidget *dither_frame;
  GtkWidget *button;
  GtkWidget *label;
  GtkWidget *scale;
  GtkWidget *intensity_area;
  GtkWidget *dither_area;

  shell = gtk_window_new ("Gamma Correction", GTK_WINDOW_TOPLEVEL);

  frame = gtk_frame_new (NULL);
  gtk_frame_set_type (frame, GTK_SHADOW_ETCHED_IN);
  gtk_container_add (shell, frame);
  vbox = gtk_vbox_new (FALSE, 0);
  gtk_container_add (frame, vbox);

  label = gtk_label_new ("Select the gamma correction that provides");
  gtk_box_pack (vbox, label, TRUE, FALSE, 0, GTK_PACK_START);
  gtk_widget_show (label);
  label = gtk_label_new ("the best matching between the images.");
  gtk_box_pack (vbox, label, TRUE, FALSE, 5, GTK_PACK_START);
  gtk_widget_show (label);

  hbox = gtk_hbox_new (FALSE, 0);
  gtk_container_set_border_width (hbox, 0);
  gtk_box_pack (vbox, hbox, TRUE, FALSE, 0, GTK_PACK_START);

  label = gtk_label_new ("Gamma: ");
  gtk_box_pack (hbox, label, FALSE, FALSE, 0, GTK_PACK_START);
  gtk_label_set_alignment (label, 0.0, 1.0);
  gtk_widget_show (label);
  gamma_adj = (GtkDataAdjustment *) gtk_data_adjustment_new (1.0, 0.1, 3.0, 0.1, 0.1, 0.1);
  scale = gtk_hscale_new (gamma_adj);
  gtk_scale_set_value_pos (scale, GTK_POS_TOP);
  gtk_box_pack (hbox, scale, TRUE, TRUE, 5, GTK_PACK_START);
  gtk_widget_show (scale);
  gtk_widget_show (hbox);

  intensity_frame = gtk_frame_new ("Intensity");
  gtk_frame_set_type (intensity_frame, GTK_SHADOW_IN);
  gtk_box_pack (vbox, intensity_frame, FALSE, FALSE, 5, GTK_PACK_START);
  intensity_area = gtk_drawing_area_new (AREA_WIDTH, AREA_HEIGHT,
					 intensity_events, EVENT_MASK);
  gtk_container_add (intensity_frame, intensity_area);
  gtk_widget_show (intensity_area);
  gtk_widget_show (intensity_frame);

  dither_frame = gtk_frame_new ("Dithering");
  gtk_frame_set_type (dither_frame, GTK_SHADOW_IN);
  gtk_box_pack (vbox, dither_frame, FALSE, FALSE, 5, GTK_PACK_START);
  dither_area = gtk_drawing_area_new (AREA_WIDTH, AREA_HEIGHT,
				      dither_events, EVENT_MASK);
  gtk_container_add (dither_frame, dither_area);
  gtk_widget_show (dither_area);
  gtk_widget_show (dither_frame);

  gamma_observer.update = gamma_update;
  gamma_observer.disconnect = gamma_disconnect;
  gamma_observer.user_data = intensity_area;
  gtk_data_attach ((GtkData *) gamma_adj, &gamma_observer);

  button = gtk_push_button_new ();
  gtk_box_pack (vbox, button, TRUE, FALSE, 0, GTK_PACK_START);
  label = gtk_label_new ("Quit");
  gtk_container_add (button, label);
  gtk_widget_show (label);
  gtk_widget_show (button);

  gtk_callback_add (gtk_button_get_state (button), quit_callback, NULL);

  gtk_widget_show (vbox);
  gtk_widget_show (frame);
  gtk_widget_show (shell);
}

static void
intensity_draw (GtkWidget *area)
{
  int i;
  int frac_width;

  /*  reset the colors based on the current gamma value  */
  set_colors (gamma_value);

  frac_width = AREA_WIDTH / 10;
  for (i = 0; i < 10; i++)
    {
      gdk_gc_set_foreground (gc, &shades[i]);
      gdk_draw_rectangle (area->window, gc, 1,
			  frac_width * i, 0,
			  frac_width, AREA_HEIGHT);
    }
}

static void
dither_render_row (int      y,
		   guint32 *row)
{
  guchar  *data_8bit;
  gushort *data_16bit;
  guint32 *data_24bit;
  int i;

  switch (visual->depth)
    {
    case 8:
      data_8bit = (guchar*) image->mem + image->bpl * y;
      for (i = 0; i < AREA_WIDTH; i++)
	*data_8bit++ = row[i];
      break;

    case 15: case 16:
      data_16bit = (gushort*) ((guchar*) image->mem + image->bpl * y);
      for (i = 0; i < AREA_WIDTH; i++)
	*data_16bit++ = row[i];
      break;

    case 24: case 32:
      data_24bit = (guint32*) ((guchar*) image->mem + image->bpl * y);
      for (i = 0; i < AREA_WIDTH; i++)
	*data_24bit++ = row[i];
      break;

    default:
      break;
    }
}

static void
dither_draw (GtkWidget *area)
{
  /* Implement a floyd-steinberg based dithering of the pattern
   * displayed in the intensity dialog
   */
  int x, y;
  int frac_width;
  double error;
  double right_err;
  double *cur;
  double *down_err;
  guint32 *pixels;

  frac_width = AREA_WIDTH / 10;
  down_err = malloc ((AREA_WIDTH + 2) * sizeof (double));
  cur = malloc (AREA_WIDTH * sizeof (double));
  pixels = malloc (AREA_WIDTH * sizeof (guint32));
  down_err += 1;

  /*  Prep the error trackers  */
  for (x = -1; x < AREA_WIDTH; x++)
    down_err[x] = 0.0;
  right_err = 0.0;

  /*  Dither the intensity tile  */
  for (y = 0; y < AREA_HEIGHT; y++)
    {
      /*  Add the error from the previous row  */
      for (x = 0; x < AREA_WIDTH; x++)
	cur[x] = INTENSITY (x / frac_width) + down_err[x];
      right_err = down_err[-1];

      /*  zero the error  */
      for (x = -1; x < AREA_WIDTH; x++)
	down_err[x] = 0.0;

      for (x = 0; x < AREA_WIDTH; x++)
	{
	  cur[x] += right_err;
	  if (cur[x] > 127)
	    {
	      error = cur[x] - 255;
	      pixels[x] = shades[WHITE].pixel;
	    }
	  else
	    {
	      error = cur[x];
	      pixels[x] = shades[BLACK].pixel;
	    }

	  right_err = RIGHT_ERROR * error;
	  down_err[x-1] += BELOW_LEFT_ERROR * error;
	  down_err[x] += BELOW_ERROR * error;
	  down_err[x+1] += BELOW_RIGHT_ERROR * error;
	}

      dither_render_row (y, pixels);
    }

  /*  Draw the image  */
  gdk_draw_image (area->window, gc, image, 0, 0, 0, 0, AREA_WIDTH, AREA_HEIGHT);

  free (cur);
  free (down_err - 1);
  free (pixels);
}

static gint
intensity_events (GtkWidget *area,
		  GdkEvent  *event)
{
  switch (event->type)
    {
    case GDK_EXPOSE:
      /*  Is this the first exposure?  */
      if (!gc)
	gc = gdk_gc_new (area->window);

      intensity_draw (area);
      break;
    default:
      break;
    }

  return FALSE;
}

static gint
dither_events (GtkWidget *area,
	       GdkEvent  *event)
{
  switch (event->type)
    {
    case GDK_EXPOSE:
      /*  Is this the first exposure?  */
      if (!gc)
	gc = gdk_gc_new (area->window);
      if (!image)
	image = gdk_image_new (GDK_IMAGE_NORMAL, visual, AREA_WIDTH, AREA_HEIGHT);

      dither_draw (area);
      break;
    default:
      break;
    }

  return FALSE;
}

static gint
gamma_update (GtkObserver *observer,
	      GtkData     *data)
{
  GtkDataAdjustment *adj_data;

  adj_data = (GtkDataAdjustment *) data;

  gamma_value = adj_data->value;
  intensity_draw (observer->user_data);

  return FALSE;
}

static void
gamma_disconnect (GtkObserver *observer,
		  GtkData     *data)
{
}

static void
quit_callback (GtkWidget *w,
	       gpointer   client_data,
	       gpointer   call_data)
{
  gtk_exit (0);
}
