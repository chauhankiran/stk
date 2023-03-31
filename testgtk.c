#include "gtk.h"

int
main (int argc, char *argv[])
{
  GdkVisual *visual;
  GdkColormap *colormap;
  GtkWidget *window;
  GtkWidget *scrolled_win;
  GtkWidget *scrolled_area;
  GtkWidget *image_widget;
  GdkImage *image;
  gchar *mem, *p;
  gint i, j;

  gdk_set_debug_level (1);
  gdk_set_show_events (0);
  gtk_init (&argc, &argv);

  visual = gdk_visual_get_system ();
  colormap = gdk_colormap_get_system ();

  gtk_push_visual (visual);
  gtk_push_colormap (colormap);

  window = gtk_window_new ("Scroll Test", GTK_WINDOW_TOPLEVEL);

  scrolled_win = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_shadow_type (scrolled_win, GTK_SHADOW_IN);
  gtk_container_add (window, scrolled_win);

  image = gdk_image_new (GDK_IMAGE_FASTEST, visual, 512, 512);

  mem = image->mem;
  for (i = 0; i < image->height; i++)
    {
      p = mem;
      mem += image->bpl;

      for (j = 0; j < image->width; j++)
	{
	  switch (image->bpp)
	    {
	    case 4:
	      *p++ = j;
	    case 3:
	      *p++ = i + j;
	    case 2:
	      *p++ = i - j;
	    case 1:
	      *p++ = i * j;
	    }
	}
    }

  image_widget = gtk_image_new (image);
  scrolled_area = gtk_scrolled_window_get_scrolled_area (scrolled_win);
  gtk_container_add (scrolled_area, image_widget);

  gtk_widget_show (image_widget);
  gtk_widget_show (scrolled_win);
  gtk_widget_show (window);

  gtk_main ();

  return 0;
}
