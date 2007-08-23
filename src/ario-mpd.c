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

#include <gtk/gtk.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <limits.h>
#include "eel-gconf-extensions.h"
#include "ario-mpd.h"
#include "ario-preferences.h"
#include "ario-i18n.h"
#include "ario-debug.h"

static void ario_mpd_class_init (ArioMpdClass *klass);
static void ario_mpd_init (ArioMpd *mpd);
static void ario_mpd_finalize (GObject *object);
static void ario_mpd_set_default (ArioMpd *mpd);
void ario_mpd_check_errors (ArioMpd *mpd);
static void ario_mpd_set_property (GObject *object,
                                   guint prop_id,
                                   const GValue *value,
                                   GParamSpec *pspec);
static void ario_mpd_get_property (GObject *object,
                                   guint prop_id,
                                   GValue *value,
                                   GParamSpec *pspec);

struct ArioMpdPrivate
{        
        mpd_Status *status;
        mpd_Connection *connection;
        mpd_Stats *stats;

        int song_id;
        int state;
        int volume;
        int elapsed;

        int total_time;

        mpd_Song *ario_mpd_song;
        int playlist_id;
        int playlist_length;

        gboolean random;
        gboolean repeat;

        unsigned long dbtime;
};

enum
{
        PROP_0,
        PROP_SONGID,
        PROP_STATE,
        PROP_VOLUME,
        PROP_ELAPSED,
        PROP_PLAYLISTID,
        PROP_RANDOM,
        PROP_DBTIME,
        PROP_REPEAT
};

enum
{
        SONG_CHANGED,
        STATE_CHANGED,
        VOLUME_CHANGED,
        ELAPSED_CHANGED,
        PLAYLIST_CHANGED,
        RANDOM_CHANGED,
        REPEAT_CHANGED,
        DBTIME_CHANGED,
        LAST_SIGNAL
};

static guint ario_mpd_signals[LAST_SIGNAL] = { 0 };

static GObjectClass *parent_class = NULL;

GType
ario_mpd_get_type (void)
{
        ARIO_LOG_FUNCTION_START
        static GType type = 0;

        if (!type) {
                static const GTypeInfo our_info =
                {
                        sizeof (ArioMpdClass),
                        NULL,
                        NULL,
                        (GClassInitFunc) ario_mpd_class_init,
                        NULL,
                        NULL,
                        sizeof (ArioMpd),
                        0,
                        (GInstanceInitFunc) ario_mpd_init
                };

                type = g_type_register_static (G_TYPE_OBJECT,
                                               "ArioMpd",
                                                &our_info, 0);
        }
        return type;
}

