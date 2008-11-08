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

#ifndef __GLISTA_TEXTVIEW_LINKIFY_H

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <gtk/gtk.h>

// regex matches http[s]://<hostname>[extra], or www.[subdomains.]<domain>.<tld>[extra]
#ifndef GTL_URL_REGEX
#define GTL_URL_REGEX "\\b(http[s]?://[a-zA-Z0-9]+[^ \t\r\n]*|www[.]+[A-Za-z]+[A-Za-z0-9.-]{2,}[.]+[A-Za-z0-9]{2,}[^ \t\r\n]*)"
#endif

gboolean glista_textview_linkify_init(GtkTextView *textview);

void     glista_textview_linkify_buffer_init(GtkTextBuffer *buffer);

#define __GLISTA_TEXTVIEW_LINKIFY_H
#endif
