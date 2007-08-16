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
#include <i18n.h>
#include <string.h>

#include "browser.h"
#include "libmpdclient.h"
#include "util.h"
#include "cover.h"
#include "shell-coverselect.h"
#include "shell-coverdownloader.h"
#include "eel/eel-gconf-extensions.h"
#include "preferences.h"
#include "debug.h"

#define DRAG_THRESHOLD 1

static void browser_class_init (BrowserClass *klass);
static void browser_init (Browser *browser);
static void browser_finalize (GObject *object);
static void browser_set_property (GObject *object,
                                         guint prop_id,
                                         const GValue *value,
                                         GParamSpec *pspec);
static void browser_get_property (GObject *object,
                                         guint prop_id,
                                         GValue *value,
                                         GParamSpec *pspec);
static void browser_state_changed_cb (Mpd *mpd,
                                      Browser *browser);
static void browser_dbtime_changed_cb (Mpd *mpd,
                                       Browser *browser);
static void browser_fill_artists (Browser *browser);
static void browser_artists_selection_changed_cb (GtkTreeSelection * selection,
                                                  Browser *browser);
static void browser_albums_selection_changed_cb (GtkTreeSelection * selection,
                                                 Browser *browser);
static void browser_artists_drag_data_get_cb (GtkWidget * widget,
                                              GdkDragContext * context,
                                               GtkSelectionData * selection_data,
                                               guint info, guint time, gpointer data);
static void browser_albums_drag_data_get_cb (GtkWidget * widget,
                                             GdkDragContext * context,
                                              GtkSelectionData * selection_data,
                                              guint info, guint time, gpointer data);
static void browser_songs_drag_data_get_cb (GtkWidget * widget,
                                            GdkDragContext * context,
                                             GtkSelectionData * selection_data,
                                             guint info, guint time, gpointer data);
static gboolean browser_button_press_cb (GtkWidget *widget,
                                           GdkEventButton *event,
                                         Browser *browser);
static gboolean browser_button_release_cb (GtkWidget *widget,
                                           GdkEventButton *event,
                                           Browser *browser);
static gboolean browser_motion_notify (GtkWidget *widget, 
                                       GdkEventMotion *event,
                                       Browser *browser);
static void browser_cmd_add_artists (GtkAction *action,
                                     Browser *browser);
static void browser_cmd_add_albums (GtkAction *action,
                                    Browser *browser);
static void browser_cmd_add_songs (GtkAction *action,
                                   Browser *browser);
static void browser_add_in_playlist (Browser *browser);
static void browser_cmd_get_artist_cover_amazon (GtkAction *action,
                                                   Browser *browser);
static void browser_cmd_remove_artist_cover (GtkAction *action,
                                             Browser *browser);
static void browser_cmd_get_album_cover_amazon (GtkAction *action,
                                                  Browser *browser);
static void browser_cmd_remove_album_cover (GtkAction *action,
                                            Browser *browser);
static void browser_covertree_visible_changed_cb (GConfClient *client,
                                                      guint cnxn_id,
                                                  GConfEntry *entry,
                                                  GtkTreeView *treeview);

struct BrowserPrivate
{        
        GtkWidget *artists;
        GtkWidget *albums;
        GtkWidget *songs;

        GtkListStore *artists_model;
        GtkListStore *albums_model;
        GtkListStore *songs_model;

        GtkTreeSelection *artists_selection;
        GtkTreeSelection *albums_selection;
        GtkTreeSelection *songs_selection;

        gboolean connected;

        gboolean dragging;
        gboolean pressed;
        gint drag_start_x;
        gint drag_start_y;

        Mpd *mpd;
        Playlist *playlist;
        GtkUIManager *ui_manager;
        GtkActionGroup *actiongroup;
};

static GtkActionEntry browser_actions [] =
{
        { "BrowserAddArtists", GTK_STOCK_ADD, N_("_Add to playlist"), NULL,
          N_("Add to the playlist"),
          G_CALLBACK (browser_cmd_add_artists) },
        { "BrowserAddAlbums", GTK_STOCK_ADD, N_("_Add to playlist"), NULL,
          N_("Add to the playlist"),
          G_CALLBACK (browser_cmd_add_albums) },
        { "BrowserAddSongs", GTK_STOCK_ADD, N_("_Add to playlist"), NULL,
          N_("Add to the playlist"),
          G_CALLBACK (browser_cmd_add_songs) },

        { "CoverArtistGetAmazon", GTK_STOCK_FIND, N_("Get the covers from _Amazon"), NULL,
          N_("Download the cover from Amazon"),
          G_CALLBACK (browser_cmd_get_artist_cover_amazon) },
        { "CoverArtistRemove", GTK_STOCK_DELETE, N_("_Delete the covers"), NULL,
          N_("Delete the selected covers"),
          G_CALLBACK (browser_cmd_remove_artist_cover) },

        { "CoverAlbumGetAmazon", GTK_STOCK_FIND, N_("Get the covers from _Amazon"), NULL,
          N_("Download the cover from Amazon"),
          G_CALLBACK (browser_cmd_get_album_cover_amazon) },
        { "CoverAlbumRemove", GTK_STOCK_DELETE, N_("_Delete the covers"), NULL,
          N_("Delete the selected covers"),
          G_CALLBACK (browser_cmd_remove_album_cover) },
};

static guint browser_n_actions = G_N_ELEMENTS (browser_actions);

enum
{
        PROP_0,
        PROP_MPD,
        PROP_PLAYLIST,
        PROP_UI_MANAGER,
        PROP_ACTION_GROUP
};

enum
{
        ALBUM_ALBUM_COLUMN,
        ALBUM_ARTIST_COLUMN,
        ALBUM_COVER_COLUMN,
        ALBUM_N_COLUMN
};

