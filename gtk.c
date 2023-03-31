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
#include <stdlib.h>
#include "gtkcontainer.h"
#include "gtkmain.h"
#include "gtkstyle.h"
#include "gtkwidget.h"
#include "gtkwindow.h"
#include "gtkprivate.h"


/* Private type definitions
 */
typedef struct _GtkOption           GtkOption;
typedef struct _GtkTimeoutFunction  GtkTimeoutFunction;

struct _GtkOption
{
  char *name;
  int has_arg;
  void (*func) (char *arg);
};

struct _GtkTimeoutFunction
{
  gint tag;
  guint32 start;
  guint32 interval;
  guint32 originterval;
  gint expired;
  GtkFunction function;
  gpointer data;
};


/* Private function declarations.
 */
static void  gtk_handle_foreground       (char *arg);
static void  gtk_handle_background       (char *arg);
static void  gtk_handle_shadow_thickness (char *arg);

static void  gtk_exit_func       (void);
static void  gtk_handle_timeouts (void);
static void  gtk_handle_timer    (void);
static void  gtk_propogate_event (GtkWidget *widget,
				  GdkEvent  *event);

static void  gtk_root_destroy             (GtkWidget         *widget);
static void  gtk_root_show                (GtkWidget         *widget);
static void  gtk_root_hide                (GtkWidget         *widget);
static void  gtk_root_map                 (GtkWidget         *widget);
static void  gtk_root_unmap               (GtkWidget         *widget);
static void  gtk_root_realize             (GtkWidget         *widget);
static void  gtk_root_draw                (GtkWidget         *widget,
					   GdkRectangle      *area,
					   gint               is_expose);
static void  gtk_root_draw_focus          (GtkWidget         *widget);
static gint  gtk_root_event               (GtkWidget         *widget,
					   GdkEvent          *event);
static void  gtk_root_size_request        (GtkWidget         *widget,
					   GtkRequisition    *requisition);
static void  gtk_root_size_allocate       (GtkWidget         *widget,
					   GtkAllocation     *allocation);
static gint  gtk_root_is_child            (GtkWidget         *widget,
					   GtkWidget         *child);
static gint  gtk_root_locate              (GtkWidget         *widget,
					   GtkWidget        **child,
					   gint               x,
					   gint               y);
static void  gtk_root_activate            (GtkWidget         *widget);
static void  gtk_root_set_state           (GtkWidget         *widget,
					   GtkStateType       state);
static gint  gtk_root_install_accelerator (GtkWidget         *widget,
					   gchar              accelerator_key,
					   guint8             accelerator_mods);
static void  gtk_root_remove_accelerator  (GtkWidget         *widget);
static void  gtk_root_add                 (GtkContainer      *container,
					   GtkWidget         *widget);
static void  gtk_root_remove              (GtkContainer      *container,
					   GtkWidget         *widget);
static void  gtk_root_need_resize         (GtkContainer      *container,
					   GtkWidget         *widget);
static void  gtk_root_focus_advance       (GtkContainer      *container,
					   GtkWidget        **child,
					   GtkDirectionType   direction);

/* Private variable declarations.
 */
static int initialized = 0;                /* 1 if "gtk" is initialized,
					    * 0 otherwise
					    */
static GdkEvent next_event;
static GtkWidget *event_widget;
static GtkWidget *grab_widget;
static gint have_event;
static gint have_next_event;

static GtkOption options[] =
{
  { "foreground",       TRUE,  gtk_handle_foreground },
  { "fg",               TRUE,  gtk_handle_foreground },
  { "background",       TRUE,  gtk_handle_background },
  { "bg",               TRUE,  gtk_handle_background },
  { "shadow_thickness", TRUE,  gtk_handle_shadow_thickness },
  { "st",               TRUE,  gtk_handle_shadow_thickness },
};
static int noptions = sizeof (options) / sizeof (GtkOption);

static GList *grabs = NULL;                /* A list of grabs. The grabbing widget
					    *  is the first one on the list.
					    */
static GList *timeout_functions = NULL;    /* A list of timeout functions sorted by
					    *  when the length of the time interval
					    *  remaining. Therefore, the first timeout
					    *  function to expire is at the head of
					    *  the list and the last to expire is at
					    *  the tail of the list.
					    */

static GdkVisual *gtk_visual;              /* The visual to be used in creating new
					    *  widgets.
					    */
static GdkColormap *gtk_colormap;          /* The colormap to be used in creating new
					    *  widgets.
					    */

