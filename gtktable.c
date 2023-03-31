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
#include "gtktable.h"
#include "gtkcontainer.h"
#include "gtkwidget.h"
#include "gtkprivate.h"


#define DEFAULT_EXPAND  FALSE
#define DEFAULT_FILL    FALSE
#define DEFAULT_PADDING 5


typedef struct _GtkTable       GtkTable;
typedef struct _GtkTableChild  GtkTableChild;
typedef struct _GtkTableRowCol GtkTableRowCol;

struct _GtkTable
{
  GtkContainer container;

  GList *children;
  GtkTableRowCol *rows;
  GtkTableRowCol *cols;
  gint16 nrows;
  gint16 ncols;

  unsigned int homogeneous : 1;
};

struct _GtkTableChild
{
  GtkWidget *widget;
  gint16 left_attach;
  gint16 right_attach;
  gint16 top_attach;
  gint16 bottom_attach;
  gint16 xpadding;
  gint16 ypadding;
  unsigned int xexpand : 1;
  unsigned int yexpand : 1;
  unsigned int xfill : 1;
  unsigned int yfill : 1;
};

struct _GtkTableRowCol
{
  gint16 requisition;
  gint16 allocation;
  gint16 spacing;
  unsigned int need_expand : 1;
  unsigned int expand : 1;
};


static void   gtk_table_destroy       (GtkWidget       *widget);
static void   gtk_table_map           (GtkWidget       *widget);
static void   gtk_table_unmap         (GtkWidget       *widget);
static void   gtk_table_realize       (GtkWidget       *widget);
static void   gtk_table_draw          (GtkWidget       *widget,
				       GdkRectangle    *area,
				       gint             is_expose);
static void   gtk_table_size_request  (GtkWidget       *widget,
				       GtkRequisition  *requisition);
static void   gtk_table_size_allocate (GtkWidget       *widget,
				       GtkAllocation   *allocation);
static gint   gtk_table_is_child      (GtkWidget       *widget,
				       GtkWidget       *child);
static gint   gtk_table_locate        (GtkWidget       *widget,
				       GtkWidget      **child,
				       gint             x,
				       gint             y);
static void   gtk_table_set_state     (GtkWidget       *widget,
				       GtkStateType     state);
static void   gtk_table_add           (GtkContainer    *container,
				       GtkWidget       *widget);
static void   gtk_table_remove        (GtkContainer    *container,
				       GtkWidget       *widget);
static void   gtk_table_foreach       (GtkContainer    *container,
				       GtkCallback      callback,
				       gpointer         callback_data);

static void   gtk_table_size_request_init  (GtkTable *table);
static void   gtk_table_size_request_pass1 (GtkTable *table);
static void   gtk_table_size_request_pass2 (GtkTable *table);
static void   gtk_table_size_request_pass3 (GtkTable *table);

static void   gtk_table_size_allocate_init  (GtkTable *table);
static void   gtk_table_size_allocate_pass1 (GtkTable *table);
static void   gtk_table_size_allocate_pass2 (GtkTable *table);
static void   gtk_table_size_allocate_pass3 (GtkTable *table);


static GtkWidgetFunctions table_widget_functions =
{
  gtk_table_destroy,
  gtk_widget_default_show,
  gtk_widget_default_hide,
  gtk_table_map,
  gtk_table_unmap,
  gtk_table_realize,
  gtk_table_draw,
  gtk_widget_default_draw_focus,
  gtk_widget_default_event,
  gtk_table_size_request,
  gtk_table_size_allocate,
  gtk_table_is_child,
  gtk_table_locate,
  gtk_widget_default_activate,
  gtk_table_set_state,
  gtk_widget_default_install_accelerator,
  gtk_widget_default_remove_accelerator,
};

static GtkContainerFunctions table_container_functions =
{
  gtk_table_add,
  gtk_table_remove,
  gtk_container_default_need_resize,
  gtk_container_default_focus_advance,
  gtk_table_foreach,
};


