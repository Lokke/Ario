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

#ifndef __ARIO_MPD_H
#define __ARIO_MPD_H

#include <glib-object.h>
#include "libmpdclient.h"

G_BEGIN_DECLS

#define ARIO_MPD_UNKNOWN     _("Unknown")

#define TYPE_ARIO_MPD         (ario_mpd_get_type ())
#define ARIO_MPD(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), TYPE_ARIO_MPD, ArioMpd))
#define ARIO_MPD_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST((k), TYPE_ARIO_MPD, ArioMpdClass))
#define IS_ARIO_MPD(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), TYPE_ARIO_MPD))
#define IS_ARIO_MPD_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), TYPE_ARIO_MPD))
#define ARIO_MPD_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), TYPE_ARIO_MPD, ArioMpdClass))

typedef struct ArioMpdPrivate ArioMpdPrivate;

typedef struct
{
        GObject parent;

        ArioMpdPrivate *priv;
} ArioMpd;

typedef struct ArioMpdAlbum
{
        gchar *artist;
        gchar *album;
} ArioMpdAlbum;

typedef struct
{
        GObjectClass parent;

        void (*song_changed)            (ArioMpd *mpd);

        void (*state_changed)           (ArioMpd *mpd);

        void (*volume_changed)     (ArioMpd *mpd);

        void (*elapsed_changed)         (ArioMpd *mpd);

        void (*playlist_changed)        (ArioMpd *mpd);

        void (*random_changed)          (ArioMpd *mpd);

        void (*repeat_changed)          (ArioMpd *mpd);

        void (*dbtime_changed)          (ArioMpd *mpd);
} ArioMpdClass;

GType                   ario_mpd_get_type                               (void);

ArioMpd *               ario_mpd_new                                    (void);

void                    ario_mpd_connect                                (ArioMpd *mpd);

void                    ario_mpd_disconnect                             (ArioMpd *mpd);

gboolean                ario_mpd_is_connected                           (ArioMpd *mpd);

gboolean                ario_mpd_update_status                          (ArioMpd *mpd);

void                    ario_mpd_update_db                              (ArioMpd *mpd);

GList *                 ario_mpd_get_artists                            (ArioMpd *mpd);

GList *                 ario_mpd_get_albums                             (ArioMpd *mpd,
                                                                         const char *artist);
GList *                 ario_mpd_get_songs                              (ArioMpd *mpd,
                                                                         const char *artist,
                                                                         const char *album);
mpd_Connection *        ario_mpd_get_connection                         (ArioMpd *mpd);

char *                  ario_mpd_get_current_title                      (ArioMpd *mpd);

char *                  ario_mpd_get_current_artist                     (ArioMpd *mpd);

char *                  ario_mpd_get_current_album                      (ArioMpd *mpd);

int                     ario_mpd_get_current_song_id                    (ArioMpd *mpd);

int                     ario_mpd_get_current_state                      (ArioMpd *mpd);

int                     ario_mpd_get_current_elapsed                    (ArioMpd *mpd);

int                     ario_mpd_get_current_volume                     (ArioMpd *mpd);

int                     ario_mpd_get_current_total_time                 (ArioMpd *mpd);

int                     ario_mpd_get_current_playlist_id                (ArioMpd *mpd);

int                     ario_mpd_get_current_playlist_length            (ArioMpd *mpd);

int                     ario_mpd_get_current_playlist_total_time        (ArioMpd *mpd);

int                     ario_mpd_get_crossfadetime                      (ArioMpd *mpd);

gboolean                ario_mpd_get_current_random                     (ArioMpd *mpd);

gboolean                ario_mpd_get_current_repeat                     (ArioMpd *mpd);

gboolean                ario_mpd_get_updating                           (ArioMpd *mpd);

unsigned long           ario_mpd_get_last_update                        (ArioMpd *mpd);

void                    ario_mpd_set_current_elapsed                    (ArioMpd *mpd,
                                                                         gint elapsed);
void                    ario_mpd_set_current_volume                     (ArioMpd *mpd,
                                                                         gint volume);
void                    ario_mpd_set_current_random                     (ArioMpd *mpd,
                                                                         gboolean random);
void                    ario_mpd_set_current_repeat                     (ArioMpd *mpd,
                                                                         gboolean repeat);
void                    ario_mpd_set_crossfadetime                      (ArioMpd *mpd,
                                                                         int crossfadetime);
gboolean                ario_mpd_is_paused                              (ArioMpd *mpd);

void                    ario_mpd_do_next                                (ArioMpd *mpd);

void                    ario_mpd_do_prev                                (ArioMpd *mpd);

void                    ario_mpd_do_play                                (ArioMpd *mpd);

void                    ario_mpd_do_play_id                             (ArioMpd *mpd,
                                                                         gint id);
void                    ario_mpd_do_pause                               (ArioMpd *mpd);

void                    ario_mpd_do_stop                                (ArioMpd *mpd);

void                    ario_mpd_free_album                             (ArioMpdAlbum *ario_mpd_album);

void                    ario_mpd_clear                                  (ArioMpd *mpd);

void                    ario_mpd_remove                                 (ArioMpd *mpd,
                                                                         GArray *songs);

G_END_DECLS

#endif /* __ARIO_MPD_H */
