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
#ifndef __GDK_TYPES_H__
#define __GDK_TYPES_H__


#include <X11/keysym.h>


/* GDK uses "glib". (And so does GTK).
 * Also include the config file so that anybody can
 *  access the config information if needed.
 */
#include "glib.h"


/* Type definitions for the basic structures.
 */

typedef struct _GdkColor          GdkColor;
typedef struct _GdkColormap       GdkColormap;
typedef struct _GdkVisual         GdkVisual;
typedef struct _GdkWindowAttr     GdkWindowAttr;
typedef struct _GdkWindow         GdkWindow;
typedef struct _GdkWindow         GdkPixmap;
typedef struct _GdkImage          GdkImage;
typedef struct _GdkGC             GdkGC;
typedef struct _GdkPoint          GdkPoint;
typedef struct _GdkRectangle      GdkRectangle;
typedef struct _GdkSegment        GdkSegment;
typedef struct _GdkFont           GdkFont;
typedef struct _GdkCursor         GdkCursor;

typedef struct _GdkEventAny       GdkEventAny;
typedef struct _GdkEventExpose    GdkEventExpose;
typedef struct _GdkEventMotion    GdkEventMotion;
typedef struct _GdkEventButton    GdkEventButton;
typedef struct _GdkEventKey       GdkEventKey;
typedef struct _GdkEventFocus     GdkEventFocus;
typedef struct _GdkEventCrossing  GdkEventCrossing;
typedef struct _GdkEventResize    GdkEventResize;
typedef union  _GdkEvent          GdkEvent;


/* Types of windows.
 *   Root: There is only 1 root window and it is initialized
 *         at startup. Creating a window of type GDK_WINDOW_ROOT
 *         is an error.
 *   Toplevel: Windows which interact with the window manager.
 *   Child: Windows which are children of some other type of window.
 *          (Any other type of window). Most windows are child windows.
 *   Dialog: A special kind of toplevel window which interacts with
 *           the window manager slightly differently than a regular
 *           toplevel window. Dialog windows should be used for any
 *           transient window.
 *   Pixmap: Pixmaps are really just another kind of window which
 *           doesn't actually appear on the screen. It can't have
 *           children, either and is really just a convenience so
 *           that the drawing functions can work on both windows
 *           and pixmaps transparently. (ie. You shouldn't pass a
 *           pixmap to any procedure which accepts a window with the
 *           exception of the drawing functions).
 */
typedef enum
{
  GDK_WINDOW_ROOT,
  GDK_WINDOW_TOPLEVEL,
  GDK_WINDOW_CHILD,
  GDK_WINDOW_DIALOG,
  GDK_WINDOW_TEMP,
  GDK_WINDOW_PIXMAP
} GdkWindowType;

/* Classes of windows.
 *   InputOutput: Almost every window should be of this type. Such windows
 *                receive events and are also displayed on screen.
 *   InputOnly: Used only in special circumstances when events need to be
 *              stolen from another window or windows. Input only windows
 *              have no visible output, so they are handy for placing over
 *              top of a group of windows in order to grab the events (or
 *              filter the events) from those windows.
 */
typedef enum
{
  GDK_INPUT_OUTPUT,
  GDK_INPUT_ONLY
} GdkWindowClass;

/* Types of images.
 *   Normal: Normal X image type. These are slow as they involve passing
 *           the entire image through the X connection each time a draw
 *           request is required.
 *   Shared: Shared memory X image type. These are fast as the X server
 *           and the program actually use the same piece of memory. They
 *           should be used with care though as there is the possibility
 *           for both the X server and the program to be reading/writing
 *           the image simultaneously and producing undesired results.
 */
typedef enum
{
  GDK_IMAGE_NORMAL,
  GDK_IMAGE_SHARED,
  GDK_IMAGE_FASTEST
} GdkImageType;

/* Types of visuals.
 *   StaticGray:
 *   Grayscale:
 *   StaticColor:
 *   PseudoColor:
 *   TrueColor:
 *   DirectColor:
 */
typedef enum
{
  GDK_VISUAL_STATIC_GRAY,
  GDK_VISUAL_GRAYSCALE,
  GDK_VISUAL_STATIC_COLOR,
  GDK_VISUAL_PSEUDO_COLOR,
  GDK_VISUAL_TRUE_COLOR,
  GDK_VISUAL_DIRECT_COLOR
} GdkVisualType;

