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
#include <string.h>
#include "glista-ui.h"
#include "glista.h"

#ifdef ENABLE_LINKIFY
#include "glista-textview-linkify.h"
#endif

/**
 * Glista - GTK UI Functions and event handler callbacks
 *
 * The functions in this file should have no functionality beyond handling the 
 * GTK user interface and passing events on to the related glista functions 
 * defined elsewhere.
 */

/**
 * on_list_selection_changed:
 * @selection: The tree's GtkTreeSelection object
 * @user_data: User data passed when the event was connected
 *
 * Called when the selection in the list was changed. The purpose is to know 
 * whether some buttons in need to be disabled when no items are selected.
 */
void 
on_list_selection_changed(GtkTreeSelection *selection, 
                                      gpointer user_data)
{
	GtkWidget   *clear_btn, *note_btn;
	GtkTreeIter *iter;

	clear_btn = GTK_WIDGET(glista_get_widget("tb_delete"));
	note_btn = GTK_WIDGET(glista_get_widget("tb_note"));
	
	if (gtk_tree_selection_count_selected_rows(selection) > 0) {
		if ((iter = glista_item_get_single_selected(selection)) != NULL) {
			gtk_widget_set_sensitive(note_btn, TRUE);
			glista_note_open_if_visible(iter);
			gtk_tree_iter_free(iter);
			
		} else {
			gtk_widget_set_sensitive(note_btn, FALSE);
			glista_note_close();
		}
		
		gtk_widget_set_sensitive(clear_btn, TRUE);
		
	} else {
		gtk_widget_set_sensitive(clear_btn, FALSE);
		gtk_widget_set_sensitive(note_btn, FALSE);
		glista_note_close();
	}
}

/**
 * on_sysicon_activate:
 * @object:    The object that triggered the event
 * @user_data: User data passed when the event was connected
 *
 * Called when the system tray status icon is clicked. Simply calls
 * glista_ui_mainwindow_toggle(), unless we know the event is a double-click
 * or triple-click event, in which case we don nothing.
 */
static void 
on_sysicon_activate(GtkObject *object, GtkWindow *window)
{
	GdkEvent *event;
	
	if ((event = gdk_event_get()) != NULL) { 
		if (event->type != GDK_2BUTTON_PRESS  &&
	        event->type != GDK_3BUTTON_PRESS) {
	        	
	        glista_ui_mainwindow_toggle();
	    }
	    
	    gdk_event_free(event);	
	     
	} else {
		glista_ui_mainwindow_toggle();
	}
}

/**
 * on_sysicon_popup_menu:
 * @icon:          The status icon
 * @button:        The button that was clicked
 * @activate_time: The activation time
 * @user_data:     User data passed when the event was connected
 *
 * Called when the user triggers the context menu of the status icon (usually by
 * right-clicking it). Shows the context menu using gtk_menu_popup() and relies
 * on gtk_status_icon_position_menu() to correctly position it.
 */
static void 
on_sysicon_popup_menu(GtkStatusIcon *icon, guint button, guint activate_time, 
                      gpointer user_data)
{
	GtkMenu *menu;
	
	menu = GTK_MENU(glista_get_widget("sysicon_menu"));
	gtk_menu_popup(menu, NULL, NULL, gtk_status_icon_position_menu, icon,
	               button, activate_time);
}

/**
 * on_item_text_edited:
 * @renderer:  Cell renderer
 * @pathstr:   edited path as string
 * @text:      new text
 * @user_data: data passed during signal connect
 *
 * Called when the user edited item text. Will call glista_item_change_text(), 
 * or ignore if the new text is invalid (eg. empty).
 */
void 
on_item_text_edited(GtkCellRendererText *renderer, gchar *pathstr, 
                    gchar *text, gpointer user_data)
{
	GtkTreePath *path;

	text = g_strstrip(text);

	if (strlen(text) > 0) {
		path = gtk_tree_path_new_from_string(pathstr);	
		glista_item_change_text(path, text);
		gtk_tree_path_free(path);
	}
}

/**
 * on_item_text_editing_started:
 * @renderer  The cell renderer of the edited cell
 * @editable  The GtkEntry created when the cell is edited
 * @pathstr   The path of the cell being edited as string
 * @user_data Data passed in at bind time
 *
 * Called when the user starts editing a cell. Will make sure that the user 
 * edits the text in the model and not the text displayed by the view - this 
 * is important because category names are displayed differently than they are
 * stored in the model. 
 */
