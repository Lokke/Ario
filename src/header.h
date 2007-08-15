/*
 *  Copyright (C) 2005 Marc Pavot <marc.pavot@gmail.com>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 */

#ifndef __HEADER_H
#define __HEADER_H

#include <gtk/gtkhbox.h>
#include "mpd.h"

G_BEGIN_DECLS

#define TYPE_HEADER         (header_get_type ())
#define HEADER(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), TYPE_HEADER, Header))
#define HEADER_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST((k), TYPE_HEADER, HeaderClass))
#define IS_HEADER(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), TYPE_HEADER))
#define IS_HEADER_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), TYPE_HEADER))
#define HEADER_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), TYPE_HEADER, HeaderClass))

typedef struct HeaderPrivate HeaderPrivate;

typedef struct
{
        GtkHBox parent;

        HeaderPrivate *priv;
} Header;

typedef struct
{
        GtkHBoxClass parent;
} HeaderClass;

GType           header_get_type                 (void);

GtkWidget *     header_new                      (GtkActionGroup *group, Mpd *mpd);

void            header_do_next                  (Header *header);

void            header_do_previous              (Header *header);

void            header_playpause                (Header *header);

void            header_stop                     (Header *header);


G_END_DECLS

#endif /* __HEADER_H */
