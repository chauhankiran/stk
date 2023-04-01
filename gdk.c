/* GDK - The General Drawing Kit (written for the GIMP)
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

/*
 * File:         gdk.c
 * Author:       Peter Mattis
 * Description:  This module contains the initialization, exit
 *               and event routines. It handles certain signals
 *               in order that the library may die gracefully.
 */
#include <ctype.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define XLIB_ILLEGAL_ACCESS
#include <X11/Xatom.h>
#include <X11/Xlib.h>
#include <X11/Xos.h>
#include <X11/Xutil.h>
#include "gdk.h"
#include "gdkprivate.h"

#ifndef X_GETTIMEOFDAY
#define X_GETTIMEOFDAY(tv)  gettimeofday (tv, NULL)
#endif /* X_GETTIMEOFDAY */

#define TOKEN_LEFT_PAREN       1
#define TOKEN_RIGHT_PAREN      2
#define TOKEN_KEY_PRESS        3
#define TOKEN_KEY_RELEASE      4
#define TOKEN_BUTTON_PRESS     5
#define TOKEN_BUTTON_RELEASE   6
#define TOKEN_MOTION_NOTIFY    7
#define TOKEN_CREATE_NOTIFY    8
#define TOKEN_MAP_NOTIFY       9
#define TOKEN_REPARENT_NOTIFY  10
#define TOKEN_CONFIGURE_NOTIFY 11
#define TOKEN_WINDOW           12
#define TOKEN_TIME             13
#define TOKEN_XY               14
#define TOKEN_STATE            15
#define TOKEN_KEYCODE          16
#define TOKEN_BUTTON           17
#define TOKEN_IS_HINT          18
#define TOKEN_SYMBOL           19

#define DOUBLE_CLICK_TIME      250
#define TRIPLE_CLICK_TIME      500

#define RETSIGTYPE void

typedef struct _GdkInput GdkInput;
struct _GdkInput {
  gint tag;
  gint source;
  GdkInputCondition condition;
  GdkInputFunction function;
  gpointer data;
};

/* Private function declarations
 */
static gint gdk_event_wait (void);
static void gdk_synthesize_click (GdkEvent *event, gint nclicks);
static void gdk_event_record (Display *display, XEvent *event);

static void gdk_event_display (char *str, XEvent *event);
static void gdk_event_get_xevent (Display *display, XEvent *event);
static void gdk_event_send_xevent (Display *display, XEvent *event);
static void gdk_event_playback (Display *display, XEvent *event);
static void gdk_event_playback_read (void);
static gint gdk_event_playback_get_next_token ();
static gint gdk_event_playback_peek_next_token ();
static gint gdk_event_playback_parse (gchar *buf, XEvent *event);
static gint gdk_event_playback_parse_token (gchar *buf);
static gint gdk_event_playback_parse_key_press (XEvent *event);
static gint gdk_event_playback_parse_key_release (XEvent *event);
static gint gdk_event_playback_parse_button_press (XEvent *event);
static gint gdk_event_playback_parse_button_release (XEvent *event);
static gint gdk_event_playback_parse_motion_notify (XEvent *event);
static gint gdk_event_playback_parse_create_notify (XEvent *event);
static gint gdk_event_playback_parse_map_notify (XEvent *event);
static gint gdk_event_playback_parse_reparent_notify (XEvent *event);
static gint gdk_event_playback_parse_configure_notify (XEvent *event);
static gint gdk_event_playback_parse_window (XEvent *event);
static gint gdk_event_playback_parse_time (XEvent *event);
static gint gdk_event_playback_parse_xy (XEvent *event);
static gint gdk_event_playback_parse_state (XEvent *event);
static gint gdk_event_playback_parse_keycode (XEvent *event);
static gint gdk_event_playback_parse_button (XEvent *event);
static gint gdk_event_playback_parse_is_hint (XEvent *event);

static void gdk_exit_func (void);
static int  gdk_x_error (Display *display, XErrorEvent *error);
static int  gdk_x_io_error (Display *display);
static RETSIGTYPE gdk_signal (int signum);

/* Private variable declarations
 */
static int initialized = 0;                         /* 1 if the library is initialized,
						     * 0 otherwise.
						     */

static int connection_number = 0;                   /* The file descriptor number of our
						     *  connection to the X server. This
						     *  is used so that we may determine
						     *  when events are pending by using
						     *  the "select" system call.
						     */

static gint received_destroy_notify = FALSE;        /* Did we just receive a destroy notify
						     *  event? If so, we need to actually
						     *  destroy the window which received
						     *  it now.
						     */

static GdkWindow *window_to_destroy = NULL;         /* If we previously received a destroy
						     *  notify event then this is the window
						     *  which received that event.
						     */

static struct timeval start;                        /* The time at which the library was
						     *  last initialized.
						     */

static struct timeval timer;                        /* Timeout interval to use in the call
						     *  to "select". This is used in
						     *  conjunction with "timerp" to create
						     *  a maximum time to wait for an event
						     *  to arrive.
						     */

static struct timeval *timerp;                      /* The actual timer passed to "select"
						     *  This may be NULL, in which case
						     *  "select" will block until an event
						     *  arrives.
						     */

static guint32 timer_val;                           /* The timeout length as specified by
						     *  the user in milliseconds.
						     */

static GList *inputs;                               /* A list of the input file descriptors
						     *  that we care about. Each list node
						     *  contains a GdkInput struct that describes
						     *  when we are interested in the specified
						     *  file descriptor. That is, when it is
						     *  available for read, write or has an
						     *  exception pending.
						     */

static guint32 button_click_time[2];                /* The last 2 button click times. Used
						     *  to determine if the latest button click
						     *  is part of a double or triple click.
						     */

static GdkWindow *button_window[2];                 /* The last 2 windows to receive button presses.
						     *  Also used to determine if the latest button
						     *  click is part of a double or triple click.
						     */

static gint button_number[2];                       /* The last 2 buttons to be pressed.
						     */

static GList *putback_events = NULL;

static gulong base_id;
static gchar *record_filename = NULL;
static gchar *playback_filename = NULL;
static FILE  *record_fp = NULL;
static FILE  *playback_fp = NULL;
static gint do_playback = FALSE;

static GList *playback_events = NULL;
static gint wait_for_send_event = FALSE;
static gint have_send_event = FALSE;
static gint wait_for_event = FALSE;
static guint32 next_event_time;
static gchar token_str[128];
static gint cur_token;
static gint next_token;

static gint autorepeat;

/*
 *--------------------------------------------------------------
 * gdk_init
 *
 *   Initialize the library for use.
 *
 * Arguments:
 *   "argc" is the number of arguments.
 *   "argv" is an array of strings.
 *
 * Results:
 *   "argc" and "argv" are modified to reflect any arguments
 *   which were not handled. (Such arguments should either
 *   be handled by the application or dismissed).
 *
 * Side effects:
 *   The library is initialized.
 *
 *--------------------------------------------------------------
 */

void
gdk_init (int *argc, char ***argv) {
  XKeyboardState keyboard_state;
  int synchronize;
  int i, j, k;

  g_function_enter ("gdk_init");

  X_GETTIMEOFDAY (&start);

  gdk_progname = (*argv)[0];
  gdk_display_name = NULL;

  signal (SIGHUP, gdk_signal);
  signal (SIGINT, gdk_signal);
  signal (SIGQUIT, gdk_signal);
  signal (SIGBUS, gdk_signal);
  signal (SIGSEGV, gdk_signal);
  signal (SIGPIPE, gdk_signal);
  signal (SIGTERM, gdk_signal);

  XSetErrorHandler (gdk_x_error);
  XSetIOErrorHandler (gdk_x_io_error);

  synchronize = FALSE;

  for (i = 1; i < *argc;) {
      if (strcmp ("-display", (*argv)[i]) == 0) {
        (*argv)[i] = NULL;

        if ((i + 1) < *argc) {
  	      gdk_display_name = g_strdup ((*argv)[i + 1]);
  	      (*argv)[i + 1] = NULL;
  	      i += 1;
        }
      } else if (strcmp ("-record", (*argv)[i]) == 0) {
        (*argv)[i] = NULL;

        if ((i + 1) < *argc) {
          if (record_filename) {
		        g_free (record_filename);
          }
	      if (record_fp) {
		      fclose (record_fp);
        }
	      record_filename = g_strdup ((*argv)[i + 1]);
	      record_fp = fopen (record_filename, "w");
	      if (!record_fp) {
		      g_error ("unable to open file \"%s\" for writing", record_filename);
        }
	      (*argv)[i + 1] = NULL;
	      i += 1;
	    }
    } else if (strcmp ("-playback", (*argv)[i]) == 0) {
      (*argv)[i] = NULL;

      if ((i + 1) < *argc) {
	      if (playback_filename) {
		      g_free (playback_filename);
        }
	      if (playback_fp) {
		      fclose (playback_fp);
        }
	      playback_filename = g_strdup ((*argv)[i + 1]);
	      playback_fp = fopen (playback_filename, "r");
	      if (!playback_fp) {
		      g_error ("unable to open file \"%s\" for reading", playback_filename);
        }
	      do_playback = TRUE;
	      (*argv)[i + 1] = NULL;
	      i += 1;
	    }
    } else if (strcmp ("-motion-events", (*argv)[i]) == 0) {
  	  (*argv)[i] = NULL;
  	  gdk_motion_events = TRUE;
  	} else if (strcmp ("-sync", (*argv)[i]) == 0) {
      (*argv)[i] = NULL;
      synchronize = TRUE;
    }
    i += 1;
  }

  for (i = 1; i < *argc; i++) {
    for (k = i; k < *argc; k++)
      if ((*argv)[k] != NULL)
    break;

    if (k > i) {
      k -= i;
  	  for (j = i + k; j < *argc; j++)
        (*argv)[j-k] = (*argv)[j];
      *argc -= k;
  	}
  }

  gdk_display = XOpenDisplay (gdk_display_name);
  if (!gdk_display) {
    g_error ("cannot open display: %s", XDisplayName (gdk_display_name));
  }

  /* This is really crappy. We have to look into the display structure
   *  to find the base resource id. This is only needed for recording
   *  and playback of events.
   */
  // Commented out following code as it throws error.
  // base_id = RESOURCE_BASE;
  // if (gdk_show_events)
  //   g_message ("base id: %ul", base_id);

  connection_number = ConnectionNumber (gdk_display);
  if (gdk_debug_level >= 1) {
    g_message ("connection number: %d", connection_number);
  }

  if (synchronize) {
    XSynchronize (gdk_display, True);
  }

  gdk_screen = DefaultScreen (gdk_display);
  gdk_root_window = RootWindow (gdk_display, gdk_screen);

  gdk_wm_delete_window = XInternAtom (gdk_display, "WM_DELETE_WINDOW", True);
  gdk_wm_take_focus = XInternAtom (gdk_display, "WM_TAKE_FOCUS", True);
  gdk_wm_protocols = XInternAtom (gdk_display, "WM_PROTOCOLS", True);

  XGetKeyboardControl (gdk_display, &keyboard_state);
  autorepeat = keyboard_state.global_auto_repeat;

  timer.tv_sec = 0;
  timer.tv_usec = 0;
  timerp = NULL;

  button_click_time[0] = 0;
  button_click_time[1] = 0;
  button_window[0] = NULL;
  button_window[1] = NULL;
  button_number[0] = -1;
  button_number[1] = -1;

  if (ATEXIT (gdk_exit_func)) {
    g_warning ("unable to register exit function");
  }

  gdk_visual_init ();
  gdk_window_init ();

  initialized = 1;

  g_function_leave ("gdk_init");
}

