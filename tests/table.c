#include "gtk.h"


static GtkWidget *hruler;
static GtkWidget *vruler;
static GtkDataAdjustment *hadj;
static GtkDataAdjustment *vadj;


gint
my_event_function (GtkWidget *widget,
		   GdkEvent  *event)
{
  gint x, y;

  switch (event->type)
    {
    case GDK_MOTION_NOTIFY:
      gdk_window_get_pointer (widget->window, &x, &y, NULL);

      x -= hruler->window->x;
      y -= vruler->window->y;

      if (x < 0) x = 0;
      if (y < 0) y = 0;
      if (x > hruler->window->width) x = hruler->window->width;
      if (y > vruler->window->height) y = vruler->window->height;

      hadj->value = x;
      vadj->value = y;

      gtk_data_notify ((GtkData*) hadj);
      gtk_data_notify ((GtkData*) vadj);

      break;
    }
  return FALSE;
}


int
main (int argc, char *argv[])
{
  GtkWidget *window;
  GtkWidget *table;
  GtkWidget *hsb;
  GtkWidget *vsb;
  GtkWidget *origin;
  GtkWidget *button;
  GtkWidget *label;
  GtkWidget *eventw;

  gdk_set_debug_level (0);
  gdk_set_show_events (0);
  gtk_init (&argc, &argv);

  window = gtk_window_new ("Table Test", GTK_WINDOW_TOPLEVEL);
  gtk_widget_set_uposition (window, 5, 5);

  eventw = gtk_event_widget_new (my_event_function, GDK_POINTER_MOTION_MASK, TRUE);
  gtk_container_add (window, eventw);

  table = gtk_table_new (3, 3, FALSE);
  gtk_container_add (eventw, table);

  gtk_table_set_row_spacing (table, 0, 0);
  gtk_table_set_row_spacing (table, 1, 2);
  gtk_table_set_col_spacing (table, 0, 0);
  gtk_table_set_col_spacing (table, 1, 2);

  hadj = (GtkDataAdjustment*) gtk_data_adjustment_new (50.0, 0.0, 200.0, 0.0, 0.0, 0.0);
  vadj = (GtkDataAdjustment*) gtk_data_adjustment_new (50.0, 0.0, 200.0, 0.0, 0.0, 0.0);

  hruler = gtk_hruler_new (hadj);
  vruler = gtk_vruler_new (vadj);

  hsb = gtk_hscrollbar_new (NULL);
  vsb = gtk_vscrollbar_new (NULL);

  origin = gtk_frame_new (NULL);
  gtk_frame_set_type (origin, GTK_SHADOW_OUT);
  gtk_container_set_border_width (origin, 0);

  button = gtk_frame_new (NULL);
  gtk_widget_set_usize (button, 200, 200);
  gtk_frame_set_type (button, GTK_SHADOW_IN);
  gtk_container_set_border_width (button, 0);
  label = gtk_label_new ("Hello World!");
  gtk_container_add (button, label);

  gtk_table_attach (table, hruler, 1, 2, 0, 1,
		    TRUE, TRUE, 0, FALSE, TRUE, 0);
  gtk_table_attach (table, hsb, 0, 2, 2, 3,
		    TRUE, TRUE, 0, FALSE, TRUE, 0);
  gtk_table_attach (table, vruler, 0, 1, 1, 2,
		    FALSE, TRUE, 0, TRUE, TRUE, 0);
  gtk_table_attach (table, vsb, 2, 3, 0, 2,
		    FALSE, TRUE, 0, TRUE, TRUE, 0);
  gtk_table_attach (table, button, 1, 2, 1, 2,
		    TRUE, TRUE, 0, TRUE, TRUE, 0);
  gtk_table_attach (table, origin, 0, 1, 0, 1,
		    FALSE, TRUE, 0, FALSE, TRUE, 0);

  gtk_container_set_border_width (table, 0);

  gtk_widget_show (hruler);
  gtk_widget_show (vruler);
  gtk_widget_show (hsb);
  gtk_widget_show (vsb);
  gtk_widget_show (origin);
  gtk_widget_show (label);
  gtk_widget_show (button);
  gtk_widget_show (table);
  gtk_widget_show (eventw);
  gtk_widget_show (window);

  gtk_main ();

  return 0;
}
