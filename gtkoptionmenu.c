#include "gtkcontainer.h"
#include "gtkdata.h"
#include "gtkdraw.h"
#include "gtkmain.h"
#include "gtkmenu.h"
#include "gtkoptionmenu.h"
#include "gtkstyle.h"
#include "gtkwidget.h"
#include "gtkprivate.h"
#include "gtkmenuprivate.h"


static void  gtk_option_menu_destroy       (GtkWidget       *widget);
static void  gtk_option_menu_map           (GtkWidget       *widget);
static void  gtk_option_menu_unmap         (GtkWidget       *widget);
static void  gtk_option_menu_realize       (GtkWidget       *widget);
static void  gtk_option_menu_draw          (GtkWidget       *widget,
					    GdkRectangle    *area,
					    gint             is_expose);
static void  gtk_option_menu_expose        (GtkWidget       *widget);
static gint  gtk_option_menu_event         (GtkWidget       *widget,
					    GdkEvent        *event);
static void  gtk_option_menu_size_request  (GtkWidget       *widget,
					    GtkRequisition  *requisition);
static void  gtk_option_menu_size_allocate (GtkWidget       *widget,
					    GtkAllocation   *allocation);
static gint  gtk_option_menu_is_child      (GtkWidget       *widget,
					    GtkWidget       *child);
static gint  gtk_option_menu_locate        (GtkWidget       *widget,
					    GtkWidget      **child,
					    gint             x,
					    gint             y);
static void  gtk_option_menu_add           (GtkContainer    *container,
					    GtkWidget       *widget);
static void  gtk_option_menu_remove        (GtkContainer    *container,
					    GtkWidget       *widget);
static void  gtk_option_menu_foreach       (GtkContainer   *container,
					    GtkCallback     callback,
					    gpointer        callback_data);

static void  gtk_option_menu_popup           (GtkWidget *widget);
static void  gtk_option_menu_popdown         (GtkWidget *widget);
static void  gtk_option_menu_update_contents (GtkWidget *widget);
static void  gtk_option_menu_remove_contents (GtkWidget *widget);
static void  gtk_option_menu_calc_size       (GtkWidget *widget);


static GtkWidgetFunctions option_menu_widget_functions =
{
  gtk_option_menu_destroy,
  gtk_widget_default_show,
  gtk_widget_default_hide,
  gtk_option_menu_map,
  gtk_option_menu_unmap,
  gtk_option_menu_realize,
  gtk_option_menu_draw,
  gtk_widget_default_draw_focus,
  gtk_option_menu_event,
  gtk_option_menu_size_request,
  gtk_option_menu_size_allocate,
  gtk_option_menu_is_child,
  gtk_option_menu_locate,
  gtk_widget_default_activate,
  gtk_widget_default_set_state,
  gtk_widget_default_install_accelerator,
  gtk_widget_default_remove_accelerator,
};

static GtkContainerFunctions option_menu_container_functions =
{
  gtk_option_menu_add,
  gtk_option_menu_remove,
  gtk_container_default_need_resize,
  gtk_container_default_focus_advance,
  gtk_option_menu_foreach,
};


GtkWidget*
gtk_option_menu_new ()
{
  GtkOptionMenu *option_menu;

  g_function_enter ("gtk_option_menu_new");

  option_menu = g_new (GtkOptionMenu, 1);

  option_menu->container.widget.type = gtk_get_option_menu_type ();
  option_menu->container.widget.function_table = &option_menu_widget_functions;
  option_menu->container.function_table = &option_menu_container_functions;

  gtk_widget_set_defaults ((GtkWidget*) option_menu);
  gtk_container_set_defaults ((GtkWidget*) option_menu);

  option_menu->child = NULL;
  option_menu->menu_item = NULL;
  option_menu->menu = NULL;
  option_menu->width = 0;
  option_menu->height = 0;
  option_menu->in_option_menu = FALSE;
  option_menu->update_contents = TRUE;
  option_menu->menu_toggle_exists = FALSE;
  option_menu->ask_for_resize = TRUE;

  g_function_leave ("gtk_option_menu_new");
  return ((GtkWidget*) option_menu);
}

