#ifndef __GTK_ENTRY_H__
#define __GTK_ENTRY_H__


#include "gtk.h"
#include "gtktypes.h"


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


typedef gint (*GtkKeyFunction) (guint keyval, guint state, gpointer client_data);


/* Text entry widgets
 */
GtkWidget* gtk_text_entry_new (void);

gchar* gtk_text_entry_get_text         (GtkWidget      *widget);
void   gtk_text_entry_set_text         (GtkWidget      *widget,
					gchar          *text);
void   gtk_text_entry_append_text      (GtkWidget      *widget,
					gchar          *text);
void   gtk_text_entry_prepend_text     (GtkWidget      *widget,
					gchar          *text);
void   gtk_text_entry_set_position     (GtkWidget      *widget,
					gint            position);
void   gtk_text_entry_set_key_function (GtkWidget      *widget,
					GtkKeyFunction  function,
					gpointer        data);

guint16 gtk_get_text_entry_type (void);


#ifdef __cplusplus
}
#endif /* __cplusplus */


#endif /* __GTK_ENTRY_H__ */
