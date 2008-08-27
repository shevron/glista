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

#include <string.h>
#include <glib.h>
#include <glib-object.h>
#include <gtk/gtk.h>
#include <gtk/gtkmain.h>
#include "glista.h"
#include "ui-callbacks.c"
#include "storage.c"

/**
 * Glista main program functions
 */

static int errno = 0;

/**
 * glista_add_to_list:
 * @text: Item text
 *
 * Adds an additional to-do item to the list. The item text must be provided, 
 * and all other values (done, color, etc.) are set to default values. Text
 * is stripped of leading and trailing spaces, and empty strings are ignored.
 */
void
glista_add_to_list(GlistaItem *item)
{
	GtkTreeIter  iter;
	
	gtk_list_store_append(gl_globs->itemstore, &iter);
	gtk_list_store_set(gl_globs->itemstore, &iter, 
	                   GL_COLUMN_DONE, item->done, 
	                   GL_COLUMN_TEXT, item->text, -1);
	                   
}

/**
 * glista_toggle_item_done:
 * @path: The path in the list to toggle
 *
 * Toggle the "done" flag on an item in the to-do list. This function only 
 * negates the value of the "done" column on @path. Other things like resorting,
 * coloring, etc. is automatically handled in the GtkTreeView layer.
 */
void 
glista_toggle_item_done(GtkTreePath *path)
{
	GtkTreeIter  iter;
	gboolean     current; 

	if (gtk_tree_model_get_iter(GTK_TREE_MODEL(gl_globs->itemstore), 
								&iter, path)) {
									
		gtk_tree_model_get(GTK_TREE_MODEL(gl_globs->itemstore), &iter, 
		                   GL_COLUMN_DONE, &current, -1);
		                   
		gtk_list_store_set(gl_globs->itemstore, &iter, GL_COLUMN_DONE, 
		                   (! current), -1);
	}
}

/**
 * glista_clear_done_items:
 * 
 * Clear off all the items marked as "done" from the list. Normally this is 
 * called when the "Clear" button is activated.
 */
void 
glista_clear_done_items()
{
	GtkTreeIter  iter;
	GtkTreePath *path;
	gboolean     status, is_done;
	
	// Get the iter set for first row
	status = gtk_tree_model_get_iter_first(GTK_TREE_MODEL(gl_globs->itemstore), 
	                                       &iter);
	                                       
	while (status) {
		gtk_tree_model_get(GTK_TREE_MODEL(gl_globs->itemstore), &iter, 
		                   GL_COLUMN_DONE, &is_done, -1);
		
		if (is_done) {
			path = gtk_tree_model_get_path(GTK_TREE_MODEL(gl_globs->itemstore), 
										   &iter);
			status = gtk_list_store_remove(gl_globs->itemstore, &iter);
			gtk_tree_model_row_deleted(GTK_TREE_MODEL(gl_globs->itemstore), 
									   path);
			gtk_tree_path_free(path);
			
		} else {
			status = gtk_tree_model_iter_next(
				GTK_TREE_MODEL(gl_globs->itemstore), &iter);
		}
	}
}

/**
 * glista_populate_reflist_cb:
 * @model:    Tree model to iterate on
 * @path:     Current path in tree
 * @iter:     Current iter in tree
 * @ref_list: List to populate with row references
 *
 * Callback function for gtk_tree_selection_selected_foreach(). Populates a list
 * of references to all selected rows in the list. 
 *
 * This function is called from glista_delete_selected_items() in order to build
 * a list of references to rows to delete, because rows cannot be deleted 
 * directly.
 */
static void 
glista_populate_reflist_cb(GtkTreeModel *model, GtkTreePath *path,
                           GtkTreeIter *iter, GList **ref_list)
{
	GtkTreeRowReference *ref;
	
	g_assert(ref_list != NULL);
	ref = gtk_tree_row_reference_new(model, path);
	*ref_list = g_list_append(*ref_list, ref);
}

/**
 * glista_delete_selected_items:
 *
 * Delete all selected items from the list.
 */
