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

#include <string.h>
#include <errno.h>
#include <glib.h>
#include <glib-object.h>
#include <gtk/gtk.h>
#include <gtk/gtkmain.h>
#include "glista.h"
#include "glista-ui.h"
#include "glista-storage.h"
#include "glista-unique.h"

#ifdef HAVE_GTKSPELL
#include <gtkspell/gtkspell.h>
#endif

static gboolean (*glista_dnd_old_drag_data_received)(
	GtkTreeDragDest *drag_dest, GtkTreePath *dest, 
	GtkSelectionData *selection_data);

/**
 * Glista main program functions
 */

/**
 * glista_item_get_single_selected:
 * @selection Current tree selection
 * 
 * If the current selection is a single item (not a category, not more or less
 * than one item) will return a GtkTreeIter pointing to this item. If not, will
 * return NULL.
 * 
 * Returns: A pointer to a GtkTreeIter referring to the single selected item or
 *          NULL if not exists.
 */
GtkTreeIter*
glista_item_get_single_selected(GtkTreeSelection *selection)
{
	gint         selected_c;
	GtkTreeIter *ret = NULL;
	
	selected_c = gtk_tree_selection_count_selected_rows(selection);
	
	// Only continue checking if we only have 1 selected item
	if (selected_c == 1) {
		GList       *list;
		GtkTreeIter  iter;
		gboolean     is_cat;
		
		list = gtk_tree_selection_get_selected_rows(selection, NULL);
		g_assert(g_list_length(list) == 1);
	
		if (gtk_tree_model_get_iter(GTK_TREE_MODEL(gl_globs->itemstore), &iter, 
		                            (GtkTreePath *) list->data)) {
			gtk_tree_model_get(GTK_TREE_MODEL(gl_globs->itemstore), &iter, 
			                   GL_COLUMN_CATEGORY, &is_cat, -1);
			if (! is_cat) {
				ret = gtk_tree_iter_copy(&iter);
			}
		}
		
		g_list_foreach(list, (GFunc) gtk_tree_path_free, NULL);
		g_list_free(list);
	}
	
	return ret;
}

/**
 * glista_note_store_in_model:
 * 
 * Store the currently open note in the item store model in memory. This
 * does not guarantee that the data will be saved to disk.
 */
static void
glista_note_store_in_model()
{
	GtkTextView   *note_view;
	GtkTextBuffer *buffer;
	gchar         *note;
	
	// Get the view and it's buffer
	note_view = GTK_TEXT_VIEW(glista_get_widget("note_textview"));
	buffer = gtk_text_view_get_buffer(note_view);
	
	if (GTK_IS_TEXT_BUFFER(buffer)) {
		g_object_get(buffer, "text", &note, NULL);	
		
		if (gl_globs->open_note != NULL) {
			note = g_strstrip(note);
			
			if (strlen(note) == 0) {
				gtk_tree_store_set(gl_globs->itemstore, gl_globs->open_note, 
								   GL_COLUMN_NOTE, NULL, -1);
			} else {
				gtk_tree_store_set(gl_globs->itemstore, gl_globs->open_note, 
								   GL_COLUMN_NOTE, note, -1);
			}
		}
		
		g_free(note);
	}
}

/**
 * glista_note_open:
 * @iter Iterator pointing to the item to add note to
 * 
 * Open the item note pane, and set a pointer to an iterator pointing to the
 * item who's note is being set
 */
static void
glista_note_open(GtkTreeIter *iter)
{
	GtkWidget     *note_textview, *note_container;
	GtkTextBuffer *note_buffer;
	gchar         *note_text;
	gboolean       is_cat;
	
	// Check that this is not a category
	gtk_tree_model_get(GTK_TREE_MODEL(gl_globs->itemstore), iter, 
	                   GL_COLUMN_NOTE,     &note_text, 
	                   GL_COLUMN_CATEGORY, &is_cat, -1);
	                   
	if (is_cat == FALSE) {
		if (gl_globs->open_note != NULL) {
			gtk_tree_iter_free(gl_globs->open_note);
		}
		gl_globs->open_note = gtk_tree_iter_copy(iter);
		
		// Get view buffer
		note_textview = GTK_WIDGET(glista_get_widget("note_textview"));
		note_buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(note_textview));
		
		// Set buffer text
		if (note_text != NULL) {
			gtk_text_buffer_set_text(note_buffer, note_text, -1);
		} else {
			gtk_text_buffer_set_text(note_buffer, "", 0);
		}
		
#ifdef HAVE_GTKSPELL
		// Attach a GtkSpell object to the note editor if available
		if (gtkspell_get_from_text_view(GTK_TEXT_VIEW(note_textview)) == NULL) {
			GError *gtkspell_err = NULL;
			if (! gtkspell_new_attach(GTK_TEXT_VIEW(note_textview), NULL, 
									  &gtkspell_err)) {
										
				g_warning("Unable to initialize GtkSpell: [%d] %s\n", 
						  gtkspell_err->code,
						  gtkspell_err->message);
				
				g_error_free(gtkspell_err);
			}
		}
#endif

		// Show parent container and all it's children
		note_container = GTK_WIDGET(glista_get_widget("note_container"));
		gtk_widget_show_all(note_container);
		
		// Grab focus
		gtk_widget_grab_focus(note_textview);
	}
}

/**
 * glista_note_open_if_visible:
 * @iter Iterator pointing to the currently selected item
 *
 * If note pane is already opened, will set the contents of the pane to the
 * newly selected item's note. 
 */
void
glista_note_open_if_visible(GtkTreeIter *iter)
{
	GtkWidget *note_container;
	gboolean   visible;
	
	note_container = GTK_WIDGET(glista_get_widget("note_container"));
	g_object_get(note_container, "visible", &visible, NULL);
	
	if (visible) {
		glista_note_store_in_model();
		glista_note_open(iter);
	}
}

/**
 * glista_note_close:
 * 
 * Close the item note pane and save the item note text according to the
 * saved iterator pointing to the item that was edited.
 */ 
void
glista_note_close()
{
	GtkWidget *note_container;
	
	glista_note_store_in_model();
	
	// Clear the link to the current open iter
	if (gl_globs->open_note != NULL) {
		gtk_tree_iter_free(gl_globs->open_note);
		gl_globs->open_note = NULL;
	}
	
	// Hide the note panel
	note_container = GTK_WIDGET(glista_get_widget("note_container"));
	gtk_widget_hide(note_container);
}

/**
 * glista_note_toggle:
 * @iter Iterator pointing to the currently activated item
 * 
 * Toggle between open and closed note panel
 */