enum
{
        TRACK_COLUMN,
        TITLE_COLUMN,
        FILENAME_COLUMN,
        N_COLUMN
};

static const GtkTargetEntry songs_targets  [] = {
        { "text/songs-list", 0, 0 },
};

static const GtkTargetEntry albums_targets  [] = {
        { "text/albums-list", 0, 0 },
};

static const GtkTargetEntry artists_targets  [] = {
        { "text/artists-list", 0, 0 },
};

static GObjectClass *parent_class = NULL;

GType
browser_get_type (void)
{
        LOG_FUNCTION_START
        static GType type = 0;

        if (!type) {
                static const GTypeInfo our_info =
                {
                        sizeof (BrowserClass),
                        NULL,
                        NULL,
                        (GClassInitFunc) browser_class_init,
                        NULL,
                        NULL,
                        sizeof (Browser),
                        0,
                        (GInstanceInitFunc) browser_init
                };

                type = g_type_register_static (GTK_TYPE_HBOX,
                                               "Browser",
                                                &our_info, 0);
        }
        return type;
}

static void
browser_class_init (BrowserClass *klass)
{
        LOG_FUNCTION_START
        GObjectClass *object_class = G_OBJECT_CLASS (klass);

        parent_class = g_type_class_peek_parent (klass);

        object_class->finalize = browser_finalize;

        object_class->set_property = browser_set_property;
        object_class->get_property = browser_get_property;

        g_object_class_install_property (object_class,
                                         PROP_MPD,
                                         g_param_spec_object ("mpd",
                                                              "mpd",
                                                              "mpd",
                                                              TYPE_MPD,
                                                              G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
        g_object_class_install_property (object_class,
                                         PROP_PLAYLIST,
                                         g_param_spec_object ("playlist",
                                                              "playlist",
                                                              "playlist",
                                                              TYPE_PLAYLIST,
                                                              G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
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
browser_init (Browser *browser)
{
        LOG_FUNCTION_START
        GtkTreeViewColumn *column;
        GtkCellRenderer *renderer;

        GtkWidget *scrolledwindow_artists;
        GtkWidget *scrolledwindow_albums;
        GtkWidget *scrolledwindow_songs;

        browser->priv = g_new0 (BrowserPrivate, 1);

        browser->priv->connected = FALSE;

        /* Artists list */
        scrolledwindow_artists = gtk_scrolled_window_new (NULL, NULL);
        gtk_widget_show (scrolledwindow_artists);
        gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolledwindow_artists), GTK_POLICY_NEVER, GTK_POLICY_ALWAYS);
        gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scrolledwindow_artists), GTK_SHADOW_IN);
        browser->priv->artists = gtk_tree_view_new ();
        renderer = gtk_cell_renderer_text_new ();
        column = gtk_tree_view_column_new_with_attributes (_("Artist"),
                                                                  renderer,
                                                                  "text", 0,
                                                                  NULL);
        gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_FIXED);
        gtk_tree_view_append_column (GTK_TREE_VIEW (browser->priv->artists), column);
        browser->priv->artists_model = gtk_list_store_new (1, G_TYPE_STRING);
        gtk_tree_sortable_set_sort_column_id (GTK_TREE_SORTABLE (browser->priv->artists_model),
                                              0, GTK_SORT_ASCENDING);
        gtk_tree_view_set_model (GTK_TREE_VIEW (browser->priv->artists),
                                 GTK_TREE_MODEL (browser->priv->artists_model));
        browser->priv->artists_selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (browser->priv->artists));
        gtk_tree_selection_set_mode (browser->priv->artists_selection,
                                     GTK_SELECTION_MULTIPLE);
        g_signal_connect_object (G_OBJECT (browser->priv->artists_selection),
                                 "changed",
                                 G_CALLBACK (browser_artists_selection_changed_cb),
                                 browser, 0);
        gtk_container_add (GTK_CONTAINER (scrolledwindow_artists), browser->priv->artists);

        gtk_drag_source_set (browser->priv->artists,
                             GDK_BUTTON1_MASK,
                             artists_targets,
                             G_N_ELEMENTS (artists_targets),
                             GDK_ACTION_COPY);

        g_signal_connect (GTK_TREE_VIEW (browser->priv->artists),
                          "drag_data_get", 
                          G_CALLBACK (browser_artists_drag_data_get_cb), browser);
        g_signal_connect_object (G_OBJECT (browser->priv->artists),
                                 "button_press_event",
                                 G_CALLBACK (browser_button_press_cb),
                                 browser,
                                 0);
        g_signal_connect_object (G_OBJECT (browser->priv->artists),
                                 "button_release_event",
                                 G_CALLBACK (browser_button_release_cb),
                                 browser,
                                 0);
        g_signal_connect_object (G_OBJECT (browser->priv->artists),
                                 "motion_notify_event",
                                 G_CALLBACK (browser_motion_notify),
                                 browser,
                                 0);

        /* Albums list */
        scrolledwindow_albums = gtk_scrolled_window_new (NULL, NULL);
        gtk_widget_show (scrolledwindow_albums);
        gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolledwindow_albums), GTK_POLICY_NEVER, GTK_POLICY_ALWAYS);
        gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scrolledwindow_albums), GTK_SHADOW_IN);
        browser->priv->albums = gtk_tree_view_new ();
        //gtk_tree_view_set_fixed_height_mode (GTK_TREE_VIEW (browser->priv->albums), TRUE);
                /* Text column */
        renderer = gtk_cell_renderer_text_new ();
        column = gtk_tree_view_column_new_with_attributes (_("Album"),
                                                           renderer,
                                                           "text", ALBUM_ALBUM_COLUMN,
                                                           NULL);
        gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_FIXED);
        gtk_tree_view_column_set_expand (column, TRUE);
        gtk_tree_view_append_column (GTK_TREE_VIEW (browser->priv->albums), column);
                /* Cover column */
        renderer = gtk_cell_renderer_pixbuf_new ();
        column = gtk_tree_view_column_new_with_attributes (_("Cover"), 
                                                          renderer, 
                                                          "pixbuf", 
                                                          ALBUM_COVER_COLUMN, NULL);        
        gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_FIXED);
        gtk_tree_view_column_set_fixed_width (column, COVER_SIZE + 30);
        gtk_tree_view_column_set_spacing (column, 0);
        gtk_tree_view_append_column (GTK_TREE_VIEW (browser->priv->albums), 
                                     column);
        gtk_tree_view_column_set_visible (column,
                                          !eel_gconf_get_boolean (CONF_COVER_TREE_HIDDEN));
                /* Model */
        browser->priv->albums_model = gtk_list_store_new (ALBUM_N_COLUMN,
                                                          G_TYPE_STRING,
                                                          G_TYPE_STRING,
                                                          GDK_TYPE_PIXBUF);
        gtk_tree_sortable_set_sort_column_id (GTK_TREE_SORTABLE (browser->priv->albums_model),
                                              0, GTK_SORT_ASCENDING);
        gtk_tree_view_set_model (GTK_TREE_VIEW (browser->priv->albums),
                                 GTK_TREE_MODEL (browser->priv->albums_model));
        browser->priv->albums_selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (browser->priv->albums));
        gtk_tree_selection_set_mode (browser->priv->albums_selection,
                                     GTK_SELECTION_MULTIPLE);
        g_signal_connect_object (G_OBJECT (browser->priv->albums_selection),
                                 "changed",
                                 G_CALLBACK (browser_albums_selection_changed_cb),
                                 browser, 0);
        gtk_container_add (GTK_CONTAINER (scrolledwindow_albums), browser->priv->albums);

        gtk_drag_source_set (browser->priv->albums,
                             GDK_BUTTON1_MASK,
                             albums_targets,
                             G_N_ELEMENTS (albums_targets),
                             GDK_ACTION_COPY);

        g_signal_connect (GTK_TREE_VIEW (browser->priv->albums),
                          "drag_data_get", 
                          G_CALLBACK (browser_albums_drag_data_get_cb), browser);
        g_signal_connect_object (G_OBJECT (browser->priv->albums),
                                 "button_press_event",
                                 G_CALLBACK (browser_button_press_cb),
                                 browser,
                                 0);
        g_signal_connect_object (G_OBJECT (browser->priv->albums),
                                 "button_release_event",
                                 G_CALLBACK (browser_button_release_cb),
                                 browser,
                                 0);
        g_signal_connect_object (G_OBJECT (browser->priv->albums),
                                 "motion_notify_event",
                                 G_CALLBACK (browser_motion_notify),
                                 browser,
                                 0);
        eel_gconf_notification_add (CONF_COVER_TREE_HIDDEN,
                                    (GConfClientNotifyFunc) browser_covertree_visible_changed_cb,
                                    GTK_TREE_VIEW (browser->priv->albums)); 

        /* Songs list */
        scrolledwindow_songs = gtk_scrolled_window_new (NULL, NULL);
        gtk_widget_show (scrolledwindow_songs);
        gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolledwindow_songs), GTK_POLICY_NEVER, GTK_POLICY_ALWAYS);
        gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scrolledwindow_songs), GTK_SHADOW_IN);
        browser->priv->songs = gtk_tree_view_new ();
                /* Track column */
        renderer = gtk_cell_renderer_text_new ();
        column = gtk_tree_view_column_new_with_attributes (_("Track"),
                                                                  renderer,
                                                                  "text", TRACK_COLUMN,
                                                                  NULL);
        gtk_tree_view_append_column (GTK_TREE_VIEW (browser->priv->songs), column);
                /* Title column */
        renderer = gtk_cell_renderer_text_new ();
        column = gtk_tree_view_column_new_with_attributes (_("Title"),
                                                                  renderer,
                                                                  "text", TITLE_COLUMN,
                                                                  NULL);
        gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_FIXED);
        gtk_tree_view_append_column (GTK_TREE_VIEW (browser->priv->songs), column);

        browser->priv->songs_model = gtk_list_store_new (N_COLUMN,
                                                         G_TYPE_STRING,
                                                         G_TYPE_STRING,
                                                         G_TYPE_STRING);
        gtk_tree_view_set_model (GTK_TREE_VIEW (browser->priv->songs),
                                 GTK_TREE_MODEL (browser->priv->songs_model));
        browser->priv->songs_selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (browser->priv->songs));
        gtk_tree_selection_set_mode (browser->priv->songs_selection,
                                     GTK_SELECTION_MULTIPLE);
        gtk_container_add (GTK_CONTAINER (scrolledwindow_songs), browser->priv->songs);

        gtk_drag_source_set (browser->priv->songs,
                             GDK_BUTTON1_MASK,
                             songs_targets,
                             G_N_ELEMENTS (songs_targets),
                             GDK_ACTION_COPY);

        g_signal_connect (GTK_TREE_VIEW (browser->priv->songs),
                          "drag_data_get", 
                          G_CALLBACK (browser_songs_drag_data_get_cb), browser);
        g_signal_connect_object (G_OBJECT (browser->priv->songs),
                                 "button_press_event",
                                 G_CALLBACK (browser_button_press_cb),
                                 browser,
                                 0);
        g_signal_connect_object (G_OBJECT (browser->priv->songs),
                                 "button_release_event",
                                 G_CALLBACK (browser_button_release_cb),
                                 browser,
                                 0);
        g_signal_connect_object (G_OBJECT (browser->priv->songs),
                                 "motion_notify_event",
                                 G_CALLBACK (browser_motion_notify),
                                 browser,
                                 0);

        gtk_tree_selection_set_mode (browser->priv->songs_selection,
                                     GTK_SELECTION_MULTIPLE);
        /* Hbox properties */
        gtk_box_set_homogeneous (GTK_BOX (browser), TRUE);
        gtk_box_set_spacing (GTK_BOX (browser), 4);

        gtk_box_pack_start (GTK_BOX (browser), scrolledwindow_artists, TRUE, TRUE, 0);
        gtk_box_pack_start (GTK_BOX (browser), scrolledwindow_albums, TRUE, TRUE, 0);
        gtk_box_pack_start (GTK_BOX (browser), scrolledwindow_songs, TRUE, TRUE, 0);
}

