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
 * Header file for all Glista plugins
 */

#ifndef __GLISTA_PLUGIN_H

// Plugin Types
typedef enum {
	GLISTA_PLUGIN_ALL,
	GLISTA_PLUGIN_STORAGE,
	GLISTA_PLUGIN_REMINDER
} GlistaPluginType; 

// Plugin Info Struct
typedef struct _glista_plugin { 
	GlistaPluginType  type;
	char             *plugin_name;
	char             *display_name;
	char             *module_path;
} GlistaPlugin;

// Plugin Declare Function Prototype
typedef GlistaPlugin*  (* GlistaPluginDeclare) ();

// Plugin Definition Macro
#define GLISTA_DECLARE_PLUGIN(_ty, _pn, _dn)\
G_MODULE_EXPORT GlistaPlugin *glista_plugin_declare() { \
	GlistaPlugin *plugin; \
	plugin = g_malloc(sizeof(GlistaPlugin)); \
	plugin->type         = _ty; \
	plugin->plugin_name  = g_strdup(_pn); \
	plugin->display_name = g_strdup(_dn); \
	plugin->module_path  = NULL; \
	return plugin; \
}

// Function Prototypes
GList *glista_plugin_query_plugins(GlistaPluginType type);
void   glista_plugin_free(GlistaPlugin *plugin);

#define __GLISTA_PLUGIN_H
#endif
