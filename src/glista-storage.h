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

#ifndef __GLISTA_STORAGE_H

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <glib.h>

// Constants
#define GL_XML_ENCODING "UTF-8"
#define GL_XML_FILENAME "itemstore.xml"

// Node names
#define GL_XNODE_ROOT "glista"
#define GL_XNODE_ITEM "item"
#define GL_XNODE_TEXT "text"
#define GL_XNODE_DONE "done"
#define GL_XNODE_PRNT "parent"
#define GL_XNODE_NOTE "note"
#define GL_XNODE_RMDR "reminder"

// Function prototypes
void glista_storage_load_all_items(GList **list);
void glista_storage_save_all_items(GList *all_items);

#define __GLISTA_STORAGE_H
#endif
