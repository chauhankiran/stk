#include "gtk.h"


int
main (int argc, char *argv[])
{
  GtkWidget *window;
  GtkWidget *vbox;
  GtkWidget *hbox;
  GtkWidget *scale;
  GtkDataAdjustment *adjustment;

  gdk_set_debug_level (0);
  gdk_set_show_events (0);
  gtk_init (&argc, &argv);

  window = gtk_window_new ("Test", GTK_WINDOW_TOPLEVEL);

  vbox = gtk_vbox_new (FALSE, 5);
  gtk_container_add (window, vbox);

  adjustment = (GtkDataAdjustment*) gtk_data_adjustment_new (5.0, -100.0, 100.0, 0.1, 1.0, 1.0);
  scale = gtk_hscale_new (adjustment);
  gtk_widget_set_usize (scale, 200, 0);
  gtk_scale_set_value_pos (scale, GTK_POS_TOP);
  gtk_box_pack (vbox, scale, FALSE, FALSE, 0, GTK_PACK_START);
  gtk_widget_show (scale);

  scale = gtk_hscale_new (adjustment);
  gtk_widget_set_usize (scale, 200, 0);
  gtk_scale_set_value_pos (scale, GTK_POS_BOTTOM);
  gtk_box_pack (vbox, scale, FALSE, FALSE, 0, GTK_PACK_START);
  gtk_widget_show (scale);

  hbox = gtk_hbox_new (FALSE, 5);
  gtk_box_pack (vbox, hbox, TRUE, TRUE, 0, GTK_PACK_START);
  gtk_container_set_border_width (hbox, 0);

  scale = gtk_vscale_new (adjustment);
  gtk_widget_set_usize (scale, 0, 200);
  gtk_scale_set_value_pos (scale, GTK_POS_LEFT);
  gtk_box_pack (hbox, scale, TRUE, FALSE, 0, GTK_PACK_START);
  gtk_widget_show (scale);

  scale = gtk_vscale_new (adjustment);
  gtk_widget_set_usize (scale, 0, 200);
  gtk_scale_set_value_pos (scale, GTK_POS_RIGHT);
  gtk_box_pack (hbox, scale, TRUE, FALSE, 0, GTK_PACK_START);
  gtk_widget_show (scale);

  gtk_widget_show (hbox);
  gtk_widget_show (vbox);
  gtk_widget_show (window);

  gtk_main ();

  return 0;
}