/*
 *--------------------------------------------------------------
 * gdk_exit
 *
 *   Restores the library to an un-itialized state and exits
 *   the program using the "exit" system call.
 *
 * Arguments:
 *   "errorcode" is the error value to pass to "exit".
 *
 * Results:
 *   Allocated structures are freed and the program exits
 *   cleanly.
 *
 * Side effects:
 *
 *--------------------------------------------------------------
 */

void gdk_exit (int errorcode) {
  g_function_enter ("gdk_exit");

  if (initialized) {
    gdk_image_exit ();

    if (autorepeat) {
      XAutoRepeatOn (gdk_display);
    } else {
      XAutoRepeatOff (gdk_display);
    }

    XCloseDisplay (gdk_display);
    initialized = 0;

    if (record_fp) {
      fclose (record_fp);
    }
    if (playback_fp) {
      fclose (playback_fp);
    }
  }

  exit (errorcode);
  g_function_leave ("gdk_exit");
}

/*
 *--------------------------------------------------------------
 * gdk_events_pending
 *
 *   Returns the number of events pending on the queue.
 *   These events have already been read from the server
 *   connection.
 *
 * Arguments:
 *
 * Results:
 *   Returns the number of events on XLib's event queue.
 *
 * Side effects:
 *
 *--------------------------------------------------------------
 */

gint gdk_events_pending() {
  return XPending (gdk_display);
}

/*
 *--------------------------------------------------------------
 * gdk_event_get
 *
 *   Gets the next event.
 *
 * Arguments:
 *   "event" is used to hold the received event.
 *   If "event" is NULL an event is received as normal
 *   however it is not placed in "event" (and thus no
 *   error occurs).
 *
 * Results:
 *   Returns TRUE if an event was received that we care about
 *   and FALSE otherwise. This function will also return
 *   before an event is received if the timeout interval
 *   runs out.
 *
 * Side effects:
 *
 *--------------------------------------------------------------
 */