GtkWidget*
gtk_option_menu_get_menu (GtkWidget *option_menu)
{
  GtkOptionMenu *roption_menu;

  g_function_enter ("gtk_option_menu_get_menu");

  g_assert (option_menu != NULL);
  roption_menu = (GtkOptionMenu*) option_menu;

  g_function_leave ("gtk_option_menu_get_menu");
  return roption_menu->menu;
}

void
gtk_option_menu_set_menu (GtkWidget *option_menu,
			  GtkWidget *menu)
{
  GtkOptionMenu *roption_menu;

  g_function_enter ("gtk_option_menu_set_menu");

  g_assert (option_menu != NULL);
  g_assert (menu != NULL);

  roption_menu = (GtkOptionMenu*) option_menu;
  roption_menu->menu = menu;

  gtk_option_menu_calc_size (option_menu);
  gtk_option_menu_update_contents (option_menu);
  roption_menu->ask_for_resize = FALSE;

  g_function_leave ("gtk_option_menu_set_menu");
}

void
gtk_option_menu_set_history (GtkWidget *option_menu,
			     gint       index)
{
  GtkOptionMenu *roption_menu;

  g_function_enter ("gtk_option_menu_set_history");

  g_assert (option_menu != NULL);

  roption_menu = (GtkOptionMenu*) option_menu;

  if (roption_menu->menu)
    {
      gtk_menu_set_active (roption_menu->menu, index);
      gtk_option_menu_remove_contents (option_menu);
      gtk_option_menu_update_contents (option_menu);
    }

  g_function_leave ("gtk_option_menu_set_history");
}

guint16
gtk_get_option_menu_type ()
{
  static guint16 option_menu_type = 0;

  g_function_enter ("gtk_get_option_menu_type");

  if (!option_menu_type)
    gtk_widget_unique_type (&option_menu_type);

  g_function_leave ("gtk_get_option_menu_type");
  return option_menu_type;
}


static void
gtk_option_menu_destroy (GtkWidget *widget)
{
  GtkOptionMenu *option_menu;

  g_function_enter ("gtk_option_menu_destroy");

  g_assert (widget != NULL);
  option_menu = (GtkOptionMenu*) widget;

  if (option_menu->child)
    if (!gtk_widget_destroy (option_menu->child))
      option_menu->child->parent = NULL;
  if (option_menu->menu)
    if (!gtk_widget_destroy (option_menu->menu))
      option_menu->menu->parent = NULL;

  g_free (option_menu);

  g_function_leave ("gtk_option_menu_destroy");
}

static void
gtk_option_menu_map (GtkWidget *widget)
{
  GtkOptionMenu *option_menu;

  g_function_enter ("gtk_option_menu_map");

  g_assert (widget != NULL);
  option_menu = (GtkOptionMenu*) widget;

  if (!GTK_WIDGET_MAPPED (widget))
    {
      GTK_WIDGET_SET_FLAGS (widget, GTK_MAPPED);
      gdk_window_show (widget->window);

      if (option_menu->child &&
	  GTK_WIDGET_VISIBLE (option_menu->child) &&
	  !GTK_WIDGET_MAPPED (option_menu->child))
	gtk_widget_map (option_menu->child);
    }

  g_function_leave ("gtk_option_menu_map");
}

static void
gtk_option_menu_unmap (GtkWidget *widget)
{
  g_function_enter ("gtk_option_menu_unmap");

  g_assert (widget);

  if (GTK_WIDGET_MAPPED (widget))
    {
      GTK_WIDGET_UNSET_FLAGS (widget, GTK_MAPPED);
      gdk_window_hide (widget->window);
    }

  g_function_leave ("gtk_option_menu_unmap");
}

