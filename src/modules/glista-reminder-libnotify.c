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
#include "../glista-plugin.h"
#include "../glista-reminder.h"

/**
 * Glista reminder messaging module: libnotify
 * 
 * Use libnotify to send reminders to the user. Requires dubs / notify 
 */

#define GLISTA_RH_NOTIFY_APPNAME "glista-reminders-notify"
#define GLISTA_RH_NOTIFY_TITLE   "Glista Task Reminder"
#define GLISTA_RH_NOTIFY_TIMEOUT 30000 // 30 seconds timeout
#define GLISTA_RH_NOTIFY_ICON    GLISTA_DATA_DIR \
                                 G_DIR_SEPARATOR_S \
                                 "glista48.png"

static void on_notification_close_clicked(NotifyNotification *nfication, 
                                          gchar *action, gpointer data);

// Declare this plugin
GLISTA_DECLARE_PLUGIN(
	GLISTA_PLUGIN_REMINDER, 
	"reminder-libnotify", 
	"libnotify popup"
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
    if (! notify_init(GLISTA_RH_NOTIFY_APPNAME)) {
        g_set_error(error, GLISTA_REMINDER_ERROR_QUARK, 
                           GLISTA_REMINDER_ERROR_INIT,
                           "Error initializing libnotify interface");
        return FALSE;
    }
    
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
    notify_uninit();
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
    GtkTreeIter         iter;
    GtkTreePath        *path;
    gchar              *item_text;
    NotifyNotification *nfication;
    gboolean            success = TRUE;
    GError             *notify_error = NULL;
    static GdkPixbuf   *icon_pb = NULL;
    
    // Make sure libnotify is initialized
    if (! notify_is_initted()) {
        g_set_error(error, GLISTA_REMINDER_ERROR_QUARK, 
                    GLISTA_REMINDER_ERROR_MESSAGE,
                    "libnotify interface is not initalized");
        return FALSE;
    }
    
    // Set up icon
    if (icon_pb == NULL) {
        icon_pb = gdk_pixbuf_new_from_file(GLISTA_RH_NOTIFY_ICON, NULL);
    }
        
    // Get the task as text
    // TODO: Concat the category ?
    path = gtk_tree_row_reference_get_path(reminder->item_ref);
    gtk_tree_model_get_iter(GL_ITEMSTM, &iter, path);
    gtk_tree_model_get(GL_ITEMSTM, &iter, GL_COLUMN_TEXT, &item_text, -1);
    
    // Create reminder
    // TODO: Attach to systray icon?
    nfication = notify_notification_new(GLISTA_RH_NOTIFY_TITLE, item_text, 
                                        NULL, NULL);
                                        
    notify_notification_set_timeout(nfication, GLISTA_RH_NOTIFY_TIMEOUT);
    notify_notification_add_action(nfication, "close", "Dismiss", 
    	(NotifyActionCallback) on_notification_close_clicked, NULL, NULL);
    
    // Set icon
    if (icon_pb != NULL) {
        notify_notification_set_icon_from_pixbuf(nfication, icon_pb);
    }
    
    // Attach to tray icon, if we have one
    if (gl_globs->trayicon != NULL) {
    	notify_notification_attach_to_status_icon(nfication, 
    		gl_globs->trayicon);
	}
    
    // Show notification
    if (! notify_notification_show(nfication, &notify_error)) {
        g_set_error(error, GLISTA_REMINDER_ERROR_QUARK, 
                    GLISTA_REMINDER_ERROR_MESSAGE,
                    "error sending message using libnotify: %s",
                    notify_error->message);
                    
        g_error_free(notify_error);
        success = FALSE;
    }
    
    g_object_unref(nfication);

    return success;
}

/**
 * on_notification_close_clicked:
 * @nfication Notification object
 * @action    The action that was executed (usualy "close")
 * @data      Any data bound at bind time
 * 
 * Close the notification after the "Dismiss" button was clicked
 */
static void
on_notification_close_clicked(NotifyNotification *nfication, 
                              gchar *action, gpointer data) 
{
	notify_notification_close(nfication, NULL);	
}
                              	