void
glista_note_toggle(GtkTreeIter *iter)
{
	GtkWidget   *note_container;
	gboolean     visible;
	
	note_container = GTK_WIDGET(glista_get_widget("note_container"));
	g_object_get(note_container, "visible", &visible, NULL);
	
	if (visible) {
		glista_note_close();
	} else {
		glista_note_open(iter);
	}
}

/**
 * glista_note_toggle_selected:
 * @selection Currently selected items
 * 
 * Toggle the note panel open / close status.
 */
void
glista_note_toggle_selected(GtkTreeSelection *selection)
{
	GtkWidget   *note_container;
	gboolean     visible;
	
	note_container = GTK_WIDGET(glista_get_widget("note_container"));
	g_object_get(note_container, "visible", &visible, NULL);
	
	if (visible) {
		glista_note_close();
		
	} else {
		GList       *list;
		GtkTreeIter  iter;
	
		list = gtk_tree_selection_get_selected_rows(selection, NULL);
		if (g_list_length(list) == 1) {
			if (gtk_tree_model_get_iter(GTK_TREE_MODEL(gl_globs->itemstore),
		    	                        &iter, (GtkTreePath *) list->data)) {
		
				glista_note_open(&iter);
			}
		}
	
		// Free the selection list
		g_list_foreach(list, (GFunc) gtk_tree_path_free, NULL);
		g_list_free (list);
	}
}

/**
 * glista_get_category_iter:
 * @key The category key to look for
 *
 * Get the category path for a category name. If category does not exist, will 
 * create it
 *
 * Returns: The path to the requested category
 */
GtkTreePath*
glista_category_get_path(gchar *key)
{
	GtkTreeRowReference *rowref;
	GtkTreeIter          iter;
	GtkTreePath         *path;
	gchar               *key_c;
	
	key_c = g_utf8_strdown (key, -1);
	rowref = g_hash_table_lookup(gl_globs->categories, key_c);

	
	if (rowref == NULL) { // Category doesn't exist yet
		// Add category
		gtk_tree_store_append(gl_globs->itemstore, &iter, NULL);
		gtk_tree_store_set(gl_globs->itemstore, &iter, 
						   GL_COLUMN_TEXT, key, 
						   GL_COLUMN_CATEGORY, TRUE,
						   -1);
		
		// Add row reference to categories hash table
		path = gtk_tree_model_get_path(GTK_TREE_MODEL(gl_globs->itemstore), 
									   &iter);
		
		rowref = gtk_tree_row_reference_new(GTK_TREE_MODEL(gl_globs->itemstore),
											path);
		
		g_hash_table_insert(gl_globs->categories, key_c, rowref);
		
	} else { // Category already exists
		path = gtk_tree_row_reference_get_path(rowref);
		g_free(key_c);
	}
	
	return path;
}

/**
 * glista_list_add:
 * @text: Item text
 *
 * Adds an additional to-do item to the list. The item text must be provided, 
 * and all other values (done, color, etc.) are set to default values. Text
 * is stripped of leading and trailing spaces, and empty strings are ignored.
 */
void
glista_list_add(GlistaItem *item, gboolean expand)
{
	GtkTreeIter  iter, parent_iter;
	GtkTreePath *parent;
	
	if (item->parent == NULL) {
		gtk_tree_store_append(gl_globs->itemstore, &iter, NULL);
		
	} else {
		parent = glista_category_get_path(item->parent);		
		gtk_tree_model_get_iter(GTK_TREE_MODEL(gl_globs->itemstore), 
								&parent_iter,
								parent);
		
		gtk_tree_store_append(gl_globs->itemstore, &iter, &parent_iter);
		
		// Expand parent so that new child is visible
		if (expand) gtk_tree_view_expand_row(
			GTK_TREE_VIEW(glista_get_widget("glista_item_list")), parent, TRUE);
	}
	
	gtk_tree_store_set(gl_globs->itemstore, &iter, 
	                   GL_COLUMN_DONE, item->done, 
	                   GL_COLUMN_TEXT, item->text, 
	                   GL_COLUMN_NOTE, item->note,
					   -1);
}

/**
 * glista_item_create_from_text:
 * @text: Input text from user
 *
 * Create a new entry from user input. Will parse and break down the text,
 * create a new item and add it to the model.
 */
void
glista_item_create_from_text(gchar *text)
{
	gchar      **tokens;
	GlistaItem  *item = NULL;
	
	// Split the input into category: item 
	tokens = g_strsplit(text, GLISTA_CAT_DELIM, 2);
	
	// Did we get anything?
	if (tokens[0] != NULL) {
		g_strstrip(tokens[0]);
	
		if (tokens[1] != NULL) { 
			g_strstrip(tokens[1]);
			if (strlen(tokens[1]) > 0) {
				item = glista_item_new(tokens[1], tokens[0]);
			}
			
		} else {
			if (strlen(tokens[0]) > 0) {
				item = glista_item_new(tokens[0], NULL);
			}
		}
		
		if (item != NULL) {
			glista_list_add(item, TRUE);
			glista_item_free(item);
		}
	}
	
	g_strfreev(tokens);
}

/**
 * glista_item_toggle_done:
 * @path: The path in the list to toggle
 *
 * Toggle the "done" flag on an item in the to-do list. This function only 
 * negates the value of the "done" column on @path. Other things like resorting,
 * coloring, etc. is automatically handled in the GtkTreeView layer.
 */
void 
glista_item_toggle_done(GtkTreePath *path)
{
	GtkTreeIter  iter;
	gboolean     current; 

	if (gtk_tree_model_get_iter(GTK_TREE_MODEL(gl_globs->itemstore), 
								&iter, path)) {
									
		gtk_tree_model_get(GTK_TREE_MODEL(gl_globs->itemstore), &iter, 
		                   GL_COLUMN_DONE, &current, -1);
		                   
		gtk_tree_store_set(gl_globs->itemstore, &iter, GL_COLUMN_DONE, 
		                   (! current), -1);
	}
}

/**
 * glista_item_redraw_parent: 
 * @child_iter: Iterator pointing to the child element
 *
 * Takes in a pointer to a modified row, and if it has a parent, will trigger
 * a redraw of the parent by calling gtk_tree_model_row_changed() on the parent.
 */
void
glista_item_redraw_parent(GtkTreeIter *child_iter)
{
	GtkTreeIter  parent_iter;
	GtkTreePath *parent_path;
	
	// Check if item has a parent
	if (gtk_tree_model_iter_parent(GTK_TREE_MODEL(gl_globs->itemstore), 
								   &parent_iter, child_iter)) {
		
		parent_path = gtk_tree_model_get_path(
			GTK_TREE_MODEL(gl_globs->itemstore), &parent_iter);
		
		// Trigger a "row-changed" signal on tha prent as well
		gtk_tree_model_row_changed(GTK_TREE_MODEL(gl_globs->itemstore), 
								   parent_path, &parent_iter);
									   
		gtk_tree_path_free (parent_path);
	}
}