void 
glista_delete_selected_items()
{
	GtkTreeView      *treeview;
	GtkTreeSelection *selection;
	GList            *ref_list, *node;
	
	treeview = GTK_TREE_VIEW(glista_get_widget("glista_item_list"));
	selection = gtk_tree_view_get_selection(treeview);
	
	// Populate list of row references to delete
	ref_list = NULL;
	gtk_tree_selection_selected_foreach(
		selection, 
	    (GtkTreeSelectionForeachFunc) glista_populate_reflist_cb,
	    &ref_list
    );

	// Iterate over reference list, deleting all rows in it
	for (node = ref_list; node != NULL; node = node->next) {
	    GtkTreePath *path;

        path = gtk_tree_row_reference_get_path(
			(GtkTreeRowReference *)node->data
		);
		
        if (path) {
        	GtkTreeIter  iter;

	        if (gtk_tree_model_get_iter(GTK_TREE_MODEL(gl_globs->itemstore), 
										&iter, path)) {
											
        		gtk_list_store_remove(gl_globs->itemstore, &iter);
			}

			gtk_tree_path_free(path);
        }
	}

	// Free reference list
	g_list_foreach(ref_list, (GFunc) gtk_tree_row_reference_free, NULL);
    g_list_free(ref_list);
}

/**
 * glista_toggle_main_window_visible:
 *
 * Toggle the visibility of the main window. This is normally called when the
 * user left-clicks the status icon in the system tray.
 */
void 
glista_toggle_main_window_visible()
{
	gboolean   current;
	GtkWidget *window;
	
	window = GTK_WIDGET(glista_get_widget("glista_main_window"));
	                                           
	// Get current window state
	g_object_get(window, "visible", &current, NULL);
	
	// If hidden - show, if visible - hide
	if (current) {
		gtk_widget_hide(window);
	} else {
		gtk_widget_show_all(window);
	}
}

void
glista_change_item_text(GtkTreePath *path, gchar *text)
{
	GtkTreeIter  iter;

	if (gtk_tree_model_get_iter(GTK_TREE_MODEL(gl_globs->itemstore), 
								&iter, path)) {
									
		gtk_list_store_set(gl_globs->itemstore, &iter, 
		                   GL_COLUMN_TEXT, text, -1);
	}
}

/**
 * glista_item_new:
 * @text: Item text
 * 
 * Create a new GlistaItem object and set the text property of it. Will also
 * set the other properties to their default values. The item will later need
 * to be freed using glista_item_free().
 *
 * Return: a newly created GlistaItem struct
 */
GlistaItem*
glista_item_new(const gchar *text)
{
	GlistaItem *item;
	
	item = g_malloc(sizeof(GlistaItem));
	item->done = FALSE;
	item->text = (gchar *) text;
	
	return item;
}

/**
 * glista_item_free:
 * @item: The item to free
 *
 * Free an allocated GlistaItem struct
 */
void 
glista_item_free(GlistaItem *item)
{
	g_free(item);
}

/**
 * glista_item_text_cell_data_func:
 * @column: Column to be rendered
 * @cell:   Cell to be rendered
 * @model:  Related data model
 * @iter:   Tree iterator
 * @data:   User data passed at connect time
 *
 * Callback function called whenever an item's text cell needs to be rendered. 
 * Will define the foreground color of the text - gray if the item is done (by
 * default at least), and black if it is pending.
 *
 * See gtk_tree_view_column_set_cell_data_func() for more info.
 */
void
glista_item_text_cell_data_func(GtkTreeViewColumn *column, 
                                GtkCellRenderer *cell, GtkTreeModel *model,
                                GtkTreeIter *iter, gpointer data)
{
	gboolean done; 

	gtk_tree_model_get(GTK_TREE_MODEL(gl_globs->itemstore), iter, 
		               GL_COLUMN_DONE, &done, -1);
		                   
	if (done == TRUE) {
		g_object_set(cell, "foreground", GLISTA_COLOR_DONE, NULL);
	} else {
		g_object_set(cell, "foreground", GLISTA_COLOR_PENDING, NULL);
	}
}

/**
 * glista_list_sort_func:
 * @model:     Model being sorted
 * @row_a:     First row to compare
 * @row_b:     Second row to compare
 * @user_data: User data passed at connect time
 *
 * Callback sorting function for the list of items. Will put pending items 
 * first, and done items at the bottom. Secondary sorting is done 
 * alphabetically. 
 *
 * Returns: negative if row_a sorts higher, 0 if both are equal, or positive if
 * row_b sorts first.
 *
 * See gtk_tree_sortable_set_sort_func() for more info.
 */
