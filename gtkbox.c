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
#include "gtkbox.h"
#include "gtkcontainer.h"
#include "gtkwidget.h"
#include "gtkprivate.h"


#define DEFAULT_EXPAND  FALSE
#define DEFAULT_FILL    FALSE
#define DEFAULT_PACK    GTK_PACK_START
#define DEFAULT_PADDING 5


typedef struct _GtkBox       GtkBox;
typedef struct _GtkBoxChild  GtkBoxChild;

struct _GtkBox
{
  GtkContainer container;
  GList *children;
  gint16 spacing;
  unsigned int homogeneous : 1;
};

struct _GtkBoxChild
{
  GtkWidget *widget;
  gint16 padding;
  unsigned int expand : 1;
  unsigned int fill : 1;
  unsigned int pack : 1;
};


static void   gtk_box_destroy            (GtkWidget         *widget);
static void   gtk_box_map                (GtkWidget         *widget);
static void   gtk_box_unmap              (GtkWidget         *widget);
static void   gtk_box_realize            (GtkWidget         *widget);
static void   gtk_box_draw               (GtkWidget         *widget,
					  GdkRectangle      *area,
					  gint               is_expose);
static gint   gtk_box_is_child           (GtkWidget         *widget,
					  GtkWidget         *child);
static gint   gtk_box_locate             (GtkWidget         *widget,
					  GtkWidget        **child,
					  gint               x,
					  gint               y);
static void   gtk_box_set_state          (GtkWidget         *widget,
					  GtkStateType       state);
static void   gtk_box_add                (GtkContainer      *container,
					  GtkWidget         *widget);
static void   gtk_box_remove             (GtkContainer      *container,
					  GtkWidget         *widget);
static void   gtk_box_focus_advance      (GtkContainer      *container,
					  GtkWidget        **widget,
					  GtkDirectionType   direction);
static void  gtk_box_foreach             (GtkContainer   *container,
					  GtkCallback     callback,
					  gpointer        callback_data);

static void   gtk_hbox_size_request      (GtkWidget      *widget,
					  GtkRequisition *requisition);
static void   gtk_hbox_size_allocate     (GtkWidget      *widget,
					  GtkAllocation  *allocation);

static void   gtk_vbox_size_request      (GtkWidget      *widget,
					  GtkRequisition *requisition);
static void   gtk_vbox_size_allocate     (GtkWidget      *widget,
					  GtkAllocation  *requisition);


static GtkWidgetFunctions hbox_widget_functions =
{
  gtk_box_destroy,
  gtk_widget_default_show,
  gtk_widget_default_hide,
  gtk_box_map,
  gtk_box_unmap,
  gtk_box_realize,
  gtk_box_draw,
  gtk_widget_default_draw_focus,
  gtk_widget_default_event,
  gtk_hbox_size_request,
  gtk_hbox_size_allocate,
  gtk_box_is_child,
  gtk_box_locate,
  gtk_widget_default_activate,
  gtk_box_set_state,
  gtk_widget_default_install_accelerator,
  gtk_widget_default_remove_accelerator,
};

static GtkWidgetFunctions vbox_widget_functions =
{
  gtk_box_destroy,
  gtk_widget_default_show,
  gtk_widget_default_hide,
  gtk_box_map,
  gtk_box_unmap,
  gtk_box_realize,
  gtk_box_draw,
  gtk_widget_default_draw_focus,
  gtk_widget_default_event,
  gtk_vbox_size_request,
  gtk_vbox_size_allocate,
  gtk_box_is_child,
  gtk_box_locate,
  gtk_widget_default_activate,
  gtk_box_set_state,
  gtk_widget_default_install_accelerator,
  gtk_widget_default_remove_accelerator,
};

static GtkContainerFunctions box_container_functions =
{
  gtk_box_add,
  gtk_box_remove,
  gtk_container_default_need_resize,
  gtk_box_focus_advance,
  gtk_box_foreach,
};


