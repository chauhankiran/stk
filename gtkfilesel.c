/* GTK - The General Toolkit (written for the GIMP)
 * Copyright (C) 1995 Peter Mattis, Josh MacDonald
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/param.h>
#include <dirent.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <pwd.h>
#include "fnmatch.h"

#include "gtkbox.h"
#include "gtkbutton.h"
#include "gtkcontainer.h"
#include "gtkdata.h"
#include "gtkdraw.h"
#include "gtkentry.h"
#include "gtkframe.h"
#include "gtklistbox.h"
#include "gtkmain.h"
#include "gtkmisc.h"
#include "gtkfilesel.h"
#include "gtkstyle.h"
#include "gtkwidget.h"
#include "gtkwindow.h"

#if defined(sun) && !defined(__svr4__)
#define STRERROR(e) sys_errlist[e]
#else
#define STRERROR(e) strerror(e)
#endif

typedef struct _CompletionState    CompletionState;
typedef struct _CompletionDir      CompletionDir;
typedef struct _CompletionDirSent  CompletionDirSent;
typedef struct _CompletionDirEntry CompletionDirEntry;
typedef struct _CompletionUserDir  CompletionUserDir;
typedef struct _PossibleCompletion PossibleCompletion;

/* File completion functions which would be external, were they used
 * outside of this file.
 */

static CompletionState*    cmpl_init_state        (void);
static void                cmpl_free_state        (CompletionState *cmpl_state);
static gint                cmpl_state_okay        (CompletionState* cmpl_state);
static gchar*              cmpl_strerror          (gint);

static PossibleCompletion* cmpl_completion_matches(gchar           *text_to_complete,
						   gchar          **remaining_text,
						   CompletionState *cmpl_state);

/* Returns a name for consideration, possibly a completion, this name
 * will be invalid after the next call to cmpl_next_completion.
 */
static char*               cmpl_this_completion   (PossibleCompletion*);

/* True if this completion matches the given text.  Otherwise, this
 * output can be used to have a list of non-completions.
 */
static gint                cmpl_is_a_completion   (PossibleCompletion*);

/* True if the completion is a directory
 */
static gint                cmpl_is_directory      (PossibleCompletion*);

/* Obtains the next completion, or NULL
 */
static PossibleCompletion* cmpl_next_completion   (CompletionState*);

/* Updating completions: the return value of cmpl_updated_text() will
 * be text_to_complete completed as much as possible after the most
 * recent call to cmpl_completion_matches.  For the present
 * application, this is the suggested replacement for the user's input
 * string.  You must CALL THIS AFTER ALL cmpl_text_completions have
 * been received.
 */
static gchar*              cmpl_updated_text       (CompletionState* cmpl_state);

/* After updating, to see if the completion was a directory, call
 * this.  If it was, you should consider re-calling completion_matches.
 */
static gint                cmpl_updated_dir        (CompletionState* cmpl_state);

/* Current location: if using file completion, return the current
 * directory, from which file completion begins.  More specifically,
 * the cwd concatenated with all exact completions up to the last
 * directory delimiter('/').
 */
static gchar*              cmpl_reference_position (CompletionState* cmpl_state);

/* backing up: if cmpl_completion_matches returns NULL, you may query
 * the index of the last completable character into cmpl_updated_text.
 */
static gint                cmpl_last_valid_char    (CompletionState* cmpl_state);

#if 0
/* When the user selects a directory, call
 * cmpl_update_reference_position and then cmpl_completion_matches
 * with the empty string.  Return value is non-zero if an error (such
 * as selecting an unreadable directory) occurs.
 */
static gint                cmpl_update_reference_position (gchar*,
							   CompletionState* cmpl_state);
#endif

/* When the user selects a non-directory, call cmpl_completion_fullname
 * to get the full name of the selected file.
 */
static gchar*              cmpl_completion_fullname (gchar*, CompletionState* cmpl_state);

/* Gtk widget specific functions and structures
 */

#define DIR_LIST_WIDTH   120
#define DIR_LIST_HEIGHT  175
#define FILE_LIST_WIDTH  120
#define FILE_LIST_HEIGHT 175

typedef struct _GtkFileSelection       GtkFileSelection;

struct _GtkFileSelection
{
  GtkWidget *child;
  GtkWidget *window;
  GtkWidget *dir_list;
  GtkWidget *file_list;
  GtkWidget *selection_entry;
  GtkWidget *selection_text;
  GtkWidget *main_vbox;

  GtkCallback ok_callback;
  GtkCallback cancel_callback;
  GtkCallback help_callback;
  gpointer ok_data;
  gpointer cancel_data;
  gpointer help_data;

  CompletionState *cmpl_state;
};


static void gtk_file_selection_populate        (GtkFileSelection *fs,
						gchar            *rel_path,
						gint              try_complete);
static void gtk_file_selection_ok_callback     (GtkWidget         *widget,
						gpointer           client_data,
						gpointer           call_data);
static void gtk_file_selection_cancel_callback (GtkWidget         *widget,
						gpointer           client_data,
						gpointer           call_data);
static void gtk_file_selection_help_callback   (GtkWidget         *widget,
						gpointer           client_data,
						gpointer           call_data);
static gint gtk_file_selection_dir_event       (GtkWidget         *widget,
						GdkEvent          *event);
static gint gtk_file_selection_file_event      (GtkWidget         *widget,
						GdkEvent          *event);
static void gtk_file_selection_free_filename   (GtkWidget         *widget,
						gpointer           client_data,
						gpointer           call_data);
static gint gtk_file_selection_key_function    (guint              keyval,
						guint              state,
						gpointer           client_data);


/* Non-external file completion decls and structures */

/* Size of the directory cache, should be prime.
 */
#define DIR_CACHE_SIZE 4
/* A constant used to determine whether a substring was an exact
 * match by first_diff_index()
 */
#define PATTERN_MATCH -1
/* The arguments used by all fnmatch() calls below
 */
#define FNMATCH_FLAGS (FNM_PATHNAME | FNM_PERIOD)

#define CMPL_ERRNO_TOO_LONG ((1<<16)-1)

/* This structure contains all the useful information about a directory
 * for the purposes of filename completion.  These structures are cached
 * in the CompletionState struct.  CompletionDir's are reference counted.
 */
struct _CompletionDirSent
{
  ino_t inode;
  time_t mtime;
  gint ref_count;
  gint entry_count;
  gchar *name_buffer; /* memory segment containing names of all entries */
  gchar fullname[MAXPATHLEN];
  gint fullname_len;
  struct _CompletionDirEntry *entries;
};

struct _CompletionDir
{
  CompletionDirSent *sent;
  struct _CompletionDir *cmpl_parent;
  gint cmpl_index;
  gchar *cmpl_text;
};

/* This structure contains pairs of directory entry names with a flag saying
 * whether or not they are a valid directory.  NOTE: This information is used
 * to provide the caller with information about whether to update its completions
 * or try to open a file.  Since directories are cached by the directory mtime,
 * a symlink which points to an invalid file (which will not be a directory),
 * will not be reevaluated if that file is created, unless the containing
 * directory is touched.  I consider this case to be worth ignoring (josh).
 */
struct _CompletionDirEntry
{
  gint is_dir;
  gchar *entry_name;
};

struct _CompletionUserDir
{
  gchar *login;
  gchar *homedir;
};

struct _PossibleCompletion
{
  /* accessible fields, all are accessed externally by functions
   * declared above
   */
  gchar *text;
  gint is_a_completion;
  gint is_directory;

  /* Private fields
   */
  gint text_alloc;
};

struct _CompletionState
{
  gint last_valid_char;
  gchar *updated_text;
  gint updated_text_len;
  gint updated_text_alloc;
  gint re_complete;

