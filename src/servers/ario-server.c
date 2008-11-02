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
#include <glib/gi18n.h>
#include "lib/ario-conf.h"
#include "servers/ario-mpd.h"
#include "servers/ario-server.h"
#ifdef ENABLE_XMMS2
#include "servers/ario-xmms.h"
#endif
#include "preferences/ario-preferences.h"
#include "ario-debug.h"
#include "ario-profiles.h"

#define NORMAL_TIMEOUT 500
#define LAZY_TIMEOUT 12000

static guint ario_server_signals[SERVER_LAST_SIGNAL] = { 0 };

char * ArioServerItemNames[MPD_TAG_NUM_OF_ITEM_TYPES] =
{
        N_("Artist"),    // MPD_TAG_ITEM_ARTIST
        N_("Album"),     // MPD_TAG_ITEM_ALBUM
        N_("Title"),     // MPD_TAG_ITEM_TITLE
        N_("Track"),     // MPD_TAG_ITEM_TRACK
        NULL,            // MPD_TAG_ITEM_NAME
        N_("Genre"),     // MPD_TAG_ITEM_GENRE
        N_("Date"),      // MPD_TAG_ITEM_DATE
        N_("Composer"),  // MPD_TAG_ITEM_COMPOSER
        N_("Performer"), // MPD_TAG_ITEM_PERFORMER
        NULL,            // MPD_TAG_ITEM_COMMENT
        NULL,            // MPD_TAG_ITEM_DISC
        N_("Filename"),  // MPD_TAG_ITEM_FILENAME
        N_("Any")        // MPD_TAG_ITEM_ANY
};

G_DEFINE_TYPE (ArioServer, ario_server, G_TYPE_OBJECT)

static ArioServer *instance = NULL;
static ArioServerInterface *interface = NULL;