gint gdk_event_get (GdkEvent *event) {
  GdkWindow *window;
  GdkEvent *temp_event;
  GList *temp_list;
  XEvent xevent;
  XComposeStatus compose;
  int charcount;
  char buf[16];
  gint return_val;

  g_function_enter ("gdk_event_get");

  /* If the last event we received was a destroy notify
   *  event then we will actually destroy the "gdk" data
   *  structures now. We don't want to destroy them at the
   *  time of receiving the event since the main program
   *  may try to access them and may need to destroy user
   *  data that has been attached to the window
   */
  if (received_destroy_notify) {
    if (gdk_show_events) {
      g_message ("destroying window:\twindow: %d", ((GdkWindowPrivate*) window_to_destroy)->xwindow - base_id);
    }

    gdk_window_real_destroy (window_to_destroy);
    received_destroy_notify = FALSE;
    window_to_destroy = NULL;
  }

  /* Initially we haven't received an event and want to
   *  return FALSE. If "event" is non-NULL, then initialize
   *  it to the nothing event.
   */
  return_val = FALSE;
  if (event) {
    event->any.type = GDK_NOTHING;
    event->any.window = NULL;
    event->any.send_event = FALSE;
  }

  if (putback_events) {
    temp_event = putback_events->data;
    *event = *temp_event;

    temp_list = putback_events;
    putback_events = putback_events->next;
    if (putback_events) {
      putback_events->prev = NULL;
    }

    temp_list->next = NULL;
    temp_list->prev = NULL;
    g_list_free (temp_list);
    g_free (temp_event);

    return_val = TRUE;
  } else if (gdk_event_wait ()) {
    /* Wait for an event to occur or the timeout to elapse.
     * If an event occurs "gdk_event_wait" will return TRUE.
     *  If the timeout elapses "gdk_event_wait" will return
     *  FALSE.
     */
    
    /* If we get here we can rest assurred that an event
     *  has occurred. Read it.
     */
    gdk_event_playback (gdk_display, &xevent);

    /* Find the GdkWindow that this event occurred in.
     * All events occur in some GdkWindow (otherwise, why
     *  would we be receiving them). It really is an error
     *  to receive an event for which we cannot find the
     *  corresponding GdkWindow.
     */
    window = gdk_window_table_lookup (xevent.xany.window);
    event->any.send_event = xevent.xany.send_event;

    /* If "event" non-NULL.
     */
    if (event) {
  	  /* We do a "manual" conversion of the XEvent to a
  	   *  GdkEvent. The structures are mostly the same so
  	   *  the conversion is fairly straightforward. We also
  	   *  optionally print debugging info regarding events
  	   *  received.
  	   */
      switch (xevent.type) {
  	    case KeyPress:
  	      /* Lookup the string corresponding to the given keysym.
  	       */
  	      charcount = XLookupString (&xevent.xkey, buf, 16,
  					(KeySym*) &event->key.keyval,
  					&compose);

  	      /* Print debugging info. */
	        if (gdk_show_events) {
		        g_message ("key press:\t\twindow: %d  key: %12s  %d", xevent.xkey.window - base_id, XKeysymToString (event->key.keyval), event->key.keyval);
          }

  	      event->key.type = GDK_KEY_PRESS;
  	      event->key.window = window;
  	      event->key.time = xevent.xkey.time;
  	      event->key.state = (GdkModifierType) xevent.xkey.state;

  	      return_val = !((GdkWindowPrivate*) window)->destroyed;
	        break;
	      case KeyRelease:
	        /* Lookup the string corresponding to the given keysym. */
	        charcount = XLookupString (&xevent.xkey, buf, 16, (KeySym*) &event->key.keyval, &compose);

          /* Print debugging info. */
	        if (gdk_show_events) {
		        g_message ("key release:\t\twindow: %d  key: %12s  %d", xevent.xkey.window - base_id, XKeysymToString (event->key.keyval), event->key.keyval);
          }

  	      event->key.type = GDK_KEY_RELEASE;
  	      event->key.window = window;
  	      event->key.time = xevent.xkey.time;
  	      event->key.state = (GdkModifierType) xevent.xkey.state;

  	      return_val = !((GdkWindowPrivate*) window)->destroyed;
	        break;
	    case ButtonPress:
	      /* Print debugging info. */
	      if (gdk_show_events) {
		      g_message ("button press:\t\twindow: %d  x,y: %d %d  button: %d", xevent.xbutton.window - base_id, xevent.xbutton.x, xevent.xbutton.y, xevent.xbutton.button);
        }

	      event->button.type = GDK_BUTTON_PRESS;
	      event->button.window = window;
	      event->button.time = xevent.xbutton.time;
	      event->button.x = xevent.xbutton.x;
	      event->button.y = xevent.xbutton.y;
	      event->button.state = (GdkModifierType) xevent.xbutton.state;
	      event->button.button = xevent.xbutton.button;

	      if ((event->button.time < (button_click_time[1] + TRIPLE_CLICK_TIME)) && (event->button.window == button_window[1]) && (event->button.button == button_number[1])) {
		    gdk_synthesize_click (event, 3);

    		  button_click_time[1] = 0;
    		  button_click_time[0] = 0;
    		  button_window[1] = NULL;
    		  button_window[0] = 0;
    		  button_number[1] = -1;
    		  button_number[0] = -1;
    		} else if ((event->button.time < (button_click_time[0] + DOUBLE_CLICK_TIME)) && (event->button.window == button_window[0]) && (event->button.button == button_number[0])) {
		      gdk_synthesize_click (event, 2);

    		  button_click_time[1] = button_click_time[0];
    		  button_click_time[0] = event->button.time;
    		  button_window[1] = button_window[0];
    		  button_window[0] = event->button.window;
    		  button_number[1] = button_number[0];
    		  button_number[0] = event->button.button;
		    } else {
    		  button_click_time[1] = 0;
    		  button_click_time[0] = event->button.time;
    		  button_window[1] = NULL;
    		  button_window[0] = event->button.window;
    		  button_number[1] = -1;
    		  button_number[0] = event->button.button;
    		}

	      return_val = !((GdkWindowPrivate*) window)->destroyed;
	      break;

	    case ButtonRelease:
	      /* Print debugging info. */
	      if (gdk_show_events) {
		      g_message ("button release:\twindow: %d  x,y: %d %d  button: %d", xevent.xbutton.window - base_id, xevent.xbutton.x, xevent.xbutton.y, xevent.xbutton.button);
        }

	      event->button.type = GDK_BUTTON_RELEASE;
	      event->button.window = window;
	      event->button.time = xevent.xbutton.time;
	      event->button.x = xevent.xbutton.x;
	      event->button.y = xevent.xbutton.y;
	      event->button.state = (GdkModifierType) xevent.xbutton.state;
	      event->button.button = xevent.xbutton.button;

	      return_val = !((GdkWindowPrivate*) window)->destroyed;
	      break;
	    case MotionNotify:
	      /* Print debugging info. */
	      if (gdk_show_events) {
		      g_message ("motion notify:\t\twindow: %d  x,y: %d %d  hint: %s", xevent.xmotion.window - base_id, xevent.xmotion.x, xevent.xmotion.y, (xevent.xmotion.is_hint) ? "true" : "false");
        }

	      event->motion.type = GDK_MOTION_NOTIFY;
	      event->motion.window = window;
	      event->motion.time = xevent.xmotion.time;
	      event->motion.x = xevent.xmotion.x;
	      event->motion.y = xevent.xmotion.y;
	      event->motion.state = (GdkModifierType) xevent.xmotion.state;
	      event->motion.is_hint = xevent.xmotion.is_hint;

	      return_val = !((GdkWindowPrivate*) window)->destroyed;
	      break;
	    case EnterNotify:
	      /* Print debugging info. */
	      if (gdk_show_events)
		g_message ("enter notify:\t\twindow: %d  detail: %d",
			     xevent.xcrossing.window - base_id,
			     xevent.xcrossing.detail);

	      event->crossing.type = GDK_ENTER_NOTIFY;
	      event->crossing.window = window;

	      /* If the subwindow field of the XEvent is non-NULL, then
	       *  lookup the corresponding GdkWindow.
	       */
	      if (xevent.xcrossing.subwindow != None)
		event->crossing.subwindow = gdk_window_table_lookup (xevent.xcrossing.subwindow);
	      else
		event->crossing.subwindow = NULL;

	      /* Translate the crossing detail into Gdk terms.
	       */
	      switch (xevent.xcrossing.detail)
		{
		case NotifyInferior:
		  event->crossing.detail = GDK_NOTIFY_INFERIOR;
		  break;
		case NotifyAncestor:
		  event->crossing.detail = GDK_NOTIFY_ANCESTOR;
		  break;
		case NotifyVirtual:
		  event->crossing.detail = GDK_NOTIFY_VIRTUAL;
		  break;
		case NotifyNonlinear:
		  event->crossing.detail = GDK_NOTIFY_NONLINEAR;
		  break;
		case NotifyNonlinearVirtual:
		  event->crossing.detail = GDK_NOTIFY_NONLINEAR_VIRTUAL;
		  break;
		default:
		  event->crossing.detail = GDK_NOTIFY_UNKNOWN;
		  break;
		}

	      return_val = !((GdkWindowPrivate*) window)->destroyed;
	      break;

	    case LeaveNotify:
	      /* Print debugging info.
	       */
	      if (gdk_show_events)
		g_message ("leave notify:\t\twindow: %d  detail: %d",
			     xevent.xcrossing.window - base_id,
			     xevent.xcrossing.detail);

	      event->crossing.type = GDK_LEAVE_NOTIFY;
	      event->crossing.window = window;

	      /* Translate the crossing detail into Gdk terms.
	       */
	      switch (xevent.xcrossing.detail)
		{
		case NotifyInferior:
		  event->crossing.detail = GDK_NOTIFY_INFERIOR;
		  break;
		case NotifyAncestor:
		  event->crossing.detail = GDK_NOTIFY_ANCESTOR;
		  break;
		case NotifyVirtual:
		  event->crossing.detail = GDK_NOTIFY_VIRTUAL;
		  break;
		case NotifyNonlinear:
		  event->crossing.detail = GDK_NOTIFY_NONLINEAR;
		  break;
		case NotifyNonlinearVirtual:
		  event->crossing.detail = GDK_NOTIFY_NONLINEAR_VIRTUAL;
		  break;
		default:
		  event->crossing.detail = GDK_NOTIFY_UNKNOWN;
		  break;
		}

	      return_val = !((GdkWindowPrivate*) window)->destroyed;
	      break;

	    case FocusIn:
	      /* Print debugging info.
	       */
	      if (gdk_show_events)
		g_message ("focus in:\t\twindow: %d",
			     xevent.xfocus.window - base_id);

	      event->focus_change.type = GDK_FOCUS_CHANGE;
	      event->focus_change.window = window;
	      event->focus_change.in = TRUE;

	      return_val = !((GdkWindowPrivate*) window)->destroyed;
	      break;

	    case FocusOut:
	      /* Print debugging info.
	       */
	      if (gdk_show_events)
		g_message ("focus out:\t\twindow: %d",
			     xevent.xfocus.window - base_id);

	      event->focus_change.type = GDK_FOCUS_CHANGE;
	      event->focus_change.window = window;
	      event->focus_change.in = FALSE;

	      return_val = !((GdkWindowPrivate*) window)->destroyed;
	      break;

	    case KeymapNotify:
	      /* Print debugging info.
	       */
	      if (gdk_show_events)
		g_message ("keymap notify");
	      break;

	    case Expose:
	      /* Print debugging info.
	       */
	      if (gdk_show_events)
		g_message ("expose:\t\twindow: %d  %d  x,y: %d %d  w,h: %d %d",
			     xevent.xexpose.window - base_id, xevent.xexpose.count,
			     xevent.xexpose.x, xevent.xexpose.y,
			     xevent.xexpose.width, xevent.xexpose.height);

	      event->expose.type = GDK_EXPOSE;
	      event->expose.window = window;
	      event->expose.area.x = xevent.xexpose.x;
	      event->expose.area.y = xevent.xexpose.y;
	      event->expose.area.width = xevent.xexpose.width;
	      event->expose.area.height = xevent.xexpose.height;

	      return_val = !((GdkWindowPrivate*) window)->destroyed;
	      break;

	    case GraphicsExpose:
	      /* Print debugging info.
	       */
	      if (gdk_show_events)
		g_message ("graphics expose:\tdrawable: %d",
			     xevent.xgraphicsexpose.drawable - base_id);

	      event->expose.type = GDK_EXPOSE;
	      event->expose.window = window;
	      event->expose.area.x = xevent.xgraphicsexpose.x;
	      event->expose.area.y = xevent.xgraphicsexpose.y;
	      event->expose.area.width = xevent.xgraphicsexpose.width;
	      event->expose.area.height = xevent.xgraphicsexpose.height;

	      return_val = !((GdkWindowPrivate*) window)->destroyed;
	      break;

	    case NoExpose:
	      /* Print debugging info.
	       */
	      if (gdk_show_events)
		g_message ("no expose:\t\tdrawable: %d",
			     xevent.xnoexpose.drawable - base_id);

	      /* Not currently handled */
	      break;

	    case VisibilityNotify:
	      /* Print debugging info.
	       */
	      if (gdk_show_events)
		switch (xevent.xvisibility.state)
		  {
		  case VisibilityFullyObscured:
		    g_message ("visibility notify:\twindow: %d  none",
				 xevent.xvisibility.window - base_id);
		    break;
		  case VisibilityPartiallyObscured:
		    g_message ("visibility notify:\twindow: %d  partial",
				 xevent.xvisibility.window - base_id);
		    break;
		  case VisibilityUnobscured:
		    g_message ("visibility notify:\twindow: %d  full",
				 xevent.xvisibility.window - base_id);
		    break;
		  }

	      /* Not currently handled */
	      break;

	    case CreateNotify:
	      /* Not currently handled */
	      break;

	    case DestroyNotify:
	      /* Print debugging info.
	       */
	      if (gdk_show_events)
		g_message ("destroy notify:\twindow: %d",
			     xevent.xdestroywindow.window - base_id);

	      event->any.type = GDK_DESTROY;
	      event->any.window = window;

	      /* Remeber which window received the destroy notify
	       *  event so that we can destroy our associated
	       *  data structures the next time the user asks
	       *  us for an event.
	       */
	      received_destroy_notify = TRUE;
	      window_to_destroy = window;

	      return_val = !((GdkWindowPrivate*) window)->destroyed;
	      break;

	    case UnmapNotify:
	      /* Print debugging info.
	       */
	      if (gdk_show_events)
		g_message ("unmap notify:\t\twindow: %d",
			     xevent.xmap.window - base_id);

	      event->any.type = GDK_UNMAP;
	      event->any.window = window;

	      return_val = !((GdkWindowPrivate*) window)->destroyed;
	      break;

	    case MapNotify:
	      /* Print debugging info.
	       */
	      if (gdk_show_events)
		g_message ("map notify:\t\twindow: %d",
			     xevent.xmap.window - base_id);

	      event->any.type = GDK_MAP;
	      event->any.window = window;

	      return_val = !((GdkWindowPrivate*) window)->destroyed;
	      break;

	    case ReparentNotify:
	      /* Print debugging info.
	       */
	      if (gdk_show_events)
		g_message ("reparent notify:\twindow: %d",
			     xevent.xreparent.window - base_id);

	      /* Not currently handled */
	      break;

	    case ConfigureNotify:
	      /* Print debugging info.
	       */
	      if (gdk_show_events)
		g_message ("configure notify:\twindow: %d  x,y: %d %d  w,h: %d %d",
			     xevent.xconfigure.window - base_id,
			     xevent.xconfigure.x, xevent.xconfigure.height,
			     xevent.xconfigure.width, xevent.xconfigure.height);

	      /* We only pass along ConfigureNotify events for toplevel
	       *  windows. (The ones which interact with the window
	       *  manager). We do this because the user expects that
	       *  resizing a window or moving a window happens immediately
	       *  and does not want to wait until some event occurs.
	       *  We hide this fact from the user and trust in the
	       *  queueing of events to correctly handle draw requests
	       *  that occur immediately after a child window resize.
	       */
	      if ((window->window_type != GDK_WINDOW_CHILD) &&
		  ((window->width != xevent.xconfigure.width) ||
		   (window->height != xevent.xconfigure.height)))
		{
		  /* Print debugging info.
		   */
		  if (gdk_show_events)
		    g_message ("resize notify:\t\twindow: %d  w,h: %d %d",
				 xevent.xconfigure.window - base_id,
				 xevent.xconfigure.width, xevent.xconfigure.height);

		  event->resize.type = GDK_RESIZE;
		  event->resize.window = window;
		  event->resize.width = xevent.xconfigure.width;
		  event->resize.height = xevent.xconfigure.height;

		  /* Set the width and height of the toplevel window.
		   */
		  window->width = xevent.xconfigure.width;
		  window->height = xevent.xconfigure.height;

		  return_val = !((GdkWindowPrivate*) window)->destroyed;
		}
	      break;

	    case PropertyNotify:
	      /* Print debugging info.
	       */
	      if (gdk_show_events)
		g_message ("property notify:\twindow: %d",
			     xevent.xproperty.window - base_id);

	      /* Not currently handled */
	      break;

	    case ColormapNotify:
	      /* Print debugging info.
	       */
	      if (gdk_show_events)
		g_message ("colormap notify:\twindow: %d",
			     xevent.xcolormap.window - base_id);

	      /* Not currently handled */
	      break;

	    case ClientMessage:
	      /* Print debugging info.
	       */
	      if (gdk_show_events)
		g_message ("client message:\twindow: %d",
			     xevent.xclient.window - base_id);

	      /* Client messages are the means of the window manager
	       *  communicating with a program. We'll first check to
	       *  see if this is really the window manager talking
	       *  to us.
	       */
	      if (xevent.xclient.message_type == gdk_wm_protocols)
		{
		  if (xevent.xclient.data.l[0] == gdk_wm_delete_window)
		    {
		      /* The delete window request specifies a window
		       *  to delete. We don't actually destroy the
		       *  window because "it is only a request". (The
		       *  window might contain vital data that the
		       *  program does not want destroyed). Instead
		       *  the event is passed along to the program,
		       *  which should then destroy the window.
		       */

		      /* Print debugging info.
		       */
		      if (gdk_show_events)
			g_message ("delete window:\t\twindow: %d",
				     xevent.xclient.window - base_id);

		      event->any.type = GDK_DELETE;
		      event->any.window = window;

		      return_val = !((GdkWindowPrivate*) window)->destroyed;
		    }
		  else if (xevent.xclient.data.l[0] == gdk_wm_take_focus)
		    {
		      /* Print debugging info.
		       */
		      if (gdk_show_events)
			g_message ("take focus:\t\twindow: %d",
				     xevent.xclient.window - base_id);

		      /* Not currently handled */
		    }
		}
	      break;

	    case MappingNotify:
	      /* Print debugging info.
	       */
	      if (gdk_show_events)
		g_message ("mapping notify");

	      /* Let XLib know that there is a new keyboard mapping.
	       */
	      XRefreshKeyboardMapping (&xevent.xmapping);
	      break;

	    default:
	      /* There shouldn't be any "unknown events".
	       */
	      g_message ("unknown event: %d", xevent.type);
	      break;
	    }
	}
    }

  g_function_leave ("gdk_event_get");
  return return_val;
}