static void
ario_mpd_class_init (ArioMpdClass *klass)
{
        ARIO_LOG_FUNCTION_START
        GObjectClass *object_class = G_OBJECT_CLASS (klass);

        parent_class = g_type_class_peek_parent (klass);

        object_class->finalize = ario_mpd_finalize;

        object_class->set_property = ario_mpd_set_property;
        object_class->get_property = ario_mpd_get_property;

        g_object_class_install_property (object_class,
                                         PROP_SONGID,
                                         g_param_spec_int ("song_id",
                                                           "song_id",
                                                           "song_id",
                                                           0, INT_MAX, 0,
                                                           G_PARAM_READWRITE));

        g_object_class_install_property (object_class,
                                         PROP_STATE,
                                         g_param_spec_int ("state",
                                                           "state",
                                                           "state",
                                                           0, 3, 0,
                                                           G_PARAM_READWRITE));

        g_object_class_install_property (object_class,
                                         PROP_VOLUME,
                                         g_param_spec_int ("volume",
                                                           "volume",
                                                           "volume",
                                                           -1, 100, 0,
                                                           G_PARAM_READWRITE));

        g_object_class_install_property (object_class,
                                         PROP_ELAPSED,
                                         g_param_spec_int ("elapsed",
                                                           "elapsed",
                                                           "elapsed",
                                                           0, INT_MAX, 0,
                                                           G_PARAM_READWRITE));

        g_object_class_install_property (object_class,
                                         PROP_PLAYLISTID,
                                         g_param_spec_int ("playlist_id",
                                                           "playlist_id",
                                                           "playlist_id",
                                                           -1, INT_MAX, 0,
                                                           G_PARAM_READWRITE));

        g_object_class_install_property (object_class,
                                         PROP_RANDOM,
                                         g_param_spec_boolean ("random",
                                                               "random",
                                                               "random",
                                                               FALSE,
                                                               G_PARAM_READWRITE));

        g_object_class_install_property (object_class,
                                         PROP_REPEAT,
                                         g_param_spec_boolean ("repeat",
                                                               "repeat",
                                                               "repeat",
                                                               FALSE,
                                                               G_PARAM_READWRITE));

        g_object_class_install_property (object_class,
                                         PROP_DBTIME,
                                         g_param_spec_ulong ("dbtime",
                                                             "dbtime",
                                                             "dbtime",
                                                             0, ULONG_MAX, 0,
                                                             G_PARAM_READWRITE));

        ario_mpd_signals[SONG_CHANGED] =
                g_signal_new ("song_changed",
                              G_OBJECT_CLASS_TYPE (object_class),
                              G_SIGNAL_RUN_LAST,
                              G_STRUCT_OFFSET (ArioMpdClass, song_changed),
                              NULL, NULL,
                              g_cclosure_marshal_VOID__VOID,
                              G_TYPE_NONE,
                              0);

        ario_mpd_signals[STATE_CHANGED] =
                g_signal_new ("state_changed",
                              G_OBJECT_CLASS_TYPE (object_class),
                              G_SIGNAL_RUN_LAST,
                              G_STRUCT_OFFSET (ArioMpdClass, state_changed),
                              NULL, NULL,
                              g_cclosure_marshal_VOID__VOID,
                              G_TYPE_NONE,
                              0);

        ario_mpd_signals[VOLUME_CHANGED] =
                g_signal_new ("volume_changed",
                              G_OBJECT_CLASS_TYPE (object_class),
                              G_SIGNAL_RUN_LAST,
                              G_STRUCT_OFFSET (ArioMpdClass, volume_changed),
                              NULL, NULL,
                              g_cclosure_marshal_VOID__VOID,
                              G_TYPE_NONE,
                              0);

        ario_mpd_signals[ELAPSED_CHANGED] =
                g_signal_new ("elapsed_changed",
                              G_OBJECT_CLASS_TYPE (object_class),
                              G_SIGNAL_RUN_LAST,
                              G_STRUCT_OFFSET (ArioMpdClass, elapsed_changed),
                              NULL, NULL,
                              g_cclosure_marshal_VOID__VOID,
                              G_TYPE_NONE,
                              0);

        ario_mpd_signals[PLAYLIST_CHANGED] =
                g_signal_new ("playlist_changed",
                              G_OBJECT_CLASS_TYPE (object_class),
                              G_SIGNAL_RUN_LAST,
                              G_STRUCT_OFFSET (ArioMpdClass, playlist_changed),
                              NULL, NULL,
                              g_cclosure_marshal_VOID__VOID,
                              G_TYPE_NONE,
                              0);

        ario_mpd_signals[RANDOM_CHANGED] =
                g_signal_new ("random_changed",
                              G_OBJECT_CLASS_TYPE (object_class),
                              G_SIGNAL_RUN_LAST,
                              G_STRUCT_OFFSET (ArioMpdClass, playlist_changed),
                              NULL, NULL,
                              g_cclosure_marshal_VOID__VOID,
                              G_TYPE_NONE,
                              0);

        ario_mpd_signals[REPEAT_CHANGED] =
                g_signal_new ("repeat_changed",
                              G_OBJECT_CLASS_TYPE (object_class),
                              G_SIGNAL_RUN_LAST,
                              G_STRUCT_OFFSET (ArioMpdClass, playlist_changed),
                              NULL, NULL,
                              g_cclosure_marshal_VOID__VOID,
                              G_TYPE_NONE,
                              0);

        ario_mpd_signals[DBTIME_CHANGED] =
                g_signal_new ("dbtime_changed",
                              G_OBJECT_CLASS_TYPE (object_class),
                              G_SIGNAL_RUN_LAST,
                              G_STRUCT_OFFSET (ArioMpdClass, dbtime_changed),
                              NULL, NULL,
                              g_cclosure_marshal_VOID__VOID,
                              G_TYPE_NONE,
                              0);
}