  gchar *user_dir_name_buffer;
  gint user_directories_len;
  gchar *user_home_dir;

  gchar *last_completion_text;

  gint user_completion_index; /* if >= 0, currently completing ~user */

  struct _CompletionDir *completion_dir; /* directory completing from */
  struct _CompletionDir *active_completion_dir;

  struct _PossibleCompletion the_completion;
  struct _CompletionDir *reference_dir; /* initial directory */
  struct _CompletionDir *directory_cache[DIR_CACHE_SIZE];
  struct _CompletionUserDir *user_directories;
};

/* Saves errno when something cmpl does fails. */
static gint cmpl_errno;

/* Directory operations. */
static CompletionDir* open_ref_dir         (gchar* text_to_complete,
					    gchar** remaining_text,
					    CompletionState* cmpl_state);
static CompletionDir* open_dir             (gchar* dir_name,
					    CompletionState* cmpl_state);
static CompletionDir* open_user_dir        (gchar* text_to_complete,
					    CompletionState *cmpl_state);
static CompletionDir* open_relative_dir    (gchar* dir_name, CompletionDir* dir,
					    CompletionState *cmpl_state);
static CompletionDir* open_new_dir         (gchar* dir_name, struct stat* sbuf);
static gint           correct_dir_fullname (CompletionDir* cmpl_dir);
static gint           correct_parent       (CompletionDir* cmpl_dir,
					    struct stat *sbuf);
static gint           find_dir_fullname    (gchar* dirname);

/* Directories are reference counted */
static void           set_dir_ptr   (CompletionDir **ptr,
				     CompletionDir *dir);
static void           unset_dir_ptr (CompletionDir **ptr);
static void           free_dir      (CompletionDir  *dir);
static CompletionDir* clone_dir     (CompletionDir  *dir);

/* Completion operations */
static PossibleCompletion* attempt_homedir_completion(gchar* text_to_complete,
						      CompletionState *cmpl_state);
static PossibleCompletion* attempt_file_completion(CompletionState *cmpl_state);
static CompletionDir* find_completion_dir(gchar* text_to_complete,
					  gchar** remaining_text,
					  CompletionState* cmpl_state);
static PossibleCompletion* append_completion_text(gchar* text,
						  CompletionState* cmpl_state);
static gint get_pwdb(CompletionState* cmpl_state);
static gint first_diff_index(gchar* pat, gchar* text);
static gint compare_user_dir(const void* a, const void* b);
static gint compare_cmpl_dir(const void* a, const void* b);
static void update_cmpl(PossibleCompletion* poss,
			CompletionState* cmpl_state);

/**********************************************************************/
/*			  External Interface                          */
/**********************************************************************/

/* The four completion state selectors
 */
static gchar*
cmpl_updated_text (CompletionState* cmpl_state)
{
  return cmpl_state->updated_text;
}

static gint
cmpl_updated_dir (CompletionState* cmpl_state)
{
  return cmpl_state->re_complete;
}

static gchar*
cmpl_reference_position (CompletionState* cmpl_state)
{
  return cmpl_state->reference_dir->sent->fullname;
}

static gint
cmpl_last_valid_char (CompletionState* cmpl_state)
{
  return cmpl_state->last_valid_char;
}

static gchar*
cmpl_completion_fullname (gchar* text, CompletionState* cmpl_state)
{
  strcpy(cmpl_state->updated_text, cmpl_state->reference_dir->sent->fullname);
  strcat(cmpl_state->updated_text, "/");
  strcat(cmpl_state->updated_text, text);

  return cmpl_state->updated_text;
}

/* A completion state setter
 */
#if 0
static gint
cmpl_update_reference_position (gchar* new_dir, CompletionState* cmpl_state)
{
  return 0;
}
#endif

/* The three completion selectors
 */
static gchar*
cmpl_this_completion (PossibleCompletion* pc)
{
  return pc->text;
}

static gint
cmpl_is_directory (PossibleCompletion* pc)
{
  return pc->is_directory;
}

static gint
cmpl_is_a_completion (PossibleCompletion* pc)
{
  return pc->is_a_completion;
}

/**********************************************************************/
/*	       Construction, deletion, reference counting             */
/**********************************************************************/

static CompletionState*
cmpl_init_state (void)
{
  gchar getcwd_buf[2*MAXPATHLEN];
  CompletionState *new_state;

  new_state = g_new (CompletionState, 1);

  if (!getcwd (getcwd_buf, MAXPATHLEN))
    {
      cmpl_errno = errno;
      return NULL;
    }

  new_state->reference_dir = NULL;
  new_state->completion_dir = NULL;
  new_state->active_completion_dir = NULL;

  if (!get_pwdb (new_state))
    return NULL;

  memset (new_state->directory_cache, 0, DIR_CACHE_SIZE*sizeof(CompletionDir*));
  new_state->last_valid_char = 0;
  new_state->updated_text = g_new (gchar, MAXPATHLEN);
  new_state->updated_text_alloc = MAXPATHLEN;
  new_state->the_completion.text = g_new (gchar, MAXPATHLEN);
  new_state->the_completion.text_alloc = MAXPATHLEN;

  set_dir_ptr(&new_state->reference_dir, open_dir (getcwd_buf, new_state));

  if (!new_state->reference_dir)
    return NULL;

  return new_state;
}

static void
cmpl_free_state (CompletionState* cmpl_state)
{
  gint i = 0;

  unset_dir_ptr(&cmpl_state->completion_dir);
  unset_dir_ptr(&cmpl_state->active_completion_dir);
  unset_dir_ptr(&cmpl_state->reference_dir);

  for (i = 0; i < DIR_CACHE_SIZE; i += 1)
    if (cmpl_state->directory_cache[i])
      free_dir (cmpl_state->directory_cache[i]);

  g_free (cmpl_state->user_dir_name_buffer);
  g_free (cmpl_state->user_directories);
  g_free (cmpl_state->the_completion.text);
  g_free (cmpl_state->updated_text);
  g_free (cmpl_state);
}

static void
set_dir_ptr (CompletionDir** ptr, CompletionDir* dir)
{
  if(*ptr && dir != *ptr)
    unset_dir_ptr(ptr);

  if(dir && dir != *ptr)
    (*ptr) = dir;
  else if(!dir)
    *ptr = NULL;
}

static void
unset_dir_ptr (CompletionDir** ptr)
{
  if(*ptr && 0)
    {
      (*ptr)->sent->ref_count -= 1;

      if((*ptr)->sent->ref_count == 0)
	free_dir(*ptr);
      else
	g_free(*ptr);

      (*ptr) = NULL;
    }
}

static void
free_dir(CompletionDir* dir)
{
  if(dir->cmpl_parent && dir->cmpl_parent != dir)
    unset_dir_ptr(&dir->cmpl_parent);

  g_free(dir->sent->name_buffer);
  g_free(dir->sent->entries);
  g_free(dir->sent);
  g_free(dir);
}

static CompletionDir*
clone_dir(CompletionDir* dir)
{
  CompletionDir* clone;

  if(dir)
    {
      clone = g_new(CompletionDir, 1);

      memcpy(clone, dir, sizeof(CompletionDir));

      clone->cmpl_index = -1;
      clone->cmpl_parent = NULL;
      clone->sent->ref_count += 1;

      return clone;
    }

  return NULL;
}

/* the rest */

