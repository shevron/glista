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

#include "../glista.h"
#include "../glista-reminder.h"

/**
 * Glista reminder messaging module: dummy
 * 
 * This module is here as an example. As a real reminder module it's pretty
 * poor - it just prints out a messages to STDOUT.
 */

/**
 * glista_remindhandler_init:
 * @error A pointer to fill with an error, if any 
 * 
 * Initialize the remind handler module. 
 * In the case of the dummy handler, this does nothing important. 
 * 
 * Returns: TRUE on success, FALSE otherwise
 */
G_MODULE_EXPORT gboolean
glista_remindhandler_init(GError **error)
{
	g_printerr("Module initialized\n");
	return TRUE;
}

/**
 * glista_remindhandler_shutdown:
 * @error A pointer to fill with an error, if any 
 * 
 * Shut down the remind handler module.  
 * In the case of the dummy handler, this does nothing important. 
 * 
 * Returns: TRUE on success, FALSE otherwise
 */
G_MODULE_EXPORT gboolean
glista_remindhandler_shutdown(GError **error)
{
	g_printerr("Module shut down\n");
	return TRUE;
}

/**
 * glista_remindhandler_remind:
 * @reminder     The reminder to handle
 * @error        A pointer to fill with an error, if any
 * 
 * Remind the user about a task. The dummy reminder simply prints out a 
 * message to STDOUT.
 * 
 * Returns: TRUE on success, FALSE otherwise
 */
G_MODULE_EXPORT gboolean
glista_remindhandler_remind(GlistaReminder *reminder, 
                            GError **error)
{
	GtkTreePath *path;
	GtkTreeIter  iter;
	gchar       *item_text, *time_str;
	struct tm   *ltime;
	
	// Get reminder time into a string
	ltime = localtime(&(reminder->remind_at));
	time_str = g_malloc0(sizeof(gchar[72]));
	strftime(time_str, 70, "%x %H:%M", ltime);
	
	// Get the item name
	path = gtk_tree_row_reference_get_path(reminder->item_ref);
	gtk_tree_model_get_iter(GL_ITEMSTM, &iter, path);
	gtk_tree_model_get(GL_ITEMSTM, &iter, GL_COLUMN_TEXT, &item_text, -1);
	
	// Remind!
	g_print("It's %s - don't forget: %s\n", time_str, item_text);
	
	// Free used memory
	g_free(time_str);
	
	return TRUE;
}