GtkWidget*
gtk_table_new (gint rows,
	       gint columns,
	       gint homogeneous)
{
  GtkTable *table;
  gint row, col;

  g_function_enter ("gtk_table_new");

  g_assert (rows >= 1);
  g_assert (columns >= 1);

  table = g_new (GtkTable, 1);

  table->container.widget.type = gtk_get_table_type ();
  table->container.widget.function_table = &table_widget_functions;
  table->container.function_table = &table_container_functions;

  gtk_widget_set_defaults ((GtkWidget*) table);
  gtk_container_set_defaults ((GtkWidget*) table);

  GTK_WIDGET_SET_FLAGS (table, GTK_NO_WINDOW);
  table->container.border_width = 10;

  table->children = NULL;

  table->rows = g_new (GtkTableRowCol, rows);
  table->cols = g_new (GtkTableRowCol, columns);

  for (row = 0; row < rows; row++)
    {
      table->rows[row].requisition = 0;
      table->rows[row].allocation = 0;
      table->rows[row].spacing = 0;
      table->rows[row].need_expand = FALSE;
      table->rows[row].expand = FALSE;
    }
  for (col = 0; col < columns; col++)
    {
      table->cols[col].requisition = 0;
      table->cols[col].allocation = 0;
      table->cols[col].spacing = 0;
      table->cols[col].need_expand = FALSE;
      table->cols[col].expand = FALSE;
    }

  table->nrows = rows;
  table->ncols = columns;
  table->homogeneous = (homogeneous) ? (TRUE) : (FALSE);

  g_function_leave ("gtk_table_new");
  return ((GtkWidget*) table);
}

void
gtk_table_attach (GtkWidget *table,
		  GtkWidget *child,
		  gint       left_attach,
		  gint       right_attach,
		  gint       top_attach,
		  gint       bottom_attach,
		  gint       xexpand,
		  gint       xfill,
		  gint       xpadding,
		  gint       yexpand,
		  gint       yfill,
		  gint       ypadding)
{
  GtkTable *rtable;
  GtkTableChild *table_child;

  g_function_enter ("gtk_table_attach");

  g_assert (table != NULL);
  rtable = (GtkTable*) table;

  g_assert ((left_attach >= 0) && (left_attach < rtable->ncols));
  g_assert ((right_attach >= 0) && (right_attach <= rtable->ncols));
  g_assert ((top_attach >= 0) && (top_attach < rtable->nrows));
  g_assert ((bottom_attach >= 0) && (bottom_attach <= rtable->nrows));

  g_assert (left_attach < right_attach);
  g_assert (top_attach < bottom_attach);

  table_child = g_new (GtkTableChild, 1);
  table_child->widget = child;
  table_child->left_attach = left_attach;
  table_child->right_attach = right_attach;
  table_child->top_attach = top_attach;
  table_child->bottom_attach = bottom_attach;
  table_child->xexpand = (xexpand) ? (TRUE) : (FALSE);
  table_child->xfill = (xfill) ? (TRUE) : (FALSE);
  table_child->xpadding = xpadding;
  table_child->yexpand = (yexpand) ? (TRUE) : (FALSE);
  table_child->yfill = (yfill) ? (TRUE) : (FALSE);
  table_child->ypadding = ypadding;

  rtable->children = g_list_prepend (rtable->children, table_child);

  child->parent = (GtkContainer*) table;

  if (GTK_WIDGET_VISIBLE (child) && GTK_WIDGET_VISIBLE (table))
    gtk_container_need_resize ((GtkContainer*) table, child);

  g_function_leave ("gtk_table_attach");
}

void
gtk_table_set_row_spacing (GtkWidget *table,
			   gint       row,
			   gint       spacing)
{
  GtkTable *rtable;

  g_function_leave ("gtk_table_set_row_spacing");

  g_assert (table != NULL);
  rtable = (GtkTable*) table;

  g_assert ((row >= 0) && (row < (rtable->nrows - 1)));

  if (rtable->rows[row].spacing != spacing)
    {
      rtable->rows[row].spacing = spacing;

      if (GTK_WIDGET_VISIBLE (table) && GTK_WIDGET_MAPPED (table))
        gtk_container_need_resize (table->parent, table);
    }

  g_function_leave ("gtk_table_set_row_spacing");
}

