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
#include "gtkcontainer.h"
#include "gtkdata.h"
#include "gtkdraw.h"
#include "gtkmisc.h"
#include "gtkstyle.h"
#include "gtkwidget.h"
#include "gtkprivate.h"

#define ARROW_DEFAULT_SIZE 11

typedef struct _GtkMisc    GtkMisc;
typedef struct _GtkLabel   GtkLabel;
typedef struct _GtkImage   GtkImage;
typedef struct _GtkArrow   GtkArrow;
typedef struct _GtkPixmap  GtkPixmap;

struct _GtkMisc
{
  GtkWidget widget;

  gint16 width;
  gint16 height;
  gdouble xalign;
  gdouble yalign;
};

struct _GtkLabel
{
  GtkMisc misc;
  gchar *label;
};

struct _GtkImage
{
  GtkMisc misc;
  GdkImage *image;
};

struct _GtkArrow
{
  GtkMisc misc;
  GtkArrowType arrow_type;
  GtkShadowType shadow_type;
};

struct _GtkPixmap
{
  GtkMisc misc;
  gint state;
  GdkPixmap *normal;
  GdkPixmap *active;
  GdkPixmap *prelight;
};


static void   gtk_misc_realize       (GtkWidget       *widget);
static void   gtk_misc_size_request  (GtkWidget       *widget,
				      GtkRequisition  *requisition);
static void   gtk_misc_size_allocate (GtkWidget       *widget,
				      GtkAllocation   *allocation);
static gint   gtk_misc_is_child      (GtkWidget       *widget,
				      GtkWidget       *child);
static gint   gtk_misc_locate        (GtkWidget       *widget,
				      GtkWidget      **child,
				      gint             x,
				      gint             y);

static void   gtk_label_destroy       (GtkWidget     *widget);
static void   gtk_label_draw          (GtkWidget     *widget,
				       GdkRectangle  *area,
				       gint           is_expose);
static void   gtk_label_size_allocate (GtkWidget     *widget,
				       GtkAllocation *allocation);
static void   gtk_image_destroy       (GtkWidget     *widget);
static void   gtk_image_draw          (GtkWidget     *widget,
				       GdkRectangle  *area,
				       gint           is_expose);
static void   gtk_arrow_destroy       (GtkWidget     *widget);
static void   gtk_arrow_draw          (GtkWidget     *widget,
				       GdkRectangle  *area,
				       gint           is_expose);
static void   gtk_pixmap_destroy      (GtkWidget     *widget);
static void   gtk_pixmap_draw         (GtkWidget     *widget,
				       GdkRectangle  *area,
				       gint           is_expose);
static void   gtk_pixmap_set_state    (GtkWidget     *widget,
				       GtkStateType   state);


static GtkWidgetFunctions label_widget_functions =
{
  gtk_label_destroy,
  gtk_widget_default_show,
  gtk_widget_default_hide,
  gtk_widget_default_map,
  gtk_widget_default_unmap,
  gtk_misc_realize,
  gtk_label_draw,
  gtk_widget_default_draw_focus,
  gtk_widget_default_event,
  gtk_misc_size_request,
  gtk_label_size_allocate,
  gtk_misc_is_child,
  gtk_misc_locate,
  gtk_widget_default_activate,
  gtk_widget_default_set_state,
  gtk_widget_default_install_accelerator,
  gtk_widget_default_remove_accelerator,
};

static GtkWidgetFunctions image_widget_functions =
{
  gtk_image_destroy,
  gtk_widget_default_show,
  gtk_widget_default_hide,
  gtk_widget_default_map,
  gtk_widget_default_unmap,
  gtk_misc_realize,
  gtk_image_draw,
  gtk_widget_default_draw_focus,
  gtk_widget_default_event,
  gtk_misc_size_request,
  gtk_misc_size_allocate,
  gtk_misc_is_child,
  gtk_misc_locate,
  gtk_widget_default_activate,
  gtk_widget_default_set_state,
  gtk_widget_default_install_accelerator,
  gtk_widget_default_remove_accelerator,
};

