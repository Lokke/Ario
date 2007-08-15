/*
 *  Copyright (C) 2004,2005 Marc Pavot <marc.pavot@gmail.com>
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
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 */

#include <gtk/gtk.h>
#include <i18n.h>
#include <string.h>

#include "cover.h"
#include "shell-coverdownloader.h"
#include "rb-glade-helpers.h"
#include "debug.h"

static void shell_coverdownloader_class_init (ShellCoverdownloaderClass *klass);
static void shell_coverdownloader_init (ShellCoverdownloader *shell_coverdownloader);
static void shell_coverdownloader_finalize (GObject *object);
static void shell_coverdownloader_set_property (GObject *object,
                                                      guint prop_id,
                                                          const GValue *value,
                                                          GParamSpec *pspec);
static void shell_coverdownloader_get_property (GObject *object,
                                                       guint prop_id,
                                                       GValue *value,
                                                       GParamSpec *pspec);
static GObject * shell_coverdownloader_constructor (GType type, guint n_construct_properties,
                                                       GObjectConstructParam *construct_properties);
static gboolean shell_coverdownloader_window_delete_cb (GtkWidget *window,
                                                           GdkEventAny *event,
                                                           ShellCoverdownloader *shell_coverdownloader);
static void shell_coverdownloader_find_amazon_image (ShellCoverdownloader *shell_coverdownloader,
                                                        const char *artist,
                                                        const char *album);
static void shell_coverdownloader_close_cb (GtkButton *button,
                                               ShellCoverdownloader *shell_coverdownloader);
static void shell_coverdownloader_cancel_cb (GtkButton *button,
                                                ShellCoverdownloader *shell_coverdownloader);
static void shell_coverdownloader_get_cover_from_album (ShellCoverdownloader *shell_coverdownloader,
                                                        Mpd_album *mpd_album,
                                                        ShellCoverdownloaderOperation operation);
enum
{
        PROP_0,
        PROP_MPD
};

struct ShellCoverdownloaderPrivate
{
        int nb_covers;
        int nb_covers_already_exist;
        int nb_covers_found;
        int nb_covers_not_found;

        gboolean cancelled;
        gboolean closed;

        GSList *entries;

        GtkWidget *progress_artist_label;
        GtkWidget *progress_album_label;
        GtkWidget *progress_operation_label;
        GtkWidget *progress_hbox;
        GtkWidget *progress_const_artist_label;
        GtkWidget *progressbar;
        GtkWidget *cancel_button;
        GtkWidget *close_button;

        Mpd *mpd;
};

static GObjectClass *parent_class = NULL;

GType
shell_coverdownloader_get_type (void)
{
        LOG_FUNCTION_START
        static GType shell_coverdownloader_type = 0;
        if (shell_coverdownloader_type == 0)
        {
                static const GTypeInfo our_info =
                {
                        sizeof (ShellCoverdownloaderClass),
                        NULL,
                        NULL,
                        (GClassInitFunc) shell_coverdownloader_class_init,
                        NULL,
                        NULL,
                        sizeof (ShellCoverdownloader),
                        0,
                        (GInstanceInitFunc) shell_coverdownloader_init
                };

                shell_coverdownloader_type = g_type_register_static (GTK_TYPE_WINDOW,
                                                                     "ShellCoverdownloader",
                                                                     &our_info, 0);
        }
        return shell_coverdownloader_type;
}

