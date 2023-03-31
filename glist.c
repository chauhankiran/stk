#include "glib.h"


static GMemChunk *list_mem_chunk = NULL;
static GList* free_list = NULL;


GList*
g_list_alloc ()
{
  GList *new_list;

  g_function_enter ("g_list_alloc");

  if (free_list)
    {
      new_list = free_list;
      free_list = free_list->next;
    }
  else
    {
      if (!list_mem_chunk)
	list_mem_chunk = g_mem_chunk_new ("list mem chunk", sizeof (GList), 64 * 1024, G_ALLOC_ONLY);
      
      new_list = g_chunk_new (GList, list_mem_chunk);
    }
  
  new_list->data = NULL;
  new_list->next = NULL;
  new_list->prev = NULL;

  g_function_leave ("g_list_alloc");
  return new_list;
}

void 
g_list_free (GList *list)
{
  GList *last;

  g_function_enter ("g_list_free");

  if (list)
    {
      last = g_list_last (list);
      last->next = free_list;
      free_list = list;
    }

  g_function_leave ("g_list_free");
}

GList* 
g_list_append (GList    *list, 
	       gpointer  data)
{
  GList *new_list;
  GList *last;

  g_function_enter ("g_list_append");

  new_list = g_list_alloc ();
  new_list->data = data;

  if (!list)
    {
      list = new_list;
    }
  else
    {
      last = g_list_last (list);
      if (last)
        {
          last->next = new_list;
          new_list->prev = last;
        }
    }

  g_function_leave ("g_list_append");
  return list;
}

GList* 
g_list_prepend (GList    *list, 
		gpointer  data)
{
  GList *new_list;

  g_function_enter ("g_list_prepend");

  new_list = g_list_alloc ();
  new_list->data = data;

  if (list)
    {
      if (list->prev)
	list->prev->next = new_list;
      new_list->prev = list->prev;
      list->prev = new_list;
    }
  new_list->next = list;

  g_function_leave ("g_list_prepend");
  return new_list;
}

GList* 
g_list_remove (GList    *list, 
	       gpointer  data)
{
  GList *tmp;

  g_function_enter ("g_list_remove");

  tmp = list;
  while (tmp)
    {
      if (tmp->data == data)
	{
	  if (tmp->prev)
	    tmp->prev->next = tmp->next;
	  if (tmp->next)
	    tmp->next->prev = tmp->prev;
	  
	  if (list == tmp)
	    list = list->next;
	  
	  tmp->next = NULL;
	  tmp->prev = NULL;
	  g_list_free (tmp);
	  
	  break;
	}
      
      tmp = tmp->next;
    }
  
  g_function_leave ("g_list_remove");
  return list;
}

GList* 
g_list_remove_link (GList *list,
		    GList *link)
{
  g_function_enter ("g_list_remove_link");

  if (link)
    {
      if (link->prev)
	link->prev->next = link->next;
      if (link->next)
	link->next->prev = link->prev;

      if (link == list)
	list = list->next;

      link->next = NULL;
      link->prev = NULL;
    }

  g_function_leave ("g_list_remove_link");
  return list;
}

GList* 
g_list_reverse (GList *list)
{
  GList *temg_list;
  GList *last;

  g_function_enter ("g_list_reverse");
  
  if (!list)
    last = NULL;
  
  while (list)
    {
      last = list;
      temg_list = list->next;
      list->next = list->prev;
      list->prev = temg_list;
      list = temg_list;
    }

  g_function_leave ("g_list_reverse");
  return last;
}

GList* 
g_list_nth (GList *list, 
	    gint   n)
{
  g_function_enter ("g_list_nth");

  while ((n-- > 0) && list)
    list = list->next;

  g_function_leave ("g_list_nth");
  return list;
}

GList* 
g_list_last (GList *list)
{
  g_function_enter ("g_list_last");
  
  if (list)
    {
      while (list->next)
	list = list->next;
    }
  
  g_function_leave ("g_list_last");
  return list;
}

gint
g_list_length (GList *list)
{
  gint length;

  g_function_enter ("g_list_length");
  
  length = 0;
  while (list)
    {
      length++;
      list = list->next;
    }

  g_function_leave ("g_list_length");
  return length;
}