static void
ario_server_class_init (ArioServerClass *klass)
{
        ARIO_LOG_FUNCTION_START
        GObjectClass *object_class = G_OBJECT_CLASS (klass);

        ario_server_signals[SERVER_SONG_CHANGED] =
                g_signal_new ("song_changed",
                              G_OBJECT_CLASS_TYPE (object_class),
                              G_SIGNAL_RUN_LAST,
                              G_STRUCT_OFFSET (ArioServerClass, song_changed),
                              NULL, NULL,
                              g_cclosure_marshal_VOID__VOID,
                              G_TYPE_NONE,
                              0);

        ario_server_signals[SERVER_ALBUM_CHANGED] =
                g_signal_new ("album_changed",
                              G_OBJECT_CLASS_TYPE (object_class),
                              G_SIGNAL_RUN_LAST,
                              G_STRUCT_OFFSET (ArioServerClass, album_changed),
                              NULL, NULL,
                              g_cclosure_marshal_VOID__VOID,
                              G_TYPE_NONE,
                              0);

        ario_server_signals[SERVER_CONNECTIVITY_CHANGED] =
                g_signal_new ("connectivity_changed",
                              G_OBJECT_CLASS_TYPE (object_class),
                              G_SIGNAL_RUN_LAST,
                              G_STRUCT_OFFSET (ArioServerClass, connectivity_changed),
                              NULL, NULL,
                              g_cclosure_marshal_VOID__VOID,
                              G_TYPE_NONE,
                              0);

        ario_server_signals[SERVER_STATE_CHANGED] =
                g_signal_new ("state_changed",
                              G_OBJECT_CLASS_TYPE (object_class),
                              G_SIGNAL_RUN_LAST,
                              G_STRUCT_OFFSET (ArioServerClass, state_changed),
                              NULL, NULL,
                              g_cclosure_marshal_VOID__VOID,
                              G_TYPE_NONE,
                              0);

        ario_server_signals[SERVER_VOLUME_CHANGED] =
                g_signal_new ("volume_changed",
                              G_OBJECT_CLASS_TYPE (object_class),
                              G_SIGNAL_RUN_LAST,
                              G_STRUCT_OFFSET (ArioServerClass, volume_changed),
                              NULL, NULL,
                              g_cclosure_marshal_VOID__INT,
                              G_TYPE_NONE,
                              1,
                              G_TYPE_INT);

        ario_server_signals[SERVER_ELAPSED_CHANGED] =
                g_signal_new ("elapsed_changed",
                              G_OBJECT_CLASS_TYPE (object_class),
                              G_SIGNAL_RUN_LAST,
                              G_STRUCT_OFFSET (ArioServerClass, elapsed_changed),
                              NULL, NULL,
                              g_cclosure_marshal_VOID__INT,
                              G_TYPE_NONE,
                              1,
                              G_TYPE_INT);

        ario_server_signals[SERVER_PLAYLIST_CHANGED] =
                g_signal_new ("playlist_changed",
                              G_OBJECT_CLASS_TYPE (object_class),
                              G_SIGNAL_RUN_LAST,
                              G_STRUCT_OFFSET (ArioServerClass, playlist_changed),
                              NULL, NULL,
                              g_cclosure_marshal_VOID__VOID,
                              G_TYPE_NONE,
                              0);

        ario_server_signals[SERVER_RANDOM_CHANGED] =
                g_signal_new ("random_changed",
                              G_OBJECT_CLASS_TYPE (object_class),
                              G_SIGNAL_RUN_LAST,
                              G_STRUCT_OFFSET (ArioServerClass, playlist_changed),
                              NULL, NULL,
                              g_cclosure_marshal_VOID__VOID,
                              G_TYPE_NONE,
                              0);

        ario_server_signals[SERVER_REPEAT_CHANGED] =
                g_signal_new ("repeat_changed",
                              G_OBJECT_CLASS_TYPE (object_class),
                              G_SIGNAL_RUN_LAST,
                              G_STRUCT_OFFSET (ArioServerClass, playlist_changed),
                              NULL, NULL,
                              g_cclosure_marshal_VOID__VOID,
                              G_TYPE_NONE,
                              0);

        ario_server_signals[SERVER_UPDATINGDB_CHANGED] =
                g_signal_new ("updatingdb_changed",
                              G_OBJECT_CLASS_TYPE (object_class),
                              G_SIGNAL_RUN_LAST,
                              G_STRUCT_OFFSET (ArioServerClass, updatingdb_changed),
                              NULL, NULL,
                              g_cclosure_marshal_VOID__VOID,
                              G_TYPE_NONE,
                              0);

        ario_server_signals[SERVER_STOREDPLAYLISTS_CHANGED] =
                g_signal_new ("storedplaylists_changed",
                              G_OBJECT_CLASS_TYPE (object_class),
                              G_SIGNAL_RUN_LAST,
                              G_STRUCT_OFFSET (ArioServerClass, storedplaylists_changed),
                              NULL, NULL,
                              g_cclosure_marshal_VOID__VOID,
                              G_TYPE_NONE,
                              0);
}

static void
ario_server_init (ArioServer *server)
{
        ARIO_LOG_FUNCTION_START
}

static void
ario_server_reset_interface (void)
{
        ARIO_LOG_FUNCTION_START
        static ArioServerType interface_type = -1;
        ArioServerType new_interface_type;

        new_interface_type = ario_profiles_get_current (ario_profiles_get ())->type;

        if (new_interface_type == interface_type)
                return;

        interface_type = new_interface_type;

        if (interface) {
                ario_server_disconnect ();
                g_object_unref (interface);
        }
        if (interface_type == ArioServerMpd) {
                interface = ARIO_SERVER_INTERFACE (ario_mpd_get_instance (instance));
#ifdef ENABLE_XMMS2
        } else if (interface_type == ArioServerXmms) {
                interface = ARIO_SERVER_INTERFACE (ario_xmms_get_instance (instance));
#endif
        } else {
                ARIO_LOG_ERROR ("Unknown server type: %d", interface_type);
                interface = ARIO_SERVER_INTERFACE (ario_mpd_get_instance (instance));
        }
}

ArioServer *
ario_server_get_instance (void)
{
        ARIO_LOG_FUNCTION_START
        if (!instance) {
                instance = g_object_new (TYPE_ARIO_SERVER, NULL);
        }
        if (!interface) {
                ario_server_reset_interface ();
        }

        return instance;
}

