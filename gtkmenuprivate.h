#ifndef __GTK_MENU_PRIVATE_H__
#define __GTK_MENU_PRIVATE_H__


#include "gtktypes.h"


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


#define SHIFT_TEXT     "Shft"
#define CONTROL_TEXT   "Ctl"
#define ALT_TEXT       "Alt"
#define SEPARATOR      "+"

#define DIRECTION_LEFT        0
#define DIRECTION_RIGHT       1
#define TOGGLE_MARK_SIZE      8
#define TOGGLE_MARK_SPACING   2

#define OPTION_INDICATOR_WIDTH    12
#define OPTION_INDICATOR_HEIGHT   8
#define OPTION_INDICATOR_SPACING  2


typedef struct _GtkMenu            GtkMenu;
typedef struct _GtkMenuBar         GtkMenuBar;
typedef struct _GtkMenuItem        GtkMenuItem;
typedef struct _GtkMenuToggleItem  GtkMenuToggleItem;
typedef struct _GtkOptionMenu  GtkOptionMenu;


struct _GtkMenu
{
  GtkContainer container;

  GtkWidget *frame;
  GtkWidget *window;
  GtkWidget *parent;
  GtkWidget *old_active_menu_item;
  GtkWidget *active_menu_item;

  GList *children;
};

struct _GtkMenuBar
{
  GtkContainer container;

  GList *children;
  GtkWidget *active_menu_item;
  gint8 active;
};

struct _GtkMenuItem
{
  GtkContainer container;

  GtkWidget *child;
  GtkWidget *submenu;

  gint16 accelerator_size;
  gchar  accelerator_key;
  guint8 accelerator_mods;
  gint8  previous_state;

  unsigned int submenu_direction : 1;
  unsigned int toggle_exists     : 1;
  unsigned int toggle_draw       : 1;

  GtkDataInt state;
  GtkObserver state_observer;
};

struct _GtkMenuToggleItem
{
  GtkMenuItem menu_item;

  GtkDataWidget *owner;
  GtkObserver owner_observer;
};

struct _GtkOptionMenu
{
  GtkContainer container;

  GtkWidget *child;
  GtkWidget *menu_item;
  GtkWidget *menu;

  guint16 width;
  guint16 height;

  unsigned int in_option_menu     : 1;
  unsigned int update_contents    : 1;
  unsigned int menu_toggle_exists : 1;
  unsigned int ask_for_resize     : 1;
};


GList*     gtk_menu_get_children   (GtkWidget *menu);
GtkWidget* gtk_menu_get_active     (GtkWidget *menu);
void       gtk_menu_set_active     (GtkWidget *menu,
				    gint       index);
GtkWidget* gtk_menu_item_get_child (GtkWidget *menu_item);

void gtk_option_menu_position (GtkWidget *option_menu, gint *x, gint *y, gint width, gint height);


#ifdef __cplusplus
}
#endif /* __cplusplus */


#endif /* __GTK_MENU_PRIVATE_H__ */
