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
#ifndef __GTK_TYPES_H__
#define __GTK_TYPES_H__


/* Include the "gdk" type definitions.
 * These are needed because "gdk" correctly defines types
 *  for the different integer sizes. (No need to repeat
 *  the effort).
 */

#include "glib.h"
#include "gdktypes.h"
#include "config.h"


/* Define a macro to clamp a value within an interval.
 *  (Useful for doing alignments).
 */

#define CLAMP(x,low,high)  (((x) > (high)) ? (high) : (((x) < (low)) ? (low) : (x)))

/* The flags that are used in the flags member of the widget
 *  structure.
 */

#define GTK_CONTAINER         0x0001
#define GTK_VISIBLE           0x0002
#define GTK_MAPPED            0x0004
#define GTK_UNMAPPED          0x0008
#define GTK_REALIZED          0x0010
#define GTK_SENSITIVE         0x0020
#define GTK_PARENT_SENSITIVE  0x0040
#define GTK_NO_WINDOW         0x0080
#define GTK_HAS_FOCUS         0x0100
#define GTK_CAN_FOCUS         0x0200
#define GTK_HAS_DEFAULT       0x0400
#define GTK_CAN_DEFAULT       0x0800
#define GTK_IN_CALL           0x1000
#define GTK_NEED_DESTROY      0x2000

/* A few macros for accessing the type and flags members of
 *  the widget structure.
 */

#define GTK_WIDGET_TYPE(obj)              (((GtkWidget*) (obj))->type)
#define GTK_WIDGET_FLAGS(obj)             (((GtkWidget*) (obj))->flags)
#define GTK_WIDGET_CONTAINER(obj)         (GTK_WIDGET_FLAGS (obj) & GTK_CONTAINER)
#define GTK_WIDGET_VISIBLE(obj)           (GTK_WIDGET_FLAGS (obj) & GTK_VISIBLE)
#define GTK_WIDGET_MAPPED(obj)            (GTK_WIDGET_FLAGS (obj) & GTK_MAPPED)
#define GTK_WIDGET_UNMAPPED(obj)          (GTK_WIDGET_FLAGS (obj) & GTK_UNMAPPED)
#define GTK_WIDGET_REALIZED(obj)          (GTK_WIDGET_FLAGS (obj) & GTK_REALIZED)
#define GTK_WIDGET_SENSITIVE(obj)         (GTK_WIDGET_FLAGS (obj) & GTK_SENSITIVE)
#define GTK_WIDGET_PARENT_SENSITIVE(obj)  (GTK_WIDGET_FLAGS (obj) & GTK_PARENT_SENSITIVE)
#define GTK_WIDGET_IS_SENSITIVE(obj)      ((GTK_WIDGET_SENSITIVE (obj) && \
                                            GTK_WIDGET_PARENT_SENSITIVE (obj)) != 0)
#define GTK_WIDGET_NO_WINDOW(obj)         (GTK_WIDGET_FLAGS (obj) & GTK_NO_WINDOW)
#define GTK_WIDGET_HAS_FOCUS(obj)         (GTK_WIDGET_FLAGS (obj) & GTK_HAS_FOCUS)
#define GTK_WIDGET_CAN_FOCUS(obj)         (GTK_WIDGET_FLAGS (obj) & GTK_CAN_FOCUS)
#define GTK_WIDGET_HAS_DEFAULT(obj)       (GTK_WIDGET_FLAGS (obj) & GTK_HAS_DEFAULT)
#define GTK_WIDGET_CAN_DEFAULT(obj)       (GTK_WIDGET_FLAGS (obj) & GTK_CAN_DEFAULT)
#define GTK_WIDGET_IN_CALL(obj)           (GTK_WIDGET_FLAGS (obj) & GTK_IN_CALL)
#define GTK_WIDGET_NEED_DESTROY(obj)      (GTK_WIDGET_FLAGS (obj) & GTK_NEED_DESTROY)

/* Two macros for setting and unsetting flags.
 */
#define GTK_WIDGET_SET_FLAGS(obj,flag)    (((GtkWidget*) (obj))->flags |= (flag))
#define GTK_WIDGET_UNSET_FLAGS(obj,flag)  (((GtkWidget*) (obj))->flags &= ~(flag))


