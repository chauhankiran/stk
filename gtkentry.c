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
#include <ctype.h>
#include <string.h>
#include "gtkcontainer.h"
#include "gtkdraw.h"
#include "gtkentry.h"
#include "gtkstyle.h"
#include "gtkwidget.h"
#include "gtkprivate.h"


#define MIN_ENTRY_WIDTH  150
#define DRAW_TIMEOUT     20


typedef struct _GtkEntry  GtkEntry;
typedef void (*GtkTextFunction) (GtkEntry *);

struct _GtkEntry
{
  GtkWidget widget;
  GdkWindow *text_area;

  gchar *text;
  gint16 text_size;
  gint16 text_length;
  gint16 current_pos;
  gint16 select_start_pos;
  gint16 select_end_pos;
  gint16 scroll_offset;
  guint32 timer;

  GtkKeyFunction key_function;
  gpointer key_function_data;
};


static void  gtk_text_entry_destroy       (GtkWidget      *widget);
static void  gtk_text_entry_realize       (GtkWidget      *widget);
static void  gtk_text_entry_draw          (GtkWidget      *widget,
					   GdkRectangle   *area,
					   gint            is_expose);
static void  gtk_text_entry_draw_text     (GtkWidget      *widget);
static void  gtk_text_entry_draw_cursor   (GtkWidget      *widget);
static void  gtk_text_entry_draw_focus    (GtkWidget      *widget);
static gint  gtk_text_entry_event         (GtkWidget      *widget,
					   GdkEvent       *event);
static void  gtk_text_entry_size_request  (GtkWidget      *widget,
					   GtkRequisition *requisition);
static void  gtk_text_entry_size_allocate (GtkWidget      *widget,
					   GtkAllocation  *allocation);
static gint  gtk_text_entry_is_child      (GtkWidget      *widget,
					   GtkWidget      *child);
static gint  gtk_text_entry_locate        (GtkWidget      *widget,
					   GtkWidget     **child,
					   gint            x,
					   gint            y);
static void  gtk_text_entry_set_state     (GtkWidget      *widget,
					   GtkStateType    state);

static void  gtk_text_entry_queue_draw    (GtkEntry       *entry);
static gint  gtk_text_entry_timer         (gpointer        data);

static gint  gtk_text_entry_position      (GtkEntry       *entry,
					   gint            x);

static void  gtk_text_entry_adjust_scroll (GtkEntry       *entry);
static gint  gtk_text_entry_handle_key    (GtkEntry       *entry,
					   GdkEvent       *event);
static void  gtk_text_entry_grow_text     (GtkEntry       *entry);
static void  gtk_text_entry_insert_text   (GtkEntry       *entry,
					   gchar          *new_text,
					   gint            new_text_length);
static void  gtk_text_entry_delete_text   (GtkEntry       *entry,
					   gint            start_pos,
					   gint            end_pos);

static void  gtk_move_forward_character    (GtkEntry *entry);
static void  gtk_move_backward_character   (GtkEntry *entry);
static void  gtk_move_forward_word         (GtkEntry *entry);
static void  gtk_move_backward_word        (GtkEntry *entry);
static void  gtk_move_beginning_of_line    (GtkEntry *entry);
static void  gtk_move_end_of_line          (GtkEntry *entry);
static void  gtk_delete_forward_character  (GtkEntry *entry);
static void  gtk_delete_backward_character (GtkEntry *entry);
static void  gtk_delete_forward_word       (GtkEntry *entry);
static void  gtk_delete_backward_word      (GtkEntry *entry);
static void  gtk_delete_line               (GtkEntry *entry);
static void  gtk_delete_to_line_end        (GtkEntry *entry);
static void  gtk_delete_selection          (GtkEntry *entry);
static void  gtk_select_word               (GtkEntry *entry);
static void  gtk_select_line               (GtkEntry *entry);


static GtkWidgetFunctions text_entry_widget_functions =
{
  gtk_text_entry_destroy,
  gtk_widget_default_show,
  gtk_widget_default_hide,
  gtk_widget_default_map,
  gtk_widget_default_unmap,
  gtk_text_entry_realize,
  gtk_text_entry_draw,
  gtk_text_entry_draw_focus,
  gtk_text_entry_event,
  gtk_text_entry_size_request,
  gtk_text_entry_size_allocate,
  gtk_text_entry_is_child,
  gtk_text_entry_locate,
  gtk_widget_default_activate,
  gtk_text_entry_set_state,
  gtk_widget_default_install_accelerator,
  gtk_widget_default_remove_accelerator,
};

