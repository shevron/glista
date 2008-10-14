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
#include "glista.h"

/**
 * Glista - GTK UI Callbacks
 *
 * The functions in this file should have no functionality beyond handling the 
 * GTK user interface and passing events on to the related glista functions 
 * defined elsewhere.
 */

/**
 * on_glista_main_window_destroy:
 * @object:    The object that triggered the event
 * @user_data: User data passed when the event was connected
 *
 * Quit the program
 *
 * TODO: Rewrite this. The program no longer exists when the main window is 
 * closed, and this function is only called when the user selects "quit" in the
 * status icon context menu.
 */
void 
on_glista_main_window_destroy(GtkObject *object, gpointer user_data)
{
	gtk_main_quit();
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
 * on_glista_item_list_selection_changed:
 * @selection: The tree's GtkTreeSelection object
 * @user_data: User data passed when the event was connected
 *
 * Called when the selection in the list was changed. The purpose is to know 
 * whether some buttons in need to be disabled when no items are selected.
 */
void 
on_glista_item_list_selection_changed(GtkTreeSelection *selection, 
                                      gpointer user_data)
{
	GtkWidget *clear_btn;

	clear_btn = GTK_WIDGET(glista_get_widget("tb_delete"));
	if (gtk_tree_selection_count_selected_rows(selection) > 0) {
		gtk_widget_set_sensitive(clear_btn, TRUE);
	} else {
		gtk_widget_set_sensitive(clear_btn, FALSE);
	}
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
 * on_sysicon_activate:
 * @object:    The object that triggered the event
 * @user_data: User data passed when the event was connected
 *
 * Called when the system tray status icon is clicked. Simply calls
 * glista_ui_mainwindow_toggle().
 */
void 
on_sysicon_activate(GtkObject *object, GtkWindow *window)
{
	glista_ui_mainwindow_toggle();
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
void 
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