static void
browser_finalize (GObject *object)
{
        LOG_FUNCTION_START
        Browser *browser;

        g_return_if_fail (object != NULL);
        g_return_if_fail (IS_BROWSER (object));

        browser = BROWSER (object);

        g_return_if_fail (browser->priv != NULL);

        g_free (browser->priv);

        G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
browser_set_property (GObject *object,
                             guint prop_id,
                             const GValue *value,
                             GParamSpec *pspec)
{
        LOG_FUNCTION_START
        Browser *browser = BROWSER (object);
        
        switch (prop_id) {
        case PROP_MPD:
                browser->priv->mpd = g_value_get_object (value);

                /* Signals to synchronize the browser with mpd */
                g_signal_connect_object (G_OBJECT (browser->priv->mpd),
                                         "state_changed", G_CALLBACK (browser_state_changed_cb),
                                         browser, 0);

                g_signal_connect_object (G_OBJECT (browser->priv->mpd),
                                         "dbtime_changed", G_CALLBACK (browser_dbtime_changed_cb),
                                         browser, 0);
                break;
        case PROP_PLAYLIST:
                browser->priv->playlist = g_value_get_object (value);
                break;
        case PROP_UI_MANAGER:
                browser->priv->ui_manager = g_value_get_object (value);
                break;
        case PROP_ACTION_GROUP:
                browser->priv->actiongroup = g_value_get_object (value);
                gtk_action_group_add_actions (browser->priv->actiongroup,
                                              browser_actions,
                                              browser_n_actions, browser);
                break;
        default:
                G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
                break;
        }
}

static void 
browser_get_property (GObject *object,
                        guint prop_id,
                        GValue *value,
                        GParamSpec *pspec)
{
        LOG_FUNCTION_START
        Browser *browser = BROWSER (object);

        switch (prop_id) {
        case PROP_MPD:
                g_value_set_object (value, browser->priv->mpd);
                break;
        case PROP_PLAYLIST:
                g_value_set_object (value, browser->priv->playlist);
                break;
        case PROP_UI_MANAGER:
                g_value_set_object (value, browser->priv->ui_manager);
                break;
        case PROP_ACTION_GROUP:
                g_value_set_object (value, browser->priv->actiongroup);
                break;
        default:
                G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
                break;
        }
}

GtkWidget *
browser_new (GtkUIManager *mgr,
             GtkActionGroup *group,
             Mpd *mpd,
             Playlist *playlist)
{
        LOG_FUNCTION_START
        Browser *browser;

        browser = g_object_new (TYPE_BROWSER,
                                "ui-manager", mgr,
                                "action-group", group,
                                "mpd", mpd,
                                "playlist", playlist,
                                NULL);

        g_return_val_if_fail (browser->priv != NULL, NULL);

        return GTK_WIDGET (browser);
}

static void
browser_state_changed_cb (Mpd *mpd,
                          Browser *browser)
{
        LOG_FUNCTION_START
        if (browser->priv->connected != mpd_is_connected (mpd))
                browser_fill_artists (browser);

        browser->priv->connected = mpd_is_connected (mpd);
}

static void
browser_dbtime_changed_cb (Mpd *mpd,
                           Browser *browser)
{
        LOG_FUNCTION_START
        browser_fill_artists (browser);
}

static void
browser_fill_artists (Browser *browser)
{
        LOG_FUNCTION_START
        GList *artists;
        GList *temp;
        GtkTreeIter artist_iter;

        g_signal_handlers_block_by_func (G_OBJECT (browser->priv->artists_selection),
                                         G_CALLBACK (browser_artists_selection_changed_cb),
                                         browser);

        gtk_list_store_clear (browser->priv->artists_model);

        artists = mpd_get_artists (browser->priv->mpd);

        temp = artists;
        while (temp) {
                gtk_list_store_append (browser->priv->artists_model, &artist_iter);
                gtk_list_store_set (browser->priv->artists_model, &artist_iter, 0,
                                    temp->data, -1);
                temp = g_list_next (temp);
        }
        g_list_foreach(artists, (GFunc) g_free, NULL);
        g_list_free (artists);

        gtk_tree_selection_unselect_all (browser->priv->artists_selection);
        if (gtk_tree_model_get_iter_first (GTK_TREE_MODEL (browser->priv->artists_model), &artist_iter))
                gtk_tree_selection_select_iter (browser->priv->artists_selection, &artist_iter);
        g_signal_handlers_unblock_by_func (G_OBJECT (browser->priv->artists_selection),
                                           G_CALLBACK (browser_artists_selection_changed_cb),
                                           browser);

        g_signal_emit_by_name (G_OBJECT (browser->priv->artists_selection), "changed", 0);
}

static void
browser_artists_selection_foreach (GtkTreeModel *model,
                                   GtkTreePath *path,
                                   GtkTreeIter *iter,
                                   gpointer userdata)
{
        LOG_FUNCTION_START
        Browser *browser = BROWSER (userdata);
        gchar* artist = NULL;
        GList *albums = NULL, *temp;
        Mpd_album *mpd_album;
        GtkTreeIter album_iter;
        gchar *cover_path;
        GdkPixbuf *cover;

        g_return_if_fail (IS_BROWSER (browser));

        gtk_tree_model_get (model, iter, 0, &artist, -1);

        if (!artist)
                return;

        albums = mpd_get_albums (browser->priv->mpd, artist);
        g_free (artist);

        temp = albums;
        while (temp) {
                mpd_album = temp->data;

                cover_path = cover_make_cover_path (mpd_album->artist, mpd_album->album, SMALL_COVER);

                /* The small cover exists, we show it */
                cover = gdk_pixbuf_new_from_file_at_size (cover_path, COVER_SIZE, COVER_SIZE, NULL);
                g_free (cover_path);

                if (!GDK_IS_PIXBUF (cover)) {
                        /* There is no cover, we show a transparent picture */
                        cover = gdk_pixbuf_new (GDK_COLORSPACE_RGB, TRUE, 8, COVER_SIZE, COVER_SIZE);
                        gdk_pixbuf_fill (cover, 0xffffff00);
                }

                gtk_list_store_append (browser->priv->albums_model, &album_iter);

                gtk_list_store_set (browser->priv->albums_model, &album_iter,
                                    ALBUM_ALBUM_COLUMN, mpd_album->album,
                                    ALBUM_ARTIST_COLUMN, mpd_album->artist,
                                    ALBUM_COVER_COLUMN, cover,
                                    -1);
                g_object_unref (G_OBJECT (cover));
                temp = g_list_next (temp);
        }


        g_list_foreach (albums, (GFunc) mpd_free_album, NULL);
        g_list_free (albums);
}

static void
browser_artists_selection_update (Browser *browser)
{
        LOG_FUNCTION_START
        GtkTreeIter album_iter;

        g_signal_handlers_block_by_func (G_OBJECT (browser->priv->albums_selection),
                                         G_CALLBACK (browser_albums_selection_changed_cb),
                                         browser);

        gtk_list_store_clear (browser->priv->albums_model);

        gtk_tree_selection_selected_foreach (browser->priv->artists_selection,
                                             browser_artists_selection_foreach,
                                             browser);

        gtk_tree_selection_unselect_all (browser->priv->albums_selection);
        if (gtk_tree_model_get_iter_first (GTK_TREE_MODEL (browser->priv->albums_model), &album_iter))
                gtk_tree_selection_select_iter (browser->priv->albums_selection, &album_iter);
        g_signal_handlers_unblock_by_func (G_OBJECT (browser->priv->albums_selection),
                                           G_CALLBACK (browser_albums_selection_changed_cb),
                                           browser);

        g_signal_emit_by_name (G_OBJECT (browser->priv->albums_selection), "changed", 0);
}

static void
browser_artists_selection_changed_cb (GtkTreeSelection * selection, Browser *browser)
{
        LOG_FUNCTION_START
        browser_artists_selection_update (browser);
}

static void
browser_albums_selection_foreach (GtkTreeModel *model,
                                  GtkTreePath *path,
                                  GtkTreeIter *iter,
                                  gpointer userdata)
{
        LOG_FUNCTION_START
        Browser *browser = BROWSER (userdata);
        gchar *artist = NULL, *album = NULL;
        GList *songs = NULL, *temp;
        mpd_Song *song;
        GtkTreeIter songs_iter;
        gchar *track;

        g_return_if_fail (IS_BROWSER (browser));

        gtk_tree_model_get (model, iter, ALBUM_ARTIST_COLUMN, &artist, -1);
        gtk_tree_model_get (model, iter, ALBUM_ALBUM_COLUMN, &album, -1);

        if (!artist || !album)
                return;

        songs = mpd_get_songs (browser->priv->mpd, artist, album);
        g_free (artist);
        g_free (album);


        temp = songs;
        while (temp) {
                song = temp->data;
                gtk_list_store_append (browser->priv->songs_model, &songs_iter);

                track = util_format_track (song->track);
                if (song->title)
                        gtk_list_store_set (browser->priv->songs_model, &songs_iter,
                                            TRACK_COLUMN, track,
                                            TITLE_COLUMN, song->title,
                                            FILENAME_COLUMN, song->file,
                                            -1);
                else {
                        gchar *title = util_format_title (song->file);
                        gtk_list_store_set (browser->priv->songs_model, &songs_iter,
                                            TRACK_COLUMN, track,
                                            TITLE_COLUMN, title,
                                            FILENAME_COLUMN, song->file,
                                            -1);
                        g_free (title);
                }
                g_free (track);
                temp = g_list_next (temp);
        }

        g_list_foreach (songs, (GFunc) mpd_freeSong, NULL);
        g_list_free (songs);
}

static void
browser_albums_selection_update (Browser *browser)
{
        LOG_FUNCTION_START
        GtkTreeIter song_iter;

        gtk_list_store_clear (browser->priv->songs_model);

        gtk_tree_selection_selected_foreach (browser->priv->albums_selection,
                                             browser_albums_selection_foreach,
                                             browser);

        gtk_tree_selection_unselect_all (browser->priv->songs_selection);

        if (gtk_tree_model_get_iter_first (GTK_TREE_MODEL (browser->priv->songs_model), &song_iter))
                gtk_tree_selection_select_iter (browser->priv->songs_selection, &song_iter);
}

static void
browser_albums_selection_changed_cb (GtkTreeSelection * selection, Browser *browser)
{
        LOG_FUNCTION_START
        browser_albums_selection_update (browser);
}

static void
browser_artists_selection_drag_foreach (GtkTreeModel *model,
                                        GtkTreePath *path,
                                        GtkTreeIter *iter,
                                        gpointer userdata)
{
        LOG_FUNCTION_START
        GString *artists = (GString *) userdata;
        g_return_if_fail (artists != NULL);

        gchar* val = NULL;

        gtk_tree_model_get (model, iter, 0, &val, -1);
        g_string_append (artists, val);
        g_string_append (artists, "\n");
        g_free (val);
}

static void
browser_artists_drag_data_get_cb (GtkWidget * widget,
                                  GdkDragContext * context,
                                  GtkSelectionData * selection_data,
                                  guint info, guint time, gpointer data)
{
        LOG_FUNCTION_START
        Browser *browser;
        GString* artists = NULL;

        browser = BROWSER (data);

        g_return_if_fail (IS_BROWSER (browser));
        g_return_if_fail (widget != NULL);
        g_return_if_fail (selection_data != NULL);

        artists = g_string_new("");
        gtk_tree_selection_selected_foreach (browser->priv->artists_selection,
                                             browser_artists_selection_drag_foreach,
                                             artists);

        gtk_selection_data_set (selection_data, selection_data->target, 8, (const guchar *) artists->str,
                                strlen (artists->str) * sizeof(guchar));
        
        g_string_free (artists, TRUE);
}

static void
browser_albums_selection_drag_foreach (GtkTreeModel *model,
                                       GtkTreePath *path,
                                       GtkTreeIter *iter,
                                       gpointer userdata)
{
        LOG_FUNCTION_START
        GString *albums = (GString *) userdata;
        g_return_if_fail (albums != NULL);

        gchar* val = NULL;

        gtk_tree_model_get (model, iter, ALBUM_ARTIST_COLUMN, &val, -1);
        g_string_append (albums, val);
        g_string_append (albums, "\n");
        g_free (val);

        gtk_tree_model_get (model, iter, ALBUM_ALBUM_COLUMN, &val, -1);
        g_string_append (albums, val);
        g_string_append (albums, "\n");
        g_free (val);
}

static void
browser_albums_drag_data_get_cb (GtkWidget * widget,
                                 GdkDragContext * context,
                                 GtkSelectionData * selection_data,
                                 guint info, guint time, gpointer data)
{
        LOG_FUNCTION_START
        Browser *browser;
        GString* albums = NULL;

        browser = BROWSER (data);

        g_return_if_fail (IS_BROWSER (browser));
        g_return_if_fail (widget != NULL);
        g_return_if_fail (selection_data != NULL);

        albums = g_string_new("");
        gtk_tree_selection_selected_foreach (browser->priv->albums_selection,
                                             browser_albums_selection_drag_foreach,
                                             albums);

        gtk_selection_data_set (selection_data, selection_data->target, 8, (const guchar *) albums->str,
                                strlen (albums->str) * sizeof(guchar));
        
        g_string_free (albums, TRUE);
}

static void
browser_songs_selection_drag_foreach (GtkTreeModel *model,
                                      GtkTreePath *path,
                                      GtkTreeIter *iter,
                                      gpointer userdata)
{
        LOG_FUNCTION_START
        GString *filenames = (GString *) userdata;
        g_return_if_fail (filenames != NULL);

        gchar* val = NULL;

        gtk_tree_model_get (model, iter, FILENAME_COLUMN, &val, -1);
        g_string_append (filenames, val);
        g_string_append (filenames, "\n");

        g_free (val);
}

static void
browser_songs_drag_data_get_cb (GtkWidget * widget,
                                GdkDragContext * context,
                                GtkSelectionData * selection_data,
                                guint info, guint time, gpointer data)
{
        LOG_FUNCTION_START
        Browser *browser;
        GString* filenames = NULL;

        browser = BROWSER (data);

        g_return_if_fail (IS_BROWSER (browser));
        g_return_if_fail (widget != NULL);
        g_return_if_fail (selection_data != NULL);

        filenames = g_string_new("");
        gtk_tree_selection_selected_foreach (browser->priv->songs_selection,
                                             browser_songs_selection_drag_foreach,
                                             filenames);

        gtk_selection_data_set (selection_data, selection_data->target, 8, (const guchar *) filenames->str,
                                strlen (filenames->str) * sizeof(guchar));
        
        g_string_free (filenames, TRUE);
}

static void
browser_popup_menu (Browser *browser)
{
        LOG_FUNCTION_START
        GtkWidget *menu;

        if (GTK_WIDGET_HAS_FOCUS (GTK_WIDGET (browser->priv->artists))) {
                menu = gtk_ui_manager_get_widget (browser->priv->ui_manager, "/BrowserArtistsPopup");
                gtk_menu_popup (GTK_MENU (menu), NULL, NULL, NULL, NULL, 3, 
                                gtk_get_current_event_time ());
        }

        if (GTK_WIDGET_HAS_FOCUS (GTK_WIDGET (browser->priv->albums))) {
                menu = gtk_ui_manager_get_widget (browser->priv->ui_manager, "/BrowserAlbumsPopup");
                gtk_menu_popup (GTK_MENU (menu), NULL, NULL, NULL, NULL, 3, 
                                gtk_get_current_event_time ());
        }

        if (GTK_WIDGET_HAS_FOCUS (GTK_WIDGET (browser->priv->songs))) {
                menu = gtk_ui_manager_get_widget (browser->priv->ui_manager, "/BrowserSongsPopup");
                gtk_menu_popup (GTK_MENU (menu), NULL, NULL, NULL, NULL, 3, 
                                gtk_get_current_event_time ());
        }
}

static gboolean
browser_button_press_cb (GtkWidget *widget,
                         GdkEventButton *event,
                         Browser *browser)
{
        LOG_FUNCTION_START
        if (!GTK_WIDGET_HAS_FOCUS (widget))
                gtk_widget_grab_focus (widget);

        if (browser->priv->dragging)
                return FALSE;

        if (event->state & GDK_CONTROL_MASK || event->state & GDK_SHIFT_MASK)
                return FALSE;

        if (event->button == 1 && event->type == GDK_2BUTTON_PRESS)
                browser_add_in_playlist (browser);

        if (event->button == 1) {
                GtkTreePath *path;

                gtk_tree_view_get_path_at_pos (GTK_TREE_VIEW (widget), event->x, event->y, &path, NULL, NULL, NULL);
                if (path != NULL) {
                        gboolean selected;
                        GtkTreeSelection *selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (widget));
                        selected = gtk_tree_selection_path_is_selected (selection, path);
                        if (!selected) {
                                gtk_tree_selection_unselect_all (selection);
                                gtk_tree_selection_select_path (selection, path);
                        }

                        GdkModifierType mods;
                        int x, y;

                        gdk_window_get_pointer (widget->window, &x, &y, &mods);
                        browser->priv->drag_start_x = x;
                        browser->priv->drag_start_y = y;
                        browser->priv->pressed = TRUE;

                        gtk_tree_path_free (path);
                        if (selected)
                                return TRUE;
                        else
                                return FALSE;
                }
        }

        if (event->button == 3) {
                GtkTreePath *path;

                gtk_tree_view_get_path_at_pos (GTK_TREE_VIEW (widget), event->x, event->y, &path, NULL, NULL, NULL);
                if (path != NULL) {
                        GtkTreeSelection *selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (widget));
                        if (!gtk_tree_selection_path_is_selected (selection, path)) {
                                gtk_tree_selection_unselect_all (selection);
                                gtk_tree_selection_select_path (selection, path);
                        }
                        browser_popup_menu (browser);
                        gtk_tree_path_free (path);
                        return TRUE;
                }
        }

        return FALSE;
}