static GtkTextFunction control_keys[26] =
{
  gtk_move_beginning_of_line,    /* a */
  gtk_move_backward_character,   /* b */
  NULL,                          /* c */
  gtk_delete_forward_character,  /* d */
  gtk_move_end_of_line,          /* e */
  gtk_move_forward_character,    /* f */
  NULL,                          /* g */
  NULL,                          /* h */
  NULL,                          /* i */
  NULL,                          /* j */
  gtk_delete_to_line_end,        /* k */
  NULL,                          /* l */
  NULL,                          /* m */
  NULL,                          /* n */
  NULL,                          /* o */
  NULL,                          /* p */
  NULL,                          /* q */
  NULL,                          /* r */
  NULL,                          /* s */
  NULL,                          /* t */
  gtk_delete_line,               /* u */
  NULL,                          /* v */
  gtk_delete_backward_word,      /* w */
  NULL,                          /* x */
  NULL,                          /* y */
  NULL,                          /* z */
};

static GtkTextFunction alt_keys[26] =
{
  NULL,                          /* a */
  gtk_move_backward_word,        /* b */
  NULL,                          /* c */
  gtk_delete_forward_word,       /* d */
  NULL,                          /* e */
  gtk_move_forward_word,         /* f */
  NULL,                          /* g */
  NULL,                          /* h */
  NULL,                          /* i */
  NULL,                          /* j */
  NULL,                          /* k */
  NULL,                          /* l */
  NULL,                          /* m */
  NULL,                          /* n */
  NULL,                          /* o */
  NULL,                          /* p */
  NULL,                          /* q */
  NULL,                          /* r */
  NULL,                          /* s */
  NULL,                          /* t */
  NULL,                          /* u */
  NULL,                          /* v */
  NULL,                          /* w */
  NULL,                          /* x */
  NULL,                          /* y */
  NULL,                          /* z */
};


GtkWidget*
gtk_text_entry_new ()
{
  GtkEntry *entry;

  g_function_enter ("gtk_text_entry_new");

  entry = g_new (GtkEntry, 1);

  entry->widget.type = gtk_get_text_entry_type ();
  entry->widget.function_table = &text_entry_widget_functions;

  gtk_widget_set_defaults ((GtkWidget*) entry);
  GTK_WIDGET_SET_FLAGS (entry, GTK_CAN_FOCUS);

  entry->text_area = NULL;
  entry->text = NULL;
  entry->text_size = 0;
  entry->text_length = 0;
  entry->current_pos = 0;
  entry->select_start_pos = 0;
  entry->select_end_pos = 0;
  entry->scroll_offset = 0;
  entry->timer = 0;

  entry->key_function = NULL;
  entry->key_function_data = NULL;

  g_function_leave ("gtk_text_entry_new");
  return ((GtkWidget*) entry);
}

gchar*
gtk_text_entry_get_text (GtkWidget *widget)
{
  GtkEntry *entry;
  gchar *text;

  g_function_enter ("gtk_text_entry_get_text");

  g_assert (widget != NULL);
  entry = (GtkEntry*) widget;
  text = entry->text;

  g_function_leave ("gtk_text_entry_get_text");
  return text;
}

void
gtk_text_entry_set_text (GtkWidget *widget,
			 gchar     *text)
{
  GtkEntry *entry;
  gchar *old_text;

  g_function_enter ("gtk_text_entry_set_text");

  g_assert (widget != NULL);
  g_assert (text != NULL);

  entry = (GtkEntry*) widget;

  old_text = entry->text;

  entry->text_length = strlen (text);
  entry->text_size = entry->text_length + 1;
  entry->text = g_new (gchar, entry->text_size);
  strcpy (entry->text, text);

  if (old_text)
    g_free (old_text);

  entry->current_pos = 0;
  entry->select_start_pos = 0;
  entry->select_end_pos = 0;

  gtk_widget_draw (widget, NULL, FALSE);

  g_function_leave ("gtk_text_entry_set_text");
}

void
gtk_text_entry_append_text (GtkWidget *widget,
			    gchar     *text)
{
  GtkEntry *entry;
  gint old_pos;

  g_function_enter ("gtk_text_entry_append_text");

  g_assert (widget != NULL);
  g_assert (text != NULL);

  entry = (GtkEntry*) widget;

  old_pos = entry->current_pos;
  entry->current_pos = entry->text_length;
  gtk_text_entry_insert_text (entry, text, strlen (text));
  entry->current_pos = old_pos;

  g_function_leave ("gtk_text_entry_append_text");
}

void
gtk_text_entry_prepend_text (GtkWidget *widget,
			     gchar     *text)
{
  GtkEntry *entry;
  gint old_pos;

  g_function_enter ("gtk_text_entry_prepend_text");

  g_assert (widget != NULL);
  g_assert (text != NULL);

  entry = (GtkEntry*) widget;

  old_pos = entry->current_pos;
  entry->current_pos = 0;
  gtk_text_entry_insert_text (entry, text, strlen (text));
  entry->current_pos = old_pos;

  g_function_leave ("gtk_text_entry_prepend_text");
}