static GList *visuals = NULL;
static GList *colormaps = NULL;
static GList *styles = NULL;

static GdkEvent current_event;


/* The root widget function table. Unlike every other
 *  widget, the root widget has some NULL functions. This
 *  is because those functions are never to be called and
 *  if they are an error should occur. By leaving them NULL,
 *  an error will caught whenever they are called.
 */
static GtkWidgetFunctions root_widget_functions =
{
  gtk_root_destroy,
  gtk_root_show,
  gtk_root_hide,
  gtk_root_map,
  gtk_root_unmap,
  gtk_root_realize,
  gtk_root_draw,
  gtk_root_draw_focus,
  gtk_root_event,
  gtk_root_size_request,
  gtk_root_size_allocate,
  gtk_root_is_child,
  gtk_root_locate,
  gtk_root_activate,
  gtk_root_set_state,
  gtk_root_install_accelerator,
  gtk_root_remove_accelerator,
};

/* The root widget container function table.
 */
static GtkContainerFunctions root_container_functions =
{
  gtk_root_add,
  gtk_root_remove,
  gtk_root_need_resize,
  gtk_root_focus_advance,
};


void
gtk_init (int    *argc,
	  char ***argv)
{
  GtkStyle *default_style;
  GdkPixmap *pixmap;
  gint i, j, k;
  char *arg;

  g_function_enter ("gtk_init");

  /* Initialize "gdk". We simply pass along the 'argc' and 'argv'
   *  parameters as they contain information that
   */
  gdk_init (argc, argv);

  /* Handle command line arguments...such as setting the foreground
   *  and background colors or the font.
   */
  for (i = 1; i < *argc; i++)
    {
      if ((*argv)[i][0] == '-')
	{
	  for (j = 0; j < noptions; j++)
	    {
	      if (strcmp (options[j].name, (char*) &((*argv)[i][1])) == 0)
		{
		  (*argv)[i] = NULL;

		  if (options[j].func)
		    {
		      arg = NULL;
		      if ((options[j].has_arg) && ((i + 1) < *argc))
			{
			  arg = (*argv)[i + 1];
			  (*argv)[i + 1] = NULL;
			  i += 1;
			}

		      (* options[j].func) (arg);
		    }

		  break;
		}
	    }
	}
    }

  /* Compress the arg vector so that the caller
   *  won't be able to tell that handled arguments
   *  were ever there.
   */
  for (i = 1; i < *argc; i++)
    {
      for (k = i; k < *argc; k++)
	if ((*argv)[k] != NULL)
	  break;

      if (k > i)
	{
	  k -= i;
	  for (j = i + k; j < *argc; j++)
	    (*argv)[j-k] = (*argv)[j];
	  *argc -= k;
	}
    }

  /* Initialize the default visual and colormap to be
   *  used in creating widgets. (We want to use the system
   *  defaults so as to be nice to the colormap).
   */
  gtk_visual = gdk_visual_get_system ();
  gtk_colormap = gdk_colormap_get_system ();

  /* Push the default visual and colormap onto their
   *  respective stacks.
   */
  gtk_push_visual (gtk_visual);
  gtk_push_colormap (gtk_colormap);

  /* Create and push the default style onto the style stack.
   */
  default_style = gtk_style_new (-1);
  gtk_push_style (default_style);

  /* Create the root container.
   */
  gtk_root = g_new (GtkContainer, 1);
  gtk_widget_set_defaults ((GtkWidget*) gtk_root);
  gtk_container_set_defaults ((GtkWidget*) gtk_root);

  pixmap = gdk_pixmap_new (NULL, 1, 1, gtk_visual->depth);
  gtk_root->widget.parent = NULL;
  gtk_root->widget.style = gtk_style_attach (gtk_root->widget.style, pixmap);
  gtk_root->widget.flags = GTK_WIDGET_SET_FLAGS (gtk_root, GTK_VISIBLE);
  gtk_root->widget.function_table = &root_widget_functions;
  gtk_root->function_table = &root_container_functions;

  /* Register an exit function to make sure we are able to cleanup.
   */
  if (ATEXIT (gtk_exit_func))
    g_warning ("unable to register exit function");

  /* Set the 'initialized' flag.
   */
  initialized = 1;

  g_function_leave ("gtk_init");
}

void
gtk_exit (int errorcode)
{
  g_function_enter ("gtk_exit");

  /* Only if "gtk" has been initialized should we de-initialize.
   */
  if (initialized)
    {
      initialized = 0;
      gdk_exit (errorcode);
    }

  g_function_leave ("gtk_exit");
}

