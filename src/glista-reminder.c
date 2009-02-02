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
#include <glib/gprintf.h>
#include <gmodule.h>
#include <gtk/gtk.h>

#include "glista.h"
#include "glista-reminder.h"

/**
 * List of pending reminders
 */
static GList *reminders      = NULL;

/**
 * ID of GSource function that periodically checks for reminders due
 */
static guint  rem_timeout_id = 0;

/**
 * The reminder module and function. Loaded dynamically depending on 
 * the selected reminder messaging module.
 */
static GModule            *remind_module = NULL;
static GlistaRHRemindFunc  remind_func   = NULL;

/**
 * glista_reminder_module_path:
 * @mod_name Name of the module
 * 
 * Build the path to the module in a system-independent manner. We do
 * not use g_module_build_path() because on Linux it adds the 'lib' 
 * prefix to everything. 
 * 
 * Return: a newly allocated string with the path. Must be freed later.
 */
static gchar*
glista_reminder_module_path(const gchar *mod_name)
{
	gchar *mod_path; 
	
	mod_path = g_strdup_printf("%s%sglistareminder-%s.%s", 
		GLISTA_LIB_DIR, G_DIR_SEPARATOR_S, mod_name, G_MODULE_SUFFIX);
	
	return mod_path;
}

/**
 * glista_reminder_init:
 * @mod_name Name of the selected reminder messaging module
 * 
 * Initialize the Glista reminder messaging module. Will open the 
 * selected module. Will also call the module's init() function if such
 * function is defined.
 * 
 * Return: TRUE on success, FALSE on failure. 
 */
static gboolean
glista_reminder_init(gchar *mod_name)
{
	gchar           *mod_path;
	GlistaRHInitFuc  init_func;
	GError          *init_error = NULL;
	
	glista_reminder_shutdown();
	
	mod_path = glista_reminder_module_path(mod_name);
	remind_module = g_module_open(mod_path, G_MODULE_BIND_LAZY);
	g_free(mod_path);
	
	if (remind_module == NULL) {
		g_critical("Unable to load reminder module %s, %s", mod_name, 
			g_module_error());
		return FALSE;
	}
	
	// Call the module's init function if it is implemented
	if (g_module_symbol(remind_module, "glista_remindhandler_init", 
		(gpointer *) &init_func)) {
	
		// Initialize module
		
		if (! init_func(&init_error)) {
			// Error initializing module
			if (init_error != NULL) {
				g_critical("Error initializing remind handler module: %s", 
			    	       init_error->message);
			    	       
			    g_error_free(init_error);
			}
			
			glista_reminder_shutdown();
			return FALSE;
		}
	}
	
	if (! g_module_symbol(remind_module, "glista_remindhandler_remind", 
		(gpointer *) &remind_func)) {
		
		g_critical("Can't find reminder function symbol in %s: %s", 
			mod_name, g_module_error());
		glista_reminder_shutdown();
		return FALSE;
	}
	
	if (remind_func == NULL) {
		g_critical("Reminder function symbol for %s is NULL, %s", 
			mod_name, g_module_error());
		glista_reminder_shutdown();
		return FALSE;
	}
	
	return TRUE;
}

/**
 * glista_reminder_shutdown:
 * 
 * Close the Glista reminder messaging module. Should be called before
 * swithing to a different module and before the program quits. 
 * 
 * Will internally call the remind handler module's shutdown function if
 * such function is defined.
 */
void 
glista_reminder_shutdown()
{
	GlistaRHShutdownFunc  shutdown_func;
	GError               *shutdown_err = NULL;
	
	if (remind_module != NULL) {
		// Call the module's shutdown function
		if (g_module_symbol(remind_module, "glista_remindhandler_shutdown", 
			(gpointer *) &shutdown_func)) {
		
			// Shut down module
			if (! shutdown_func(&shutdown_err)) {
				// Error shutting down module
				if (shutdown_err != NULL) {
					g_critical("Error shutting down remind handler module: %s", 
						shutdown_err->message);
			    	       
			    	g_error_free(shutdown_err);
				}
			}
		}
		
		// Close module
		if (! g_module_close(remind_module)) {
			g_warning("Unable to properly close reminder module: %s",
				g_module_error());
		}
		
		remind_module = NULL;
	}
}

/**
 * glista_reminder_insert_compare_func:
 * @a First item to compare
 * @b Second item to compare
 * 
 * A comparison function to be used when inserting new reminders to the list of
 * reminders. The goal is to have the reminders ordered by time, so that only 
 * the first reminder needs to be checked for due time - instead of iterating 
 * over the entire list. 
 * 
 * This is called internally by g_list_insert_sorted()
 * 
 * Returns: 1 if a > b, -1 if a < b and 0 if they are equal
 */
