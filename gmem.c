#include <stdlib.h>
#include "glib.h"


#define MAX_MEM_AREA  65536L
#define MEM_AREA_SIZE 4L
#define MEM_ALIGN     SIZEOF_LONG


typedef struct _GMemArea      GMemArea;
typedef struct _GRealMemChunk GRealMemChunk;

struct _GMemArea {
  GMemArea *next;            /* the next mem area */
  GMemArea *prev;            /* the previous mem area */
  gulong index;              /* the current index into the "mem" array */
  gulong free;               /* the number of free bytes in this mem area */
  gulong allocated;          /* the number of atoms allocated from this area */
  gulong mark;               /* is this mem area marked for deletion */
  gchar mem[MEM_AREA_SIZE];  /* the mem array from which atoms get allocated 
			      * this actual size of this array is determined by
			      *  the mem chunk "area_size". ANSI says that it
			      *  must be declared to be the maximum size it
			      *  can possibly be (even though the actual size
			      *  may be less).
			      */
};

struct _GRealMemChunk {
  gchar *name;               /* name of this MemChunk...used for debugging output */
  gint type;                 /* the type of MemChunk: ALLOC_ONLY or ALLOC_AND_FREE */
  gint num_mem_areas;        /* the number of memory areas */
  gint num_marked_areas;     /* the number of areas marked for deletion */
  gint atom_size;            /* the size of an atom */
  gulong area_size;          /* the size of a memory area */
  GMemArea *mem_area;        /* the current memory area */
  GMemArea *mem_areas;       /* a list of all the mem areas owned by this chunk */
  GMemArea *free_mem_area;   /* the free area...which is about to be destroyed */
  GList *free_atoms;         /* the free atoms list */
  GRealMemChunk *next;       /* pointer to the next chunk */
  GRealMemChunk *prev;       /* pointer to the previous chunk */
};


static GRealMemChunk *mem_chunks = NULL;


gpointer 
g_malloc (gulong size)
{
  gpointer p;

  g_function_enter ("g_malloc");
  
  p = (gpointer) malloc (size);
  if (!p)
    g_error ("could not allocate %d bytes", size);

  g_function_leave ("g_malloc");
  return p;
}

gpointer
g_realloc (gpointer mem, 
	   gulong   size)
{
  gpointer p;
  
  g_function_enter ("g_realloc");
  
  if (!mem)
    p = (gpointer) malloc (size);
  else
    p = (gpointer) realloc (mem, size);

  if (!p)
    g_error ("could not reallocate %d bytes", size);

  g_function_leave ("g_realloc");
  return p;
}

void 
g_free (gpointer mem)
{
  g_function_enter ("g_free");
  
  if (mem)
    free (mem);

  g_function_leave ("g_free");
}

GMemChunk*
g_mem_chunk_new (gchar  *name,
		 gint    atom_size, 
		 gulong  area_size, 
		 gint    type)
{
  GRealMemChunk *mem_chunk;

  g_function_enter ("g_mem_chunk_new");
  
  mem_chunk = g_new (struct _GRealMemChunk, 1);
  mem_chunk->name = name;
  mem_chunk->type = type;
  mem_chunk->num_mem_areas = 0;
  mem_chunk->num_marked_areas = 0;
  mem_chunk->mem_area = NULL;
  mem_chunk->free_mem_area = NULL;
  mem_chunk->free_atoms = NULL;
  mem_chunk->mem_areas = NULL;

  switch (mem_chunk->type)
    {
    case G_ALLOC_ONLY:
      mem_chunk->atom_size = atom_size;
      break;
    case G_ALLOC_AND_FREE:
      mem_chunk->atom_size = atom_size + SIZEOF_VOID_P;
      break;
    default:
      g_error ("unknown memory chunk type: %d", type);
      break;
    }
  
  if (mem_chunk->atom_size % MEM_ALIGN)
    mem_chunk->atom_size += MEM_ALIGN - (mem_chunk->atom_size % MEM_ALIGN);

  mem_chunk->area_size = area_size;
  if (mem_chunk->area_size > MAX_MEM_AREA)
    mem_chunk->area_size = MAX_MEM_AREA;
  if (mem_chunk->area_size < mem_chunk->atom_size)
    mem_chunk->area_size = mem_chunk->atom_size;
  
  if (mem_chunk->area_size % mem_chunk->atom_size)
    mem_chunk->area_size += mem_chunk->atom_size - (mem_chunk->area_size % mem_chunk->atom_size);

  mem_chunk->next = mem_chunks;
  mem_chunk->prev = NULL;
  if (mem_chunks)
    mem_chunks->prev = mem_chunk;
  mem_chunks = mem_chunk;

  g_function_leave ("g_mem_chunk_new");
  return ((GMemChunk*) mem_chunk);
}

