#ifndef __GTK_WINDOW_H__
#define __GTK_WINDOW_H__


#include "gdk.h"
#include "gtktypes.h"


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


/* Types of windows.
 *   Main: These are the toplevel windows that one might
 *         use for the main toolbox or the document window.
 *   Dialog: Transient windows which some window managers
 *           deal with specially.
 *   Popup: Windows which bypass the window manager. These
 *          should normally not be used. In fact, the only
 *          use for which they are needed is menus.
 */
typedef enum
{
  GTK_WINDOW_TOPLEVEL,
  GTK_WINDOW_DIALOG,
  GTK_WINDOW_POPUP
} GtkWindowType;


/* Windows
 */
GtkWidget* gtk_window_new (gchar         *title,
			   GtkWindowType  type);

void gtk_window_set_focus             (GtkWidget           *widget,
				       GtkWidget           *focus);
void gtk_window_set_default           (GtkWidget           *widget,
				       GtkWidget           *defaultw);
void gtk_window_add_accelerator_table (GtkWidget           *widget,
				       GtkAcceleratorTable *table);

guint16 gtk_get_window_type (void);


#ifdef __cplusplus
}
#endif /* __cplusplus */


#endif /* __GTK_WINDOW_H__ */