void
gtk_table_set_col_spacing (GtkWidget *table,
			   gint       col,
			   gint       spacing)
{
  GtkTable *rtable;

  g_function_leave ("gtk_table_set_col_spacing");

  g_assert (table != NULL);
  rtable = (GtkTable*) table;

  g_assert ((col >= 0) && (col < (rtable->ncols - 1)));

  if (rtable->cols[col].spacing != spacing)
    {
      rtable->cols[col].spacing = spacing;

      if (GTK_WIDGET_VISIBLE (table) && GTK_WIDGET_MAPPED (table))
        gtk_container_need_resize (table->parent, table);
    }

  g_function_leave ("gtk_table_set_col_spacing");
}

guint16
gtk_get_table_type ()
{
  static guint16 table_type = 0;

  g_function_enter ("gtk_get_table_type");

  if (!table_type)
    gtk_widget_unique_type (&table_type);

  g_function_leave ("gtk_get_table_type");
  return table_type;
}


static void
gtk_table_destroy (GtkWidget *widget)
{
  GtkTable *table;
  GtkTableChild *child;
  GList *children;

  g_function_enter ("gtk_table_destroy");

  g_assert (widget != NULL);
  table = (GtkTable*) widget;

  children = table->children;
  while (children)
    {
      child = children->data;
      children = children->next;

      if (!gtk_widget_destroy (child->widget))
	child->widget->parent = NULL;
      g_free (child);
    }

  g_list_free (table->children);
  g_free (table->rows);
  g_free (table->cols);
  g_free (table);

  g_function_leave ("gtk_table_destroy");
}

static void
gtk_table_map (GtkWidget *widget)
{
  GtkTable *table;
  GtkTableChild *child;
  GList *children;

  g_function_enter ("gtk_table_map");

  g_assert (widget != NULL);

  table = (GtkTable*) widget;
  GTK_WIDGET_SET_FLAGS (table, GTK_MAPPED);

  children = table->children;
  while (children)
    {
      child = children->data;
      children = children->next;

      if (GTK_WIDGET_VISIBLE (child->widget) &&
	  !GTK_WIDGET_MAPPED (child->widget))
	gtk_widget_map (child->widget);
    }

  g_function_leave ("gtk_table_map");
}

static void
gtk_table_unmap (GtkWidget *widget)
{
  GtkTable *table;
  GtkTableChild *child;
  GList *children;

  g_function_enter ("gtk_table_unmap");

  g_assert (widget != NULL);

  table = (GtkTable*) widget;
  GTK_WIDGET_UNSET_FLAGS (table, GTK_MAPPED);

  children = table->children;
  while (children)
    {
      child = children->data;
      children = children->next;

      if (GTK_WIDGET_VISIBLE (child->widget) &&
	  GTK_WIDGET_MAPPED (child->widget))
	gtk_widget_unmap (child->widget);
    }

  g_function_leave ("gtk_table_unmap");
}

static void
gtk_table_realize (GtkWidget *widget)
{
  g_function_enter ("gtk_table_realize");

  g_assert (widget != NULL);

  GTK_WIDGET_SET_FLAGS (widget, GTK_REALIZED);
  widget->window = widget->parent->widget.window;

  g_function_leave ("gtk_table_realize");
}