void
gtk_main ()
{
  g_function_enter ("gtk_main");

  /* 'have_event' specifies whether the 'current_event' stucture contains
   *  a valid new event.
   * 'have_next_event' specifies whether the 'next_event' structure
   *  contains a valid new event.
   * The 'current_event' and 'next_event' structures are used to implement
   *  enter/leave notify event compression.
   */
  have_event = FALSE;
  have_next_event = FALSE;

  grabs = NULL;

  while (1)
    gtk_main_iteration ();

  g_function_leave ("gtk_main");
}

void
gtk_main_iteration ()
{
  g_function_enter ("gtk_main_iteration");

  /* Handle a timeout functions that may have expired.
   */
  gtk_handle_timeouts ();

  /* Handle setting of the "gdk" timer. If there are no
   *  timeout functions, then the timer is turned off.
   *  If there are timeout functions, then the timer is
   *  set to the shortest timeout interval (which is
   *  the first timeout function).
   */
  gtk_handle_timer ();

  /* If there is a valid event in 'next_event' then copy
   *  it to 'event' an unset the flag.
   */
  if (have_next_event)
    {
      have_next_event = FALSE;
      have_event = TRUE;
      current_event = next_event;
    }

  /* If we don't have an event then get one.
   */
  if (!have_event)
    have_event = gdk_event_get (&current_event);

  /* "gdk_event_get" can return FALSE if the timer goes off
   *  and no events are pending. Therefore, we should make
   *  sure that we got an event before continuing.
   */
  if (have_event)
    {
      have_event = FALSE;

      /* If there are any events pending then get the next one.
       */
      if (gdk_events_pending () > 0)
	have_next_event = gdk_event_get (&next_event);

      /* Try to compress enter/leave notify events. These event
       *  pairs occur when the mouse is dragged quickly across
       *  a window with many buttons (or through a menu). Instead
       *  of highlighting and de-highlighting each widget that
       *  is crossed it is better to simply de-highlight the widget
       *  which contained the mouse initially and highlight the
       *  widget which ends up containing the mouse.
       */
      if (have_next_event)
	if (((current_event.type == GDK_ENTER_NOTIFY) ||
	     (current_event.type == GDK_LEAVE_NOTIFY)) &&
	    ((next_event.type == GDK_ENTER_NOTIFY) ||
	     (next_event.type == GDK_LEAVE_NOTIFY)) &&
	    (next_event.type != current_event.type) &&
	    (next_event.any.window == current_event.any.window))
	  goto done;

      /* Find the widget which got the event. We store the widget
       *  in the user_data field of GdkWindow's.
       */
      event_widget = gtk_get_event_widget (&current_event);

      /* If there is a grab in effect...
       */
      if (grabs)
	grab_widget = grabs->data;
      else
	grab_widget = event_widget;

      /* Not all events get sent to the grabbing widget.
       * The delete, destroy, expose, focus change and resize
       *  events still get sent to the event widget because
       *  1) these events have no meaning for the grabbing widget
       *  and 2) redirecting these events to the grabbing widget
       *  could cause the display to be messed up.
       */
      switch (current_event.type)
	{
	case GDK_NOTHING:
	  break;

	case GDK_DELETE:
	case GDK_DESTROY:
	  gtk_widget_destroy (event_widget);
	  break;

	case GDK_EXPOSE:
	case GDK_FOCUS_CHANGE:
	case GDK_RESIZE:
	case GDK_MAP:
	case GDK_UNMAP:
	  gtk_widget_event (event_widget, &current_event);
	  break;

	case GDK_MOTION_NOTIFY:
	case GDK_BUTTON_PRESS:
	case GDK_2BUTTON_PRESS:
	case GDK_3BUTTON_PRESS:
	case GDK_BUTTON_RELEASE:
	case GDK_KEY_PRESS:
	case GDK_KEY_RELEASE:
	  if (GTK_WIDGET_IS_SENSITIVE (grab_widget))
	    gtk_propogate_event (grab_widget, &current_event);
	  break;

	case GDK_ENTER_NOTIFY:
	case GDK_LEAVE_NOTIFY:
	  if (GTK_WIDGET_IS_SENSITIVE (grab_widget))
	    gtk_widget_event (grab_widget, &current_event);
	  break;
	}
    }

done:
  g_function_leave ("gtk_main_iteration");
}


