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
#include <gdk/gdkkeysyms.h>
#include <i18n.h>
#include <string.h>

#include "playlist.h"
#include "mpd.h"
#include "libmpdclient.h"
#include "util.h"
#include "debug.h"

#define DRAG_THRESHOLD 1

static void playlist_class_init (PlaylistClass *klass);
static void playlist_init (Playlist *playlist);
static void playlist_finalize (GObject *object);
static void playlist_set_property (GObject *object,
                                  guint prop_id,
                                  const GValue *value,
                                  GParamSpec *pspec);
static void playlist_get_property (GObject *object,
                                   guint prop_id,
                                   GValue *value,
                                   GParamSpec *pspec);
static void playlist_changed_cb (Mpd *mpd,
                                 Playlist *playlist);
static void playlist_song_changed_cb (Mpd *mpd,
                                      Playlist *playlist);
static void playlist_state_changed_cb (Mpd *mpd,
                                       Playlist *playlist);
static void playlist_drag_leave_cb (GtkWidget *widget,
                                    GdkDragContext *context,
                                    gint x,
                                    gint y,
                                    GtkSelectionData *data,
                                    guint info,
                                    guint time,
                                    gpointer user_data);
static void playlist_drag_data_get_cb (GtkWidget * widget,
                                       GdkDragContext * context,
                                       GtkSelectionData * selection_data,
                                       guint info, guint time, gpointer data);
static void playlist_cmd_clear (GtkAction *action,
                                Playlist *playlist);
static void playlist_cmd_remove (GtkAction *action,
                                 Playlist *playlist);
static gboolean playlist_view_button_press_cb (GtkTreeView *treeview,
                                               GdkEventButton *event,
                                               Playlist *playlist);
static gboolean playlist_view_key_press_cb (GtkWidget *widget,
                                            GdkEventKey *event,
                                            Playlist *playlist);
static gboolean playlist_view_button_release_cb (GtkWidget *widget,
                                                 GdkEventButton *event,
                                                 Playlist *playlist);
static gboolean playlist_view_motion_notify (GtkWidget *widget, 
                                             GdkEventMotion *event,
                                             Playlist *playlist);
static void playlist_activate_row (Playlist *playlist,
                                   GtkTreePath *path);

struct PlaylistPrivate
{        
        GtkWidget *tree;
        GtkListStore *model;
        GtkTreeSelection *selection;

        Mpd *mpd;

        int playlist_id;
        int playlist_length;

        GdkPixbuf *play_pixbuf;

        gboolean dragging;
        gboolean pressed;
        gint drag_start_x;
        gint drag_start_y;

        GtkUIManager *ui_manager;
        GtkActionGroup *actiongroup;
};

static GtkActionEntry playlist_actions [] =
{
        { "PlaylistClear", GTK_STOCK_CLEAR, N_("_Clear"), NULL,
          N_("Clear the playlist"),
          G_CALLBACK (playlist_cmd_clear) },
        { "PlaylistRemove", GTK_STOCK_REMOVE, N_("_Remove"), NULL,
          N_("Remove the selected songs"),
          G_CALLBACK (playlist_cmd_remove) },
};

static guint playlist_n_actions = G_N_ELEMENTS (playlist_actions);

enum
{
        PROP_0,
        PROP_MPD,
        PROP_UI_MANAGER,
        PROP_ACTION_GROUP
};

enum
{
        PIXBUF_COLUMN,
        TRACK_COLUMN,
        TITLE_COLUMN,
        ARTIST_COLUMN,
        ALBUM_COLUMN,
        DURATION_COLUMN,
        ID_COLUMN,
        N_COLUMN
};

static const GtkTargetEntry targets  [] = {
        { "text/internal-list", 0, 10},
        { "text/artists-list", 0, 20 },
        { "text/albums-list", 0, 30 },
        { "text/songs-list", 0, 40 },
};

static const GtkTargetEntry internal_targets  [] = {
        { "text/internal-list", 0, 10 },
};

static GObjectClass *parent_class = NULL;

GType
playlist_get_type (void)
{
        LOG_FUNCTION_START
        static GType type = 0;

        if (!type) {
                static const GTypeInfo our_info =
                {
                        sizeof (PlaylistClass),
                        NULL,
                        NULL,
                        (GClassInitFunc) playlist_class_init,
                        NULL,
                        NULL,
                        sizeof (Playlist),
                        0,
                        (GInstanceInitFunc) playlist_init
                };

                type = g_type_register_static (GTK_TYPE_HBOX,
                                               "Playlist",
                                                &our_info, 0);
        }
        return type;
}

