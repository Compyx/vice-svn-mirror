/** \file   contentpreviewwidget.h
 * \brief   GTK3 disk/tape/archive preview widget - header
 *
 * \author  Bas Wassink <b.wassink@ziggo.nl>
 */

/*
 * This file is part of VICE, the Versatile Commodore Emulator.
 * See README for copyright notice.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
 *  02111-1307  USA.
 */

#ifndef VICE_CONTENTPREVIEWWIDGET_H
#define VICE_CONTENTPREVIEWWIDGET_H

#include "vice.h"
#include <gtk/gtk.h>

#include "imagecontents.h"

GtkWidget *content_preview_widget_create(
        GtkWidget *dialog,
        read_contents_func_type func,
        void (*response)(GtkWidget *, gint, gpointer),
        int unit);
void content_preview_widget_set_image(GtkWidget *widget, const char *path);
gboolean content_preview_widget_set_index(GtkWidget *widget, int index);
int content_preview_widget_get_index(GtkWidget *widget);

#endif