static void
gtk_option_menu_realize (GtkWidget *widget)
{
  GtkOptionMenu *option_menu;
  GdkWindowAttr attributes;

  g_function_enter ("gtk_option_menu_realize");

  g_assert (widget != NULL);

  option_menu = (GtkOptionMenu*) widget;
  GTK_WIDGET_SET_FLAGS (widget, GTK_REALIZED);

  attributes.window_type = GDK_WINDOW_CHILD;
  attributes.x = widget->allocation.x;
  attributes.y = widget->allocation.y;
  attributes.width = widget->allocation.width;
  attributes.height = widget->allocation.height;
  attributes.wclass = GDK_INPUT_OUTPUT;
  attributes.event_mask = (GDK_EXPOSURE_MASK |
                           GDK_BUTTON_PRESS_MASK |
                           GDK_BUTTON_RELEASE_MASK |
                           GDK_ENTER_NOTIFY_MASK |
                           GDK_LEAVE_NOTIFY_MASK);

  option_menu->container.widget.window = gdk_window_new (widget->parent->widget.window,
							 &attributes, GDK_WA_X | GDK_WA_Y);
  gdk_window_set_user_data (option_menu->container.widget.window, option_menu);

  option_menu->container.widget.style = gtk_style_attach (option_menu->container.widget.style,
							  option_menu->container.widget.window);
  gdk_window_set_background (option_menu->container.widget.window,
                             &option_menu->container.widget.style->background[GTK_STATE_NORMAL]);


  g_function_leave ("gtk_option_menu_realize");
}

static void
gtk_option_menu_draw (GtkWidget    *widget,
		      GdkRectangle *area,
		      gint          is_expose)
{
  GtkOptionMenu *option_menu;

  g_function_enter ("gtk_option_menu_draw");

  g_assert (widget != NULL);

  if (GTK_WIDGET_VISIBLE (widget) && GTK_WIDGET_MAPPED (widget))
    {
      option_menu = (GtkOptionMenu*) widget;
      gtk_option_menu_expose (widget);

      if (option_menu->child)
	if (!is_expose || GTK_WIDGET_NO_WINDOW (option_menu->child))
	  gtk_widget_draw (option_menu->child, NULL, is_expose);
    }

  g_function_leave ("gtk_option_menu_draw");
}

static void
gtk_option_menu_expose (GtkWidget *widget)
{
  GtkOptionMenu *option_menu;
  gint x, y;
  gint width, height;
  gint state;

  g_function_enter ("gtk_option_menu_expose");

  g_assert (widget != NULL);

  if (GTK_WIDGET_VISIBLE (widget) && GTK_WIDGET_MAPPED (widget))
    {
      option_menu = (GtkOptionMenu*) widget;

      state = GTK_STATE_NORMAL;
      if (option_menu->in_option_menu)
	state = GTK_STATE_PRELIGHT;

      if (option_menu->child)
	gtk_widget_set_state (option_menu->child, state);

      gdk_window_set_background (widget->window,
				 &option_menu->container.widget.style->background[state]);
      gdk_window_clear (widget->window);

      x = option_menu->container.border_width;
      y = option_menu->container.border_width;
      width = widget->allocation.width - 2 * x;
      height = widget->allocation.height - 2 * y;

      gtk_draw_shadow (widget->window,
		       option_menu->container.widget.style->highlight_gc[state],
                       option_menu->container.widget.style->shadow_gc[state],
                       NULL,
                       GTK_SHADOW_OUT,
                       x, y, width, height,
                       option_menu->container.widget.style->shadow_thickness);

      if (option_menu->menu_item || !option_menu->child)
	{
	  gtk_draw_shadow (widget->window,
			   option_menu->container.widget.style->highlight_gc[state],
			   option_menu->container.widget.style->shadow_gc[state],
			   NULL,
			   GTK_SHADOW_OUT,
			   x + width - x - OPTION_INDICATOR_WIDTH - OPTION_INDICATOR_SPACING * 4,
			   y + height / 2 - OPTION_INDICATOR_HEIGHT / 2,
			   OPTION_INDICATOR_WIDTH, OPTION_INDICATOR_HEIGHT,
			   option_menu->container.widget.style->shadow_thickness);
	}
    }

  g_function_leave ("gtk_option_menu_expose");
}