static PossibleCompletion*
cmpl_completion_matches (gchar* text_to_complete,
			 gchar** remaining_text,
			 CompletionState* cmpl_state)
{
  gchar* first_slash;
  PossibleCompletion *poss;

  g_assert(text_to_complete);

  cmpl_state->user_completion_index = -1;
  cmpl_state->last_completion_text = text_to_complete;
  cmpl_state->the_completion.text[0] = 0;
  cmpl_state->last_valid_char = 0;
  cmpl_state->updated_text_len = -1;
  cmpl_state->updated_text[0] = 0;
  cmpl_state->re_complete = FALSE;

  first_slash = strchr(text_to_complete, '/');

  if(text_to_complete[0] == '~' && !first_slash)
    {
      /* Text starts with ~ and there is no slash, show all the
       * home directory completions.
       */
      poss = attempt_homedir_completion(text_to_complete, cmpl_state);

      update_cmpl(poss, cmpl_state);

      return poss;
    }

  set_dir_ptr(&cmpl_state->reference_dir,
	      open_ref_dir(text_to_complete, remaining_text, cmpl_state));

  if(!cmpl_state->reference_dir)
    return NULL;

  set_dir_ptr(&cmpl_state->completion_dir,
	      find_completion_dir(*remaining_text, remaining_text, cmpl_state));

  cmpl_state->last_valid_char = *remaining_text - text_to_complete;

  if(!cmpl_state->completion_dir)
    return NULL;

  cmpl_state->completion_dir->cmpl_index = -1;
  cmpl_state->completion_dir->cmpl_parent = NULL;
  cmpl_state->completion_dir->cmpl_text = *remaining_text;

  set_dir_ptr(&cmpl_state->active_completion_dir,
	      clone_dir(cmpl_state->completion_dir));

  set_dir_ptr(&cmpl_state->reference_dir,
	      clone_dir(cmpl_state->completion_dir));

  poss = attempt_file_completion(cmpl_state);

  update_cmpl(poss, cmpl_state);

  return poss;
}

static PossibleCompletion*
cmpl_next_completion (CompletionState* cmpl_state)
{
  PossibleCompletion* poss = NULL;

  cmpl_state->the_completion.text[0] = 0;

  if(cmpl_state->user_completion_index >= 0)
    poss = attempt_homedir_completion(cmpl_state->last_completion_text, cmpl_state);
  else
    poss = attempt_file_completion(cmpl_state);

  update_cmpl(poss, cmpl_state);

  return poss;
}

/**********************************************************************/
/*			 Directory Operations                         */
/**********************************************************************/

/* Open the directory where completion will begin from, if possible. */
static CompletionDir*
open_ref_dir(gchar* text_to_complete,
	     gchar** remaining_text,
	     CompletionState* cmpl_state)
{
  gchar* first_slash;
  CompletionDir *new_dir;

  first_slash = strchr(text_to_complete, '/');

  if (text_to_complete[0] == '/' || !cmpl_state->reference_dir)
    {
      new_dir = open_dir("/", cmpl_state);

      if(new_dir)
	*remaining_text = text_to_complete + 1;
    }
  else if (text_to_complete[0] == '~')
    {
      new_dir = open_user_dir(text_to_complete, cmpl_state);

      if(new_dir)
	{
	  if(first_slash)
	    *remaining_text = first_slash + 1;
	  else
	    *remaining_text = text_to_complete + strlen(text_to_complete);
	}
      else
	{
	  return NULL;
	}
    }
  else
    {
      *remaining_text = text_to_complete;

      new_dir = open_dir(cmpl_state->reference_dir->sent->fullname, cmpl_state);
    }

  if(new_dir)
    {
      new_dir->cmpl_index = -1;
      new_dir->cmpl_parent = NULL;
    }

  return new_dir;
}

/* open a directory by user name */
static CompletionDir*
open_user_dir(gchar* text_to_complete,
	      CompletionState *cmpl_state)
{
  gchar *first_slash;
  gint cmp_len;

  g_assert(text_to_complete && text_to_complete[0] == '~');

  first_slash = strchr(text_to_complete, '/');

  if(first_slash)
    cmp_len = first_slash - text_to_complete - 1;
  else
    cmp_len = strlen(text_to_complete + 1);

  if(!cmp_len)
    /* ~/ */
    return open_dir(cmpl_state->user_home_dir, cmpl_state);
  else
    /* ~user/ */
    {
      gint i;
      for(i = 0; i < cmpl_state->user_directories_len; i += 1)
	{
	  if(strncmp(cmpl_state->user_directories[i].login,
		     text_to_complete + 1, cmp_len) == 0)
	    return open_dir(cmpl_state->user_directories[i].homedir, cmpl_state);
	}
    }

  return NULL;
}

/* open a directory relative the the current relative directory */
static CompletionDir*
open_relative_dir(gchar* dir_name,
		  CompletionDir* dir,
		  CompletionState *cmpl_state)
{
  CompletionDir* new_dir;

  if(dir->sent->fullname_len + strlen(dir_name) + 2 >= MAXPATHLEN)
    {
      cmpl_errno = CMPL_ERRNO_TOO_LONG;
      return NULL;
    }

  if(dir->sent->fullname_len > 1)
    {

      dir->sent->fullname[dir->sent->fullname_len] = '/';
      strcpy(dir->sent->fullname + dir->sent->fullname_len + 1, dir_name);
    }
  else
    {
      strcpy(dir->sent->fullname + dir->sent->fullname_len, dir_name);
    }

  new_dir = open_dir(dir->sent->fullname, cmpl_state);

  dir->sent->fullname[dir->sent->fullname_len] = 0;

  return new_dir;
}

/* after the cache lookup fails, really open a new directory */
static CompletionDir*
open_new_dir(gchar* dir_name, struct stat* sbuf)
{
  CompletionDir* new_dir;
  DIR* directory;
  gchar *buffer_ptr;
  struct dirent *dirent_ptr;
  gint buffer_size = 0;
  gint entry_count = 0;
  gint i;
  struct stat ent_sbuf;

  new_dir = g_new(CompletionDir, 1);
  new_dir->sent         = g_new(CompletionDirSent, 1);
  new_dir->cmpl_parent  = NULL;
  new_dir->cmpl_text    = NULL;
  new_dir->cmpl_index   = -1;
  new_dir->sent->ref_count = 1;
  new_dir->sent->mtime      = sbuf->st_mtime;
  new_dir->sent->inode      = sbuf->st_ino;
  new_dir->sent->fullname_len = strlen(dir_name);

  if(new_dir->sent->fullname_len + 1 > MAXPATHLEN)
    {
      cmpl_errno = CMPL_ERRNO_TOO_LONG;
      return NULL;
    }

  strcpy(new_dir->sent->fullname, dir_name);

  directory = opendir(dir_name);

  if(!directory)
    {
      cmpl_errno = errno;
      return NULL;
    }

  while((dirent_ptr = readdir(directory)) != NULL)
    {
      int entry_len = strlen(dirent_ptr->d_name);
      buffer_size += entry_len + 1;
      entry_count += 1;

      if(new_dir->sent->fullname_len + entry_len + 2 >= MAXPATHLEN)
	{
	  cmpl_errno = CMPL_ERRNO_TOO_LONG;
 	  closedir(directory);
	  return NULL;
	}
    }

  new_dir->sent->name_buffer = g_new(gchar, buffer_size);
  new_dir->sent->entries = g_new(CompletionDirEntry, entry_count);
  new_dir->sent->entry_count = entry_count;

  buffer_ptr = new_dir->sent->name_buffer;

  rewinddir(directory);

  for(i = 0; i < entry_count; i += 1)
    {
      dirent_ptr = readdir(directory);

      if(!dirent_ptr)
	{
	  cmpl_errno = errno;
	  closedir(directory);
	  return NULL;
	}

      strcpy(buffer_ptr, dirent_ptr->d_name);
      new_dir->sent->entries[i].entry_name = buffer_ptr;
      buffer_ptr += strlen(dirent_ptr->d_name);
      *buffer_ptr = 0;
      buffer_ptr += 1;

      new_dir->sent->fullname[new_dir->sent->fullname_len] = '/';
      strcpy(new_dir->sent->fullname + new_dir->sent->fullname_len + 1, dirent_ptr->d_name);

      if(stat(new_dir->sent->fullname, &ent_sbuf) >= 0 && S_ISDIR(ent_sbuf.st_mode))
	new_dir->sent->entries[i].is_dir = 1;
      else
	/* stat may fail, and we don't mind, since it could be a
	 * dangling symlink. */
	new_dir->sent->entries[i].is_dir = 0;
    }

  new_dir->sent->fullname[new_dir->sent->fullname_len] = 0;

  qsort(new_dir->sent->entries, new_dir->sent->entry_count,
	sizeof(CompletionDirEntry), compare_cmpl_dir);

  closedir(directory);

  return new_dir;
}