/**
 * glista_category_delete:
 * @category: The GtkTreeIter of the category row to remove 
 *
 * deletes a gategory, including all it's child items, from the tree model and
 * from the categories hash table
 */
void 
glista_category_delete(GtkTreeIter *category)
{
	gchar               *cat_name, *key;
	GtkTreeRowReference *rowref;
	
	// Get the name of the category we are deleting
	gtk_tree_model_get(GTK_TREE_MODEL(gl_globs->itemstore), category, 
					   GL_COLUMN_TEXT, &cat_name, -1);
	
	// Remove category from categories hashtable
	key = g_utf8_strdown (cat_name, -1);
	if ((rowref = g_hash_table_lookup(gl_globs->categories, key)) != NULL) {
		g_hash_table_remove(gl_globs->categories, key);
	}
	g_free(key);
	
	// Remove category from model
	gtk_tree_store_remove(gl_globs->itemstore, category);
}

/**
 * glista_category_confirm_delete:
 * @category: A GtkTreeIter pointing to the category to delete
 *
 * Show a message dialog confirming the deletion of a category with all it's
 * child items. If the user approves, will call glista_category_delete().
 */
void
glista_category_confirm_delete(GtkTreeIter *category) 
{
	gchar            *cat_name;
	gint              response;
	GtkWidget        *dialog;
	
	if (gtk_tree_model_iter_n_children(GTK_TREE_MODEL(gl_globs->itemstore), 
									   category) > 0) {
										   
		// If category has children, show confirmation dialog
		gtk_tree_model_get(GTK_TREE_MODEL(gl_globs->itemstore), category, 
						   GL_COLUMN_TEXT, &cat_name, -1);
		
		dialog = gtk_message_dialog_new(
			GTK_WINDOW(glista_get_widget("glista_main_window")),
			GTK_DIALOG_DESTROY_WITH_PARENT | GTK_DIALOG_MODAL,
			GTK_MESSAGE_WARNING,
			GTK_BUTTONS_OK_CANCEL,
			"Are you sure you want to delete the category \"%s\" and "
			"all the items in it?",
			cat_name);
		
		response = gtk_dialog_run(GTK_DIALOG(dialog));
		gtk_widget_destroy(dialog);
		
		// Delete category only if confirmed
		if (response == GTK_RESPONSE_OK) {
			glista_category_delete (category);
		}
										
	} else {
		// If it has no children, just delete it
		glista_category_delete (category);	
	}	
}

/**
 * glista_item_delete: 
 * @iter: Iterator pointing to the item to delete
 *
 * Delete an item (not a category) from the list. If item is in a category and
 * the category becomes empty, the parent category will also be deleted. 
 */
void
glista_item_delete(GtkTreeIter *iter)
{
	gboolean    has_parent;
	GtkTreeIter parent;
	
	// Check if this item has a parent category
	has_parent = gtk_tree_model_iter_parent(
		GTK_TREE_MODEL(gl_globs->itemstore), &parent, iter);
	
	// Remove item
	gtk_tree_store_remove(gl_globs->itemstore, iter);
	
	// Check if parent is now empty
	if (has_parent && gtk_tree_model_iter_n_children(
		GTK_TREE_MODEL(gl_globs->itemstore), &parent) < 1) {
		
		// Delete parent as well
		glista_category_delete(&parent);
	}
}
				   
/**
 * glista_list_delete_reflist:
 * @ref_list: A linked list of items to delete
 *
 * Delete all items passed in in a linked list. This function is called from 
 * other deletion functions to clear out selected or done items. It does not 
 * free the list of items for now. 
 * 
 * If the list contains a category that still has items in it, it will confirm
 * with the user first before deleting.
 */
static void 
glista_list_delete_reflist(GList *ref_list)
{
	GList *node;
	
	for (node = ref_list; node != NULL; node = node->next) {
	    GtkTreePath *path;

        path = gtk_tree_row_reference_get_path(
			(GtkTreeRowReference *)node->data
		);
		
        if (path) {
        	GtkTreeIter iter;
			gboolean    is_cat;

	        if (gtk_tree_model_get_iter(GTK_TREE_MODEL(gl_globs->itemstore), 
										&iter, path)) {
				
				// Check if this is a category we are deleting
				gtk_tree_model_get(GTK_TREE_MODEL(gl_globs->itemstore), &iter, 
								   GL_COLUMN_CATEGORY, &is_cat, -1);
				if (is_cat) {
					// Show the confirm dialog before deleting any categories
					glista_category_confirm_delete(&iter);
					
				} else {	
					glista_item_delete(&iter);
				}
			}

			gtk_tree_path_free(path);
        }
	}
}

/**
 * glista_list_get_selected_reflist:
 * @model:    Tree model to iterate on
 * @path:     Current path in tree
 * @iter:     Current iter in tree
 * @ref_list: List to populate with row references
 *
 * Callback function for gtk_tree_selection_selected_foreach(). Populates a list
 * of references to all selected rows in the list. 
 *
 * This function is called from glista_list_delete_selected() in order to build
 * a list of references to rows to delete, because rows cannot be deleted 
 * directly.
 */
static void 
glista_list_get_selected_reflist(GtkTreeModel *model, GtkTreePath *path,
								 GtkTreeIter *iter, GList **ref_list)
{
	GtkTreeRowReference *ref;
	
	g_assert(ref_list != NULL);
	ref = gtk_tree_row_reference_new(model, path);
	*ref_list = g_list_append(*ref_list, ref);
}

/**
 * glista_list_delete_selected:
 *
 * Delete all selected items from the list. Populates a list of all selected
 * items (using glista_list_get_selected_reflist() as a callback function) and
 * passes it on to glista_list_delete_reflist() to do the actual deletion.
 */
void 
glista_list_delete_selected()
{
	GtkTreeView      *treeview;
	GtkTreeSelection *selection;
	GList            *ref_list;
	
	treeview = GTK_TREE_VIEW(glista_get_widget("glista_item_list"));
	selection = gtk_tree_view_get_selection(treeview);
	
	// Populate list of row references to delete
	ref_list = NULL;
	gtk_tree_selection_selected_foreach(
		selection, 
	    (GtkTreeSelectionForeachFunc) glista_list_get_selected_reflist,
	    &ref_list
    );
	
	// Delete items
	glista_list_delete_reflist(ref_list);

	// Free reference list
	g_list_foreach(ref_list, (GFunc) gtk_tree_row_reference_free, NULL);
    g_list_free(ref_list);
}

/**
 * glista_list_get_done_reflist: 
 * @ref_list a pointer-pointer to the GList to populate
 * @parent   the parent iter when recursing into category children
 *
 * Populate a list of done items to be deleted. Called internally by 
 * glista_list_delete_done(), which later on calles 
 * glista_list_delete_reflist() to do the actual deletion. 
 */