static gint
gtk_option_menu_event (GtkWidget *widget,
		       GdkEvent  *event)
{
  GtkOptionMenu *option_menu;
  GtkWidget *event_widget;

  g_function_enter ("gtk_option_menu_event");

  g_assert (widget != NULL);
  g_assert (event != NULL);

  option_menu = (GtkOptionMenu*) widget;

  switch (event->type)
    {
    case GDK_EXPOSE:
      gtk_widget_draw (widget, &event->expose.area, TRUE);
      break;

    case GDK_BUTTON_PRESS:
      if (event->button.button == 1)
	gtk_option_menu_popup (widget);
      break;

    case GDK_BUTTON_RELEASE:
      if (event->button.button == 1)
	gtk_option_menu_popdown (widget);
      break;

    case GDK_ENTER_NOTIFY:
      event_widget = gtk_get_event_widget (event);

      if ((event_widget == widget) &&
	  (!option_menu->menu || !GTK_WIDGET_VISIBLE (option_menu->menu)))
	{
	  if ((event->crossing.detail != GDK_NOTIFY_INFERIOR) && !option_menu->in_option_menu)
	    {
	      option_menu->in_option_menu = TRUE;
	      gtk_widget_draw (widget, NULL, FALSE);
	    }
	}
      break;

    case GDK_LEAVE_NOTIFY:
      event_widget = gtk_get_event_widget (event);

      if ((event_widget == widget) &&
	  (!option_menu->menu || !GTK_WIDGET_VISIBLE (option_menu->menu)))
	{
	  if ((event->crossing.detail != GDK_NOTIFY_INFERIOR) && option_menu->in_option_menu)
	    {
	      option_menu->in_option_menu = FALSE;
	      gtk_widget_draw (widget, NULL, FALSE);
	    }
	}
      break;

    default:
      break;
    }

  g_function_leave ("gtk_option_menu_event");
  return FALSE;
}

static void
gtk_option_menu_size_request (GtkWidget      *widget,
			      GtkRequisition *requisition)
{
  GtkOptionMenu *option_menu;
  gint shadow_thickness;
  gint temp;

  g_function_enter ("gtk_option_menu_size_request");

  g_assert (widget != NULL);
  option_menu = (GtkOptionMenu*) widget;

  if (GTK_WIDGET_VISIBLE (widget))
    {
      shadow_thickness = widget->style->shadow_thickness;

      if (option_menu->update_contents && option_menu->menu)
	{
          requisition->width = (option_menu->width +
                                option_menu->container.border_width * 2 +
                                shadow_thickness * 2 +
				OPTION_INDICATOR_WIDTH +
				OPTION_INDICATOR_SPACING * 5);
          requisition->height = (option_menu->height +
                                 option_menu->container.border_width * 2 +
                                 shadow_thickness * 2);

	  if (option_menu->menu_toggle_exists)
	    requisition->width += TOGGLE_MARK_SIZE + 2 * TOGGLE_MARK_SPACING;

	  temp = (option_menu->container.border_width * 2 + shadow_thickness * 2 +
		  OPTION_INDICATOR_HEIGHT + OPTION_INDICATOR_SPACING * 2);
	  requisition->height = MAX (requisition->height, temp);
	}
      else if (option_menu->child)
        {
          option_menu->child->requisition.width = 0;
          option_menu->child->requisition.height = 0;

          gtk_widget_size_request (option_menu->child, &option_menu->child->requisition);

          requisition->width = (option_menu->child->requisition.width +
                                option_menu->container.border_width * 2 +
                                shadow_thickness * 2);
          requisition->height = (option_menu->child->requisition.height +
                                 option_menu->container.border_width * 2 +
                                 shadow_thickness * 2);
        }
      else
        {
          requisition->width = (option_menu->container.border_width * 2 +
				shadow_thickness * 2 +
				OPTION_INDICATOR_WIDTH +
				OPTION_INDICATOR_SPACING * 5);
          requisition->height = (option_menu->container.border_width * 2 +
				 shadow_thickness * 2 +
				 OPTION_INDICATOR_HEIGHT +
				 OPTION_INDICATOR_SPACING * 2);
        }
    }
  else
    {
      requisition->width = 0;
      requisition->height = 0;
    }

  g_function_leave ("gtk_option_menu_size_request");
}