gint
glista_list_sort_func(GtkTreeModel *model, GtkTreeIter *row_a, 
                      GtkTreeIter *row_b, gpointer user_data)
{
	gboolean done_a, done_b; 
	gint     ret;
	
	// First, compare done / not done
	gtk_tree_model_get(model, row_a, GL_COLUMN_DONE, &done_a, -1);
	gtk_tree_model_get(model, row_b, GL_COLUMN_DONE, &done_b, -1);
	
	// If they are equal, go on to comparing the text
	if (done_a == done_b) {
		gchar *text_a, *text_b;
	
		gtk_tree_model_get(model, row_a, GL_COLUMN_TEXT, &text_a, -1);
		gtk_tree_model_get(model, row_b, GL_COLUMN_TEXT, &text_b, -1);
		
		if (text_a == NULL || text_b == NULL) {
			if (text_a == NULL && text_b == NULL) {
				ret = 0;
			} else {
				ret = (text_a == NULL) ? -1 : 1;
			}
		} else {
			ret = g_utf8_collate(text_a, text_b);
		}
		
		g_free(text_a);
		g_free(text_b);

	// If they are not equal, order the pending tasks on top
	} else {
		ret = (done_a ? 1 : 0) - (done_b ? 1 : 0);
	}
	
	return ret;
}

/**
 * glista_init_list:
 *
 * Initialize the list of items and the view layer to display it.
 */
static 
void glista_init_list()
{
	GtkCellRenderer   *text_ren, *done_ren;
	GtkTreeViewColumn *text_column, *done_column;
	GtkTreeView       *treeview;
	GtkTreeSelection  *selection;
	GList             *item, *all_items = NULL;
	
	treeview = GTK_TREE_VIEW(glista_get_widget("glista_item_list"));
	
	text_ren = gtk_cell_renderer_text_new();
	done_ren = gtk_cell_renderer_toggle_new();
	g_object_set(text_ren, "editable", TRUE, NULL);
	g_signal_connect(text_ren, "edited", G_CALLBACK(on_item_text_edited), NULL);

	// Connect the toggled event of the "done" column to change the model
	g_signal_connect(done_ren, "toggled", 
					 G_CALLBACK(on_item_done_toggled), NULL);

	done_column = gtk_tree_view_column_new_with_attributes("Done", done_ren, 
		"active", GL_COLUMN_DONE, NULL);	
	text_column = gtk_tree_view_column_new_with_attributes("Item", text_ren, 
		"text", GL_COLUMN_TEXT, "strikethrough", GL_COLUMN_DONE, NULL);
	
	gtk_tree_view_column_set_cell_data_func(text_column, text_ren, 
	                                        glista_item_text_cell_data_func,
	                                        NULL, NULL);
	
	gtk_tree_view_append_column(treeview, done_column);	
	gtk_tree_view_append_column(treeview, text_column);
	gtk_tree_view_set_model(treeview, GTK_TREE_MODEL(gl_globs->itemstore));
	
	// Set sort function and column
	gtk_tree_sortable_set_sort_func(GTK_TREE_SORTABLE(gl_globs->itemstore), 
	                                GL_COLUMN_DONE, glista_list_sort_func,
	                                NULL, NULL);
	                                
	gtk_tree_sortable_set_sort_column_id(GTK_TREE_SORTABLE(gl_globs->itemstore),
	                                     GL_COLUMN_DONE, GTK_SORT_ASCENDING);
	                                     
	// Set selection mode and connect selection changed event
	selection = gtk_tree_view_get_selection(treeview);
	gtk_tree_selection_set_mode(selection, GTK_SELECTION_MULTIPLE);
	g_signal_connect(selection, "changed", 
	                 G_CALLBACK(on_glista_item_list_selection_changed), NULL);
	
	// Load data
	glista_storage_load_all_items(&all_items);
	for (item = all_items; item != NULL; item = item->next) {
		glista_add_to_list(item->data);
		glista_item_free(item->data);
	}
	g_list_free(all_items);
}

/**
 * glista_save_list:
 *
 * Tell the storage module to save the entire list of items. Implements a simple
 * locking mechanism. 
 * 
 * Returns: TRUE if save was successful, or FALSE if currently locked 
 * (meaning another save is in progress).
 */
static gboolean
glista_save_list()
{
	GList           *all_items = NULL;
	GtkTreeIter      iter;
	GlistaItem      *item;
	static gboolean  locked = FALSE;
	
	if (locked) {
		return FALSE;
	}
	
	locked = TRUE;
	
	if (gtk_tree_model_get_iter_first(GTK_TREE_MODEL(gl_globs->itemstore), 
	                                  &iter)) {
	                                  
		do {
			item = glista_item_new(NULL);
			
			gtk_tree_model_get(GTK_TREE_MODEL(gl_globs->itemstore), &iter, 
		                       GL_COLUMN_DONE, &item->done, 
		                       GL_COLUMN_TEXT, &item->text, -1);
		                       
    		all_items = g_list_append(all_items, item);
    		
    	} while (gtk_tree_model_iter_next(GTK_TREE_MODEL(gl_globs->itemstore), 
    	                                  &iter));
    	
    	glista_storage_save_all_items(all_items);
    	
    	// Free items list
    	while (all_items != NULL) {
    		glista_item_free(all_items->data);
    		all_items = all_items->next;
		}
		g_list_free(all_items);
	}
	
	locked = FALSE;
	return TRUE;
}