void
gtk_text_entry_set_position (GtkWidget *widget,
			     gint       position)
{
  GtkEntry *entry;

  g_function_enter ("gtk_text_entry_set_position");

  g_assert (widget != NULL);
  entry = (GtkEntry*) widget;

  if ((position == -1) || (position > entry->text_length))
    entry->current_pos = entry->text_length;
  else
    entry->current_pos = position;

  g_function_leave ("gtk_text_entry_set_position");
}

void
gtk_text_entry_set_key_function (GtkWidget       *widget,
				 GtkKeyFunction  function,
				 gpointer         data)
{
  GtkEntry *entry;

  g_function_enter ("gtk_text_entry_set_key_function");

  g_assert (widget != NULL);
  entry = (GtkEntry*) widget;

  entry->key_function = function;
  entry->key_function_data = data;

  g_function_leave ("gtk_text_entry_set_key_function");
}

guint16
gtk_get_text_entry_type ()
{
  static guint16 text_entry_type = 0;

  g_function_enter ("gtk_get_text_entry_type");

  if (!text_entry_type)
    gtk_widget_unique_type (&text_entry_type);

  g_function_leave ("gtk_get_text_entry_type");
  return text_entry_type;
}


static void
gtk_text_entry_destroy (GtkWidget *widget)
{
  GtkEntry *entry;

  g_function_enter ("gtk_text_entry_destroy");

  g_assert (widget != NULL);

  entry = (GtkEntry*) widget;
  if (entry->text_area)
    gdk_window_destroy (entry->text_area);
  if (entry->widget.window)
    gdk_window_destroy (entry->widget.window);
  if (entry->text)
    g_free (entry->text);
  g_free (entry);

  g_function_leave ("gtk_text_entry_destroy");
}

static void
gtk_text_entry_realize (GtkWidget *widget)
{
  GtkEntry *entry;
  GdkWindowAttr attributes;
  gint attributes_mask;

  g_function_enter ("gtk_text_entry_realize");

  g_assert (widget != NULL);

  entry = (GtkEntry*) widget;
  GTK_WIDGET_SET_FLAGS (widget, GTK_REALIZED);

  attributes.window_type = GDK_WINDOW_CHILD;
  attributes.x = widget->allocation.x;
  attributes.y = widget->allocation.y;
  attributes.width = widget->allocation.width;
  attributes.height = widget->allocation.height;
  attributes.wclass = GDK_INPUT_OUTPUT;
  attributes.visual = gtk_peek_visual ();
  attributes.colormap = gtk_peek_colormap ();
  attributes.event_mask = (GDK_EXPOSURE_MASK |
			   GDK_BUTTON_PRESS_MASK |
			   GDK_BUTTON_RELEASE_MASK |
			   GDK_BUTTON_MOTION_MASK |
			   GDK_ENTER_NOTIFY_MASK |
			   GDK_LEAVE_NOTIFY_MASK |
			   GDK_KEY_PRESS_MASK);
  attributes_mask = GDK_WA_X | GDK_WA_Y | GDK_WA_VISUAL | GDK_WA_COLORMAP;

  entry->widget.window = gdk_window_new (widget->parent->widget.window,
					 &attributes, attributes_mask);
  gdk_window_set_user_data (entry->widget.window, entry);

  attributes.x = widget->style->shadow_thickness + 2;
  attributes.y = widget->style->shadow_thickness + 2;
  attributes.width = widget->allocation.width - attributes.x * 2;
  attributes.width = widget->allocation.height - attributes.y * 2;

  entry->text_area = gdk_window_new (entry->widget.window,
				     &attributes, attributes_mask);
  gdk_window_set_user_data (entry->text_area, entry);

  entry->widget.style = gtk_style_attach (entry->widget.style,
					  entry->widget.window);
  gdk_window_set_background (entry->widget.window,
			     &entry->widget.style->background[GTK_STATE_NORMAL]);
  gdk_window_set_background (entry->text_area,
			     &entry->widget.style->background[GTK_STATE_NORMAL]);

  gdk_window_show (entry->text_area);

  g_function_leave ("gtk_text_entry_realize");
}

static void
gtk_text_entry_draw (GtkWidget    *widget,
		     GdkRectangle *area,
		     gint          is_expose)
{
  GtkEntry *entry;

  g_function_enter ("gtk_text_entry_draw");

  g_assert (widget != NULL);

  if (GTK_WIDGET_VISIBLE (widget) && GTK_WIDGET_MAPPED (widget))
    {
      entry = (GtkEntry*) widget;

      gtk_draw_shadow (widget->window,
		       widget->style->highlight_gc[GTK_STATE_NORMAL],
		       widget->style->shadow_gc[GTK_STATE_NORMAL],
		       NULL,
		       GTK_SHADOW_IN,
		       1, 1,
		       widget->window->width - 2,
		       widget->window->height - 2,
		       widget->style->shadow_thickness);

      gtk_text_entry_draw_text (widget);
    }

  g_function_leave ("gtk_text_entry_draw");
}