static gboolean
browser_button_release_cb (GtkWidget *widget,
                           GdkEventButton *event,
                           Browser *browser)
{
        LOG_FUNCTION_START
        if (!browser->priv->dragging && !(event->state & GDK_CONTROL_MASK) && !(event->state & GDK_SHIFT_MASK)) {
                GtkTreePath *path;

                gtk_tree_view_get_path_at_pos (GTK_TREE_VIEW (widget), event->x, event->y, &path, NULL, NULL, NULL);
                if (path != NULL) {
                        GtkTreeSelection *selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (widget));
                        gtk_tree_selection_unselect_all (selection);
                        gtk_tree_selection_select_path (selection, path);
                        gtk_tree_path_free (path);
                }
        }

        browser->priv->dragging = FALSE;
        browser->priv->pressed = FALSE;

        return FALSE;
}

static gboolean
browser_motion_notify (GtkWidget *widget, 
                       GdkEventMotion *event,
                       Browser *browser)
{
        // desactivated to make the logs more readable
        // LOG_FUNCTION_START
        GdkModifierType mods;
        int x, y;
        int dx, dy;

        if ((browser->priv->dragging) || !(browser->priv->pressed))
                return FALSE;

        gdk_window_get_pointer (widget->window, &x, &y, &mods);

        dx = x - browser->priv->drag_start_x;
        dy = y - browser->priv->drag_start_y;

        if ((util_abs (dx) > DRAG_THRESHOLD) || (util_abs (dy) > DRAG_THRESHOLD))
                browser->priv->dragging = TRUE;

        return FALSE;
}

