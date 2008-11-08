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
 * Credit for most of the code here goes to jcinacio
 */

#include <gtk/gtk.h>
#include <regex.h>
#include "glista-textview-linkify.h"

/**
 * URL regex: compiled on glista_textview_linkify_init()
 */
static regex_t url_re;

/**
 * glista_open_url:
 * @url The URL to open
 * 
 * Will try to cause the system to opena URL. Currently based on executing
 * a list of external programs which should be available.
 *  
 * Returns: TRUE on success, FALSE otherwise
 */
static gboolean
glista_open_url(gchar *url)
{
	/**
	 * FIXME: Need to add a default 'http://' prefix to URLs starting with
	 * 'www.' because the system can't open them!
	 */
	
	// Try opening the URL by calling these programs:
	const gchar *xdg_open_argv[]   = { "xdg-open", url, NULL };
	const gchar *gnome_open_argv[] = { "gnome-open", url, NULL };
	const gchar *kfmclient_argv[]  = { "kfmclient", "exec", url, NULL };

	if (g_spawn_async(NULL, (gchar **)xdg_open_argv, NULL,
		G_SPAWN_SEARCH_PATH, NULL, NULL, NULL, NULL)) {
		return TRUE;
	}
	
	if (g_spawn_async(NULL, (gchar **)gnome_open_argv, NULL,
		G_SPAWN_SEARCH_PATH, NULL, NULL, NULL, NULL)) {
		return TRUE;
	}
	
	if (g_spawn_async(NULL, (gchar **)kfmclient_argv, NULL,
		G_SPAWN_SEARCH_PATH, NULL, NULL, NULL, NULL)) {
		return TRUE;
	}

	return FALSE;
}

/**
 * on_note_motion_even:
 * @widget		a note TextView
 * @event		generated motion event object (mouse move)
 * @user_data	User data bound at signal connect time
 *
 * Handle changing the cursor to a hand when the mouse is over a "link" tag.
 */
static gboolean
on_motion_event(GtkWidget *widget, GdkEventMotion *event,
					 gpointer user_data)
{
	GSList          *tag_list;
	gint             x, y;
	GtkTextIter      iter;
	GdkModifierType  state;
	GtkTextTag      *tag = NULL;

	g_assert(GTK_IS_TEXT_VIEW(widget));
	
	gdk_window_get_pointer(event->window, &x, &y, &state);
	gtk_text_view_get_iter_at_location(GTK_TEXT_VIEW(widget), &iter, x, y);
	tag_list = gtk_text_iter_get_tags(&iter);

	while (tag_list != NULL) {
		if (g_object_get_data(G_OBJECT(tag_list->data), "is_link") != NULL) {
			tag = (GtkTextTag *)tag_list->data;
			break;
		}
		tag_list = tag_list->next;
	}
	g_slist_free(tag_list);

	if (tag != NULL) {
		GdkCursor *cursor = gdk_cursor_new(GDK_HAND2);
		gdk_window_set_cursor (event->window, cursor);
		gdk_cursor_unref (cursor);
	} else {
		GdkCursor *cursor = gdk_cursor_new(GDK_XTERM);
		gdk_window_set_cursor (event->window, cursor);
		gdk_cursor_unref (cursor);
	}
	
	return FALSE;
}

/**
 * on_note_link_tag_event:
 * @tag			the link tag
 * @object		the object the event was fired from (the GtkTextView)
 * @event		the event which triggered the signal
 * @iter		a GtkTextIter pointing at the location the event occured
 * @user_data	User data bound at signal connect time
 *
 * Handle clicking on a link tag - open url in browser.
 */
static gboolean
on_link_tag_event(GtkTextTag *tag, GObject *object, GdkEvent *event,
					   GtkTextIter *iter, gpointer user_data)
{
	if (g_object_get_data(G_OBJECT(tag), "is_link") != (void *)1) {
		return FALSE;
	}

	GtkTextIter start = *iter;
	GtkTextIter end = *iter;
	gchar *url;

	if (event->type == GDK_BUTTON_PRESS) {
		// get tag bounds in start..end iter
		gtk_text_iter_backward_to_tag_toggle(&start, tag);
		gtk_text_iter_forward_to_tag_toggle(&end, tag);

		// url = tag text
		url = gtk_text_iter_get_text(&start, &end);

		// open in browser
		if (glista_open_url(url) == FALSE) {
			g_printerr("Couldn't ask system to handle URL: %s\n", url);
		}
		
		g_free(url);
		return TRUE;
	}
	
	return FALSE;
}