static void
gtk_table_draw (GtkWidget    *widget,
	      GdkRectangle *area,
	      gint          is_expose)
{
  GtkTable *table;
  GtkTableChild *child;
  GList *children;
  GdkRectangle child_area;

  g_function_enter ("gtk_table_draw");

  g_assert (widget != NULL);
  g_assert (area != NULL);

  if (GTK_WIDGET_VISIBLE (widget) && GTK_WIDGET_MAPPED (widget))
    {
      table = (GtkTable*) widget;
      children = table->children;

      if (is_expose)
	{
	  children = table->children;
	  while (children)
	    {
	      child = children->data;
	      children = children->next;

	      if (GTK_WIDGET_NO_WINDOW (child->widget) &&
		  gtk_widget_intersect (child->widget, area, &child_area))
		gtk_widget_draw (child->widget, &child_area, is_expose);
	    }
	}
      else
	{
	  children = table->children;
	  while (children)
	    {
	      child = children->data;
	      children = children->next;

	      if (gtk_widget_intersect (child->widget, area, &child_area))
		gtk_widget_draw (child->widget, &child_area, is_expose);
	    }
	}
    }

  g_function_leave ("gtk_table_draw");
}

static void
gtk_table_size_request (GtkWidget      *widget,
			GtkRequisition *requisition)
{
  GtkTable *table;
  gint row, col;

  g_function_enter ("gtk_table_size_request");

  g_assert (widget != NULL);
  g_assert (requisition != NULL);

  requisition->width = 0;
  requisition->height = 0;

  if (GTK_WIDGET_VISIBLE (widget))
    {
      table = (GtkTable*) widget;

      gtk_table_size_request_init (table);
      gtk_table_size_request_pass1 (table);
      gtk_table_size_request_pass2 (table);
      gtk_table_size_request_pass3 (table);
      gtk_table_size_request_pass2 (table);

      for (col = 0; col < table->ncols; col++)
	requisition->width += table->cols[col].requisition;
      for (col = 0; col < table->ncols - 1; col++)
	requisition->width += table->cols[col].spacing;

      for (row = 0; row < table->nrows; row++)
	requisition->height += table->rows[row].requisition;
      for (row = 0; row < table->nrows - 1; row++)
	requisition->height += table->rows[row].spacing;

      requisition->width += table->container.border_width * 2;
      requisition->height += table->container.border_width * 2;
    }

  g_function_leave ("gtk_table_size_request");
}

static void
gtk_table_size_allocate (GtkWidget     *widget,
			 GtkAllocation *allocation)
{
  GtkTable *table;

  g_function_enter ("gtk_table_size_allocate");

  g_assert (widget != NULL);
  g_assert (allocation != NULL);

  table = (GtkTable*) widget;
  widget->allocation = *allocation;

  gtk_table_size_allocate_init (table);
  gtk_table_size_allocate_pass1 (table);
  gtk_table_size_allocate_pass2 (table);
  gtk_table_size_allocate_pass3 (table);

  g_function_leave ("gtk_table_size_allocate");
}

static gint
gtk_table_is_child (GtkWidget *widget,
		    GtkWidget *child)
{
  GtkTable *table;
  GtkTableChild *tchild;
  GList *children;
  gint return_val;

  g_function_enter ("gtk_table_is_child");

  g_assert (widget != NULL);
  g_assert (child != NULL);

  table = (GtkTable*) widget;
  return_val = FALSE;

  children = table->children;
  while (children)
    {
      tchild = children->data;
      children = children->next;

      if (tchild->widget == child)
	{
	  return_val = TRUE;
	  break;
	}
    }

  if (!return_val)
    {
      children = table->children;
      while (children)
	{
	  tchild = children->data;
	  children = children->next;

	  if (gtk_widget_is_child (tchild->widget, child))
	    {
	      return_val = TRUE;
	      break;
	    }
	}
    }

  g_function_leave ("gtk_table_is_child");
  return return_val;
}

static gint
gtk_table_locate (GtkWidget  *widget,
		  GtkWidget **child,
		  gint        x,
		  gint        y)
{
  g_function_enter ("gtk_table_locate");
  g_warning ("gtk_table_locate: UNFINISHED");
  g_function_leave ("gtk_table_locate");
  return FALSE;
}

static void
gtk_table_set_state (GtkWidget    *widget,
		     GtkStateType  state)
{
  GtkTable *table;
  GtkTableChild *child;
  GList *children;

  g_function_enter ("gtk_table_set_state");

  g_assert (widget != NULL);
  table = (GtkTable*) widget;

  children = table->children;
  while (children)
    {
      child = children->data;
      children = children->next;

      gtk_widget_set_state (child->widget, state);
    }

  g_function_leave ("gtk_table_set_state");
}

