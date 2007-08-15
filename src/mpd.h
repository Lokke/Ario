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

#ifndef __MPD_H
#define __MPD_H

#include <glib-object.h>
#include "libmpdclient.h"

G_BEGIN_DECLS

#define MPD_UNKNOWN     _("Unknown")

#define TYPE_MPD         (mpd_get_type ())
#define MPD(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), TYPE_MPD, Mpd))
#define MPD_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST((k), TYPE_MPD, MpdClass))
#define IS_MPD(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), TYPE_MPD))
#define IS_MPD_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), TYPE_MPD))
#define MPD_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), TYPE_MPD, MpdClass))

typedef struct MpdPrivate MpdPrivate;

typedef struct
{
        GObject parent;

        MpdPrivate *priv;
} Mpd;

typedef struct Mpd_album
{
        gchar *artist;
        gchar *album;
} Mpd_album;

typedef struct
{
        GObjectClass parent;

        void (*song_changed)                (Mpd *mpd);

        void (*state_changed)                (Mpd *mpd);

        void (*volume_changed)                (Mpd *mpd);

        void (*elapsed_changed)                (Mpd *mpd);

        void (*playlist_changed)        (Mpd *mpd);

        void (*random_changed)                (Mpd *mpd);

        void (*repeat_changed)                (Mpd *mpd);

        void (*dbtime_changed)                (Mpd *mpd);
} MpdClass;

GType                        mpd_get_type                                (void);

Mpd *                         mpd_new                                        (void);

void                        mpd_connect                                (Mpd *mpd);

void                        mpd_disconnect                                (Mpd *mpd);

gboolean                mpd_is_connected                        (Mpd *mpd);

gboolean                mpd_update_status                         (Mpd *mpd);

void                        mpd_update_db                                (Mpd *mpd);

GList *                        mpd_get_artists                                (Mpd *mpd);

GList *                        mpd_get_albums                                (Mpd *mpd, const char *artist);

GList *                        mpd_get_songs                                (Mpd *mpd, const char *artist, const char *album);

mpd_Connection *        mpd_get_connection                        (Mpd *mpd);

char *                        mpd_get_current_title                        (Mpd *mpd);

char *                        mpd_get_current_artist                        (Mpd *mpd);

char *                        mpd_get_current_album                        (Mpd *mpd);

int                        mpd_get_current_song_id                        (Mpd *mpd);

int                        mpd_get_current_state                        (Mpd *mpd);

int                        mpd_get_current_elapsed                        (Mpd *mpd);

int                        mpd_get_current_volume                        (Mpd *mpd);

int                        mpd_get_current_total_time                (Mpd *mpd);

int                        mpd_get_current_playlist_id                (Mpd *mpd);

int                        mpd_get_current_playlist_length                (Mpd *mpd);

int                        mpd_get_current_playlist_total_time        (Mpd *mpd);

int                        mpd_get_crossfadetime                        (Mpd *mpd);

gboolean                mpd_get_current_random                        (Mpd *mpd);

gboolean                mpd_get_current_repeat                        (Mpd *mpd);

gboolean                mpd_get_updating                        (Mpd *mpd);

unsigned long                mpd_get_last_update                        (Mpd *mpd);

void                        mpd_set_current_elapsed                        (Mpd *mpd, gint elapsed);

void                        mpd_set_current_volume                        (Mpd *mpd, gint volume);

void                        mpd_set_current_random                        (Mpd *mpd, gboolean random);

void                        mpd_set_current_repeat                        (Mpd *mpd, gboolean repeat);

void                        mpd_set_crossfadetime                        (Mpd *mpd, int crossfadetime);

gboolean                mpd_is_paused                                (Mpd *mpd);

void                        mpd_do_next                                (Mpd *mpd);

void                        mpd_do_prev                                (Mpd *mpd);

void                        mpd_do_play                                (Mpd *mpd);

void                        mpd_do_play_id                                (Mpd *mpd, gint id);

void                        mpd_do_pause                                (Mpd *mpd);

void                        mpd_do_stop                                (Mpd *mpd);

void                        mpd_free_album                                (Mpd_album *mpd_album);

void                        mpd_clear                                (Mpd *mpd);

void                        mpd_remove                                (Mpd *mpd, GArray *songs);

G_END_DECLS

#endif /* __MPD_H */
