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

#include "shell/ario-shell-songinfos.h"
#include <config.h>
#include <gtk/gtk.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <glib/gi18n.h>
#include "lib/gtk-builder-helpers.h"
#include "ario-debug.h"
#include "ario-util.h"
#include "ario-profiles.h"
#include "widgets/ario-lyrics-editor.h"
#ifdef ENABLE_TAGLIB
#include "taglib/tag_c.h"
#endif

#define ARIO_PREVIOUS 987
#define ARIO_NEXT 998
#define ARIO_SAVE 999

#define VALUE(b) b ? b : "n/a"

static void ario_shell_songinfos_finalize (GObject *object);
static gboolean ario_shell_songinfos_window_delete_cb (GtkWidget *window,
                                                       GdkEventAny *event,
                                                       ArioShellSonginfos *shell_songinfos);
static void ario_shell_songinfos_response_cb (GtkDialog *dial,
                                              int response_id,
                                              ArioShellSonginfos *shell_songinfos);
static void ario_shell_songinfos_set_current_song (ArioShellSonginfos *shell_songinfos);
G_MODULE_EXPORT void ario_shell_songinfos_text_changed_cb (GtkWidget *widget,
                                                           ArioShellSonginfos *shell_songinfos);
struct ArioShellSonginfosPrivate
{
        GtkWidget *notebook;

        GList *songs;

        GtkWidget *title_entry;
        GtkWidget *artist_entry;
        GtkWidget *album_entry;
        GtkWidget *track_entry;
        GtkWidget *date_entry;
        GtkWidget *genre_entry;
        GtkWidget *comment_entry;

        GtkWidget *file_label;
        GtkWidget *length_label;
        GtkWidget *composer_label;
        GtkWidget *performer_label;
        GtkWidget *disc_label;

        GtkWidget *lyrics_editor;

        GtkWidget *previous_button;
        GtkWidget *next_button;
        GtkWidget *save_button;
};

#define ARIO_SHELL_SONGINFOS_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), TYPE_ARIO_SHELL_SONGINFOS, ArioShellSonginfosPrivate))
G_DEFINE_TYPE (ArioShellSonginfos, ario_shell_songinfos, GTK_TYPE_DIALOG)

static void
ario_shell_songinfos_class_init (ArioShellSonginfosClass *klass)
{
        ARIO_LOG_FUNCTION_START
        GObjectClass *object_class = G_OBJECT_CLASS (klass);

        object_class->finalize = ario_shell_songinfos_finalize;

        g_type_class_add_private (klass, sizeof (ArioShellSonginfosPrivate));
}

static void
ario_shell_songinfos_init (ArioShellSonginfos *shell_songinfos)
{
        ARIO_LOG_FUNCTION_START
        shell_songinfos->priv = ARIO_SHELL_SONGINFOS_GET_PRIVATE (shell_songinfos);
}

static gboolean
ario_shell_songinfos_can_edit_tags ()
{
        ARIO_LOG_FUNCTION_START
#ifdef ENABLE_TAGLIB
        return (ario_profiles_get_current (ario_profiles_get ())->local
                        && ario_profiles_get_current (ario_profiles_get ())->musicdir);
#else
        return FALSE;
#endif
}

static void
ario_shell_songinfos_fill_tags (ArioServerSong *song)
{
        ARIO_LOG_FUNCTION_START
#ifdef ENABLE_TAGLIB
        gchar *filename;
        TagLib_File *file;
        TagLib_Tag *tag;
        const TagLib_AudioProperties *properties;

        filename = g_strconcat (ario_profiles_get_current (ario_profiles_get ())->musicdir, "/", song->file, NULL);
        file = taglib_file_new (filename);
        g_free (filename);

        if (file && taglib_file_is_valid (file)) {
                tag = taglib_file_tag (file);
                properties = taglib_file_audioproperties (file);
                if (tag) {
                        g_free (song->title);
                        song->title = g_strdup (taglib_tag_title (tag));
                        g_free (song->artist);
                        song->artist = g_strdup (taglib_tag_artist (tag));
                        g_free (song->album);
                        song->album = g_strdup (taglib_tag_album (tag));
                        g_free (song->track);
                        song->track = g_strdup_printf ("%i", taglib_tag_track (tag));
                        g_free (song->date);
                        song->date = g_strdup_printf ("%i", taglib_tag_year (tag));
                        g_free (song->genre);
                        song->genre = g_strdup (taglib_tag_genre (tag));
                        g_free (song->comment);
                        song->comment = g_strdup (taglib_tag_comment (tag));
                }

                if (properties)
                        song->time = taglib_audioproperties_length (properties);

                taglib_tag_free_strings ();
                taglib_file_free (file);
        }
#endif
}

