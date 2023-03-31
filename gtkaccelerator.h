#ifndef __GTK_ACCELERATOR_H__
#define __GTK_ACCELERATOR_H__


#include "gtktypes.h"


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


/* Accelerator tables.
 */
GtkAcceleratorTable* gtk_accelerator_table_new (void);

void gtk_accelerator_table_destroy (GtkAcceleratorTable *table);
void gtk_accelerator_table_ref     (GtkAcceleratorTable *table);
void gtk_accelerator_table_unref   (GtkAcceleratorTable *table);


#ifdef __cplusplus
}
#endif /* __cplusplus */


#endif /* __GTK_ACCELERATOR_H__ */