/**
 * glista_note_linkify:
 * @note_buffer		the note's GtkTextBuffer
 *
 * Parse the the note text for urls and apply "link" tags to all matches
 */
static void
glista_note_linkify(GtkTextBuffer *note_buffer)
{
	gchar       *text;
	GtkTextTag 	*tag;
	GtkTextIter  start, end;
	regmatch_t   match, newmatch;

	// get the full text from the buffer
	gtk_text_buffer_get_bounds(note_buffer, &start, &end);
	text = gtk_text_buffer_get_text(note_buffer, &start, &end, FALSE);

	// clear all tags
	/* FIXME: remove only "link" tags */
	gtk_text_buffer_remove_all_tags(note_buffer, &start, &end);

	int error = regexec(&url_re, text, 1, &match, 0);
	while (error == 0) { // while we have a match
		// create a new styled tag
		tag = gtk_text_buffer_create_tag(note_buffer, NULL,
					"foreground", "blue",
					"underline", PANGO_UNDERLINE_SINGLE,
					NULL);
		
		// mark the tag as a link
		g_object_set_data (G_OBJECT(tag), "is_link", (void *)1);

		// connect the tag "event" handler to handle clicks
		g_signal_connect(tag, "event", G_CALLBACK(on_link_tag_event), NULL);

		// get start/end iters from the regex match offsets
		gtk_text_buffer_get_iter_at_offset(note_buffer, &start, match.rm_so);
		gtk_text_buffer_get_iter_at_offset(note_buffer, &end, match.rm_eo);

		// apply tag to buffer
		gtk_text_buffer_apply_tag(note_buffer, tag, &start, &end);

    	// next match
    	error = regexec(&url_re, text + match.rm_eo+1, 1, &newmatch, 0);
		match.rm_so = newmatch.rm_so + match.rm_eo+1;
		match.rm_eo = newmatch.rm_eo + match.rm_eo+1;
	}

	g_free(text);
}

/**
 * on_after_insert_text: 
 * @textbuffer The modified text buffer
 * @location   The insert location
 * @text       Inserted text
 * @len        Inserted text length
 * @user_data  User data bound at connect time
 * 
 * Handle text insertion - will call linkify() to make sure any modified text
 * is properly linkified. 
 */
static void
on_after_insert_text(GtkTextBuffer *textbuffer, GtkTextIter *location,
                     gchar *text, gint len, gpointer user_data)
{
	glista_note_linkify(textbuffer);
}

/**
 * on_after_delete_range: 
 * @textbuffer The modified text buffer
 * @start      Modification start point
 * @end        Modification end point
 * @user_data  User data bound at connect time
 * 
 * Handle text deletion - will call linkify() to make sure any modified text
 * is properly linkified. 
 */
static void
on_after_delete_range(GtkTextBuffer *textbuffer, GtkTextIter *start,
                      GtkTextIter *end, gpointer user_data)
{
	glista_note_linkify(textbuffer);
}

/**
 * glista_textview_linkify_buffer_init:
 * @buffer The GtkTextBuffer to initialize
 * 
 * Initialize URL "linkification" on the provided text buffer. Must be called
 * when the text view linkification is initialized, and can also be called 
 * whenever a new text buffer is set on a linkified textview.
 */
void
glista_textview_linkify_buffer_init(GtkTextBuffer *buffer)
{
	g_signal_connect_after(buffer, "insert-text", 
	                       G_CALLBACK(on_after_insert_text), NULL);
	                       
	g_signal_connect_after(buffer, "delete-range",
						   G_CALLBACK(on_after_delete_range), NULL);
}

/**
 * glista_textview_linkify_init:
 * @textview The GtkTextView to linkify
 * 
 * Initialize URL "linkification" on a provided GtkTextView. Will bind all
 * needed callbacks to the text view, and will also call 
 * glista_textview_linkify_buffer_init() to make sure the default buffer is
 * initialized.
 * 
 * Returns: TRUE on success, FALSE otherwise. 
 */
gboolean
glista_textview_linkify_init(GtkTextView *textview)
{
	GtkTextBuffer *textbuffer;
	
	// compile the regex for url matching
	if (regcomp(&url_re, GTL_URL_REGEX, REG_EXTENDED | REG_ICASE) != 0) {
		g_printerr("linkify: error compiling URL regex\n");
		return FALSE;
	}
	
	// connect "motion" event on text view, to change pointer over link tags
	g_signal_connect(textview, "motion-notify-event",
					 G_CALLBACK(on_motion_event), NULL);

	// Initialize the connected buffer
	textbuffer = gtk_text_view_get_buffer(textview);
	glista_textview_linkify_buffer_init(textbuffer);
	
	return TRUE;
}
