/**
 * Glista - A simple task list management utility
 * Copyright (C) 2008 Shahar Evron, shahar@prematureoptimization.org
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
 
/**
 * Glista main header file
 */

#ifndef __GLISTA_H

#include <sys/types.h>

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif 

#include <glib.h>
#include <gtk/gtk.h>

#define _XOPEN_SOURCE

#ifndef GLISTA_DATA_DIR 
#define GLISTA_DATA_DIR "."
#endif

#ifndef GLISTA_CONFIG_DIR
#define GLISTA_CONFIG_DIR "glista"
#endif

#ifndef GLISTA_SAVE_TIMEOUT
#define GLISTA_SAVE_TIMEOUT 3000
#endif

#ifndef GLISTA_CAT_DELIM
#define GLISTA_CAT_DELIM ":"
#endif

#ifndef GLISTA_PARAM_STRING
#define GLISTA_PARAM_STRING "- a super-simple personal to-do list manager"
#endif

#ifndef PACKAGE_NAME
#deinfe PACKAGE_NAME "glista"
#endif

// A couple of convenience macros to access the item store
#define GL_ITEMSTM GTK_TREE_MODEL(gl_globs->itemstore)
#define GL_ITEMSTS GTK_TREE_STORE(gl_globs->itemstore)

// Glista configuration data struct
typedef struct _glista_config_struct {
	gint     xpos;
	gint     ypos;
	gint     width;
	gint     height;
	gboolean visible;
	gboolean note_vpane_pos;
} GlistaConfig;

// Glista globals container struct
typedef struct  _glista_globals_struct {
	GlistaConfig  *config;     // Configuration Data
	GtkTreeStore  *itemstore;  // Item storage
	GHashTable    *categories; // HT containing pointers to item categories
	GtkBuilder    *uibuilder;  // UI Builder
	gchar         *configdir;  // Configuration directory path
	GtkTreeIter   *open_note;  // Iterator pointing to the current open note
	guint          save_tag;   // Data save timeout tag - see g_timeout_add()
	gboolean       trayicon;   // Whether to use system tray icon or not
} GlistaGlobals;

// Glista item data structure
typedef struct _glista_data_struct {
	gboolean  done;
	gchar    *text;
	gchar    *parent;
	gchar    *note;
	time_t    remind_at;
} GlistaItem;

// Globals container
GlistaGlobals *gl_globs;

// Function Prototypes
GlistaItem  *glista_item_new(const gchar *text, const gchar *parent);
void         glista_item_create_from_text(gchar *text);
void         glista_item_toggle_done(GtkTreePath *path);
void         glista_item_change_text(GtkTreePath *path, gchar *text);
void         glista_item_redraw_parent(GtkTreeIter *child_iter);
void         glista_item_free(GlistaItem *item);
GtkTreeIter *glista_item_get_single_selected(GtkTreeSelection *selection);
void         glista_list_save_timeout();
GList*       glista_list_get_selected();
void         glista_list_delete_done();
void         glista_list_delete_selected();
void         glista_note_toggle(GtkTreeIter *iter);
void         glista_note_toggle_selected(GtkTreeSelection *selection);
void         glista_note_open_if_visible(GtkTreeIter *iter);
void         glista_note_close();
void         glista_note_clear_selected();

// Column names & order ENUM
typedef enum {
	GL_COLUMN_DONE,
	GL_COLUMN_TEXT,
	GL_COLUMN_CATEGORY,
	GL_COLUMN_NOTE,
	GL_COLUMN_REMINDER
} GlistaColumn;

#define __GLISTA_H
#endif