void
gtk_grab_add (GtkWidget *widget)
{
  g_function_enter ("gtk_grab_add");

  /* Place the grab on the front of the list of grabs.
   */
  grabs = g_list_prepend (grabs, widget);

  g_function_leave ("gtk_grab_add");
}

void
gtk_grab_remove (GtkWidget *widget)
{
  g_function_enter ("gtk_grab_remove");

  /* Remove the grab from the list of grabs.
   * Note: the grab being removed may be in
   *  the middle of the list.
   */
  grabs = g_list_remove (grabs, widget);

  g_function_leave ("gtk_grab_remove");
}

gint
gtk_timeout_add (guint32     interval,
		 GtkFunction function,
		 gpointer    data)
{
  static gint timeout_tag = 1;

  GList *temp_list;
  GList *new_list;
  GtkTimeoutFunction *timeoutf;
  GtkTimeoutFunction *temp;

  g_function_enter ("gtk_timeout_add");

  /* Create a new timeout function structure.
   * The start time is the current time.
   */
  timeoutf = g_new (GtkTimeoutFunction, 1);
  timeoutf->tag = timeout_tag++;
  timeoutf->start = gdk_time_get ();
  timeoutf->interval = interval;
  timeoutf->originterval = interval;
  timeoutf->expired = FALSE;
  timeoutf->function = function;
  timeoutf->data = data;

  /* Insert the timeout function appropriately.
   * Appropriately meaning sort it into the list
   *  of timeout functions.
   */
  temp_list = timeout_functions;
  while (temp_list)
    {
      temp = temp_list->data;
      if (timeoutf->interval < temp->interval)
	{
	  new_list = g_list_alloc ();
	  new_list->data = timeoutf;
	  new_list->next = temp_list;
	  new_list->prev = temp_list->prev;
	  if (temp_list->prev)
	    temp_list->prev->next = new_list;
	  temp_list->prev = new_list;

	  if (temp_list == timeout_functions)
	    {
	      timeout_functions = new_list;

	      /* Set the "gdk" timer. We only really need to do this if the
	       *  new timeout function was placed on the front of the timeout
	       *  functions list. (This is the case if we got here).
	       */
	      gtk_handle_timer ();
	    }

	  goto done;
	}

      temp_list = temp_list->next;
    }

  timeout_functions = g_list_append (timeout_functions, timeoutf);

 done:
  g_function_leave ("gtk_timeout_add");
  return timeoutf->tag;
}

void
gtk_timeout_remove (tag)
     gint tag;
{
  GList *temp_list;
  GtkTimeoutFunction *timeoutf;

  g_function_enter ("gtk_timeout_remove");

  /* Remove a timeout function.
   * (Which, basically, involves searching the
   *  list for the tag).
   */

  temp_list = timeout_functions;
  while (temp_list)
    {
      timeoutf = temp_list->data;

      if (timeoutf->tag == tag)
	{
	  timeout_functions = g_list_remove_link (timeout_functions, temp_list);
	  g_list_free (temp_list);
	  g_free (timeoutf);
	  break;
	}

      temp_list = temp_list->next;
    }

  g_function_leave ("gtk_timeout_remove");
}

void
gtk_get_current_event (GdkEvent *event)
{
  g_function_enter ("gtk_get_current_event");

  g_assert (event != NULL);

  *event = current_event;

  g_function_leave ("gtk_get_current_event");
}

GtkWidget*
gtk_get_event_widget (GdkEvent *event)
{
  GtkWidget *widget;

  g_function_enter ("gtk_get_event_widget");

  gdk_window_get_user_data (event->any.window, (void**) &widget);

  g_function_leave ("gtk_get_event_widget");
  return widget;
}

GdkVisual*
gtk_peek_visual ()
{
  GdkVisual *return_val;

  g_function_enter ("gtk_peek_visual");

  if (visuals)
    return_val = visuals->data;
  else
    return_val = gtk_visual;

  g_function_leave ("gtk_peek_visual");
  return return_val;
}

GdkColormap*
gtk_peek_colormap ()
{
  GdkColormap *return_val;

  g_function_enter ("gtk_peek_colormap");

  if (colormaps)
    return_val = colormaps->data;
  else
    return_val = gtk_colormap;

  g_function_leave ("gtk_peek_colormap");
  return return_val;
}

GtkStyle*
gtk_peek_style ()
{
  GtkStyle *return_val;

  g_function_enter ("gtk_peek_style");

  if (styles)
    return_val = styles->data;
  else
    return_val = gtk_root->widget.style;

  g_function_leave ("gtk_peek_style");
  return return_val;
}

