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
#include "gtkwidget.h"


void
gtk_container_add (GtkWidget *container,
		   GtkWidget *widget)
{
  GtkContainer *rcontainer;

  g_function_enter ("gtk_container_add");

  g_assert (container != NULL);
  g_assert (widget != NULL);
  rcontainer = (GtkContainer*) container;
  g_assert (rcontainer->function_table != NULL);
  g_assert (rcontainer->function_table->add != NULL);

  widget->parent = rcontainer;
  (* rcontainer->function_table->add) (rcontainer, widget);

  g_function_leave ("gtk_container_add");
}

void
gtk_container_remove (GtkWidget *container,
		      GtkWidget *widget)
{
  GtkContainer *rcontainer;

  g_function_enter ("gtk_container_remove");

  g_assert (container != NULL);
  g_assert (widget != NULL);
  rcontainer = (GtkContainer*) container;
  g_assert (rcontainer->function_table != NULL);
  g_assert (rcontainer->function_table->remove != NULL);

  if (rcontainer->focus_child == widget)
    rcontainer->focus_child = NULL;

  (* rcontainer->function_table->remove) (rcontainer, widget);
  widget->parent = NULL;

  g_function_leave ("gtk_container_remove");
}

void
gtk_container_need_resize (GtkContainer *container,
			   GtkWidget    *widget)
{
  g_function_enter ("gtk_container_need_resize");

  g_assert (container != NULL);
  g_assert (widget != NULL);
  g_assert (container->function_table != NULL);
  g_assert (container->function_table->need_resize != NULL);

  (* container->function_table->need_resize) (container, widget);

  g_function_leave ("gtk_container_need_resize");
}

void
gtk_container_focus_advance (GtkContainer      *container,
			     GtkWidget        **widget,
			     GtkDirectionType   direction)
{
  g_function_enter ("gtk_container_focus_advance");

  g_assert (container != NULL);
  g_assert (widget != NULL);
  g_assert (container->function_table != NULL);
  g_assert (container->function_table->need_resize != NULL);

  (* container->function_table->focus_advance) (container, widget, direction);

  g_function_leave ("gtk_container_focus_advance");
}

void
gtk_container_foreach (GtkContainer *container,
		       GtkCallback   callback,
		       gpointer      callback_data)
{
  g_function_enter ("gtk_container_foreach");

  g_assert (container != NULL);
  g_assert (container->function_table != NULL);
  g_assert (container->function_table->foreach != NULL);

  (* container->function_table->foreach) (container, callback, callback_data);

  g_function_leave ("gtk_container_foreach");
}

void
gtk_container_set_defaults (GtkWidget *widget)
{
  GtkContainer *container;

  g_function_enter ("gtk_container_set_defaults");

  g_assert (widget != NULL);
  container = (GtkContainer*) widget;

  GTK_WIDGET_SET_FLAGS (widget, GTK_CONTAINER);
  container->border_width = 0;
  container->focus_child = NULL;

  g_function_leave ("gtk_container_set_defaults");
}

void
gtk_container_set_border_width (GtkWidget *widget,
				gint       border_width)
{
  GtkContainer *container;

  g_function_enter ("gtk_container_set_border_width");

  g_assert (widget != NULL);
  g_assert (GTK_WIDGET_CONTAINER (widget));
  container = (GtkContainer*) widget;

  if (container->border_width != border_width)
    {
      container->border_width = border_width;

      if (GTK_WIDGET_VISIBLE (container))
	gtk_container_need_resize (container->widget.parent, (GtkWidget*) container);
    }

  g_function_leave ("gtk_container_set_border_width");
}

void
gtk_container_default_need_resize (GtkContainer *container,
				   GtkWidget    *widget)
{
  g_function_enter ("gtk_container_default_need_resize");

  g_assert (container != NULL);
  g_assert (widget != NULL);

  if (GTK_WIDGET_VISIBLE (container))
    {
      gtk_container_need_resize (container->widget.parent, (GtkWidget*) container);

      if (GTK_WIDGET_VISIBLE (widget))
	{
	  if (GTK_WIDGET_REALIZED (container) &&
	      !GTK_WIDGET_REALIZED (widget))
	    gtk_widget_realize (widget);

	  if (GTK_WIDGET_MAPPED (container) &&
	      !GTK_WIDGET_MAPPED (widget))
	    gtk_widget_map (widget);
	}
    }

  g_function_leave ("gtk_container_default_need_resize");
}

void
gtk_container_default_focus_advance (GtkContainer      *container,
				     GtkWidget        **child,
				     GtkDirectionType   direction)
{
  g_function_enter ("gtk_container_default_focus_advance");

  g_assert (container != NULL);
  g_assert (child != NULL);

  g_function_leave ("gtk_container_default_focus_advance");
}