static void
gtk_text_entry_draw_text (GtkWidget *widget)
{
  GtkEntry *entry;
  gint select_start_pos;
  gint select_end_pos;
  gint select_start_xoffset;
  gint select_end_xoffset;

  g_function_enter ("gtk_text_entry_draw_cursor");

  g_assert (widget != NULL);

  if (GTK_WIDGET_VISIBLE (widget) && GTK_WIDGET_MAPPED (widget))
    {
      entry = (GtkEntry*) widget;

      gdk_window_clear (entry->text_area);

      if (entry->text)
	{
	  if (entry->select_start_pos != entry->select_end_pos)
	    {
	      select_start_pos = MIN (entry->select_start_pos, entry->select_end_pos);
	      select_end_pos = MAX (entry->select_start_pos, entry->select_end_pos);

	      select_start_xoffset = gdk_text_width (widget->style->font,
						     entry->text,
						     select_start_pos);
	      select_end_xoffset = gdk_text_width (widget->style->font,
						   entry->text,
						   select_end_pos);

	      if (select_start_pos > 0)
		gdk_draw_text (entry->text_area,
			       widget->style->foreground_gc[GTK_STATE_NORMAL],
			       -entry->scroll_offset, widget->style->font->ascent + 1,
			       entry->text, select_start_pos);

	      gdk_draw_rectangle (entry->text_area,
				  widget->style->background_gc[GTK_STATE_SELECTED],
				  TRUE,
				  -entry->scroll_offset + select_start_xoffset,
				  0,
				  select_end_xoffset - select_start_xoffset,
				  entry->text_area->height);

	      gdk_draw_text (entry->text_area,
			     widget->style->foreground_gc[GTK_STATE_SELECTED],
			     -entry->scroll_offset + select_start_xoffset,
			     widget->style->font->ascent + 1,
			     entry->text + select_start_pos,
			     select_end_pos - select_start_pos);

	      if (select_end_pos < entry->text_length)
		gdk_draw_string (entry->text_area,
				 widget->style->foreground_gc[GTK_STATE_NORMAL],
				 -entry->scroll_offset + select_end_xoffset,
				 widget->style->font->ascent + 1,
				 entry->text + select_end_pos);
	    }
	  else
	    {
	      gdk_draw_string (entry->text_area,
			       widget->style->foreground_gc[GTK_STATE_NORMAL],
			       -entry->scroll_offset, widget->style->font->ascent + 1,
			       entry->text);
	    }
	}

      gtk_text_entry_draw_cursor (widget);
    }

  g_function_leave ("gtk_text_entry_draw_cursor");
}

static void
gtk_text_entry_draw_cursor (GtkWidget *widget)
{
  GtkEntry *entry;
  GdkGC *gc;
  gint xoffset;

  g_function_enter ("gtk_text_entry_draw_cursor");

  g_assert (widget != NULL);

  if (GTK_WIDGET_VISIBLE (widget) && GTK_WIDGET_MAPPED (widget))
    {
      entry = (GtkEntry*) widget;

      if (entry->current_pos > 0)
	xoffset = gdk_text_width (widget->style->font, entry->text, entry->current_pos);
      else
	xoffset = 0;
      xoffset -= entry->scroll_offset;

      if (GTK_WIDGET_HAS_FOCUS (widget) &&
	  (entry->select_start_pos == entry->select_end_pos))
	gc = widget->style->foreground_gc[GTK_STATE_NORMAL];
      else
	gc = widget->style->background_gc[GTK_STATE_NORMAL];

      gdk_draw_line (entry->text_area, gc,
		     xoffset, 0,
		     xoffset, entry->text_area->height);
    }

  g_function_leave ("gtk_text_entry_draw_cursor");
}

static void
gtk_text_entry_draw_focus (GtkWidget *widget)
{
  g_function_enter ("gtk_text_entry_draw_focus");

  g_assert (widget != NULL);

  if (GTK_WIDGET_VISIBLE (widget) && GTK_WIDGET_MAPPED (widget))
    {
      if (GTK_WIDGET_HAS_FOCUS (widget))
	gdk_draw_rectangle (widget->window,
			    widget->style->foreground_gc[GTK_STATE_NORMAL],
			    FALSE, 0, 0,
			    widget->window->width - 1,
			    widget->window->height - 1);
      else
	gdk_draw_rectangle (widget->window,
			    widget->style->background_gc[GTK_STATE_NORMAL],
			    FALSE, 0, 0,
			    widget->window->width - 1,
			    widget->window->height - 1);

      gtk_text_entry_draw_cursor (widget);
    }

  g_function_leave ("gtk_text_entry_draw_focus");
}