/* open a directory by absolute pathname */
static CompletionDir*
open_dir(gchar* dir_name, CompletionState* cmpl_state)
{
  struct stat sbuf;
  gint cache_index;
  CompletionDir *dir;

  if(stat(dir_name, &sbuf) < 0)
    {
      cmpl_errno = errno;
      return NULL;
    }

  cache_index = (sbuf.st_dev + sbuf.st_ino) % DIR_CACHE_SIZE;

  /* First look in the cache
   */
  if(cmpl_state->directory_cache[cache_index])
    {
      dir = cmpl_state->directory_cache[cache_index];

      /* now check that the inodes are the same and whether the
       * directory has been modified since the last read
       */
      if(dir->sent->inode == sbuf.st_ino &&
	 dir->sent->mtime == sbuf.st_mtime)
	{
	  return clone_dir(dir);
	}
    }

  dir = open_new_dir(dir_name, &sbuf);

  /* Either a cache collision has occured, or the directory has
   * been modified, so free the old one
   */
  set_dir_ptr(cmpl_state->directory_cache + cache_index, dir);

  return clone_dir(dir);
}

static gint
correct_dir_fullname(CompletionDir* cmpl_dir)
{
  gint length = strlen(cmpl_dir->sent->fullname);
  struct stat sbuf;

  if (strcmp(cmpl_dir->sent->fullname + length - 2, "/.") == 0)
    cmpl_dir->sent->fullname[length - 2] = 0;
  else if (strcmp(cmpl_dir->sent->fullname + length - 3, "/./") == 0)
    cmpl_dir->sent->fullname[length - 3] = 0;
  else if (strcmp(cmpl_dir->sent->fullname + length - 3, "/..") == 0)
    {
      if(length == 3)
	{
	  strcpy(cmpl_dir->sent->fullname, "/");
	  cmpl_dir->sent->fullname_len = 1;
	  return TRUE;
	}

      if(stat(cmpl_dir->sent->fullname, &sbuf) < 0)
	{
	  cmpl_errno = errno;
	  return FALSE;
	}

      cmpl_dir->sent->fullname[length - 3] = 0;

      if(!correct_parent(cmpl_dir, &sbuf))
	return FALSE;
    }
  else if (strcmp(cmpl_dir->sent->fullname + length - 4, "/../") == 0)
    {
      if(length == 4)
	{
	  strcpy(cmpl_dir->sent->fullname, "/");
	  cmpl_dir->sent->fullname_len = 1;
	  return TRUE;
	}

      if(stat(cmpl_dir->sent->fullname, &sbuf) < 0)
	{
	  cmpl_errno = errno;
	  return FALSE;
	}

      cmpl_dir->sent->fullname[length - 4] = 0;

      if(!correct_parent(cmpl_dir, &sbuf))
	return FALSE;
    }

  cmpl_dir->sent->fullname_len = strlen(cmpl_dir->sent->fullname);

  return TRUE;
}

static gint
correct_parent(CompletionDir* cmpl_dir, struct stat *sbuf)
{
  struct stat parbuf;
  gchar *last_slash;
  gchar c = 0;

  last_slash = strrchr(cmpl_dir->sent->fullname, '/');

  g_assert(last_slash);

  if(last_slash != cmpl_dir->sent->fullname)
    last_slash[0] = 0;
  else
    {
      c = last_slash[1];
      last_slash[1] = 0;
    }

  if (stat(cmpl_dir->sent->fullname, &parbuf) < 0)
    {
      /* this should actually raise an error */
      if(c)
	last_slash[1] = c;
      else
	last_slash[0] = '/';

      strcat(cmpl_dir->sent->fullname, "/../");

      return TRUE;
    }

  if (parbuf.st_ino == sbuf->st_ino && parbuf.st_dev == sbuf->st_dev)
    /* it wasn't a link */
    return TRUE;

  if(c)
    last_slash[1] = c;
  else
    last_slash[0] = '/';

  /* it was a link, have to figure it out the hard way */
  strcat(cmpl_dir->sent->fullname, "/../");

  return find_dir_fullname(cmpl_dir->sent->fullname);
}

static gint
find_dir_fullname(gchar* dirname)
{
  gchar buffer[MAXPATHLEN];
  if(!getcwd(buffer, MAXPATHLEN))
    {
      cmpl_errno = errno;
      return FALSE;
    }

  if(chdir(dirname) != 0)
    {
      cmpl_errno = errno;
      return FALSE;
    }

  if(!getcwd(dirname, MAXPATHLEN))
    {
      if(chdir(buffer) != 0)
	{
	  cmpl_errno = errno;
	  return FALSE;
	}
      return TRUE;
    }

  if(chdir(buffer) != 0)
    {
      cmpl_errno = errno;

      return FALSE;
    }

  return TRUE;
}

/**********************************************************************/
/*                        Completion Operations                       */
/**********************************************************************/

static PossibleCompletion*
attempt_homedir_completion(gchar* text_to_complete,
			   CompletionState *cmpl_state)
{
  gint index, length;

  length = strlen(text_to_complete) - 1;

  cmpl_state->user_completion_index += 1;

  while(cmpl_state->user_completion_index < cmpl_state->user_directories_len)
    {
      index = first_diff_index(text_to_complete + 1,
			       cmpl_state->user_directories
			       [cmpl_state->user_completion_index].login);

      switch(index)
	{
	case PATTERN_MATCH:
	  break;
	default:
	  if(cmpl_state->last_valid_char < (index + 1))
	    cmpl_state->last_valid_char = index + 1;
	  cmpl_state->user_completion_index += 1;
	  continue;
	}

      cmpl_state->the_completion.is_a_completion = 1;
      cmpl_state->the_completion.is_directory = 1;

      append_completion_text("~", cmpl_state);

      append_completion_text(cmpl_state->
			      user_directories[cmpl_state->user_completion_index].login,
			     cmpl_state);

      return append_completion_text("/", cmpl_state);
    }

  if(text_to_complete[1] ||
     cmpl_state->user_completion_index > cmpl_state->user_directories_len)
    {
      cmpl_state->user_completion_index = -1;
      return NULL;
    }
  else
    {
      cmpl_state->user_completion_index += 1;
      cmpl_state->the_completion.is_a_completion = 1;
      cmpl_state->the_completion.is_directory = 1;

      return append_completion_text("~/", cmpl_state);
    }
}

/* returns the index (>= 0) of the first differing character,
 * PATTERN_MATCH if the completion matches */
static gint
first_diff_index(gchar* pat, gchar* text)
{
  gint diff = 0;

  while(*pat && *text && *text == *pat)
    {
      pat += 1;
      text += 1;
      diff += 1;
    }

  if(*pat)
    return diff;

  return PATTERN_MATCH;
}

