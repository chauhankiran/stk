#ifndef __GTK_DRAWING_AREA_H__
#define __GTK_DRAWING_AREA_H__


#include "gdk.h"
#include "gtktypes.h"


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


/* Drawing areas
 */
GtkWidget* gtk_drawing_area_new (gint             width,
				 gint             height,
				 GtkEventFunction event_function,
				 GdkEventMask     event_mask);

guint16 gtk_get_drawing_area_type (void);


#ifdef __cplusplus
}
#endif /* __cplusplus */


#endif /* __GTK_DRAWING_AREA_H__ */