GtkWidget*
gtk_hbox_new (gint homogeneous,
	      gint spacing)
{
  GtkBox *hbox;

  g_function_enter ("gtk_hbox_new");

  hbox = g_new (GtkBox, 1);

  hbox->container.widget.type = gtk_get_box_type ();
  hbox->container.widget.function_table = &hbox_widget_functions;
  hbox->container.function_table = &box_container_functions;

  gtk_widget_set_defaults ((GtkWidget*) hbox);
  gtk_container_set_defaults ((GtkWidget*) hbox);

  GTK_WIDGET_SET_FLAGS (hbox, GTK_NO_WINDOW);
  hbox->container.border_width = 10;

  hbox->children = NULL;
  hbox->spacing = spacing;
  hbox->homogeneous = (homogeneous) ? (TRUE) : (FALSE);

  g_function_leave ("gtk_hbox_new");
  return ((GtkWidget*) hbox);
}

GtkWidget*
gtk_vbox_new (gint homogeneous,
	      gint spacing)
{
  GtkBox *vbox;

  g_function_enter ("gtk_vbox_new");

  vbox = g_new (GtkBox, 1);

  vbox->container.widget.type = gtk_get_box_type ();
  vbox->container.widget.function_table = &vbox_widget_functions;
  vbox->container.function_table = &box_container_functions;

  gtk_widget_set_defaults ((GtkWidget*) vbox);
  gtk_container_set_defaults ((GtkWidget*) vbox);

  GTK_WIDGET_SET_FLAGS (vbox, GTK_NO_WINDOW);
  vbox->container.border_width = 10;

  vbox->children = NULL;
  vbox->spacing = spacing;
  vbox->homogeneous = (homogeneous) ? (TRUE) : (FALSE);

  g_function_leave ("gtk_vbox_new");
  return ((GtkWidget*) vbox);
}

void
gtk_box_pack (GtkWidget   *box,
	      GtkWidget   *child,
	      gint         expand,
	      gint         fill,
	      gint         padding,
	      GtkPackType  pack)
{
  GtkBox *rbox;
  GtkBoxChild *child_info;

  g_function_enter ("gtk_box_pack");

  g_assert (box != NULL);
  g_assert (child != NULL);

  rbox = (GtkBox*) box;

  child_info = g_new (GtkBoxChild, 1);
  child_info->widget = child;
  child_info->padding = padding;
  child_info->expand = (expand) ? (TRUE) : (FALSE);
  child_info->fill = (fill) ? (TRUE) : (FALSE);
  child_info->pack = pack;

  rbox->children = g_list_append (rbox->children, child_info);

  child->parent = (GtkContainer*) box;

  if (GTK_WIDGET_VISIBLE (child) && GTK_WIDGET_VISIBLE (box))
    gtk_container_need_resize ((GtkContainer*) box, child);

  g_function_leave ("gtk_box_pack");
}

guint16
gtk_get_box_type ()
{
  static guint16 box_type = 0;

  g_function_enter ("gtk_get_hbox_type");

  if (!box_type)
    gtk_widget_unique_type (&box_type);

  g_function_leave ("gtk_get_hbox_type");
  return box_type;
}


static void
gtk_box_destroy (GtkWidget *widget)
{
  GtkBox *box;
  GtkBoxChild *child;
  GList *children;

  g_function_enter ("gtk_box_destroy");

  g_assert (widget != NULL);
  box = (GtkBox*) widget;

  children = box->children;
  while (children)
    {
      child = children->data;
      children = children->next;

      if (!gtk_widget_destroy (child->widget))
	child->widget->parent = NULL;
      g_free (child);
    }

  g_list_free (box->children);
  g_free (box);

  g_function_leave ("gtk_box_destroy");
}

static void
gtk_box_map (GtkWidget *widget)
{
  GtkBox *box;
  GtkBoxChild *child;
  GList *children;

  g_function_enter ("gtk_box_map");

  g_assert (widget != NULL);

  box = (GtkBox*) widget;
  GTK_WIDGET_SET_FLAGS (box, GTK_MAPPED);

  children = box->children;
  while (children)
    {
      child = children->data;
      children = children->next;

      if (GTK_WIDGET_VISIBLE (child->widget) &&
	  !GTK_WIDGET_MAPPED (child->widget))
	gtk_widget_map (child->widget);
    }

  g_function_leave ("gtk_box_map");
}