gboolean
ario_server_connect (void)
{
        ARIO_LOG_FUNCTION_START

        ario_server_reset_interface ();

        /* check if there is a connection */
        if (ario_server_is_connected () || interface->connecting)
                return FALSE;

        interface->connecting = TRUE;
        ARIO_SERVER_INTERFACE_GET_CLASS (interface)->connect ();
        g_signal_emit (G_OBJECT (instance), ario_server_signals[SERVER_CONNECTIVITY_CHANGED], 0);
        return FALSE;
}

void
ario_server_disconnect (void)
{
        ARIO_LOG_FUNCTION_START
        ARIO_SERVER_INTERFACE_GET_CLASS (interface)->disconnect ();
        g_signal_emit (G_OBJECT (instance), ario_server_signals[SERVER_CONNECTIVITY_CHANGED], 0);
}

void
ario_server_reconnect (void)
{
        ARIO_LOG_FUNCTION_START
        ario_server_disconnect ();
        ario_server_connect ();
        g_signal_emit (G_OBJECT (instance), ario_server_signals[SERVER_CONNECTIVITY_CHANGED], 0);
}

void
ario_server_update_db (void)
{
        ARIO_LOG_FUNCTION_START
        ARIO_SERVER_INTERFACE_GET_CLASS (interface)->update_db ();
}

gboolean
ario_server_is_connected (void)
{
        // desactivated to make the logs more readable
        //ARIO_LOG_FUNCTION_START

        return ARIO_SERVER_INTERFACE_GET_CLASS (interface)->is_connected ();
}

GSList *
ario_server_list_tags (const ArioServerTag tag,
                       const ArioServerCriteria *criteria)
{
        ARIO_LOG_FUNCTION_START
        return ARIO_SERVER_INTERFACE_GET_CLASS (interface)->list_tags (tag, criteria);
}

GSList *
ario_server_get_albums (const ArioServerCriteria *criteria)
{
        ARIO_LOG_FUNCTION_START
        return ARIO_SERVER_INTERFACE_GET_CLASS (interface)->get_albums (criteria);
}

GSList *
ario_server_get_songs (const ArioServerCriteria *criteria,
                       const gboolean exact)
{
        ARIO_LOG_FUNCTION_START
        return ARIO_SERVER_INTERFACE_GET_CLASS (interface)->get_songs (criteria, exact);
}

GSList *
ario_server_get_songs_from_playlist (char *playlist)
{
        ARIO_LOG_FUNCTION_START
        return ARIO_SERVER_INTERFACE_GET_CLASS (interface)->get_songs_from_playlist (playlist);
}

GSList *
ario_server_get_playlists (void)
{
        ARIO_LOG_FUNCTION_START
        return ARIO_SERVER_INTERFACE_GET_CLASS (interface)->get_playlists ();
}

GSList *
ario_server_get_playlist_changes (int playlist_id)
{
        ARIO_LOG_FUNCTION_START
        return ARIO_SERVER_INTERFACE_GET_CLASS (interface)->get_playlist_changes (playlist_id);
}

gboolean
ario_server_update_status (void)
{
        return ARIO_SERVER_INTERFACE_GET_CLASS (interface)->update_status ();
}

char *
ario_server_get_current_title (void)
{
        ARIO_LOG_FUNCTION_START
        if (interface->server_song)
                return interface->server_song->title;
        else
                return NULL;
}

char *
ario_server_get_current_name (void)
{
        ARIO_LOG_FUNCTION_START
        if (interface->server_song)
                return interface->server_song->name;
        else
                return NULL;
}

ArioServerSong *
ario_server_get_current_song_on_server (void)
{
        ARIO_LOG_FUNCTION_START
        return ARIO_SERVER_INTERFACE_GET_CLASS (interface)->get_current_song_on_server ();
}

ArioServerSong *
ario_server_get_current_song (void)
{
        ARIO_LOG_FUNCTION_START
        if (interface->server_song)
                return interface->server_song;
        else
                return NULL;
}