GtkWidget *
ario_shell_songinfos_new (GSList *paths)
{
        ARIO_LOG_FUNCTION_START
        ArioShellSonginfos *shell_songinfos;
        GtkWidget *widget;
        GtkBuilder *builder;
        GList *tmp;

        shell_songinfos = g_object_new (TYPE_ARIO_SHELL_SONGINFOS, NULL);

        g_return_val_if_fail (shell_songinfos->priv != NULL, NULL);

        builder = gtk_builder_helpers_new (UI_PATH "song-infos.ui",
                                           shell_songinfos);

        widget = GTK_WIDGET (gtk_builder_get_object (builder, "vbox"));

        shell_songinfos->priv->title_entry =
                GTK_WIDGET (gtk_builder_get_object (builder, "title_entry"));
        shell_songinfos->priv->artist_entry =
                GTK_WIDGET (gtk_builder_get_object (builder, "artist_entry"));
        shell_songinfos->priv->album_entry =
                GTK_WIDGET (gtk_builder_get_object (builder, "album_entry"));
        shell_songinfos->priv->track_entry =
                GTK_WIDGET (gtk_builder_get_object (builder, "track_entry"));
        shell_songinfos->priv->length_label =
                GTK_WIDGET (gtk_builder_get_object (builder, "length_label"));
        shell_songinfos->priv->date_entry =
                GTK_WIDGET (gtk_builder_get_object (builder, "date_entry"));
        shell_songinfos->priv->file_label =
                GTK_WIDGET (gtk_builder_get_object (builder, "file_label"));
        shell_songinfos->priv->genre_entry =
                GTK_WIDGET (gtk_builder_get_object (builder, "genre_entry"));
        shell_songinfos->priv->composer_label =
                GTK_WIDGET (gtk_builder_get_object (builder, "composer_label"));
        shell_songinfos->priv->performer_label =
                GTK_WIDGET (gtk_builder_get_object (builder, "performer_label"));
        shell_songinfos->priv->disc_label =
                GTK_WIDGET (gtk_builder_get_object (builder, "disc_label"));
        shell_songinfos->priv->comment_entry =
                GTK_WIDGET (gtk_builder_get_object (builder, "comment_entry"));

        gtk_builder_helpers_boldify_label (builder, "frame_label");
        gtk_builder_helpers_boldify_label (builder, "title_const_label");
        gtk_builder_helpers_boldify_label (builder, "artist_const_label");
        gtk_builder_helpers_boldify_label (builder, "album_const_label");
        gtk_builder_helpers_boldify_label (builder, "track_const_label");
        gtk_builder_helpers_boldify_label (builder, "length_const_label");
        gtk_builder_helpers_boldify_label (builder, "date_const_label");
        gtk_builder_helpers_boldify_label (builder, "file_const_label");
        gtk_builder_helpers_boldify_label (builder, "genre_const_label");
        gtk_builder_helpers_boldify_label (builder, "composer_const_label");
        gtk_builder_helpers_boldify_label (builder, "performer_const_label");
        gtk_builder_helpers_boldify_label (builder, "disc_const_label");
        gtk_builder_helpers_boldify_label (builder, "comment_const_label");

        gtk_widget_set_size_request(shell_songinfos->priv->artist_entry, 280, -1);
        gtk_widget_set_size_request(shell_songinfos->priv->album_entry, 280, -1);
        gtk_widget_set_size_request(shell_songinfos->priv->track_entry, 280, -1);
        gtk_widget_set_size_request(shell_songinfos->priv->length_label, 280, -1);
        gtk_widget_set_size_request(shell_songinfos->priv->date_entry, 280, -1);
        gtk_widget_set_size_request(shell_songinfos->priv->file_label, 280, -1);
        gtk_widget_set_size_request(shell_songinfos->priv->genre_entry, 280, -1);
        gtk_widget_set_size_request(shell_songinfos->priv->composer_label, 280, -1);
        gtk_widget_set_size_request(shell_songinfos->priv->performer_label, 280, -1);
        gtk_widget_set_size_request(shell_songinfos->priv->disc_label, 280, -1);
        gtk_widget_set_size_request(shell_songinfos->priv->comment_entry, 280, -1);

        gtk_window_set_title (GTK_WINDOW (shell_songinfos), _("Song Properties"));
        gtk_window_set_resizable (GTK_WINDOW (shell_songinfos), TRUE);
        gtk_window_set_default_size (GTK_WINDOW (shell_songinfos), 450, 350);

        shell_songinfos->priv->notebook = GTK_WIDGET (gtk_notebook_new ());
        gtk_container_set_border_width (GTK_CONTAINER (shell_songinfos->priv->notebook), 5);

        gtk_container_add (GTK_CONTAINER (GTK_DIALOG (shell_songinfos)->vbox),
                           shell_songinfos->priv->notebook);

        gtk_container_set_border_width (GTK_CONTAINER (shell_songinfos), 5);
        gtk_box_set_spacing (GTK_BOX (GTK_DIALOG (shell_songinfos)->vbox), 2);
        gtk_dialog_set_has_separator (GTK_DIALOG (shell_songinfos), FALSE);


        gtk_notebook_append_page (GTK_NOTEBOOK (shell_songinfos->priv->notebook),
                                  widget,
                                  gtk_label_new (_("Song Properties")));

        shell_songinfos->priv->lyrics_editor = ario_lyrics_editor_new ();
        gtk_notebook_append_page (GTK_NOTEBOOK (shell_songinfos->priv->notebook),
                                  shell_songinfos->priv->lyrics_editor,
                                  gtk_label_new (_("Lyrics")));

        g_signal_connect (shell_songinfos,
                          "delete_event",
                          G_CALLBACK (ario_shell_songinfos_window_delete_cb),
                          shell_songinfos);
        g_signal_connect (shell_songinfos,
                          "response",
                          G_CALLBACK (ario_shell_songinfos_response_cb),
                          shell_songinfos);

        shell_songinfos->priv->songs = ario_server_get_songs_info (paths);
        if (ario_shell_songinfos_can_edit_tags ()) {
                gtk_editable_set_editable (GTK_EDITABLE (shell_songinfos->priv->title_entry), TRUE);
                gtk_editable_set_editable (GTK_EDITABLE (shell_songinfos->priv->artist_entry), TRUE);
                gtk_editable_set_editable (GTK_EDITABLE (shell_songinfos->priv->album_entry), TRUE);
                gtk_editable_set_editable (GTK_EDITABLE (shell_songinfos->priv->track_entry), TRUE);
                gtk_editable_set_editable (GTK_EDITABLE (shell_songinfos->priv->date_entry), TRUE);
                gtk_editable_set_editable (GTK_EDITABLE (shell_songinfos->priv->genre_entry), TRUE);
                gtk_editable_set_editable (GTK_EDITABLE (shell_songinfos->priv->comment_entry), TRUE);

                shell_songinfos->priv->save_button = gtk_button_new_from_stock (GTK_STOCK_SAVE);
                gtk_dialog_add_action_widget (GTK_DIALOG (shell_songinfos),
                                              shell_songinfos->priv->save_button,
                                              ARIO_SAVE);

                for (tmp = shell_songinfos->priv->songs; tmp; tmp = g_list_next (tmp)) {
                        ario_shell_songinfos_fill_tags (tmp->data);
                }
        } else {
                gtk_editable_set_editable (GTK_EDITABLE (shell_songinfos->priv->title_entry), FALSE);
                gtk_editable_set_editable (GTK_EDITABLE (shell_songinfos->priv->artist_entry), FALSE);
                gtk_editable_set_editable (GTK_EDITABLE (shell_songinfos->priv->album_entry), FALSE);
                gtk_editable_set_editable (GTK_EDITABLE (shell_songinfos->priv->track_entry), FALSE);
                gtk_editable_set_editable (GTK_EDITABLE (shell_songinfos->priv->date_entry), FALSE);
                gtk_editable_set_editable (GTK_EDITABLE (shell_songinfos->priv->genre_entry), FALSE);
                gtk_editable_set_editable (GTK_EDITABLE (shell_songinfos->priv->comment_entry), FALSE);
        }

        shell_songinfos->priv->previous_button = gtk_button_new_from_stock (GTK_STOCK_GO_BACK);
        gtk_dialog_add_action_widget (GTK_DIALOG (shell_songinfos),
                                      shell_songinfos->priv->previous_button,
                                      ARIO_PREVIOUS);

        shell_songinfos->priv->next_button = gtk_button_new_from_stock (GTK_STOCK_GO_FORWARD);
        gtk_dialog_add_action_widget (GTK_DIALOG (shell_songinfos),
                                      shell_songinfos->priv->next_button,
                                      ARIO_NEXT);

        gtk_dialog_add_button (GTK_DIALOG (shell_songinfos),
                               GTK_STOCK_CLOSE,
                               GTK_RESPONSE_CLOSE);

        gtk_dialog_set_default_response (GTK_DIALOG (shell_songinfos),
                                         GTK_RESPONSE_CLOSE);

        ario_shell_songinfos_set_current_song (shell_songinfos);

        g_object_unref (builder);

        return GTK_WIDGET (shell_songinfos);
}