static void
gtk_option_menu_size_allocate (GtkWidget     *widget,
			       GtkAllocation *allocation)
{
  GtkOptionMenu *option_menu;
  GtkMenuItem *active;
  GtkAllocation child_allocation;
  gint shadow_thickness;

  g_function_enter ("gtk_option_menu_allocate");

  g_assert (widget != NULL);
  option_menu = (GtkOptionMenu*) widget;

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

  if (option_menu->child)
    {
      shadow_thickness = widget->style->shadow_thickness;
      child_allocation.x = option_menu->container.border_width + shadow_thickness;
      child_allocation.y = option_menu->container.border_width + shadow_thickness;
      child_allocation.width = allocation->width - child_allocation.x * 2;
      child_allocation.height = allocation->height - child_allocation.y * 2;

      if (option_menu->update_contents)
	{
	  if (option_menu->menu_toggle_exists)
	    child_allocation.x += TOGGLE_MARK_SIZE + 2 * TOGGLE_MARK_SPACING;

	  active = (GtkMenuItem*) gtk_menu_get_active (option_menu->menu);
	  if (active)
	    {
	      child_allocation.x += (active->container.border_width +
				     active->container.widget.style->shadow_thickness + 2);
	      child_allocation.y += (active->container.border_width +
				     active->container.widget.style->shadow_thickness + 2);
	      child_allocation.width -= 2 * (active->container.border_width +
					     active->container.widget.style->shadow_thickness + 2) +
					     OPTION_INDICATOR_WIDTH + OPTION_INDICATOR_SPACING * 5;
	      child_allocation.height -= 2 * (active->container.border_width +
					      active->container.widget.style->shadow_thickness + 2);
	    }
	}

      if (child_allocation.width <= 0)
        child_allocation.width = 1;
      if (child_allocation.height <= 0)
        child_allocation.height = 1;

      gtk_widget_size_allocate (option_menu->child, &child_allocation);
    }

  g_function_leave ("gtk_option_menu_size_allocate");
}

static gint
gtk_option_menu_is_child (GtkWidget *widget,
			  GtkWidget *child)
{
  GtkOptionMenu *option_menu;
  gint return_val;

  g_function_enter ("gtk_option_menu_is_child");

  g_assert (widget != NULL);
  g_assert (child != NULL);

  option_menu = (GtkOptionMenu*) widget;

  return_val = FALSE;
  if (option_menu->child == child)
    return_val = TRUE;
  else if (option_menu->child)
    return_val = gtk_widget_is_child (option_menu->child, child);

  g_function_leave ("gtk_option_menu_is_child");
  return return_val;
}

static gint
gtk_option_menu_locate (GtkWidget  *widget,
			GtkWidget **child,
			gint        x,
			gint        y)
{
  GtkOptionMenu *option_menu;
  gint return_val;
  gint child_x;
  gint child_y;

  g_function_enter ("gtk_option_menu_locate");

  g_assert (widget != NULL);
  g_assert (child != NULL);

  return_val = FALSE;
  *child = NULL;

  if ((x >= 0) && (y >= 0) &&
      (x < widget->allocation.width) &&
      (y < widget->allocation.height))
    {
      return_val = TRUE;

      option_menu = (GtkOptionMenu*) widget;
      if (option_menu->child)
        {
          child_x = x - option_menu->child->allocation.x;
          child_y = y - option_menu->child->allocation.y;

          gtk_widget_locate (option_menu->child, child, child_x, child_y);
        }

      if (!(*child))
        *child = widget;
    }

  g_function_leave ("gtk_option_menu_locate");
  return return_val;
}