static void
glista_list_get_done_reflist(GList **ref_list, GtkTreeIter *parent)
{
	GtkTreeIter  iter;
	gboolean     status;
	
	// Get the iter set for first row
	status = gtk_tree_model_iter_children(GTK_TREE_MODEL(gl_globs->itemstore), 
	                                      &iter, parent);

	// Iterate on all rows
	while (status) {
		GtkTreeRowReference *rowref;
		GtkTreePath         *path;
		gboolean             is_done, is_cat;				 
		
		gtk_tree_model_get(GTK_TREE_MODEL(gl_globs->itemstore), &iter, 
		                   GL_COLUMN_DONE, &is_done,
						   GL_COLUMN_CATEGORY, &is_cat, 
						   -1);
		
		// If it is a category, look into it's child items
		if (is_cat) {
			glista_list_get_done_reflist(ref_list, &iter);
		
		// If it is done, add it to the list of references
		} else if (is_done) {
			// Get a reference to the row
			path = gtk_tree_model_get_path(GTK_TREE_MODEL(gl_globs->itemstore), 
										   &iter);
			rowref = gtk_tree_row_reference_new(
				GTK_TREE_MODEL(gl_globs->itemstore), path);
			
			// Add reference to the linked list
			*ref_list = g_list_append(*ref_list, rowref);
			
			// Free path
			gtk_tree_path_free(path);
		}
		
		// Advance to next row
		status = gtk_tree_model_iter_next(GTK_TREE_MODEL(gl_globs->itemstore), 
										  &iter);
	}
}


/**
 * glista_list_delete_done:
 * 
 * Clear off all the items marked as "done" from the list. Normally this is 
 * called when the "Clear" button is activated.
 */
void 
glista_list_delete_done()
{
	GList *ref_list;
	
	// Populate reference list
	ref_list = NULL;
	glista_list_get_done_reflist(&ref_list, NULL);

	// Delete items
	glista_list_delete_reflist(ref_list);

	// Free reference list
	g_list_foreach(ref_list, (GFunc) gtk_tree_row_reference_free, NULL);
    g_list_free(ref_list);
}

/**
 * glista_category_rename: 
 * @old_path: The path of the category to rename
 * @old_iter: The iterator pointing to the category to rename
 * @new_name: The new name to change to
 *
 * Rename / move a category. If category with the new name already exists, will
 * merge the children of the old category into the new one. 
 */
static void 
glista_category_rename(GtkTreePath *old_path, GtkTreeIter *old_iter, 
					   gchar *new_name)
{
	GtkTreeIter  child_iter;
	GtkTreePath *new_cat;
	gchar       *old_name;
	
	// First of all make sure that the name is actually changing
	gtk_tree_model_get(GTK_TREE_MODEL(gl_globs->itemstore), old_iter, 
					   GL_COLUMN_TEXT, &old_name, -1);
	if (g_strcmp0(old_name, new_name) != 0) {
		
		// Create a new category
		new_cat = glista_category_get_path(new_name);
		
		// Move all child elements to the path of this category
		if (gtk_tree_model_iter_children(GTK_TREE_MODEL(gl_globs->itemstore), 
										 &child_iter, old_iter)) {
			
			do {
				GlistaItem *item;
				gchar      *item_text, *item_note;
				gboolean    item_done;
				
				gtk_tree_model_get(GTK_TREE_MODEL(gl_globs->itemstore), 
								   &child_iter, 
								   GL_COLUMN_TEXT, &item_text,
								   GL_COLUMN_NOTE, &item_note,
								   GL_COLUMN_DONE, &item_done, -1);
				
				// Add new item to new parent
				item = glista_item_new(item_text, new_name);
				item->done = item_done;
				item->note = item_note; 
				glista_list_add(item, FALSE);
				glista_item_free(item);

			} while (gtk_tree_model_iter_next(
				GTK_TREE_MODEL(gl_globs->itemstore), &child_iter));
					
			// If the old category was expanded, expand the new one
			if (gtk_tree_view_row_expanded(
				GTK_TREE_VIEW(glista_get_widget("glista_item_list")), 
				old_path)) {
					
				gtk_tree_view_expand_row(
					GTK_TREE_VIEW(glista_get_widget("glista_item_list")),
					new_cat, FALSE);
			}
		}
		
		// Delete old category with it's children
		glista_category_delete(old_iter);
		
		// Free the new category path
		gtk_tree_path_free(new_cat);
	}
}

/**
 * glista_item_change_text:
 * @path: The path of the item to change
 * @text: The new text to set
 *
 * Change the text of one of the list items. If it is a category we are 
 * renaming, call glista_category_rename().
 */
void
glista_item_change_text(GtkTreePath *path, gchar *text)
{
	GtkTreeIter iter;
	gboolean    is_cat;
	
	if (gtk_tree_model_get_iter(GTK_TREE_MODEL(gl_globs->itemstore), 
								&iter, path)) {
									
		gtk_tree_model_get(GTK_TREE_MODEL(gl_globs->itemstore), &iter, 
						   GL_COLUMN_CATEGORY, &is_cat, -1);
		
		if (is_cat) {
			glista_category_rename (path, &iter, text);
		} else {
			gtk_tree_store_set(gl_globs->itemstore, &iter, 
		    	               GL_COLUMN_TEXT, text, -1);
		}
	}
}

/**
 * glista_item_new:
 * @text: Item text
 * 
 * Create a new GlistaItem object and set the text property of it. Will also
 * set the other properties to their default values. The item will later need
 * to be freed using glista_item_free().
 *
 * Return: a newly created GlistaItem struct
 */
GlistaItem*
glista_item_new(const gchar *text, const gchar *parent)
{
	GlistaItem *item;
	
	item = g_malloc(sizeof(GlistaItem));
	g_assert(item != NULL);
	
	item->done   = FALSE;
	item->text   = (gchar *) text;
	item->parent = (gchar *) parent;
	item->note   = NULL;
	
	return item;
}

/**
 * glista_item_free:
 * @item: The item to free
 *
 * Free an allocated GlistaItem struct
 */
void 
glista_item_free(GlistaItem *item)
{
	// Free note if set
	if (item->note != NULL) {
		g_free(item->note);
	}
	
	g_free(item);
}

/**
 * glista_get_category_cell_text:
 * @model: The tree model 
 * @iter:  The iterator pointing to the parent category
 *
 * Get the text to display next to an item or a category in the tree view. Will
 * add the count of done tasks out of the total tasks in the category to the 
 * category name
 *
 * Returns: the category string to display
 */