static void
gtk_table_add (GtkContainer *container,
	       GtkWidget    *widget)
{
  g_function_enter ("gtk_table_add");

  g_assert (container != NULL);
  g_assert (widget != NULL);

  gtk_table_attach ((GtkWidget*) container,
		    widget,
		    0, 0, 1, 1,
		    DEFAULT_EXPAND,
		    DEFAULT_FILL,
		    DEFAULT_PADDING,
		    DEFAULT_EXPAND,
		    DEFAULT_FILL,
		    DEFAULT_PADDING);

  g_function_leave ("gtk_table_add");
}

static void
gtk_table_remove (GtkContainer *container,
		  GtkWidget    *widget)
{
  GtkTable *table;
  GtkTableChild *child;
  GList *children;

  g_function_enter ("gtk_table_remove");

  g_assert (container != NULL);
  g_assert (widget != NULL);

  table = (GtkTable*) container;
  children = table->children;

  while (children)
    {
      child = children->data;
      children = children->next;

      if (child->widget == widget)
	{
	  table->children = g_list_remove (table->children, child);
	  g_free (child);

	  if (GTK_WIDGET_VISIBLE (widget) && GTK_WIDGET_VISIBLE (container))
	    gtk_container_need_resize (container, widget);
	  break;
	}
    }

  g_function_leave ("gtk_table_remove");
}

static void
gtk_table_foreach (GtkContainer *container,
		   GtkCallback   callback,
		   gpointer      callback_data)
{
  GtkTable *table;
  GtkTableChild *child;
  GList *children;

  g_function_enter ("gtk_table_foreach");

  g_assert (container != NULL);
  g_assert (callback != NULL);

  table = (GtkTable*) container;
  children = table->children;

  while (children)
    {
      child = children->data;
      children = children->next;

      (* callback) (child->widget, callback_data, NULL);
    }

  g_function_leave ("gtk_table_foreach");
}

static void
gtk_table_size_request_init (GtkTable *table)
{
  GtkTableChild *child;
  GList *children;
  gint row, col;

  g_function_enter ("gtk_table_size_request_init");

  g_assert (table != NULL);

  for (row = 0; row < table->nrows; row++)
    table->rows[row].requisition = 0;
  for (col = 0; col < table->ncols; col++)
    table->cols[col].requisition = 0;

  children = table->children;
  while (children)
    {
      child = children->data;
      children = children->next;

      if (GTK_WIDGET_VISIBLE (child->widget))
	{
	  child->widget->requisition.width = 0;
	  child->widget->requisition.height = 0;

	  gtk_widget_size_request (child->widget, &child->widget->requisition);
	}
    }

  g_function_leave ("gtk_table_size_request_init");
}

static void
gtk_table_size_request_pass1 (GtkTable *table)
{
  GtkTableChild *child;
  GList *children;
  gint width;
  gint height;

  g_function_enter ("gtk_table_size_request_pass1");

  g_assert (table != NULL);

  children = table->children;
  while (children)
    {
      child = children->data;
      children = children->next;

      if (GTK_WIDGET_VISIBLE (child->widget))
	{
	  /* Child spans a single column.
	   */
	  if (child->left_attach == (child->right_attach - 1))
	    {
	      width = child->widget->requisition.width + child->xpadding * 2;
	      table->cols[child->left_attach].requisition = MAX (table->cols[child->left_attach].requisition, width);
	    }

	  /* Child spans a single row.
	   */
	  if (child->top_attach == (child->bottom_attach - 1))
	    {
	      height = child->widget->requisition.height + child->ypadding * 2;
	      table->rows[child->top_attach].requisition = MAX (table->rows[child->top_attach].requisition, height);
	    }
	}
    }

  g_function_leave ("gtk_table_size_request_pass1");
}

