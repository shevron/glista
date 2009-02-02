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
 
/**
 * Glista reminders functionality header file
 */
 
#ifndef _GLISTA_REMINDER_H

#include <time.h>
#include <gtk/gtk.h>

#ifndef GLISTA_REMINDER_INTERVAL
#define GLISTA_REMINDER_INTERVAL 3
#endif 

#ifndef GLISTA_REMINDER_TIME_STRLEN
#define GLISTA_REMINDER_TIME_STRLEN 100
#endif

#ifndef GLISTA_REMINDER_TIME_FORMAT
#define GLISTA_REMINDER_TIME_FORMAT "%c"
#endif

/**
 * Reminder struct 
 */

typedef struct _glista_reminder_struct {
	GtkTreeRowReference *item_ref;
	time_t               remind_at;
} GlistaReminder;

/**
 * Public function signatures 
 */

void   glista_reminder_set(GtkTreeRowReference *item_ref, 
                           time_t remind_at);
                         
void   glista_reminder_set_on_selected(time_t remind_at);

void   glista_reminder_remove(GlistaReminder *reminder);

void   glista_reminder_shutdown();

void   glista_reminder_free(GlistaReminder *reminder);

GQuark glista_reminder_error_quark();

gchar* glista_reminder_time_as_string(GlistaReminder *reminder, 
                                      const gchar *prefix);

void   glista_reminder_remove_selected();

/**
 * Function signatures for reminderhandler modules 
 */

typedef gboolean (* GlistaRHInitFuc) (GError **error);

typedef gboolean (* GlistaRHRemindFunc) (GlistaReminder *reminder, 
                                         GError **error);

typedef gboolean (* GlistaRHShutdownFunc) (GError **error);

/**
 * GError domain GQuark and error codes
 */

#define GLISTA_REMINDER_ERROR_QUARK_S "glista-reminder"
#define GLISTA_REMINDER_ERROR_QUARK glista_reminder_error_quark()

enum {
	GLISTA_REMINDER_ERROR_INIT,
	GLISTA_REMINDER_ERROR_MESSAGE,
	GLISTA_REMINDER_ERROR_SHUTDOWN,
	GLISTA_REMINDER_ERROR_OTHER
};

#define GLISTA_RH_MODULE "libnotify"

#define _GLISTA_REMINDER_H
#endif
