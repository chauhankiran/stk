#ifndef __GTK_EVENT_H__
#define __GTK_EVENT_H__


#include "gtktypes.h"

/* Event widget
 */
GtkWidget* gtk_event_widget_new (GtkEventFunction event_function,
				 GdkEventMask     event_mask,
				 gint             create_window);

guint16 gtk_get_event_widget_type (void);


#endif /* __GTK_EVENT_H__ */