static gint
gtk_text_entry_event (GtkWidget *widget,
		      GdkEvent  *event)
{
  GtkEntry *entry;
  gint return_val;

  g_function_enter ("gtk_text_entry_event");

  g_assert (widget != NULL);
  g_assert (event != NULL);

  entry = (GtkEntry*) widget;
  return_val = FALSE;

  switch (event->type)
    {
    case GDK_EXPOSE:
      gtk_widget_draw (widget, &event->expose.area, TRUE);
      break;

    case GDK_BUTTON_PRESS:
      gtk_grab_add (widget);

      if (!GTK_WIDGET_HAS_FOCUS (widget))
	gtk_widget_grab_focus (widget);

      entry->select_start_pos = gtk_text_entry_position (entry, event->button.x +
							 entry->scroll_offset);
      entry->select_end_pos = entry->select_start_pos;
      entry->current_pos = entry->select_end_pos;
      gtk_text_entry_queue_draw (entry);
      break;

    case GDK_2BUTTON_PRESS:
      gtk_select_word (entry);
      gtk_text_entry_queue_draw (entry);
      break;

    case GDK_3BUTTON_PRESS:
      gtk_select_line (entry);
      gtk_text_entry_queue_draw (entry);
      break;

    case GDK_BUTTON_RELEASE:
      gtk_grab_remove (widget);
      break;

    case GDK_MOTION_NOTIFY:
      entry->select_end_pos = gtk_text_entry_position (entry, event->button.x +
						       entry->scroll_offset);
      entry->current_pos = entry->select_end_pos;
      gtk_text_entry_adjust_scroll (entry);
      gtk_text_entry_queue_draw (entry);
      break;

    case GDK_KEY_PRESS:
      return_val = gtk_text_entry_handle_key (entry, event);
      if (return_val)
	gtk_text_entry_queue_draw (entry);
      break;

    case GDK_ENTER_NOTIFY:
      break;
    case GDK_LEAVE_NOTIFY:
      break;

    default:
      break;
    }

  g_function_leave ("gtk_text_entry_event");
  return return_val;
}

static void
gtk_text_entry_size_request (GtkWidget      *widget,
			     GtkRequisition *requisition)
{
  GtkEntry *entry;
  gint shadow_thickness;

  g_function_enter ("gtk_text_entry_size_request");

  g_assert (widget != NULL);
  g_assert (requisition != NULL);

  entry = (GtkEntry*) widget;
  shadow_thickness = widget->style->shadow_thickness;

  requisition->width = MIN_ENTRY_WIDTH + shadow_thickness * 2 + 2;
  requisition->height = (widget->style->font->ascent +
			 widget->style->font->descent +
			 shadow_thickness * 2 + 4 + 2);

  g_function_leave ("gtk_text_entry_size_request");
}

static void
gtk_text_entry_size_allocate (GtkWidget     *widget,
			      GtkAllocation *allocation)
{
  GtkEntry *entry;

  g_function_enter ("gtk_text_entry_size_allocate");

  g_assert (widget != NULL);
  g_assert (allocation != NULL);

  entry = (GtkEntry*) widget;

  widget->allocation = *allocation;
  if (GTK_WIDGET_REALIZED (widget))
    {
      gdk_window_move (widget->window,
		       allocation->x,
		       allocation->y + (allocation->height - widget->requisition.height) / 2);
      gdk_window_set_size (widget->window,
			   allocation->width,
			   widget->requisition.height);

      gdk_window_move (entry->text_area,
		       widget->style->shadow_thickness + 2,
		       widget->style->shadow_thickness + 2);
      gdk_window_set_size (entry->text_area,
			   widget->window->width - (widget->style->shadow_thickness + 2) * 2,
			   widget->window->height - (widget->style->shadow_thickness + 2) * 2);

      entry->scroll_offset = 0;
      gtk_text_entry_adjust_scroll (entry);
    }

  g_function_leave ("gtk_text_entry_size_allocate");
}

static gint
gtk_text_entry_is_child (GtkWidget *widget,
			 GtkWidget *child)
{
  g_function_enter ("gtk_text_entry_is_child");

  g_assert (widget != NULL);
  g_assert (child != NULL);

  g_function_leave ("gtk_text_entry_is_child");
  return FALSE;
}

static gint
gtk_text_entry_locate (GtkWidget  *widget,
		       GtkWidget **child,
		       gint        x,
		       gint        y)
{
  g_function_enter ("gtk_text_entry_locate");
  g_warning ("gtk_text_entry_locate: UNFINISHED");
  g_function_leave ("gtk_text_entry_locate");
  return FALSE;
}