static void
playlist_class_init (PlaylistClass *klass)
{
        LOG_FUNCTION_START
        GObjectClass *object_class = G_OBJECT_CLASS (klass);

        parent_class = g_type_class_peek_parent (klass);

        object_class->finalize = playlist_finalize;

        object_class->set_property = playlist_set_property;
        object_class->get_property = playlist_get_property;

        g_object_class_install_property (object_class,
                                         PROP_MPD,
                                         g_param_spec_object ("mpd",
                                                              "mpd",
                                                              "mpd",
                                                              TYPE_MPD,
                                                              G_PARAM_READWRITE));
        g_object_class_install_property (object_class,
                                         PROP_UI_MANAGER,
                                         g_param_spec_object ("ui-manager",
                                                              "GtkUIManager",
                                                              "GtkUIManager object",
                                                              GTK_TYPE_UI_MANAGER,
                                                              G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
        g_object_class_install_property (object_class,
                                         PROP_ACTION_GROUP,
                                         g_param_spec_object ("action-group",
                                                              "GtkActionGroup",
                                                              "GtkActionGroup object",
                                                              GTK_TYPE_ACTION_GROUP,
                                                              G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
}

static void
playlist_init (Playlist *playlist)
{
        LOG_FUNCTION_START
        GtkTreeViewColumn *column;
        GtkCellRenderer *renderer;
        GtkWidget *scrolledwindow;

        playlist->priv = g_new0 (PlaylistPrivate, 1);
        playlist->priv->playlist_id = -1;
        playlist->priv->playlist_length = 0;
        playlist->priv->play_pixbuf = gdk_pixbuf_new_from_file (PIXMAP_PATH "play.png", NULL);

        scrolledwindow = gtk_scrolled_window_new (NULL, NULL);
        gtk_widget_show (scrolledwindow);
        gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolledwindow), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
        gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scrolledwindow), GTK_SHADOW_IN);

        playlist->priv->tree = gtk_tree_view_new ();

        /* Pixbuf column */
        renderer = gtk_cell_renderer_pixbuf_new ();
        column = gtk_tree_view_column_new_with_attributes (" ",
                                                           renderer,
                                                           "pixbuf", PIXBUF_COLUMN,
                                                           NULL);
        gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_FIXED);
        gtk_tree_view_column_set_fixed_width (column, 30);
        gtk_tree_view_append_column (GTK_TREE_VIEW (playlist->priv->tree), column);

        /* Track column */
        renderer = gtk_cell_renderer_text_new ();
        column = gtk_tree_view_column_new_with_attributes (_("Track"),
                                                           renderer,
                                                           "text", TRACK_COLUMN,
                                                           NULL);
        gtk_tree_view_column_set_resizable (column, TRUE);
        gtk_tree_view_append_column (GTK_TREE_VIEW (playlist->priv->tree), column);

        /* Title column */
        renderer = gtk_cell_renderer_text_new ();
        column = gtk_tree_view_column_new_with_attributes (_("Title"),
                                                           renderer,
                                                           "text", TITLE_COLUMN,
                                                           NULL);
        gtk_tree_view_column_set_resizable (column, TRUE);
        gtk_tree_view_append_column (GTK_TREE_VIEW (playlist->priv->tree), column);

        /* Artist column */
        renderer = gtk_cell_renderer_text_new ();
        column = gtk_tree_view_column_new_with_attributes (_("Artist"),
                                                           renderer,
                                                           "text", ARTIST_COLUMN,
                                                           NULL);
        gtk_tree_view_column_set_resizable (column, TRUE);
        gtk_tree_view_append_column (GTK_TREE_VIEW (playlist->priv->tree), column);

        /* Album column */
        renderer = gtk_cell_renderer_text_new ();
        column = gtk_tree_view_column_new_with_attributes (_("Album"),
                                                           renderer,
                                                           "text", ALBUM_COLUMN,
                                                           NULL);
        gtk_tree_view_column_set_resizable (column, TRUE);
        gtk_tree_view_append_column (GTK_TREE_VIEW (playlist->priv->tree), column);

        /* Duration column */
        renderer = gtk_cell_renderer_text_new ();
        column = gtk_tree_view_column_new_with_attributes (_("Duration"),
                                                           renderer,
                                                           "text", DURATION_COLUMN,
                                                           NULL);
        gtk_tree_view_column_set_resizable (column, TRUE);
        gtk_tree_view_append_column (GTK_TREE_VIEW (playlist->priv->tree), column);

        playlist->priv->model = gtk_list_store_new (N_COLUMN,
                                                    GDK_TYPE_PIXBUF,
                                                    G_TYPE_STRING,
                                                    G_TYPE_STRING,
                                                    G_TYPE_STRING,
                                                    G_TYPE_STRING,
                                                    G_TYPE_STRING,
                                                    G_TYPE_INT);

        gtk_tree_view_set_model (GTK_TREE_VIEW (playlist->priv->tree),
                                 GTK_TREE_MODEL (playlist->priv->model));
        gtk_tree_view_set_reorderable (GTK_TREE_VIEW (playlist->priv->tree),
                                       TRUE);
        gtk_tree_view_set_rules_hint (GTK_TREE_VIEW (playlist->priv->tree),
                                      TRUE);
        playlist->priv->selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (playlist->priv->tree));
        gtk_tree_selection_set_mode (playlist->priv->selection,
                                     GTK_SELECTION_MULTIPLE);
        gtk_container_add (GTK_CONTAINER (scrolledwindow), playlist->priv->tree);

