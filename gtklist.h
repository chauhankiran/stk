#ifndef __GTK_LIST_H__
#define __GTK_LIST_H__


#include "gdk.h"
#include "gtktypes.h"


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


/* Types of selections that listboxes support.
 */
typedef enum
{
  GTK_SELECTION_SINGLE,
  GTK_SELECTION_BROWSE,
  GTK_SELECTION_MULTIPLE,
  GTK_SELECTION_EXTENDED
} GtkSelectionMode;


/* Lists
 */
GtkWidget* gtk_list_new                 (GtkDataAdjustment *hadjustment,
					 GtkDataAdjustment *vadjustment);
GtkWidget* gtk_list_item_new            (void);

GtkWidget* gtk_list_item_new_with_label (gchar *label);

void  gtk_list_insert_items       (GtkWidget        *list,
				   GList            *items,
				   gint              position);
void  gtk_list_append_items       (GtkWidget        *list,
				   GList            *items);
void  gtk_list_prepend_items      (GtkWidget        *list,
				   GList            *items);
void  gtk_list_remove_items       (GtkWidget        *list,
				   GList            *items);
void  gtk_list_clear_items        (GtkWidget        *list,
				   gint              start,
				   gint              end);
void  gtk_list_set_selection_mode (GtkWidget        *list,
				   GtkSelectionMode  mode);
void  gtk_list_get_list_size      (GtkWidget        *list,
				   gint             *width,
				   gint             *height);
GtkData* gtk_list_get_hadjustment (GtkWidget        *list);
GtkData* gtk_list_get_vadjustment (GtkWidget        *list);
GtkData* gtk_list_item_get_state  (GtkWidget        *item);

void   gtk_list_select_item   (GtkWidget   *list,
			       gint         item);
void   gtk_list_unselect_item (GtkWidget   *list,
			       gint         item);
gint*  gtk_list_get_selected  (GtkWidget   *list,
			       gint        *nitems);

guint16 gtk_get_list_type      (void);
guint16 gtk_get_list_item_type (void);


#ifdef __cplusplus
}
#endif /* __cplusplus */


#endif /* __GTK_LIST_H__ */
