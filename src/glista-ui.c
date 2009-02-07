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

#define _XOPEN_SOURCE /* glibc2 needs this */
#include <time.h>
 
#include <glib/gi18n.h>
#include <gtk/gtk.h>
#include <string.h>
#include "glista.h"
#include "glista-ui.h"
#include "glista-reminder.h"
#include "glista-plugin.h"

#ifdef ENABLE_LINKIFY
#include "glista-textview-linkify.h"
#endif

/**
 * Columns enum for plugin dropdown GtkListStore
 */
enum {
	GL_PL_COL_NAME,
	GL_PL_COL_PATH,
	GL_PL_NUM_COLS
};

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
	GtkWidget   *clear_btn, *note_btn, *reminder_btn;
	GtkTreeIter *iter;

	clear_btn    = GTK_WIDGET(glista_get_widget("tb_delete"));
	note_btn     = GTK_WIDGET(glista_get_widget("tb_note"));
	reminder_btn = GTK_WIDGET(glista_get_widget("tb_reminder"));
	
	if (gtk_tree_selection_count_selected_rows(selection) > 0) {
		if ((iter = glista_item_get_single_selected(selection)) != NULL) {
			gtk_widget_set_sensitive(note_btn, TRUE);
			glista_note_open_if_visible(iter);
			gtk_tree_iter_free(iter);
			
		} else {
			gtk_widget_set_sensitive(note_btn, FALSE);
			glista_note_close();
		}
		
		gtk_widget_set_sensitive(reminder_btn, TRUE);
		gtk_widget_set_sensitive(clear_btn, TRUE);
		
	} else {
		gtk_widget_set_sensitive(reminder_btn, FALSE);
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
			gtk_tree_model_get_iter(GL_ITEMSTM, &iter, path)) {
			gtk_tree_model_get(GL_ITEMSTM, &iter, GL_COLUMN_TEXT, &text, -1);
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
 * @use_trayicon Whether we are using a tray icon or not
 * 
 * Initialize the Gtk+ UI. Called by main() at program startup.
 */
gboolean
glista_ui_init(gboolean use_trayicon)
{
	GtkWidget      *window;
	GtkAboutDialog *about;
	GtkIconFactory *iconfactory;
	
	gl_globs->uibuilder = gtk_builder_new();
	
	// Load icons file and initialize icon factory
	if (gtk_builder_add_from_file(gl_globs->uibuilder, 
			GLISTA_DATA_DIR "/glista-icons.xml", NULL) == 0) {
		
		g_printerr(_("Unable to read icon data file: %s\n"), 
		           GLISTA_DATA_DIR "/glista-icons.xml");
		           
		return FALSE;
	}
	iconfactory = (GtkIconFactory *) glista_get_widget("glista-iconfactory");
	gtk_icon_factory_add_default(iconfactory);
	
	// Load UI file
	if (gtk_builder_add_from_file(gl_globs->uibuilder, 
			GLISTA_DATA_DIR "/glista.ui", NULL) == 0) {
		
		g_printerr(_("Unable to read UI file: %s\n"), 
		           GLISTA_DATA_DIR "/glista.ui");
		           
		return FALSE;
	}
	
	// Load main window and connect signals
	window = GTK_WIDGET(glista_get_widget("glista_main_window"));
	gtk_builder_connect_signals(gl_globs->uibuilder, NULL);

	if (use_trayicon) {
		// Set up the status icon and connect the left-click and right-click signals
		gl_globs->trayicon = gtk_status_icon_new_from_stock("glista-icon");
		g_signal_connect(gl_globs->trayicon, "activate", 
						 G_CALLBACK(on_sysicon_activate), window);
		g_signal_connect(gl_globs->trayicon, "popup-menu", 
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

/**
 * glista_ui_remwindow_show:
 * 
 * Show the "set reminder" window. Will set the date / time fields to 
 * show the current time + 1 hour and show the window.
 */
static void 
glista_ui_remwindow_show()
{
	GtkWidget     *rem_window;
	GtkSpinButton *hr_in, *min_in;
	GtkEntry      *date_in; 
	GtkCalendar   *cal;
	time_t         now_t;
	struct tm      now_tm;
	char           date_str[64];
	
	rem_window = GTK_WIDGET(glista_get_widget("set_reminder_window"));
	date_in    = GTK_ENTRY(glista_get_widget("reminder_date"));
	hr_in      = GTK_SPIN_BUTTON(glista_get_widget("reminder_hour"));
	min_in     = GTK_SPIN_BUTTON(glista_get_widget("reminder_min"));
	
	// Get the current time + one hour
	time(&now_t);
	now_t = now_t + 3600;
	localtime_r(&now_t, &now_tm);
	
	// Set the date in the calendar (still hidden)
	cal = GTK_CALENDAR(glista_get_widget("reminder_cal"));
	gtk_calendar_select_day(cal, (guint) now_tm.tm_mday);
	gtk_calendar_select_month(cal, (guint) now_tm.tm_mon, 
	                               (guint) now_tm.tm_year + 1900);
	
	// Set the same date / time in the text field
	strftime(date_str, sizeof(date_str), "%x", &now_tm);
	gtk_entry_set_text(date_in, (gchar *) &date_str);
	gtk_spin_button_set_value(hr_in, (gdouble) now_tm.tm_hour);
	gtk_spin_button_set_value(min_in, (gdouble) now_tm.tm_min);
	
	// Show the window
	gtk_widget_show(rem_window);
}

/**
 * glista_ui_remwindow_hide:
 * 
 * Hide the "set reminder" window. This does not grab any data from the
 * window - only hides it. 
 */
static void 
glista_ui_remwindow_hide()
{
	GtkWidget       *rem_window;
	GtkToggleButton *cal_btn;
	
	// Hide the calendar
	cal_btn = GTK_TOGGLE_BUTTON(glista_get_widget("reminder_cal_btn"));
	gtk_toggle_button_set_active(cal_btn, FALSE);
	
	// Hide the window
	rem_window = GTK_WIDGET(glista_get_widget("set_reminder_window"));
	gtk_widget_hide(rem_window);
}

/**
 * glista_ui_show_item_menu:
 * @treeview The GtkTreeView containing the items
 * @event    Event information
 * 
 * Will display the item context menu - hooked to the right-click event on an
 * item. 
 */
static void
glista_ui_show_item_menu(GtkTreeView *treeview, GdkEventButton *event)
{
	GtkTreeSelection *selection;
	GtkMenu *menu;
	
	selection = gtk_tree_view_get_selection(treeview);
	if (gtk_tree_selection_count_selected_rows(selection) > 0) {
		
		// Show menu
		menu = GTK_MENU(glista_get_widget("item_cmenu"));
		gtk_widget_show_all(GTK_WIDGET(menu));
		gtk_menu_popup(menu, NULL, NULL, NULL, NULL, 
		               (event != NULL) ? event->button : 0,
					   gdk_event_get_time((GdkEvent *) event));
	}				   
}

/**
 * glista_ui_prefswindow_load_pluginlist:
 * @combo   The target GtkComboBox
 * @plugins Linked list of plugins
 * 
 * Load a list of plugins into a combo box, optionally initializing the combo
 * box model and cell renderer. 
 * 
 * Will free each plugin in the list that was moved into the model. Will not 
 * free the list itself (even if empty).
 */
static void
glista_ui_prefswindow_load_pluginlist(GtkComboBox *combo, GList *plugins)
{
	GList        *node;
	GlistaPlugin *plugin;
	GtkListStore *pluginstore;
	GtkTreeIter   iter;
	
	// Do we already have a GtkListStore attached to this box?
	pluginstore = GTK_LIST_STORE(gtk_combo_box_get_model(combo));
	if (pluginstore == NULL) {
		GtkCellRenderer *cell;
		
		// Load all plugins into a GtkListStore
		pluginstore = gtk_list_store_new(GL_PL_NUM_COLS, 
	                                     G_TYPE_STRING,  // Display Name 
	                                     G_TYPE_STRING); // Plugin path

		// Set up the combobox cell renderrer
		cell = gtk_cell_renderer_text_new();
		gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(combo), cell, TRUE);
		gtk_cell_layout_set_attributes(GTK_CELL_LAYOUT(combo), cell, 
		                               "text", 0, 
									   NULL);
		
		// Attach the model to the ComboBox
		gtk_combo_box_set_model(combo, GTK_TREE_MODEL(pluginstore));
		
	} else {
		// Clear out current list
		gtk_list_store_clear(pluginstore);
	}
	
	// Load current plugin list into the combo box model
	for (node = plugins; node != NULL; node = node->next) {
		plugin = (GlistaPlugin *) node->data;
		gtk_list_store_append(pluginstore, &iter);
		gtk_list_store_set(pluginstore, &iter, 
	    	               GL_PL_COL_NAME, plugin->display_name,
						   GL_PL_COL_PATH, plugin->module_path,
						   -1);

		glista_plugin_free(node->data);
	}
}

static void
glista_ui_prefswindow_show()
{
	GList        *plugins;
	GtkWidget    *prefs_win;
	GtkComboBox  *rem_plugins_box;
	
	prefs_win = GTK_WIDGET(glista_get_widget("glista_prefs_window"));
	plugins = glista_plugin_query_plugins(GLISTA_PLUGIN_ALL);
	
	// Load the plugin list into the reminder plugin combobox
	rem_plugins_box = GTK_COMBO_BOX(glista_get_widget("reminder_plugin_combo"));
	glista_ui_prefswindow_load_pluginlist(rem_plugins_box, plugins);
	g_list_free(plugins);
	
	gtk_widget_show_all(prefs_win);
}

/**
 * glista_ui_prefswindow_hide:
 * 
 * Hide the "preferences" window. This does not grab any data from the
 * window - only hides it. 
 */
static void
glista_ui_prefswindow_hide()
{
	GtkWidget *window;
	
	// Hide the window
	window = GTK_WIDGET(glista_get_widget("glista_prefs_window"));
	gtk_widget_hide(window);
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
	if (gl_globs->trayicon != NULL) {
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
 * on_tb_prefs_clicked:
 * @object:    The object that triggered the event
 * @user_data: User data passed when the event was connected
 *
 * Called when the "Preferences" button in the toolbar is clicked. Will open
 * the preferences window.
 */
void
on_tb_prefs_clicked(GtkObject *object, gpointer user_data)
{
	glista_ui_prefswindow_show();
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
	
	if (gtk_tree_model_get_iter(GL_ITEMSTM, &iter, path)) {
		glista_note_toggle(&iter);
	}
}

/**
 * on_list_query_tooltip:
 * @widget     The widget activating the event
 * @x          X mouse position
 * @y          Y mouse position
 * @keyboard   Was this a keyboard tooltip event?
 * @tooltip    Tooltip object
 * @user_data  Data bound at signal connect time
 * 
 * Called when it's time to show a tooltip on the item list. Will check on what
 * column / row the mouse is, and will act accordingly - showing the item text
 * if we are on the text column, or showing reminder time if we are on the 
 * reminder indicator column. Will not show tooltips on other columns. 
 * 
 * Returns: TRUE if widget is to be displayed, FALSE otherwise. 
 */
gboolean 
on_list_query_tooltip(GtkWidget *widget, gint x, gint y, gboolean keyboard, 
                      GtkTooltip *tooltip, gpointer user_data)
{
	GtkTreePath       *path;
	GtkTreeViewColumn *column;
	GtkTreeIter        iter;
	gint               bin_x, bin_y, cell_x, cell_y, col_id;
	gboolean           ret = FALSE;
	gchar             *text;
	GlistaReminder    *reminder;
	
	if (! GTK_IS_TREE_VIEW(widget)) {
		return FALSE;
	}
	
	gtk_tree_view_convert_widget_to_bin_window_coords(GTK_TREE_VIEW(widget),
													  x, y, &bin_x, &bin_y);
	if (gtk_tree_view_get_path_at_pos(GTK_TREE_VIEW(widget), bin_x, bin_y, 
									  &path, &column, &cell_x, &cell_y)) {
		
		if (column != NULL && path != NULL) {
			// Find out what column we are over
			col_id = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(column), 
													   "col-id"));
			switch(col_id) {
				case GL_COLUMN_TEXT: // Tooltip is task text
					gtk_tree_model_get_iter(
						GTK_TREE_MODEL(gl_globs->itemstore), &iter, path);	
					gtk_tree_model_get(GTK_TREE_MODEL(gl_globs->itemstore),
									   &iter, GL_COLUMN_TEXT, &text, -1);
									   
					if (text != NULL) {
						gtk_tooltip_set_text(tooltip, text);
						gtk_tree_view_set_tooltip_cell(GTK_TREE_VIEW(widget), 
						                               tooltip, path, 
													   column, NULL);
						ret = TRUE;
					}
					
					break;
					
				case GL_COLUMN_REMINDER: // Tooltip is reminder time
					gtk_tree_model_get_iter(
						GTK_TREE_MODEL(gl_globs->itemstore), &iter, path);	
					gtk_tree_model_get(GTK_TREE_MODEL(gl_globs->itemstore),
									   &iter, GL_COLUMN_REMINDER, 
									   (gpointer) &reminder, -1);
									   
					if (reminder != NULL) {
						text = glista_reminder_time_as_string(reminder, 
							"Remind at ");
							
						gtk_tooltip_set_text(tooltip, text);
						gtk_tree_view_set_tooltip_cell(GTK_TREE_VIEW(widget), 
												   tooltip, path, column, 
												   NULL);			   
						g_free(text);
						ret = TRUE;
					}
					
					break;
			}
		}
		
		if (path != NULL)
			gtk_tree_path_free(path);
	}

	return ret;
}

/**
 * on_tb_add_reminder_clicked:
 * @object    The GtkObject that triggered the event
 * @user_data Any data bound at signal connect time
 * 
 * Handle the clicking of the "Set Reminder" toolbar button. Simply
 * shows the reminder window.
 */
void
on_tb_add_reminder_clicked(GtkObject *object, gpointer user_data)
{
	glista_ui_remwindow_show();
}

/**
 * on_reminder_window_closed:
 * @object    The GtkObject that triggered the event
 * @user_data Any data bound at signal connect time
 * 
 * Handle the closing of the reminder window - this can be done by 
 * either clicking "Cancel" or the "X" buttons. Will simply hide the 
 * window without doing anything with the data. 
 */
void 
on_reminder_window_closed(GtkObject *object, gpointer user_data)
{
	glista_ui_remwindow_hide();
}

/**
 * on_reminder_cal_btn_toggled:
 * @togglebtn The toggle button clicked 
 * @user_data Any data bound at signal connect time
 * 
 * Handle clicking of the toggle button to show the calendar widget. 
 * Might either show or hide the calendar. 
 */
void
on_reminder_cal_btn_toggled(GtkToggleButton *togglebtn, 
                            gpointer user_data)
{
	GtkCalendar *cal;
	
	cal = GTK_CALENDAR(glista_get_widget("reminder_cal"));
	
	if (gtk_toggle_button_get_active(togglebtn)) {
		// Show calendar
		gtk_widget_show(GTK_WIDGET(cal));		
	} else {
		// Hide calendar
		gtk_widget_hide(GTK_WIDGET(cal));
	}
}

/**
 * on_reminder_time_output:
 * @spin_button  The modified GtkSpinButton
 * @user_data    User data attached at signal connect time
 * 
 * Manipulate the display of the minute / hour spin buttons to show
 * zero-padded hours / minutes.
 * 
 * Code copied from the GtkSpinButton docs (see 'output' signal docs)
 */
gboolean 
on_reminder_time_output(GtkSpinButton *spin_button, gpointer user_data)
{
	GtkAdjustment *adj;
	gchar *text;
	int value;
	
	adj = gtk_spin_button_get_adjustment(spin_button);
	value = (int) gtk_adjustment_get_value(adj);
	text = g_strdup_printf("%02d", value);
	gtk_entry_set_text (GTK_ENTRY(spin_button), text);
	g_free (text);
	
	return TRUE;
}

/**
 * on_reminder_cal_day_selected:
 * @cal      The modified GtkCalendar
 * @gpointer User data attached at signal connect time
 * 
 * Handle date selection in the calendar - set the same date in the 
 * date text entry
 */
void
on_reminder_cal_day_selected(GtkCalendar *cal, gpointer user_data) 
{
	guint     year, month, day;
	GtkEntry *date_entry; 
	GDate    *date;
	char      date_str[64];
	
	gtk_calendar_get_date(cal, &year, &month, &day);
	month = month + 1;
	
	date = g_date_new_dmy(day, month, year);
	if (g_date_valid_dmy(day, month, year)) {
		g_date_strftime(date_str, sizeof(date_str), "%x", date);
		
		date_entry = GTK_ENTRY(glista_get_widget("reminder_date"));
		gtk_entry_set_text(date_entry, (gchar *) &date_str);
	}
	
	g_date_free(date);
}

/**
 * on_reminder_add_btn_clicked:
 * @object    The GtkObject that triggered the event
 * @user_data Any data bound at signal connect time
 * 
 * Handle the reminder window "Add" button clicked. This will add a new
 * reminder and close the window.
 */
void
on_reminder_add_btn_clicked(GtkObject *object, gpointer user_data)
{
	GtkEntry  *date_in, *hour_in, *min_in;
	gchar     *time_str;
	struct tm  time_tm;
	time_t     now, remind_at;
	
	date_in = GTK_ENTRY(glista_get_widget("reminder_date"));
	hour_in = GTK_ENTRY(glista_get_widget("reminder_hour"));
	min_in  = GTK_ENTRY(glista_get_widget("reminder_min"));
	
	time_str = g_malloc0(sizeof(gchar[71]));
	
	// Convert the input into timestamp format
	g_snprintf(time_str, 70, "%s %s:%s:00", 
		gtk_entry_get_text(date_in), 
		gtk_entry_get_text(hour_in), 
		gtk_entry_get_text(min_in));
	
	// Init our time structure to current local time
	time(&now);
	localtime_r(&now, &time_tm);
	
	// Set the values in time struct as specified by user and create timestamp
	strptime((char *) time_str, "%x %T", &time_tm);
	remind_at = mktime(&time_tm);
	g_free(time_str);
	
	glista_reminder_set_on_selected(remind_at);
	
	glista_ui_remwindow_hide();
}

/**
 * on_reminder_date_changed:
 * @date_in   The modified date GtkEntry
 * @user_data User data bound at signal connect time
 * 
 * Called when the set reminder dialog's date entry is changed
 */
void
on_reminder_date_changed(GtkEntry *date_in, gpointer user_data)
{
	struct tm  date;
	GtkWidget *ok_btn; 
	
	const gchar *date_str = gtk_entry_get_text(date_in);
	
	ok_btn = GTK_WIDGET(glista_get_widget("reminder_add_btn"));
	
	// Validate the date
	if ((strptime((gchar *) date_str, "%x", &date)) == NULL) {
		gtk_widget_set_sensitive(ok_btn, FALSE);
	} else {
		gtk_widget_set_sensitive(ok_btn, TRUE);
	}
}

/**
 * on_list_button_press:
 * @widget    The widget that triggered the event
 * @event     The event information 
 * @user_data Any user data bound at signal connect time
 * 
 * Called on any mouse button press on the item list. Currently, will only
 * handle right-mouse button clicks by opening the context menu (if applicable). 
 * Other clicks are not captured and events passed further to other possibly 
 * connected signal handlers.
 */
gboolean
on_list_button_press(GtkWidget *widget, GdkEventButton *event, 
                     gpointer user_data)
{
	GtkTreeSelection *selection;
	GtkTreePath      *path;
	
	if (event->type == GDK_BUTTON_PRESS && event->button == 3) {
		selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(widget));
		
		// Select the row under the mouse
		if (gtk_tree_selection_count_selected_rows(selection) <= 1) {
			
			if (gtk_tree_view_get_path_at_pos(GTK_TREE_VIEW(widget), 
			                                  (gint) event->x, (gint) event->y, 
					    					  &path, NULL, NULL, NULL)) {
				gtk_tree_selection_unselect_all(selection);
				gtk_tree_selection_select_path(selection, path);
				gtk_tree_path_free(path);
			}
		}
		
		glista_ui_show_item_menu(GTK_TREE_VIEW(widget), event);
		return TRUE;
	}
	
	return FALSE;
}

/**
 * on_list_popup_menu:
 * @widget     The widget triggering the event
 * @user_data  Any data bound at signal connect time
 * 
 * Handle the popup-menu signal on the item list. This signal is usually 
 * triggered when the user used the keyboard to open the popup menu (e.g. using
 * Shift + F12). It is treated similarily to the button-press event when the 
 * right mouse button was pressed. 
 */
gboolean 
on_list_popup_menu(GtkWidget *widget, gpointer user_data)
{
	glista_ui_show_item_menu(GTK_TREE_VIEW(widget), NULL);
	return TRUE;
}

/**
 * on_icmenu_note_clear_activate:
 * @item       The selected item
 * @user_data  Any data bound at signal connect time
 * 
 * Handle the "Clear Note" item context menu option
 */
void
on_icmenu_note_clear_activate(GtkMenuItem *item, gpointer user_data)
{
	glista_note_clear_selected();
}

/**
 * on_icmenu_reminder_clear_activate:
 * @item       The selected item
 * @user_data  Any data bound at signal connect time
 * 
 * Handle the "Clear Reminder" item context menu option
 */
void
on_icmenu_reminder_clear_activate(GtkMenuItem *item, gpointer user_data)
{
	glista_reminder_remove_selected();
}

/**
 * on_prefs_window_close:
 * @object    The GtkObject that triggered the event
 * @user_data Any data bound at signal connect time
 * 
 * Handle the closing of the preferences window - this can be done by 
 * either clicking "Close" or the "X" buttons. Will simply hide the 
 * window without doing anything with the data. 
 */
void 
on_prefs_window_closed(GtkObject *object, gpointer user_data)
{
	glista_ui_prefswindow_hide();
}
