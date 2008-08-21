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

#include <libxml/xmlreader.h>
#include <libxml/xmlwriter.h>
#include "glista.h"

/**
 * Glista Storage Module
 * 
 * Responsible for persistant storage of the items in list. For now we store 
 * data on disk - but in the future might offer more than one storage module 
 * (yeah right ;) )
 */

#define GL_XML_ENCODING "UTF-8"
#define GL_XML_FILENAME "itemstore.xml"

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
	GlistaItem       *item = NULL;
	gchar            *storage_file;
	xmlChar          *text, *done;
	xmlTextReaderPtr  xml;
	int               ret;

	// Build storage file path
	storage_file = g_build_filename(gl_globs->configdir, GL_XML_FILENAME, NULL);
	
	if ((xml = xmlReaderForFile(storage_file, GL_XML_ENCODING, 0)) != NULL) {
		// Process the XML
		// Read the root node
		ret = xmlTextReaderRead(xml);
		while (ret == 1) {
			ret = xmlTextReaderRead(xml);
			
			// Process items
			while (xmlStrEqual(xmlTextReaderName(xml), BAD_CAST "item")) {
				item = glista_item_new(NULL);
				
				// Process item properties
				while (ret == 1) {
					ret = xmlTextReaderRead(xml);
										
					if (xmlStrEqual(xmlTextReaderName(xml), BAD_CAST "text")) {
						ret = xmlTextReaderRead(xml);
						if (xmlTextReaderNodeType(xml) == 3) { // Text node
							// Read the item from XML
							text = xmlTextReaderValue(xml);
							if (strlen(g_strstrip((gchar *) text)) > 0) {
								item->text = (gchar *) text;
							}
						}
						
					} else if (xmlStrEqual(xmlTextReaderName(xml), 
										   BAD_CAST "done")) {
											   
						ret = xmlTextReaderRead(xml);
						if (xmlTextReaderNodeType(xml) == 3) { // Text node
							// Read the item from XML
							done = xmlTextReaderValue(xml);
							if (xmlStrEqual(BAD_CAST g_strstrip((gchar *) done),
											BAD_CAST "1")) {
												
								item->done = TRUE;
							}
							xmlFree(done);
						}
					} else if (xmlStrEqual(xmlTextReaderName(xml), 
										   BAD_CAST "item")) {
						break;
					}
				}
				
				if (item->text != NULL) {
					*list = g_list_append(*list, item);
				} else {
					glista_item_free(item);
				}
				
				ret = xmlTextReaderRead(xml);
			}	
		}
		
		xmlFreeTextReader(xml);
	}
	
	g_free(storage_file);
}

void 
glista_storage_save_all_items(GList *all_items)
{
	GlistaItem       *item;
	xmlTextWriterPtr  xml;
	int               ret;
	gchar            *storage_file;
	gchar             done_str[2];
	
	// Make sure we actually have something to write	
	if (all_items != NULL) {

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
		do {
			item = all_items->data;
			
			g_snprintf((gchar *) &done_str, 2, "%d", item->done);
			
			ret = xmlTextWriterStartElement(xml, BAD_CAST "item");
			ret = xmlTextWriterWriteElement(xml, BAD_CAST "text", 
				                            BAD_CAST item->text);
			ret = xmlTextWriterWriteElement(xml, BAD_CAST "done", 
			                                BAD_CAST &done_str);
			ret = xmlTextWriterEndElement(xml);
			
		} while ((all_items = all_items->next) != NULL);
		
		// End XML
		ret = xmlTextWriterEndElement(xml);
		ret = xmlTextWriterEndDocument(xml);
		
		xmlTextWriterFlush(xml);
		xmlFreeTextWriter(xml);
	}
}

