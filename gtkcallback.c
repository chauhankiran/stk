#include "gtkcallback.h"
#include "gtkdata.h"


typedef struct _GtkCallbackRecord  GtkCallbackRecord;

struct _GtkCallbackRecord
{
  gint tag;
  GtkData *data;
  GtkObserver observer;
  GtkCallback callback;
  gpointer user_data;
};


static gint gtk_callback_update     (GtkObserver *observer,
				     GtkData     *data);
static void gtk_callback_disconnect (GtkObserver *observer,
				     GtkData     *data);


static GList *callbacks;
static gint next_tag = 1;


gint 
gtk_callback_add (GtkData     *data,
		  GtkCallback  callback,
		  gpointer     user_data)
{
  GtkCallbackRecord *record;
  
  g_function_enter ("gtk_callback_add");

  g_assert (data != NULL);
  g_assert (callback != NULL);
  g_assert (data->type == gtk_data_int_type ());
  
  record = g_new (GtkCallbackRecord, 1);
  record->tag = next_tag++;
  record->data = data;
  record->observer.update = gtk_callback_update;
  record->observer.disconnect = gtk_callback_disconnect;
  record->observer.user_data = record;
  record->callback = callback;
  record->user_data = user_data;

  gtk_data_attach (data, &record->observer);

  g_function_leave ("gtk_callback_add");
  return record->tag;
}

void 
gtk_callback_remove (gint tag)
{
  GtkCallbackRecord *record;
  GList *temp_list;
  GList *list;
  
  g_function_enter ("gtk_callback_remove");

  list = callbacks;
  while (list)
    {
      record = list->data;
      
      if (record->tag == tag)
	{
	  temp_list = list;
	  if (temp_list->next)
	    temp_list->next->prev = temp_list->prev;
	  if (temp_list->prev)
	    temp_list->prev->next = temp_list->next;

	  if (list == temp_list)
	    list = temp_list->next;
	  
	  temp_list->next = NULL;
	  temp_list->prev = NULL;
	  g_list_free (temp_list);

	  gtk_data_detach (record->data, &record->observer);
	  g_free (record);
	  break;
	}
    }

  g_function_leave ("gtk_callback_remove");
}

static gint 
gtk_callback_update (GtkObserver *observer,
		     GtkData     *data)
{
  GtkCallbackRecord *record;
  GtkDataInt *int_data;
  
  g_function_enter ("gtk_callback_update");
  
  g_assert (observer != NULL);
  g_assert (data != NULL);

  record = observer->user_data;
  int_data = (GtkDataInt*) data;

  if (int_data->value == GTK_STATE_ACTIVATED)
    (* record->callback) (NULL, record->user_data, data);
  
  g_function_leave ("gtk_callback_update");
  return FALSE;
}

static void 
gtk_callback_disconnect (GtkObserver *observer,
			 GtkData     *data)
{
  GtkCallbackRecord *record;
  
  g_function_enter ("gtk_callback_disconnect");
  
  g_assert (observer != NULL);
  g_assert (data != NULL);

  record = observer->user_data;
  
  gtk_callback_remove (record->tag);
  
  g_function_leave ("gtk_callback_disconnect");
}
