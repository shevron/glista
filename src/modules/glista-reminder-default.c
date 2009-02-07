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
#include <gmodule.h>
#include <gtk/gtk.h>
#include <libnotify/notify.h>

#include "../glista.h"
#include "../glista-ui.h"
#include "../glista-plugin.h"
#include "../glista-reminder.h"

/**
 * Glista reminder messaging module: default
 * 
 * Use a GtkMessageDialog message which should be always available
 */

#define GLISTA_RH_DEFAULT_ICON GLISTA_DATA_DIR \
                               G_DIR_SEPARATOR_S \
                               "glista48.png"

// Declare this plugin
GLISTA_DECLARE_PLUGIN(
	GLISTA_PLUGIN_REMINDER, 
	"reminder-default", 
	_("Gtk+ popup message dialog")
);

/**
 * glista_remindhandler_init:
 * @error A pointer to fill with an error, if any 
 * 
 * Initialize the remind handler module. 
 *
 * Returns: TRUE on success, FALSE otherwise
 */
G_MODULE_EXPORT gboolean
glista_remindhandler_init(GError **error)
{
    return TRUE;
}

/**
 * glista_remindhandler_shutdown:
 * @error A pointer to fill with an error, if any 
 * 
 * Shut down the remind handler module. 
 * 
 * Returns: TRUE on success, FALSE otherwise 
 */
G_MODULE_EXPORT gboolean
glista_remindhandler_shutdown(GError **error)
{
    return TRUE;
}

/**
 * glista_remindhandler_remind:
 * @reminder     The reminder to handle
 * @error        A pointer to fill with an error, if any
 * 
 * Remind the user about a task. 
 * 
 * Returns: TRUE on success, FALSE otherwise
 */
G_MODULE_EXPORT gboolean
glista_remindhandler_remind(GlistaReminder *reminder, 
                            GError **error)
{
    GtkWidget        *dialog;
    GtkWindow        *mainwindow;
    GtkTreeIter       iter;
    GtkTreePath      *path;
    gchar            *item_text;
    
    mainwindow = GTK_WINDOW(glista_get_widget("glista_main_window"));
    
    // Get the task as text
    // TODO: Concat the category ?
    path = gtk_tree_row_reference_get_path(reminder->item_ref);
    gtk_tree_model_get_iter(GL_ITEMSTM, &iter, path);
    gtk_tree_model_get(GL_ITEMSTM, &iter, GL_COLUMN_TEXT, &item_text, -1);
    
    dialog = gtk_message_dialog_new(mainwindow, 
                                    GTK_DIALOG_DESTROY_WITH_PARENT, 
                                    GTK_MESSAGE_INFO,
                                    GTK_BUTTONS_CLOSE,
                                    _("Glista Reminder"));
                                    
    gtk_message_dialog_format_secondary_text(GTK_MESSAGE_DIALOG(dialog), 
                                             "%s", item_text);
    
    g_signal_connect_swapped(dialog, "response", 
                             G_CALLBACK(gtk_widget_destroy), dialog);

    gtk_widget_show(dialog);
    
    return TRUE;
}