static GtkWidgetFunctions arrow_widget_functions =
{
  gtk_arrow_destroy,
  gtk_widget_default_show,
  gtk_widget_default_hide,
  gtk_widget_default_map,
  gtk_widget_default_unmap,
  gtk_misc_realize,
  gtk_arrow_draw,
  gtk_widget_default_draw_focus,
  gtk_widget_default_event,
  gtk_misc_size_request,
  gtk_misc_size_allocate,
  gtk_misc_is_child,
  gtk_misc_locate,
  gtk_widget_default_activate,
  gtk_widget_default_set_state,
  gtk_widget_default_install_accelerator,
  gtk_widget_default_remove_accelerator,
};

static GtkWidgetFunctions pixmap_widget_functions =
{
  gtk_pixmap_destroy,
  gtk_widget_default_show,
  gtk_widget_default_hide,
  gtk_widget_default_map,
  gtk_widget_default_unmap,
  gtk_misc_realize,
  gtk_pixmap_draw,
  gtk_widget_default_draw_focus,
  gtk_widget_default_event,
  gtk_misc_size_request,
  gtk_misc_size_allocate,
  gtk_misc_is_child,
  gtk_misc_locate,
  gtk_widget_default_activate,
  gtk_pixmap_set_state,
  gtk_widget_default_install_accelerator,
  gtk_widget_default_remove_accelerator,
};


GtkWidget*
gtk_label_new (gchar *label)
{
  GtkLabel *label_widget;

  g_function_enter ("gtk_label_new");

  g_assert (label != NULL);

  label_widget = g_new (GtkLabel, 1);

  label_widget->misc.widget.type = gtk_get_label_type ();
  label_widget->misc.widget.function_table = &label_widget_functions;

  gtk_widget_set_defaults ((GtkWidget*) label_widget);
  GTK_WIDGET_SET_FLAGS (label_widget, GTK_NO_WINDOW);

  if (label)
    {
      label_widget->misc.width = gdk_string_width (label_widget->misc.widget.style->font, label) + 2;
      label_widget->misc.height = (label_widget->misc.widget.style->font->ascent +
				   label_widget->misc.widget.style->font->descent) + 2;
    }
  else
    {
      label_widget->misc.width = 0;
      label_widget->misc.height = 0;
    }

  label_widget->misc.xalign = 0.5;
  label_widget->misc.yalign = 0.5;
  label_widget->label = g_strdup (label);

  g_function_leave ("gtk_label_new");
  return ((GtkWidget*) label_widget);
}

GtkWidget*
gtk_image_new (GdkImage *image)
{
  GtkImage *image_widget;

  g_function_enter ("gtk_image_new");

  g_assert (image != NULL);

  image_widget = g_new (GtkImage, 1);

  image_widget->misc.widget.type = gtk_get_image_type ();
  image_widget->misc.widget.function_table = &image_widget_functions;

  gtk_widget_set_defaults ((GtkWidget*) image_widget);

  image_widget->misc.width = image->width;
  image_widget->misc.height = image->height;
  image_widget->misc.xalign = 0.5;
  image_widget->misc.yalign = 0.5;
  image_widget->image = image;

  g_function_leave ("gtk_image_new");
  return ((GtkWidget*) image_widget);
}

GtkWidget*
gtk_arrow_new (GtkArrowType arrow_type,
	       GtkShadowType shadow_type)
{
  GtkArrow *arrow_widget;

  g_function_enter ("gtk_arrow_new");

  arrow_widget = g_new (GtkArrow, 1);

  arrow_widget->misc.widget.type = gtk_get_arrow_type ();
  arrow_widget->misc.widget.function_table = &arrow_widget_functions;

  gtk_widget_set_defaults ((GtkWidget*) arrow_widget);

  arrow_widget->misc.width = ARROW_DEFAULT_SIZE;
  arrow_widget->misc.height = ARROW_DEFAULT_SIZE;
  arrow_widget->misc.xalign = 0.5;
  arrow_widget->misc.yalign = 0.5;
  arrow_widget->arrow_type = arrow_type;
  arrow_widget->shadow_type = shadow_type;

  g_function_leave ("gtk_arrow_new");
  return ((GtkWidget*) arrow_widget);
}


