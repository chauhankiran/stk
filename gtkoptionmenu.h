#ifndef __GTK_OPTION_MENU_H__
#define __GTK_OPTION_MENU_H__


#include "gdk.h"
#include "gtktypes.h"


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


/* Option menus
 */
GtkWidget* gtk_option_menu_new         (void);
GtkWidget* gtk_option_menu_get_menu    (GtkWidget *option_menu);
void       gtk_option_menu_set_menu    (GtkWidget *option_menu,
					GtkWidget *menu);
void       gtk_option_menu_set_history (GtkWidget *option_menu,
					gint       index);

guint16 gtk_get_option_menu_type (void);


#ifdef __cplusplus
}
#endif /* __cplusplus */


#endif /* __GTK_OPTION_MENU_H__ */