void
on_item_text_editing_started(GtkCellRenderer *renderer, 
							 GtkCellEditable *editable, gchar *pathstr, 
							 gpointer user_data)
{
	GtkTreeIter  iter;
	GtkTreePath *path;
	gchar       *text;
	
	if (GTK_IS_ENTRY(editable)) {
		
		if (((path = gtk_tree_path_new_from_string(pathstr)) != NULL) &&
			gtk_tree_model_get_iter(GTK_TREE_MODEL(gl_globs->itemstore), 
									&iter, path)) {
		
			gtk_tree_model_get(GTK_TREE_MODEL(gl_globs->itemstore), &iter, 
							   GL_COLUMN_TEXT, &text, -1);
			
			gtk_entry_set_text(GTK_ENTRY(editable), text);
										
			g_free(text);
			gtk_tree_path_free(path);
		}
	}
}

/**
 * on_item_done_toggled:
 * @renderer:  Toggled item cell renderer
 * @pathstr:   Toggles path in tree
 * @user_data: User data passed when the event was connected
 *
 * Called when an item in the list is marked as done / not done. Calls 
 * glista_item_toggle_done().
 */
void 
on_item_done_toggled(GtkCellRendererToggle *renderer, gchar *pathstr, 
                     gpointer user_data)
{
	GtkTreePath *path;
	
	path = gtk_tree_path_new_from_string(pathstr);	
	glista_item_toggle_done(path);
	
	gtk_tree_path_free(path);
}

/**
 * on_itemstore_row_changed:
 * @model:     Data model
 * @path:      Path in the tree
 * @iter:      Tree iter
 * @user_data: User data
 *
 * Called when the data in a row has changed. Will schedule a data save timeout
 * by calling glista_list_save_timeout()
 */
void 
on_itemstore_row_changed(GtkTreeModel *model, GtkTreePath *path, 
                         GtkTreeIter *iter, gpointer user_data)
{
	glista_item_redraw_parent(iter);
	glista_list_save_timeout();
}

/**
 * on_itemstore_row_inserted:
 * @model:     Data model
 * @path:      Path in the tree
 * @iter:      Tree iter
 * @user_data: User data
 *
 * Called when a new row is inserted to the model. Will schedule a data save 
 * timeout by calling glista_list_save_timeout()
 */
void 
on_itemstore_row_inserted(GtkTreeModel *model, GtkTreePath *path, 
                          GtkTreeIter *iter, gpointer user_data)
{
	glista_list_save_timeout();
}

/**
 * on_itemstore_row_deleted:
 * @model:     Data model
 * @path:      Path in the tree
 * @user_data: User data
 *
 * Called when a row in the model is deleted. Will schedule a data save timeout
 * by calling glista_list_save_timeout()
 */
void 
on_itemstore_row_deleted(GtkTreeModel *model, GtkTreePath *path, 
                         gpointer user_data)
{
	glista_list_save_timeout();
}

/**
 * glista_ui_mainwindow_store_geo:
 * @window The main window
 *
 * Save the current geometry (position and size) of the window in memory if the
 * window is visible
 */
static void
glista_ui_mainwindow_store_geo(GtkWindow *window)
{
	gboolean visible;
	gint     width, height;
	gint     xpos,  ypos;
	
	// Get current window state
	g_object_get(window, "visible", &visible, NULL);
	
	if (visible) {
		// Store size
		gtk_window_get_size(window, &width, &height);
		gl_globs->config->width  = width;
		gl_globs->config->height = height;
		
		// Store position
		gtk_window_get_position(window, &xpos, &ypos);
		gl_globs->config->xpos = xpos;
		gl_globs->config->ypos = ypos;
	}
}

/**
 * glista_ui_mainwindow_show:
 *
 * Show the main window, positioning and resizing it according to configuration
 * data
 */
