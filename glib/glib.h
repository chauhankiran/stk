#ifndef __G_LIB_H__
#define __G_LIB_H__


#include "gconfig.h"


#ifdef HAVE_FLOAT_H

#include <float.h>

#define G_MINFLOAT   FLT_MIN
#define G_MAXFLOAT   FLT_MAX
#define G_MINDOUBLE  DBL_MIN
#define G_MAXDOUBLE  DBL_MAX

#elif HAVE_VALUES_H

#include <values.h>

#define G_MINFLOAT  MINFLOAT
#define G_MAXFLOAT  MAXFLOAT
#define G_MINDOUBLE MINDOUBLE
#define G_MAXDOUBLE MAXDOUBLE

#endif /* HAVE_VALUES_H */


#ifdef HAVE_LIMITS_H

#include <limits.h>

#define G_MINSHORT  SHRT_MIN
#define G_MAXSHORT  SHRT_MAX
#define G_MININT    INT_MIN
#define G_MAXINT    INT_MAX
#define G_MINLONG   LONG_MIN
#define G_MAXLONG   LONG_MAX

#elif HAVE_VALUES_H

#ifdef HAVE_FLOAT_H
#include <values.h>
#endif /* HAVE_FLOAT_H */

#define G_MINSHORT  MINSHORT
#define G_MAXSHORT  MAXSHORT
#define G_MININT    MININT
#define G_MAXINT    MAXINT
#define G_MINLONG   MINLONG
#define G_MAXLONG   MAXLONG

#endif /* HAVE_VALUES_H */


#ifndef FALSE
#define FALSE 0
#endif /* FALSE */

#ifndef TRUE
#define TRUE 1
#endif /* TRUE */

#ifndef NULL
#define NULL ((void*) 0)
#endif /* NULL */

#ifndef MAX
#define MAX(a, b)  (((a) > (b)) ? (a) : (b))
#endif /* MAX */

#ifndef MIN
#define MIN(a, b)  (((a) < (b)) ? (a) : (b))
#endif /* MIN */

#ifndef ABS
#define ABS(a)     (((a) < 0) ? -(a) : (a))
#endif /* ABS */

#ifndef ATEXIT
#define ATEXIT(proc)   (atexit (proc))
#endif /* ATEXIT */


#define G_MEM_TEMP 0
#define G_MEM_PERM 1

#ifndef G_DO_STACK_TRACE
#define G_DO_STACK_TRACE TRUE
#endif /* G_DO_STACK_TRACE */

#if (G_DO_STACK_TRACE == TRUE)

#define g_function_enter(fname)  (g_real_function_enter (fname))
#define g_function_leave(fname)  (g_real_function_leave (fname))
#define g_function_trace()       (g_real_function_trace ())

#else /* G_DO_STACK_TRACE */

#define g_function_enter(fname)
#define g_function_leave(fname)
#define g_function_trace()

#endif /* G_DO_STACK_TRACE */

#define g_new(type, count)        \
    ((type *) g_malloc ((unsigned) sizeof (type) * (count)))
#define g_chunk_new(type, chunk)  \
    ((type *) g_mem_chunk_alloc (chunk))

#define g_string(x) #x

#ifdef __PRETTY_FUNCTION__

#define g_assert(expr)  if (!(expr)) ||                                  \
			  g_error ("file %s: line %d (%s): \"%s\"",      \
				    __FILE__,                            \
				    __LINE__,                            \
				    __PRETTY_FUNCTION__,                 \
				    g_string(expr));

#else /* __PRETTY_FUNCTION__ */

#define g_assert(expr)  if (!(expr))                                     \
			  g_error ("file %s: line %d: \"%s\"",           \
				    __FILE__,                            \
				    __LINE__,                            \
				    g_string(expr));

#endif /* __PRETTY_FUNCTION__ */


typedef char   gchar;
typedef short  gshort;
typedef long   glong;
typedef int    gint;

typedef unsigned char   guchar;
typedef unsigned short  gushort;
typedef unsigned long   gulong;
typedef unsigned int    guint;

typedef float   gfloat;
typedef double  gdouble;

#ifdef HAVE_LONG_DOUBLE
typedef long double gldouble;
#else /* HAVE_LONG_DOUBLE */
typedef double gldouble;
#endif /* HAVE_LONG_DOUBLE */