static PossibleCompletion*
append_completion_text(gchar* text, CompletionState* cmpl_state)
{
  guint len, i = 1;

  if(!cmpl_state->the_completion.text)
    return NULL;

  len = strlen(text) + strlen(cmpl_state->the_completion.text) + 1;

  if(cmpl_state->the_completion.text_alloc > len)
    {
      strcat(cmpl_state->the_completion.text, text);
      return &cmpl_state->the_completion;
    }

  while(i < len) { i <<= 1; }

  cmpl_state->the_completion.text_alloc = i;

  cmpl_state->the_completion.text = (gchar*)g_realloc(cmpl_state->the_completion.text, i);

  if(!cmpl_state->the_completion.text)
    return NULL;
  else
    {
      strcat(cmpl_state->the_completion.text, text);
      return &cmpl_state->the_completion;
    }
}

static CompletionDir*
find_completion_dir(gchar* text_to_complete,
		    gchar** remaining_text,
		    CompletionState* cmpl_state)
{
  gchar* first_slash = strchr(text_to_complete, '/');
  CompletionDir* dir = clone_dir(cmpl_state->reference_dir);
  *remaining_text = text_to_complete;

  while(first_slash)
    {
      gint len = first_slash - *remaining_text;
      gint found = 0;
      gint found_index;
      gint i;
      gchar* pat_buf = g_new (gchar, len + 1);

      strncpy(pat_buf, *remaining_text, len);
      pat_buf[len] = 0;

      for(i = 0; i < dir->sent->entry_count; i += 1)
	{
	  if(dir->sent->entries[i].is_dir &&
	     fnmatch(pat_buf, dir->sent->entries[i].entry_name,
		     FNMATCH_FLAGS)!= FNM_NOMATCH)
	    {
	      if(found)
		{
		  g_free (pat_buf);
		  return dir;
		}
	      else
		{
		  found = 1;
		  found_index = i;
		}
	    }
	}

      if(found)
	{
	  CompletionDir* next = open_relative_dir(dir->sent->entries[found_index].entry_name,
						  dir, cmpl_state);

	  if(!next)
	    {
	      g_free (pat_buf);
	      return NULL;
	    }

	  next->cmpl_parent = dir;

	  set_dir_ptr(&dir, next);

	  if(!correct_dir_fullname(dir))
	    {
	      g_free(pat_buf);
	      return NULL;
	    }

	  *remaining_text = first_slash + 1;
	  first_slash = strchr(*remaining_text, '/');
	}
      else
	{
	  g_free (pat_buf);
	  return NULL;
	}

      g_free (pat_buf);
    }

  return dir;
}

static void
update_cmpl(PossibleCompletion* poss, CompletionState* cmpl_state)
{
  gint cmpl_len;

  if(!poss || !cmpl_is_a_completion(poss))
    return;

  cmpl_len = strlen(cmpl_this_completion(poss));

  if(cmpl_state->updated_text_alloc < cmpl_len + 1)
    {
      cmpl_state->updated_text =
	(gchar*)g_realloc(cmpl_state->updated_text,
			  cmpl_state->updated_text_alloc);
      cmpl_state->updated_text_alloc = 2*cmpl_len;
    }

  if(cmpl_state->updated_text_len < 0)
    {
      strcpy(cmpl_state->updated_text, cmpl_this_completion(poss));
      cmpl_state->updated_text_len = cmpl_len;
      cmpl_state->re_complete = cmpl_is_directory(poss);
    }
  else if(cmpl_state->updated_text_len == 0)
    {
      cmpl_state->re_complete = FALSE;
    }
  else
    {
      gint first_diff =
	first_diff_index(cmpl_state->updated_text,
			 cmpl_this_completion(poss));

      cmpl_state->re_complete = FALSE;

      if(first_diff == PATTERN_MATCH)
	return;

      if(first_diff > cmpl_state->updated_text_len)
	strcpy(cmpl_state->updated_text, cmpl_this_completion(poss));

      cmpl_state->updated_text_len = first_diff;
      cmpl_state->updated_text[first_diff] = 0;
    }
}

static PossibleCompletion*
attempt_file_completion(CompletionState *cmpl_state)
{
  gchar *pat_buf, *first_slash;
  CompletionDir *dir = cmpl_state->active_completion_dir;

  dir->cmpl_index += 1;

  if(dir->cmpl_index == dir->sent->entry_count)
    {
      /* Note: Can't move the set_dir_ptr outside the if since otherwise
       * you'll be testing freed memory */

      if(dir->cmpl_parent == NULL)
	{
	  set_dir_ptr(&cmpl_state->active_completion_dir, NULL);

	  return NULL;
	}
      else
	{
	  set_dir_ptr(&cmpl_state->active_completion_dir, dir->cmpl_parent);

	  return attempt_file_completion(cmpl_state);
	}
    }

  g_assert(dir->cmpl_text);

  first_slash = strchr(dir->cmpl_text, '/');

  if(first_slash)
    {
      gint len = first_slash - dir->cmpl_text;

      pat_buf = g_new (gchar, len + 1);
      strncpy(pat_buf, dir->cmpl_text, len);
      pat_buf[len] = 0;
    }
  else
    {
      gint len = strlen(dir->cmpl_text);

      pat_buf = g_new (gchar, len + 2);
      strcpy(pat_buf, dir->cmpl_text);
      strcpy(pat_buf + len, "*");
    }

  if(first_slash)
    {
      if(dir->sent->entries[dir->cmpl_index].is_dir)
	{
	  if(fnmatch(pat_buf, dir->sent->entries[dir->cmpl_index].entry_name,
		     FNMATCH_FLAGS) != FNM_NOMATCH)
	    {
	      CompletionDir* new_dir;

	      new_dir = open_relative_dir(dir->sent->entries[dir->cmpl_index].entry_name,
					  dir, cmpl_state);

	      if(!new_dir)
		{
		  g_free (pat_buf);
		  return NULL;
		}

	      set_dir_ptr(&new_dir->cmpl_parent, dir);

	      new_dir->cmpl_index = -1;
	      new_dir->cmpl_text = first_slash + 1;

	      set_dir_ptr(&cmpl_state->active_completion_dir, new_dir);

	      g_free (pat_buf);
	      return attempt_file_completion(cmpl_state);
	    }
	  else
	    {
	      g_free (pat_buf);
	      return attempt_file_completion(cmpl_state);
	    }
	}
      else
	{
	  g_free (pat_buf);
	  return attempt_file_completion(cmpl_state);
	}
    }
  else
    {
      if(dir->cmpl_parent != NULL)
	{
	  append_completion_text(dir->sent->fullname +
				 strlen(cmpl_state->completion_dir->sent->fullname) + 1,
				 cmpl_state);
	  append_completion_text("/", cmpl_state);
	}

      append_completion_text(dir->sent->entries[dir->cmpl_index].entry_name, cmpl_state);

      cmpl_state->the_completion.is_a_completion =
	(fnmatch(pat_buf, dir->sent->entries[dir->cmpl_index].entry_name,
		 FNMATCH_FLAGS) != FNM_NOMATCH);

      cmpl_state->the_completion.is_directory = dir->sent->entries[dir->cmpl_index].is_dir;
      if(dir->sent->entries[dir->cmpl_index].is_dir)
	append_completion_text("/", cmpl_state);

      g_free (pat_buf);
      return &cmpl_state->the_completion;
    }
}