void 
glista_ui_mainwindow_show()
{
	GtkWindow *window;
	GtkWidget *entry;
	
	window = GTK_WINDOW(glista_get_widget("glista_main_window"));
	
	gtk_window_present(window);
	gl_globs->config->visible = TRUE;
	
	/**
	 * TODO: Check window limits are not exceeded
	 */
	if (gl_globs->config->xpos > -1 && gl_globs->config->ypos > -1) {
		gtk_window_move(window, gl_globs->config->xpos, 
						gl_globs->config->ypos);
	}
	
	if (gl_globs->config->width > 0 && gl_globs->config->height > 0) {
		gtk_window_resize(window, gl_globs->config->width, 
						  gl_globs->config->height);
	}
	
	// Set the focus to the new item GtkEntry
	entry = GTK_WIDGET(glista_get_widget("add-entry"));
	gtk_widget_grab_focus (entry);
}

/**
 * glista_ui_mainwindow_hide:
 *
 * Hide the main window. Save the window position and geometry before hiding
 * it.
 */
void
glista_ui_mainwindow_hide()
{
	GtkWidget *window;
	
	window = GTK_WIDGET(glista_get_widget("glista_main_window"));
	glista_ui_mainwindow_store_geo(GTK_WINDOW(window));
	gtk_widget_hide(window);
	gl_globs->config->visible = FALSE;
}

/**
 * glista_ui_mainwindow_toggle:
 *
 * Toggle the visibility of the main window. This is normally called when the
 * user left-clicks the status icon in the system tray.
 */
void 
glista_ui_mainwindow_toggle()
{
	gboolean   current;
	GtkWidget *window;
	
	window = GTK_WIDGET(glista_get_widget("glista_main_window"));
	
	// Get current window state
	g_object_get(window, "visible", &current, NULL);
	
	// If hidden - show, if visible - hide
	if (current) {
		glista_ui_mainwindow_hide();
	} else {
		glista_ui_mainwindow_show();
	}
}

/**
 * glista_ui_init:
 * 
 * Initialize the Gtk+ UI. Called by main() at program startup.
 */
gboolean
glista_ui_init()
{
	GtkWidget      *window;
	GtkAboutDialog *about;
	GtkStatusIcon  *sysicon;
	
	gl_globs->uibuilder = gtk_builder_new();
	
	// Load UI file
	if (gtk_builder_add_from_file(gl_globs->uibuilder, 
								  GLISTA_DATA_DIR "/glista.ui", NULL) == 0) {
		g_printerr("Unable to read UI file: %s\n", 
		           GLISTA_DATA_DIR "/glista.ui");
		           
		return FALSE;
	}
	
	// Load main window and connect signals
	window = GTK_WIDGET(glista_get_widget("glista_main_window"));
	gtk_builder_connect_signals(gl_globs->uibuilder, NULL);

	if (gl_globs->trayicon) {
		// Set up the status icon and connect the left-click and right-click signals
		sysicon = gtk_status_icon_new_from_file(GLISTA_DATA_DIR "/glista-icon.png");
		g_signal_connect(sysicon, "activate", 
						 G_CALLBACK(on_sysicon_activate), window);
		g_signal_connect(sysicon, "popup-menu", 
						 G_CALLBACK(on_sysicon_popup_menu), NULL);
	}
		
	// Set the version number and icon in the about dialog
#ifdef PACKAGE_VERSION
	about = GTK_ABOUT_DIALOG(glista_get_widget("glista_about_dialog"));
	gtk_about_dialog_set_version(about, PACKAGE_VERSION);
#endif

#ifdef ENABLE_LINKIFY
	// Do we linkify URLs? 
	GtkTextView *textview = GTK_TEXT_VIEW(glista_get_widget("note_textview"));
	glista_textview_linkify_init(textview);
#endif
				 
	return TRUE;
}

/**
 * glista_ui_shutdown:
 * 
 * UI shutdown procedures. Called by main() just before the program exits.
 */
void 
glista_ui_shutdown()
{
	GtkWidget *window;
	
	window = GTK_WIDGET(glista_get_widget("glista_main_window"));
	glista_ui_mainwindow_store_geo(GTK_WINDOW(window));
}


/*****************************************************************************
 * Note: the following callback event handlers are connected automatically
 * through the UI file, and cannot be declared static.
 *****************************************************************************/


/**
 * on_sysicon_quit_activate:
 * @menuitem  Activating menu item
 * @user_data User data bound at connect time
 *
 * Handle the "quit" menu item selected. Simply quit the program.
 */