/*
        gtk_tree_view_enable_model_drag_source (GTK_TREE_VIEW (playlist->priv->tree),
                                                GDK_BUTTON1_MASK,
                                                internal_targets, G_N_ELEMENTS (internal_targets),
                                                GDK_ACTION_MOVE);
*/
        gtk_drag_source_set (playlist->priv->tree,
                             GDK_BUTTON1_MASK,
                             internal_targets,
                             G_N_ELEMENTS (internal_targets),
                             GDK_ACTION_MOVE);

        gtk_tree_view_enable_model_drag_dest (GTK_TREE_VIEW (playlist->priv->tree),
                                              targets, G_N_ELEMENTS (targets),
                                              GDK_ACTION_MOVE);

        g_signal_connect_object (G_OBJECT (playlist->priv->tree),
                                 "button_press_event",
                                 G_CALLBACK (playlist_view_button_press_cb),
                                 playlist,
                                 0);
        g_signal_connect_object (G_OBJECT (playlist->priv->tree),
                                 "button_release_event",
                                 G_CALLBACK (playlist_view_button_release_cb),
                                 playlist,
                                 0);
        g_signal_connect_object (G_OBJECT (playlist->priv->tree),
                                 "motion_notify_event",
                                 G_CALLBACK (playlist_view_motion_notify),
                                 playlist,
                                 0);
        g_signal_connect_object (G_OBJECT (playlist->priv->tree),
                                 "key_press_event",
                                 G_CALLBACK (playlist_view_key_press_cb),
                                 playlist,
                                 0);

        g_signal_connect_object (G_OBJECT (playlist->priv->tree), "drag_data_received",
                                 G_CALLBACK (playlist_drag_leave_cb),
                                 playlist, 0);

        g_signal_connect (GTK_TREE_VIEW (playlist->priv->tree),
                          "drag_data_get", 
                          G_CALLBACK (playlist_drag_data_get_cb),
                          playlist);


        gtk_box_set_homogeneous (GTK_BOX (playlist), TRUE);
        gtk_box_set_spacing (GTK_BOX (playlist), 4);

        gtk_box_pack_start (GTK_BOX (playlist), scrolledwindow, TRUE, TRUE, 0);
}

