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
#include <libxml/xmlreader.h>
#include <libxml/xmlwriter.h>
#include <string.h>

#include "glista-storage.h"
#include "glista.h"

/**
 * Glista Storage Module
 * 
 * Responsible for persistant storage of the items in list. For now we store 
 * data on disk - but in the future might offer more than one storage module 
 * (yeah right ;) )
 */

/**
 * read_next_text_node:
 * @xml XML reader
 * 
 * Read the text value of the next node, assuming that it is a text node. If no
 * text node value is encountered before the tag ends, will return NULL
 * 
 * Returns: text read from node value or NULL if none
 */
static gchar*
read_next_text_node(xmlTextReaderPtr xml)
{
	gchar    *text = NULL;
	gboolean  text_done = FALSE;
	
	while ((! text_done) && xmlTextReaderRead(xml) == 1) {
		switch(xmlTextReaderNodeType(xml)) {
			case 3: // Text node, read value
				text = (gchar *) xmlTextReaderValue(xml);
				g_strstrip(text);
				text_done = TRUE;
				break;
				
			case 15: // End node - no text
				text_done = TRUE;
				break;
		}
	}
	
	return text;
}

/**
 * read_next_item: 
 * @xml XML reader
 * 
 * Read the next item from the XML file and populate it's properties
 * 
 * Returns: a newly created GlistaItem or NULL if nothing more to read
 */
static GlistaItem*
read_next_item(xmlTextReaderPtr xml) 
{
	gchar      *text, *done, *parent, *note;
	xmlChar    *node_name;
	gboolean    item_done;
	GlistaItem *item;
	
	item   = NULL;
	text   = NULL;
	done   = NULL;
	parent = NULL;
	note   = NULL;
	
	item_done = FALSE;
	
	while ((! item_done) && xmlTextReaderRead(xml) == 1) {
		node_name = xmlTextReaderName(xml);
		
		if (xmlStrEqual(node_name, BAD_CAST GL_XNODE_ITEM) && 
		    xmlTextReaderNodeType(xml) == 1) {
		    
		    // Create item
		    item = glista_item_new(NULL, NULL);
		    
		    // Read item data
		    while ((! item_done) && xmlTextReaderRead(xml) == 1) {
		    	xmlFree(node_name);		    		
		    	node_name = xmlTextReaderName(xml);
				
				// Node text
				if (xmlStrEqual(node_name, BAD_CAST GL_XNODE_TEXT)) {
					if (text == NULL) {
						text = read_next_text_node(xml);
					}
					
				} else 
				
				// Node 'done' flag
				if (xmlStrEqual(node_name, BAD_CAST GL_XNODE_DONE)) {
					if (done == NULL) {
						done = read_next_text_node(xml);
					}
					
				} else 
				
				// Node parent
				if (xmlStrEqual(node_name, BAD_CAST GL_XNODE_PRNT)) {
					if (parent == NULL) {
						parent = read_next_text_node(xml);
					}
					
				} else 
				
				// Node item note
				if (xmlStrEqual(node_name, BAD_CAST GL_XNODE_NOTE)) {
					if (note == NULL) {
						note = read_next_text_node(xml);
					}
					
				} else 
				
				if (xmlStrEqual(node_name, BAD_CAST GL_XNODE_ITEM) && 
				    xmlTextReaderNodeType(xml) == 15) {
				    	
				    item_done = TRUE;
				}
			}
			
			// Check that we have text and set it
			if (text != NULL) {
				if (strlen(text) > 0) {
					item->text = text;
				} else {
					 g_free(text);
				}
			}
			
			// Set "done" flag
			if (done != NULL) {
				if (*done == '1') {
					item->done = TRUE;	
				}
				g_free(done);
			}
				
			
			// Set the parent if any
			if (parent != NULL) {
				if (strlen(parent) > 0) {
					item->parent = parent;
				} else {
					 g_free(parent);
				}
			}
			
			// Set the note if any
			if (note != NULL) {
				if (strlen(note) > 0) {
					item->note = note; 
				} else {
					 g_free(note);
				}
			}
		}
		
		xmlFree(node_name);
	}

	return item;
}