char *
ario_server_get_current_artist (void)
{
        ARIO_LOG_FUNCTION_START
        if (interface->server_song)
                return interface->server_song->artist;
        else
                return NULL;
}

char *
ario_server_get_current_album (void)
{
        ARIO_LOG_FUNCTION_START
        if (interface->server_song)
                return interface->server_song->album;
        else
                return NULL;
}

char *
ario_server_get_current_song_path (void)
{
        ARIO_LOG_FUNCTION_START
        if (interface->server_song)
                return interface->server_song->file;
        else
                return NULL;
}

int
ario_server_get_current_song_id (void)
{
        ARIO_LOG_FUNCTION_START
        return interface->song_id;
}

int
ario_server_get_current_state (void)
{
        ARIO_LOG_FUNCTION_START
        return interface->state;
}

int
ario_server_get_current_elapsed (void)
{
        ARIO_LOG_FUNCTION_START
        return interface->elapsed;
}

int
ario_server_get_current_volume (void)
{
        ARIO_LOG_FUNCTION_START
        return interface->volume;
}

int
ario_server_get_current_total_time (void)
{
        ARIO_LOG_FUNCTION_START
        if (interface->server_song)
                return interface->server_song->time;
        else
                return 0;
}

int
ario_server_get_current_playlist_id (void)
{
        ARIO_LOG_FUNCTION_START
        return interface->playlist_id;
}

int
ario_server_get_current_playlist_length (void)
{
        ARIO_LOG_FUNCTION_START
        return interface->playlist_length;
}

int
ario_server_get_current_playlist_total_time (void)
{
        ARIO_LOG_FUNCTION_START
        return ARIO_SERVER_INTERFACE_GET_CLASS (interface)->get_current_playlist_total_time ();
}

int
ario_server_get_crossfadetime (void)
{
        ARIO_LOG_FUNCTION_START
        if (ario_server_is_connected ())
                return interface->crossfade;
        else
                return 0;
}

gboolean
ario_server_get_current_random (void)
{
        ARIO_LOG_FUNCTION_START
        return interface->random;
}

gboolean
ario_server_get_current_repeat (void)
{
        ARIO_LOG_FUNCTION_START
        return interface->repeat;
}

gboolean
ario_server_get_updating (void)
{
        ARIO_LOG_FUNCTION_START
        return ario_server_is_connected () && interface->updatingdb;
}

unsigned long
ario_server_get_last_update (void)
{
        return ARIO_SERVER_INTERFACE_GET_CLASS (interface)->get_last_update ();
}

gboolean
ario_server_is_paused (void)
{
        ARIO_LOG_FUNCTION_START
        return (interface->state == MPD_STATUS_STATE_PAUSE) || (interface->state == MPD_STATUS_STATE_STOP);
}

void
ario_server_do_next (void)
{
        ARIO_LOG_FUNCTION_START
        ARIO_SERVER_INTERFACE_GET_CLASS (interface)->do_next ();
}

void
ario_server_do_prev (void)
{
        ARIO_LOG_FUNCTION_START
        ARIO_SERVER_INTERFACE_GET_CLASS (interface)->do_prev ();
}

void
ario_server_do_play (void)
{
        ARIO_LOG_FUNCTION_START
        ARIO_SERVER_INTERFACE_GET_CLASS (interface)->do_play ();
}

void
ario_server_do_play_pos (gint id)
{
        ARIO_LOG_FUNCTION_START
        ARIO_SERVER_INTERFACE_GET_CLASS (interface)->do_play_pos (id);
}

void
ario_server_do_pause (void)
{
        ARIO_LOG_FUNCTION_START
        ARIO_SERVER_INTERFACE_GET_CLASS (interface)->do_pause ();
}

void
ario_server_do_stop (void)
{
        ARIO_LOG_FUNCTION_START
        ARIO_SERVER_INTERFACE_GET_CLASS (interface)->do_stop ();
}

void
ario_server_free_album (ArioServerAlbum *server_album)
{
        ARIO_LOG_FUNCTION_START
        if (server_album) {
                g_free (server_album->album);
                g_free (server_album->artist);
                g_free (server_album->path);
                g_free (server_album->date);
                g_free (server_album);
        }
}