void
g_mem_chunk_destroy (GMemChunk *mem_chunk)
{
  GRealMemChunk *rmem_chunk;
  GMemArea *mem_areas;
  GMemArea *temp_area;

  g_function_enter ("g_mem_chunk_destroy");
  
  g_assert (mem_chunk != NULL);
  
  rmem_chunk = (GRealMemChunk*) mem_chunk;

  mem_areas = rmem_chunk->mem_areas;
  while (mem_areas)
    {
      temp_area = mem_areas;
      mem_areas = mem_areas->next;
      g_free (temp_area);
    }
  
  g_list_free (rmem_chunk->free_atoms);

  if (rmem_chunk->next)
    rmem_chunk->next->prev = rmem_chunk->prev;
  if (rmem_chunk->prev)
    rmem_chunk->prev->next = rmem_chunk->next;

  if (rmem_chunk == mem_chunks)
    mem_chunks = mem_chunks->next;

  g_free (rmem_chunk);

  g_function_leave ("g_mem_chunk_destroy");
}

gpointer
g_mem_chunk_alloc (GMemChunk *mem_chunk)
{
  GRealMemChunk *rmem_chunk;
  GMemArea *temp_area;
  GList *temp_list;
  gpointer mem;
  gpointer *t;

  g_function_enter ("g_mem_chunk_alloc");

  g_assert (mem_chunk != NULL);

  rmem_chunk = (GRealMemChunk*) mem_chunk;
  while (rmem_chunk->free_atoms)
    {
      /* Get the first piece of memory on the "free_atoms" list.
       * We can go ahead and destroy the list node we used to keep
       *  track of it with and to update the "free_atoms" list to
       *  point to its next element.
       */
      mem = rmem_chunk->free_atoms->data;
      temp_list = rmem_chunk->free_atoms;
      rmem_chunk->free_atoms = rmem_chunk->free_atoms->next;
      temp_list->next = NULL;
      g_list_free (temp_list);
      
      /* Determine which area this piece of memory is allocated from */
      temp_area = (GMemArea*) *((gpointer*) ((gchar*) mem - SIZEOF_VOID_P));
      
      /* If the area has been marked, then it is being destroyed.
       *  (ie marked to be destroyed).
       * We check to see if all of the segments on the free list that
       *  reference this area have been removed. This occurs when
       *  the ammount of free memory is less than the allocatable size.
       * If the chunk should be freed, then we place it in the "free_mem_area".
       * This is so we make sure not to free the mem area here and then
       *  allocate it again a few lines down.
       * If we don't allocate a chunk a few lines down then the "free_mem_area"
       *  will be freed.
       * If there is already a "free_mem_area" then we'll just free this mem area.
       */
      if (temp_area->mark)
        {
          /* Update the "free" memory available in that area */
          temp_area->free += rmem_chunk->atom_size;

          if (temp_area->free == rmem_chunk->area_size)
            {
              if (temp_area == rmem_chunk->mem_area)
                rmem_chunk->mem_area = NULL;
	      
              if (rmem_chunk->free_mem_area)
                {
                  rmem_chunk->num_mem_areas -= 1;
                  rmem_chunk->num_marked_areas -= 1;
		  
                  if (temp_area->next)
                    temp_area->next->prev = temp_area->prev;
                  if (temp_area->prev)
                    temp_area->prev->next = temp_area->next;
                  if (temp_area == rmem_chunk->mem_areas)
                    rmem_chunk->mem_areas = rmem_chunk->mem_areas->next;
                  if (temp_area == rmem_chunk->mem_area)
                    rmem_chunk->mem_area = NULL;

                  g_free (temp_area);
                }
              else
                rmem_chunk->free_mem_area = temp_area;
	    }
	}
      else
        {
          /* Update the "free" memory available in that area 
	   */
          temp_area->free -= rmem_chunk->atom_size;
          temp_area->allocated += 1;
	  
          /* The area wasn't marked...return the memory 
	   */
          goto done;
        }
    }
  
  /* If there isn't a current mem area or the current mem area is out of space
   *  then allocate a new mem area. We'll first check and see if we can use
   *  the "free_mem_area". Otherwise we'll just malloc the mem area.
   */
  if ((!rmem_chunk->mem_area) || 
      ((rmem_chunk->mem_area->index + rmem_chunk->atom_size) > rmem_chunk->area_size))
    {
      if (rmem_chunk->free_mem_area)
        {
          rmem_chunk->mem_area = rmem_chunk->free_mem_area;
	  rmem_chunk->free_mem_area = NULL;
        }
      else
        {
	  rmem_chunk->mem_area = (GMemArea*) g_malloc (sizeof (GMemArea) - 
						       MEM_AREA_SIZE +
						       rmem_chunk->area_size);
	  
	  rmem_chunk->num_mem_areas += 1;
	  rmem_chunk->mem_area->next = rmem_chunk->mem_areas;
	  rmem_chunk->mem_area->prev = NULL;
	  
	  if (rmem_chunk->mem_areas)
	    rmem_chunk->mem_areas->prev = rmem_chunk->mem_area;
	  rmem_chunk->mem_areas = rmem_chunk->mem_area;
        }

      rmem_chunk->mem_area->index = 0;
      rmem_chunk->mem_area->free = rmem_chunk->area_size;
      rmem_chunk->mem_area->allocated = 0;
      rmem_chunk->mem_area->mark = 0;
    }
  else if (rmem_chunk->free_mem_area)
    {
      rmem_chunk->num_mem_areas -= 1;
      
      if (rmem_chunk->free_mem_area->next)
	rmem_chunk->free_mem_area->next->prev = rmem_chunk->free_mem_area->prev;
      if (rmem_chunk->free_mem_area->prev)
	rmem_chunk->free_mem_area->prev->next = rmem_chunk->free_mem_area->next;
      if (rmem_chunk->free_mem_area == rmem_chunk->mem_areas)
	rmem_chunk->mem_areas = rmem_chunk->mem_areas->next;

      g_free (rmem_chunk->free_mem_area);
      rmem_chunk->free_mem_area = NULL;
    }

  /* Get the memory and modify the state variables appropriately.
   */
  mem = (gpointer) &rmem_chunk->mem_area->mem[rmem_chunk->mem_area->index];
  rmem_chunk->mem_area->index += rmem_chunk->atom_size;
  rmem_chunk->mem_area->free -= rmem_chunk->atom_size;
  rmem_chunk->mem_area->allocated += 1;

  /* If this is an ALLOC_AND_FREE chunk we calculated the atom_size with 4 extra bytes
   *  so that we can use that space to keep track of which mem area this piece 
   *  of memory came from.
   */
  if (rmem_chunk->type == G_ALLOC_AND_FREE)
    {
      t = (gpointer*) mem;
      *t = (gpointer) rmem_chunk->mem_area;
      mem = (gpointer) ((gchar*) mem + SIZEOF_VOID_P);
    }

done:
  g_function_leave ("g_mem_chunk_alloc");
  return mem;
}