static void
gtk_text_entry_set_state (GtkWidget    *widget,
			  GtkStateType  state)
{
  g_function_enter ("gtk_text_entry_set_state");
  g_warning ("gtk_text_entry_set_state: UNFINISHED");
  g_function_leave ("gtk_text_entry_set_state");
}


static void
gtk_text_entry_queue_draw (GtkEntry *entry)
{
  g_function_enter ("gtk_text_entry_queue_draw");

  g_assert (entry != NULL);

  if (!entry->timer)
    entry->timer = gtk_timeout_add (50, gtk_text_entry_timer, (gpointer) entry);

  g_function_leave ("gtk_text_entry_queue_draw");
}

static gint
gtk_text_entry_timer (gpointer data)
{
  GtkEntry *entry;

  g_function_enter ("gtk_text_entry_queue_draw");

  entry = (GtkEntry*) data;
  g_assert (entry != NULL);

  gtk_text_entry_draw_text ((GtkWidget*) entry);
  entry->timer = 0;

  g_function_leave ("gtk_text_entry_queue_draw");
  return FALSE;
}

static gint
gtk_text_entry_position (GtkEntry *entry,
			 gint      x)
{
  gint return_val;
  gint sum;
  gint i;

  g_function_enter ("gtk_text_entry_position");

  g_assert (entry != NULL);
  return_val = x;

  i = 0;
  sum = 0;
  if (x > sum)
    for (; i < entry->text_length; i++)
      {
	sum += gdk_char_width (entry->widget.style->font, entry->text[i]);
	if (x < sum)
	  break;
      }
  return_val = i;

  g_function_leave ("gtk_text_entry_position");
  return return_val;
}

static void
gtk_text_entry_adjust_scroll (GtkEntry *entry)
{
  gint xoffset;

  g_function_enter ("gtk_text_entry_adjust_scroll");

  g_assert (entry != NULL);

  if (entry->current_pos > 0)
    xoffset = gdk_text_width (entry->widget.style->font, entry->text, entry->current_pos);
  else
    xoffset = 0;
  xoffset -= entry->scroll_offset;

  if (xoffset < 0)
    entry->scroll_offset += xoffset;
  else if (xoffset > entry->text_area->width)
    entry->scroll_offset += (xoffset - entry->text_area->width) + 1;

  g_function_leave ("gtk_text_entry_adjust_scroll");
}

static gint
gtk_text_entry_handle_key (GtkEntry *entry,
			   GdkEvent *event)
{
  gchar key;
  gint return_val;

  g_function_enter ("gtk_text_entry_handle_key");

  g_assert (event != NULL);
  g_assert (event->type == GDK_KEY_PRESS);

  return_val = FALSE;

  if (entry->key_function)
    if ((* entry->key_function) (event->key.keyval, event->key.state, entry->key_function_data))
      return_val = TRUE;

  if (!return_val)
    {
      switch (event->key.keyval)
	{
	case XK_BackSpace:
	  return_val = TRUE;
	  if (entry->select_start_pos != entry->select_end_pos)
	    gtk_delete_selection (entry);
	  else
	    gtk_delete_backward_character (entry);
	  break;
	case XK_Clear:
	  return_val = TRUE;
	  gtk_delete_line (entry);
	  break;
	case XK_Delete:
	  return_val = TRUE;
	  gtk_delete_forward_character (entry);
	  break;
	case XK_Tab:
	  break;
	case XK_Up:
	  break;
	case XK_Down:
	  break;
	case XK_Left:
	  return_val = TRUE;
	  gtk_move_backward_character (entry);
	  break;
	case XK_Right:
	  return_val = TRUE;
	  gtk_move_forward_character (entry);
	  break;
	default:
	  if ((event->key.keyval >= 0x20) && (event->key.keyval <= 0x7e))
	    {
	      return_val = TRUE;
	      key = (gchar) event->key.keyval;

	      if (event->key.state & GDK_CONTROL_MASK)
		{
		  if ((key >= 'A') && (key <= 'Z'))
		    key -= 'A' - 'a';

		  if ((key >= 'a') && (key <= 'z') && control_keys[(int) (key - 'a')])
		    (* control_keys[(int) (key - 'a')]) (entry);
		}
	      else if (event->key.state & GDK_MOD1_MASK)
		{
		  g_message ("alt key");

		  if ((key >= 'A') && (key <= 'Z'))
		    key -= 'A' - 'a';

		  if ((key >= 'a') && (key <= 'z') && alt_keys[(int) (key - 'a')])
		    (* alt_keys[(int) (key - 'a')]) (entry);
		}
	      else
		{
		  gtk_delete_selection (entry);
		  gtk_text_entry_insert_text (entry, &key, 1);
		}
	    }
	  break;
	}
    }

  if (return_val)
    gtk_text_entry_adjust_scroll (entry);

  g_function_leave ("gtk_text_entry_handle_key");
  return return_val;
}