void
gtk_push_visual (GdkVisual *visual)
{
  g_function_enter ("gtk_push_visual");

  visuals = g_list_prepend (visuals, visual);

  g_function_leave ("gtk_push_visual");
}

void
gtk_push_colormap (GdkColormap *colormap)
{
  g_function_enter ("gtk_push_colormap");

  colormaps = g_list_prepend (colormaps, colormap);

  g_function_leave ("gtk_push_colormap");
}

void
gtk_push_style (GtkStyle *style)
{
  g_function_enter ("gtk_push_style");

  styles = g_list_prepend (styles, style);

  g_function_leave ("gtk_push_style");
}

void
gtk_pop_visual ()
{
  GList *list;

  g_function_enter ("gtk_pop_visual");

  if (visuals)
    {
      list = visuals;
      visuals = visuals->next;
      if (visuals)
	visuals->prev = NULL;

      list->next = NULL;
      g_list_free (list);
    }

  g_function_leave ("gtk_pop_visual");
}

void
gtk_pop_colormap ()
{
  GList *list;

  g_function_enter ("gtk_pop_colormap");

  if (colormaps)
    {
      list = colormaps;
      colormaps = colormaps->next;
      if (colormaps)
	colormaps->prev = NULL;

      list->next = NULL;
      g_list_free (list);
    }

  g_function_leave ("gtk_pop_colormap");
}

void
gtk_pop_style ()
{
  GList *list;

  g_function_enter ("gtk_pop_style");

  if (styles)
    {
      list = styles;
      styles = styles->next;
      if (styles)
	styles->prev = NULL;

      list->next = NULL;
      g_list_free (list);
    }

  g_function_leave ("gtk_pop_style");
}


static void
gtk_handle_foreground (char *arg)
{
  g_function_enter ("gtk_handle_foreground");

  g_assert (arg != NULL);
  g_message ("foreground: %s", arg);

  gdk_color_parse (arg, &gtk_default_foreground);

  g_function_leave ("gtk_handle_foreground");
}

static void
gtk_handle_background (char *arg)
{
  g_function_enter ("gtk_handle_background");

  g_assert (arg != NULL);
  g_message ("background: %s", arg);

  gdk_color_parse (arg, &gtk_default_background);

  g_function_leave ("gtk_handle_background");
}

static void
gtk_handle_shadow_thickness (char *arg)
{
  g_function_enter ("gtk_handle_shadow_thickness");

  g_assert (arg != NULL);
  g_message ("background: %s", arg);

  gtk_default_shadow_thickness = atoi (arg);
  if (gtk_default_shadow_thickness < 0)
    gtk_default_shadow_thickness = 2;

  g_function_leave ("gtk_handle_shadow_thickness");
}

static void
gtk_exit_func ()
{
  g_function_enter ("gtk_exit_func");

  if (initialized)
    gtk_exit (0);

  g_function_leave ("gtk_exit_func");
}

static void
gtk_handle_timeouts ()
{
  guint32 the_time;
  GList *temp_list;
  GList *temp_list2;
  GtkTimeoutFunction *timeoutf;
  GtkTimeoutFunction *timeoutf2;
  GtkTimeoutFunction temp_timeoutf;
  gint i, j, length;

  g_function_enter ("gtk_handle_timeouts");

  if (timeout_functions)
    {
      the_time = gdk_time_get ();

      temp_list = timeout_functions;
      while (temp_list)
	{
	  timeoutf = temp_list->data;

	  if (timeoutf->interval <= (the_time - timeoutf->start))
	    {
	      timeoutf->interval = 0;
	      timeoutf->expired = TRUE;
	    }
	  else
	    timeoutf->interval -= (the_time - timeoutf->start);
	  timeoutf->start = the_time;

	  temp_list = temp_list->next;
	}

      temp_list = timeout_functions;
      while (temp_list)
	{
	  timeoutf = temp_list->data;

	  if (timeoutf->expired)
	    {
	      if (((* timeoutf->function) (timeoutf->data)) == FALSE)
		{
		  temp_list2 = temp_list;
		  temp_list = temp_list->next;

		  if (temp_list2->prev)
		    temp_list2->prev->next = temp_list2->next;
		  if (temp_list2->next)
		    temp_list2->next->prev = temp_list2->prev;

		  if (temp_list2 == timeout_functions)
		    timeout_functions = timeout_functions->next;

		  temp_list2->next = NULL;
		  temp_list2->prev = NULL;
		  g_list_free (temp_list2);
		  g_free (timeoutf);
		}
	      else
		{
		  timeoutf->expired = FALSE;
		  timeoutf->interval = timeoutf->originterval;
		  temp_list = temp_list->next;
		}
	    }
	  else
	    break;
	}

      length = g_list_length (timeout_functions);
      for (i = 1; i < length; i++)
	{
	  j = i;
	  temp_list = g_list_nth (timeout_functions, j);
	  timeoutf = temp_list->data;

	  while (j > 0)
	    {
	      timeoutf2 = temp_list->prev->data;
	      if (timeoutf->interval < timeoutf2->interval)
		{
		  temp_timeoutf = *timeoutf;
		  *timeoutf = *timeoutf2;
		  *timeoutf2 = temp_timeoutf;
		  j--;
		}
	      else
		break;
	    }
	}
    }

  g_function_leave ("gtk_handle_timeouts");
}