typedef void* gpointer;

typedef char   gint8;
typedef short  gint16;
typedef long   gint32;

typedef unsigned char   guint8;
typedef unsigned short  guint16;
typedef unsigned long   guint32;

typedef struct _GList     GList;
typedef struct _GTimer    GTimer;
typedef struct _GMemChunk GMemChunk;


struct _GList
{
  gpointer data;
  GList *next;
  GList *prev;
};

struct _GTimer { gint dummy; };
struct _GMemChunk { gint dummy; };


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


/* Lists
 */
GList* g_list_alloc       (void);
void   g_list_free        (GList     *list);
GList* g_list_append      (GList     *list,
			   gpointer   data);
GList* g_list_prepend     (GList     *list,
			   gpointer   data);
GList* g_list_remove      (GList     *list,
			   gpointer   data);
GList* g_list_remove_link (GList     *list,
			   GList     *link);
GList* g_list_reverse     (GList     *list);
GList* g_list_nth         (GList     *list,
			   gint       n);
GList* g_list_last        (GList     *list);
gint   g_list_length      (GList     *list);


/* Memory
 */
gpointer g_malloc  (gulong    size);
gpointer g_realloc (gpointer  mem,
		    gulong    size);
void     g_free    (gpointer  mem);


/* "g_mem_chunk_new" creates a new memory chunk.
 * Memory chunks are used to allocate pieces of memory which are
 *  always the same size. Lists are a good example of such a data type.
 * The memory chunk allocates and frees blocks of memory as needed.
 *  Just be sure to call "g_mem_chunk_free" and not "g_free" on data
 *  allocated in a mem chunk. ("g_free" will most likely cause a seg
 *  fault...somewhere).
 *
 * Oh yeah, GMemChunk is an opaque data type. (You don't really
 *  want to know what's going on inside do you?)
 */

/* ALLOC_ONLY MemChunk's can only allocate memory. The free operation
 *  is interpreted as a no op. ALLOC_ONLY MemChunk's save 4 bytes per
 *  atom. (They are also useful for lists which use MemChunk to allocate
 *  memory but are also part of the MemChunk implementation).
 * ALLOC_AND_FREE MemChunk's can allocate and free memory.
 */

#define G_ALLOC_ONLY      1
#define G_ALLOC_AND_FREE  2

GMemChunk* g_mem_chunk_new     (gchar     *name,
				gint       atom_size,
				gulong     area_size,
				gint       type);
void       g_mem_chunk_destroy (GMemChunk *mem_chunk);
gpointer   g_mem_chunk_alloc   (GMemChunk *mem_chunk);
void       g_mem_chunk_free    (GMemChunk *mem_chunk,
				gpointer   mem);
void       g_mem_chunk_clean   (GMemChunk *mem_chunk);
void       g_mem_chunk_reset   (GMemChunk *mem_chunk);
void       g_mem_chunk_print   (GMemChunk *mem_chunk);
void       g_mem_chunk_info    (void);

/* Ah yes...we have a "g_blow_chunks" function.
 * "g_blow_chunks" simply compresses all the chunks. This operation
 *  consists of freeing every memory area that should be freed (but
 *  which we haven't gotten around to doing yet). And, no,
 *  "g_blow_chunks" doesn't follow the naming scheme, but it is a
 *  much better name than "g_mem_chunk_clean_all" or something
 *  similar.
 */
void g_blow_chunks (void);


/* Timer
 */
GTimer* g_timer_new     (void);
void    g_timer_destroy (GTimer  *timer);
void    g_timer_start   (GTimer  *timer);
void    g_timer_stop    (GTimer  *timer);
void    g_timer_reset   (GTimer  *timer);
gdouble g_timer_elapsed (GTimer  *timer,
			 gulong  *microseconds);


/* Output
 */
void g_error   (char *format, ...);
void g_warning (char *format, ...);
void g_message (char *format, ...);
void g_print   (char *format, ...);


/* Stack trace
 */
void g_real_function_enter (char *fname);
void g_real_function_leave (char *fname);
void g_real_function_trace (void);

/* Utility routines
 */
gchar* g_strdup (gchar *str);


#ifdef __cplusplus
}
#endif /* __cplusplus */


#endif /* __G_LIB_H__ */