static void
gtk_text_entry_grow_text (GtkEntry *entry)
{
  gint previous_size;
  gint i;

  g_function_enter ("gtk_text_entry_grow_text");

  g_assert (entry != NULL);

  previous_size = entry->text_size;
  if (!entry->text_size)
    entry->text_size = 128;
  else
    entry->text_size *= 2;
  entry->text = g_realloc (entry->text, entry->text_size);

  for (i = previous_size; i < entry->text_size; i++)
    entry->text[i] = '\0';

  g_function_leave ("gtk_text_entry_grow_text");
}

static void
gtk_text_entry_insert_text (GtkEntry *entry,
			    gchar    *new_text,
			    gint      new_text_length)
{
  gchar *text;
  gint start_pos;
  gint end_pos;
  gint last_pos;
  gint i;

  g_function_enter ("gtk_text_entry_insert_text");

  g_assert (entry != NULL);

  start_pos = entry->current_pos;
  end_pos = start_pos + new_text_length;
  last_pos = new_text_length + entry->text_length;

  while (last_pos >= entry->text_size)
    gtk_text_entry_grow_text (entry);

  text = entry->text;
  for (i = last_pos - 1; i >= end_pos; i--)
    text[i] = text[i- (end_pos - start_pos)];
  for (i = start_pos; i < end_pos; i++)
    text[i] = new_text[i - start_pos];

  entry->text_length += new_text_length;
  entry->current_pos = end_pos;

  g_function_leave ("gtk_text_entry_insert_text");
}

static void
gtk_text_entry_delete_text (GtkEntry *entry,
			    gint      start_pos,
			    gint      end_pos)
{
  gchar *text;
  gint deletion_length;
  gint i;

  g_function_enter ("gtk_text_entry_delete_text");

  g_assert (entry != NULL);

  if ((start_pos < end_pos) &&
      (start_pos >= 0) &&
      (end_pos <= entry->text_length))
    {
      text = entry->text;
      deletion_length = end_pos - start_pos;

      for (i = end_pos; i < entry->text_length; i++)
	text[i - deletion_length] = text[i];

      for (i = entry->text_length - deletion_length; i < entry->text_length; i++)
	text[i] = '\0';

      entry->text_length -= deletion_length;
      entry->current_pos = start_pos;
    }

  g_function_leave ("gtk_text_entry_delete_text");
}

static void
gtk_move_forward_character (GtkEntry *entry)
{
  g_function_enter ("gtk_move_forward_character");

  g_assert (entry != NULL);

  entry->current_pos += 1;
  if (entry->current_pos > entry->text_length)
    entry->current_pos = entry->text_length;

  entry->select_start_pos = 0;
  entry->select_end_pos = 0;

  g_function_leave ("gtk_move_forward_character");
}

static void
gtk_move_backward_character (GtkEntry *entry)
{
  g_function_enter ("gtk_move_backward_character");

  g_assert (entry != NULL);

  entry->current_pos -= 1;
  if (entry->current_pos < 0)
    entry->current_pos = 0;

  entry->select_start_pos = 0;
  entry->select_end_pos = 0;

  g_function_leave ("gtk_move_backward_character");
}

static void
gtk_move_forward_word (GtkEntry *entry)
{
  gchar *text;
  gint i;

  g_function_enter ("gtk_move_forward_word");

  g_assert (entry != NULL);

  if (entry->text)
    {
      text = entry->text;
      i = entry->current_pos;

      if (!((text[i] == '_') || isalnum (text[i])))
	for (; i < entry->text_length; i++)
	  if ((text[i] == '_') || isalnum (text[i]))
	    break;

      for (; i < entry->text_length; i++)
	if (!((text[i] == '_') || isalnum (text[i])))
	  {
	    i -= 1;
	    break;
	  }

      entry->current_pos = i;
    }

  entry->select_start_pos = 0;
  entry->select_end_pos = 0;

  g_function_leave ("gtk_move_forward_word");
}

static void
gtk_move_backward_word (GtkEntry *entry)
{
  gchar *text;
  gint i;

  g_function_enter ("gtk_move_backward_word");

  g_assert (entry != NULL);

  if (entry->text)
    {
      text = entry->text;
      i = entry->current_pos - 1;

      if (!((text[i] == '_') || isalnum (text[i])))
	for (; i >= 0; i--)
	  if ((text[i] == '_') || isalnum (text[i]))
	    break;

      for (; i >= 0; i--)
	if (!((text[i] == '_') || isalnum (text[i])))
	  {
	    i += 1;
	    break;
	  }

      entry->current_pos = i;
      if (entry->current_pos < 0)
	entry->current_pos = 0;

      if (text[entry->current_pos] == ' ')
	entry->current_pos += 1;
    }

  entry->select_start_pos = 0;
  entry->select_end_pos = 0;

  g_function_leave ("gtk_move_backward_word");
}