static gint
get_pwdb(CompletionState* cmpl_state)
{
  struct passwd *pwd_ptr;
  gchar* buf_ptr, *home_dir;
  gint len = 0, i, count = 0;

  setpwent ();

  while((pwd_ptr = getpwent()) != NULL)
    {
      len += strlen(pwd_ptr->pw_name);
      len += strlen(pwd_ptr->pw_dir);
      len += 2;
      count += 1;
    }

  home_dir = getenv("HOME");

  if(!home_dir)
    {
      setpwent ();

      pwd_ptr = getpwuid(getuid());
      if(!pwd_ptr)
	{
	  cmpl_errno = errno;
	  goto error;
	}
      home_dir = pwd_ptr->pw_dir;
    }

  len += strlen(home_dir);
  len += 1;

  setpwent ();

  cmpl_state->user_dir_name_buffer = g_new(gchar, len);
  cmpl_state->user_directories = g_new(CompletionUserDir, count);
  cmpl_state->user_directories_len = count;

  buf_ptr = cmpl_state->user_dir_name_buffer;

  strcpy(buf_ptr, home_dir);
  cmpl_state->user_home_dir = buf_ptr;
  buf_ptr += strlen(buf_ptr);
  buf_ptr += 1;

  for(i = 0; i < count; i += 1)
    {
      pwd_ptr = getpwent();
      if(!pwd_ptr)
	{
	  cmpl_errno = errno;
	  goto error;
	}

      strcpy(buf_ptr, pwd_ptr->pw_name);
      cmpl_state->user_directories[i].login = buf_ptr;
      buf_ptr += strlen(buf_ptr);
      buf_ptr += 1;
      strcpy(buf_ptr, pwd_ptr->pw_dir);
      cmpl_state->user_directories[i].homedir = buf_ptr;
      buf_ptr += strlen(buf_ptr);
      buf_ptr += 1;
    }

  qsort(cmpl_state->user_directories,
	cmpl_state->user_directories_len,
	sizeof(CompletionUserDir),
	compare_user_dir);

  endpwent();

  return TRUE;

error:

  if(cmpl_state->user_dir_name_buffer)
    g_free(cmpl_state->user_dir_name_buffer);
  if(cmpl_state->user_directories)
    g_free(cmpl_state->user_directories);

  cmpl_state->user_dir_name_buffer = NULL;
  cmpl_state->user_directories = NULL;

  return FALSE;
}

static gint
compare_user_dir(const void* a, const void* b)
{
  return strcmp((((CompletionUserDir*)a))->login,
		(((CompletionUserDir*)b))->login);
}

static gint
compare_cmpl_dir(const void* a, const void* b)
{
  return strcmp((((CompletionDirEntry*)a))->entry_name,
		(((CompletionDirEntry*)b))->entry_name);
}

static gint
cmpl_state_okay(CompletionState* cmpl_state)
{
  return cmpl_state && cmpl_state->reference_dir;
}

static gchar*
cmpl_strerror(gint err)
{
  if(err == CMPL_ERRNO_TOO_LONG)
    return "Name too long";
  else
    return STRERROR(err);
}

/**********************************************************************/
/*			     Widget Stuff                             */
/**********************************************************************/

GtkWidget*
gtk_file_selection_new (gchar *title,
			gchar *initial_filename)
{
  GtkFileSelection *fs;
  GtkWidget *vbox;
  GtkWidget *action_vbox;
  GtkWidget *dir_vbox;
  GtkWidget *file_vbox;
  GtkWidget *entry_vbox;
  GtkWidget *listbox;
  GtkWidget *label;
  GtkWidget *list_hbox;
  GtkWidget *dir_event_widget;
  GtkWidget *file_event_widget;
  GtkWidget *action_area;
  GtkWidget *ok;
  GtkWidget *cancel;
  GtkWidget *help;

  g_function_enter ("gtk_file_selection_new");

  fs = g_new (GtkFileSelection, 1);
  fs->ok_callback = NULL;
  fs->cancel_callback = NULL;
  fs->help_callback = NULL;
  fs->ok_data = NULL;
  fs->cancel_data = NULL;
  fs->help_data = NULL;

  fs->window = gtk_window_new (title, GTK_WINDOW_DIALOG);
  gtk_widget_set_user_data (fs->window, fs);

  /*  The dialog-sized vertical box  */
  vbox = gtk_vbox_new (FALSE, 10);
  gtk_container_add (fs->window, vbox);

  fs->main_vbox = gtk_vbox_new (FALSE, 10);
  gtk_container_set_border_width (fs->main_vbox, 0);
  gtk_box_pack (vbox, fs->main_vbox, TRUE, TRUE, 0, GTK_PACK_START);

  action_vbox = gtk_vbox_new (FALSE, 10);
  gtk_container_set_border_width (action_vbox, 0);
  gtk_box_pack (vbox, action_vbox, TRUE, TRUE, 0, GTK_PACK_START);


  /*  The horizontal box containing the directory and file listboxes  */
  list_hbox = gtk_hbox_new (TRUE, 5);
  gtk_container_set_border_width (list_hbox, 0);
  gtk_box_pack (fs->main_vbox, list_hbox, TRUE, TRUE, 0, GTK_PACK_START);

  dir_event_widget = gtk_event_widget_new (gtk_file_selection_dir_event, 0, FALSE);
  file_event_widget = gtk_event_widget_new (gtk_file_selection_file_event, 0, FALSE);

  gtk_widget_set_user_data (dir_event_widget, fs);
  gtk_widget_set_user_data (file_event_widget, fs);


  /* The directories listbox  */
  dir_vbox = gtk_vbox_new (FALSE, 2);
  gtk_container_set_border_width (dir_vbox, 0);
  gtk_box_pack (list_hbox, dir_vbox, TRUE, TRUE, 0, GTK_PACK_START);

  label = gtk_label_new ("Directories");
  gtk_label_set_alignment (label, 0.0, 0.5);
  gtk_box_pack (dir_vbox, label, FALSE, FALSE, 0, GTK_PACK_START);
  gtk_widget_show (label);

  listbox = gtk_listbox_new ();
  gtk_listbox_set_shadow_type (listbox, GTK_SHADOW_IN);
  gtk_container_add (dir_event_widget, listbox);
  gtk_box_pack (dir_vbox, dir_event_widget, TRUE, TRUE, 0, GTK_PACK_START);

  gtk_widget_show (listbox);
  gtk_widget_show (dir_event_widget);

  fs->dir_list = gtk_listbox_get_list (listbox);
  gtk_list_set_selection_mode (fs->dir_list, GTK_SELECTION_BROWSE);
  gtk_widget_set_usize (fs->dir_list, DIR_LIST_WIDTH, DIR_LIST_HEIGHT);


  /* The files listbox  */
  file_vbox = gtk_vbox_new (FALSE, 2);
  gtk_container_set_border_width (file_vbox, 0);
  gtk_box_pack (list_hbox, file_vbox, TRUE, TRUE, 0, GTK_PACK_START);

  label = gtk_label_new ("Files");
  gtk_label_set_alignment (label, 0.0, 0.5);
  gtk_box_pack (file_vbox, label, FALSE, FALSE, 0, GTK_PACK_START);
  gtk_widget_show (label);

  listbox = gtk_listbox_new ();
  gtk_listbox_set_shadow_type (listbox, GTK_SHADOW_IN);
  gtk_container_add (file_event_widget, listbox);
  gtk_box_pack (file_vbox, file_event_widget, TRUE, TRUE, 0, GTK_PACK_START);

  gtk_widget_show (listbox);
  gtk_widget_show (file_event_widget);

  fs->file_list = gtk_listbox_get_list (listbox);
  gtk_list_set_selection_mode (fs->file_list, GTK_SELECTION_BROWSE);
  gtk_widget_set_usize (fs->file_list, FILE_LIST_WIDTH, FILE_LIST_HEIGHT);


  /*  The selection entry widget  */
  entry_vbox = gtk_vbox_new (FALSE, 2);
  gtk_container_set_border_width (entry_vbox, 0);
  gtk_box_pack (action_vbox, entry_vbox, FALSE, FALSE, 10, GTK_PACK_START);

  fs->selection_text = label = gtk_label_new ("");
  gtk_label_set_alignment (label, 0.0, 0.5);
  gtk_box_pack (entry_vbox, label, FALSE, FALSE, 0, GTK_PACK_START);
  gtk_widget_show (label);

  fs->selection_entry = gtk_text_entry_new ();
  gtk_text_entry_set_key_function (fs->selection_entry, gtk_file_selection_key_function, fs);
  gtk_box_pack (entry_vbox, fs->selection_entry, TRUE, TRUE, 0, GTK_PACK_START);
  gtk_widget_show (entry_vbox);


  /*  The action area  */
  action_area = gtk_hbox_new (TRUE, 10);
  gtk_container_set_border_width (action_area, 0);
  gtk_box_pack (action_vbox, action_area, FALSE, FALSE, 0, GTK_PACK_START);

  /*  The OK button  */
  ok = gtk_push_button_new ();
  gtk_box_pack (action_area, ok, TRUE, TRUE, 0, GTK_PACK_START);
  label = gtk_label_new ("OK");
  gtk_container_add (ok, label);
  gtk_widget_show (label);
  gtk_callback_add (gtk_button_get_state (ok),
		    gtk_file_selection_ok_callback, fs);
  gtk_widget_grab_default (ok);

  /*  The Cancel button  */
  cancel = gtk_push_button_new ();
  gtk_box_pack (action_area, cancel, TRUE, TRUE, 0, GTK_PACK_START);
  label = gtk_label_new ("Cancel");
  gtk_container_add (cancel, label);
  gtk_widget_show (label);
  gtk_callback_add (gtk_button_get_state (cancel),
		    gtk_file_selection_cancel_callback, fs);

  /*  The Help button  */
  help = gtk_push_button_new ();
  gtk_box_pack (action_area, help, TRUE, TRUE, 0, GTK_PACK_START);
  label = gtk_label_new ("Help");
  gtk_container_add (help, label);
  gtk_widget_show (label);
  gtk_callback_add (gtk_button_get_state (help),
		    gtk_file_selection_help_callback, fs);


  /*  Show all widgets  */
  gtk_widget_show (ok);
  gtk_widget_show (cancel);
  gtk_widget_show (help);
  gtk_widget_show (action_area);
  gtk_widget_show (fs->selection_entry);
  gtk_widget_show (dir_vbox);
  gtk_widget_show (file_vbox);
  gtk_widget_show (list_hbox);
  gtk_widget_show (action_vbox);
  gtk_widget_show (fs->main_vbox);
  gtk_widget_show (vbox);

  fs->cmpl_state = cmpl_init_state();

  if(!cmpl_state_okay(fs->cmpl_state))
    {
      gchar err_buf[256];

      sprintf(err_buf, "Directory unreadable: %s", cmpl_strerror(cmpl_errno));

      gtk_label_set (fs->selection_text, err_buf);
    }
  else
    {
      gtk_file_selection_populate (fs, (initial_filename ? initial_filename : ""), FALSE);
    }

  g_function_leave ("gtk_file_selection_new");

  return (fs->cmpl_state ? ((GtkWidget*) fs->window) : ((GtkWidget*) 0));
}

