#ifndef __GTK_SCROLLBAR_H__
#define __GTK_SCROLLBAR_H__


#include "gdk.h"
#include "gtktypes.h"


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


/* Scrollbars
 */
GtkWidget* gtk_hscrollbar_new (GtkDataAdjustment *adjustment);
GtkWidget* gtk_vscrollbar_new (GtkDataAdjustment *adjustment);

GtkData* gtk_scrollbar_get_adjustment (GtkWidget *scrollbar);

guint16 gtk_get_scrollbar_type (void);


#ifdef __cplusplus
}
#endif /* __cplusplus */


#endif /* __GTK_SCROLLBAR_H__ */
