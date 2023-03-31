#include "gtk.h"


typedef struct _MenuItem   MenuItem;

struct _MenuItem
{
  char *label;
  char  accelerator_key;
  int   accelerator_mods;
};

MenuItem file_items[] =
{
  { "New",            'N',   GDK_CONTROL_MASK },
  { "Open",           'O',   GDK_CONTROL_MASK },
  { "Preferences...",  0,    0 },
  { "-",               0,    0 },
  { "Quit",           'Q',   GDK_CONTROL_MASK },
};

int nmenu_items[] =
{
  sizeof (file_items) / sizeof (file_items[0])
};

MenuItem *menus[] =
{
  file_items,
};

GtkWidget *menu;


GtkWidget*
create_menu ()
{
  GtkStyle *style;
  GdkFont *font;
  GtkWidget *menu;
  GtkWidget *menu_item;
  gint i, j;

  font = gdk_font_load ("-Adobe-Helvetica-Bold-R-Normal--*-120-*-*-*-*-*-*");
  style = gtk_style_new (-1);
  style->font = font;
  gtk_push_style (style);

  menu = gtk_menu_new ();

  for (i = 0, j = 0; j < nmenu_items[i]; j++)
    {
      if (menus[i][j].label[0] == '-')
	{
	  menu_item = gtk_menu_item_new ();
	}
      else
	{
	  menu_item = gtk_menu_item_new_with_label (menus[i][j].label);
	}

      gtk_container_add (menu, menu_item);
      gtk_widget_show (menu_item);
    }

  gtk_pop_style ();
  return menu;
}

gint
handle_event (GtkWidget *widget,
	      GdkEvent  *event)
{
  gint x, y;

  switch (event->type)
    {
    case GDK_BUTTON_PRESS:
      gdk_window_get_pointer (NULL, &x, &y, NULL);
      gtk_widget_popup (menu, x, y);
      break;

    case GDK_BUTTON_RELEASE:
      break;

    default:
      break;
    }

  return FALSE;
}

int
main (int argc, char *argv[])
{
  GtkWidget *window;
  GtkWidget *drawing_area;

  gdk_set_debug_level (0);
  gdk_set_show_events (0);
  gtk_init (&argc, &argv);

  window = gtk_window_new ("Popup Test", GTK_WINDOW_TOPLEVEL);

  menu = create_menu ();

  drawing_area = gtk_drawing_area_new (200, 200, handle_event,
				       GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK);
  gtk_container_add (window, drawing_area);

  gtk_widget_show (drawing_area);
  gtk_widget_show (window);

  gtk_main ();

  return 0;
}
