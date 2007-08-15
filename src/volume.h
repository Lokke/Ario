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

#ifndef __VOLUME_H
#define __VOLUME_H

#include <gtk/gtkbutton.h>
#include "mpd.h"

G_BEGIN_DECLS

#define TYPE_VOLUME         (volume_get_type ())
#define VOLUME(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), TYPE_VOLUME, Volume))
#define VOLUME_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST((k), TYPE_VOLUME, VolumeClass))
#define IS_VOLUME(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), TYPE_VOLUME))
#define IS_VOLUME_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), TYPE_VOLUME))
#define VOLUME_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), TYPE_VOLUME, VolumeClass))

typedef struct VolumePrivate VolumePrivate;

typedef struct
{
        GtkEventBox parent;

        VolumePrivate *priv;
} Volume;

typedef struct
{
        GtkEventBoxClass parent;
} VolumeClass;

GType                volume_get_type                (void);

Volume *        volume_new                (Mpd *mpd);

G_END_DECLS

#endif /* __VOLUME_H */