/**
 * glista_storage_get_all_items:
 * @list: Pointer to a GList* to populate with GlistaItem objects
 *
 * Load all items from storage into a linked-list, which will be in turn used
 * to load the data into the GtkListStore of the UI.
 */
void
glista_storage_load_all_items(GList **list)
{
	xmlTextReaderPtr  xml;
	gchar            *storage_file;
	xmlChar          *node_name;
	GlistaItem       *item;
	
	// Build storage file path
	storage_file = g_build_filename(gl_globs->configdir, GL_XML_FILENAME, NULL);
	
	// Open XML file
	if ((xml = xmlReaderForFile(storage_file, GL_XML_ENCODING, 0)) != NULL) {
		
		// Read the XML root node
		if (xmlTextReaderRead(xml) == 1) {
			node_name = xmlTextReaderName(xml);
			
			if (xmlStrEqual(node_name, BAD_CAST GL_XNODE_ROOT)) {
				
				// Read all items 
				while ((item = read_next_item(xml)) != NULL) {
					if (item->text != NULL) {
						*list = g_list_append(*list, item);
					} else {
						glista_item_free(item);
					}
				}
				
			} else {
				g_warning("Invalid XML file: unexpected root element '%s'\n", 
				           node_name);
			}
			
			xmlFree(node_name);
			
		} else {
			g_warning("Invalid XML file: unable to read root element\n");
		}
		
		xmlFreeTextReader(xml);
	}
	
	g_free(storage_file);
}

/**
 * glista_storage_save_all_items: 
 * @all_items: A linked list of all items to save
 * 
 * Save all items to the storage XML file
 */
void 
glista_storage_save_all_items(GList *all_items)
{
	GlistaItem       *item;
	xmlTextWriterPtr  xml;
	int               ret;
	gchar            *storage_file;
	gchar             done_str[2];
	
	// Build storage file path
	storage_file = g_build_filename(gl_globs->configdir, 
									GL_XML_FILENAME, NULL);
	
	// Start XML
	xml = xmlNewTextWriterFilename(storage_file, 0);
	g_free(storage_file);
	
	if (xml == NULL) {
		fprintf(stderr, "Unable to write data to storage XML file\n");
		return;
	}
	
	xmlTextWriterSetIndent(xml, 1);
	xmlTextWriterSetIndentString(xml, BAD_CAST "  ");
	
	ret = xmlTextWriterStartDocument(xml, NULL, GL_XML_ENCODING, "yes");
	ret = xmlTextWriterStartElement(xml, BAD_CAST "glista");

	// Iterate over items, writing them to the XML file
	while (all_items != NULL) {
		item = all_items->data;
		
		g_snprintf((gchar *) &done_str, 2, "%d", item->done);
		
		ret = xmlTextWriterStartElement(xml, BAD_CAST "item");
		ret = xmlTextWriterWriteElement(xml, BAD_CAST "text", 
										BAD_CAST item->text);
		ret = xmlTextWriterWriteElement(xml, BAD_CAST "done", 
										BAD_CAST &done_str);
		
		if (item->parent != NULL) {
			ret = xmlTextWriterWriteElement(xml, BAD_CAST "parent", 
											BAD_CAST item->parent);
		}
		
		if (item->note != NULL) {
			ret = xmlTextWriterWriteElement(xml, BAD_CAST "note", 
			                                BAD_CAST item->note);
		}
		
		ret = xmlTextWriterEndElement(xml);
		
		all_items = all_items->next;
	}
	
	// End XML
	ret = xmlTextWriterEndElement(xml);
	ret = xmlTextWriterEndDocument(xml);
	
	xmlTextWriterFlush(xml);
	xmlFreeTextWriter(xml);
}