static gint 
glista_reminder_insert_compare_func(gconstpointer a, gconstpointer b)
{
	GlistaReminder *a_rem, *b_rem;
	
	a_rem = (GlistaReminder *) a;
	b_rem = (GlistaReminder *) b;
	
	if (a_rem->remind_at > b_rem->remind_at) {
		return 1;
	} else if (a_rem->remind_at < b_rem->remind_at) {
		return -1;
	} else {
		return 0;
	}
}

/**
 * glista_reminder_new:
 * @ref  A reference pointing to the item in the model
 * @time Reminder time
 * 
 * Initialize a new reminder object
 */
static GlistaReminder*
glista_reminder_new(GtkTreeRowReference *ref, time_t time)
{
	GlistaReminder      *reminder;
	GtkTreeRowReference *ref_copy;
	
	ref_copy = gtk_tree_row_reference_copy(ref);
	
	reminder = g_malloc(sizeof(GlistaReminder));
	reminder->item_ref  = ref_copy;
	reminder->remind_at = time;	
	
	return reminder;
}
 
/**
 * glista_reminder_free:
 * @reminder The reminder object to free
 * 
 * Free a reminder object
 */
void
glista_reminder_free(GlistaReminder *reminder)
{
	gtk_tree_row_reference_free(reminder->item_ref);
	g_free(reminder);
}

/**
 * glista_reminder_error_quark:
 * 
 * Get the error domain GQuark for glista reminders functionality. 
 * You should use the GLISTA_REMINDER_ERROR_QUARK constant and not this 
 * function directly
 * 
 * Returns: the error quark for this module
 */
GQuark
glista_reminder_error_quark()
{
	static GQuark equark = 0;
	
	if (equark == 0) { 
		equark = g_quark_from_static_string(GLISTA_REMINDER_ERROR_QUARK_S);
	}
	
	return equark;
}

/**
 * glista_reminder_call_mod_remind:
 * @reminder The reminder object to handle
 * 
 * Call the reminder module function which is supposed to do the 
 * actual "reminding". 
 */
static void
glista_reminder_call_reminder_func(GlistaReminder *reminder)
{
	GError *error = NULL;
	
	if (remind_func == NULL) {
		return;
	}
	
	// Call the remind handler module remind function
	if (! remind_func(reminder, &error)) {
		// There was some error
		if (error != NULL) {
			g_warning("Error calling remind handler: %s", error->message);
			g_error_free(error);
		}
	}
}

/**
 * glista_reminder_check_reminders:
 * @data data passed at scheduling time
 * 
 * Periodically called to check if any reminders are due - and if so, will 
 * call glista_reminder_remind() for them and then remove them from the list. 
 * 
 * This function is executed in intervals using the g_timeout mechanism.
 * 
 * If the list of reminders is empty, the timeout will be cleared until new 
 * reminders are added. 
 */
static gboolean 
glista_reminder_check_reminders(gpointer data)
{
	GList          *rem_node;
	GlistaReminder *reminder;
	time_t          now;
	
	while (reminders != NULL) {
		// Find the current time and compare to first reminder
		time(&now);
		reminder = (GlistaReminder *) reminders->data;
		
		if (now >= reminder->remind_at) {	
			GtkTreePath *path;
			GtkTreeIter  iter;
			gboolean     is_done;
			
			if ((path = gtk_tree_row_reference_get_path(
				 reminder->item_ref)) != NULL) {
				
				if (gtk_tree_model_get_iter(GL_ITEMSTM, &iter, path)) {
												
					gtk_tree_model_get(GL_ITEMSTM, &iter, 
					                   GL_COLUMN_DONE, &is_done, -1);
					
					// Remind
					if (! is_done) {
						glista_reminder_call_reminder_func(reminder);
					}
					
					// Clear reminder from item
					gtk_tree_store_set(gl_globs->itemstore, &iter, 
			        		           GL_COLUMN_REMINDER, NULL, -1);
				}
			}
			
			// Remove reminder from list and free it
			glista_reminder_free(reminder);
			rem_node = reminders;
			reminders = g_list_remove_link(reminders, rem_node);
			g_list_free_1(rem_node);
			
		} else {
			// If the first reminder is not due yet, break.
			break;
		}
	}
	
	if (reminders == NULL) { 
		// We are out of reminders
		rem_timeout_id = 0;
		return FALSE;
	} else {
		return TRUE;
	}
}

/**
 * glista_reminder_remove:
 * @reminder Reminder to clear out
 * 
 * Remove an existing reminder from the reminders queue and free it.
 */
void 
glista_reminder_remove(GlistaReminder *reminder)
{
	reminders = g_list_remove_all(reminders, reminder);
	glista_reminder_free(reminder);
}