static void
get_selected_artists_foreach (GtkTreeModel *model,
                 GtkTreePath *path,
                 GtkTreeIter *iter,
                 gpointer userdata)
{
        LOG_FUNCTION_START
        GList **artists = (GList **) userdata;
        gchar *val = NULL;

        gtk_tree_model_get (model, iter, 0, &val, -1);

        *artists = g_list_append (*artists, val);
}

static void
browser_add_artists (Browser *browser)
{
        LOG_FUNCTION_START
        GList *artists = NULL;

        gtk_tree_selection_selected_foreach (browser->priv->artists_selection,
                                             get_selected_artists_foreach,
                                             &artists);
        playlist_append_artists (browser->priv->playlist, artists);

        g_list_foreach (artists, (GFunc) g_free, NULL);
        g_list_free (artists);
}

static void
browser_cmd_add_artists (GtkAction *action,
                         Browser *browser)
{
        LOG_FUNCTION_START
        browser_add_artists (browser);
}

static void
get_selected_albums_foreach (GtkTreeModel *model,
                GtkTreePath *path,
                GtkTreeIter *iter,
                gpointer userdata)
{
        LOG_FUNCTION_START
        GList **albums = (GList **) userdata;

        Mpd_album *mpd_album;

        mpd_album = (Mpd_album *) g_malloc (sizeof (Mpd_album));
        gtk_tree_model_get (model, iter, ALBUM_ARTIST_COLUMN, &mpd_album->artist, -1);
        gtk_tree_model_get (model, iter, ALBUM_ALBUM_COLUMN, &mpd_album->album, -1);

        *albums = g_list_append (*albums, mpd_album);
}