/* Window attribute mask values.
 *   GDK_WA_TITLE: The "title" field is valid.
 *   GDK_WA_X: The "x" field is valid.
 *   GDK_WA_Y: The "y" field is valid.
 *   GDK_WA_CURSOR: The "cursor" field is valid.
 *   GDK_WA_COLORMAP: The "colormap" field is valid.
 *   GDK_WA_VISUAL: The "visual" field is valid.
 */
typedef enum
{
  GDK_WA_TITLE    = 1 << 1,
  GDK_WA_X        = 1 << 2,
  GDK_WA_Y        = 1 << 3,
  GDK_WA_CURSOR   = 1 << 4,
  GDK_WA_COLORMAP = 1 << 5,
  GDK_WA_VISUAL   = 1 << 6
} GdkWindowAttributesType;

/* Size restriction enumeration.
 */
typedef enum
{
  GDK_MIN_SIZE  = 1 << 1,
  GDK_MAX_SIZE  = 1 << 2
} GdkWindowSize;

/* GC function types.
 *   Copy: Overwrites destination pixels with the source pixels.
 *   Invert: Inverts the destination pixels.
 *   Xor: Xor's the destination pixels with the source pixels.
 */
typedef enum
{
  GDK_COPY,
  GDK_INVERT,
  GDK_XOR
} GdkFunction;

/* GC fill types.
 *  Solid:
 *  Tiled:
 *  Stippled:
 *  OpaqueStippled:
 */
typedef enum
{
  GDK_SOLID,
  GDK_TILED,
  GDK_STIPPLED,
  GDK_OPAQUE_STIPPLED
} GdkFill;

/* GC line styles
 *  Solid:
 *  OnOffDash:
 *  DoubleDash:
 */
typedef enum
{
  GDK_LINE_SOLID,
  GDK_LINE_ON_OFF_DASH,
  GDK_LINE_DOUBLE_DASH
} GdkLineStyle;

/* GC cap styles
 *  CapNotLast:
 *  CapButt:
 *  CapRound:
 *  CapProjecting:
 */
typedef enum
{
  GDK_CAP_NOT_LAST,
  GDK_CAP_BUTT,
  GDK_CAP_ROUND,
  GDK_CAP_PROJECTING
} GdkCapStyle;

/* GC join styles
 *  JoinMiter:
 *  JoinRound:
 *  JoinBevel:
 */
typedef enum
{
  GDK_JOIN_MITER,
  GDK_JOIN_ROUND,
  GDK_JOIN_BEVEL
} GdkJoinStyle;

/* Cursor types.
 */
typedef enum
{
  GDK_LEFT_ARROW,
  GDK_RIGHT_ARROW,
  GDK_TEXT_CURSOR,
  GDK_DIRECTIONAL,
  GDK_PENCIL,
  GDK_CROSS,
  GDK_TCROSS,
  GDK_FLEUR,
  GDK_BI_ARROW_HORZ,
  GDK_BI_ARROW_VERT
} GdkCursorType;

/* Event types.
 *   Nothing: No event occurred.
 *   Delete: A window delete event was sent by the window manager.
 *           The specified window should be deleted.
 *   Destroy: A window has been destroyed.
 *   Expose: Part of a window has been uncovered.
 *   MotionNotify: The mouse has moved.
 *   ButtonPress: A mouse button was pressed.
 *   ButtonRelease: A mouse button was release.
 *   KeyPress: A key was pressed.
 *   KeyRelease: A key was released.
 *   EnterNotify: A window was entered.
 *   LeaveNotify: A window was exited.
 *   FocusChange: The focus window has changed. (The focus window gets
 *                keyboard events).
 *   Resize: A window has been resized.
 *   Map: A window has been mapped. (It is now visible on the screen).
 *   Unmap: A window has been unmapped. (It is no longer visible on
 *          the screen).
 */
typedef enum
{
  GDK_NOTHING         = -1,
  GDK_DELETE          = 0,
  GDK_DESTROY         = 1,
  GDK_EXPOSE          = 2,
  GDK_MOTION_NOTIFY   = 3,
  GDK_BUTTON_PRESS    = 4,
  GDK_2BUTTON_PRESS   = 5,
  GDK_3BUTTON_PRESS   = 6,
  GDK_BUTTON_RELEASE  = 7,
  GDK_KEY_PRESS       = 8,
  GDK_KEY_RELEASE     = 9,
  GDK_ENTER_NOTIFY    = 10,
  GDK_LEAVE_NOTIFY    = 11,
  GDK_FOCUS_CHANGE    = 12,
  GDK_RESIZE          = 13,
  GDK_MAP             = 14,
  GDK_UNMAP           = 15
} GdkEventType;