void gdk_event_put (GdkEvent *event) {
  GdkEvent *new_event;

  g_function_enter ("gdk_event_put");

  g_assert (event != NULL);

  new_event = g_new (GdkEvent, 1);
  *new_event = *event;

  putback_events = g_list_prepend (putback_events, new_event);

  g_function_leave ("gdk_event_put");
}


void gdk_events_record (char *filename) {
  g_function_enter ("gdk_events_record");

  if (record_filename) {
    g_free (record_filename);
  }
  if (record_fp) {
    fclose (record_fp);
  }

  record_filename = g_strdup (filename);
  record_fp = fopen (record_filename, "w");
  if (!record_fp) {
    g_error ("unable to open file \"%s\" for writing", record_filename);
  }

  g_function_leave ("gdk_events_record");
}

void gdk_events_playback (char *filename) {
  g_function_enter ("gdk_events_playback");

  if (playback_filename) {
    g_free (playback_filename);
  }
  if (playback_fp) {
    fclose (playback_fp);
  }

  playback_filename = g_strdup (filename);
  playback_fp = fopen (playback_filename, "r");
  if (!playback_fp) {
    g_error ("unable to open file \"%s\" for reading", playback_filename);
  }
  do_playback = TRUE;

  g_function_leave ("gdk_events_playback");
}

void gdk_events_stop () {
  g_function_enter ("gdk_events_stop");

  if (record_filename) {
    g_free (record_filename);
  }
  if (record_fp) {
    fclose (record_fp);
  }

  if (playback_filename) {
    g_free (playback_filename);
  }
  if (playback_fp) {
    fclose (playback_fp);
  }

  record_filename = NULL;
  record_fp = NULL;
  playback_filename = NULL;
  playback_fp = NULL;
  do_playback = FALSE;

  g_function_leave ("gdk_events_stop");
}

/*
 *--------------------------------------------------------------
 * gdk_set_debug_level
 *
 *   Sets the debugging level.
 *
 * Arguments:
 *   "level" is the new debugging level.
 *
 * Results:
 *
 * Side effects:
 *   Other function calls to "gdk" use the debugging
 *   level to determine what kind of debugging information
 *   to print out.
 *
 *--------------------------------------------------------------
 */

void gdk_set_debug_level (int level) {
  gdk_debug_level = level;
}

/*
 *--------------------------------------------------------------
 * gdk_set_show_events
 *
 *   Turns on/off the showing of events.
 *
 * Arguments:
 *   "show_events" is a boolean describing whether or
 *   not to show the events gdk receives.
 *
 * Results:
 *
 * Side effects:
 *   When "show_events" is TRUE, calls to "gdk_event_get"
 *   will output debugging informatin regarding the event
 *   received to stdout.
 *
 *--------------------------------------------------------------
 */

void gdk_set_show_events (int show_events) {
  gdk_show_events = show_events;
}

/*
 *--------------------------------------------------------------
 * gdk_time_get
 *
 *   Get the number of milliseconds since the library was
 *   initialized.
 *
 * Arguments:
 *
 * Results:
 *   The time since the library was initialized is returned.
 *   This time value is accurate to milliseconds even though
 *   a more accurate time down to the microsecond could be
 *   returned.
 *
 * Side effects:
 *
 *--------------------------------------------------------------
 */

guint32 gdk_time_get () {
  struct timeval end;
  struct timeval elapsed;
  guint32 milliseconds;

  g_function_enter ("gdk_time_get");

  X_GETTIMEOFDAY (&end);

  if (start.tv_usec > end.tv_usec) {
    end.tv_usec += 1000000;
    end.tv_sec--;
  }
  elapsed.tv_sec = end.tv_sec - start.tv_sec;
  elapsed.tv_usec = end.tv_usec - start.tv_usec;

  milliseconds = (elapsed.tv_sec * 1000) + (elapsed.tv_usec / 1000);

  g_function_leave ("gdk_time_get");
  return milliseconds;
}

/*
 *--------------------------------------------------------------
 * gdk_timer_get
 *
 *   Returns the current timer.
 *
 * Arguments:
 *
 * Results:
 *   Returns the current timer interval. This interval is
 *   in units of milliseconds.
 *
 * Side effects:
 *
 *--------------------------------------------------------------
 */

guint32 gdk_timer_get () {
  return timer_val;
}

/*
 *--------------------------------------------------------------
 * gdk_timer_set
 *
 *   Sets the timer interval.
 *
 * Arguments:
 *   "milliseconds" is the new value for the timer.
 *
 * Results:
 *
 * Side effects:
 *   Calls to "gdk_event_get" will last for a maximum
 *   of time of "milliseconds". However, a value of 0
 *   milliseconds will cause "gdk_event_get" to block
 *   indefinately until an event is received.
 *
 *--------------------------------------------------------------
 */

void gdk_timer_set (guint32 milliseconds) {
  g_function_enter ("gdk_timer_set");

  timer_val = milliseconds;
  if (milliseconds == 0) {
      timerp = NULL;
  } else {
    timerp = &timer;
    timer.tv_sec = milliseconds / 1000;
    timer.tv_usec = (milliseconds % 1000) * 1000;
  }

  g_function_leave ("gdk_timer_set");
}

gint gdk_input_add (gint source, GdkInputCondition condition, GdkInputFunction function, gpointer data) {
  static gint next_tag = 1;
  GList *list;
  GdkInput *input;
  gint tag;

  g_function_enter ("gdk_input_add");

  tag = 0;
  list = inputs;

  while (list) {
    input = list->data;
    list = list->next;

    if ((input->source == source) && (input->condition == condition)) {
      input->function = function;
      input->data = data;
      tag = input->tag;
    }
  }

  if (!tag) {
    input = g_new (GdkInput, 1);
    input->tag = next_tag++;
    input->source = source;
    input->condition = condition;
    input->function = function;
    input->data = data;
    tag = input->tag;

    inputs = g_list_prepend (inputs, input);
  }

  g_function_leave ("gdk_input_add");
  return tag;
}

void gdk_input_remove (gint tag) {
  GList *list;
  GList *temp_list;
  GdkInput *input;

  g_function_enter ("gdk_input_remove");

  list = inputs;
  while (list) {
    input = list->data;

    if (input->tag == tag) {
      temp_list = list;

      if (list->next) {
        list->next->prev = list->prev;
      }
      if (list->prev) {
        list->prev->next = list->next;
      }
      if (inputs == list) {
        inputs = list->next;
      }

  	  temp_list->next = NULL;
  	  temp_list->prev = NULL;

	    g_free (temp_list->data);
	    g_list_free (temp_list);
	    break;
    }

    list = list->next;
  }

  g_function_leave ("gdk_input_remove");
}

/*
 *--------------------------------------------------------------
 * gdk_pointer_grab
 *
 *   Grabs the pointer to a specific window
 *
 * Arguments:
 *   "window" is the window which will receive the grab
 *   "owner_events" specifies whether events will be reported as is,
 *     or relative to "window"
 *   "event_mask" masks only interesting events
 *   "confine_to" limits the cursor movement to the specified window
 *   "cursor" changes the cursor for the duration of the grab
 *   "time" specifies the time
 *
 * Results:
 *
 * Side effects:
 *   requires a corresponding call to gdk_pointer_ungrab
 *
 *--------------------------------------------------------------
 */

gint
gdk_pointer_grab (GdkWindow *     window,
		  gint            owner_events,
		  GdkEventMask    event_mask,
		  GdkWindow *     confine_to,
		  GdkCursor *     cursor,
		  guint32         time)
{
  /*  From gdkwindow.c  */
  extern int nevent_masks;
  extern int event_mask_table[];
  gint return_val;
  GdkWindowPrivate *window_private;
  GdkWindowPrivate *confine_to_private;
  GdkCursorPrivate *cursor_private;
  guint xevent_mask;
  Window xwindow;
  Window xconfine_to;
  Cursor xcursor;
  int i;

  if (!window)
    g_error ("passed NULL for window in gdk_pointer_grab");

  window_private = (GdkWindowPrivate*) window;
  confine_to_private = (GdkWindowPrivate*) confine_to;
  cursor_private = (GdkCursorPrivate*) cursor;

  xwindow = window_private->xwindow;

  if (!confine_to)
    xconfine_to = None;
  else
    xconfine_to = confine_to_private->xwindow;

  if (!cursor)
    xcursor = None;
  else
    xcursor = cursor_private->xcursor;

  g_function_enter ("gdk_pointer_grab");

  xevent_mask = 0;
  for (i = 0; i < nevent_masks; i++)
    {
      if (event_mask & (1 << (i + 1)))
	xevent_mask |= event_mask_table[i];
    }

  return_val = XGrabPointer (window_private->xdisplay,
			     xwindow,
			     owner_events,
			     xevent_mask,
			     GrabModeAsync, GrabModeAsync,
			     xconfine_to,
			     xcursor,
			     time);

  g_function_leave ("gdk_pointer_grab");

  return return_val;
}

/*
 *--------------------------------------------------------------
 * gdk_pointer_ungrab
 *
 *   Releases any pointer grab
 *
 * Arguments:
 *
 * Results:
 *
 * Side effects:
 *
 *--------------------------------------------------------------
 */