static void
shell_coverdownloader_class_init (ShellCoverdownloaderClass *klass)
{
        LOG_FUNCTION_START
        GObjectClass *object_class = G_OBJECT_CLASS (klass);

        parent_class = g_type_class_peek_parent (klass);

        object_class->finalize = shell_coverdownloader_finalize;
        object_class->constructor = shell_coverdownloader_constructor;

        object_class->set_property = shell_coverdownloader_set_property;
        object_class->get_property = shell_coverdownloader_get_property;

        g_object_class_install_property (object_class,
                                         PROP_MPD,
                                         g_param_spec_object ("mpd",
                                                              "mpd",
                                                              "mpd",
                                                              TYPE_MPD,
                                                              G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
}

static void
shell_coverdownloader_init (ShellCoverdownloader *shell_coverdownloader)
{
        LOG_FUNCTION_START
        shell_coverdownloader->priv = g_new0 (ShellCoverdownloaderPrivate, 1);
        shell_coverdownloader->priv->cancelled = FALSE;
        shell_coverdownloader->priv->closed = FALSE;
}

static void
shell_coverdownloader_finalize (GObject *object)
{
        LOG_FUNCTION_START
        ShellCoverdownloader *shell_coverdownloader;

        g_return_if_fail (object != NULL);
        g_return_if_fail (IS_SHELL_COVERDOWNLOADER (object));

        shell_coverdownloader = SHELL_COVERDOWNLOADER (object);

        g_return_if_fail (shell_coverdownloader->priv != NULL);
        g_free (shell_coverdownloader->priv);
        G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
shell_coverdownloader_set_property (GObject *object,
                                       guint prop_id,
                                       const GValue *value,
                                       GParamSpec *pspec)
{
        LOG_FUNCTION_START
        ShellCoverdownloader *shell_coverdownloader = SHELL_COVERDOWNLOADER (object);

        switch (prop_id)
        {
        case PROP_MPD:
                shell_coverdownloader->priv->mpd = g_value_get_object (value);
                break;        
        default:
                G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
                break;
        }
}

static void 
shell_coverdownloader_get_property (GObject *object,
                                    guint prop_id,
                                    GValue *value,
                                    GParamSpec *pspec)
{
        LOG_FUNCTION_START
        ShellCoverdownloader *shell_coverdownloader = SHELL_COVERDOWNLOADER (object);

        switch (prop_id)
        {
        case PROP_MPD:
                g_value_set_object (value, shell_coverdownloader->priv->mpd);
                break;
        default:
                G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
                break;
        }
}

GtkWidget *
shell_coverdownloader_new (Mpd *mpd)
{
        LOG_FUNCTION_START
        ShellCoverdownloader *shell_coverdownloader;

        shell_coverdownloader = g_object_new (TYPE_SHELL_COVERDOWNLOADER,
                                              "mpd", mpd,
                                              NULL);

        g_return_val_if_fail (shell_coverdownloader->priv != NULL, NULL);

        return GTK_WIDGET (shell_coverdownloader);
}

static GObject *
shell_coverdownloader_constructor (GType type, guint n_construct_properties,
                                      GObjectConstructParam *construct_properties)
{
        LOG_FUNCTION_START
        ShellCoverdownloader *shell_coverdownloader;
        ShellCoverdownloaderClass *klass;
        GObjectClass *parent_class;
        GladeXML *xml = NULL;
        GtkWidget *vbox;

        klass = SHELL_COVERDOWNLOADER_CLASS (g_type_class_peek (TYPE_SHELL_COVERDOWNLOADER));

        parent_class = G_OBJECT_CLASS (g_type_class_peek_parent (klass));
        shell_coverdownloader = SHELL_COVERDOWNLOADER (parent_class->constructor (type, n_construct_properties,
                                                                                construct_properties));

        xml = rb_glade_xml_new (GLADE_PATH "cover-progress.glade", "vbox", NULL);
        vbox =
                glade_xml_get_widget (xml, "vbox");
        shell_coverdownloader->priv->progress_artist_label =
                glade_xml_get_widget (xml, "artist_label");
        shell_coverdownloader->priv->progress_album_label =
                glade_xml_get_widget (xml, "album_label");
        shell_coverdownloader->priv->progress_operation_label =
                glade_xml_get_widget (xml, "operation_label");
        shell_coverdownloader->priv->progressbar =
                glade_xml_get_widget (xml, "progressbar");
        shell_coverdownloader->priv->progress_hbox =
                glade_xml_get_widget (xml, "hbox2");
        shell_coverdownloader->priv->progress_const_artist_label =
                glade_xml_get_widget (xml, "const_artist_label");
        shell_coverdownloader->priv->cancel_button =
                glade_xml_get_widget (xml, "cancel_button");
        shell_coverdownloader->priv->close_button =
                glade_xml_get_widget (xml, "close_button");

        g_object_unref (G_OBJECT (xml));

        gtk_window_set_title (GTK_WINDOW (shell_coverdownloader), _("Music Player Cover Download"));
        gtk_container_add (GTK_CONTAINER (shell_coverdownloader), 
                           vbox);
        gtk_window_set_position (GTK_WINDOW (shell_coverdownloader),
                                  GTK_WIN_POS_CENTER);

        g_signal_connect_object (G_OBJECT (shell_coverdownloader->priv->cancel_button),
                                 "clicked",
                                 G_CALLBACK (shell_coverdownloader_cancel_cb),
                                 shell_coverdownloader, 0);

        g_signal_connect_object (G_OBJECT (shell_coverdownloader->priv->close_button),
                                 "clicked",
                                 G_CALLBACK (shell_coverdownloader_close_cb),
                                 shell_coverdownloader, 0);

        g_signal_connect_object (G_OBJECT (shell_coverdownloader),
                                 "delete_event",
                                 G_CALLBACK (shell_coverdownloader_window_delete_cb),
                                 shell_coverdownloader, 0);

        return G_OBJECT (shell_coverdownloader);
}

static void
shell_coverdownloader_close_cb (GtkButton *button,
                                   ShellCoverdownloader *shell_coverdownloader)
{
        LOG_FUNCTION_START
        /* Close button pressed : we close and destroy the window */
        shell_coverdownloader->priv->closed = TRUE;
}

static void
shell_coverdownloader_cancel_cb (GtkButton *button,
                                    ShellCoverdownloader *shell_coverdownloader)
{
        LOG_FUNCTION_START
        /* Cancel button pressed : we wait until the end of the current download and we stop the search */
        shell_coverdownloader->priv->cancelled = TRUE;
}

static gboolean
shell_coverdownloader_window_delete_cb (GtkWidget *window,
                                           GdkEventAny *event,
                                           ShellCoverdownloader *shell_coverdownloader)
{
        LOG_FUNCTION_START
        if (!shell_coverdownloader->priv->cancelled)
                /*window destroyed for the first time : we wait until the end of the current download and we stop the search */
                shell_coverdownloader->priv->cancelled = TRUE;
        else
                /* window destroyed for the second time : we close and destroy the window */
                shell_coverdownloader->priv->closed = TRUE;

        return TRUE;
}

static void 
shell_coverdownloader_progress_start (ShellCoverdownloader *shell_coverdownloader)
{
        LOG_FUNCTION_START
        gtk_window_resize (GTK_WINDOW (shell_coverdownloader), 350, 150);

        /*gtk_window_set_policy (GTK_WINDOW (shell_coverdownloader), FALSE, TRUE, FALSE);*/
        gtk_window_set_resizable (GTK_WINDOW (shell_coverdownloader),
                                  FALSE);

        gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR (shell_coverdownloader->priv->progressbar), 0);
        gtk_widget_show_all (GTK_WIDGET (shell_coverdownloader));

        /* We only want the cancel button at beginning, not the close button */
        gtk_widget_hide (shell_coverdownloader->priv->close_button);

        /* We refresh the window */
        while (gtk_events_pending ())
                gtk_main_iteration ();
}

static void
shell_coverdownloader_progress_update (ShellCoverdownloader *shell_coverdownloader,
                          const gchar *artist,
                          const gchar *album)
{
        LOG_FUNCTION_START
        /* We have already searched for nb_covers_done covers */
        gdouble nb_covers_done = (shell_coverdownloader->priv->nb_covers_found 
                                + shell_coverdownloader->priv->nb_covers_not_found 
                                + shell_coverdownloader->priv->nb_covers_already_exist);

        /* We update the progress bar */
        gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR (shell_coverdownloader->priv->progressbar), 
                                       nb_covers_done / shell_coverdownloader->priv->nb_covers);

        /* We update the artist and the album label */
        gtk_label_set_text (GTK_LABEL (shell_coverdownloader->priv->progress_artist_label), artist);
        gtk_label_set_text (GTK_LABEL (shell_coverdownloader->priv->progress_album_label), album);

        /* We refresh the window */
        while (gtk_events_pending ())
                gtk_main_iteration ();
}

static void
shell_coverdownloader_progress_end (ShellCoverdownloader *shell_coverdownloader)
{
        LOG_FUNCTION_START
        char *label_text;

        /* We only want the close button at the end, not the cancel button */
        gtk_widget_hide (shell_coverdownloader->priv->cancel_button);
        gtk_widget_show (shell_coverdownloader->priv->close_button);

        gtk_label_set_text (GTK_LABEL (shell_coverdownloader->priv->progress_artist_label), "");
        gtk_label_set_text (GTK_LABEL (shell_coverdownloader->priv->progress_album_label), "");
        gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR (shell_coverdownloader->priv->progressbar), 1);

        gtk_label_set_text (GTK_LABEL (shell_coverdownloader->priv->progress_const_artist_label),
                            _("Download Finished!"));

        /* We show the numbers of covers found and not found */
        label_text = g_strdup_printf (_("%i covers found\n%i covers not found\n%i covers already exist"), 
                                      shell_coverdownloader->priv->nb_covers_found, 
                                      shell_coverdownloader->priv->nb_covers_not_found,
                                      shell_coverdownloader->priv->nb_covers_already_exist);

        gtk_label_set_text (GTK_LABEL (shell_coverdownloader->priv->progress_const_artist_label),
                            label_text);
        g_free (label_text);

        gtk_widget_destroy (shell_coverdownloader->priv->progress_hbox);
        gtk_widget_destroy (shell_coverdownloader->priv->progress_artist_label);

        /* We wait until the user click on the close button */
        while (!shell_coverdownloader->priv->closed)
                gtk_main_iteration ();
}