static void
gtk_table_size_request_pass2 (GtkTable *table)
{
  gint max_width;
  gint max_height;
  gint row, col;

  g_function_enter ("gtk_table_size_request_pass2");

  g_assert (table != NULL);

  if (table->homogeneous)
    {
      max_width = 0;
      max_height = 0;

      for (col = 0; col < table->ncols; col++)
	max_width = MAX (max_width, table->cols[col].requisition);
      for (row = 0; row < table->nrows; row++)
	max_height = MAX (max_height, table->rows[row].requisition);

      for (col = 0; col < table->ncols; col++)
	table->cols[col].requisition = max_width;
      for (row = 0; row < table->nrows; row++)
	table->rows[row].requisition = max_height;
    }

  g_function_leave ("gtk_table_size_request_pass2");
}

static void
gtk_table_size_request_pass3 (GtkTable *table)
{
  GtkTableChild *child;
  GList *children;
  gint width, height;
  gint row, col;
  gint extra;

  g_function_enter ("gtk_table_size_request_pass3");

  g_assert (table != NULL);

  children = table->children;
  while (children)
    {
      child = children->data;
      children = children->next;

      if (GTK_WIDGET_VISIBLE (child->widget))
	{
	  /* Child spans multiple columns.
	   */
	  if (child->left_attach != (child->right_attach - 1))
	    {
	      /* Check and see if there is already enough space
	       *  for the child.
	       */
	      width = 0;
	      for (col = child->left_attach; col < child->right_attach; col++)
		{
		  width += table->cols[col].requisition;
		  if ((col + 1) < child->right_attach)
		    width += table->cols[col].spacing;
		}

	      /* If we need to request more space for this child to fill
	       *  its requisition, then divide up the needed space evenly
	       *  amongst the columns it spans.
	       */
	      if (width < child->widget->requisition.width)
		{
		  width = child->widget->requisition.width - width;
		  extra = width / (child->right_attach - child->left_attach);

		  for (col = child->left_attach; col < child->right_attach; col++)
		    {
		      if ((col + 1) < child->right_attach)
			table->cols[col].requisition += extra;
		      else
			table->cols[col].requisition += width;
		      width -= extra;
		    }
		}
	    }

	  /* Child spans multiple rows.
	   */
	  if (child->top_attach != (child->bottom_attach - 1))
	    {
	      /* Check and see if there is already enough space
	       *  for the child.
	       */
	      height = 0;
	      for (row = child->top_attach; row < child->bottom_attach; row++)
		{
		  height += table->rows[row].requisition;
		  if ((row + 1) < child->bottom_attach)
		    height += table->rows[row].spacing;
		}

	      /* If we need to request more space for this child to fill
	       *  its requisition, then divide up the needed space evenly
	       *  amongst the columns it spans.
	       */
	      if (height < child->widget->requisition.height)
		{
		  height = child->widget->requisition.height - height;
		  extra = height / (child->bottom_attach - child->top_attach);

		  for (row = child->top_attach; row < child->bottom_attach; row++)
		    {
		      if ((row + 1) < child->bottom_attach)
			table->rows[row].requisition += extra;
		      else
			table->rows[row].requisition += height;
		      height -= extra;
		    }
		}
	    }
	}
    }

  g_function_leave ("gtk_table_size_request_pass3");
}

