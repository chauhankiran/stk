#ifndef __GTK_FRAME_H__
#define __GTK_FRAME_H__


#include "gdk.h"
#include "gtktypes.h"


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


/* Frames
 */
GtkWidget* gtk_frame_new (gchar *label);

void gtk_frame_set_label       (GtkWidget     *widget,
				gchar         *label);
void gtk_frame_set_label_align (GtkWidget     *widget,
				gfloat         xalign,
				gfloat         yalign);
void gtk_frame_set_type        (GtkWidget     *widget,
				GtkShadowType  type);

guint16 gtk_get_frame_type (void);


#ifdef __cplusplus
}
#endif /* __cplusplus */


#endif /* __GTK_FRAME_H__ */
