#include <sys/time.h>
#include <unistd.h>
#include "glib.h"


typedef struct _PRealTimer PRealTimer;

struct _PRealTimer
{
  struct timeval start;
  struct timeval end;
  gint active;
};


GTimer* 
g_timer_new ()
{
  PRealTimer *timer;
  
  g_function_enter ("g_timer_new");

  timer = g_new (PRealTimer, 1);
  timer->active = TRUE;
  
  gettimeofday (&timer->start, NULL);

  g_function_leave ("g_timer_new");
  return ((GTimer*) timer);
}

void 
g_timer_destroy (GTimer *timer)
{
  g_function_enter ("g_timer_destroy");

  g_assert (timer != NULL);

  g_free (timer);
  
  g_function_leave ("g_timer_destroy");
}

void 
g_timer_start (GTimer *timer)
{
  PRealTimer *rtimer;
  
  g_function_enter ("g_timer_start");

  g_assert (timer != NULL);

  rtimer = (PRealTimer*) timer;
  gettimeofday (&rtimer->start, NULL);
  rtimer->active = 1;

  g_function_leave ("g_timer_start");
}

void 
g_timer_stop (GTimer *timer)
{
  PRealTimer *rtimer;
  
  g_function_enter ("g_timer_stop");

  g_assert (timer != NULL);

  rtimer = (PRealTimer*) timer;
  gettimeofday (&rtimer->end, NULL);
  rtimer->active = 0;

  g_function_leave ("g_timer_stop");
}

void 
g_timer_reset (GTimer *timer)
{
  PRealTimer *rtimer;
  
  g_function_enter ("g_timer_reset");

  g_assert (timer != NULL);

  rtimer = (PRealTimer*) timer;
  gettimeofday (&rtimer->start, NULL);
  
  g_function_leave ("g_timer_reset");
}

gdouble
g_timer_elapsed (GTimer *timer, 
		 gulong *microseconds)
{
  PRealTimer *rtimer;
  struct timeval elapsed;
  gdouble total;

  g_function_enter ("g_timer_elapsed");

  g_assert (timer != NULL);

  rtimer = (PRealTimer*) timer;

  if (rtimer->active)
    gettimeofday (&rtimer->end, NULL);

  if (rtimer->start.tv_usec > rtimer->end.tv_usec)
    {
      rtimer->end.tv_usec += 1000000;
      rtimer->end.tv_sec--;
    }

  elapsed.tv_usec = rtimer->end.tv_usec - rtimer->start.tv_usec;
  elapsed.tv_sec = rtimer->end.tv_sec - rtimer->start.tv_sec;

  total = elapsed.tv_sec + ((gdouble) elapsed.tv_usec / 1e6);

  if (microseconds)
    *microseconds = elapsed.tv_usec;

  g_function_leave ("g_timer_elapsed");
  return total;
}