void
gtk_file_selection_set_ok_callback (GtkWidget *fs_window,
				    GtkCallback ok_callback,
				    gpointer ok_data)
{
  GtkFileSelection *fs;

  g_function_enter ("gtk_file_selection_ok_callback");

  g_assert (fs_window != NULL);

  fs = (GtkFileSelection *) gtk_widget_get_user_data (fs_window);
  fs->ok_callback = ok_callback;
  fs->ok_data = ok_data;

  g_function_leave ("gtk_file_selection_ok_callback");
}

void
gtk_file_selection_set_cancel_callback (GtkWidget *fs_window,
					GtkCallback cancel_callback,
					gpointer cancel_data)
{
  GtkFileSelection *fs;

  g_function_enter ("gtk_file_selection_cancel_callback");

  g_assert (fs_window != NULL);

  fs = (GtkFileSelection *) gtk_widget_get_user_data (fs_window);
  fs->cancel_callback = cancel_callback;
  fs->cancel_data = cancel_data;

  g_function_leave ("gtk_file_selection_cancel_callback");
}

void
gtk_file_selection_set_help_callback (GtkWidget *fs_window,
				      GtkCallback help_callback,
				      gpointer help_data)
{
  GtkFileSelection *fs;

  g_function_enter ("gtk_file_selection_help_callback");

  g_assert (fs_window != NULL);

  fs = (GtkFileSelection *) gtk_widget_get_user_data (fs_window);
  fs->help_callback = help_callback;
  fs->help_data = help_data;

  g_function_leave ("gtk_file_selection_help_callback");
}

void gtk_file_selection_destroy (GtkWidget *fs_window)
{
  GtkFileSelection* fs;

  g_function_enter("gtk_file_selection_destroy");

  fs = (GtkFileSelection*) gtk_widget_get_user_data(fs_window);

  cmpl_free_state (fs->cmpl_state);

  g_function_leave("gtk_file_selection_destroy");
}

GtkWidget* gtk_file_selection_get_main_vbox (GtkWidget* fs_window)
{
  GtkFileSelection* fs;

  g_function_enter("gtk_file_selection_destroy");

  g_assert(fs_window != NULL);

  fs = (GtkFileSelection*) gtk_widget_get_user_data(fs_window);

  g_function_leave("gtk_file_selection_destroy");

  return fs->main_vbox;
}

static void
gtk_file_selection_populate (GtkFileSelection *fs,
			     gchar *rel_path,
			     gint try_complete)
{
  PossibleCompletion* poss;
  GList *dir_list;
  GList *file_list;
  GtkWidget *label;
  gchar* filename;
  gchar* rem_path = rel_path;
  gchar* sel_text;
  gint did_recurse = FALSE;
  gint possible_count = 0;

  g_function_enter ("gtk_file_selection_populate");

  poss = cmpl_completion_matches (rel_path, &rem_path, fs->cmpl_state);

  if(!cmpl_state_okay(fs->cmpl_state))
    {
      gchar err_buf[256];

      sprintf(err_buf, "Directory unreadable: %s", cmpl_strerror(cmpl_errno));

      gtk_label_set(fs->selection_text, err_buf);
    }

  dir_list = NULL;
  file_list = NULL;

  filename = "./";
  label = gtk_list_item_new_with_label (filename);
  gtk_widget_set_user_data (label, g_strdup (filename));
  gtk_widget_show (label);
  dir_list = g_list_prepend (dir_list, label);

  filename = "../";
  label = gtk_list_item_new_with_label (filename);
  gtk_widget_set_user_data (label, g_strdup (filename));
  gtk_widget_show (label);
  dir_list = g_list_prepend (dir_list, label);

  while (poss)
    {
      if (cmpl_is_a_completion (poss))
	{
	  possible_count += 1;

	  filename = cmpl_this_completion (poss);
	  label = gtk_list_item_new_with_label (filename);
	  gtk_widget_set_user_data (label, g_strdup (filename));

	  if (cmpl_is_directory (poss))
	    {
	      if ((strcmp (filename, "./") != 0) &&
		  (strcmp (filename, "../") != 0))
		dir_list = g_list_prepend (dir_list, label);
	    }
	  else
	    file_list = g_list_prepend (file_list, label);

	  /* if recursing, this is probably bad */
	  gtk_widget_show (label);
	}

      poss = cmpl_next_completion (fs->cmpl_state);
    }

  g_assert(fs->cmpl_state->reference_dir);

  if (try_complete)
    {
      if (cmpl_updated_text (fs->cmpl_state)[0])
	{
	  if(cmpl_updated_dir(fs->cmpl_state))
	    {
	      gchar* dir_name = g_strdup(cmpl_updated_text (fs->cmpl_state));

	      did_recurse = TRUE;

	      gtk_file_selection_populate(fs, dir_name, TRUE);

	      g_free(dir_name);
	    }
	  else
	    {
	      gtk_text_entry_set_text (fs->selection_entry,
				       cmpl_updated_text (fs->cmpl_state));
	    }
	}
      else if (possible_count == 0 && rem_path[0])
	{
	  gtk_text_entry_set_position (fs->selection_entry, cmpl_last_valid_char(fs->cmpl_state));
	}
      else
	{
	  gtk_text_entry_set_text (fs->selection_entry, rem_path);
	}
    }
  else
    {
      gtk_text_entry_set_text (fs->selection_entry, "");
    }

  if(!did_recurse)
    {
      gtk_text_entry_set_position (fs->selection_entry, -1);

      sel_text = g_new(char, strlen(cmpl_reference_position(fs->cmpl_state)) +
		             sizeof("Selection: "));
      strcpy(sel_text, "Selection: ");
      strcat(sel_text, cmpl_reference_position(fs->cmpl_state));
      gtk_label_set(fs->selection_text, sel_text);
      g_free(sel_text);

      gtk_container_foreach ((GtkContainer*) fs->dir_list, gtk_file_selection_free_filename, NULL);
      gtk_container_foreach ((GtkContainer*) fs->file_list, gtk_file_selection_free_filename, NULL);

      gtk_list_clear_items (fs->dir_list, 0, -1);
      gtk_list_clear_items (fs->file_list, 0, -1);

      if (dir_list)
	{
	  dir_list = g_list_reverse (dir_list);
	  gtk_list_append_items (fs->dir_list, dir_list);
	}

      if (file_list)
	{
	  file_list = g_list_reverse (file_list);
	  gtk_list_append_items (fs->file_list, file_list);
	}
    }
  else
    {
      /*
      while(dir_list)
	{
	  GList *tmp = dir_list;
	  gtk_file_selection_free_filename(tmp->data, NULL, NULL);
	  gtk_widget_destroy(tmp->data);
	  g_free(tmp);
	  dir_list = dir_list->next;
	}

      while(file_list)
	{
	  GList *tmp = file_list;
	  gtk_file_selection_free_filename(tmp->data, NULL, NULL);
	  gtk_widget_destroy(tmp->data);
	  g_free(tmp);
	  file_list = file_list->next;
	}
      */
    }

  g_function_leave ("gtk_file_selection_populate");
}