static void
browser_add_albums (Browser *browser)
{
        LOG_FUNCTION_START
        GList *albums = NULL;

        gtk_tree_selection_selected_foreach (browser->priv->albums_selection,
                                             get_selected_albums_foreach,
                                             &albums);

        playlist_append_albums (browser->priv->playlist, albums);

        g_list_foreach (albums, (GFunc) mpd_free_album, NULL);
        g_list_free (albums);
}

static void
browser_cmd_add_albums (GtkAction *action,
                        Browser *browser)
{
        LOG_FUNCTION_START
        browser_add_albums (browser);
}

static void
songs_foreach (GtkTreeModel *model,
               GtkTreePath *path,
               GtkTreeIter *iter,
               gpointer userdata)
{
        LOG_FUNCTION_START
        GList **songs = (GList **) userdata;
        gchar *val = NULL;

        gtk_tree_model_get (model, iter, FILENAME_COLUMN, &val, -1);

        *songs = g_list_append (*songs, val);
}

static void
browser_add_songs (Browser *browser)
{
        LOG_FUNCTION_START
        GList *songs = NULL;

        gtk_tree_selection_selected_foreach (browser->priv->songs_selection,
                                             songs_foreach,
                                             &songs);
        playlist_append_songs (browser->priv->playlist, songs);

        g_list_foreach (songs, (GFunc) g_free, NULL);
        g_list_free (songs);
}

