#ifndef __GTK_BUTTON_H__
#define __GTK_BUTTON_H__


#include "gdk.h"
#include "gtktypes.h"


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


/* Buttons
 */
GtkWidget* gtk_push_button_new   (void);
GtkWidget* gtk_toggle_button_new (GtkData *owner);
GtkWidget* gtk_radio_button_new (GtkData *owner);
GtkWidget* gtk_check_button_new (void);

void     gtk_button_reset     (GtkWidget *button);
GtkData* gtk_button_get_state (GtkWidget *button);
GtkData* gtk_button_get_owner (GtkWidget *button);

guint16 gtk_get_push_button_type   (void);
guint16 gtk_get_toggle_button_type (void);


#ifdef __cplusplus
}
#endif /* __cplusplus */


#endif /* __GTK_BUTTON_H__ */