static void
gtk_option_menu_add (GtkContainer *container,
		     GtkWidget    *widget)
{
  GtkOptionMenu *option_menu;

  g_function_enter ("gtk_option_menu_add");

  g_assert (container != NULL);
  option_menu = (GtkOptionMenu*) container;

  if (option_menu->child)
    g_error ("option_menu already has a child");
  else
    option_menu->child = widget;

  if (option_menu->ask_for_resize)
    if (GTK_WIDGET_VISIBLE (widget) && GTK_WIDGET_VISIBLE (container))
      gtk_container_need_resize (container, widget);

  g_function_leave ("gtk_option_menu_add");
}

static void
gtk_option_menu_remove (GtkContainer *container,
			GtkWidget    *widget)
{
  GtkOptionMenu *option_menu;

  g_function_enter ("gtk_option_menu_remove");

  g_assert (container != NULL);
  g_assert (widget != NULL);

  option_menu = (GtkOptionMenu*) container;

  if (option_menu->child != widget)
    g_error ("attempted to remove widget which wasn't a child");

  if (!option_menu->child)
    g_error ("option_menu has no child to remove");

  option_menu->child = NULL;

  if (option_menu->ask_for_resize)
    if (GTK_WIDGET_VISIBLE (widget) && GTK_WIDGET_VISIBLE (container))
      gtk_container_need_resize (container, widget);

  g_function_leave ("gtk_option_menu_remove");
}

static void
gtk_option_menu_foreach (GtkContainer *container,
			 GtkCallback   callback,
			 gpointer      callback_data)
{
  GtkOptionMenu *option_menu;

  g_function_enter ("gtk_option_menu_foreach");

  g_assert (container != NULL);
  g_assert (callback != NULL);

  option_menu = (GtkOptionMenu*) container;

  if (option_menu->child)
    (* callback) (option_menu->child, callback_data, NULL);

  g_function_leave ("gtk_option_menu_foreach");
}

static void
gtk_option_menu_popup (GtkWidget *widget)
{
  GtkOptionMenu *option_menu;
  GtkMenu *menu;

  g_function_enter ("gtk_option_menu_popup");

  g_assert (widget != NULL);
  option_menu = (GtkOptionMenu*) widget;

  if (option_menu->menu)
    {
      gtk_option_menu_remove_contents (widget);

      menu = (GtkMenu*) option_menu->menu;
      menu->parent = (GtkWidget*) option_menu;

      gtk_widget_show (option_menu->menu);
    }

  g_function_leave ("gtk_option_menu_popup");
}

static void
gtk_option_menu_popdown (GtkWidget *widget)
{
  GtkOptionMenu *option_menu;

  g_function_enter ("gtk_option_menu_popdown");

  g_assert (widget != NULL);
  option_menu = (GtkOptionMenu*) widget;

  if (option_menu->menu)
    {
      gtk_widget_hide (option_menu->menu);
      gtk_option_menu_update_contents (widget);
    }

  g_function_leave ("gtk_option_menu_popdown");
}

static void
gtk_option_menu_update_contents (GtkWidget *widget)
{
  GtkOptionMenu *option_menu;
  GtkWidget *child;

  g_function_enter ("gtk_option_menu_update_contents");

  g_assert (widget != NULL);
  option_menu = (GtkOptionMenu*) widget;

  if (option_menu->update_contents)
    {
      if (option_menu->menu)
	{
	  option_menu->child = NULL;
	  option_menu->menu_item = gtk_menu_get_active (option_menu->menu);

	  if (option_menu->menu_item)
	    {
	      child = gtk_menu_item_get_child (option_menu->menu_item);
	      gtk_widget_reparent (child, widget);
	      gtk_widget_map (child);
	      gtk_widget_size_allocate (widget, &widget->allocation);
	      gtk_widget_draw (widget, NULL, FALSE);
	    }
	}
    }

  g_function_leave ("gtk_option_menu_update_contents");
}