/* Type definitions for the basic structures.
 */

typedef struct _GtkRequisition         GtkRequisition;
typedef struct _GtkAllocation          GtkAllocation;
typedef struct _GtkStyle               GtkStyle;
typedef struct _GtkWidget              GtkWidget;
typedef struct _GtkContainer           GtkContainer;
typedef struct _GtkWidgetFunctions     GtkWidgetFunctions;
typedef struct _GtkContainerFunctions  GtkContainerFunctions;

/* Data/observer type definiitions.
 */

typedef struct _GtkData                GtkData;
typedef struct _GtkObserver            GtkObserver;

typedef struct _GtkDataInt             GtkDataInt;
typedef struct _GtkDataFloat           GtkDataFloat;
typedef struct _GtkDataAdjustment      GtkDataAdjustment;
typedef struct _GtkDataWidget          GtkDataWidget;
typedef struct _GtkDataList            GtkDataList;

/* Accelerator table type definitions.
 */

typedef struct _GtkAcceleratorEntry    GtkAcceleratorEntry;
typedef struct _GtkAcceleratorTable    GtkAcceleratorTable;


/* Function type definitions.
 */
typedef gint (*GtkFunction)         (gpointer  data);
typedef void (*GtkCallback)         (GtkWidget *widget,
				     gpointer   client_data,
				     gpointer   call_data);
typedef gint (*GtkEventFunction)    (GtkWidget *widget,
				     GdkEvent  *event);


/* The various types of shadows.
 */
typedef enum
{
  GTK_SHADOW_NONE,
  GTK_SHADOW_IN,
  GTK_SHADOW_OUT,
  GTK_SHADOW_ETCHED_IN,
  GTK_SHADOW_ETCHED_OUT
} GtkShadowType;

/* The various directions of arrows.
 */
typedef enum
{
  GTK_ARROW_UP,
  GTK_ARROW_DOWN,
  GTK_ARROW_LEFT,
  GTK_ARROW_RIGHT
} GtkArrowType;

/* States which an widget can be in. These states
 *  determine the highlighting to be used during
 *  drawing. (Prelighting is the highlighting done
 *  when the mouse passes over a button).
 */
typedef enum
{
  GTK_STATE_NORMAL,
  GTK_STATE_ACTIVE,
  GTK_STATE_PRELIGHT,
  GTK_STATE_SELECTED,
  GTK_STATE_INSENSITIVE,
  GTK_STATE_PREVIOUS,
  GTK_STATE_ACTIVATED,
  GTK_STATE_DEACTIVATED
} GtkStateType;

/* Directions.
 */
typedef enum
{
  GTK_DIR_UP,
  GTK_DIR_DOWN,
  GTK_DIR_LEFT,
  GTK_DIR_RIGHT,
  GTK_DIR_TAB_FORWARD,
  GTK_DIR_TAB_BACKWARD
} GtkDirectionType;

/* Callbacks.
 */
typedef enum
{
  GTK_CALLBACK_DESTROY,
  GTK_CALLBACK_MAP,
  GTK_CALLBACK_UNMAP,
  GTK_CALLBACK_ICONIFY
} GtkCallbackType;


struct _GtkRequisition
{
  /* The width on widget requires.
   */
  guint16 width;

  /* The height on widget requires.
   */
  guint16 height;
};

struct _GtkAllocation
{
  /* The x, y location the widget was assigned.
   */
  gint16 x;
  gint16 y;

  /* The width, height the widget was allocated.
   */
  guint16 width;
  guint16 height;
};

struct _GtkStyle
{
  /* Foreground and background colors and font specified
   *  for this style. The font is set for all of the gc's
   *  so that any text drawn with any of the gc's will use
   *  the specified font. If the font is NULL, the default
   *  font will be used.
   */
  GdkColor  foreground[5];
  GdkColor  background[5];
  GdkColor  highlight[5];
  GdkColor  shadow[5];
  GdkFont  *font;

  /* The thickness of shadows.
   */
  gint shadow_thickness;