static void
ario_mpd_init (ArioMpd *mpd)
{
        ARIO_LOG_FUNCTION_START
        mpd->priv = g_new0 (ArioMpdPrivate, 1);

        mpd->priv->song_id = -1;
        mpd->priv->playlist_id = -1;
        mpd->priv->volume = 0;

        if (eel_gconf_get_boolean (CONF_AUTOCONNECT))
                ario_mpd_connect (mpd);
}

static void
ario_mpd_finalize (GObject *object)
{
        ARIO_LOG_FUNCTION_START
        ArioMpd *mpd;

        g_return_if_fail (object != NULL);
        g_return_if_fail (IS_ARIO_MPD (object));

        mpd = ARIO_MPD (object);

        g_return_if_fail (mpd->priv != NULL);

        ario_mpd_disconnect (mpd);

        g_free (mpd->priv);

        G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
ario_mpd_set_property (GObject *object,
                       guint prop_id,
                       const GValue *value,
                       GParamSpec *pspec)
{
        ARIO_LOG_FUNCTION_START
        ArioMpd *mpd = ARIO_MPD (object);
        mpd_InfoEntity *ent = NULL;
        gboolean need_emit = FALSE;

        switch (prop_id) {
        case PROP_SONGID:
                mpd->priv->song_id = g_value_get_int (value);

                if (mpd->priv->ario_mpd_song != NULL) {
                        mpd_freeSong (mpd->priv->ario_mpd_song);
                        mpd->priv->ario_mpd_song = NULL;
                }

                /* check if there is a connection */
                if (ario_mpd_is_connected (mpd)) {
                        mpd_sendCurrentSongCommand (mpd->priv->connection);
                        ent = mpd_getNextInfoEntity (mpd->priv->connection);
                        if (ent != NULL) {
                                mpd->priv->ario_mpd_song = mpd_songDup (ent->info.song);
                                mpd_finishCommand (mpd->priv->connection);
                                mpd_freeInfoEntity (ent);
                        }
                        mpd->priv->total_time = mpd->priv->status->totalTime;
                }
                g_signal_emit (G_OBJECT (mpd), ario_mpd_signals[SONG_CHANGED], 0);
                break;
        case PROP_STATE:
                mpd->priv->state = g_value_get_int (value);
                g_signal_emit (G_OBJECT (mpd), ario_mpd_signals[STATE_CHANGED], 0);
                break;
        case PROP_VOLUME:
                mpd->priv->volume = g_value_get_int (value);
                g_signal_emit (G_OBJECT (mpd), ario_mpd_signals[VOLUME_CHANGED], 0);
                break;
        case PROP_ELAPSED:
                mpd->priv->elapsed = g_value_get_int (value);
                g_signal_emit (G_OBJECT (mpd), ario_mpd_signals[ELAPSED_CHANGED], 0);
                break;
        case PROP_PLAYLISTID:
                mpd->priv->playlist_id = g_value_get_int (value);
                if (ario_mpd_is_connected (mpd))
                        mpd->priv->playlist_length = mpd->priv->status->playlistLength;
                else
                        mpd->priv->playlist_length = 0;
                g_signal_emit (G_OBJECT (mpd), ario_mpd_signals[PLAYLIST_CHANGED], 0);
                break;
        case PROP_RANDOM:
                mpd->priv->random = g_value_get_boolean (value);
                g_signal_emit (G_OBJECT (mpd), ario_mpd_signals[RANDOM_CHANGED], 0);
                break;
        case PROP_REPEAT:
                mpd->priv->repeat = g_value_get_boolean (value);
                g_signal_emit (G_OBJECT (mpd), ario_mpd_signals[REPEAT_CHANGED], 0);
                break;
        case PROP_DBTIME:
                need_emit = (mpd->priv->dbtime != 0);
                mpd->priv->dbtime = g_value_get_ulong (value);
                if (need_emit)
                        g_signal_emit (G_OBJECT (mpd), ario_mpd_signals[DBTIME_CHANGED], 0);
                break;
        default:
                G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
                break;
        }
}

static void 
ario_mpd_get_property (GObject *object,
                       guint prop_id,
                       GValue *value,
                       GParamSpec *pspec)
{
        ARIO_LOG_FUNCTION_START
        ArioMpd *mpd = ARIO_MPD (object);

        switch (prop_id) {
        case PROP_SONGID:
                g_value_set_int (value, mpd->priv->song_id);
                break;
        case PROP_STATE:
                g_value_set_int (value, mpd->priv->state);
                break;
        case PROP_VOLUME:
                g_value_set_int (value, mpd->priv->volume);
                break;
        case PROP_ELAPSED:
                g_value_set_int (value, mpd->priv->elapsed);
                break;
        case PROP_PLAYLISTID:
                g_value_set_int (value, mpd->priv->playlist_id);
                break;
        case PROP_RANDOM:
                g_value_set_boolean (value, mpd->priv->random);
                break;
        case PROP_REPEAT:
                g_value_set_boolean (value, mpd->priv->repeat);
                break;
        case PROP_DBTIME:
                g_value_set_ulong (value, mpd->priv->dbtime);
                break;
        default:
                G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
                break;
        }
}

ArioMpd *
ario_mpd_new (void)
{
        ARIO_LOG_FUNCTION_START
        ArioMpd *mpd;

        mpd = g_object_new (TYPE_ARIO_MPD,
                            NULL);

        g_return_val_if_fail (mpd->priv != NULL, NULL);

        return ARIO_MPD (mpd);
}

static void
ario_mpd_connect_to (ArioMpd *mpd,
                     gchar *hostname,
                     int port,
                     float timeout)
{
        ARIO_LOG_FUNCTION_START
        mpd_Stats *stats;

        mpd->priv->connection = mpd_newConnection (hostname, port, timeout);

        if (eel_gconf_get_boolean (CONF_AUTH)) {
                gchar *password;

                password = eel_gconf_get_string (CONF_PASSWORD);
                mpd_sendPasswordCommand (mpd->priv->connection, password);
                mpd_finishCommand (mpd->priv->connection);
                g_free (password);
        }

        mpd_sendStatsCommand (mpd->priv->connection);
        stats = mpd_getStats (mpd->priv->connection);
        mpd_finishCommand (mpd->priv->connection);
        if (stats == NULL) {
                GtkWidget *dialog = gtk_message_dialog_new (NULL, GTK_DIALOG_MODAL, 
                                                            GTK_MESSAGE_ERROR,
                                                            GTK_BUTTONS_OK,
                                                            _("Impossible to connect to mpd. Check the connection options."));
                ario_mpd_disconnect (mpd);
                gtk_dialog_run (GTK_DIALOG (dialog));
                gtk_widget_destroy (dialog);        
        }
        else
                mpd_freeStats(stats);
}

void
ario_mpd_connect (ArioMpd *mpd)
{
        ARIO_LOG_FUNCTION_START
        gchar *hostname;
        int port;
        float timeout;

        /* check if there is a connection */
        if (ario_mpd_is_connected (mpd))
                return;

        hostname = eel_gconf_get_string (CONF_HOST);
        port = eel_gconf_get_integer (CONF_PORT);
        timeout = 8.0;

        if (hostname == NULL)
                hostname = g_strdup ("localhost");

        if (port == 0)
                port = 6600;

        ario_mpd_connect_to (mpd, hostname, port, timeout);

        g_free (hostname);
}

void
ario_mpd_disconnect (ArioMpd *mpd)
{
        ARIO_LOG_FUNCTION_START
        /* check if there is a connection */
        if (!ario_mpd_is_connected (mpd))
                return;

        mpd_closeConnection (mpd->priv->connection);
        mpd->priv->connection = NULL;
}

void
ario_mpd_update_db (ArioMpd *mpd)
{
        ARIO_LOG_FUNCTION_START
        /* check if there is a connection */
        if (!ario_mpd_is_connected (mpd))
                return;

        mpd_sendUpdateCommand (mpd->priv->connection, "");
        mpd_finishCommand (mpd->priv->connection);
}

void
ario_mpd_check_errors (ArioMpd *mpd)
{
        // desactivated to make the logs more readable
        // ARIO_LOG_FUNCTION_START
        if (!ario_mpd_is_connected(mpd))
                return;

        if  (mpd->priv->connection->error) {
                ARIO_LOG_DBG("Error nb: %d, Msg:%s\n", mpd->priv->connection->errorCode, mpd->priv->connection->errorStr);
                ario_mpd_disconnect(mpd);
        }
}

gboolean
ario_mpd_is_connected (ArioMpd *mpd)
{
        // desactivated to make the logs more readable
        // ARIO_LOG_FUNCTION_START
        return (mpd->priv->connection != NULL);
}

GList *
ario_mpd_get_artists (ArioMpd *mpd)
{
        ARIO_LOG_FUNCTION_START
        GList *artists = NULL;
        gchar *artist_char;

        /* check if there is a connection */
        if (!ario_mpd_is_connected (mpd))
                return NULL;

        mpd_sendListCommand (mpd->priv->connection, MPD_TABLE_ARTIST, NULL);

        while ((artist_char = mpd_getNextArtist (mpd->priv->connection)) != NULL)
                artists = g_list_append (artists, artist_char);

        return artists;
}

GList *
ario_mpd_get_albums (ArioMpd *mpd,
                     const char *artist)
{
        ARIO_LOG_FUNCTION_START
        GList *albums = NULL;
        mpd_InfoEntity *entity = NULL;
        gchar *prev_album = "";

        /* check if there is a connection */
        if (!ario_mpd_is_connected (mpd))
                return NULL;

        mpd_sendFindCommand (mpd->priv->connection, MPD_TABLE_ARTIST, artist);
        while ((entity = mpd_getNextInfoEntity (mpd->priv->connection))) {
                if (!entity->info.song->album) {
                        if (!g_utf8_collate (prev_album, ARIO_MPD_UNKNOWN)) {
                                mpd_freeInfoEntity (entity);
                                continue;
                        }
                } else {
                        if (!g_utf8_collate (entity->info.song->album, prev_album)) {
                                mpd_freeInfoEntity (entity);
                                continue;
                        }
                }

                ArioMpdAlbum *ario_mpd_album = (ArioMpdAlbum *) g_malloc (sizeof (ArioMpdAlbum));
                if (entity->info.song->album)
                        ario_mpd_album->album = g_strdup (entity->info.song->album);
                else
                        ario_mpd_album->album = g_strdup (ARIO_MPD_UNKNOWN);

                if (entity->info.song->artist)
                        ario_mpd_album->artist = g_strdup (entity->info.song->artist);
                else
                        ario_mpd_album->artist = g_strdup (ARIO_MPD_UNKNOWN);

                prev_album = ario_mpd_album->album;
                albums = g_list_append (albums, ario_mpd_album);
                
                mpd_freeInfoEntity (entity);
        }

        return albums;
}

GList *
ario_mpd_get_songs (ArioMpd *mpd,
                    const char *artist, 
                    const char *album)
{
        ARIO_LOG_FUNCTION_START
        GList *songs = NULL;
        mpd_InfoEntity *entity = NULL;
        gboolean is_album_unknown;

        /* check if there is a connection */
        if (!ario_mpd_is_connected (mpd))
                return NULL;

        is_album_unknown = !g_utf8_collate (album, ARIO_MPD_UNKNOWN);

        if (is_album_unknown)
                mpd_sendFindCommand (mpd->priv->connection, MPD_TABLE_ARTIST, artist);
        else
                mpd_sendFindCommand (mpd->priv->connection, MPD_TABLE_ALBUM, album);

        while ((entity = mpd_getNextInfoEntity (mpd->priv->connection))) {
                if (!g_utf8_collate (entity->info.song->artist, artist)) {
                        if (entity->info.song)
                                if (!is_album_unknown || !entity->info.song->album)
                                        songs = g_list_append (songs, mpd_songDup (entity->info.song));
                }
                mpd_freeInfoEntity (entity);
        }

        return songs;
}

mpd_Connection *
ario_mpd_get_connection (ArioMpd *mpd)
{
        ARIO_LOG_FUNCTION_START
        return mpd->priv->connection;
}

void
ario_mpd_set_default (ArioMpd *mpd)
{
        ARIO_LOG_FUNCTION_START
        if (mpd->priv->song_id != 0)
                g_object_set (G_OBJECT (mpd), "song_id", 0, NULL);

        if (mpd->priv->state != MPD_STATUS_STATE_UNKNOWN)
                g_object_set (G_OBJECT (mpd), "state", MPD_STATUS_STATE_UNKNOWN, NULL);

        if (mpd->priv->volume != MPD_STATUS_NO_VOLUME)
                g_object_set (G_OBJECT (mpd), "volume", MPD_STATUS_NO_VOLUME, NULL);

        if (mpd->priv->elapsed != 0)
                g_object_set (G_OBJECT (mpd), "elapsed", 0, NULL);

        if (mpd->priv->playlist_id != -1)
                g_object_set (G_OBJECT (mpd), "playlist_id", -1, NULL);

        if (mpd->priv->random != FALSE)
                g_object_set (G_OBJECT (mpd), "random", FALSE, NULL);

        if (mpd->priv->repeat != FALSE)
                g_object_set (G_OBJECT (mpd), "repeat", FALSE, NULL);

        if (mpd->priv->dbtime != 0)
                g_object_set (G_OBJECT (mpd), "dbtime", 0, NULL);
}

gboolean
ario_mpd_update_status (ArioMpd *mpd)
{
        // desactivated to make the logs more readable
        // ARIO_LOG_FUNCTION_START
        /* check if there is a connection */
        if (!ario_mpd_is_connected (mpd)) {
                ario_mpd_set_default (mpd);
                return TRUE;
        }

        if (mpd->priv->status != NULL)
                mpd_freeStatus (mpd->priv->status);
        mpd_sendStatusCommand (mpd->priv->connection);
        mpd->priv->status = mpd_getStatus (mpd->priv->connection);
        mpd_finishCommand (mpd->priv->connection);

        if (mpd->priv->stats != NULL)
                mpd_freeStats (mpd->priv->stats);
        mpd_sendStatsCommand (mpd->priv->connection);
        mpd->priv->stats = mpd_getStats (mpd->priv->connection);

        mpd_finishCommand (mpd->priv->connection);

        ario_mpd_check_errors(mpd);
        /* check if there is a connection */
        if (!ario_mpd_is_connected (mpd)) {
                ario_mpd_set_default (mpd);
                return TRUE;
        }

        if (mpd->priv->song_id != mpd->priv->status->songid)
                g_object_set (G_OBJECT (mpd), "song_id", mpd->priv->status->songid, NULL);

        if (mpd->priv->state != mpd->priv->status->state)
                g_object_set (G_OBJECT (mpd), "state", mpd->priv->status->state, NULL);

        if (mpd->priv->volume != mpd->priv->status->volume)
                g_object_set (G_OBJECT (mpd), "volume", mpd->priv->status->volume, NULL);

        if (mpd->priv->elapsed != mpd->priv->status->elapsedTime)
                g_object_set (G_OBJECT (mpd), "elapsed", mpd->priv->status->elapsedTime, NULL);

        if (mpd->priv->playlist_id != (int) mpd->priv->status->playlist)
                g_object_set (G_OBJECT (mpd), "playlist_id", mpd->priv->status->playlist, NULL);

        if (mpd->priv->random != (gboolean) mpd->priv->status->random)
                g_object_set (G_OBJECT (mpd), "random", mpd->priv->status->random, NULL);

        if (mpd->priv->repeat != (gboolean) mpd->priv->status->repeat)
                g_object_set (G_OBJECT (mpd), "repeat", mpd->priv->status->repeat, NULL);

        if (mpd->priv->dbtime != (unsigned long) mpd->priv->stats->dbUpdateTime)
                g_object_set (G_OBJECT (mpd), "dbtime", mpd->priv->stats->dbUpdateTime, NULL);

        return TRUE;
}

char *
ario_mpd_get_current_title (ArioMpd *mpd)
{
        ARIO_LOG_FUNCTION_START
        if (mpd->priv->ario_mpd_song)
                return mpd->priv->ario_mpd_song->title;
        else
                return NULL;
}

char *
ario_mpd_get_current_artist (ArioMpd *mpd)
{
        ARIO_LOG_FUNCTION_START
        if (mpd->priv->ario_mpd_song)
                return mpd->priv->ario_mpd_song->artist;
        else
                return NULL;
}

char *
ario_mpd_get_current_album (ArioMpd *mpd)
{
        ARIO_LOG_FUNCTION_START
        if (mpd->priv->ario_mpd_song)
                return mpd->priv->ario_mpd_song->album;
        else
                return NULL;
}

int
ario_mpd_get_current_song_id (ArioMpd *mpd)
{
        ARIO_LOG_FUNCTION_START
        return mpd->priv->song_id;
}

int
ario_mpd_get_current_state (ArioMpd *mpd)
{
        ARIO_LOG_FUNCTION_START
        return mpd->priv->state;
}

int
ario_mpd_get_current_elapsed (ArioMpd *mpd)
{
        ARIO_LOG_FUNCTION_START
        return mpd->priv->elapsed;
}

int
ario_mpd_get_current_volume (ArioMpd *mpd)
{
        ARIO_LOG_FUNCTION_START
        return mpd->priv->volume;
}

int
ario_mpd_get_current_total_time (ArioMpd *mpd)
{
        ARIO_LOG_FUNCTION_START
        return mpd->priv->total_time;
}

int
ario_mpd_get_current_playlist_id (ArioMpd *mpd)
{
        ARIO_LOG_FUNCTION_START
        return mpd->priv->playlist_id;
}

int
ario_mpd_get_current_playlist_length (ArioMpd *mpd)
{
        ARIO_LOG_FUNCTION_START
        return mpd->priv->playlist_length;
}

int
ario_mpd_get_current_playlist_total_time (ArioMpd *mpd)
{
        ARIO_LOG_FUNCTION_START
        int total_time = 0;
        mpd_Song *song;
        mpd_Connection *connection;
        mpd_InfoEntity *ent = NULL;

        if (!ario_mpd_is_connected (mpd))
                return 0;

        connection = ario_mpd_get_connection (mpd);

        mpd_sendPlaylistInfoCommand(connection, -1);

        while ((ent = mpd_getNextInfoEntity (connection)) != NULL) {
                song = ent->info.song;
                if (song->time != MPD_SONG_NO_TIME)
                        total_time = total_time + song->time;
                mpd_freeInfoEntity(ent);
        }

        return total_time;
}

int
ario_mpd_get_crossfadetime (ArioMpd *mpd)
{
        ARIO_LOG_FUNCTION_START
        if (mpd->priv->status)
                return mpd->priv->status->crossfade;
        else
                return 0;
}

gboolean
ario_mpd_get_current_random (ArioMpd *mpd)
{
        ARIO_LOG_FUNCTION_START
        return mpd->priv->random;
}

gboolean
ario_mpd_get_current_repeat (ArioMpd *mpd)
{
        ARIO_LOG_FUNCTION_START
        return mpd->priv->repeat;
}

gboolean
ario_mpd_get_updating (ArioMpd *mpd)
{
        ARIO_LOG_FUNCTION_START
        if (mpd->priv->status)
                return mpd->priv->status->updatingDb;
        else
                return FALSE;
}

unsigned long
ario_mpd_get_last_update (ArioMpd *mpd)
{
        ARIO_LOG_FUNCTION_START
        if (mpd->priv->stats)
                return mpd->priv->stats->dbUpdateTime;
        else
                return 0;
}

gboolean
ario_mpd_is_paused (ArioMpd *mpd)
{
        ARIO_LOG_FUNCTION_START
        return (mpd->priv->state == MPD_STATUS_STATE_PAUSE) || (mpd->priv->state == MPD_STATUS_STATE_STOP);
}

void
ario_mpd_do_next (ArioMpd *mpd)
{
        ARIO_LOG_FUNCTION_START
        /* check if there is a connection */
        if (!ario_mpd_is_connected (mpd))
                return;

        mpd_sendNextCommand (mpd->priv->connection);
        mpd_finishCommand (mpd->priv->connection);
}

void
ario_mpd_do_prev (ArioMpd *mpd)
{
        ARIO_LOG_FUNCTION_START
        /* check if there is a connection */
        if (!ario_mpd_is_connected (mpd))
                return;

        mpd_sendPrevCommand (mpd->priv->connection);
        mpd_finishCommand (mpd->priv->connection);
}

void
ario_mpd_do_play (ArioMpd *mpd)
{
        ARIO_LOG_FUNCTION_START
        /* check if there is a connection */
        if (!ario_mpd_is_connected (mpd))
                return;

        mpd_sendPlayCommand (mpd->priv->connection, -1);
        mpd_finishCommand (mpd->priv->connection);
}

void
ario_mpd_do_play_id (ArioMpd *mpd,
                     gint id)
{
        ARIO_LOG_FUNCTION_START
        /* check if there is a connection */
        if (!ario_mpd_is_connected (mpd))
                return;

        /* send mpd the play command */
        mpd_sendPlayIdCommand (mpd->priv->connection, id);
        mpd_finishCommand (mpd->priv->connection);
}

void
ario_mpd_do_pause (ArioMpd *mpd)
{
        ARIO_LOG_FUNCTION_START
        /* check if there is a connection */
        if (!ario_mpd_is_connected (mpd))
                return;

        mpd_sendPauseCommand (mpd->priv->connection, TRUE);
        mpd_finishCommand (mpd->priv->connection);
}

void
ario_mpd_do_stop (ArioMpd *mpd)
{
        ARIO_LOG_FUNCTION_START
        /* check if there is a connection */
        if (!ario_mpd_is_connected (mpd))
                return;

        mpd_sendStopCommand (mpd->priv->connection);
        mpd_finishCommand (mpd->priv->connection);
}

void
ario_mpd_free_album (ArioMpdAlbum *ario_mpd_album)
{
        ARIO_LOG_FUNCTION_START
        g_free (ario_mpd_album->album);
        g_free (ario_mpd_album->artist);
        g_free (ario_mpd_album);
}

void
ario_mpd_set_current_elapsed (ArioMpd *mpd,
                              gint elapsed)
{
        ARIO_LOG_FUNCTION_START
        /* check if there is a connection */
        if (!ario_mpd_is_connected (mpd))
                return;

        mpd_sendSeekCommand (mpd->priv->connection, mpd->priv->status->song, elapsed);
        mpd_finishCommand (mpd->priv->connection);
}

void
ario_mpd_set_current_volume (ArioMpd *mpd,
                             gint volume)
{
        ARIO_LOG_FUNCTION_START
        /* check if there is a connection */
        if (!ario_mpd_is_connected (mpd))
                return;

        mpd_sendSetvolCommand (mpd->priv->connection, volume);
        mpd_finishCommand (mpd->priv->connection);
}

void
ario_mpd_set_current_random (ArioMpd *mpd,
                             gboolean random)
{
        ARIO_LOG_FUNCTION_START
        /* check if there is a connection */
        if (!ario_mpd_is_connected (mpd))
                return;

        mpd_sendRandomCommand (mpd->priv->connection, random);
        mpd_finishCommand (mpd->priv->connection);
}

void
ario_mpd_set_current_repeat (ArioMpd *mpd,
                             gboolean repeat)
{
        ARIO_LOG_FUNCTION_START
        /* check if there is a connection */
        if (!ario_mpd_is_connected (mpd))
                return;

        mpd_sendRepeatCommand (mpd->priv->connection, repeat);
        mpd_finishCommand (mpd->priv->connection);
}

void
ario_mpd_set_crossfadetime (ArioMpd *mpd,
                            int crossfadetime)
{
        ARIO_LOG_FUNCTION_START
        /* check if there is a connection */
        if (!ario_mpd_is_connected (mpd))
                return;

        mpd_sendCrossfadeCommand (mpd->priv->connection, crossfadetime);        
        mpd_finishCommand (mpd->priv->connection);          
}

void
ario_mpd_clear (ArioMpd *mpd)
{
        ARIO_LOG_FUNCTION_START
        /* check if there is a connection */
        if (!ario_mpd_is_connected (mpd))
                return;

        mpd_sendClearCommand (mpd->priv->connection);
        mpd_finishCommand (mpd->priv->connection);
}

void
ario_mpd_remove (ArioMpd *mpd,
                 GArray *song)
{
        ARIO_LOG_FUNCTION_START
        int i;

        /* check if there is a connection */
        if (!ario_mpd_is_connected (mpd))
                return;

        mpd_sendCommandListBegin (mpd->priv->connection);

        for (i=0; i<song->len; i++)
                if ((g_array_index (song, int, i) - i) >= 0)
                        mpd_sendDeleteCommand (mpd->priv->connection, g_array_index (song, int, i) - i);

        mpd_sendCommandListEnd (mpd->priv->connection);
        mpd_finishCommand (mpd->priv->connection);
}