void
gdk_pointer_ungrab (guint32 time)
{
  g_function_enter ("gdk_pointer_ungrab");

  XUngrabPointer (gdk_display, time);

  g_function_leave ("gdk_pointer_ungrab");
}

/*
 *--------------------------------------------------------------
 * gdk_screen_width
 *
 *   Return the width of the screen.
 *
 * Arguments:
 *
 * Results:
 *
 * Side effects:
 *
 *--------------------------------------------------------------
 */

gint
gdk_screen_width ()
{
  gint return_val;

  g_function_enter ("gdk_screen_width");

  return_val = DisplayWidth (gdk_display, gdk_screen);

  g_function_leave ("gdk_screen_width");
  return return_val;
}

/*
 *--------------------------------------------------------------
 * gdk_screen_height
 *
 *   Return the height of the screen.
 *
 * Arguments:
 *
 * Results:
 *
 * Side effects:
 *
 *--------------------------------------------------------------
 */

gint
gdk_screen_height ()
{
  gint return_val;

  g_function_enter ("gdk_screen_height");

  return_val = DisplayHeight (gdk_display, gdk_screen);

  g_function_leave ("gdk_screen_height");
  return return_val;
}

/*
 *--------------------------------------------------------------
 * gdk_flush
 *
 *   Flushes the Xlib output buffer and then waits
 *   until all requests have been received and processed
 *   by the X server. The only real use for this function
 *   is in dealing with XShm.
 *
 * Arguments:
 *
 * Results:
 *
 * Side effects:
 *
 *--------------------------------------------------------------
 */

void gdk_flush ()
{
  XSync (gdk_display, False);
}


/*
 *--------------------------------------------------------------
 * gdk_event_wait
 *
 *   Waits until an event occurs or the timer runs out.
 *
 * Arguments:
 *
 * Results:
 *   Returns TRUE if an event is ready to be read and FALSE
 *   if the timer ran out.
 *
 * Side effects:
 *
 *--------------------------------------------------------------
 */

static gint
gdk_event_wait ()
{
  GList *list;
  GdkInput *input;
  GdkInputCondition condition;
  guint32 time_interval;
  guint32 cur_time;
  fd_set readfds;
  fd_set writefds;
  fd_set exceptfds;
  int max_input;
  int return_val;
  int nfd;

  g_function_enter ("gdk_event_wait");


  /* If there are no events pending we will wait for an event.
   * The time we wait is dependant on the "timer". If no timer
   *  has been specified then we'll block until an event arrives.
   *  If a timer has been specified we'll block until an event
   *  arrives or the timer expires. (This is all done using the
   *  "select" system call).
   */
  return_val = FALSE;

  if (do_playback)
    {
      gdk_event_playback_read ();

      if (!wait_for_send_event && have_send_event && playback_events && !playback_events->next)
	{
	  cur_time = gdk_time_get ();
	  if (next_event_time <= cur_time)
	    {
	      gdk_event_send_xevent (gdk_display, (XEvent*) playback_events->data);

	      if (!wait_for_send_event && have_send_event && playback_events && !playback_events->next)
		gdk_timer_set (1);
	    }
	  else if (next_event_time > cur_time)
	    {
	      time_interval = next_event_time - cur_time;
	      if ((time_interval < timer_val) || (timer_val == 0))
		gdk_timer_set (time_interval);
	      wait_for_event = TRUE;
	    }
	}
    }

  if (XPending (gdk_display) == 0)
    {
      FD_ZERO (&readfds);
      FD_ZERO (&writefds);
      FD_ZERO (&exceptfds);

      FD_SET (connection_number, &readfds);
      max_input = connection_number;

      list = inputs;
      while (list)
	{
	  input = list->data;
	  list = list->next;

	  if (input->condition & GDK_INPUT_READ)
	    FD_SET (input->source, &readfds);
	  if (input->condition & GDK_INPUT_WRITE)
	    FD_SET (input->source, &writefds);
	  if (input->condition & GDK_INPUT_EXCEPTION)
	    FD_SET (input->source, &exceptfds);

	  max_input = MAX (max_input, input->source);
	}

      nfd = select (max_input+1, &readfds, &writefds, &exceptfds, timerp);

      timerp = NULL;
      timer_val = 0;

      wait_for_event = FALSE;
      if (wait_for_event && !wait_for_send_event)
	gdk_event_send_xevent (gdk_display, (XEvent*) playback_events->data);

      if (nfd > 0)
	{
	  if (FD_ISSET (connection_number, &readfds))
	    {
	      if (XPending (gdk_display) == 0)
		{
		  if (nfd == 1)
		    {
		      XNoOp (gdk_display);
		      XFlush (gdk_display);
		    }
		  return_val = FALSE;
		}
	      else
		return_val = TRUE;
	    }

	  list = inputs;
	  while (list)
	    {
	      input = list->data;
	      list = list->next;

	      condition = 0;
	      if (FD_ISSET (input->source, &readfds))
		condition |= GDK_INPUT_READ;
	      if (FD_ISSET (input->source, &writefds))
		condition |= GDK_INPUT_WRITE;
	      if (FD_ISSET (input->source, &exceptfds))
		condition |= GDK_INPUT_EXCEPTION;

	      if (condition && input->function)
		(* input->function) (input->data, input->source, condition);
	    }
	}
    }
  else
    return_val = TRUE;

  g_function_leave ("gdk_event_wait");
  return return_val;
}

static void
gdk_synthesize_click (GdkEvent *event,
		      gint      nclicks)
{
  GdkEvent temp_event;

  g_function_enter ("gdk_synthesize_click");

  g_assert (event != NULL);

  temp_event = *event;
  temp_event.type = (nclicks == 2) ? GDK_2BUTTON_PRESS : GDK_3BUTTON_PRESS;

  gdk_event_put (&temp_event);

  g_function_leave ("gdk_synthesize_click");
}

static void
gdk_event_record (Display *display, XEvent *event)
{
  g_function_enter ("gdk_event_record");

  g_assert (display != NULL);
  g_assert (event != NULL);
  g_assert (record_fp != NULL);

  switch (event->type)
    {
    case KeyPress:
      fprintf (record_fp, "(key_press (window %ld) (time %lu) (xy %d %d) (state %u) (keycode %u))\n",
	       event->xkey.window - base_id, gdk_time_get (),
	       event->xkey.x, event->xkey.y, event->xkey.state,
	       event->xkey.keycode);
      break;

    case KeyRelease:
      fprintf (record_fp, "(key_release (window %ld) (time %lu) (xy %d %d) (state %u) (keycode %u))\n",
	       event->xkey.window - base_id, gdk_time_get (),
	       event->xkey.x, event->xkey.y, event->xkey.state,
	       event->xkey.keycode);
      break;

    case ButtonPress:
      fprintf (record_fp, "(button_press (window %ld) (time %lu) (xy %d %d) (state %u) (button %u))\n",
	       event->xbutton.window - base_id, gdk_time_get (),
	       event->xbutton.x, event->xbutton.y, event->xbutton.state,
	       event->xbutton.button);
      break;

    case ButtonRelease:
      fprintf (record_fp, "(button_release (window %ld) (time %lu) (xy %d %d) (state %u) (button %u))\n",
	       event->xbutton.window - base_id, gdk_time_get (),
	       event->xbutton.x, event->xbutton.y, event->xbutton.state,
	       event->xbutton.button);
      break;

    case MotionNotify:
      fprintf (record_fp, "(motion_notify (window %ld) (time %lu) (xy %d %d) (state %u) (is_hint %d))\n",
	       event->xmotion.window - base_id, gdk_time_get (),
	       event->xmotion.x, event->xmotion.y, event->xmotion.state,
	       event->xmotion.is_hint);
      break;

    case CreateNotify:
      fprintf (record_fp, "(create_notify (window %ld))\n",
	       event->xany.window - base_id);
      break;

    case MapNotify:
      fprintf (record_fp, "(map_notify (window %ld))\n",
	       event->xany.window - base_id);
      break;

    case ReparentNotify:
      fprintf (record_fp, "(reparent_notify (window %ld))\n",
	       event->xany.window - base_id);
      break;

    case ConfigureNotify:
      fprintf (record_fp, "(configure_notify (window %ld))\n",
	       event->xany.window - base_id);
      break;

    default:
      break;
    }

  g_function_leave ("gdk_event_record");
}

static void
gdk_event_display (char *str, XEvent *event)
{
  char *event_name;

  g_function_enter ("gdk_event_display");

  g_assert (str != NULL);
  g_assert (event != NULL);

  switch (event->type)
    {
    case KeyPress:
      event_name = "key_press";
      break;
    case KeyRelease:
      event_name = "key_release";
      break;
    case ButtonPress:
      event_name = "button_press";
      break;
    case ButtonRelease:
      event_name = "button_release";
      break;
    case MotionNotify:
      event_name = "motion_notify";
      break;
    case CreateNotify:
      event_name = "create_notify";
      break;
    case MapNotify:
      event_name = "map_notify";
      break;
    case ReparentNotify:
      event_name = "reparent_notify";
      break;
    case ConfigureNotify:
      event_name = "configure_notify";
      break;
    default:
      event_name = "";
      break;
    }

  g_message ("%s: %s: %d", str, event_name,
	     event->xany.window - base_id);

  g_function_leave ("gdk_event_display");
}

static void
gdk_event_get_xevent (Display *display, XEvent *event)
{
  GList *list;
  GList *temp_list;
  XEvent *playback_event;

  g_function_enter ("gdk_event_get_xevent");

  g_assert (display != NULL);
  g_assert (event != NULL);

  if (!wait_for_send_event && have_send_event && !playback_events->next)
    gdk_event_send_xevent (display, (XEvent*) playback_events->data);

  XNextEvent (display, event);

  if (record_fp)
    gdk_event_record (gdk_display, event);

  switch (event->type)
    {
    case KeyPress:
    case KeyRelease:
    case ButtonPress:
    case ButtonRelease:
    case MotionNotify:
      if (do_playback && !event->xany.send_event)
	{
	  if (playback_fp)
	    fclose (playback_fp);
	  if (playback_filename)
	    g_free (playback_filename);

	  list = playback_events;
	  while (list)
	    {
	      g_free (list->data);
	      list = list->next;
	    }

	  if (playback_events)
	    g_list_free (playback_events);

	  playback_fp = NULL;
	  playback_filename = NULL;
	  playback_events = NULL;
	  wait_for_send_event = FALSE;
	  have_send_event = FALSE;
	  do_playback = FALSE;
	}
      break;
    }

  if (do_playback)
    {
      if (wait_for_send_event && event->xany.send_event)
	{
	  g_assert (playback_events != NULL);
	  g_assert (playback_events->next == NULL);

	  playback_event = playback_events->data;
	  if ((event->type == playback_event->type) &&
	      (event->xany.window == playback_event->xany.window))
	    {
	      g_free (playback_event);
	      g_list_free (playback_events);
	      playback_events = NULL;

	      if (!playback_fp)
		do_playback = FALSE;

	      wait_for_send_event = FALSE;
	      have_send_event = FALSE;
	    }
	}
      else
	{
	  list = playback_events;
	  while (list)
	    {
	      playback_event = list->data;

	      if ((event->type == playback_event->type) &&
		  (event->xany.window == playback_event->xany.window))
		{
		  g_free (playback_event);

		  temp_list = list;
		  if (temp_list->next)
		    temp_list->next->prev = temp_list->prev;
		  if (temp_list->prev)
		    temp_list->prev->next = temp_list->next;
		  if (temp_list == playback_events)
		    playback_events = temp_list->next;

		  temp_list->next = NULL;
		  temp_list->prev = NULL;
		  g_list_free (temp_list);
		  break;
		}

	      list = list->next;
	    }

	  if (!wait_for_send_event)
	    {
	      if (!playback_events)
		do_playback = FALSE;
	      else if (!playback_events->next)
		gdk_event_send_xevent (display, (XEvent*) playback_events->data);
	    }
	}
    }

  g_function_leave ("gdk_event_get_xevent");
}