void
g_mem_chunk_free (GMemChunk *mem_chunk, 
		  gpointer   mem)
{
  GRealMemChunk *rmem_chunk;
  GMemArea *temp_area;
  GList *new_list;

  g_function_enter ("g_mem_chunk_free");

  g_assert (mem_chunk != NULL);
  g_assert (mem != NULL);

  rmem_chunk = (GRealMemChunk*) mem_chunk;

  /* Don't do anything if this is an ALLOC_ONLY chunk
   */
  if (rmem_chunk->type == G_ALLOC_AND_FREE)
    {
      /* Place the memory on the "free_atoms" list 
       */
      new_list = g_list_alloc ();
      new_list->data = mem;
      new_list->next = rmem_chunk->free_atoms;
      rmem_chunk->free_atoms = new_list;
      
      temp_area = (GMemArea*) *((gpointer*) ((gchar*) mem - SIZEOF_VOID_P));
      temp_area->allocated -= 1;
      
      if (temp_area->allocated == 0)
	{
	  temp_area->mark = 1;
	  rmem_chunk->num_marked_areas += 1;
	  
	  g_mem_chunk_clean (mem_chunk);
	}
    }
  
  g_function_leave ("g_mem_chunk_free");
}

void
g_mem_chunk_clean (GMemChunk *mem_chunk)
{
  GRealMemChunk *rmem_chunk;
  GMemArea *mem_area;
  GList *temp_list;
  GList *temp_list2;
  GList *prev_list;
  gpointer mem;

  g_function_enter ("g_mem_chunk_clean");

  g_assert (mem_chunk != NULL);

  rmem_chunk = (GRealMemChunk*) mem_chunk;
  
  if (rmem_chunk->type == G_ALLOC_AND_FREE)
    {
      prev_list = NULL;
      temp_list = rmem_chunk->free_atoms;
      
      while (temp_list)
	{
	  mem = temp_list->data;
	  temp_list2 = temp_list;
	  temp_list = temp_list->next;
	  
	  mem_area = (GMemArea*) *((gpointer*) ((gchar*) mem - SIZEOF_VOID_P));
	  
          /* If this mem area is marked for destruction then delete the
	   *  area and list node and decrement the free mem.
           */
	  if (mem_area->mark)
	    {
	      if (temp_list2 == rmem_chunk->free_atoms)
		rmem_chunk->free_atoms = temp_list2->next;
	      if (prev_list)
		prev_list->next = temp_list2->next;
	      temp_list2->next = NULL;
	      g_list_free (temp_list2);

	      mem_area->free += rmem_chunk->atom_size;
	      if (mem_area->free == rmem_chunk->area_size)
		{
		  rmem_chunk->num_mem_areas -= 1;
		  rmem_chunk->num_marked_areas -= 1;
		  
		  if (mem_area->next)
		    mem_area->next->prev = mem_area->prev;
		  if (mem_area->prev)
		    mem_area->prev->next = mem_area->next;
		  if (mem_area == rmem_chunk->mem_areas)
		    rmem_chunk->mem_areas = rmem_chunk->mem_areas->next;
		  if (mem_area == rmem_chunk->mem_area)
		    rmem_chunk->mem_area = NULL;
		  
		  g_free (mem_area);
		}
	    }
	  else
	    {
	      prev_list = temp_list2;
	    }
	}
    }

  g_function_leave ("g_mem_chunk_clean");
}

