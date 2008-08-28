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
} GlistaConfig;

// Glista globals container struct
typedef struct  _glista_globals_struct {
	GtkListStore  *itemstore;  // GtkListStore for the list of items
	GtkBuilder    *uibuilder;  // UI Builder
	gchar         *configdir;  // Configuration directory path
	GlistaConfig  *config;     // Configuration Data
	guint          save_tag;   // Data save timeout tag - see g_timeout_add()
} GlistaGlobals;

// Glista item data structure
typedef struct _glista_data_struct {
	gboolean  done;
	gchar    *text;
} GlistaItem;	

// Globals container
static GlistaGlobals *gl_globs;

// Function Prototypes
void        glista_add_to_list(GlistaItem *item);
void        glista_toggle_item_done(GtkTreePath *path);
void        glista_change_item_text(GtkTreePath *path, gchar *text);
void        glista_clear_done_items();
void        glista_delete_selected_items();
void        glista_toggle_main_window_visible();
GlistaItem *glista_item_new(const gchar *text);
void        glista_item_free(GlistaItem *item);
void        glista_save_list_timeout();
void        glista_store_window_geomerty(gint x, gint y, 
										 gint width, gint height);

// Column names & order ENUM
typedef enum {
	GL_COLUMN_DONE,
	GL_COLUMN_TEXT,
	GL_COLUMN_COLOR
} GlistaColumn;

#define _GLISTA_HEADER
#endif