static gchar *
glista_item_get_display_text(GtkTreeModel *model, GtkTreeIter *iter)
{
	gboolean     is_cat, is_done;
	gint         i, child_c, done_c;
	gchar       *text, *newtext, *child_c_str, *done_c_str;
	GtkTreeIter  child;
	
	// Get category name
	gtk_tree_model_get(model, iter, GL_COLUMN_TEXT, &text, 
					                GL_COLUMN_CATEGORY, &is_cat,
					                -1);
	
	// If the row is not a category, just return the text
	if (is_cat == FALSE) {
		return text;
	}
	
	// Get count / status
	done_c = 0;
	child_c = gtk_tree_model_iter_n_children(model, iter);
	for (i = 0; i < child_c; i++) {
		if (gtk_tree_model_iter_nth_child(model, &child, iter, i) == FALSE) {
			break;
		}
		
		gtk_tree_model_get(model, &child, GL_COLUMN_DONE, &is_done, -1);
		if (is_done == TRUE) ++done_c;
	}
	
	child_c_str = g_strdup_printf("%d", child_c);
	done_c_str = g_strdup_printf("%d", done_c);
	
	newtext = g_strconcat(text, " (", done_c_str, "/", child_c_str, ")", NULL);
	
	g_free(child_c_str);
	g_free(done_c_str);
	
	return newtext;
}

/**
 * glista_item_text_cell_data_func:
 * @column: Column to be rendered
 * @cell:   Cell to be rendered
 * @model:  Related data model
 * @iter:   Tree iterator
 * @data:   User data passed at connect time
 *
 * Callback function called whenever an item's text cell needs to be rendered. 
 * Will define the foreground color of the text - gray if the item is done (by
 * default at least), and black if it is pending.
 *
 * See gtk_tree_view_column_set_cell_data_func() for more info.
 */
void
glista_item_text_cell_data_func(GtkTreeViewColumn *column, 
                                GtkCellRenderer *cell, GtkTreeModel *model,
                                GtkTreeIter *iter, gpointer data)
{
	gboolean  done, category;
	gchar    *text;
	gint      weight;

	gtk_tree_model_get(model, iter, GL_COLUMN_DONE, &done, 
					   				GL_COLUMN_CATEGORY, &category,
					   				-1);
	
	// Set color according to done / not done
	if (done == TRUE) {
		g_object_set(cell, "foreground", GLISTA_COLOR_DONE, NULL);
	} else {
		g_object_set(cell, "foreground", GLISTA_COLOR_PENDING, NULL);
	}
	
	// Set weight and text depending on whether this is a category or not
	text = glista_item_get_display_text(model, iter);
	weight = (category ? 800 : 400);
	g_object_set(cell, "weight", weight, "text", text, NULL);
	
	g_free(text);
}

/**
 * glista_item_done_cell_data_func:
 * @column: Column to be rendered
 * @cell:   Cell to be rendered
 * @model:  Related data model
 * @iter:   Tree iterator
 * @data:   User data passed at connect time
 *
 * Callback function called whenever an item's "done" toggle cell needs to be 
 * rendered. For now mostly hides the toggle for category rows.
 *
 * See gtk_tree_view_column_set_cell_data_func() for more info.
 */
void
glista_item_done_cell_data_func(GtkTreeViewColumn *column, 
                                GtkCellRenderer *cell, GtkTreeModel *model,
                                GtkTreeIter *iter, gpointer data)
{
	gboolean is_cat;
	
	gtk_tree_model_get(model, iter, GL_COLUMN_CATEGORY, &is_cat, -1);
	g_object_set(cell, "visible", ! is_cat, NULL);
}

/**
 * glista_item_note_cell_data_func:
 * @column: Column to be rendered
 * @cell:   Cell to be rendered
 * @model:  Related data model
 * @iter:   Tree iterator
 * @data:   User data passed at connect time
 *
 * Callback function called whenever an item's "done" toggle cell needs to be 
 * rendered. For now mostly hides the toggle for category rows.
 *
 * See gtk_tree_view_column_set_cell_data_func() for more info.
 */
void
glista_item_note_cell_data_func(GtkTreeViewColumn *column, 
                                GtkCellRenderer *cell, GtkTreeModel *model,
                                GtkTreeIter *iter, gpointer data)
{
	gchar *note;
	
	gtk_tree_model_get(model, iter, GL_COLUMN_NOTE, &note, -1);
	
	if (note != NULL && strlen(note) > 0) {
		// has note!
		g_object_set(cell, "stock-id", GTK_STOCK_PASTE, NULL);
	} else {
		g_object_set(cell, "stock-id", NULL, NULL);
	}
	
	g_free(note);
}

/**
 * glista_list_sort_func:
 * @model:     Model being sorted
 * @row_a:     First row to compare
 * @row_b:     Second row to compare
 * @user_data: User data passed at connect time
 *
 * Callback sorting function for the list of items. Will put pending items 
 * first, and done items at the bottom. Secondary sorting is done 
 * alphabetically. 
 *
 * Returns: negative if row_a sorts higher, 0 if both are equal, or positive if
 * row_b sorts first.
 *
 * See gtk_tree_sortable_set_sort_func() for more info.
 */
gint
glista_list_sort_func(GtkTreeModel *model, GtkTreeIter *row_a, 
                      GtkTreeIter *row_b, gpointer user_data)
{
	gboolean done_a, done_b, cat_a, cat_b; 
	gint     ret;
	
	// Check if any of the items is a category
	// Fetch done & category falgs for both rows
	gtk_tree_model_get(model, row_a, GL_COLUMN_DONE, &done_a, 
					                 GL_COLUMN_CATEGORY, &cat_a, 
					                 -1);
	gtk_tree_model_get(model, row_b, GL_COLUMN_DONE, &done_b, 
					                 GL_COLUMN_CATEGORY, &cat_b, 
					   -1);
		
	// Is one a category and the other is not?
	if (cat_a == cat_b) {
	
		// Is one done and the other is not?
		if (done_a == done_b) {
			
			// Go on to comparing the text
			gchar *text_a, *text_b;
		
			gtk_tree_model_get(model, row_a, GL_COLUMN_TEXT, &text_a, -1);
			gtk_tree_model_get(model, row_b, GL_COLUMN_TEXT, &text_b, -1);
			
			if (text_a == NULL || text_b == NULL) {
				if (text_a == NULL && text_b == NULL) {
					ret = 0;
				} else {
					ret = (text_a == NULL) ? -1 : 1;
				}
			} else {
				ret = g_utf8_collate(text_a, text_b);
			}
			
			g_free(text_a);
			g_free(text_b);

		// If they are not equal, order the pending tasks on top
		} else {
			ret = (done_a ? 1 : -1);
		}
		
	} else {
		ret = (cat_a ? -1 : 1);
	}
	
	return ret;
}

