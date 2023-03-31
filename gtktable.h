#ifndef __GTK_TABLE_H__
#define __GTK_TABLE_H__


#include "gdk.h"
#include "gtktypes.h"


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


/* Table geometry manager
 */

/* Create a new table with the specified number of rows and columns.
 *  If the "homogeneous" flag is set then every "block" in the table
 *  will be the same size. (All rows will be the same height and all
 *  columns will be the same width). If the "homogeneous" flag is not
 *  set then rows and columns will only grow as much as necessary to
 *  handle child size requests.
 */
GtkWidget* gtk_table_new (gint rows,
			  gint columns,
			  gint homogeneous);

void gtk_table_attach          (GtkWidget *table,
				GtkWidget *child,
				gint       left_attach,
				gint       right_attach,
				gint       top_attach,
				gint       bottom_attach,
				gint       xexpand,
				gint       xfill,
				gint       xpadding,
				gint       yexpand,
				gint       yfill,
				gint       ypadding);
void gtk_table_set_row_spacing (GtkWidget *table,
				gint       row,
				gint       spacing);
void gtk_table_set_col_spacing (GtkWidget *table,
				gint       col,
				gint       spacing);

guint16 gtk_get_table_type (void);


#ifdef __cplusplus
}
#endif /* __cplusplus */


#endif /* __GTK_TABLE_H__ */