GtkWidget*
gtk_pixmap_new (GdkPixmap *normal,
		GdkPixmap *active,
		GdkPixmap *prelight)
{
  GtkPixmap *pixmap_widget;

  g_function_enter ("gtk_pixmap_new");

  g_assert (normal != NULL);

  if (!active)
    active = normal;

  if (!prelight)
    prelight = normal;

  pixmap_widget = g_new (GtkPixmap, 1);

  pixmap_widget->misc.widget.type = gtk_get_pixmap_type ();
  pixmap_widget->misc.widget.function_table = &pixmap_widget_functions;

  gtk_widget_set_defaults ((GtkWidget*) pixmap_widget);

  pixmap_widget->misc.width = normal->width;
  pixmap_widget->misc.height = normal->height;
  pixmap_widget->misc.xalign = 0.5;
  pixmap_widget->misc.yalign = 0.5;
  pixmap_widget->state = GTK_STATE_NORMAL;
  pixmap_widget->normal = normal;
  pixmap_widget->active = active;
  pixmap_widget->prelight = prelight;

  g_function_leave ("gtk_pixmap_new");
  return ((GtkWidget*) pixmap_widget);
}

void
gtk_label_get (GtkWidget  *widget,
	       gchar     **label)
{
  GtkLabel *label_widget;

  g_function_enter ("gtk_label_get");

  g_assert (widget != NULL);
  label_widget = (GtkLabel*) widget;
  *label = label_widget->label;

  g_function_leave ("gtk_label_get");
}

void
gtk_image_get (GtkWidget  *widget,
	       GdkImage  **image)
{
  GtkImage *image_widget;

  g_function_enter ("gtk_image_get");

  g_assert (widget != NULL);
  image_widget = (GtkImage*) widget;
  *image = image_widget->image;

  g_function_leave ("gtk_image_get");
}

void
gtk_pixmap_get (GtkWidget  *widget,
		GdkPixmap **normal,
		GdkPixmap **active,
		GdkPixmap **prelight)
{
  GtkPixmap *pixmap_widget;

  g_function_enter ("gtk_pixmap_get");

  g_assert (widget != NULL);
  pixmap_widget = (GtkPixmap*) widget;
  *normal = pixmap_widget->normal;
  *active = pixmap_widget->active;
  *prelight = pixmap_widget->prelight;

  g_function_leave ("gtk_pixmap_get");
}

void
gtk_label_set (GtkWidget  *widget,
	       gchar      *label)
{
  GtkLabel *label_widget;

  g_function_enter ("gtk_label_set");

  g_assert (widget != NULL);
  g_assert (label != NULL);

  label_widget = (GtkLabel*) widget;
  if (label_widget->label)
    g_free (label_widget->label);
  label_widget->label = g_strdup (label);
  label_widget->misc.width = gdk_string_width (label_widget->misc.widget.style->font, label) + 2;

  if (GTK_WIDGET_VISIBLE (widget))
    gtk_container_need_resize (widget->parent, widget);

  g_function_leave ("gtk_label_set");
}

void
gtk_image_set (GtkWidget  *widget,
	       GdkImage   *image)
{
  GtkImage *image_widget;

  g_function_enter ("gtk_image_set");

  g_assert (widget != NULL);
  g_assert (image != NULL);

  image_widget = (GtkImage*) widget;
  if (image_widget->image != image)
    {
      if (image_widget->image)
	gdk_image_destroy (image_widget->image);
      image_widget->image = image;

      if (GTK_WIDGET_VISIBLE (widget))
	gtk_container_need_resize (widget->parent, widget);
    }

  g_function_leave ("gtk_image_set");
}