static void
gdk_event_send_xevent (Display *display, XEvent *event)
{
  guint32 cur_time;
  gint done;
  gint index;

  g_function_enter ("gdk_event_send_xevent");

  done = FALSE;
  index = 1;

  cur_time = gdk_time_get ();
  while ((next_event_time <= cur_time) && !done)
    {
      switch (event->type)
	{
	case KeyPress:
	case KeyRelease:
	  XWarpPointer (gdk_display, None, event->xany.window,
			0, 0, 0, 0, event->xkey.x, event->xkey.y);
	  XSendEvent (gdk_display, event->xany.window,
		      TRUE, KeyPressMask|KeyReleaseMask, event);
	  wait_for_send_event = TRUE;
	  next_event_time = 0;
	  done = TRUE;
	  break;

	case ButtonPress:
	case ButtonRelease:
	  XWarpPointer (gdk_display, None, event->xany.window,
			0, 0, 0, 0, event->xbutton.x, event->xbutton.y);
	  XSendEvent (gdk_display, event->xany.window,
		      TRUE, ButtonPressMask|ButtonReleaseMask, event);
	  wait_for_send_event = TRUE;
	  next_event_time = 0;
	  done = TRUE;
	  break;

	case MotionNotify:
	  XWarpPointer (gdk_display, None, event->xany.window,
			0, 0, 0, 0, event->xmotion.x, event->xmotion.y);

	  if (event->xmotion.is_hint)
	    XSendEvent (gdk_display, event->xany.window,
			TRUE, PointerMotionHintMask|ButtonMotionMask|Button1MotionMask, event);

	  g_free (playback_events->data);
	  g_list_free (playback_events);
	  playback_events = NULL;

	  if (!playback_fp)
	    do_playback = FALSE;

	  wait_for_send_event = FALSE;
	  have_send_event = FALSE;
	  next_event_time = 0;

	  gdk_event_playback_read ();

	  if (wait_for_send_event || (playback_events && playback_events->next))
	    done = TRUE;
	  else
	    event = playback_events->data;
	  break;

	default:
	  g_error ("unexpected event: %d", event->type);
	  break;
	}

      cur_time = gdk_time_get ();
    }

  g_function_leave ("gdk_event_send_xevent");
}

static void
gdk_event_playback (Display *display, XEvent *event)
{
  g_function_enter ("gdk_event_playback");

  g_assert (display != NULL);
  g_assert (event != NULL);

  if (do_playback)
    {
      gdk_event_playback_read ();
      gdk_event_get_xevent (gdk_display, event);
    }
  else
    {
      gdk_event_get_xevent (gdk_display, event);
    }

  g_function_leave ("gdk_event_playback");
}

static void
gdk_event_playback_read ()
{
  XEvent *event;
  gchar buffer[1024];

  g_function_enter ("gdk_event_playback_read");

  g_assert (do_playback);

  if (playback_fp && !have_send_event)
    {
      do {
	fgets (buffer, 1024, playback_fp);
	if (feof (playback_fp))
	  {
	    fclose (playback_fp);
	    g_free (playback_filename);

	    playback_fp = NULL;
	    playback_filename = NULL;
	    break;
	  }

	event = g_new (XEvent, 1);
	memset (event, '\0', sizeof (XEvent));
	if (gdk_event_playback_parse (buffer, event))
	  playback_events = g_list_prepend (playback_events, event);
	} while (!have_send_event);

      playback_events = g_list_reverse (playback_events);
    }

  g_function_leave ("gdk_event_playback_read");
}

static gint
gdk_event_playback_get_next_token ()
{
  g_function_enter ("gdk_event_playback_get_next_token");

  if (next_token != -1)
    {
      cur_token = next_token;
      next_token = -1;
    }
  else
    {
      cur_token = gdk_event_playback_parse_token (NULL);
    }

  g_function_leave ("gdk_event_playback_get_next_token");
  return cur_token;
}

static gint
gdk_event_playback_peek_next_token ()
{
  g_function_enter ("gdk_event_playback_peek_next_token");

  if (next_token == -1)
    next_token = gdk_event_playback_get_next_token ();

  g_function_leave ("gdk_event_playback_peek_next_token");
  return next_token;
}

static gint
gdk_event_playback_parse (gchar *buf, XEvent *event)
{
  gint token;
  gint return_val;

  g_function_enter ("gdk_event_playback_parse");

  cur_token = -1;
  next_token = -1;

  if (gdk_event_playback_parse_token (buf) != TOKEN_LEFT_PAREN)
    g_error ("unexpected token in playback stream");

  token = gdk_event_playback_peek_next_token ();
  switch (token)
    {
    case TOKEN_KEY_PRESS:
      return_val = gdk_event_playback_parse_key_press (event);
      if (return_val)
	have_send_event = TRUE;
      break;
    case TOKEN_KEY_RELEASE:
      return_val = gdk_event_playback_parse_key_release (event);
      if (return_val)
	have_send_event = TRUE;
      break;
    case TOKEN_BUTTON_PRESS:
      return_val = gdk_event_playback_parse_button_press (event);
      if (return_val)
	have_send_event = TRUE;
      break;
    case TOKEN_BUTTON_RELEASE:
      return_val = gdk_event_playback_parse_button_release (event);
      if (return_val)
	have_send_event = TRUE;
      break;
    case TOKEN_MOTION_NOTIFY:
      return_val = gdk_event_playback_parse_motion_notify (event);
      if (return_val)
	have_send_event = TRUE;
      break;
    case TOKEN_CREATE_NOTIFY:
      return_val = gdk_event_playback_parse_create_notify (event);
      break;
    case TOKEN_MAP_NOTIFY:
      return_val = gdk_event_playback_parse_map_notify (event);
      break;
    case TOKEN_REPARENT_NOTIFY:
      return_val = gdk_event_playback_parse_reparent_notify (event);
      break;
    case TOKEN_CONFIGURE_NOTIFY:
      return_val = gdk_event_playback_parse_configure_notify (event);
      break;
    default:
      return_val = FALSE;
      break;
    }

  /*  if (return_val)
    gdk_event_display ("playback", event); */

  g_function_leave ("gdk_event_playback_parse");
  return return_val;
}

static gint
gdk_event_playback_parse_token (gchar *buf)
{
  static gchar *internal_buf = NULL;
  gchar *start_token;
  gint token;

  g_function_enter ("gdk_event_playback_parse_token");

  if (buf)
    internal_buf = buf;

  while (isspace (*internal_buf))
    internal_buf++;
  start_token = internal_buf;

  if (start_token[0] == '(')
    {
      internal_buf++;
      strncpy (token_str, start_token, (long) internal_buf - (long) start_token);
      token_str[strlen(token_str)] = '\0';
      token = TOKEN_LEFT_PAREN;
    }
  else if (start_token[0] == ')')
    {
      internal_buf++;
      strncpy (token_str, start_token, (long) internal_buf - (long) start_token);
      token_str[strlen(token_str)] = '\0';
      token = TOKEN_RIGHT_PAREN;
    }
  else
    {
      internal_buf++;
      while (1)
	{
	  if (isspace (*internal_buf) ||
	      (*internal_buf == '(') ||
	      (*internal_buf == ')'))
	    break;
	  internal_buf++;
	}

      strncpy (token_str, start_token, (long) internal_buf - (long) start_token);
      token_str[(long) internal_buf - (long) start_token] = '\0';

      if (strcmp (token_str, "key_press") == 0)
	token = TOKEN_KEY_PRESS;
      else if (strcmp (token_str, "key_release") == 0)
	token = TOKEN_KEY_RELEASE;
      else if (strcmp (token_str, "button_press") == 0)
	token = TOKEN_BUTTON_PRESS;
      else if (strcmp (token_str, "button_release") == 0)
	token = TOKEN_BUTTON_RELEASE;
      else if (strcmp (token_str, "motion_notify") == 0)
	token = TOKEN_MOTION_NOTIFY;
      else if (strcmp (token_str, "create_notify") == 0)
	token = TOKEN_CREATE_NOTIFY;
      else if (strcmp (token_str, "map_notify") == 0)
	token = TOKEN_MAP_NOTIFY;
      else if (strcmp (token_str, "reparent_notify") == 0)
	token = TOKEN_REPARENT_NOTIFY;
      else if (strcmp (token_str, "configure_notify") == 0)
	token = TOKEN_CONFIGURE_NOTIFY;
      else if (strcmp (token_str, "window") == 0)
	token = TOKEN_WINDOW;
      else if (strcmp (token_str, "time") == 0)
	token = TOKEN_TIME;
      else if (strcmp (token_str, "xy") == 0)
	token = TOKEN_XY;
      else if (strcmp (token_str, "state") == 0)
	token = TOKEN_STATE;
      else if (strcmp (token_str, "keycode") == 0)
	token = TOKEN_KEYCODE;
      else if (strcmp (token_str, "button") == 0)
	token = TOKEN_BUTTON;
      else if (strcmp (token_str, "is_hint") == 0)
	token = TOKEN_IS_HINT;
      else
	token = TOKEN_SYMBOL;
    }

  g_function_leave ("gdk_event_playback_parse_token");
  return token;
}