static void
gtk_handle_timer ()
{
  GtkTimeoutFunction *timeoutf;

  g_function_enter ("gtk_handle_timers");

  if (timeout_functions)
    {
      timeoutf = timeout_functions->data;
      gdk_timer_set (timeoutf->interval);
    }
  else
    gdk_timer_set (0);

  g_function_leave ("gtk_handle_timers");
}

static void
gtk_propogate_event (GtkWidget *widget,
		     GdkEvent  *event)
{
  guint16 window_type;

  g_function_enter ("gtk_propogate_event");

  g_assert (widget != NULL);
  g_assert (event != NULL);

  if ((event->type == GDK_KEY_PRESS) ||
      (event->type == GDK_KEY_RELEASE))
    {
      /* Only send key events to window widgets.
       *  The window widget will in turn pass the
       *  key event on to the currently focused widget
       *  for that window.
       */
      window_type = gtk_get_window_type ();

      while (widget && (widget->type != window_type))
	widget = (GtkWidget*) widget->parent;

      if (widget && (widget->type == window_type))
	gtk_widget_event (widget, event);
    }
  else
    {
      /* Other events get propogated up the widget tree
       *  so that parents can see the button and motion
       *  events of the children.
       */
      while (widget)
	{
	  if (gtk_widget_event (widget, event))
	    break;
	  widget = (GtkWidget*) widget->parent;
	}
    }

  g_function_leave ("gtk_propogate_event");
}


static void
gtk_root_destroy (GtkWidget *widget)
{
}

static void
gtk_root_show (GtkWidget *widget)
{
}

static void
gtk_root_hide (GtkWidget *widget)
{
}
static void
gtk_root_map (GtkWidget *widget)
{
}

static void
gtk_root_unmap (GtkWidget *widget)
{
}

static void
gtk_root_realize (GtkWidget *widget)
{
}

static void
gtk_root_draw (GtkWidget    *widget,
	       GdkRectangle *area,
	       gint          is_expose)
{
}

static void
gtk_root_draw_focus (GtkWidget *widget)
{
}

static gint
gtk_root_event (GtkWidget *widget,
		GdkEvent  *event)
{
  return TRUE;
}

static void
gtk_root_size_request (GtkWidget      *widget,
		       GtkRequisition *requisition)
{
}

static void
gtk_root_size_allocate (GtkWidget     *widget,
			GtkAllocation *allocation)
{
}

static gint
gtk_root_is_child (GtkWidget *widget,
		   GtkWidget *child)
{
  return FALSE;
}

static gint
gtk_root_locate (GtkWidget  *widget,
		 GtkWidget **child,
		 gint        x,
		 gint        y)
{
  return FALSE;
}
static void
gtk_root_activate (GtkWidget *widget)
{
}

static void
gtk_root_set_state (GtkWidget    *widget,
		    GtkStateType  state)
{
}

static gint
gtk_root_install_accelerator (GtkWidget *widget,
			      gchar      accelerator_key,
			      guint8     accelerator_mods)
{
  return FALSE;
}

static void
gtk_root_remove_accelerator (GtkWidget *widget)
{
}

static void
gtk_root_add (GtkContainer *container,
	      GtkWidget    *widget)
{
}

static void
gtk_root_remove (GtkContainer *container,
		 GtkWidget    *widget)
{
}

static void
gtk_root_need_resize (GtkContainer *container,
		      GtkWidget    *widget)
{
}

static void
gtk_root_focus_advance (GtkContainer      *container,
			GtkWidget        **child,
			GtkDirectionType   direction)
{
}
