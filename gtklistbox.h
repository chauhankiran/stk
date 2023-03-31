#ifndef __GTK_LISTBOX_H__
#define __GTK_LISTBOX_H__


#include "gdk.h"
#include "gtktypes.h"


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


/* Listboxes
 */
GtkWidget* gtk_listbox_new (void);

GtkWidget* gtk_listbox_get_list        (GtkWidget     *listbox);
GtkWidget* gtk_listbox_get_hscrollbar  (GtkWidget     *listbox);
GtkWidget* gtk_listbox_get_vscrollbar  (GtkWidget     *listbox);
void       gtk_listbox_set_shadow_type (GtkWidget     *listbox,
					GtkShadowType  type);

guint16 gtk_get_listbox_type (void);


#ifdef __cplusplus
}
#endif /* __cplusplus */


#endif /* __GTK_LISTBOX_H__ */