static void
browser_cmd_add_songs (GtkAction *action,
                       Browser *browser)
{
        LOG_FUNCTION_START
        browser_add_songs (browser);
}

static void
browser_add_in_playlist (Browser *browser)
{
        LOG_FUNCTION_START
        if (GTK_WIDGET_HAS_FOCUS (GTK_WIDGET (browser->priv->artists)))
                browser_add_artists (browser);

        if (GTK_WIDGET_HAS_FOCUS (GTK_WIDGET (browser->priv->albums)))
                browser_add_albums (browser);

        if (GTK_WIDGET_HAS_FOCUS (GTK_WIDGET (browser->priv->songs)))
                browser_add_songs (browser);
}

void 
browser_refresh_albumview (Browser *browser)
{
        LOG_FUNCTION_START
        gtk_widget_queue_draw (GTK_WIDGET (browser->priv->albums));
}

static void 
browser_get_covers_end (Browser *browser)
{
        LOG_FUNCTION_START
        browser_artists_selection_update (browser);
        /*g_signal_emit (G_OBJECT (browser), browser_signals[COVER_CHANGED], 0);*/
}

static void
get_artist_cover (Browser *browser,
                  guint operation)
{
        LOG_FUNCTION_START
        GtkWidget *coverdownloader;
        GList *artists = NULL;

        coverdownloader = shell_coverdownloader_new (browser->priv->mpd);

        gtk_tree_selection_selected_foreach (browser->priv->artists_selection,
                                             get_selected_artists_foreach,
                                             &artists);

        shell_coverdownloader_get_covers_from_artists (SHELL_COVERDOWNLOADER (coverdownloader),
                                                       artists,
                                                       operation);

        g_list_foreach (artists, (GFunc) g_free, NULL);
        g_list_free (artists);

        gtk_widget_destroy (coverdownloader);
        browser_get_covers_end (browser);
}

