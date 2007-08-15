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

#ifndef __PLAYLIST_H
#define __PLAYLIST_H

#include <gtk/gtkhbox.h>
#include "mpd.h"

G_BEGIN_DECLS

#define TYPE_PLAYLIST         (playlist_get_type ())
#define PLAYLIST(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), TYPE_PLAYLIST, Playlist))
#define PLAYLIST_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST((k), TYPE_PLAYLIST, PlaylistClass))
#define IS_PLAYLIST(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), TYPE_PLAYLIST))
#define IS_PLAYLIST_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), TYPE_PLAYLIST))
#define PLAYLIST_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), TYPE_PLAYLIST, PlaylistClass))

typedef struct PlaylistPrivate PlaylistPrivate;

typedef struct
{
        GtkHBox parent;

        PlaylistPrivate *priv;
} Playlist;

typedef struct
{
        GtkHBoxClass parent;
} PlaylistClass;

GType                        playlist_get_type                                (void);

GtkWidget *                 playlist_new                                        (GtkUIManager *mgr,
                                                                         GtkActionGroup *group,
                                                                         Mpd *mpd);

void                        playlist_append_artists                                (Playlist *playlist, GList *artists);

void                        playlist_append_albums                                (Playlist *playlist, GList *albums);

void                        playlist_append_songs                                (Playlist *playlist, GList *songs);

G_END_DECLS

#endif /* __PLAYLIST_H */