static void
gtk_box_unmap (GtkWidget *widget)
{
  GtkBox *box;
  GtkBoxChild *child;
  GList *children;

  g_function_enter ("gtk_box_unmap");

  g_assert (widget != NULL);

  box = (GtkBox*) widget;
  GTK_WIDGET_UNSET_FLAGS (box, GTK_MAPPED);

  children = box->children;
  while (children)
    {
      child = children->data;
      children = children->next;

      if (GTK_WIDGET_VISIBLE (child->widget) &&
	  GTK_WIDGET_MAPPED (child->widget))
	gtk_widget_unmap (child->widget);
    }

  g_function_leave ("gtk_box_unmap");
}

static void
gtk_box_realize (GtkWidget *widget)
{
  g_function_enter ("gtk_box_realize");

  g_assert (widget != NULL);

  GTK_WIDGET_SET_FLAGS (widget, GTK_REALIZED);
  widget->window = widget->parent->widget.window;

  g_function_leave ("gtk_box_realize");
}

static void
gtk_box_draw (GtkWidget    *widget,
	      GdkRectangle *area,
	      gint          is_expose)
{
  GtkBox *box;
  GtkBoxChild *child;
  GList *children;
  GdkRectangle child_area;

  g_function_enter ("gtk_box_draw");

  g_assert (widget != NULL);
  g_assert (area != NULL);

  if (GTK_WIDGET_VISIBLE (widget) && GTK_WIDGET_MAPPED (widget))
    {
      box = (GtkBox*) widget;
      children = box->children;

      if (is_expose)
	{
	  while (children)
	    {
	      child = children->data;
	      children = children->next;

	      if (GTK_WIDGET_NO_WINDOW (child->widget))
		if (gtk_widget_intersect (child->widget, area, &child_area))
		  gtk_widget_draw (child->widget, &child_area, is_expose);
	    }
	}
      else
	{
	  while (children)
	    {
	      child = children->data;
	      children = children->next;

	      if (gtk_widget_intersect (child->widget, area, &child_area))
                gtk_widget_draw (child->widget, &child_area, is_expose);
	    }
	}
    }

  g_function_leave ("gtk_box_draw");
}

static gint
gtk_box_is_child (GtkWidget *widget,
		  GtkWidget *child)
{
  GtkBox *box;
  GtkBoxChild *box_child;
  GList *children;
  gint return_val;

  g_function_enter ("gtk_box_is_child");

  g_assert (widget != NULL);
  g_assert (child != NULL);

  box = (GtkBox*) widget;
  return_val = FALSE;

  children = box->children;
  while (children)
    {
      box_child = children->data;
      children = children->next;

      if (box_child->widget == child)
	{
	  return_val = TRUE;
	  break;
	}
    }

  if (!return_val)
    {
      children = box->children;
      while (children)
	{
	  box_child = children->data;
	  children = children->next;

	  if (gtk_widget_is_child (box_child->widget, child))
	    {
	      return_val = TRUE;
	      break;
	    }
	}
    }

  g_function_leave ("gtk_box_is_child");
  return return_val;
}

static gint
gtk_box_locate (GtkWidget  *widget,
		GtkWidget **child,
		gint        x,
		gint        y)
{
  g_function_enter ("gtk_box_locate");
  g_warning ("gtk_box_locate: UNFINISHED");
  g_function_leave ("gtk_box_locate");
  return FALSE;
}

static void
gtk_box_set_state (GtkWidget    *widget,
		   GtkStateType  state)
{
  GtkBox *box;
  GtkBoxChild *child;
  GList *children;

  g_function_enter ("gtk_box_set_state");

  g_assert (widget != NULL);
  box = (GtkBox*) widget;

  children = box->children;
  while (children)
    {
      child = children->data;
      children = children->next;

      gtk_widget_set_state (child->widget, state);
    }

  g_function_leave ("gtk_box_set_state");
}

static void
gtk_box_add (GtkContainer *container,
	     GtkWidget    *widget)
{
  g_function_enter ("gtk_box_add");

  g_assert (container != NULL);
  g_assert (widget != NULL);

  gtk_box_pack ((GtkWidget*) container,
		widget,
		DEFAULT_EXPAND,
		DEFAULT_FILL,
		DEFAULT_PADDING,
		DEFAULT_PACK);

  g_function_leave ("gtk_box_add");
}