static void
ario_shell_songinfos_finalize (GObject *object)
{
        ARIO_LOG_FUNCTION_START
        ArioShellSonginfos *shell_songinfos;

        g_return_if_fail (object != NULL);
        g_return_if_fail (IS_ARIO_SHELL_SONGINFOS (object));

        shell_songinfos = ARIO_SHELL_SONGINFOS (object);

        g_return_if_fail (shell_songinfos->priv != NULL);

        shell_songinfos->priv->songs = g_list_first (shell_songinfos->priv->songs);
        g_list_foreach (shell_songinfos->priv->songs, (GFunc) ario_server_free_song, NULL);
        g_list_free (shell_songinfos->priv->songs);

        G_OBJECT_CLASS (ario_shell_songinfos_parent_class)->finalize (object);
}

static gboolean
ario_shell_songinfos_window_delete_cb (GtkWidget *window,
                                       GdkEventAny *event,
                                       ArioShellSonginfos *shell_songinfos)
{
        ARIO_LOG_FUNCTION_START
        gtk_widget_hide (GTK_WIDGET (shell_songinfos));
        gtk_widget_destroy (GTK_WIDGET (shell_songinfos));

        return TRUE;
}

static void
ario_shell_songinfos_response_cb (GtkDialog *dial,
                                  int response_id,
                                  ArioShellSonginfos *shell_songinfos)
{
        ARIO_LOG_FUNCTION_START
#ifdef ENABLE_TAGLIB 
        gchar *filename;
        TagLib_File *file;
        TagLib_Tag *tag;
        GtkWidget *dialog;
        gboolean success;
        ArioServerSong *song;
#endif

        switch (response_id) {
        case GTK_RESPONSE_CLOSE:
                gtk_widget_hide (GTK_WIDGET (shell_songinfos));
                gtk_widget_destroy (GTK_WIDGET (shell_songinfos));
                break;
#ifdef ENABLE_TAGLIB 
        case ARIO_SAVE:
                /* Save tags */
                success = FALSE;
                song = shell_songinfos->priv->songs->data;
                filename = g_strconcat (ario_profiles_get_current (ario_profiles_get ())->musicdir, "/", song->file, NULL);
                file = taglib_file_new (filename);

                if (file && taglib_file_is_valid (file)) {
                        tag = taglib_file_tag (file);
                        if (tag) {
                                taglib_tag_set_title (tag, gtk_entry_get_text (GTK_ENTRY (shell_songinfos->priv->title_entry)));
                                taglib_tag_set_artist (tag, gtk_entry_get_text (GTK_ENTRY (shell_songinfos->priv->artist_entry)));
                                taglib_tag_set_album (tag, gtk_entry_get_text (GTK_ENTRY (shell_songinfos->priv->album_entry)));
                                taglib_tag_set_track (tag, atoi (gtk_entry_get_text (GTK_ENTRY (shell_songinfos->priv->track_entry))));
                                taglib_tag_set_year (tag, atoi (gtk_entry_get_text (GTK_ENTRY (shell_songinfos->priv->date_entry))));
                                taglib_tag_set_genre (tag, gtk_entry_get_text (GTK_ENTRY (shell_songinfos->priv->genre_entry)));
                                taglib_tag_set_comment (tag, gtk_entry_get_text (GTK_ENTRY (shell_songinfos->priv->comment_entry)));
                        }

                        if (taglib_file_save (file)) {
                                success = TRUE;
                                g_free (song->title);
                                song->title = g_strdup (taglib_tag_title (tag));
                                g_free (song->artist);
                                song->artist = g_strdup (taglib_tag_artist (tag));
                                g_free (song->album);
                                song->album = g_strdup (taglib_tag_album (tag));
                                g_free (song->track);
                                song->track = g_strdup_printf ("%i", taglib_tag_track (tag));
                                g_free (song->date);
                                song->date = g_strdup_printf ("%i", taglib_tag_year (tag));
                                g_free (song->genre);
                                song->genre = g_strdup (taglib_tag_genre (tag));
                                g_free (song->comment);
                                song->comment = g_strdup (taglib_tag_comment (tag));
                                ario_server_update_db ();
                        }

                        taglib_tag_free_strings ();
                        taglib_file_free (file);
                }
                if (!success) {
                        dialog = gtk_message_dialog_new (GTK_WINDOW (shell_songinfos),
                                                         GTK_DIALOG_MODAL,
                                                         GTK_MESSAGE_ERROR,
                                                         GTK_BUTTONS_OK,
                                                         "%s %s",
                                                         _("Error saving tags of file:"), filename);
                        gtk_dialog_run (GTK_DIALOG (dialog));
                        gtk_widget_destroy (dialog);
                } else if (shell_songinfos->priv->save_button) {
                        gtk_widget_set_sensitive (GTK_WIDGET (shell_songinfos->priv->save_button), FALSE);
                }

                g_free (filename);
                break;
#endif
        case ARIO_PREVIOUS:
                if (g_list_previous (shell_songinfos->priv->songs)) {
                        shell_songinfos->priv->songs = g_list_previous (shell_songinfos->priv->songs);
                        ario_shell_songinfos_set_current_song (shell_songinfos);
                }
                break;
        case ARIO_NEXT:
                if (g_list_next (shell_songinfos->priv->songs)) {
                        shell_songinfos->priv->songs = g_list_next (shell_songinfos->priv->songs);
                        ario_shell_songinfos_set_current_song (shell_songinfos);
                }
                break;
        }
}

