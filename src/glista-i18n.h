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
 * Glista I18N support header file
 * Must be included by all files that need i18n / l10n support
 */

#ifndef __GLISTA_I18N_H

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif 

#ifdef ENABLE_NLS

#include <libintl.h>
#include <locale.h>

#define _(string) gettext(string)

#ifdef gettext_noop
#define N_(string) gettext_noop(string)
#else
#define N_(string) (string)
#endif

#else /* NLS is disabled */

#define _(string) (string)
#define N_(string) (string)
#define textdomain(string) (string)
#define gettext(string) (string)
#define dgettext(domain, string) (string)
#define dcgettext(domain, string, type) (string)
#define bindtextdomain(domain, directory) (domain) 
#define bind_textdomain_codeset(domain, codeset) (codeset)

#endif /* ENABLE_NLS */

#define __GLISTA_I18N_H
#endif