/**
 * glista_save_list_timeout_cb: 
 * @user_data: User data passed when timeout was created
 * 
 * Called by glista_save_list_timeout() after a timeout of X ms. Will try to 
 * save the list to storage by calling glista_save_list(). If save was not 
 * successful, will run again.
 *
 * Returns: FALSE if no need to run again, TRUE otherwise.
 */
static gboolean
glista_save_list_timeout_cb(gpointer user_data)
{
	if (glista_save_list()) {
		gl_globs->save_tag = 0;
		return FALSE;
	} else {
		return TRUE;
	}
}

/**
 * glista_save_list_timeout:
 *
 * Called whenever the user changes something in the data model. Will schedule 
 * an X ms timeout and call the save procedure after that time. If additional
 * save requests are recieved at that period, will postpone saving until we have
 * X ms of idle time. 
 */
void 
glista_save_list_timeout()
{
	if (gl_globs->save_tag != 0) {
		g_source_remove(gl_globs->save_tag);
	}
	
	gl_globs->save_tag = g_timeout_add(GLISTA_SAVE_TIMEOUT, 
	                                   glista_save_list_timeout_cb, NULL);
}

/**
 * glista_verify_config_dir:
 *
 * Make sure the configuration directory exists. 
 * Returns: TRUE if directory exists or was successfuly created, or FALSE if 
 * there was an error creating it.
 */
static gboolean
glista_verify_config_dir()
{
	if (! g_file_test(gl_globs->configdir, G_FILE_TEST_IS_DIR)) {
		if (! g_file_test(gl_globs->configdir, G_FILE_TEST_EXISTS)) {
			
			// Create the directory
			errno = 0;
			if (g_mkdir_with_parents(gl_globs->configdir, 0700) != 0) {
				// We have an error!
				g_printerr("Error creating configuration directory: %d\n",
				            errno);
				
				return FALSE;
	        }
	        
        } else { // File exists but it is not a directory
        	g_printerr("Error: unable to create config directory" 
        	           " (%s): file exists\n", gl_globs->configdir);
			
			return FALSE;
		}
	}
	
	return TRUE;
}

/**
 * glista_init_load_configuration:
 * 
 * Initialize and load the program's configuration data from file
 * 
 * Return: configuration data in a newly allocated GlistaConfig struct
 */
static GlistaConfig*
glista_init_load_configuration()
{
	gchar        *cfgfile;
	GKeyFile     *keyfile;
	GlistaConfig *config; 
	GError       *error = NULL;
	
	// Initialize configuration stuct
	config = g_malloc(sizeof(GlistaConfig));
	config->xpos    = 0;
	config->ypos    = 0;
	config->visible = TRUE;
	
	cfgfile = g_build_filename(gl_globs->configdir, "glista.conf", NULL);
	
	glista_verify_config_dir();

	// Load configuration data from file
	keyfile = g_key_file_new();
	if (g_key_file_load_from_file(keyfile, cfgfile, G_KEY_FILE_NONE, &error)) {
		config->visible = g_key_file_get_boolean(keyfile, 
		                                      "glistaui", "visible", NULL);
        config->xpos = g_key_file_get_integer(keyfile, 
                                              "glistaui", "xpos", NULL);
    	config->ypos = g_key_file_get_integer(keyfile, 
    	                                      "glistaui", "ypos", NULL);
		
	} else {
		if (error != NULL) {
			fprintf(stderr, "Error loading config file: [%d] %s\n"
			                "Using default configuration.\n", 
			                error->code, error->message);
			                
            g_error_free(error);
        }
	}
	
	g_key_file_free(keyfile);
	g_free(cfgfile);
	
	return config;
}

/**
 * glista_save_configuration:
 *
 * Save the configuration data to file
 */
