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
 
#include <gtk/gtk.h>
#include <config.h>

/**
 * Glista main header file
 */

#ifndef _GLISTA_HEADER

#define _XOPEN_SOURCE

#ifndef GLISTA_UI_DIR 
#define GLISTA_UI_DIR "."
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

#ifdef PACKAGE_NAME
#define GLISTA_PROGNAME PACKAGE_NAME
#else
#define GLISTA_PROGNAME "glista"
#endif

// Text colors
#ifndef GLISTA_COLOR_PENDING
#define GLISTA_COLOR_PENDING "#000000"
#endif

#ifndef GLISTA_COLOR_DONE
#define GLISTA_COLOR_DONE "#a0a0a0"
#endif

// Some useful macros
#define glista_get_widget(w) gtk_builder_get_object(gl_globs->uibuilder, w)

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
	GtkTreeStore  *itemstore;  // Item storage
	GHashTable    *categories; // HT containing pointers to item categories
	GtkBuilder    *uibuilder;  // UI Builder
	gchar         *configdir;  // Configuration directory path
	GlistaConfig  *config;     // Configuration Data
	guint          save_tag;   // Data save timeout tag - see g_timeout_add()
	GtkTreeIter   *open_note;  // Iterator pointing to the current open note
} GlistaGlobals;

// Glista item data structure
typedef struct _glista_data_struct {
	gboolean  done;
	gchar    *text;
	gchar    *parent;
	gchar    *note;
} GlistaItem;	

// Globals container
static GlistaGlobals *gl_globs;

// Function Prototypes
GlistaItem  *glista_item_new(const gchar *text, const gchar *parent);
void         glista_item_create_from_text(gchar *text);
void         glista_item_toggle_done(GtkTreePath *path);
void         glista_item_change_text(GtkTreePath *path, gchar *text);
void         glista_item_redraw_parent(GtkTreeIter *child_iter);
void         glista_item_free(GlistaItem *item);
GtkTreeIter *glista_item_get_single_selected(GtkTreeSelection *selection);
void         glista_list_save_timeout();
void         glista_list_delete_done();
void         glista_list_delete_selected();
void         glista_ui_mainwindow_show();
void         glista_ui_mainwindow_hide();
void         glista_ui_mainwindow_toggle();
void         glista_note_toggle(GtkTreeIter *iter);
void         glista_note_toggle_selected(GtkTreeSelection *selection);
void         glista_note_open_if_visible(GtkTreeIter *iter);
void         glista_note_close();

// Column names & order ENUM
typedef enum {
	GL_COLUMN_DONE,
	GL_COLUMN_TEXT,
	GL_COLUMN_CATEGORY,
	GL_COLUMN_NOTE
} GlistaColumn;

#define _GLISTA_HEADER
#endif