/**
 * glista_dnd_is_draggable:
 * @drag_source The drag source
 * @oath        The path in the tree of the dragged row
 *
 * Tells whether or not a row can be dragged. A row can be dragged if it is
 * not a category. If row no longer exists will return FALSE.
 *
 * Returns: TRUE if row can be dragged, FALSE otherwise
 */
static gboolean
glista_dnd_is_draggable(GtkTreeDragSource *drag_source, GtkTreePath *path)
{
	GtkTreeIter iter;
	gboolean    is_cat;
	
	if (gtk_tree_model_get_iter(GTK_TREE_MODEL(gl_globs->itemstore), 
								&iter, path)) {
		gtk_tree_model_get(GTK_TREE_MODEL(gl_globs->itemstore), &iter,
						   GL_COLUMN_CATEGORY, &is_cat, -1);
		return (! is_cat);
	} else {
		return FALSE;
	}
}

/**
 * glista_dnd_drop_possible:
 * @drag_dest:     The drag destination
 * @path:          The path in the tree to check
 * @selection_data The selected data
 *
 * Tells whether a drop is possible before @path, on the same level. In our
 * case, drops are always possible. 
 *
 * Returns: TRUE
 */
static gboolean 
glista_dnd_drop_possible(GtkTreeDragDest *drag_dest, GtkTreePath *path, 
						 GtkSelectionData *selection_data)
{
	GtkTreePath *parent;
	GtkTreeIter  iter;
	gint         depth;
	gboolean     can_drop = TRUE;
	
	depth = gtk_tree_path_get_depth(path);
	
	// Can't create 3rd level or more
	if (depth > 2) {
		return FALSE;
	}
	
	// Can always drop on root level
	if (depth < 2) {
		return TRUE;
	}
	
	// If level is 2, we have to check that the parent is a category
	parent = gtk_tree_path_copy (path);
	if (gtk_tree_path_up(parent)) {
		if (gtk_tree_model_get_iter(GTK_TREE_MODEL(gl_globs->itemstore), 
									&iter, parent)) {
			gtk_tree_model_get(GTK_TREE_MODEL(gl_globs->itemstore), &iter,
							   GL_COLUMN_CATEGORY, &can_drop, -1);
		}
	}
	gtk_tree_path_free(parent);
	
	return can_drop;
}

/**
 * glista_dnd_delete_row:
 * @drag_source: The source of the drag operation
 * @path:        The path to delete
 *
 * Deletes a row after it has been dragged to a new location. Will call 
 * glista_item_delete() to make sure the empty parent category is also deleted. 
 *
 * Returns: TRUE if the deletion succeeded, FALSE otherwise. 
 */
static gboolean
glista_dnd_delete_row(GtkTreeDragSource *drag_source, GtkTreePath *path)
{
	GtkTreeIter iter;
	
	if (gtk_tree_model_get_iter(GTK_TREE_MODEL(gl_globs->itemstore), &iter, 
								path)) {

		glista_item_delete(&iter);
		return TRUE;
	
	} else {
		return FALSE;
	}
}

/**
 * glista_dnd_drag_data_received:
 * @drag_dest:      The drag destination
 * @path:           The path being dragged
 * @selection_data: The selection data
 *
 * Wrap the orignal drop handler, triggering a sort of the tree after a drop
 * is made. 
 */
static gboolean
glista_dnd_drag_data_received(GtkTreeDragDest *drag_dest, GtkTreePath *path,
							  GtkSelectionData *selection_data)
{
	gboolean    res;

	res = glista_dnd_old_drag_data_received(drag_dest, path, selection_data);
	if (res) {
		
		// FIXME / HACK: The only way I found to trigger a sort is to change 
		// the sort column and revert it back.
		
		gtk_tree_sortable_set_sort_column_id(
			GTK_TREE_SORTABLE(gl_globs->itemstore), GL_COLUMN_CATEGORY, 
			GTK_SORT_ASCENDING);
		
		gtk_tree_sortable_set_sort_column_id(
			GTK_TREE_SORTABLE(gl_globs->itemstore), GL_COLUMN_DONE, 
			GTK_SORT_ASCENDING);
	}
	
	return res;
}

/**
 * glista_list_init:
 *
 * Initialize the list of items and the view layer to display it.
 */
static void 
glista_list_init()
{
	GtkCellRenderer        *text_ren, *done_ren, *note_ren;
	GtkTreeViewColumn      *text_column, *done_column, *note_column;
	GtkTreeView            *treeview;
	GtkTreeSelection       *selection;
	GtkTreeDragSourceIface *dnd_siface;
	GtkTreeDragDestIface   *dnd_diface;
	GList                  *item, *all_items = NULL;
	
	treeview = GTK_TREE_VIEW(glista_get_widget("glista_item_list"));
	
	text_ren = gtk_cell_renderer_text_new();
	done_ren = gtk_cell_renderer_toggle_new();
	note_ren = gtk_cell_renderer_pixbuf_new();
	
	g_object_set(text_ren, "editable", TRUE, 
	                       "ellipsize", PANGO_ELLIPSIZE_END,
	                       NULL);
	
	// Connect the edited signal of the text column
	g_signal_connect(text_ren, "edited", G_CALLBACK(on_item_text_edited), NULL);
	g_signal_connect(text_ren, "editing-started", 
					 G_CALLBACK(on_item_text_editing_started), NULL);

	// Connect the toggled event of the "done" column to change the model
	g_signal_connect(done_ren, "toggled", 
					 G_CALLBACK(on_item_done_toggled), NULL);

	done_column = gtk_tree_view_column_new_with_attributes("Done", done_ren, 
		"active", GL_COLUMN_DONE, NULL);	
	text_column = gtk_tree_view_column_new_with_attributes("Item", text_ren, 
		"strikethrough", GL_COLUMN_DONE, NULL);
	note_column = gtk_tree_view_column_new_with_attributes("Note", note_ren, 
		NULL);
	
	gtk_tree_view_column_set_cell_data_func(text_column, text_ren, 
	                                        glista_item_text_cell_data_func,
	                                        NULL, NULL);
	
	gtk_tree_view_column_set_cell_data_func(done_column, done_ren, 
	                                        glista_item_done_cell_data_func,
	                                        NULL, NULL);
	
	gtk_tree_view_column_set_cell_data_func(note_column, note_ren, 
	                                        glista_item_note_cell_data_func,
	                                        NULL, NULL);
	
	// Set the text column to expand
	g_object_set(text_column, "expand", TRUE, NULL);
	
	gtk_tree_view_append_column(treeview, text_column);
	gtk_tree_view_append_column(treeview, note_column);
	gtk_tree_view_append_column(treeview, done_column);	
	
	gtk_tree_view_set_reorderable (treeview, TRUE);
	
	gtk_tree_view_set_model(treeview, GTK_TREE_MODEL(gl_globs->itemstore));
	
	
	// Set sort function and column
	gtk_tree_sortable_set_sort_func(GTK_TREE_SORTABLE(gl_globs->itemstore), 
				  GL_COLUMN_DONE, glista_list_sort_func,
	                                NULL, NULL);
	                                
	gtk_tree_sortable_set_sort_column_id(GTK_TREE_SORTABLE(gl_globs->itemstore),
	                                     GL_COLUMN_DONE, GTK_SORT_ASCENDING);
	                                     
	// Set selection mode and connect selection changed event
	selection = gtk_tree_view_get_selection(treeview);
	gtk_tree_selection_set_mode(selection, GTK_SELECTION_MULTIPLE);
	g_signal_connect(selection, "changed", 
	                 G_CALLBACK(on_list_selection_changed), NULL);
	
	// Set drag-and-drop interface functions
	dnd_siface = GTK_TREE_DRAG_SOURCE_GET_IFACE(GTK_TREE_MODEL(
		gl_globs->itemstore));
	dnd_diface = GTK_TREE_DRAG_DEST_GET_IFACE(GTK_TREE_MODEL(
		gl_globs->itemstore));
	
	dnd_siface->row_draggable = glista_dnd_is_draggable;
	dnd_siface->drag_data_delete = glista_dnd_delete_row;
	dnd_diface->row_drop_possible = glista_dnd_drop_possible;
	
	// Hook into the old drop handler 
	glista_dnd_old_drag_data_received = dnd_diface->drag_data_received;
	dnd_diface->drag_data_received = glista_dnd_drag_data_received;
	
	// Load data
	glista_storage_load_all_items(&all_items);
	for (item = all_items; item != NULL; item = item->next) {
		glista_list_add(item->data, FALSE);
		glista_item_free(item->data);
	}
	g_list_free(all_items);
	
	// Expand list
	gtk_tree_view_expand_all(treeview);
}