void 
glista_reminder_remove_selected()
{
	GList   *selected, *node;
	
	// Iterate over list of selected rows
	selected = glista_list_get_selected();	
	for (node = selected; node != NULL; node = node->next) {
	    GtkTreePath *path;

        path = gtk_tree_row_reference_get_path(
			(GtkTreeRowReference *)node->data
		);
		
        if (path) {
        	GtkTreeIter     iter;
			GlistaReminder *reminder;
			
	        if (gtk_tree_model_get_iter(GL_ITEMSTM, &iter, path)) {
				gtk_tree_model_get(GL_ITEMSTM, &iter, 
				                   GL_COLUMN_REMINDER, &reminder, -1);
								   
				if (reminder != NULL) {
					glista_reminder_remove(reminder);
					gtk_tree_store_set(GL_ITEMSTS, &iter, 
								       GL_COLUMN_REMINDER, NULL, -1);
				}
			}

			gtk_tree_path_free(path);
        }
	}
	
	// Free the list of selected rows
	g_list_foreach(selected, (GFunc) gtk_tree_row_reference_free, NULL);
	g_list_free (selected);
}

/**
 * glista_reminder_set:
 * @item_ref  A reference to the item to set a reminder on
 * @remind_at The time to remind at
 * 
 * Set a new reminder for an item.
 * 
 * If not done already, will start the periodical reminder checks.
 */
void
glista_reminder_set(GtkTreeRowReference *item_ref, time_t remind_at)
{
	GlistaReminder *reminder;
	GtkTreePath    *path;
	GtkTreeIter     iter;
	
	if ((path = gtk_tree_row_reference_get_path(item_ref)) != NULL) {
		if (gtk_tree_model_get_iter(GL_ITEMSTM, &iter, path)) {		
				
			// Check if the item is a category - if so set on all children
			if (gtk_tree_model_iter_has_child(GL_ITEMSTM, &iter)) {
				GtkTreeIter child;
				gint i;
				
				// Iterate over childred calling glista_reminder_set() on them
				for (i = 0; gtk_tree_model_iter_nth_child(GL_ITEMSTM, &child, 
				                                          &iter, i); i++) {
					GtkTreeRowReference *c_ref;
					GtkTreePath         *c_path;
					
					c_path = gtk_tree_model_get_path(GL_ITEMSTM, &child);
					c_ref = gtk_tree_row_reference_new(GL_ITEMSTM, c_path);
					
					glista_reminder_set(c_ref, remind_at);
					
					gtk_tree_path_free(c_path);
					gtk_tree_row_reference_free(c_ref);
				}
				
				return;
			}

			// Create a new GlistaReminder struct
			reminder = glista_reminder_new(item_ref, remind_at);
			reminders = g_list_insert_sorted(reminders, (gpointer) reminder, 
				(GCompareFunc) glista_reminder_insert_compare_func);
			
			// Set the item "reminder" column to point to the reminder object
			gtk_tree_store_set(gl_globs->itemstore, &iter, 
			                   GL_COLUMN_REMINDER, (gpointer) reminder, -1);
			
			// Make sure we have an inverval to check reminders
			if (rem_timeout_id == 0) {
				if (remind_module == NULL) {
					glista_reminder_init(GLISTA_RH_MODULE);
				}
				rem_timeout_id = g_timeout_add_seconds(GLISTA_REMINDER_INTERVAL,
					(GSourceFunc) glista_reminder_check_reminders, NULL);
			}
		}
		
		gtk_tree_path_free(path);
	}
}

/**
 * glista_reminder_set_on_selected:
 * @remind_at Time to remind at
 * 
 * Set reminders at the specified time on all currently selected items 
 */
void
glista_reminder_set_on_selected(time_t remind_at)
{
	GList *ref_list, *node;
	
	ref_list = glista_list_get_selected();
	
	// Iterate over the selected rows, and set a reminder
	for (node = ref_list; node != NULL; node = node->next) {
		GtkTreeRowReference *row_ref = node->data;
		
		glista_reminder_set(row_ref, remind_at);
		gtk_tree_row_reference_free(row_ref);
	}
	
	// Free reference list
    g_list_free(ref_list);
}

/**
 * glista_reminder_time_as_string:
 * @reminder The reminder who's time we want to get
 * @prefix   Any prefix to add to the returned string
 * 
 * Get the time set for a reminder as string. String format is set at compile
 * time - see the GLISTA_REMINDER_TIME_FORMAT macro.
 * 
 * Returns a newly allocated gchar containing the time. Must be freed when no
 * longer needed.
 */
gchar*
glista_reminder_time_as_string(GlistaReminder *reminder, const gchar *prefix)
{
	struct tm *time;
	gchar     *time_text, *text;
	
	// Get time as text
	time = localtime(&(reminder->remind_at));
	time_text = g_malloc0(sizeof(gchar) * (GLISTA_REMINDER_TIME_STRLEN + 1));
	strftime(time_text, GLISTA_REMINDER_TIME_STRLEN, 
	         GLISTA_REMINDER_TIME_FORMAT, time);
	
	// Concat time and prefix
	text = g_strdup_printf("%s%s", prefix, time_text);
	
	g_free(time_text);
	
	return text;
}