static void
gtk_option_menu_remove_contents (GtkWidget *widget)
{
  GtkOptionMenu *option_menu;

  g_function_enter ("gtk_option_menu_remove_contents");

  g_assert (widget != NULL);
  option_menu = (GtkOptionMenu*) widget;

  if (option_menu->update_contents)
    {
      gtk_widget_reparent (option_menu->child, option_menu->menu_item);
      option_menu->child = NULL;
      option_menu->menu_item = NULL;
    }

  g_function_leave ("gtk_option_menu_remove_contents");
}

static void
gtk_option_menu_calc_size (GtkWidget *widget)
{
  GtkOptionMenu *option_menu;
  GtkMenuItem *rmenu_item;
  GtkWidget *menu_item;
  GList *children;

  g_function_enter ("gtk_option_menu_calc_size");

  g_assert (widget != NULL);
  option_menu = (GtkOptionMenu*) widget;

  option_menu->width = 0;
  option_menu->height = 0;
  children = gtk_menu_get_children (option_menu->menu);

  while (children)
    {
      menu_item = children->data;
      children = children->next;

      if (GTK_WIDGET_VISIBLE (menu_item))
	{
	  menu_item->requisition.width = 0;
	  menu_item->requisition.height = 0;

	  gtk_widget_size_request (menu_item, &menu_item->requisition);

	  option_menu->width = MAX (option_menu->width, menu_item->requisition.width);
	  option_menu->height = MAX (option_menu->height, menu_item->requisition.height);

	  rmenu_item = (GtkMenuItem*) menu_item;
	  if (rmenu_item->toggle_exists)
	    option_menu->menu_toggle_exists = TRUE;
	}
    }

  g_function_leave ("gtk_option_menu_calc_size");
}

void
gtk_option_menu_position (GtkWidget *option_menu,
			  gint      *x,
			  gint      *y,
			  gint       width,
			  gint       height)
{
  GtkOptionMenu *roption_menu;
  GtkWidget *active;
  GList *children;
  gint menu_xpos;
  gint menu_ypos;
  gint screen_width;
  gint screen_height;
  gint shift_menu;

  g_function_enter ("gtk_option_menu_position");

  g_assert (option_menu != NULL);
  g_assert (x != NULL);
  g_assert (y != NULL);

  roption_menu = (GtkOptionMenu*) option_menu;

  g_assert (roption_menu->menu != NULL);

  active = gtk_menu_get_active (roption_menu->menu);
  children = gtk_menu_get_children (roption_menu->menu);
  gdk_window_get_origin (option_menu->window, &menu_xpos, &menu_ypos);

  while (children)
    {
      if (active == (GtkWidget*) children->data)
	break;

      menu_ypos -= active->requisition.height;
      children = children->next;
    }

  screen_width = gdk_screen_width ();
  screen_height = gdk_screen_height ();

  shift_menu = FALSE;
  if (menu_ypos < 0)
    {
      menu_ypos = 0;
      shift_menu = TRUE;
    }
  else if ((menu_ypos + height) > screen_height)
    {
      menu_ypos -= ((menu_ypos + height) - screen_height);
      shift_menu = TRUE;
    }

  if (shift_menu)
    {
      if ((menu_xpos + option_menu->window->width + width) <= screen_width)
	menu_xpos += option_menu->window->width;
      else
	menu_xpos -= width;
    }

  if (menu_xpos < 0)
    menu_xpos = 0;
  else if ((menu_xpos + width) > screen_width)
    menu_xpos -= ((menu_xpos + width) - screen_width);

  *x = menu_xpos;
  *y = menu_ypos;

  g_function_leave ("gtk_option_menu_position");
}
