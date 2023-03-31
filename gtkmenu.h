#ifndef __GTK_MENU_H__
#define __GTK_MENU_H__


#include "gdk.h"
#include "gtktypes.h"


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


/* Menus
 */
GtkWidget* gtk_menu_new             (void);
GtkWidget* gtk_menu_bar_new         (void);
GtkWidget* gtk_menu_item_new        (void);
GtkWidget* gtk_menu_toggle_item_new (GtkData *owner);

GtkWidget* gtk_menu_item_new_with_label  (gchar     *label);
GtkWidget* gtk_menu_item_new_with_image  (GdkImage  *image);
GtkWidget* gtk_menu_item_new_with_pixmap (GdkPixmap *pixmap);

GtkWidget* gtk_menu_toggle_item_new_with_label  (GtkData   *owner,
						 gchar     *label);
GtkWidget* gtk_menu_toggle_item_new_with_image  (GtkData   *owner,
						 GdkImage  *image);
GtkWidget* gtk_menu_toggle_item_new_with_pixmap (GtkData   *owner,
						 GdkPixmap *pixmap);

void     gtk_menu_item_set_submenu      (GtkWidget  *menu_item,
					 GtkWidget  *submenu);
GtkData* gtk_menu_item_get_state        (GtkWidget  *menu_item);
GtkData* gtk_menu_toggle_item_get_owner (GtkWidget  *menu_item);

guint16 gtk_get_menu_type             (void);
guint16 gtk_get_menu_bar_type         (void);
guint16 gtk_get_menu_item_type        (void);
guint16 gtk_get_menu_toggle_item_type (void);


#ifdef __cplusplus
}
#endif /* __cplusplus */


#endif /* __GTK_MENU_H__ */