/* Event masks. (Used to select what types of events a window
 *  will receive).
 */
typedef enum
{
  GDK_EXPOSURE_MASK             = 1 << 1,
  GDK_POINTER_MOTION_MASK       = 1 << 2,
  GDK_POINTER_MOTION_HINT_MASK  = 1 << 3,
  GDK_BUTTON_MOTION_MASK        = 1 << 4,
  GDK_BUTTON1_MOTION_MASK       = 1 << 5,
  GDK_BUTTON2_MOTION_MASK       = 1 << 6,
  GDK_BUTTON3_MOTION_MASK       = 1 << 7,
  GDK_BUTTON_PRESS_MASK         = 1 << 8,
  GDK_BUTTON_RELEASE_MASK       = 1 << 9,
  GDK_KEY_PRESS_MASK            = 1 << 10,
  GDK_KEY_RELEASE_MASK          = 1 << 11,
  GDK_ENTER_NOTIFY_MASK         = 1 << 12,
  GDK_LEAVE_NOTIFY_MASK         = 1 << 13,
  GDK_FOCUS_CHANGE_MASK         = 1 << 14,
  GDK_STRUCTURE_MASK            = 1 << 15,
  GDK_ALL_EVENTS_MASK           = 0xFFFF
} GdkEventMask;

/* Types of enter/leave notifications.
 *   Ancestor:
 *   Virtual:
 *   Inferior:
 *   Nonlinear:
 *   NonlinearVirtual:
 *   Unknown: An unknown type of enter/leave event occurred.
 */
typedef enum
{
  GDK_NOTIFY_ANCESTOR           = 0,
  GDK_NOTIFY_VIRTUAL            = 1,
  GDK_NOTIFY_INFERIOR           = 2,
  GDK_NOTIFY_NONLINEAR          = 3,
  GDK_NOTIFY_NONLINEAR_VIRTUAL  = 4,
  GDK_NOTIFY_UNKNOWN            = 5
} GdkNotifyType;

/* Types of modifiers.
 */
typedef enum
{
  GDK_SHIFT_MASK    = 1 << 0,
  GDK_LOCK_MASK     = 1 << 1,
  GDK_CONTROL_MASK  = 1 << 2,
  GDK_MOD1_MASK     = 1 << 3,
  GDK_MOD2_MASK     = 1 << 4,
  GDK_MOD3_MASK     = 1 << 5,
  GDK_MOD4_MASK     = 1 << 6,
  GDK_MOD5_MASK     = 1 << 7,
  GDK_BUTTON1_MASK  = 1 << 8,
  GDK_BUTTON2_MASK  = 1 << 9,
  GDK_BUTTON3_MASK  = 1 << 10,
  GDK_BUTTON4_MASK  = 1 << 11,
  GDK_BUTTON5_MASK  = 1 << 12
} GdkModifierType;

typedef enum
{
  GDK_CLIP_BY_CHILDREN  = 0,
  GDK_INCLUDE_INFERIORS = 1
} GdkSubwindowMode;

typedef enum
{
  GDK_INPUT_READ       = 1 << 0,
  GDK_INPUT_WRITE      = 1 << 1,
  GDK_INPUT_EXCEPTION  = 1 << 2
} GdkInputCondition;

typedef enum
{
  GDK_OK          = 0,
  GDK_ERROR       = -1,
  GDK_ERROR_PARAM = -2,
  GDK_ERROR_FILE  = -3,
  GDK_ERROR_MEM   = -4
} GdkStatus;

typedef enum
{
  GDK_LSB_FIRST,
  GDK_MSB_FIRST
} GdkByteOrder;


typedef void (*GdkInputFunction) (gpointer          data,
				  gint              source,
				  GdkInputCondition condition);

/* The color type.
 *   A color consists of red, green and blue values in the
 *    range 0-65535 and a pixel value. The pixel value is highly
 *    dependent on the depth and colormap which this color will
 *    be used to draw into. Therefore, sharing colors between
 *    colormaps is a bad idea.
 */
struct _GdkColor
{
  guint32 pixel;
  guint16 red;
  guint16 green;
  guint16 blue;
};

/* The colormap type.
 *   Colormaps consist of 256 colors.
 */
struct _GdkColormap
{
  GdkColor colors[256];
};