  /* There are 2 defined foreground colors.
   * The first one is used when an widget is sensitive.
   * The second one is used when an widget is insensitive.
   * The difference between the 2 colors is that the
   *  insensitive version is lighter.
   */

  /* There are 5 defined colors.
   * These colors correspond to the GTK_NORMAL,
   *  GTK_ACTIVE, GTK_PRELIGHT, GTK_SELECTED
   *  and GTK_INSENSITIVE states.
   * The active state color is a darker version of
   *  the normal state color and the prelight state
   *  color is a lighter version. The selected state
   *  is a different color and is used for listbox
   *  selections and the like. The insensitive state
   *  is used for coloring insensitive widgets.
   * The 'highlight_gc' and 'shadow_gc' members
   *  correspond to the highlight and shadow colors
   *  that should be used in a particular state.
   */
  GdkGC *foreground_gc[5];
  GdkGC *background_gc[5];
  GdkGC *highlight_gc[5];
  GdkGC *shadow_gc[5];

  /* The depth the gc's for this style can be used with. Styles can
   *  be used in multiple windows if the depths of the various
   *  windows is the same. This member specifies the depth of
   *  the windows for which the gc's are valid.
   */
  gint depth;

  /* The colormap associated with this style. A styles gc's are
   *  only valid for this colormap due to the colors having been
   *  allocated for it (and possibly in it). Using the styles gc's
   *  in another colormap may produce incorrect results.
   */
  GdkColormap *colormap;

  /* The reference count for this style. Widgets reference a style
   *  with "gtk_style_ref" which increments the reference count and
   *  decrement the reference count with "gtk_style_unref". A style
   *  can be referenced by a widget without it being attached to
   *  that widget.
   */
  gint ref_count;

  /* The attachment count for this style. Widgets attach to a style
   *  with "gtk_style_attach" which increments the attachment count
   *  and detach with "gtk_style_detach" which decrements the
   *  attachment count.
   */
  gint attach_count;
};

struct _GtkWidget
{
  /* The widget flags specify whether an widget is visible,
   *  mapped, has its own window, etc.
   * The difference between being visible and being mapped
   *  is that visibility determines whether the widget is
   *  considered during geometry negotiation. Mapping
   *  determines if the widget is actually displayed on
   *  screen.
   */
  guint16 flags;

  /* The type of widget. When a new type of widget is created
   *  for the first time it should call "gtk_get_unique_type"
   *  to get a unique value to be used in this field.
   */
  guint16 type;

  /* Every widget has a style which determines the colors
   *  and font it uses for drawing.
   */
  GtkStyle *style;

  /* The parent container of this widget
   */
  GtkContainer *parent;

  /* The widgets requested size.
   */
  GtkRequisition requisition;

  /* The widgets allocated size and location.
   */
  GtkAllocation allocation;

  /* The user specified allocated size and location.
   *  This overrides any parent allocated position and
   *  any widget specified requisition.
   */
  GtkAllocation user_allocation;

  /* The GdkWindow this widget is associated with. (Or this
   *  is the widget that the GdkWindow is associated with).
   *  This may be shared between parents and children.
   */
  GdkWindow *window;

  /* Hook onto which a user can place any data he/she wishes.
   */
  gpointer user_data;

  /* The function table holds pointers to the various functions
   *  this widget uses. The reason for using a function table
   *  instead of many individual function pointers is that there
   *  need only be 1 function table per widget type. This cuts
   *  the memory overhead of widgets down dramatically.
   */
  GtkWidgetFunctions *function_table;
};

struct _GtkContainer
{
  /* Subclass of widget. It therefore needs to handle to size request
   *  and size allocations as well as destruction requests.
   */
  GtkWidget widget;

  /* The width of the border which separates the edge
   *  of the container from its children.
   */
  gint border_width;

  /* The current focus child of this container. This widget
   *  might not actually have the focus, in which case it
   *  is taken to be the next child to receive focus when
   *  focus is given to the container.
   */
  gpointer focus_child;

  /* The function table for containers serves a similar purpose
   *  as it does for widgets. (ie. Read the comment above on
   *  widget function tables).
   */
  GtkContainerFunctions *function_table;
};