static void
glista_save_configuration()
{
	gchar     *cfgfile, *cfgdata;
	gsize      cfgdatalen;
	GKeyFile  *keyfile;
	GtkWindow *window;
	gboolean   visible;
	gint       xpos, ypos;
	GError    *error = NULL;

	window = GTK_WINDOW(glista_get_widget("glista_main_window"));
	cfgfile = g_build_filename(gl_globs->configdir, "glista.conf", NULL);
	keyfile = g_key_file_new();
	
	// Set window visibility
	g_object_get(window, "visible", &visible, NULL);
	g_key_file_set_boolean(keyfile, "glistaui", "visible", visible);
	
	// If window is invisible, show it so we can get the position 
	// (this is the only way I could get it to work...)
	if (! visible) {
		gtk_widget_show(GTK_WIDGET(window));
	}
	
	// Set window position
	gtk_window_get_position(window, &xpos, &ypos);
	g_key_file_set_integer(keyfile, "glistaui", "xpos", xpos);
	g_key_file_set_integer(keyfile, "glistaui", "ypos", ypos);	
	
	glista_verify_config_dir();
		
	// Save configuration file
	cfgdata = g_key_file_to_data(keyfile, &cfgdatalen, NULL);
	if (! g_file_set_contents(cfgfile, cfgdata, cfgdatalen, &error)) {
		if (error != NULL) {
			fprintf(stderr, "Error saving configuration file: [%d] %s\n",
			                error->code, error->message);
			                
            g_error_free(error);
        }
	}
	
	g_key_file_free(keyfile);
	g_free(cfgdata);
	g_free(cfgfile);
}

/**
 * main:
 * @argc: Number of arguments
 * @argv: Command line arguments
 *
 * Main program function (do I need to explain this?)
 */
int 
main(int argc, char *argv[])
{
	GtkWidget      *window;
	GtkAboutDialog *about;
	GtkStatusIcon  *sysicon;
	GlistaConfig   *config;
		
	gtk_init(&argc, &argv);

	g_type_init();
	g_thread_init(NULL);
	
	// Initialize globals
	gl_globs = g_malloc(sizeof(GlistaGlobals));
	gl_globs->save_tag  = 0;
	gl_globs->itemstore = gtk_list_store_new(2, G_TYPE_BOOLEAN, G_TYPE_STRING);
	gl_globs->uibuilder = gtk_builder_new();
	gl_globs->configdir = g_build_filename(g_get_user_config_dir(),
	                                       GLISTA_CONFIG_DIR,
	                                       NULL);

	// Load UI file
	if (gtk_builder_add_from_file(gl_globs->uibuilder, 
								  GLISTA_UI_DIR "/glista.ui", NULL) == 0) {
									  
		g_printerr("Unable to read UI file: %s\n", GLISTA_UI_DIR "/glista.ui");
		return 1;
	}

	// Load main window and connect signals
	window = GTK_WIDGET(glista_get_widget("glista_main_window"));
	gtk_builder_connect_signals(gl_globs->uibuilder, NULL);
	g_signal_connect(window, "destroy", 
					 G_CALLBACK(on_glista_main_window_destroy), NULL);
	
	// Set the version number in the about dialog
#ifdef PACKAGE_VERSION
	about = GTK_ABOUT_DIALOG(glista_get_widget("glista_about_dialog"));
	gtk_about_dialog_set_version(about, PACKAGE_VERSION);
#endif

	// Initialize the item list
	glista_init_list();
	
	// Set up the status icon and connect the left-click and right-click signals
	sysicon = gtk_status_icon_new_from_file(GLISTA_UI_DIR "/glista-icon.svg");
	g_signal_connect(sysicon, "activate", 
					 G_CALLBACK(on_sysicon_activate), window);
	g_signal_connect(sysicon, "popup-menu", 
					 G_CALLBACK(on_sysicon_popup_menu), NULL);

	// Load configuration
	config = glista_init_load_configuration();
	
	// Set configuration on main window
	gtk_widget_show_all(window);
	if (config->xpos > 0 && config->ypos > 0) {
		gtk_window_move(GTK_WINDOW(window), config->xpos, config->ypos);
	}
	if (! config->visible) {
		gtk_widget_hide(window);
	}

	g_free(config);

	// Hook up model change signals to the data save handler
	g_signal_connect(gl_globs->itemstore, "row-changed", 
		G_CALLBACK(on_itemstore_row_changed), NULL);
	g_signal_connect(gl_globs->itemstore, "row-deleted", 
		G_CALLBACK(on_itemstore_row_deleted), NULL);
	g_signal_connect(gl_globs->itemstore, "row-inserted", 
		G_CALLBACK(on_itemstore_row_inserted), NULL);

	// Run main loop
	gtk_main();
	
	// Save list
	while (! glista_save_list());
	
	// Save configuration
	glista_save_configuration();

	// Free globals
	g_free(gl_globs->configdir);
	g_free(gl_globs);

	// Normal termination
	return 0;
}