ArioServerAlbum*
ario_server_copy_album (const ArioServerAlbum *server_album)
{
        ARIO_LOG_FUNCTION_START
        ArioServerAlbum *ret = NULL;

        if (server_album) {
                ret = (ArioServerAlbum *) g_malloc (sizeof (ArioServerAlbum));
                ret->album = g_strdup (server_album->album);
                ret->artist = g_strdup (server_album->artist);
                ret->path = g_strdup (server_album->path);
                ret->date = g_strdup (server_album->date);
        }

        return ret;
}

void
ario_server_set_current_elapsed (const gint elapsed)
{
        ARIO_LOG_FUNCTION_START
        ARIO_SERVER_INTERFACE_GET_CLASS (interface)->set_current_elapsed (elapsed);
}

void
ario_server_set_current_volume (const gint volume)
{
        ARIO_LOG_FUNCTION_START
        ARIO_SERVER_INTERFACE_GET_CLASS (interface)->set_current_volume (volume);
}

void
ario_server_set_current_random (const gboolean random)
{
        ARIO_LOG_FUNCTION_START
        ARIO_SERVER_INTERFACE_GET_CLASS (interface)->set_current_random (random);
}

void
ario_server_set_current_repeat (const gboolean repeat)
{
        ARIO_LOG_FUNCTION_START
        ARIO_SERVER_INTERFACE_GET_CLASS (interface)->set_current_repeat (repeat);
}

void
ario_server_set_crossfadetime (const int crossfadetime)
{
        ARIO_LOG_FUNCTION_START
        ARIO_SERVER_INTERFACE_GET_CLASS (interface)->set_crossfadetime (crossfadetime);
}

void
ario_server_clear (void)
{
        ARIO_LOG_FUNCTION_START
        ARIO_SERVER_INTERFACE_GET_CLASS (interface)->clear ();
}

void
ario_server_queue_add (const char *path)
{
        ARIO_LOG_FUNCTION_START

        ArioServerQueueAction *queue_action = (ArioServerQueueAction *) g_malloc (sizeof (ArioServerQueueAction));
        queue_action->type = ARIO_SERVER_ACTION_ADD;
        queue_action->path = path;

        interface->queue = g_slist_append (interface->queue, queue_action);
}

void
ario_server_queue_delete_id (const int id)
{
        ArioServerQueueAction *queue_action = (ArioServerQueueAction *) g_malloc (sizeof (ArioServerQueueAction));
        queue_action->type = ARIO_SERVER_ACTION_DELETE_ID;
        queue_action->id = id;

        interface->queue = g_slist_append (interface->queue, queue_action);
}

void
ario_server_queue_delete_pos (const int pos)
{
        ARIO_LOG_FUNCTION_START
        ArioServerQueueAction *queue_action = (ArioServerQueueAction *) g_malloc (sizeof (ArioServerQueueAction));
        queue_action->type = ARIO_SERVER_ACTION_DELETE_POS;
        queue_action->pos = pos;

        interface->queue = g_slist_append (interface->queue, queue_action);
}

void
ario_server_queue_move (const int old_pos,
                        const int new_pos)
{
        ARIO_LOG_FUNCTION_START
        ArioServerQueueAction *queue_action = (ArioServerQueueAction *) g_malloc (sizeof (ArioServerQueueAction));
        queue_action->type = ARIO_SERVER_ACTION_MOVE;
        queue_action->old_pos = old_pos;
        queue_action->new_pos = new_pos;

        interface->queue = g_slist_append (interface->queue, queue_action);
}


void
ario_server_queue_commit (void)
{
        ARIO_LOG_FUNCTION_START
        ARIO_SERVER_INTERFACE_GET_CLASS (interface)->queue_commit ();
}

void
ario_server_insert_at (const GSList *songs,
                       const gint pos)
{
        ARIO_LOG_FUNCTION_START
        ARIO_SERVER_INTERFACE_GET_CLASS (interface)->insert_at (songs, pos);
}

