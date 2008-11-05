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

#ifndef __GLISTA_UNIQUE_H
#ifdef HAVE_UNIQUE

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifndef GLISTA_UNIQUE_ID
#define GLISTA_UNIQUE_ID "org.prematureoptimization.Glista"
#endif

// Function prototypes

void     glista_unique_unref();

gboolean glista_unique_is_single_inst();

#endif
#define __GLISTA_UNIQUE_H
#endif