void 
g_mem_chunk_reset (GMemChunk *mem_chunk)
{
  GRealMemChunk *rmem_chunk;
  GMemArea *mem_areas;
  GMemArea *temp_area;

  g_function_enter ("g_mem_chunk_reset");

  g_assert (mem_chunk != NULL);

  rmem_chunk = (GRealMemChunk*) mem_chunk;

  mem_areas = rmem_chunk->mem_areas;
  rmem_chunk->num_mem_areas = 0;
  rmem_chunk->mem_areas = NULL;
  rmem_chunk->mem_area = NULL;
  
  while (mem_areas)
    {
      temp_area = mem_areas;
      mem_areas = mem_areas->next;
      g_free (temp_area);
    }

  g_list_free (rmem_chunk->free_atoms);
  rmem_chunk->free_atoms = NULL;
  
  g_function_leave ("g_mem_chunk_reset");
}

void 
g_mem_chunk_print (GMemChunk *mem_chunk)
{
  GRealMemChunk *rmem_chunk;
  GMemArea *mem_areas;
  gulong mem;
  
  g_function_enter ("g_mem_chunk_print");

  g_assert (mem_chunk != NULL);
  
  rmem_chunk = (GRealMemChunk*) mem_chunk;
  mem_areas = rmem_chunk->mem_areas;
  mem = 0;

  while (mem_areas)
    {
      mem += rmem_chunk->area_size - mem_areas->free;
      mem_areas = mem_areas->next;
    }

  g_message ("%s: %ld bytes using %d mem areas", rmem_chunk->name, mem, rmem_chunk->num_mem_areas);

  g_function_leave ("g_mem_chunk_print");
}

void 
g_mem_chunk_info ()
{
  GRealMemChunk *mem_chunk;
  gint count;

  g_function_enter ("g_mem_chunk_info");

  count = 0;
  mem_chunk = mem_chunks;
  while (mem_chunk)
    {
      count += 1;
      mem_chunk = mem_chunk->next;
    }

  g_message ("%d mem chunks", count);

  mem_chunk = mem_chunks;
  while (mem_chunk)
    {
      g_mem_chunk_print ((GMemChunk*) mem_chunk);
      mem_chunk = mem_chunk->next;
    }

  g_function_leave ("g_mem_chunk_info");
}

void
g_blow_chunks ()
{
  GRealMemChunk *mem_chunk;

  g_function_enter ("g_blow_chunks");

  mem_chunk = mem_chunks;
  while (mem_chunk)
    {
      g_mem_chunk_clean ((GMemChunk*) mem_chunk);
      mem_chunk = mem_chunk->next;
    }

  g_function_leave ("g_blow_chunks");
}