static gint
gdk_event_playback_parse_key_press (XEvent *event)
{
  gint token;
  gint return_val;

  g_function_enter ("gdk_event_playback_parse_key_press");

  token = gdk_event_playback_peek_next_token ();
  if (token != TOKEN_KEY_PRESS)
    {
      return_val = FALSE;
      goto done;
    }
  token = gdk_event_playback_get_next_token ();
  return_val = TRUE;

  event->type = KeyPress;
  event->xany.display = gdk_display;
  event->xany.window = None;
  event->xany.send_event = TRUE;
  event->xkey.time = 0;
  event->xkey.x = -1;
  event->xkey.y = -1;
  event->xkey.state = 0;
  event->xkey.keycode = 0;

  while (gdk_event_playback_parse_window (event) ||
	 gdk_event_playback_parse_time (event) ||
	 gdk_event_playback_parse_xy (event) ||
	 gdk_event_playback_parse_state (event) ||
	 gdk_event_playback_parse_keycode (event))
    ;

  token = gdk_event_playback_get_next_token ();
  if (token != TOKEN_RIGHT_PAREN)
    g_error ("unexpected token");

done:
  g_function_leave ("gdk_event_playback_parse_key_press");
  return return_val;
}

static gint
gdk_event_playback_parse_key_release (XEvent *event)
{
  gint token;
  gint return_val;

  g_function_enter ("gdk_event_playback_parse_key_release");

  token = gdk_event_playback_peek_next_token ();
  if (token != TOKEN_KEY_RELEASE)
    {
      return_val = FALSE;
      goto done;
    }
  token = gdk_event_playback_get_next_token ();
  return_val = TRUE;

  event->type = KeyRelease;
  event->xany.display = gdk_display;
  event->xany.window = None;
  event->xany.send_event = TRUE;
  event->xkey.time = 0;
  event->xkey.x = -1;
  event->xkey.y = -1;
  event->xkey.state = 0;
  event->xkey.keycode = 0;

  while (gdk_event_playback_parse_window (event) ||
	 gdk_event_playback_parse_time (event) ||
	 gdk_event_playback_parse_xy (event) ||
	 gdk_event_playback_parse_state (event) ||
	 gdk_event_playback_parse_keycode (event))
    ;

  token = gdk_event_playback_get_next_token ();
  if (token != TOKEN_RIGHT_PAREN)
    g_error ("unexpected token");

done:
  g_function_leave ("gdk_event_playback_parse_key_release");
  return return_val;
}

static gint
gdk_event_playback_parse_button_press (XEvent *event)
{
  gint token;
  gint return_val;

  g_function_enter ("gdk_event_playback_parse_button_press");

  token = gdk_event_playback_peek_next_token ();
  if (token != TOKEN_BUTTON_PRESS)
    {
      return_val = FALSE;
      goto done;
    }
  token = gdk_event_playback_get_next_token ();
  return_val = TRUE;

  event->type = ButtonPress;
  event->xany.display = gdk_display;
  event->xany.window = None;
  event->xany.send_event = TRUE;
  event->xbutton.time = 0;
  event->xbutton.x = -1;
  event->xbutton.y = -1;
  event->xbutton.state = 0;
  event->xbutton.button = 0;

  while (gdk_event_playback_parse_window (event) ||
	 gdk_event_playback_parse_time (event) ||
	 gdk_event_playback_parse_xy (event) ||
	 gdk_event_playback_parse_state (event) ||
	 gdk_event_playback_parse_button (event))
    ;

  token = gdk_event_playback_get_next_token ();
  if (token != TOKEN_RIGHT_PAREN)
    g_error ("unexpected token");

done:
  g_function_leave ("gdk_event_playback_parse_button_press");
  return return_val;
}

static gint
gdk_event_playback_parse_button_release (XEvent *event)
{
  gint token;
  gint return_val;

  g_function_enter ("gdk_event_playback_parse_button_release");

  token = gdk_event_playback_peek_next_token ();
  if (token != TOKEN_BUTTON_RELEASE)
    {
      return_val = FALSE;
      goto done;
    }
  token = gdk_event_playback_get_next_token ();
  return_val = TRUE;

  event->type = ButtonRelease;
  event->xany.display = gdk_display;
  event->xany.window = None;
  event->xany.send_event = TRUE;
  event->xbutton.time = 0;
  event->xbutton.x = -1;
  event->xbutton.y = -1;
  event->xbutton.state = 0;
  event->xbutton.button = 0;

  while (gdk_event_playback_parse_window (event) ||
	 gdk_event_playback_parse_time (event) ||
	 gdk_event_playback_parse_xy (event) ||
	 gdk_event_playback_parse_state (event) ||
	 gdk_event_playback_parse_button (event))
    ;

  token = gdk_event_playback_get_next_token ();
  if (token != TOKEN_RIGHT_PAREN)
    g_error ("unexpected token");

done:
  g_function_leave ("gdk_event_playback_parse_button_release");
  return return_val;
}

static gint
gdk_event_playback_parse_motion_notify (XEvent *event)
{
  gint token;
  gint return_val;

  g_function_enter ("gdk_event_playback_parse_motion_notify");

  token = gdk_event_playback_peek_next_token ();
  if (token != TOKEN_MOTION_NOTIFY)
    {
      return_val = FALSE;
      goto done;
    }
  token = gdk_event_playback_get_next_token ();
  return_val = TRUE;

  event->type = MotionNotify;
  event->xany.display = gdk_display;
  event->xany.window = None;
  event->xany.send_event = TRUE;
  event->xmotion.time = 0;
  event->xmotion.x = -1;
  event->xmotion.y = -1;
  event->xmotion.state = 0;
  event->xmotion.is_hint = 0;

  while (gdk_event_playback_parse_window (event) ||
	 gdk_event_playback_parse_time (event) ||
	 gdk_event_playback_parse_xy (event) ||
	 gdk_event_playback_parse_state (event) ||
	 gdk_event_playback_parse_is_hint (event))
    ;

  token = gdk_event_playback_get_next_token ();
  if (token != TOKEN_RIGHT_PAREN)
    g_error ("unexpected token");

done:
  g_function_leave ("gdk_event_playback_parse_motion_notify");
  return return_val;
}

static gint
gdk_event_playback_parse_create_notify (XEvent *event)
{
  gint token;
  gint return_val;

  g_function_enter ("gdk_event_playback_parse_create_notify");

  token = gdk_event_playback_peek_next_token ();
  if (token != TOKEN_CREATE_NOTIFY)
    {
      return_val = FALSE;
      goto done;
    }
  token = gdk_event_playback_get_next_token ();
  return_val = TRUE;

  event->type = CreateNotify;
  event->xany.display = gdk_display;
  event->xany.window = None;
  event->xany.send_event = TRUE;

  while (gdk_event_playback_parse_window (event))
    ;

  token = gdk_event_playback_get_next_token ();
  if (token != TOKEN_RIGHT_PAREN)
    g_error ("unexpected token");

done:
  g_function_leave ("gdk_event_playback_parse_create_notify");
  return return_val;
}

static gint
gdk_event_playback_parse_map_notify (XEvent *event)
{
  gint token;
  gint return_val;

  g_function_enter ("gdk_event_playback_parse_map_notify");

  token = gdk_event_playback_peek_next_token ();
  if (token != TOKEN_MAP_NOTIFY)
    {
      return_val = FALSE;
      goto done;
    }
  token = gdk_event_playback_get_next_token ();
  return_val = TRUE;

  event->type = MapNotify;
  event->xany.display = gdk_display;
  event->xany.window = None;
  event->xany.send_event = TRUE;

  while (gdk_event_playback_parse_window (event))
    ;

  token = gdk_event_playback_get_next_token ();
  if (token != TOKEN_RIGHT_PAREN)
    g_error ("unexpected token");

done:
  g_function_leave ("gdk_event_playback_parse_map_notify");
  return return_val;
}

static gint
gdk_event_playback_parse_reparent_notify (XEvent *event)
{
  gint token;
  gint return_val;

  g_function_enter ("gdk_event_playback_parse_reparent_notify");

  token = gdk_event_playback_peek_next_token ();
  if (token != TOKEN_REPARENT_NOTIFY)
    {
      return_val = FALSE;
      goto done;
    }
  token = gdk_event_playback_get_next_token ();
  return_val = TRUE;

  event->type = ReparentNotify;
  event->xany.display = gdk_display;
  event->xany.window = None;
  event->xany.send_event = TRUE;

  while (gdk_event_playback_parse_window (event))
    ;

  token = gdk_event_playback_get_next_token ();
  if (token != TOKEN_RIGHT_PAREN)
    g_error ("unexpected token");

done:
  g_function_leave ("gdk_event_playback_parse_reparent_notify");
  return return_val;
}

static gint
gdk_event_playback_parse_configure_notify (XEvent *event)
{
  gint token;
  gint return_val;

  g_function_enter ("gdk_event_playback_parse_configure_notify");

  token = gdk_event_playback_peek_next_token ();
  if (token != TOKEN_CONFIGURE_NOTIFY)
    {
      return_val = FALSE;
      goto done;
    }
  token = gdk_event_playback_get_next_token ();
  return_val = TRUE;

  event->type = ConfigureNotify;
  event->xany.display = gdk_display;
  event->xany.window = None;
  event->xany.send_event = TRUE;

  while (gdk_event_playback_parse_window (event))
    ;

  token = gdk_event_playback_get_next_token ();
  if (token != TOKEN_RIGHT_PAREN)
    g_error ("unexpected token");

done:
  g_function_leave ("gdk_event_playback_parse_configure_notify");
  return return_val;
}

static gint
gdk_event_playback_parse_window (XEvent *event)
{
  gint token;
  gint return_val;

  g_function_enter ("gdk_event_playback_parse_window");

  token = gdk_event_playback_peek_next_token ();
  if (token == TOKEN_LEFT_PAREN)
    {
      token = gdk_event_playback_get_next_token ();
      token = gdk_event_playback_peek_next_token ();
    }

  if (token != TOKEN_WINDOW)
    {
      return_val = FALSE;
      goto done;
    }
  token = gdk_event_playback_get_next_token ();
  return_val = TRUE;

  token = gdk_event_playback_get_next_token ();
  if (token != TOKEN_SYMBOL)
    g_error ("unexpected token");

  sscanf (token_str, "%ld", &event->xany.window);
  event->xany.window += base_id;

  token = gdk_event_playback_get_next_token ();
  if (token != TOKEN_RIGHT_PAREN)
    g_error ("unexpected token");

done:
  g_function_leave ("gdk_event_playback_parse_window");
  return return_val;
}