void 
on_sysicon_quit_activate(GtkMenuItem *menuitem, gpointer user_data)
{
	gtk_main_quit();
}

/**
 * on_glista_main_window_delete_event: 
 * @widget    The deleted widget (usually main window)
 * @event     The triggered event object
 * @user_data User data bound at connect time
 *
 * Handle a "delete-event" on the main window, that is the "X" to close the
 * window was pressed. Will hide the window and return TRUE to stop the window
 * from being destroyed.
 */
gboolean 
on_glista_main_window_delete_event(GtkWidget *widget, GdkEvent *event, 
								   gpointer user_data)
{
	if (gl_globs->trayicon) {
		glista_ui_mainwindow_hide();
	} else {
		gtk_main_quit();
	}
	
	return TRUE;
}

/**
 * on_add_button_clicked:
 * @object:    The object that triggered the event
 * @user_data: User data passed when the event was connected
 *
 * Triggered when the user clicks the "add" button. Reads the text from the
 * "new item" text entry and sends it off to glista_item_create_from_text().
 */
void 
on_add_button_clicked(GtkObject *object, gpointer user_data)
{
	GtkEntry    *entry;
	gchar       *text;
	
	entry = GTK_ENTRY(glista_get_widget("add-entry"));
	text  = (gchar *) gtk_entry_get_text(entry);
	glista_item_create_from_text(text);
	gtk_entry_set_text(entry, "");
}

/**
 * on_about_clicked:
 * @object:    The object that triggered the event
 * @user_data: User data passed when the event was connected
 *
 * Shows the "About" dialog. Can be called when the about item in the context
 * menu was selected, or when the about button in the toolbar was clicked.
 */
void 
on_about_clicked(GtkObject *object, gpointer user_data)
{
	GtkDialog *about;
	
	about = GTK_DIALOG(glista_get_widget("glista_about_dialog"));
	gtk_dialog_run(about);
	gtk_widget_hide(GTK_WIDGET(about));
}

/**
 * on_tb_clean_clicked:
 * @object:    The object that triggered the event
 * @user_data: User data passed when the event was connected
 *
 * Called when the "Clear" button in the toolbar is clicked. Simply calls 
 * glista_list_delete_done().
 */
void 
on_tb_clear_clicked(GtkObject *object, gpointer user_data)
{
	glista_list_delete_done();
}

/**
 * on_tb_delete_clicked:
 * @object:    The object that triggered the event
 * @user_data: User data passed when the event was connected
 *
 * Called when the toolbar "Delete" button is clicked. Will simply call
 * glista_list_delete_selected().
 */
void 
on_tb_delete_clicked(GtkObject *object, gpointer user_data)
{
	glista_list_delete_selected();
}

/**
 * on_tb_note_clicked:
 * @object:    The object that triggered the event
 * @user_data: User data passed when the event was connected
 *
 * Called when the "Note" button in the toolbar is clicked. Will toggle
 * the note panel for the currently selected item.
 */
void
on_tb_note_clicked(GtkObject *object, gpointer user_data)
{
	GtkTreeSelection *selection;
	
	selection = gtk_tree_view_get_selection(
		GTK_TREE_VIEW(glista_get_widget("glista_item_list")));
		
	glista_note_toggle_selected(selection);
}

/**
 * on_note_close_btn_clicked:
 * @btn       Button clicked
 * @user_data User data bound at signal connect time
 * 
 * Handle the note panel "close" button being clicked
 */
void
on_note_close_btn_clicked(GtkButton *btn, gpointer user_data)
{
	glista_note_close();
}

/**
 * on_list_row_activated:
 * @view      the tree view
 * @path      the path in the model
 * @column    the column clicked
 * @user_data data bound at connect time
 * 
 * Handle double-click on a row item, opening up the note panel
 */
void
on_list_row_activated(GtkTreeView *view, GtkTreePath *path, 
                      GtkTreeViewColumn *column, gpointer user_data)
{
	GtkTreeIter iter;
	
	if (gtk_tree_model_get_iter(GTK_TREE_MODEL(gl_globs->itemstore), 
	                            &iter, path)) {
	                            	
		glista_note_toggle(&iter);
	}
}