static void
ario_shell_songinfos_set_current_song (ArioShellSonginfos *shell_songinfos)
{
        ARIO_LOG_FUNCTION_START
        ArioServerSong *song;
        gchar *length;
        gchar *window_title;
        ArioLyricsEditorData *data;

        if (!shell_songinfos->priv->songs)
                return;

        song = shell_songinfos->priv->songs->data;
        if (song) {
                gtk_entry_set_text (GTK_ENTRY (shell_songinfos->priv->title_entry), song->title ? song->title : "");
                gtk_entry_set_text (GTK_ENTRY (shell_songinfos->priv->artist_entry), song->artist ? song->artist : "");
                gtk_entry_set_text (GTK_ENTRY (shell_songinfos->priv->album_entry), song->album ? song->album : "");
                gtk_entry_set_text (GTK_ENTRY (shell_songinfos->priv->track_entry), song->track ? song->track : "");
                gtk_entry_set_text (GTK_ENTRY (shell_songinfos->priv->date_entry), song->date ? song->date : "");
                gtk_entry_set_text (GTK_ENTRY (shell_songinfos->priv->genre_entry), song->genre ? song->genre : "");
                gtk_entry_set_text (GTK_ENTRY (shell_songinfos->priv->comment_entry), song->comment ? song->comment : "");
                length = ario_util_format_time (song->time);
                gtk_label_set_text (GTK_LABEL (shell_songinfos->priv->length_label), VALUE (length));
                g_free (length);
                gtk_label_set_text (GTK_LABEL (shell_songinfos->priv->file_label), VALUE (song->file));
                gtk_label_set_text (GTK_LABEL (shell_songinfos->priv->composer_label), VALUE (song->composer));
                gtk_label_set_text (GTK_LABEL (shell_songinfos->priv->performer_label), VALUE (song->performer));
                gtk_label_set_text (GTK_LABEL (shell_songinfos->priv->disc_label), VALUE (song->disc));
                if (shell_songinfos->priv->save_button)
                        gtk_widget_set_sensitive (GTK_WIDGET (shell_songinfos->priv->save_button), FALSE);
        }

        gtk_widget_set_sensitive (shell_songinfos->priv->previous_button, g_list_previous (shell_songinfos->priv->songs) != NULL);
        gtk_widget_set_sensitive (shell_songinfos->priv->next_button, g_list_next (shell_songinfos->priv->songs) != NULL);

        data = (ArioLyricsEditorData *) g_malloc0 (sizeof (ArioLyricsEditorData));
        data->artist = g_strdup (song->artist);
        data->title = ario_util_format_title (song);
        ario_lyrics_editor_push (ARIO_LYRICS_EDITOR (shell_songinfos->priv->lyrics_editor), data);

        window_title = g_strdup_printf ("%s - %s", _("Song Properties"), data->title);
        gtk_window_set_title (GTK_WINDOW (shell_songinfos), window_title);
        g_free (window_title);
}

void
ario_shell_songinfos_text_changed_cb (GtkWidget *widget,
                                      ArioShellSonginfos *shell_songinfos)
{
        ARIO_LOG_FUNCTION_START
        if (shell_songinfos->priv->save_button)
                gtk_widget_set_sensitive (GTK_WIDGET (shell_songinfos->priv->save_button), TRUE);
}

