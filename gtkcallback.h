#ifndef __GTK_CALLBACK_H__
#define __GTK_CALLBACK_H__


#include "gtktypes.h"


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


/* Callback routines.
 */
gint gtk_callback_add    (GtkData     *data,
			  GtkCallback  callback,
			  gpointer     user_data);
void gtk_callback_remove (gint         tag);
			      

#ifdef __cplusplus
}
#endif /* __cplusplus */


#endif /* __GTK_CALLBACK_H__ */