/**
 * glista_list_get_all_items:
 * @item_list: GList to populate with item
 * @parent:    The parent node, or NULL for root
 *
 * This function will iterate over the tree model, including child items, and
 * will populate a hashtable with GlistaItem values from the model.
 *
 * Returns: The pointer to the first element of the list
 */
static GList*
glista_list_get_all_items(GList *item_list, GtkTreeIter *parent)
{
	GtkTreeIter  iter;
	GlistaItem  *item;
	
	if (gtk_tree_model_iter_children(GTK_TREE_MODEL(gl_globs->itemstore),
									 &iter, parent)) {
										 
		do {
			if (gtk_tree_model_iter_has_child(
					GTK_TREE_MODEL(gl_globs->itemstore), &iter)) {
						
				item_list = glista_list_get_all_items(item_list, &iter);
				
			} else {
				item = glista_item_new(NULL, NULL);
				
				gtk_tree_model_get(GTK_TREE_MODEL(gl_globs->itemstore), &iter, 
		                       GL_COLUMN_DONE, &item->done, 
		                       GL_COLUMN_TEXT, &item->text, 
		                       GL_COLUMN_NOTE, &item->note, -1);
				
				if (parent != NULL) {
					gtk_tree_model_get(GTK_TREE_MODEL(gl_globs->itemstore), 
									   parent, 
									   GL_COLUMN_TEXT, &item->parent, -1);
				}
				
				item_list = g_list_append(item_list, item);
			}
							
		} while (gtk_tree_model_iter_next(GTK_TREE_MODEL(gl_globs->itemstore), 
    	                                  &iter));
	}
	
	return item_list;
}
					  
/**
 * glista_list_save:
 *
 * Tell the storage module to save the entire list of items. Implements a simple
 * locking mechanism. 
 * 
 * Returns: TRUE if save was successful, or FALSE if currently locked 
 * (meaning another save is in progress).
 */
static gboolean
glista_list_save()
{
	GList           *all_items = NULL;
	static gboolean  locked = FALSE;

	if (locked) {
		return FALSE;
	}
	locked = TRUE;
	
	all_items = glista_list_get_all_items(all_items, NULL);
	glista_storage_save_all_items(all_items);
    	
   	// Free items list
   	while (all_items != NULL) {
   		glista_item_free(all_items->data);
   		all_items = all_items->next;
	}
	g_list_free(all_items);
	
	locked = FALSE;
	return TRUE;
}

/**
 * glista_list_save_timeout_cb: 
 * @user_data: User data passed when timeout was created
 * 
 * Called by glista_list_save_timeout() after a timeout of X ms. Will try to 
 * save the list to storage by calling glista_list_save(). If save was not 
 * successful, will run again.		return &iter;
 *
 * Returns: FALSE if no need to run again, TRUE otherwise.
 */
static gboolean
glista_list_save_timeout_cb(gpointer user_data)
{
	if (glista_list_save()) {
		gl_globs->save_tag = 0;
		return FALSE;
	} else {
		return TRUE;
	}
}

/**
 * glista_list_save_timeout:
 *
 * Called whenever the user changes something in the data model. Will schedule 
 * an X ms timeout and call the save procedure after that time. If additional
 * save requests are recieved at that period, will postpone saving until we have
 * X ms of idle time. 
 */
void 
glista_list_save_timeout()
{
	if (gl_globs->save_tag != 0) {
		g_source_remove(gl_globs->save_tag);
	}
	
	gl_globs->save_tag = g_timeout_add(GLISTA_SAVE_TIMEOUT, 
	                                   glista_list_save_timeout_cb, NULL);
}

/**
 * glista_cfg_check_dir:
 *
 * Make sure the configuration directory exists. 
 * Returns: TRUE if directory exists or was successfuly created, or FALSE if 
 * there was an error creating it.
 */
static gboolean
glista_cfg_check_dir()
{
	if (! g_file_test(gl_globs->configdir, G_FILE_TEST_IS_DIR)) {
		if (! g_file_test(gl_globs->configdir, G_FILE_TEST_EXISTS)) {
			
			// Create the directory
			if (g_mkdir_with_parents(gl_globs->configdir, 0700) != 0) {
				// We have an error!
				g_printerr("Error creating configuration directory: %s\n",
				            strerror(errno));
				
				return FALSE;
	        }
	        
        } else { // File exists but it is not a directory
        	g_printerr("Error: unable to create config directory" 
        	           " (%s): file exists\n", gl_globs->configdir);
			
			return FALSE;
		}
	}
	
	return TRUE;
}

/**
 * glista_cfg_init_load:
 * 
 * Initialize and load the program's configuration data from file
 */