void
gtk_pixmap_set (GtkWidget  *widget,
		GdkPixmap  *normal,
		GdkPixmap  *active,
		GdkPixmap  *prelight)
{
  GtkPixmap *pixmap_widget;

  g_function_enter ("gtk_pixmap_set");

  g_assert (widget != NULL);
  g_assert (normal != NULL);

  if (!active)
    active = normal;
  if (!prelight)
    prelight = normal;

  pixmap_widget = (GtkPixmap*) widget;

  if (pixmap_widget->normal == pixmap_widget->active)
    pixmap_widget->active = NULL;
  if (pixmap_widget->normal == pixmap_widget->prelight)
    pixmap_widget->prelight = NULL;

  if (pixmap_widget->normal)
    gdk_pixmap_destroy (pixmap_widget->normal);
  if (pixmap_widget->active)
    gdk_pixmap_destroy (pixmap_widget->active);
  if (pixmap_widget->prelight)
    gdk_pixmap_destroy (pixmap_widget->prelight);

  pixmap_widget->normal = normal;
  pixmap_widget->active = active;
  pixmap_widget->prelight = prelight;

  if (GTK_WIDGET_VISIBLE (widget))
    gtk_container_need_resize (widget->parent, widget);

  g_function_leave ("gtk_pixmap_set");
}

void
gtk_label_set_alignment (GtkWidget *label,
			 gdouble    xalign,
			 gdouble    yalign)
{
  GtkMisc *misc;

  g_function_enter ("gtk_label_set_alignment");

  g_assert (label != NULL);

  misc = (GtkMisc*) label;

  xalign = CLAMP (xalign, 0.0, 1.0);
  yalign = CLAMP (yalign, 0.0, 1.0);

  if ((xalign != misc->xalign) || (yalign != misc->yalign))
    {
      misc->xalign = xalign;
      misc->yalign = yalign;

      gtk_widget_draw (label, NULL, FALSE);
    }

  g_function_leave ("gtk_label_set_alignment");
}

void
gtk_image_set_alignment (GtkWidget *image,
			 gdouble    xalign,
			 gdouble    yalign)
{
  g_function_enter ("gtk_image_set_alignment");

  g_assert (image != NULL);

  gtk_label_set_alignment (image, xalign, yalign);

  g_function_leave ("gtk_image_set_alignment");
}

void
gtk_arrow_set_alignment (GtkWidget *arrow,
			 gdouble    xalign,
			 gdouble    yalign)
{
  g_function_enter ("gtk_arrow_set_alignment");

  g_assert (arrow != NULL);

  gtk_label_set_alignment (arrow, xalign, yalign);

  g_function_leave ("gtk_arrow_set_alignment");
}

void
gtk_pixmap_set_alignment (GtkWidget *pixmap,
			  gdouble    xalign,
			  gdouble    yalign)
{
  g_function_enter ("gtk_pixmap_set_alignment");

  g_assert (pixmap != NULL);

  gtk_label_set_alignment (pixmap, xalign, yalign);

  g_function_leave ("gtk_pixmap_set_alignment");
}

guint16
gtk_get_label_type ()
{
  static guint16 label_type = 0;

  g_function_enter ("gtk_get_label_type");

  if (!label_type)
    gtk_widget_unique_type (&label_type);

  g_function_leave ("gtk_get_label_type");
  return label_type;
}

guint16
gtk_get_image_type ()
{
  static guint16 image_type = 0;

  g_function_enter ("gtk_get_image_type");

  if (!image_type)
    image_type = gtk_get_label_type ();

  g_function_leave ("gtk_get_image_type");
  return image_type;
}

guint16
gtk_get_arrow_type ()
{
  static guint16 arrow_type = 0;

  g_function_enter ("gtk_get_arrow_type");

  if (!arrow_type)
    arrow_type = gtk_get_label_type ();

  g_function_leave ("gtk_get_arrow_type");
  return arrow_type;
}