static void
gtk_move_beginning_of_line (GtkEntry *entry)
{
  g_function_enter ("gtk_move_beginning_of_line");

  g_assert (entry != NULL);

  entry->current_pos = 0;

  entry->select_start_pos = 0;
  entry->select_end_pos = 0;

  g_function_leave ("gtk_move_beginning_of_line");
}

static void
gtk_move_end_of_line (GtkEntry *entry)
{
  g_function_enter ("gtk_move_end_of_line");

  g_assert (entry != NULL);

  entry->current_pos = entry->text_length;

  entry->select_start_pos = 0;
  entry->select_end_pos = 0;

  g_function_leave ("gtk_move_end_of_line");
}

static void
gtk_delete_forward_character (GtkEntry *entry)
{
  gint old_pos;

  g_function_enter ("gtk_delete_forward_character");

  g_assert (entry != NULL);

  old_pos = entry->current_pos;
  gtk_move_forward_character (entry);
  gtk_text_entry_delete_text (entry, old_pos, entry->current_pos);

  entry->select_start_pos = 0;
  entry->select_end_pos = 0;

  g_function_leave ("gtk_delete_forward_character");
}

static void
gtk_delete_backward_character (GtkEntry *entry)
{
  gint old_pos;

  g_function_enter ("gtk_delete_backward_character");

  g_assert (entry != NULL);

  old_pos = entry->current_pos;
  gtk_move_backward_character (entry);
  gtk_text_entry_delete_text (entry, entry->current_pos, old_pos);

  entry->select_start_pos = 0;
  entry->select_end_pos = 0;

  g_function_leave ("gtk_delete_backward_character");
}

static void
gtk_delete_forward_word (GtkEntry *entry)
{
  gint old_pos;

  g_function_enter ("gtk_delete_forward_word");

  g_assert (entry != NULL);

  old_pos = entry->current_pos;
  gtk_move_forward_word (entry);
  gtk_text_entry_delete_text (entry, old_pos, entry->current_pos);

  entry->select_start_pos = 0;
  entry->select_end_pos = 0;

  g_function_leave ("gtk_delete_forward_word");
}

static void
gtk_delete_backward_word (GtkEntry *entry)
{
  gint old_pos;

  g_function_enter ("gtk_delete_backward_word");

  g_assert (entry != NULL);

  old_pos = entry->current_pos;
  gtk_move_backward_word (entry);
  gtk_text_entry_delete_text (entry, entry->current_pos, old_pos);

  entry->select_start_pos = 0;
  entry->select_end_pos = 0;

  g_function_leave ("gtk_delete_backward_word");
}

static void
gtk_delete_line (GtkEntry *entry)
{
  g_function_enter ("gtk_delete_line");

  g_assert (entry != NULL);

  gtk_text_entry_delete_text (entry, 0, entry->text_length);

  entry->select_start_pos = 0;
  entry->select_end_pos = 0;

  g_function_leave ("gtk_delete_line");
}

static void
gtk_delete_to_line_end (GtkEntry *entry)
{
  g_function_enter ("gtk_delete_to_line_end");

  g_assert (entry != NULL);

  gtk_text_entry_delete_text (entry, entry->current_pos, entry->text_length);

  entry->select_start_pos = 0;
  entry->select_end_pos = 0;

  g_function_leave ("gtk_delete_to_line_end");
}

static void
gtk_delete_selection (GtkEntry *entry)
{
  g_function_enter ("gtk_delete_selection");

  g_assert (entry != NULL);

  if (entry->select_start_pos != entry->select_end_pos)
    gtk_text_entry_delete_text (entry,
				MIN (entry->select_start_pos, entry->select_end_pos),
				MAX (entry->select_start_pos, entry->select_end_pos));

  entry->select_start_pos = 0;
  entry->select_end_pos = 0;

  g_function_leave ("gtk_delete_selection");
}

static void
gtk_select_word (GtkEntry *entry)
{
  gint start_pos;
  gint end_pos;

  g_function_enter ("gtk_select_word");

  g_assert (entry != NULL);

  gtk_move_backward_word (entry);
  start_pos = entry->current_pos;

  gtk_move_forward_word (entry);
  end_pos = entry->current_pos;

  entry->select_start_pos = start_pos;
  entry->select_end_pos = end_pos;

  g_function_leave ("gtk_select_word");
}

static void
gtk_select_line (GtkEntry *entry)
{
  g_function_enter ("gtk_select_line");

  g_assert (entry != NULL);

  entry->select_start_pos = 0;
  entry->select_end_pos = entry->text_length;
  entry->current_pos = entry->select_end_pos;

  g_function_leave ("gtk_select_line");
}