/* The visual type.
 *   "type" is the type of visual this is (PseudoColor, TrueColor, etc).
 *   "depth" is the bit depth of this visual.
 *   "colormap_size" is the size of a colormap for this visual.
 *   "bits_per_rgb" is the number of significant bits per red, green and blue.
 *  The red, green and blue masks, shifts and precisions refer
 *   to value needed to calculate pixel values in TrueColor and DirectColor
 *   visuals. The "mask" is the significant bits within the pixel. The
 *   "shift" is the number of bits left we must shift a primary for it
 *   to be in position (according to the "mask"). "prec" refers to how
 *   much precision the pixel value contains for a particular primary.
 */
struct _GdkVisual
{
  GdkVisualType type;
  gint depth;
  gint colormap_size;
  gint bits_per_rgb;

  guint32 red_mask;
  gint red_shift;
  gint red_prec;

  guint32 green_mask;
  gint green_shift;
  gint green_prec;

  guint32 blue_mask;
  gint blue_shift;
  gint blue_prec;
};

struct _GdkWindowAttr
{
  gchar *title;
  gint event_mask;
  gint16 x, y;
  gint16 width;
  gint16 height;
  GdkWindowClass wclass;
  GdkVisual *visual;
  GdkColormap *colormap;
  GdkWindowType window_type;
  GdkCursor *cursor;
};

struct _GdkWindow
{
  GdkWindowType  window_type;
  GdkVisual     *visual;
  GdkColormap   *colormap;

  gint16   x;
  gint16   y;
  guint16  width;
  guint16  height;
  gint16   depth;

  GdkWindow  *parent;
  GdkWindow  *children;
  GdkWindow  *next_sibling;
  GdkWindow  *prev_sibling;

  gpointer user_data;
};

struct _GdkImage
{
  GdkImageType  type;
  GdkVisual    *visual;     /* visual used to create the image */
  GdkByteOrder  byte_order;
  guint16       width;
  guint16       height;
  guint16       depth;
  guint16       bpp;        /* bytes per pixel */
  guint16       bpl;        /* bytes per line */
  gpointer      mem;
};

struct _GdkGC
{
  GdkColor          foreground;
  GdkColor          background;
  GdkFont          *font;
  GdkFunction       function;
  GdkFill           fill;
  GdkPixmap        *tile;
  GdkPixmap        *stipple;
  GdkSubwindowMode  subwindow_mode;
  gint              graphics_exposures;
};

struct _GdkPoint
{
  gint16 x;
  gint16 y;
};

struct _GdkRectangle
{
  gint16 x;
  gint16 y;
  guint16 width;
  guint16 height;
};

struct _GdkSegment
{
  gint16 x1;
  gint16 y1;
  gint16 x2;
  gint16 y2;
};

struct _GdkFont
{
  gint ascent;
  gint descent;
};

struct _GdkCursor
{
  GdkCursorType type;
};

struct _GdkEventAny
{
  GdkEventType type;
  GdkWindow *window;
  gint8 send_event;
};

struct _GdkEventExpose
{
  GdkEventType type;
  GdkWindow *window;
  GdkRectangle area;
};

struct _GdkEventMotion
{
  GdkEventType type;
  GdkWindow *window;
  guint32 time;
  gint16 x;
  gint16 y;
  guint state;
  gint16 is_hint;
};

struct _GdkEventButton
{
  GdkEventType type;
  GdkWindow *window;
  guint32 time;
  gint16 x;
  gint16 y;
  guint state;
  guint button;
};

struct _GdkEventKey
{
  GdkEventType type;
  GdkWindow *window;
  guint32 time;
  guint state;
  guint keyval;
};

struct _GdkEventCrossing
{
  GdkEventType type;
  GdkWindow *window;
  GdkWindow *subwindow;
  GdkNotifyType detail;
};

struct _GdkEventFocus
{
  GdkEventType type;
  GdkWindow *window;
  gint16 in;
};

struct _GdkEventResize
{
  GdkEventType type;
  GdkWindow *window;
  gint16 width;
  gint16 height;
};

union _GdkEvent
{
  GdkEventType      type;
  GdkEventAny       any;
  GdkEventExpose    expose;
  GdkEventMotion    motion;
  GdkEventButton    button;
  GdkEventKey       key;
  GdkEventCrossing  crossing;
  GdkEventFocus     focus_change;
  GdkEventResize    resize;
};


#endif /* __GDK_TYPES_H__ */