static void
shell_coverdownloader_nocover (ShellCoverdownloader *shell_coverdownloader)
{
        LOG_FUNCTION_START
        /* This information will be displayed at the end of the search */
        shell_coverdownloader->priv->nb_covers_not_found ++;
}

void
shell_coverdownloader_get_covers (ShellCoverdownloader *shell_coverdownloader,
                                  ShellCoverdownloaderOperation operation)
{
        LOG_FUNCTION_START
        shell_coverdownloader_get_covers_from_artists (shell_coverdownloader,
                                                       mpd_get_artists(shell_coverdownloader->priv->mpd),
                                                       operation);
}

void
shell_coverdownloader_get_covers_from_artists (ShellCoverdownloader *shell_coverdownloader,
                                               GList *artists,
                                               ShellCoverdownloaderOperation operation)
{
        LOG_FUNCTION_START
        GList *albums = NULL, *temp_albums = NULL, *temp;

        if (!artists)
                return;

        for (temp = artists; temp; temp = temp->next) {
                /* We make a query for each selected artist */
                temp_albums = mpd_get_albums (shell_coverdownloader->priv->mpd, (gchar *) temp->data);

                /* We add the result of the query to a list */
                albums = g_list_concat (albums, temp_albums);
        }

        /* We search for all the covers of the entry in the list */
        shell_coverdownloader_get_covers_from_albums (shell_coverdownloader, albums, operation);

        /* We free the list */
        g_list_foreach (albums, (GFunc) mpd_free_album, NULL);
        g_list_free (albums);
}