static void 
browser_cmd_get_artist_cover_amazon (GtkAction *action,
                                     Browser *browser)
{
        LOG_FUNCTION_START
        get_artist_cover (browser, GET_AMAZON_COVERS);
}

static void 
browser_cmd_remove_artist_cover (GtkAction *action,
                                 Browser *browser)
{
        LOG_FUNCTION_START
        GtkWidget *dialog;
        gint retval = GTK_RESPONSE_NO;

        dialog = gtk_message_dialog_new (NULL,
                                        GTK_DIALOG_MODAL,
                                        GTK_MESSAGE_QUESTION,
                                        GTK_BUTTONS_YES_NO,
                                        _("Are you sure that you want to remove all the covers of the selected artists?"));

        retval = gtk_dialog_run (GTK_DIALOG(dialog));
        gtk_widget_destroy (dialog);
        if (retval != GTK_RESPONSE_YES)
                return;

        get_artist_cover (browser, REMOVE_COVERS);
}

static void
get_album_cover (Browser *browser,
                 guint operation)
{
        LOG_FUNCTION_START
        GList *albums = NULL;

        gtk_tree_selection_selected_foreach (browser->priv->albums_selection,
                                             get_selected_albums_foreach,
                                             &albums);

        if (g_list_length(albums) == 1 && operation == GET_AMAZON_COVERS)
        {
                GtkWidget *coverselect;
                Mpd_album *mpd_album = albums->data;
                coverselect = shell_coverselect_new (mpd_album->artist,
                                                     mpd_album->album);
                gtk_dialog_run (GTK_DIALOG(coverselect));
                gtk_widget_destroy (coverselect);
        }
        else
        {
                GtkWidget *coverdownloader;
                coverdownloader = shell_coverdownloader_new (browser->priv->mpd);

                shell_coverdownloader_get_covers_from_albums (SHELL_COVERDOWNLOADER (coverdownloader),
                                                              albums,
                                                              operation);
                gtk_widget_destroy (coverdownloader);
        }

        g_list_foreach (albums, (GFunc) mpd_free_album, NULL);
        g_list_free (albums);

        browser_get_covers_end (browser);
}

static void 
browser_cmd_get_album_cover_amazon (GtkAction *action,
                                    Browser *browser)
{
        LOG_FUNCTION_START
        get_album_cover (browser, GET_AMAZON_COVERS);
}

static void 
browser_cmd_remove_album_cover (GtkAction *action,
                                Browser *browser)
{
        LOG_FUNCTION_START
        GtkWidget *dialog;
        gint retval = GTK_RESPONSE_NO;

        dialog = gtk_message_dialog_new (NULL,
                                        GTK_DIALOG_MODAL,
                                        GTK_MESSAGE_QUESTION,
                                        GTK_BUTTONS_YES_NO,
                                        _("Are you sure that you want to remove all the covers of the selected albums?"));

        retval = gtk_dialog_run (GTK_DIALOG(dialog));
        gtk_widget_destroy (dialog);
        if (retval != GTK_RESPONSE_YES)
                return;

        get_album_cover (browser, REMOVE_COVERS);
}

static void
browser_covertree_visible_changed_cb (GConfClient *client,
                                      guint cnxn_id,
                                      GConfEntry *entry,
                                      GtkTreeView *treeview)
{
        LOG_FUNCTION_START
        gtk_tree_view_column_set_visible (gtk_tree_view_get_column (treeview, 1),
                                          !eel_gconf_get_boolean (CONF_COVER_TREE_HIDDEN));
}