static void
gtk_table_size_allocate_init (GtkTable *table)
{
  GtkTableChild *child;
  GList *children;
  gint row, col;
  gint has_expand;

  g_function_enter ("gtk_table_size_allocate_init");

  g_assert (table != NULL);

  for (col = 0; col < table->ncols; col++)
    {
      table->cols[col].allocation = table->cols[col].requisition;
      table->cols[col].need_expand = FALSE;
      table->cols[col].expand = FALSE;
    }
  for (row = 0; row < table->nrows; row++)
    {
      table->rows[row].allocation = table->rows[row].requisition;
      table->rows[row].need_expand = FALSE;
      table->rows[row].expand = FALSE;
    }

  children = table->children;
  while (children)
    {
      child = children->data;
      children = children->next;

      if (GTK_WIDGET_VISIBLE (child->widget))
	{
	  if (child->xexpand && (child->left_attach == (child->right_attach - 1)))
	    table->cols[child->left_attach].expand = TRUE;

	  if (child->yexpand && (child->top_attach == (child->bottom_attach - 1)))
	    table->rows[child->top_attach].expand = TRUE;
	}
    }

  children = table->children;
  while (children)
    {
      child = children->data;
      children = children->next;

      if (GTK_WIDGET_VISIBLE (child->widget))
	{
	  if (child->xexpand && (child->left_attach != (child->right_attach - 1)))
	    {
	      has_expand = FALSE;
	      for (col = child->left_attach; col < child->right_attach; col++)
		if (table->cols[col].expand)
		  {
		    has_expand = TRUE;
		    break;
		  }

	      if (!has_expand)
		for (col = child->left_attach; col < child->right_attach; col++)
		  table->cols[col].need_expand = TRUE;
	    }

	  if (child->yexpand && (child->top_attach != (child->bottom_attach - 1)))
	    {
	      has_expand = FALSE;
	      for (row = child->top_attach; row < child->bottom_attach; row++)
		if (table->rows[row].expand)
		  {
		    has_expand = TRUE;
		    break;
		  }

	      if (!has_expand)
		for (row = child->top_attach; row < child->bottom_attach; row++)
		  table->rows[row].need_expand = TRUE;
	    }
	}
    }

  for (col = 0; col < table->ncols; col++)
    if (table->cols[col].need_expand)
      table->cols[col].expand = TRUE;

  for (row = 0; row < table->nrows; row++)
    if (table->rows[row].need_expand)
      table->rows[row].expand = TRUE;

  g_function_leave ("gtk_table_size_allocate_init");
}

static void
gtk_table_size_allocate_pass1 (GtkTable *table)
{
  gint real_width;
  gint real_height;
  gint width, height;
  gint row, col;
  gint nexpand;
  gint extra;

  g_function_enter ("gtk_table_size_allocate_pass1");

  g_assert (table != NULL);

  /* If we were allocated more space than we requested
   *  then we have to expand any expandable rows and columns
   *  to fill in the extra space.
   */

  real_width = table->container.widget.allocation.width - table->container.border_width * 2;
  real_height = table->container.widget.allocation.height - table->container.border_width * 2;

  if (table->homogeneous)
    {
      nexpand = 0;
      for (col = 0; col < table->ncols; col++)
	if (table->cols[col].expand)
	  {
	    nexpand += 1;
	    break;
	  }

      if (nexpand > 0)
	{
	  width = real_width;

	  for (col = 0; col < table->ncols - 1; col++)
	    width -= table->cols[col].spacing;

	  extra = width / table->ncols;

	  for (col = 0; col < table->ncols; col++)
	    {
	      if ((col + 1) == table->ncols)
		table->cols[col].allocation = width;
	      else
		table->cols[col].allocation = extra;

	      width -= extra;
	    }
	}
    }
  else
    {
      width = 0;
      nexpand = 0;
      for (col = 0; col < table->ncols; col++)
	{
	  width += table->cols[col].requisition;
	  if (table->cols[col].expand)
	    nexpand += 1;
	}
      for (col = 0; col < table->ncols - 1; col++)
	width += table->cols[col].spacing;

      /* Check to see if we were allocated more width than we requested.
       */
      if ((width < real_width) && (nexpand >= 1))
	{
	  width = real_width - width;
	  extra = width / nexpand;

	  for (col = 0; col < table->ncols; col++)
	    if (table->cols[col].expand)
	      {
		if (nexpand == 1)
		  table->cols[col].allocation += width;
		else
		  table->cols[col].allocation += extra;

		width -= extra;
		nexpand -= 1;
	      }
	}
    }

  if (table->homogeneous)
    {
      nexpand = 0;
      for (row = 0; row < table->nrows; row++)
	if (table->rows[row].expand)
	  {
	    nexpand += 1;
	    break;
	  }

      if (nexpand > 0)
	{
	  height = real_height;

 	  for (row = 0; row < table->nrows - 1; row++)
	    height -= table->rows[row].spacing;

	  extra = height / table->nrows;

	  for (row = 0; row < table->nrows; row++)
	    {
	      if ((row + 1) == table->nrows)
		table->rows[row].allocation = height;
	      else
		table->rows[row].allocation = extra;

	      height -= extra;
	    }
	}
    }
  else
    {
      height = 0;
      nexpand = 0;
      for (row = 0; row < table->nrows; row++)
	{
	  height += table->rows[row].requisition;
	  if (table->rows[row].expand)
	    nexpand += 1;
	}
      for (row = 0; row < table->nrows - 1; row++)
	height += table->rows[row].spacing;

      /* Check to see if we were allocated more height than we requested.
       */
      if ((height < real_height) && (nexpand >= 1))
	{
	  height = real_height - height;
	  extra = height / nexpand;

	  for (row = 0; row < table->nrows; row++)
	    if (table->rows[row].expand)
	      {
		if (nexpand == 1)
		  table->rows[row].allocation += height;
		else
		  table->rows[row].allocation += extra;

		height -= extra;
		nexpand -= 1;
	      }
	}
    }

  g_function_leave ("gtk_table_size_allocate_pass1");
}