guint16
gtk_get_pixmap_type ()
{
  static guint16 pixmap_type = 0;

  g_function_enter ("gtk_get_pixmap_type");

  if (!pixmap_type)
    pixmap_type = gtk_get_label_type ();

  g_function_leave ("gtk_get_pixmap_type");
  return pixmap_type;
}


static void
gtk_misc_realize (GtkWidget *widget)
{
  GtkMisc *misc;
  GdkWindowAttr attributes;

  g_function_enter ("gtk_misc_realize");

  g_assert (widget != NULL);

  misc = (GtkMisc*) widget;
  GTK_WIDGET_SET_FLAGS (widget, GTK_REALIZED);

  if (GTK_WIDGET_NO_WINDOW (widget))
    {
      widget->window = widget->parent->widget.window;
      widget->style = gtk_style_attach (widget->style, widget->window);
    }
  else
    {
      attributes.window_type = GDK_WINDOW_CHILD;
      attributes.x = widget->allocation.x;
      attributes.y = widget->allocation.y;
      attributes.width = widget->allocation.width;
      attributes.height = widget->allocation.height;
      attributes.wclass = GDK_INPUT_OUTPUT;
      attributes.event_mask = GDK_EXPOSURE_MASK;

      widget->window = gdk_window_new (widget->parent->widget.window,
				       &attributes, GDK_WA_X | GDK_WA_Y);
      gdk_window_set_user_data (widget->window, widget);

      widget->style = gtk_style_attach (widget->style, widget->window);
      gdk_window_set_background (widget->window,
				 &widget->style->background[GTK_STATE_NORMAL]);
    }

  g_function_leave ("gtk_misc_realize");
}

static void
gtk_misc_size_request (GtkWidget      *widget,
		       GtkRequisition *requisition)
{
  GtkMisc *misc;

  g_function_enter ("gtk_misc_size_request");

  g_assert (widget != NULL);
  g_assert (requisition != NULL);

  misc = (GtkMisc*) widget;

  requisition->width = misc->width;
  requisition->height = misc->height;

  g_function_leave ("gtk_misc_size_request");
}

static void
gtk_misc_size_allocate (GtkWidget     *widget,
			GtkAllocation *allocation)
{
  g_function_enter ("gtk_misc_size_allocate");

  g_assert (widget != NULL);
  g_assert (allocation != NULL);

  widget->allocation = *allocation;
  if (GTK_WIDGET_REALIZED (widget))
    {
      gdk_window_move (widget->window,
		       allocation->x,
		       allocation->y);
      gdk_window_set_size (widget->window,
			   allocation->width,
			   allocation->height);
    }

  g_function_leave ("gtk_misc_size_allocate");
}

static gint
gtk_misc_is_child (GtkWidget *widget,
		   GtkWidget *child)
{
  g_function_enter ("gtk_misc_is_child");

  g_assert (widget != NULL);
  g_assert (child != NULL);

  g_function_leave ("gtk_misc_is_child");
  return FALSE;
}

static gint
gtk_misc_locate (GtkWidget  *widget,
		 GtkWidget **child,
		 gint      x,
		 gint      y)
{
  gint return_val;

  g_function_enter ("gtk_misc_locate");

  g_assert (widget != NULL);
  g_assert (child != NULL);

  return_val = FALSE;
  *child = NULL;

  if ((x >= 0) && (y >= 0) &&
      (x < widget->allocation.width) &&
      (y < widget->allocation.height))
    {
      return_val = TRUE;
      *child = widget;
    }

  g_function_leave ("gtk_misc_locate");
  return return_val;
}

static void
gtk_label_destroy (GtkWidget *widget)
{
  GtkLabel *label;

  g_function_enter ("gtk_label_destroy");

  g_assert (widget != NULL);
  label = (GtkLabel*) widget;

  g_free (label->label);
  g_free (label);

  g_function_leave ("gtk_label_destroy");
}