static void
gtk_box_remove (GtkContainer *container,
		GtkWidget    *widget)
{
  GtkBox *box;
  GtkBoxChild *child;
  GList *children;

  g_function_enter ("gtk_box_remove");

  g_assert (container != NULL);
  g_assert (widget != NULL);

  box = (GtkBox*) container;
  children = box->children;

  while (children)
    {
      child = children->data;
      children = children->next;

      if (child->widget == widget)
	{
	  box->children = g_list_remove (box->children, child);
	  g_free (child);
	  break;
	}
    }

  if (GTK_WIDGET_VISIBLE (widget) && GTK_WIDGET_VISIBLE (container))
    gtk_container_need_resize (container, widget);

  g_function_leave ("gtk_box_remove");
}

static void
gtk_box_focus_advance (GtkContainer      *container,
		       GtkWidget        **widget,
		       GtkDirectionType   direction)
{
  g_function_enter ("gtk_box_focus_advance");

  g_assert (container != NULL);
  g_assert (widget != NULL);

  g_function_leave ("gtk_box_focus_advance");
}

static void
gtk_box_foreach (GtkContainer *container,
		 GtkCallback   callback,
		 gpointer      callback_data)
{
  GtkBox *box;
  GtkBoxChild *child;
  GList *children;

  g_function_enter ("gtk_box_foreach");

  g_assert (container != NULL);
  g_assert (callback != NULL);

  box = (GtkBox*) container;
  children = box->children;

  while (children)
    {
      child = children->data;
      children = children->next;

      (* callback) (child->widget, callback_data, NULL);
    }

  g_function_leave ("gtk_box_foreach");
}

static void
gtk_hbox_size_request (GtkWidget      *widget,
		       GtkRequisition *requisition)
{
  GtkBox *box;
  GtkBoxChild *child;
  GList *children;
  gint nvis_children;
  gint width;

  g_function_enter ("gtk_hbox_size_request");

  g_assert (widget != NULL);
  g_assert (requisition != NULL);

  requisition->width = 0;
  requisition->height = 0;

  if (GTK_WIDGET_VISIBLE (widget))
    {
      box = (GtkBox*) widget;
      children = box->children;
      nvis_children = 0;

      while (children)
	{
	  child = children->data;
	  children = children->next;

	  if (GTK_WIDGET_VISIBLE (child->widget))
	    {
	      child->widget->requisition.width = 0;
	      child->widget->requisition.height = 0;

	      gtk_widget_size_request (child->widget, &child->widget->requisition);

	      if (box->homogeneous)
		{
		  width = child->widget->requisition.width + child->padding * 2;
		  requisition->width = MAX (requisition->width, width);
		  requisition->height = MAX (requisition->height, child->widget->requisition.height);
		}
	      else
		{
		  requisition->width += child->widget->requisition.width + child->padding * 2;
		  requisition->height = MAX (requisition->height, child->widget->requisition.height);
		}

	      nvis_children += 1;
	    }
	}

      if (box->homogeneous)
	requisition->width *= nvis_children;
      requisition->width += (nvis_children - 1) * box->spacing;

      requisition->width += box->container.border_width * 2;
      requisition->height += box->container.border_width * 2;
    }

  g_function_leave ("gtk_hbox_size_request");
}