static void
gtk_table_size_allocate_pass2 (GtkTable *table)
{
  GtkTableChild *child;
  GList *children;
  gint max_width;
  gint max_height;
  gint x, y;
  gint row, col;

  g_function_enter ("gtk_table_size_allocate_pass2");

  g_assert (table != NULL);

  children = table->children;
  while (children)
    {
      child = children->data;
      children = children->next;

      if (GTK_WIDGET_VISIBLE (child->widget))
	{
	  x = table->container.widget.allocation.x + table->container.border_width;
	  y = table->container.widget.allocation.y + table->container.border_width;
	  max_width = 0;
	  max_height = 0;

	  for (col = 0; col < child->left_attach; col++)
	    {
	      x += table->cols[col].allocation;
	      x += table->cols[col].spacing;
	    }

	  for (col = child->left_attach; col < child->right_attach; col++)
	    {
	      max_width += table->cols[col].allocation;
	      if ((col + 1) < child->right_attach)
		max_width += table->cols[col].spacing;
	    }

	  for (row = 0; row < child->top_attach; row++)
	    {
	      y += table->rows[row].allocation;
	      y += table->rows[row].spacing;
	    }

	  for (row = child->top_attach; row < child->bottom_attach; row++)
	    {
	      max_height += table->rows[row].allocation;
	      if ((row + 1) < child->bottom_attach)
		max_height += table->rows[row].spacing;
	    }

	  if (child->xfill)
	    {
	      child->widget->allocation.width = max_width - child->xpadding * 2;
	      child->widget->allocation.x = x + (max_width - child->widget->allocation.width) / 2;
	    }
	  else
	    {
	      child->widget->allocation.width = child->widget->requisition.width;
	      child->widget->allocation.x = x + (max_width - child->widget->allocation.width) / 2;
	    }

	  if (child->yfill)
	    {
	      child->widget->allocation.height = max_height - child->ypadding * 2;
	      child->widget->allocation.y = y + (max_height - child->widget->allocation.height) / 2;
	    }
	  else
	    {
	      child->widget->allocation.height = child->widget->requisition.height;
	      child->widget->allocation.y = y + (max_height - child->widget->allocation.height) / 2;
	    }
	}
    }

  g_function_leave ("gtk_table_size_allocate_pass2");
}

static void
gtk_table_size_allocate_pass3 (GtkTable *table)
{
  GtkTableChild *child;
  GList *children;

  g_function_enter ("gtk_table_size_allocate_pass3");

  g_assert (table != NULL);

  children = table->children;
  while (children)
    {
      child = children->data;
      children = children->next;

      if (GTK_WIDGET_VISIBLE (child->widget))
	gtk_widget_size_allocate (child->widget, &child->widget->allocation);
    }

  g_function_leave ("gtk_table_size_allocate_pass3");
}
