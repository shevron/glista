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

#include "glista.h"

#ifdef HAVE_UNIQUE

#include "glista-unique.h"
#include <unique/unique.h>

static UniqueApp *glista_uapp;

/**
 * activate_message_cb:
 * @app       Unique application object
 * @command   the command recieved
 * @data      message data
 * @time_     message time
 * @user_data user data bound at signal connection time
 *
 * Handle an "ACTIVATE" message sent to this instance by another instance just
 * starting up. Will show the main window and return an OK message if all is 
 * well.
 */
static UniqueResponse
activate_message_cb(UniqueApp *app, gint command, UniqueMessageData *data,
                    guint time_, gpointer user_data)
{
	if(command == UNIQUE_ACTIVATE) {
		glista_ui_mainwindow_show();
		return UNIQUE_RESPONSE_OK;
	} else {
		return UNIQUE_RESPONSE_CANCEL;
	}
}

/**
 * glista_unique_unref:
 * 
 * Dereference the Unique app object. Should be called to free resources before 
 * the program is shut down.
 */
void 
glista_unique_unref()
{
	g_object_unref(glista_uapp);
}

/**
 * glista_unique_is_single_inst:
 *
 * Initialize the Unique environment and check if there is a Glista instance 
 * already running. If so, will send the ACTIVATE message to this instance.
 *
 * Returns: TRUE if we are the first instance, FALSE otherwise.
 */
gboolean
glista_unique_is_single_inst()
{
	glista_uapp = unique_app_new(GLISTA_UNIQUE_ID, NULL);
	
	if (unique_app_is_running(glista_uapp)) {
		UniqueResponse response;
		
		response = unique_app_send_message(glista_uapp, UNIQUE_ACTIVATE, NULL);
		g_assert(response == UNIQUE_RESPONSE_OK);
		
		glista_unique_unref();
		return FALSE;
		
	} else {
		GtkWindow *window;
		
		window = GTK_WINDOW(glista_get_widget("glista_main_window"));
		unique_app_watch_window(glista_uapp, window);
		g_signal_connect(glista_uapp, "message-received", 
						 G_CALLBACK(activate_message_cb), NULL);
	
		return TRUE;
	}
}

#endif