static void
gtk_label_draw (GtkWidget    *widget,
		GdkRectangle *area,
		gint          is_expose)
{
  GtkLabel *label;
  gint x, y;

  g_function_enter ("gtk_label_draw");

  g_assert (widget != NULL);

  if (GTK_WIDGET_VISIBLE (widget) && GTK_WIDGET_MAPPED (widget))
    {
      label = (GtkLabel*) widget;

      if ((widget->allocation.width >= widget->requisition.width) &&
	  (widget->allocation.height >= widget->requisition.height))
	{
	  x = (widget->allocation.x * (1 - label->misc.xalign) +
	       (widget->allocation.x + widget->allocation.width - label->misc.width) *
	       label->misc.xalign);
	  y = (widget->allocation.y * (1 - label->misc.yalign) +
	       (widget->allocation.y + widget->allocation.height - label->misc.height) *
	       label->misc.yalign);

	  x += 1;
	  y += widget->style->foreground_gc[0]->font->ascent + 1;

	  gdk_draw_string (widget->window,
			   widget->style->foreground_gc[GTK_WIDGET_IS_SENSITIVE (widget)],
			   x, y,
			   label->label);
	}
      else
	{
	  gdk_draw_string (widget->window,
			   widget->style->foreground_gc[GTK_WIDGET_IS_SENSITIVE (widget)],
			   widget->allocation.x + widget->allocation.width / 2 -
			   label->misc.width / 2 + 1,
			   widget->allocation.y + widget->allocation.height / 2 -
			   label->misc.height / 2 + widget->style->foreground_gc[0]->font->ascent + 1,
			   label->label);
	}
    }

  g_function_leave ("gtk_label_draw");
}

static void
gtk_label_size_allocate (GtkWidget     *widget,
			 GtkAllocation *allocation)
{
  g_function_enter ("gtk_label_allocate");

  g_assert (widget != NULL);
  g_assert (allocation != NULL);

  if (GTK_WIDGET_REALIZED (widget))
    gdk_window_clear_area (widget->window,
			   widget->allocation.x + 1,
			   widget->allocation.y + 1,
			   widget->allocation.width - 2,
			   widget->allocation.height - 2);
  widget->allocation = *allocation;
  gtk_widget_draw (widget, NULL, FALSE);

  g_function_leave ("gtk_label_size_allocate");
}

static void
gtk_image_destroy (GtkWidget *widget)
{
  GtkImage *image;

  g_function_enter ("gtk_image_destroy");

  g_assert (widget != NULL);
  image = (GtkImage*) widget;

  if (image->misc.widget.window)
    gdk_window_destroy (image->misc.widget.window);
  gdk_image_destroy (image->image);
  g_free (image);

  g_function_leave ("gtk_image_destroy");
}

static void
gtk_image_draw (GtkWidget    *widget,
		GdkRectangle *area,
		gint          is_expose)
{
  GtkImage *image;

  g_function_enter ("gtk_image_draw");

  g_assert (widget != NULL);

  if (GTK_WIDGET_VISIBLE (widget) && GTK_WIDGET_MAPPED (widget))
    {
      image = (GtkImage*) widget;

      if ((image->image->width != widget->window->width) ||
	  (image->image->height != widget->window->height))
	gdk_window_clear (widget->window);

      gdk_draw_image (widget->window,
		      widget->style->foreground_gc[GTK_WIDGET_IS_SENSITIVE (widget)],
		      image->image,
		      0, 0,
		      widget->allocation.width / 2 - image->misc.width / 2,
		      widget->allocation.height / 2 - image->misc.height / 2,
		      image->misc.width,
		      image->misc.height);
    }

  g_function_leave ("gtk_image_draw");
}

static void
gtk_arrow_destroy (GtkWidget *widget)
{
  GtkArrow *arrow;

  g_function_enter ("gtk_arrow_destroy");

  g_assert (widget != NULL);
  arrow = (GtkArrow*) widget;

  if (arrow->misc.widget.window)
    gdk_window_destroy (arrow->misc.widget.window);
  g_free (arrow);

  g_function_leave ("gtk_arrow_destroy");
}

