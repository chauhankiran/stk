#include "glib.h"

int
main (int   argc, 
      char *argv[])
{
  GList *list, *t;
  GMemChunk *mem_chunk;
  GTimer *timer;
  gint nums[10] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 };
  gchar *mem[10000];
  gint i, j;

  g_print ("checking lists...");

  list = NULL;
  for (i = 0; i < 10; i++)
    list = g_list_append (list, &nums[i]);
  list = g_list_reverse (list);

  for (i = 0; i < 10; i++)
    {
      t = g_list_nth (list, i);
      if (*((gint*) t->data) != (9 - i))
	g_error ("failed");
    }

  g_list_free (list);
      
  g_print ("ok\n");


  g_print ("checking mem chunks...");

  mem_chunk = g_mem_chunk_new ("test mem chunk", 50, 100, G_ALLOC_AND_FREE);

  for (i = 0; i < 10000; i++)
    {
      mem[i] = g_chunk_new (gchar, mem_chunk);

      for (j = 0; j < 50; j++)
	mem[i][j] = i * j;
    }

  for (i = 0; i < 10000; i++)
    g_mem_chunk_free (mem_chunk, mem[i]);
  
  g_print ("ok\n");


  g_print ("checking timers...");

  timer = g_timer_new ();
  g_print ("\n  spinning for 3 seconds...\n");

  g_timer_start (timer);
  while (g_timer_elapsed (timer, NULL) < 3)
    ;

  g_timer_stop (timer);
  g_timer_destroy (timer);

  g_print ("ok\n");
  

  return 0;
}