static void
gtk_file_selection_ok_callback (GtkWidget * widget,
				gpointer    client_data,
				gpointer    call_data)
{
  GtkFileSelection *fs;
  gchar *text;

  g_function_enter ("gtk_file_selection_ok_callback");

  fs = (GtkFileSelection *) client_data;

  text = gtk_text_entry_get_text (fs->selection_entry);

  if (fs->ok_callback)
    (* fs->ok_callback) (fs->window, fs->ok_data,
			 (gpointer) cmpl_completion_fullname (text, fs->cmpl_state));

  g_function_leave ("gtk_file_selection_ok_callback");
}

static void
gtk_file_selection_cancel_callback (GtkWidget * widget,
				    gpointer    client_data,
				    gpointer    call_data)
{
  GtkFileSelection *fs;

  g_function_enter ("gtk_file_selection_cancel_callback");

  fs = (GtkFileSelection *) client_data;

  /*  call the cancel callback if one was provided  */
  if (fs->cancel_callback)
    (* fs->cancel_callback) (fs->window, fs->cancel_data, NULL);

  g_function_leave ("gtk_file_selection_cancel_callback");
  /*
    gtk_container_foreach ((GtkContainer*) fs->dir_list, gtk_file_selection_free_filename, NULL);
    gtk_container_foreach ((GtkContainer*) fs->file_list, gtk_file_selection_free_filename, NULL);
    gtk_list_clear_items (fs->dir_list, 0, -1);
    gtk_list_clear_items (fs->file_list, 0, -1);
    cmpl_free_state(fs->cmpl_state);
    gtk_exit (0);
  */
}

static void
gtk_file_selection_help_callback (GtkWidget * widget,
				  gpointer    client_data,
				  gpointer    call_data)
{
  GtkFileSelection *fs;

  g_function_enter ("gtk_file_selection_help_callback");

  fs = (GtkFileSelection *) client_data;

  /*  call the help callback if one was provided  */
  if (fs->help_callback)
    (* fs->help_callback) (fs->window, fs->help_data, NULL);

  g_function_leave ("gtk_file_selection_help_callback");
}

static gint
gtk_file_selection_dir_event (GtkWidget *widget,
			      GdkEvent  *event)
{
  GtkWidget *event_widget;
  GtkFileSelection *fs;
  gint return_val;
  gchar *filename;

  g_function_enter ("gtk_file_selection_dir_event");

  g_assert (widget != NULL);
  g_assert (event != NULL);

  return_val = FALSE;
  event_widget = gtk_get_event_widget (event);

  if (GTK_WIDGET_TYPE (event_widget) == gtk_get_list_item_type ())
    {
      filename = gtk_widget_get_user_data (event_widget);
      fs = gtk_widget_get_user_data (widget);

      switch (event->type)
     {
     case GDK_2BUTTON_PRESS:
       gtk_file_selection_populate (fs, filename, FALSE);
       return_val = TRUE;
       break;

     default:
       break;
     }
    }

  g_function_leave ("gtk_file_selection_dir_event");
  return return_val;
}

static gint
gtk_file_selection_file_event (GtkWidget *widget,
			       GdkEvent  *event)
{
  GtkWidget *event_widget;
  GtkFileSelection *fs;
  gchar *filename;

  g_function_enter ("gtk_file_selection_file_event");

  g_assert (widget != NULL);
  g_assert (event != NULL);

  event_widget = gtk_get_event_widget (event);
  if (GTK_WIDGET_TYPE (event_widget) == gtk_get_list_item_type ())
    {
      filename = gtk_widget_get_user_data (event_widget);
      fs = gtk_widget_get_user_data (widget);

      switch (event->type)
	{
	case GDK_2BUTTON_PRESS:
	  /*  josh you motha' fucka', fix this piece of shit you whoremuff  */
	  if (fs->ok_callback)
 	    (* fs->ok_callback) (fs->window, fs->ok_data,
				 (gpointer) cmpl_completion_fullname (filename, fs->cmpl_state));
	  break;

	default:
	  break;
	}
    }

  g_function_leave ("gtk_file_selection_file_event");
  return FALSE;
}

static void
gtk_file_selection_free_filename (GtkWidget *widget,
				  gpointer   client_data,
				  gpointer   call_data)
{
  g_function_enter ("gtk_file_selection_free_filename");

  g_assert (widget != NULL);

  g_free (gtk_widget_get_user_data (widget));
  gtk_widget_set_user_data (widget, NULL);

  g_function_leave ("gtk_file_selection_free_filename");
}

static gint
gtk_file_selection_key_function (guint    keyval,
				 guint    state,
				 gpointer client_data)
{
  GtkFileSelection *fs;
  gchar *text;
  gint return_val;

  g_function_enter ("gtk_file_selection_key_function");

  fs = (GtkFileSelection*) client_data;
  return_val = FALSE;

  if (keyval == XK_Tab)
    {
      return_val = TRUE;

      text = gtk_text_entry_get_text (fs->selection_entry);
      gtk_file_selection_populate (fs, (text) ? (text) : (""), TRUE);
    }
  else if (keyval == XK_Return)
    {
      return_val = TRUE;

      text = gtk_text_entry_get_text (fs->selection_entry);

      /* A file has been selected, call user callback */
      if (fs->ok_callback)
	(* fs->ok_callback) (fs->window, fs->ok_data,
			     (gpointer) cmpl_completion_fullname (text, fs->cmpl_state));
    }

  g_function_leave ("gtk_file_selection_key_function");
  return return_val;
}