static void
gtk_hbox_size_allocate (GtkWidget     *widget,
			GtkAllocation *allocation)
{
  GtkBox *box;
  GtkBoxChild *child;
  GList *children;
  GtkAllocation child_allocation;
  gint nvis_children;
  gint nexpand_children;
  gint child_width;
  gint width;
  gint extra;
  gint x;

  g_function_enter ("gtk_hbox_size_allocate");

  g_assert (widget != NULL);
  g_assert (allocation != NULL);

  box = (GtkBox*) widget;
  widget->allocation = *allocation;

  children = box->children;
  nvis_children = 0;
  nexpand_children = 0;

  while (children)
    {
      child = children->data;
      children = children->next;

      if (GTK_WIDGET_VISIBLE (child->widget))
	{
	  nvis_children += 1;
	  if (child->expand)
	    nexpand_children += 1;
	}
    }

  if (nvis_children > 0)
    {
      if (box->homogeneous)
	{
	  width = allocation->width - box->container.border_width * 2 - (nvis_children - 1) * box->spacing;
	  extra = width / nvis_children;
	}
      else if (nexpand_children > 0)
	{
	  width = allocation->width - box->container.widget.requisition.width;
	  extra = width / nexpand_children;
	}
      else
	{
	  width = 0;
	  extra = 0;
	}

      x = allocation->x + box->container.border_width;
      child_allocation.y = allocation->y + box->container.border_width;
      child_allocation.height = allocation->height - box->container.border_width * 2;

      children = box->children;
      while (children)
	{
	  child = children->data;
	  children = children->next;

	  if ((child->pack == GTK_PACK_START) && GTK_WIDGET_VISIBLE (child->widget))
	    {
	      if (box->homogeneous)
		{
		  if (nvis_children == 1)
		    child_width = width;
		  else
		    child_width = extra;

		  nvis_children -= 1;
		  width -= extra;
		}
	      else
		{
		  if (child->expand)
		    {
		      child_width = child->widget->requisition.width + child->padding * 2;
		      if (nexpand_children == 1)
			child_width += width;
		      else
			child_width += extra;

		      nexpand_children -= 1;
		      width -= extra;
		    }
		  else
		    {
		      child_width = child->widget->requisition.width + child->padding * 2;
		    }
		}

	      if (child->fill)
		{
		  child_allocation.width = child_width - child->padding * 2;
		  child_allocation.x = x + child->padding;
		}
	      else
		{
		  child_allocation.width = child->widget->requisition.width;
		  child_allocation.x = x + (child_width - child_allocation.width) / 2;
		}

	      gtk_widget_size_allocate (child->widget, &child_allocation);

	      x += child_width + box->spacing;
	    }
	}

      x = allocation->x + allocation->width - box->container.border_width;

      children = box->children;
      while (children)
	{
	  child = children->data;
	  children = children->next;

	  if ((child->pack == GTK_PACK_END) && GTK_WIDGET_VISIBLE (child->widget))
	    {
	      if (box->homogeneous)
		{
		  if (nvis_children == 1)
		    child_width = width;
		  else
		    child_width = extra;

		  nvis_children -= 1;
		  width -= extra;
		}
	      else
		{
		  if (child->expand)
		    {
		      child_width = child->widget->requisition.width + child->padding * 2;
		      if (nexpand_children == 1)
			child_width += width;
		      else
			child_width += extra;

		      nexpand_children -= 1;
		      width -= extra;
		    }
		  else
		    {
		      child_width = child->widget->requisition.width + child->padding * 2;
		    }
		}

	      if (child->fill)
		{
		  child_allocation.width = child_width - child->padding * 2;
		  child_allocation.x = x + child->padding - child_width;
		}
	      else
		{
		  child_allocation.width = child->widget->requisition.width;
		  child_allocation.x = x + (child_width - child_allocation.width) / 2 - child_width;
		}

	      gtk_widget_size_allocate (child->widget, &child_allocation);

	      x -= child_width - box->spacing;
	    }
	}
    }

  g_function_leave ("gtk_hbox_size_allocate");
}

static void
gtk_vbox_size_request (GtkWidget      *widget,
		       GtkRequisition *requisition)
{
  GtkBox *box;
  GtkBoxChild *child;
  GList *children;
  gint nvis_children;
  gint height;

  g_function_enter ("gtk_vbox_size_request");

  g_assert (widget != NULL);
  g_assert (requisition != NULL);

  requisition->width = 0;
  requisition->height = 0;

  if (GTK_WIDGET_VISIBLE (widget))
    {
      box = (GtkBox*) widget;
      children = box->children;
      nvis_children = 0;

      while (children)
	{
	  child = children->data;
	  children = children->next;

	  if (GTK_WIDGET_VISIBLE (child->widget))
	    {
	      child->widget->requisition.width = 0;
	      child->widget->requisition.height = 0;

	      gtk_widget_size_request (child->widget, &child->widget->requisition);

	      if (box->homogeneous)
		{
		  height = child->widget->requisition.height + child->padding * 2;
		  requisition->width = MAX (requisition->width, child->widget->requisition.width);
		  requisition->height = MAX (requisition->height, height);
		}
	      else
		{
		  requisition->width = MAX (requisition->width, child->widget->requisition.width);
		  requisition->height += child->widget->requisition.height + child->padding * 2;
		}

	      nvis_children += 1;
	    }
	}

      if (box->homogeneous)
	requisition->height *= nvis_children;
      requisition->height += (nvis_children - 1) * box->spacing;

      requisition->width += box->container.border_width * 2;
      requisition->height += box->container.border_width * 2;
    }

  g_function_leave ("gtk_vbox_size_request");
}