void
glista_cfg_init_load()
{
	gchar        *cfgfile;
	GKeyFile     *keyfile;
	GError       *error = NULL;
	
	// Initialize configuration stuct
	gl_globs->config = g_malloc(sizeof(GlistaConfig));
	gl_globs->config->xpos    = -1;
	gl_globs->config->ypos    = -1;
	gl_globs->config->width   = -1;
	gl_globs->config->height  = -1;
	gl_globs->config->visible = TRUE;

	cfgfile = g_build_filename(gl_globs->configdir, "glista.conf", NULL);
	
	glista_cfg_check_dir();

	// Load configuration data from file
	keyfile = g_key_file_new();
	if (g_key_file_load_from_file(keyfile, cfgfile, G_KEY_FILE_NONE, &error)) {
		gl_globs->config->visible = g_key_file_get_boolean(keyfile, 
		                                      "glistaui", "visible", NULL);
        gl_globs->config->xpos = g_key_file_get_integer(keyfile, 
                                              "glistaui", "xpos", NULL);
    	gl_globs->config->ypos = g_key_file_get_integer(keyfile, 
    	                                      "glistaui", "ypos", NULL);
		gl_globs->config->width = g_key_file_get_integer(keyfile, 
    	                                      "glistaui", "width", NULL);
		gl_globs->config->height = g_key_file_get_integer(keyfile, 
    	                                      "glistaui", "height", NULL);
	} else {
		if (error != NULL) {
			fprintf(stderr, "Error loading config file: [%d] %s\n"
			                "Using default configuration.\n", 
			                error->code, error->message);
			                
            g_error_free(error);
        }
	}
	
	g_key_file_free(keyfile);
	g_free(cfgfile);
}

/**
 * glista_cfg_save:
 *
 * Save the configuration data to file
 */
static void
glista_cfg_save()
{
	gchar     *cfgfile, *cfgdata;
	gsize      cfgdatalen;
	GKeyFile  *keyfile;
	GError    *error = NULL;

	//window = GTK_WINDOW(glista_get_widget("glista_main_window"));
	cfgfile = g_build_filename(gl_globs->configdir, "glista.conf", NULL);
	keyfile = g_key_file_new();
	
	// Set window position and width
	g_key_file_set_integer(keyfile, "glistaui", "xpos", 
						   gl_globs->config->xpos);
	g_key_file_set_integer(keyfile, "glistaui", "ypos", 
						   gl_globs->config->ypos);	
	g_key_file_set_integer(keyfile, "glistaui", "width", 
						   gl_globs->config->width);
	g_key_file_set_integer(keyfile, "glistaui", "height", 
						   gl_globs->config->height);
	g_key_file_set_boolean(keyfile, "glistaui", "visible", 
						   gl_globs->config->visible);
	
	glista_cfg_check_dir();
		
	// Save configuration file
	cfgdata = g_key_file_to_data(keyfile, &cfgdatalen, NULL);
	if (! g_file_set_contents(cfgfile, cfgdata, cfgdatalen, &error)) {
		if (error != NULL) {
			fprintf(stderr, "Error saving configuration file: [%d] %s\n",
			                error->code, error->message);
			                
            g_error_free(error);
        }
	}
	
	g_key_file_free(keyfile);
	g_free(cfgdata);
	g_free(cfgfile);
}

/**
 * main:
 * @argc: Number of arguments
 * @argv: Command line arguments
 *
 * Main program function (do I need to explain this?)
 */
int 
main(int argc, char *argv[])
{
	gboolean     no_tray = FALSE;
	gboolean     minimized = FALSE;
	GOptionEntry entries[] = {
		{ "no-tray", 'T', 0, G_OPTION_ARG_NONE, &no_tray, 
		  "Do not use the system tray (conflicts with -m)", NULL },
		{ "minimized", 'm', 0, G_OPTION_ARG_NONE, &minimized, 
		  "Start up minimized (conflicts with -T)", NULL},
		{ NULL }
	};

	g_set_prgname(PACKAGE_NAME);
	g_type_init();
	if (! g_thread_supported()) 
		g_thread_init(NULL);
	
	// Initialize globals
	gl_globs = g_malloc(sizeof(GlistaGlobals));
	gl_globs->uibuilder  = NULL;
	gl_globs->config     = NULL;
	gl_globs->open_note  = NULL;
	gl_globs->save_tag   = 0;

	// Set configuration directory name
	gl_globs->configdir  = g_build_filename(g_get_user_config_dir(),
	                                       GLISTA_CONFIG_DIR,
	                                       NULL);
	// Load configuration
	glista_cfg_init_load();
	
	// Parse commandline arguments
	gtk_init_with_args(&argc, &argv, GLISTA_PARAM_STRING, entries, NULL, NULL);
	gl_globs->trayicon = (! no_tray);

	// Initialize the UI
	if (glista_ui_init() == FALSE) {
		g_printerr("Unable to initialize UI.\n");
		return 1;	
	}
	
#ifdef HAVE_UNIQUE
	// Are we the single instance?
	if (! glista_unique_is_single_inst()) {
		// There is a Glista instance already running - shut down
		g_print("Activating an already-running Glista instance.\n");
		return 0;
	}
#endif

	// Initialize item storage model
	gl_globs->itemstore  = gtk_tree_store_new(4, 
		G_TYPE_BOOLEAN, // Done?
		G_TYPE_STRING,  // Text
		G_TYPE_BOOLEAN, // Category?
		G_TYPE_STRING   // Note
	);
	
	// Initialize categories hashtable
	gl_globs->categories = g_hash_table_new_full(g_str_hash, g_str_equal,
		(GDestroyNotify) g_free, (GDestroyNotify) gtk_tree_row_reference_free);

	// Initialize the item list
	glista_list_init();
	
	// Hook up model change signals to the data save handler
	g_signal_connect(gl_globs->itemstore, "row-changed", 
		G_CALLBACK(on_itemstore_row_changed), NULL);
	g_signal_connect(gl_globs->itemstore, "row-deleted", 
		G_CALLBACK(on_itemstore_row_deleted), NULL);
	g_signal_connect(gl_globs->itemstore, "row-inserted", 
		G_CALLBACK(on_itemstore_row_inserted), NULL);
	
	// Show the main window if needed
	if ((! gl_globs->trayicon) || 
	    (! minimized && gl_globs->config->visible)) {
		glista_ui_mainwindow_show();
	}
		
	// Run main loop
	gtk_main();
	
	// Close and store note if open
	glista_note_close();
	
	// Save list
	while (! glista_list_save());
	
	glista_ui_shutdown();
	
	// Save configuration
	glista_cfg_save();

#ifdef HAVE_UNIQUE
	// Free the Unique app object
	glista_unique_unref();
#endif
	
	// Free globals
	g_hash_table_destroy(gl_globs->categories);
	g_free(gl_globs->configdir);
	g_free(gl_globs->config);
	g_free(gl_globs);

	// Normal termination
	return 0;
}
