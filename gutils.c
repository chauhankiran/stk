#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "glib.h"

/* #define PRINT_TRACE */
#define CALL_STACK_SIZE 1024

static char *call_stack[CALL_STACK_SIZE];
static int call_stack_index = 0;

gchar* g_strdup (gchar *str) {
  gchar *new_str;

  new_str = NULL;
  if (str) {
    new_str = g_new (char, strlen (str) + 1);
    strcpy (new_str, str);
  }

  return new_str;
}

void g_error (char *format, ...) {
  va_list args;

  g_function_enter("g_error");

  va_start(args, format);
  fputs("\n** ERROR **: ", stderr);
  vfprintf(stderr, format, args);
  fputc('\n', stderr);
  va_end(args);

  g_function_trace();
  /*  abort (); */
  exit(1);

  g_function_leave("g_error");
}

void g_warning(char *format, ...) {
  va_list args;

  g_function_enter("g_warning");

  va_start(args, format);
  fputs("\n** WARNING **: ", stdout);
  vfprintf(stdout, format, args);
  fputc('\n', stdout);
  va_end(args);

  g_function_leave("g_warning");
}

void g_message(char *format, ...) {
  va_list args;

  g_function_enter("g_message");

  va_start(args, format);
  fputs("message: ", stdout);
  vfprintf(stdout, format, args);
  fputc('\n', stdout);
  va_end(args);

  g_function_leave("g_message");
}

void g_print(char *format, ...) {
  va_list args;

  g_function_enter("g_print");

  va_start(args, format);
  vfprintf(stdout, format, args);
  va_end(args);

  g_function_leave("g_print");
}

void g_real_function_enter(char *fname) {
  #ifdef PRINT_TRACE
    g_int i;

    printf("enter: ");
    for (i = 0; i < call_stack_index; i++) {
      fputc(' ', stdout);
    }
    printf("%s\n", fname);
  #endif /* PRINT_TRACE */

  if (call_stack_index >= CALL_STACK_SIZE) {
    g_function_trace ();
  } else {
    call_stack[call_stack_index] = fname;
  }

  call_stack_index++;
}

void g_real_function_leave(char *fname) {
  #ifdef PRINT_TRACE
    g_int i;
  #endif /* PRINT_TRACE */

  if (call_stack_index > 0) {
    call_stack_index--;
  }

  #ifdef PRINT_TRACE
    printf("leave: ");
    for (i = 0; i < call_stack_index; i++) {
      fputc(' ', stdout);
    }
    printf("%s\n", fname);
  #endif /* PRINT_TRACE */
}

void g_real_function_trace() {
  int i;

  for (i = 0; i < call_stack_index; i++) {
    printf ("#%d: %s\n", i, call_stack[call_stack_index - i - 1]);
  }
}
