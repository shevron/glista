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

#include <glib.h>
#include <glib/gstdio.h>
#include <gmodule.h>
#include <gtk/gtk.h>

#include "glista.h"
#include "glista-plugin.h"

/**
 * glista_plugin_free: 
 * @plugin The plugin struct to free
 * 
 * Free a plugin information struct
 */
void
glista_plugin_free(GlistaPlugin *plugin)
{
	g_free(plugin->plugin_name);
	g_free(plugin->display_name);
	
	if (plugin->module_path != NULL) {
		g_free(plugin->module_path);
	}
	
	g_free(plugin);
}

/**
 * get_plugin_info:
 * @file File name to query
 * 
 * Get the plugin information from a file. Called by 
 * glista_plugin_query_plugins() for each file that is thought to be a plugin.
 * 
 * Returns the plugin info as a newly-allocated GlistaPlugin struct
 */
static GlistaPlugin*
get_plugin_info(const gchar *file)
{
	gchar               *path;
	GModule             *module;
	GlistaPluginDeclare  declare;
	GlistaPlugin        *plugin = NULL;
	
	// Build module path
	path = g_strdup_printf("%s%s%s", GLISTA_LIB_DIR, G_DIR_SEPARATOR_S, file);
	
	// Open module
	module = g_module_open(path, G_MODULE_BIND_LAZY);
	g_free(path);
	
	if (module != NULL) {
		// Call the module's 'declare' function
		if (g_module_symbol(module, "glista_plugin_declare", 
		                    (gpointer *) &declare)) {
								
			plugin = declare();
			plugin->module_path = g_strdup(g_module_name(module));
		}
		
		g_module_close(module);
	}
	
	return plugin;
}

/**
 * glista_plugin_query_plugins:
 * @type Type of plugins to query, seee GlistaPluginType
 * 
 * Get a list of all available plugins. This is a fairly expensive process of
 * getting a list of files in the lib directory that have the system's loadable
 * module extension, openining and loading it, querying it and then closing it. 
 * 
 * It should be executed rarely.
 * 
 * Returns a linked list of GlistaPlugin structs
 */
GList *
glista_plugin_query_plugins(GlistaPluginType type)
{
	GDir         *plugindir;
	GError       *error;
	GPatternSpec *pattern;
	const gchar  *file;
	GlistaPlugin *plugin;
	GList        *pluginlist = NULL; 
	
	if ((plugindir = g_dir_open(GLISTA_LIB_DIR, 0, &error)) != NULL) {
		pattern = g_pattern_spec_new("*." G_MODULE_SUFFIX);
		while((file = g_dir_read_name(plugindir)) != NULL) {
			if (g_pattern_match_string(pattern, file)) {
				plugin = get_plugin_info(file);
				if (plugin != NULL) {
					if (type == GLISTA_PLUGIN_ALL || plugin->type == type) {
						pluginlist = g_list_append(pluginlist, plugin);
					} else {
						glista_plugin_free(plugin);
					}
				}
			}
		}
		
		g_dir_close(plugindir);
		g_pattern_spec_free(pattern);
		
	} else if(error != NULL) {
		g_printerr("Can't read plugin directory %s: %s", GLISTA_LIB_DIR, 
			error->message);
		g_error_free(error);
	}
	
	return pluginlist;
}