int
ario_server_save_playlist (const char *name)
{
        ARIO_LOG_FUNCTION_START
        int ret = ARIO_SERVER_INTERFACE_GET_CLASS (interface)->save_playlist (name);

        g_signal_emit (G_OBJECT (instance), ario_server_signals[SERVER_STOREDPLAYLISTS_CHANGED], 0);

        return ret;
}

void
ario_server_delete_playlist (const char *name)
{
        ARIO_LOG_FUNCTION_START
        ARIO_SERVER_INTERFACE_GET_CLASS (interface)->delete_playlist (name);

        g_signal_emit (G_OBJECT (instance), ario_server_signals[SERVER_STOREDPLAYLISTS_CHANGED], 0);
}

void
ario_server_use_count_inc (void)
{
        ARIO_LOG_FUNCTION_START
        ARIO_SERVER_INTERFACE_GET_CLASS (interface)->use_count_inc ();
}

void
ario_server_use_count_dec (void)
{
        ARIO_LOG_FUNCTION_START
        ARIO_SERVER_INTERFACE_GET_CLASS (interface)->use_count_dec ();
}

GSList *
ario_server_get_outputs (void)
{
        ARIO_LOG_FUNCTION_START
        return ARIO_SERVER_INTERFACE_GET_CLASS (interface)->get_outputs ();
}

void
ario_server_enable_output (int id,
                           gboolean enabled)
{
        ARIO_LOG_FUNCTION_START
        ARIO_SERVER_INTERFACE_GET_CLASS (interface)->enable_output (id, enabled);
}

ArioServerStats *
ario_server_get_stats (void)
{
        ARIO_LOG_FUNCTION_START
        return ARIO_SERVER_INTERFACE_GET_CLASS (interface)->get_stats ();
}

GList *
ario_server_get_songs_info (GSList *paths)
{
        ARIO_LOG_FUNCTION_START
        return ARIO_SERVER_INTERFACE_GET_CLASS (interface)->get_songs_info (paths);
}

ArioServerFileList *
ario_server_list_files (const char *path,
                        gboolean recursive)
{
        ARIO_LOG_FUNCTION_START
        return ARIO_SERVER_INTERFACE_GET_CLASS (interface)->list_files (path, recursive);
}

void
ario_server_free_file_list (ArioServerFileList *files)
{
        ARIO_LOG_FUNCTION_START
        if (files) {
                g_slist_foreach(files->directories, (GFunc) g_free, NULL);
                g_slist_free (files->directories);
                g_slist_foreach(files->songs, (GFunc) ario_server_free_song, NULL);
                g_slist_free (files->songs);
                g_free (files);
        }
}

void
ario_server_criteria_free (ArioServerCriteria *criteria)
{
        ARIO_LOG_FUNCTION_START
        GSList *tmp;
        ArioServerAtomicCriteria *atomic_criteria;

        for (tmp = criteria; tmp; tmp = g_slist_next (tmp)) {
                atomic_criteria = tmp->data;
                g_free (atomic_criteria->value);
                g_free (atomic_criteria);
        }
        g_slist_free (criteria);
}

ArioServerCriteria *
ario_server_criteria_copy (const ArioServerCriteria *criteria)
{
        ARIO_LOG_FUNCTION_START
        ArioServerCriteria *ret = NULL;
        const GSList *tmp;
        ArioServerAtomicCriteria *atomic_criteria;
        ArioServerAtomicCriteria *new_atomic_criteria;

        for (tmp = criteria; tmp; tmp = g_slist_next (tmp)) {
                atomic_criteria = tmp->data;
                if (criteria) {
                        new_atomic_criteria = (ArioServerAtomicCriteria *) g_malloc0 (sizeof (ArioServerAtomicCriteria));
                        new_atomic_criteria->tag = atomic_criteria->tag;
                        new_atomic_criteria->value = g_strdup (atomic_criteria->value);
                        ret = g_slist_append (ret, new_atomic_criteria);
                }
        }
        return ret;
}

gchar **
ario_server_get_items_names (void)
{
        return ArioServerItemNames;
}