struct _GtkWidgetFunctions
{
  void  (* destroy)             (GtkWidget *);
  void  (* show)                (GtkWidget *);
  void  (* hide)                (GtkWidget *);
  void  (* map)                 (GtkWidget *);
  void  (* unmap)               (GtkWidget *);
  void  (* realize)             (GtkWidget *);
  void  (* draw)                (GtkWidget *, GdkRectangle *, gint);
  void  (* draw_focus)          (GtkWidget *);
  gint  (* event)               (GtkWidget *, GdkEvent *);
  void  (* size_request)        (GtkWidget *, GtkRequisition *);
  void  (* size_allocate)       (GtkWidget *, GtkAllocation *);
  gint  (* is_child)            (GtkWidget *, GtkWidget *);
  gint  (* locate)              (GtkWidget *, GtkWidget **, gint, gint);
  void  (* activate)            (GtkWidget *);
  void  (* set_state)           (GtkWidget *, GtkStateType);
  gint  (* install_accelerator) (GtkWidget *, gchar, guint8);
  void  (* remove_accelerator)  (GtkWidget *);
};

struct _GtkContainerFunctions
{
  void (* add)           (GtkContainer *, GtkWidget *);
  void (* remove)        (GtkContainer *, GtkWidget *);
  void (* need_resize)   (GtkContainer *, GtkWidget *);
  void (* focus_advance) (GtkContainer *, GtkWidget **, GtkDirectionType);
  void (* foreach)       (GtkContainer *, GtkCallback, gpointer);
};

struct _GtkData
{
  /* The type of data.
   */
  guint type;

  /* The list of observers that are attached to
   *  this data object.
   */
  GList *observers;

  /* True if the data object is currently handling
   *  a call.
   */
  gint8 in_call;

  /* True if the data object should be destroyed after
   *  it finished handling the current call.
   */
  gint8 need_destroy;
};

struct _GtkObserver
{
  /* Receive an update as to the state of a data
   *  object. If the update function returns TRUE
   *  then it has changed the data and is requesting
   *  another update. The current notification in process
   *  will be terminated and a new one will be started.
   *  Returning FALSE causes the notification in process
   *  to continue.
   */
  gint (* update) (GtkObserver *observer, GtkData *data);

  /* Receive notice that the observer has been
   *  disconnected from a data object. This only
   *  happens because the data object has been
   *  destroyed.
   */
  void (* disconnect) (GtkObserver *observer, GtkData *data);

  gpointer user_data;
};

struct _GtkDataInt
{
  /* An integer data type. (Useful for buttons
   */
  GtkData data;

  gint value;
};

struct _GtkDataFloat
{
  /* A float data type.
   */
  GtkData data;

  gfloat value;
};

struct _GtkDataAdjustment
{
  /* An adjustment data type. (Useful for scrollbars and scales).
   */
  GtkData data;

  /* An adjustment has a "value" and a "lower" and "upper" bounds.
   * It also has 2 increment values. One for stepping and one for
   *  paging. (Normally the paging increment is larger than the
   *  stepping increment).
   * Lastly, it has the page size. Note that this may be different
   *  than the page increment.
   */
  gfloat value;
  gfloat lower;
  gfloat upper;

  gfloat step_increment;
  gfloat page_increment;

  gfloat page_size;
};

struct _GtkDataWidget
{
  /* A widget data type. (Useful for radio buttons).
   */
  GtkData data;

  GtkWidget *widget;
};

struct _GtkDataList
{
  /* A list data type. (Useful for lists...big surprise).
   */
  GtkData data;

  GList *list;
};

struct _GtkAcceleratorEntry
{
  /* The modifiers that need to be active for this
   *  accelerator to be activated.
   */
  guint8 modifiers;

  /* The widget this accelerator is bound to.
   *  When the accelerator is activated it will
   *  call "gtk_widget_activate" on this widget.
   */
  GtkWidget *widget;
};

struct _GtkAcceleratorTable
{
  GList *entries[69];
  gint ref_count;
};


#endif /* __GTK_TYPES_H__ */