static gint
gdk_event_playback_parse_time (XEvent *event)
{
  gint token;
  gint return_val;

  g_function_enter ("gdk_event_playback_parse_time");

  token = gdk_event_playback_peek_next_token ();
  if (token == TOKEN_LEFT_PAREN)
    {
      token = gdk_event_playback_get_next_token ();
      token = gdk_event_playback_peek_next_token ();
    }

  if (token != TOKEN_TIME)
    {
      return_val = FALSE;
      goto done;
    }
  token = gdk_event_playback_get_next_token ();
  return_val = TRUE;

  token = gdk_event_playback_get_next_token ();
  if (token != TOKEN_SYMBOL)
    g_error ("unexpected token");

  sscanf (token_str, "%lu", &next_event_time);

  token = gdk_event_playback_get_next_token ();
  if (token != TOKEN_RIGHT_PAREN)
    g_error ("unexpected token");

done:
  g_function_leave ("gdk_event_playback_parse_time");
  return return_val;
}

static gint
gdk_event_playback_parse_xy (XEvent *event)
{
  gint token;
  gint return_val;

  g_function_enter ("gdk_event_playback_parse_xy");

  token = gdk_event_playback_peek_next_token ();
  if (token == TOKEN_LEFT_PAREN)
    {
      token = gdk_event_playback_get_next_token ();
      token = gdk_event_playback_peek_next_token ();
    }

  if (token != TOKEN_XY)
    {
      return_val = FALSE;
      goto done;
    }
  token = gdk_event_playback_get_next_token ();
  return_val = TRUE;

  token = gdk_event_playback_get_next_token ();
  if (token != TOKEN_SYMBOL)
    g_error ("unexpected token");

  if ((event->type == KeyPress) ||
      (event->type == KeyRelease))
    sscanf (token_str, "%d", &event->xkey.x);
  else if ((event->type == ButtonPress) ||
	   (event->type == ButtonRelease))
    sscanf (token_str, "%d", &event->xbutton.x);
  else if (event->type == MotionNotify)
    sscanf (token_str, "%d", &event->xmotion.x);

  token = gdk_event_playback_get_next_token ();
  if (token != TOKEN_SYMBOL)
    g_error ("unexpected token");

  if ((event->type == KeyPress) ||
      (event->type == KeyRelease))
    sscanf (token_str, "%d", &event->xkey.y);
  else if ((event->type == ButtonPress) ||
	   (event->type == ButtonRelease))
    sscanf (token_str, "%d", &event->xbutton.y);
  else if (event->type == MotionNotify)
    sscanf (token_str, "%d", &event->xmotion.y);

  token = gdk_event_playback_get_next_token ();
  if (token != TOKEN_RIGHT_PAREN)
    g_error ("unexpected token");

done:
  g_function_leave ("gdk_event_playback_parse_xy");
  return return_val;
}

static gint
gdk_event_playback_parse_state (XEvent *event)
{
  gint token;
  gint return_val;

  g_function_enter ("gdk_event_playback_parse_state");

  token = gdk_event_playback_peek_next_token ();
  if (token == TOKEN_LEFT_PAREN)
    {
      token = gdk_event_playback_get_next_token ();
      token = gdk_event_playback_peek_next_token ();
    }

  if (token != TOKEN_STATE)
    {
      return_val = FALSE;
      goto done;
    }
  token = gdk_event_playback_get_next_token ();
  return_val = TRUE;

  token = gdk_event_playback_get_next_token ();
  if (token != TOKEN_SYMBOL)
    g_error ("unexpected token");

  if ((event->type == KeyPress) ||
      (event->type == KeyRelease))
    sscanf (token_str, "%u", &event->xkey.state);
  else if ((event->type == ButtonPress) ||
	   (event->type == ButtonRelease))
    sscanf (token_str, "%u", &event->xbutton.state);
  else if (event->type == MotionNotify)
    sscanf (token_str, "%u", &event->xmotion.state);

  token = gdk_event_playback_get_next_token ();
  if (token != TOKEN_RIGHT_PAREN)
    g_error ("unexpected token");

done:
  g_function_leave ("gdk_event_playback_parse_state");
  return return_val;
}

static gint
gdk_event_playback_parse_keycode (XEvent *event)
{
  gint token;
  gint return_val;

  g_function_enter ("gdk_event_playback_parse_keycode");

  token = gdk_event_playback_peek_next_token ();
  if (token == TOKEN_LEFT_PAREN)
    {
      token = gdk_event_playback_get_next_token ();
      token = gdk_event_playback_peek_next_token ();
    }

  if (token != TOKEN_KEYCODE)
    {
      return_val = FALSE;
      goto done;
    }
  token = gdk_event_playback_get_next_token ();
  return_val = TRUE;

  token = gdk_event_playback_get_next_token ();
  if (token != TOKEN_SYMBOL)
    g_error ("unexpected token");

  if ((event->type == KeyPress) ||
      (event->type == KeyPress))
    sscanf (token_str, "%u", &event->xkey.keycode);

  token = gdk_event_playback_get_next_token ();
  if (token != TOKEN_RIGHT_PAREN)
    g_error ("unexpected token");

done:
  g_function_leave ("gdk_event_playback_parse_keycode");
  return return_val;
}

static gint
gdk_event_playback_parse_button (XEvent *event)
{
  gint token;
  gint return_val;

  g_function_enter ("gdk_event_playback_parse_button");

  token = gdk_event_playback_peek_next_token ();
  if (token == TOKEN_LEFT_PAREN)
    {
      token = gdk_event_playback_get_next_token ();
      token = gdk_event_playback_peek_next_token ();
    }

  if (token != TOKEN_BUTTON)
    {
      return_val = FALSE;
      goto done;
    }
  token = gdk_event_playback_get_next_token ();
  return_val = TRUE;

  token = gdk_event_playback_get_next_token ();
  if (token != TOKEN_SYMBOL)
    g_error ("unexpected token");

  if ((event->type == ButtonPress) ||
      (event->type == ButtonRelease))
    sscanf (token_str, "%u", &event->xbutton.button);

  token = gdk_event_playback_get_next_token ();
  if (token != TOKEN_RIGHT_PAREN)
    g_error ("unexpected token");

done:
  g_function_leave ("gdk_event_playback_parse_button");
  return return_val;
}

static gint
gdk_event_playback_parse_is_hint (XEvent *event)
{
  gint token;
  gint return_val;

  g_function_enter ("gdk_event_playback_parse_is_hint");

  token = gdk_event_playback_peek_next_token ();
  if (token == TOKEN_LEFT_PAREN)
    {
      token = gdk_event_playback_get_next_token ();
      token = gdk_event_playback_peek_next_token ();
    }

  if (token != TOKEN_IS_HINT)
    {
      return_val = FALSE;
      goto done;
    }
  token = gdk_event_playback_get_next_token ();
  return_val = TRUE;

  token = gdk_event_playback_get_next_token ();
  if (token != TOKEN_SYMBOL)
    g_error ("unexpected token");

  if (event->type == MotionNotify)
    sscanf (token_str, "%d", &event->xmotion.is_hint);

  token = gdk_event_playback_get_next_token ();
  if (token != TOKEN_RIGHT_PAREN)
    g_error ("unexpected token");

done:
  g_function_leave ("gdk_event_playback_parse_is_hint");
  return return_val;
}

/*
 *--------------------------------------------------------------
 * gdk_exit_func
 *
 *   This is the "atexit" function that makes sure the
 *   library gets a chance to cleanup.
 *
 * Arguments:
 *
 * Results:
 *
 * Side effects:
 *   The library is un-initialized and the program exits.
 *
 *--------------------------------------------------------------
 */

static void
gdk_exit_func ()
{
  g_function_enter ("gdk_exit_func");

  if (initialized)
    gdk_exit (0);

  g_function_leave ("gdk_exit_func");
}

/*
 *--------------------------------------------------------------
 * gdk_x_error
 *
 *   The X error handling routine.
 *
 * Arguments:
 *   "display" is the X display the error orignated from.
 *   "error" is the XErrorEvent that we are handling.
 *
 * Results:
 *   Either we were expecting some sort of error to occur,
 *   in which case we set the "gdk_error_code" flag, or this
 *   error was unexpected, in which case we will print an
 *   error message and exit. (Since trying to continue will
 *   most likely simply lead to more errors).
 *
 * Side effects:
 *
 *--------------------------------------------------------------
 */

static int
gdk_x_error (Display     *display,
	     XErrorEvent *error)
{
  char buf[64];

  g_function_enter ("gdk_x_error");

  if (gdk_error_warnings)
    {
      XGetErrorText (display, error->error_code, buf, 63);
      g_error ("%s", buf);
    }

  gdk_error_code = -1;
  g_function_leave ("gdk_x_error");
  return 0;
}

/*
 *--------------------------------------------------------------
 * gdk_x_io_error
 *
 *   The X I/O error handling routine.
 *
 * Arguments:
 *   "display" is the X display the error orignated from.
 *
 * Results:
 *   An X I/O error basically means we lost our connection
 *   to the X server. There is not much we can do to
 *   continue, so simply print an error message and exit.
 *
 * Side effects:
 *
 *--------------------------------------------------------------
 */

static int
gdk_x_io_error (Display *display)
{
  g_function_enter ("gdk_x_io_error");
  g_error ("an x io error occurred");
  g_function_leave ("gdk_x_io_error");
  return 0;
}

/*
 *--------------------------------------------------------------
 * gdk_signal
 *
 *   The signal handler.
 *
 * Arguments:
 *   "sig_num" is the number of the signal we received.
 *
 * Results:
 *   The signals we catch are all fatal. So we simply build
 *   up a nice little error message and print it and exit.
 *   If in the process of doing so another signal is received
 *   we notice that we are already exiting and simply kill
 *   our process.
 *
 * Side effects:
 *
 *--------------------------------------------------------------
 */

static RETSIGTYPE
gdk_signal (int sig_num)
{
  static int caught_fatal_sig = 0;

  char *sig;

  g_function_enter ("gdk_signal");

  if (caught_fatal_sig)
    kill (getpid (), sig_num);
  caught_fatal_sig = 1;

  switch (sig_num)
    {
    case SIGHUP:
      sig = "sighup";
      break;
    case SIGINT:
      sig = "sigint";
      break;
    case SIGQUIT:
      sig = "sigquit";
      break;
    case SIGBUS:
      sig = "sigbus";
      break;
    case SIGSEGV:
      sig = "sigsegv";
      break;
    case SIGPIPE:
      sig = "sigpipe";
      break;
    case SIGTERM:
      sig = "sigterm";
      break;
    default:
      sig = "unknown signal";
      break;
    }

  g_error ("%s caught", sig);
  g_function_leave ("gdk_signal");
}