void
shell_coverdownloader_get_covers_from_albums (ShellCoverdownloader *shell_coverdownloader,
                                              GList *albums,
                                              ShellCoverdownloaderOperation operation)
{
        LOG_FUNCTION_START
        GList *temp;

        if (!albums)
                return;

        temp = albums;

        /* We show the window with the progress bar */
        if (operation == GET_AMAZON_COVERS)
                shell_coverdownloader_progress_start (shell_coverdownloader);

        shell_coverdownloader->priv->nb_covers = g_list_length (albums);

        /* While there are still covers to search */
        while (temp) {
                /* The user has pressed the "cancel button" or has closed the window : we stop the search */
                if (shell_coverdownloader->priv->cancelled)
                        break;

                /* We search for a new cover */
                shell_coverdownloader_get_cover_from_album (shell_coverdownloader,
                                                            temp->data,
                                                            operation);
                temp = g_list_next (temp);
        }

        /* We change the window to show a close button and infos about the search */
        if (operation == GET_AMAZON_COVERS)
                shell_coverdownloader_progress_end (shell_coverdownloader);
}

static void
shell_coverdownloader_get_cover_from_album (ShellCoverdownloader *shell_coverdownloader,
                                            Mpd_album *mpd_album,
                                             ShellCoverdownloaderOperation operation)
{
        LOG_FUNCTION_START
        const gchar *artist;
        const gchar *album;

        if (!mpd_album)
                return;

        artist = mpd_album->artist;
        album = mpd_album->album;

        if (!album || !artist)
                return;
        
        switch (operation) {
        case GET_AMAZON_COVERS: {
                /* We update the progress bar */
                shell_coverdownloader_progress_update (shell_coverdownloader, artist, album);

                if (cover_cover_exists (artist, album))
                        /* The cover already exists, we do nothing */
                        shell_coverdownloader->priv->nb_covers_already_exist++;
                else
                        /* We search for the cover on amazon */
                        shell_coverdownloader_find_amazon_image (shell_coverdownloader, artist, album);
        }
                break;

        case REMOVE_COVERS: {
                /* We remove the cover from the ~/.gnome2/ario/covers/ directory */
                cover_remove_cover (artist, album);
        }
                break;

        default:
                break;
        }
}

