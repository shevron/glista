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

#ifndef __GLISTA_UI_H

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

// Text colors
#ifndef GLISTA_COLOR_PENDING
#define GLISTA_COLOR_PENDING "#000000"
#endif

#ifndef GLISTA_COLOR_DONE
#define GLISTA_COLOR_DONE "#a0a0a0"
#endif

// Macro to fetch UI components from GtkBuilder
#define glista_get_widget(w) gtk_builder_get_object(gl_globs->uibuilder, w)

gboolean glista_ui_init();

void glista_ui_shutdown();

void glista_ui_mainwindow_show();

void glista_ui_mainwindow_hide();

void glista_ui_mainwindow_toggle(); 

void on_itemstore_row_changed(GtkTreeModel *model, 
                              GtkTreePath *path,
                              GtkTreeIter *iter, 
                              gpointer user_data);

void on_itemstore_row_inserted(GtkTreeModel *model, 
                               GtkTreePath *path, 
                               GtkTreeIter *iter, 
                               gpointer user_data);

void on_itemstore_row_deleted(GtkTreeModel *model, 
                              GtkTreePath *path, 
                              gpointer user_data);

void on_list_selection_changed(GtkTreeSelection *selection, 
                               gpointer user_data);
                                                                                           
void on_item_text_edited(GtkCellRendererText *renderer, 
                         gchar *pathstr, 
                         gchar *text, 
                         gpointer user_data);
                    
void on_item_text_editing_started(GtkCellRenderer *renderer, 
                                  GtkCellEditable *editable, 
                                  gchar *pathstr, 
                                  gpointer user_data);

void on_item_done_toggled(GtkCellRendererToggle *renderer, 
                          gchar *pathstr, 
                          gpointer user_data);
								  							 
#define __GLISTA_UI_H
#endif