static void
gtk_vbox_size_allocate (GtkWidget     *widget,
			GtkAllocation *allocation)
{
  GtkBox *box;
  GtkBoxChild *child;
  GList *children;
  GtkAllocation child_allocation;
  gint nvis_children;
  gint nexpand_children;
  gint child_height;
  gint height;
  gint extra;
  gint y;

  g_function_enter ("gtk_vbox_size_allocate");

  g_assert (widget != NULL);
  g_assert (allocation != NULL);

  box = (GtkBox*) widget;
  widget->allocation = *allocation;

  children = box->children;
  nvis_children = 0;
  nexpand_children = 0;

  while (children)
    {
      child = children->data;
      children = children->next;

      if (GTK_WIDGET_VISIBLE (child->widget))
	{
	  nvis_children += 1;
	  if (child->expand)
	    nexpand_children += 1;
	}
    }

  if (nvis_children > 0)
    {
      if (box->homogeneous)
	{
	  height = allocation->height - box->container.border_width * 2 - (nvis_children - 1) * box->spacing;
	  extra = height / nvis_children;
	}
      else if (nexpand_children > 0)
	{
	  height = allocation->height - box->container.widget.requisition.height;
	  extra = height / nexpand_children;
	}
      else
	{
	  height = 0;
	  extra = 0;
	}

      y = allocation->y + box->container.border_width;
      child_allocation.x = allocation->x + box->container.border_width;
      child_allocation.width = allocation->width - box->container.border_width * 2;

      children = box->children;
      while (children)
	{
	  child = children->data;
	  children = children->next;

	  if ((child->pack == GTK_PACK_START) && GTK_WIDGET_VISIBLE (child->widget))
	    {
	      if (box->homogeneous)
		{
		  if (nvis_children == 1)
		    child_height = height;
		  else
		    child_height = extra;

		  nvis_children -= 1;
		  height -= extra;
		}
	      else
		{
		  if (child->expand)
		    {
		      child_height = child->widget->requisition.height + child->padding * 2;
		      if (nexpand_children == 1)
			child_height += height;
		      else
			child_height += extra;

		      nexpand_children -= 1;
		      height -= extra;
		    }
		  else
		    {
		      child_height = child->widget->requisition.height + child->padding * 2;
		    }
		}

	      if (child->fill)
		{
		  child_allocation.height = child_height - child->padding * 2;
		  child_allocation.y = y + child->padding;
		}
	      else
		{
		  child_allocation.height = child->widget->requisition.height;
		  child_allocation.y = y + (child_height - child_allocation.height) / 2;
		}

	      gtk_widget_size_allocate (child->widget, &child_allocation);

	      y += child_height + box->spacing;
	    }
	}

      y = allocation->y + allocation->height - box->container.border_width;

      children = box->children;
      while (children)
	{
	  child = children->data;
	  children = children->next;

	  if ((child->pack == GTK_PACK_END) && GTK_WIDGET_VISIBLE (child->widget))
	    {
	      if (box->homogeneous)
		{
		  if (nvis_children == 1)
		    child_height = height;
		  else
		    child_height = extra;

		  nvis_children -= 1;
		  height -= extra;
		}
	      else
		{
		  if (child->expand)
		    {
		      child_height = child->widget->requisition.height + child->padding * 2;
		      if (nexpand_children == 1)
			child_height += height;
		      else
			child_height += extra;

		      nexpand_children -= 1;
		      height -= extra;
		    }
		  else
		    {
		      child_height = child->widget->requisition.height + child->padding * 2;
		    }
		}

	      if (child->fill)
		{
		  child_allocation.height = child_height - child->padding * 2;
		  child_allocation.y = y + child->padding - child_height;
		}
	      else
		{
		  child_allocation.height = child->widget->requisition.height;
		  child_allocation.y = y + (child_height - child_allocation.height) / 2 - child_height;
		}

	      gtk_widget_size_allocate (child->widget, &child_allocation);

	      y -= child_height - box->spacing;
	    }
	}
    }

  g_function_leave ("gtk_vbox_size_allocate");
}