static void
gtk_arrow_draw (GtkWidget    *widget,
		GdkRectangle *area,
		gint          is_expose)
{
  GtkArrow *arrow;
  GtkStateType state_type;
  gint extent;

  g_function_enter ("gtk_arrow_draw");

  g_assert (widget != NULL);

  if (GTK_WIDGET_VISIBLE (widget) && GTK_WIDGET_MAPPED (widget))
    {
      arrow = (GtkArrow*) widget;

      gdk_window_clear (widget->window);

      extent = MIN (widget->allocation.width, widget->allocation.height);

      state_type = GTK_WIDGET_IS_SENSITIVE (widget);
      gtk_draw_arrow (widget->window,
		      widget->style->highlight_gc[state_type],
		      widget->style->shadow_gc[state_type],
		      widget->style->background_gc[state_type],
		      arrow->arrow_type,
		      arrow->shadow_type,
		      widget->allocation.width / 2 - extent / 2,
		      widget->allocation.height / 2 - extent / 2,
		      extent, extent,
		      widget->style->shadow_thickness);
    }

  g_function_leave ("gtk_arrow_draw");
}

static void
gtk_pixmap_destroy (GtkWidget *widget)
{
  GtkPixmap *pixmap;

  g_function_enter ("gtk_pixmap_destroy");

  g_assert (widget != NULL);
  pixmap = (GtkPixmap*) widget;

  if (pixmap->misc.widget.window)
    gdk_window_destroy (pixmap->misc.widget.window);

  if (pixmap->normal == pixmap->active)
    pixmap->active = NULL;
  if (pixmap->normal == pixmap->prelight)
    pixmap->prelight = NULL;

  if (pixmap->normal)
    gdk_pixmap_destroy (pixmap->normal);
  if (pixmap->active)
    gdk_pixmap_destroy (pixmap->active);
  if (pixmap->prelight)
    gdk_pixmap_destroy (pixmap->prelight);

  g_free (pixmap);

  g_function_leave ("gtk_pixmap_destroy");
}

static void
gtk_pixmap_draw (GtkWidget    *widget,
		 GdkRectangle *area,
		 gint          is_expose)
{
  GtkPixmap *pixmap;
  GdkPixmap *the_pixmap;

  g_function_enter ("gtk_pixmap_expose");

  g_assert (widget != NULL);

  if (GTK_WIDGET_VISIBLE (widget) && GTK_WIDGET_MAPPED (widget))
    {
      pixmap = (GtkPixmap*) widget;
      switch (pixmap->state)
	{
	case GTK_STATE_NORMAL:
	  the_pixmap = pixmap->normal;
	  break;
	case GTK_STATE_ACTIVE:
	  the_pixmap = pixmap->active;
	  break;
	case GTK_STATE_PRELIGHT:
	  the_pixmap = pixmap->prelight;
	  break;
	default:
	  the_pixmap = pixmap->normal;
	  break;
	}

      gdk_window_clear (widget->window);

      gdk_draw_pixmap (widget->window,
		       widget->style->foreground_gc[GTK_WIDGET_IS_SENSITIVE (widget)],
		       the_pixmap,
		       0, 0,
		       widget->allocation.width / 2 - pixmap->misc.width / 2,
		       widget->allocation.height / 2 - pixmap->misc.height / 2,
		       pixmap->misc.width,
		       pixmap->misc.height);
    }

  g_function_leave ("gtk_pixmap_draw");
}

static void
gtk_pixmap_set_state (GtkWidget    *widget,
		      GtkStateType  state)
{
  GtkPixmap *pixmap;

  g_function_enter ("gtk_pixmap_set_state");

  g_assert (widget != NULL);
  pixmap = (GtkPixmap*) widget;
  pixmap->state = state;

  g_function_leave ("gtk_pixmap_set_state");
}