static void 
shell_coverdownloader_find_amazon_image (ShellCoverdownloader *shell_coverdownloader,
                                         const char *artist,
                                         const char *album)
{
        LOG_FUNCTION_START
        GArray *size;
        GList *data = NULL, *cover_uris = NULL;
        int ret;
        GnomeVFSResult result;

        size = g_array_new (TRUE, TRUE, sizeof (int));

        /* If a cover is found on amazon, it is loaded in data(0) */
        ret = cover_load_amazon_covers (artist,
                                        album,
                                        &cover_uris,
                                        &size,
                                        &data,
                                        GET_FIRST_COVER,
                                        AMAZON_MEDIUM_COVER);
        /* If the cover is not too big and not too small (blank amazon image), we save it */
        if (ret == 0 && cover_size_is_valid (g_array_index (size, int, 0))) {
                result = cover_save_cover (artist,
                                           album,
                                           g_list_nth_data (data, 0),
                                           g_array_index (size, int, 0),
                                           OVERWRITE_MODE_SKIP);

                if (result == GNOME_VFS_OK)
                        shell_coverdownloader->priv->nb_covers_found ++;
                else
                        shell_coverdownloader->priv->nb_covers_not_found ++;
        } else {
                shell_coverdownloader_nocover (shell_coverdownloader);
        }

        g_array_free (size, TRUE);
        g_list_foreach (data, (GFunc) g_free, NULL);
        g_list_free (data);
        g_list_foreach (cover_uris, (GFunc) g_free, NULL);
        g_list_free (cover_uris);
}