static void
playlist_finalize (GObject *object)
{
        LOG_FUNCTION_START
        Playlist *playlist;

        g_return_if_fail (object != NULL);
        g_return_if_fail (IS_PLAYLIST (object));

        playlist = PLAYLIST (object);

        g_return_if_fail (playlist->priv != NULL);

        g_object_unref (G_OBJECT (playlist->priv->play_pixbuf));
        g_free (playlist->priv);

        G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
playlist_set_property (GObject *object,
                             guint prop_id,
                             const GValue *value,
                             GParamSpec *pspec)
{
        LOG_FUNCTION_START
        Playlist *playlist = PLAYLIST (object);
        
        switch (prop_id) {
        case PROP_MPD:
                playlist->priv->mpd = g_value_get_object (value);
                g_signal_connect_object (G_OBJECT (playlist->priv->mpd),
                                         "playlist_changed", G_CALLBACK (playlist_changed_cb),
                                         playlist, 0);
                g_signal_connect_object (G_OBJECT (playlist->priv->mpd),
                                         "song_changed", G_CALLBACK (playlist_song_changed_cb),
                                         playlist, 0);
                g_signal_connect_object (G_OBJECT (playlist->priv->mpd),
                                         "state_changed", G_CALLBACK (playlist_state_changed_cb),
                                         playlist, 0);
                break;
        case PROP_UI_MANAGER:
                playlist->priv->ui_manager = g_value_get_object (value);
                break;
                break;
        case PROP_ACTION_GROUP:
                playlist->priv->actiongroup = g_value_get_object (value);
                gtk_action_group_add_actions (playlist->priv->actiongroup,
                                              playlist_actions,
                                              playlist_n_actions, playlist);
                break;
        default:
                G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
                break;
        }
}

static void 
playlist_get_property (GObject *object,
                        guint prop_id,
                        GValue *value,
                        GParamSpec *pspec)
{
        LOG_FUNCTION_START
        Playlist *playlist = PLAYLIST (object);

        switch (prop_id) {
        case PROP_MPD:
                g_value_set_object (value, playlist->priv->mpd);
                break;
        case PROP_UI_MANAGER:
                g_value_set_object (value, playlist->priv->ui_manager);
                break;
        case PROP_ACTION_GROUP:
                g_value_set_object (value, playlist->priv->actiongroup);
                break;
        default:
                G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
                break;
        }
}

static void
playlist_sync_song (Playlist *playlist)
{
        LOG_FUNCTION_START
        int new_song_id;
        int song_id;
        int i;
        GtkTreeIter iter;
        int state;

        new_song_id = mpd_get_current_song_id (playlist->priv->mpd);

        state = mpd_get_current_state (playlist->priv->mpd);

        if (state == MPD_STATUS_STATE_UNKNOWN || state == MPD_STATUS_STATE_STOP)
                new_song_id = -1;

        for (i=0; i<playlist->priv->playlist_length; i++) {
                gchar *path = g_strdup_printf ("%i", i);

                if (gtk_tree_model_get_iter_from_string (GTK_TREE_MODEL (playlist->priv->model), &iter, path)) {
                        gtk_tree_model_get (GTK_TREE_MODEL (playlist->priv->model), &iter, ID_COLUMN, &song_id, -1);

                        if (song_id == new_song_id)
                                gtk_list_store_set (playlist->priv->model, &iter,
                                                    PIXBUF_COLUMN, playlist->priv->play_pixbuf,
                                                    -1);
                        else
                                gtk_list_store_set (playlist->priv->model, &iter,
                                                    PIXBUF_COLUMN, NULL,
                                                    -1);
                }
                g_free (path);
        }
}

static void
playlist_changed_cb (Mpd *mpd,
                     Playlist *playlist)
{
        LOG_FUNCTION_START
        int playlist_id;
        mpd_InfoEntity *ent = NULL;
        gint old_length;
        GtkTreeIter iter;
        mpd_Connection *connection;
        gchar *time, *track;
        gchar *artist, *album, *title;

        if (!mpd_is_connected (mpd)) {
                playlist->priv->playlist_length = 0;
                playlist->priv->playlist_id = -1;
                gtk_list_store_clear (playlist->priv->model);
                return;
        }

        connection = mpd_get_connection (mpd);
        playlist_id = mpd_get_current_playlist_id (mpd);
        old_length = playlist->priv->playlist_length;
        mpd_sendPlChangesCommand (connection, playlist->priv->playlist_id);
        
        while ((ent = mpd_getNextInfoEntity (connection)) != NULL) {
                /*
                 * decide wether to update or to add 
                 */
                if (ent->info.song->pos < old_length) {
                        /*
                         * needed for getting the row 
                         */
                        gchar *path = g_strdup_printf ("%i", ent->info.song->pos);

                        if (gtk_tree_model_get_iter_from_string (GTK_TREE_MODEL (playlist->priv->model), &iter, path)) {
                                time = util_format_time (ent->info.song->time);
                                track = util_format_track (ent->info.song->track);
                                artist = ent->info.song->artist ? ent->info.song->artist : MPD_UNKNOWN;
                                album = ent->info.song->album ? ent->info.song->album : MPD_UNKNOWN;
                                title = ent->info.song->title ? ent->info.song->title : MPD_UNKNOWN;
                                gtk_list_store_set (playlist->priv->model, &iter,
                                                    TRACK_COLUMN, track,
                                                    TITLE_COLUMN, title,
                                                    ARTIST_COLUMN, artist,
                                                    ALBUM_COLUMN, album,
                                                    DURATION_COLUMN, time,
                                                    ID_COLUMN, ent->info.song->id,
                                                    -1);
                                g_free (time);
                                g_free (track);
                        }
                        g_free (path);
                } else {
                        gtk_list_store_append (playlist->priv->model, &iter);
                        time = util_format_time (ent->info.song->time);
                        track = util_format_track (ent->info.song->track);
                        artist = ent->info.song->artist ? ent->info.song->artist : MPD_UNKNOWN;
                        album = ent->info.song->album ? ent->info.song->album : MPD_UNKNOWN;
                        title = ent->info.song->title ? ent->info.song->title : MPD_UNKNOWN;
                        gtk_list_store_set (playlist->priv->model, &iter,
                                            TRACK_COLUMN, track,
                                            TITLE_COLUMN, title,
                                            ARTIST_COLUMN, artist,
                                            ALBUM_COLUMN, album,
                                            DURATION_COLUMN, time,
                                            ID_COLUMN, ent->info.song->id,
                                            -1);
                        g_free (time);
                        g_free (track);
                }
                mpd_freeInfoEntity (ent);
        }

        playlist->priv->playlist_length = mpd_get_current_playlist_length (mpd);

        while (playlist->priv->playlist_length < old_length) {
                gchar *path = g_strdup_printf ("%i", old_length - 1);
                if (gtk_tree_model_get_iter_from_string (GTK_TREE_MODEL (playlist->priv->model), &iter, path))
                        gtk_list_store_remove (playlist->priv->model, &iter);

                g_free (path);
                old_length--;
        }

        playlist->priv->playlist_id = playlist_id;

        playlist_sync_song (playlist);
}

static void
playlist_song_changed_cb (Mpd *mpd,
                          Playlist *playlist)
{
        LOG_FUNCTION_START
        playlist_sync_song (playlist);
}

static void
playlist_state_changed_cb (Mpd *mpd,
                           Playlist *playlist)
{
        LOG_FUNCTION_START
        playlist_sync_song (playlist);
}

GtkWidget *
playlist_new (GtkUIManager *mgr,
              GtkActionGroup *group,
              Mpd *mpd)
{
        LOG_FUNCTION_START
        Playlist *playlist;

        playlist = g_object_new (TYPE_PLAYLIST,
                                 "ui-manager", mgr,
                                 "action-group", group,
                                 "mpd", mpd,
                                 NULL);

        g_return_val_if_fail (playlist->priv != NULL, NULL);

        return GTK_WIDGET (playlist);
}

static void
playlist_activate_row (Playlist *playlist,
                       GtkTreePath *path)
{
        LOG_FUNCTION_START
        GtkTreeIter iter;

        /* get the iter from the path */
        if (gtk_tree_model_get_iter (GTK_TREE_MODEL (playlist->priv->model), &iter, path)) {
                gint id;
                /* get the song id */
                gtk_tree_model_get (GTK_TREE_MODEL (playlist->priv->model), &iter, ID_COLUMN, &id, -1);
                mpd_do_play_id (playlist->priv->mpd, id);
        }
}

static void
playlist_move_rows (Playlist *playlist,
                    int x, int y)
{
        LOG_FUNCTION_START
        GtkTreePath *path = NULL;
        GtkTreeViewDropPosition pos;
        gint pos1, pos2;
        gint *indice;
        mpd_Connection *connection;
        GtkTreeModel *model = GTK_TREE_MODEL (playlist->priv->model);

        if (!mpd_is_connected (playlist->priv->mpd))
                return;

        connection = mpd_get_connection (playlist->priv->mpd);

        /* get drop location */
        gtk_tree_view_get_dest_row_at_pos (GTK_TREE_VIEW (playlist->priv->tree), x, y, &path, &pos);

        /* adjust position acording to drop after */
        if (path == NULL) {
                pos2 = playlist->priv->playlist_length;
        } else {
                /* grab drop localtion */
                indice = gtk_tree_path_get_indices (path);
                pos2 = indice[0];

                /* adjust position acording to drop after or for, also take last song in account */
                if (pos == GTK_TREE_VIEW_DROP_AFTER && pos2 < playlist->priv->playlist_length - 1)
                        pos2 ++;
        }

        gtk_tree_path_free (path);
        /* move every dragged row */
        if (gtk_tree_selection_count_selected_rows (playlist->priv->selection) > 0) {
                GList *list = NULL;
                int offset = 0;
                list = gtk_tree_selection_get_selected_rows (playlist->priv->selection, &model);
                gtk_tree_selection_unselect_all (playlist->priv->selection);
                list = g_list_last (list);
                /* start a command list */
                mpd_sendCommandListBegin (connection);
                do {
                        GtkTreePath *path_to_select;
                        /* get start pos */
                        indice = gtk_tree_path_get_indices ((GtkTreePath *) list->data);
                        pos1 = indice[0];

                        /* compensate */
                        if (pos2 > pos1)
                                pos2 --;

                        mpd_sendMoveCommand (connection, pos1 + offset, pos2);

                        if (pos2 < pos1)
                                offset ++;

                        path_to_select = gtk_tree_path_new_from_indices (pos2, -1);
                        gtk_tree_selection_select_path (playlist->priv->selection, path_to_select);
                        gtk_tree_path_free (path_to_select);
                } while ((list = g_list_previous (list)));
                /* free list */
                g_list_foreach (list, (GFunc) gtk_tree_path_free, NULL);
                g_list_free (list);

                mpd_sendCommandListEnd (connection);
                mpd_finishCommand (connection);
        }
}

static void
playlist_add_songs (Playlist *playlist,
                    GList *songs,
                    gint x, gint y)
{
        LOG_FUNCTION_START
        GList *temp_songs;
        mpd_Connection *connection;
        gint *indice;
        int end, offset = 0, drop = 0;
        GtkTreePath *path = NULL;
        GtkTreeViewDropPosition pos;
        gboolean do_not_move = FALSE;

        if (!mpd_is_connected (playlist->priv->mpd))
                return;

        end = playlist->priv->playlist_length - 1;

        if (x < 0 || y < 0) {
                do_not_move = TRUE;
        } else {
                /* get drop location */
                gtk_tree_view_get_dest_row_at_pos (GTK_TREE_VIEW (playlist->priv->tree), x, y, &path, &pos);

                if (path != NULL) {
                        /* grab drop localtion */
                        indice = gtk_tree_path_get_indices (path);
                        drop = indice[0];
                } else {
                        do_not_move = TRUE;
                }
        }

        connection = mpd_get_connection (playlist->priv->mpd);
        mpd_sendCommandListBegin (connection);

        temp_songs = songs;        
        /* For each filename :*/
        while (temp_songs) {
                /* Add it in the playlist*/
                mpd_sendAddCommand (connection, temp_songs->data);
                offset ++;
                /* move it in the right place */
                if (!do_not_move)
                        mpd_sendMoveCommand (connection, end + offset, drop + offset);

                temp_songs = g_list_next (temp_songs);
        }

        mpd_sendCommandListEnd (connection);
        mpd_finishCommand (connection);
}

static void
playlist_add_albums (Playlist *playlist,
                     GList *albums,
                     gint x, gint y)
{
        LOG_FUNCTION_START
        GList *filenames = NULL, *songs = NULL, *temp_albums, *temp_songs;
        Mpd_album *mpd_album;
        mpd_Song *mpd_song;

        temp_albums = albums;        
        /* For each album :*/
        while (temp_albums) {
                mpd_album = temp_albums->data;
                songs = mpd_get_songs (playlist->priv->mpd, mpd_album->artist, mpd_album->album);

                /* For each song */
                temp_songs = songs;
                while (temp_songs) {
                        mpd_song = temp_songs->data;
                        filenames = g_list_append (filenames, g_strdup (mpd_song->file));
                        temp_songs = g_list_next (temp_songs);
                }

                g_list_foreach (songs, (GFunc) mpd_freeSong, NULL);
                g_list_free (songs);
                temp_albums = g_list_next (temp_albums);
        }

        playlist_add_songs (playlist,
                            filenames,
                            x, y);

        g_list_foreach (filenames, (GFunc) g_free, NULL);
        g_list_free (filenames);
}

static void
playlist_add_artists (Playlist *playlist,
                      GList *artists,
                      gint x, gint y)
{
        LOG_FUNCTION_START
        GList *albums = NULL, *filenames = NULL, *songs = NULL, *temp_artists, *temp_albums, *temp_songs;
        Mpd_album *mpd_album;
        mpd_Song *mpd_song;

        temp_artists = artists;
        /* For each artist :*/
        while (temp_artists) {
                albums = mpd_get_albums (playlist->priv->mpd, temp_artists->data);
                /* For each album */
                temp_albums = albums;
                while (temp_albums) {
                        mpd_album = temp_albums->data;
                        songs = mpd_get_songs (playlist->priv->mpd, mpd_album->artist, mpd_album->album);

                        /* For each song */
                        temp_songs = songs;
                        while (temp_songs) {
                                mpd_song = temp_songs->data;
                                filenames = g_list_append (filenames, g_strdup (mpd_song->file));
                                temp_songs = g_list_next (temp_songs);
                        }
                        g_list_foreach (songs, (GFunc) mpd_freeSong, NULL);
                        g_list_free (songs);
                        temp_albums = g_list_next (temp_albums);
                }
                g_list_foreach (albums, (GFunc) mpd_free_album, NULL);
                g_list_free (albums);
                temp_artists = g_list_next (temp_artists);
        }

        playlist_add_songs (playlist,
                            filenames,
                            x, y);

        g_list_foreach (filenames, (GFunc) g_free, NULL);
        g_list_free (filenames);
}

static void
playlist_drop_songs (Playlist *playlist,
                     int x, int y,
                     GtkSelectionData *data)
{
        LOG_FUNCTION_START
        gchar **songs;
        GList *filenames = NULL;
        int i;

        songs = g_strsplit ((const gchar *) data->data, "\n", 0);

        /* For each filename :*/
        for (i=0; songs[i]!=NULL && g_utf8_collate (songs[i], ""); i++)
                filenames = g_list_append (filenames, songs[i]);

        playlist_add_songs (playlist,
                            filenames,
                            x, y);

        g_strfreev (songs);
        g_list_free (filenames);
}

static void
playlist_drop_albums (Playlist *playlist,
                      int x, int y,
                      GtkSelectionData *data)
{
        LOG_FUNCTION_START
        gchar **artists_albums;
        GList *albums_list = NULL;
        Mpd_album *mpd_album;
        int i;

        artists_albums = g_strsplit ((const gchar *) data->data, "\n", 0);

        /* For each album :*/
        for (i=0; artists_albums[i]!=NULL && g_utf8_collate (artists_albums[i], ""); i+=2) {
                mpd_album = (Mpd_album *) g_malloc (sizeof (Mpd_album));
                mpd_album->artist = artists_albums[i];
                mpd_album->album = artists_albums[i+1];
                albums_list = g_list_append (albums_list, mpd_album);
        }

        playlist_add_albums (playlist,
                             albums_list,
                             x, y);

        g_strfreev (artists_albums);

        g_list_foreach (albums_list, (GFunc) g_free, NULL);
        g_list_free (albums_list);
}

static void
playlist_drop_artists (Playlist *playlist,
                       int x, int y,
                       GtkSelectionData *data)
{
        LOG_FUNCTION_START
        gchar **artists;
        GList *artists_list = NULL;
        int i;

        artists = g_strsplit ((const gchar *) data->data, "\n", 0);

        /* For each artist :*/
        for (i=0; artists[i]!=NULL && g_utf8_collate (artists[i], ""); i++)
                artists_list = g_list_append (artists_list, artists[i]);

        playlist_add_artists (playlist,
                              artists_list,
                              x, y);

        g_strfreev (artists);
        g_list_free (artists_list);
}

void
playlist_append_songs (Playlist *playlist,
                       GList *songs)
{
        LOG_FUNCTION_START
        playlist_add_songs (playlist, songs, -1, -1);
}

void
playlist_append_albums (Playlist *playlist,
                        GList *albums)
{
        LOG_FUNCTION_START
        playlist_add_albums (playlist, albums, -1, -1);
}

void
playlist_append_artists (Playlist *playlist,
                         GList *artists)
{
        LOG_FUNCTION_START
        playlist_add_artists (playlist, artists, -1, -1);
}

static void
playlist_drag_leave_cb (GtkWidget *widget,
                        GdkDragContext *context,
                        gint x,
                        gint y,
                        GtkSelectionData *data,
                        guint info,
                        guint time,
                        gpointer user_data)
{
        LOG_FUNCTION_START
        Playlist *playlist = PLAYLIST (user_data);
        g_return_if_fail (IS_PLAYLIST (playlist));

        if (data->type == gdk_atom_intern ("text/internal-list", TRUE))
                playlist_move_rows (playlist, x, y);
        else if (data->type == gdk_atom_intern ("text/artists-list", TRUE))
                playlist_drop_artists (playlist, x, y, data);
        else if (data->type == gdk_atom_intern ("text/albums-list", TRUE))
                playlist_drop_albums (playlist, x, y, data);
        else if (data->type == gdk_atom_intern ("text/songs-list", TRUE))
                playlist_drop_songs (playlist, x, y, data);

        /* finish the drag */
        gtk_drag_finish (context, TRUE, FALSE, time);

        mpd_update_status (playlist->priv->mpd);
}

static void
playlist_drag_data_get_cb (GtkWidget * widget,
                           GdkDragContext * context,
                           GtkSelectionData * selection_data,
                           guint info, guint time, gpointer data)
{
        LOG_FUNCTION_START
        Playlist *playlist;

        playlist = PLAYLIST (data);

        g_return_if_fail (IS_PLAYLIST (playlist));
        g_return_if_fail (widget != NULL);
        g_return_if_fail (selection_data != NULL);


        gtk_selection_data_set (selection_data, selection_data->target, 8,
                                NULL, 0);
}

static void
playlist_popup_menu (Playlist *playlist)
{
        LOG_FUNCTION_START
        GtkWidget *menu;

        menu = gtk_ui_manager_get_widget (playlist->priv->ui_manager, "/PlaylistPopup");
        gtk_menu_popup (GTK_MENU (menu), NULL, NULL, NULL, NULL, 3, 
                        gtk_get_current_event_time ());
}

static void
playlist_cmd_clear (GtkAction *action,
                    Playlist *playlist)
{
        LOG_FUNCTION_START
        mpd_clear (playlist->priv->mpd);
        mpd_update_status (playlist->priv->mpd);
}

static void
playlist_selection_remove_foreach (GtkTreeModel *model,
                                   GtkTreePath *path,
                                   GtkTreeIter *iter,
                                   gpointer userdata)
{
        LOG_FUNCTION_START
        GArray *songs = (GArray *) userdata;
        gint *indice;

        indice = gtk_tree_path_get_indices (path);
        g_array_append_val (songs, indice[0]);
}

static void
playlist_remove (Playlist *playlist)
{
        LOG_FUNCTION_START
        GArray *songs;

        songs = g_array_new (TRUE, TRUE, sizeof (int));

        gtk_tree_selection_selected_foreach (playlist->priv->selection,
                                             playlist_selection_remove_foreach,
                                             songs);

        mpd_remove (playlist->priv->mpd, songs);

        g_array_free (songs, TRUE);

        mpd_update_status (playlist->priv->mpd);
        gtk_tree_selection_unselect_all (playlist->priv->selection);
}

static void
playlist_cmd_remove (GtkAction *action,
                     Playlist *playlist)
{
        LOG_FUNCTION_START
        playlist_remove (playlist);
}

static gboolean
playlist_view_key_press_cb (GtkWidget *widget,
                            GdkEventKey *event,
                            Playlist *playlist)
{
        LOG_FUNCTION_START
        if (event->keyval == GDK_Delete)
                playlist_remove (playlist);

        return FALSE;        
}

static gboolean
playlist_view_button_press_cb (GtkTreeView *treeview,
                               GdkEventButton *event,
                               Playlist *playlist)
{
        LOG_FUNCTION_START
        if (playlist->priv->dragging)
                return FALSE;

        if (event->state & GDK_CONTROL_MASK || event->state & GDK_SHIFT_MASK)
                return FALSE;

        if (event->button == 1) {
                GtkTreePath *path;

                gtk_tree_view_get_path_at_pos (treeview, event->x, event->y, &path, NULL, NULL, NULL);
                if (path != NULL) {
                        gboolean selected;
                        if (event->type == GDK_2BUTTON_PRESS)
                                playlist_activate_row (playlist, path);

                        selected = gtk_tree_selection_path_is_selected (playlist->priv->selection, path);

                        GdkModifierType mods;
                        GtkWidget *widget = GTK_WIDGET (treeview);
                        int x, y;

                        gdk_window_get_pointer (widget->window, &x, &y, &mods);
                        playlist->priv->drag_start_x = x;
                        playlist->priv->drag_start_y = y;
                        playlist->priv->pressed = TRUE;

                        gtk_tree_path_free (path);
                        if (selected)
                                return TRUE;
                        else
                                return FALSE;
                }
        }

        if (event->button == 3) {
                GtkTreePath *path;

                gtk_tree_view_get_path_at_pos (treeview, event->x, event->y, &path, NULL, NULL, NULL);
                if (path != NULL) {
                        if (!gtk_tree_selection_path_is_selected (playlist->priv->selection, path)) {
                                gtk_tree_selection_unselect_all (playlist->priv->selection);
                                gtk_tree_selection_select_path (playlist->priv->selection, path);
                        }
                        playlist_popup_menu (playlist);
                        gtk_tree_path_free (path);
                        return TRUE;
                }
        }

        return FALSE;
}


static gboolean
playlist_view_button_release_cb (GtkWidget *widget,
                                 GdkEventButton *event,
                                 Playlist *playlist)
{
        LOG_FUNCTION_START
        if (!playlist->priv->dragging && !(event->state & GDK_CONTROL_MASK) && !(event->state & GDK_SHIFT_MASK)) {
                GtkTreePath *path;

                gtk_tree_view_get_path_at_pos (GTK_TREE_VIEW (widget), event->x, event->y, &path, NULL, NULL, NULL);
                if (path != NULL) {
                        GtkTreeSelection *selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (widget));
                        gtk_tree_selection_unselect_all (selection);
                        gtk_tree_selection_select_path (selection, path);
                        gtk_tree_path_free (path);
                }
        }

        playlist->priv->dragging = FALSE;
        playlist->priv->pressed = FALSE;

        return FALSE;
}

static gboolean
playlist_view_motion_notify (GtkWidget *widget, 
                             GdkEventMotion *event,
                             Playlist *playlist)
{
        // desactivated to make the logs more readable
        // LOG_FUNCTION_START
        GdkModifierType mods;
        int x, y;
        int dx, dy;

        if ((playlist->priv->dragging) || !(playlist->priv->pressed))
                return FALSE;

        gdk_window_get_pointer (widget->window, &x, &y, &mods);

        dx = x - playlist->priv->drag_start_x;
        dy = y - playlist->priv->drag_start_y;

        if ((util_abs (dx) > DRAG_THRESHOLD) || (util_abs (dy) > DRAG_THRESHOLD))
                playlist->priv->dragging = TRUE;

        return FALSE;
}
