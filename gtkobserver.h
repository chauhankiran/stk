#ifndef __GTK_OBSERVER_H__ 
#define __GTK_OBSERVER_H__


#include "gdk.h"
#include "gtktypes.h"


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


/* Observer routines
 */
gint gtk_observer_update     (GtkObserver *observer,
			      GtkData     *data);
void gtk_observer_disconnect (GtkObserver *observer,
			      GtkData     *data);


#ifdef __cplusplus
}
#endif /* __cplusplus */


#endif /* __GTK_OBSERVER_H__ */
