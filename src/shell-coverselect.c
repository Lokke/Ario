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

#include "eel/eel-gconf-extensions.h"
#include "shell-coverselect.h"
#include "cover.h"
#include "rb-glade-helpers.h"
#include "debug.h"

#define CURRENT_COVER_SIZE 130

static void shell_coverselect_class_init (ShellCoverselectClass *klass);
static void shell_coverselect_init (ShellCoverselect *shell_coverselect);
static void shell_coverselect_finalize (GObject *object);
static GObject * shell_coverselect_constructor (GType type, guint n_construct_properties,
                                                         GObjectConstructParam *construct_properties);
static gboolean shell_coverselect_window_delete_cb (GtkWidget *window,
                                                       GdkEventAny *event,
                                                       ShellCoverselect *shell_coverselect);
static void shell_coverselect_response_cb (GtkDialog *dialog,
                                              int response_id,
                                              ShellCoverselect *shell_coverselect);
static void shell_coverselect_option_small_cb (GtkWidget *widget,
                                                  ShellCoverselect *shell_coverselect);
static void shell_coverselect_option_medium_cb (GtkWidget *widget,
                                                   ShellCoverselect *shell_coverselect);
static void shell_coverselect_option_large_cb (GtkWidget *widget,
                                                  ShellCoverselect *shell_coverselect);
static void shell_coverselect_local_open_button_cb (GtkWidget *widget,
                                                    ShellCoverselect *shell_coverselect);
static void shell_coverselect_get_amazon_covers_cb (GtkWidget *widget,
                                                       ShellCoverselect *shell_coverselect);
static void shell_coverselect_show_covers (ShellCoverselect *shell_coverselect);

static void shell_coverselect_save_cover (ShellCoverselect *shell_coverselect);
static void shell_coverselect_set_current_cover (ShellCoverselect *shell_coverselect);

enum
{
        PROP_0,
};

enum 
{
        BMP_COLUMN,
        N_COLUMN
};

enum 
{
        AMAZON_PAGE,
        LOCAL_PAGE
};

struct ShellCoverselectPrivate
{
        GtkWidget *amazon_artist_entry;
        GtkWidget *amazon_album_entry;

        GtkWidget *notebook;

        GtkWidget *artist_label;
        GtkWidget *album_label;

        GtkWidget *get_amazon_covers_button;
        GtkWidget *current_cover;

        GtkWidget *listview;
        GtkListStore *liststore;

        GtkWidget *option_small;
        GtkWidget *option_medium;
        GtkWidget *option_large;

        GtkWidget *local_file_entry;
        GtkWidget *local_open_button;

        const gchar *file_artist;
        const gchar *file_album;

        int coversize;

        GArray *file_size;
        GList *cover_uris;
        GList *file_contents;
};

static GObjectClass *parent_class = NULL;

GType
shell_coverselect_get_type (void)
{
        LOG_FUNCTION_START
        static GType shell_coverselect_type = 0;

        if (shell_coverselect_type == 0)
        {
                static const GTypeInfo our_info =
                {
                        sizeof (ShellCoverselectClass),
                        NULL,
                        NULL,
                        (GClassInitFunc) shell_coverselect_class_init,
                        NULL,
                        NULL,
                        sizeof (ShellCoverselect),
                        0,
                        (GInstanceInitFunc) shell_coverselect_init
                };

                shell_coverselect_type = g_type_register_static (GTK_TYPE_DIALOG,
                                                                    "ShellCoverselect",
                                                                    &our_info, 0);
        }

        return shell_coverselect_type;
}

static void
shell_coverselect_class_init (ShellCoverselectClass *klass)
{
        LOG_FUNCTION_START
        GObjectClass *object_class = G_OBJECT_CLASS (klass);

        parent_class = g_type_class_peek_parent (klass);

        object_class->finalize = shell_coverselect_finalize;
        object_class->constructor = shell_coverselect_constructor;
}

static void
shell_coverselect_init (ShellCoverselect *shell_coverselect)
{
        LOG_FUNCTION_START
        shell_coverselect->priv = g_new0 (ShellCoverselectPrivate, 1);
        shell_coverselect->priv->liststore = gtk_list_store_new (1, GDK_TYPE_PIXBUF);
        shell_coverselect->priv->coversize = AMAZON_MEDIUM_COVER;
        shell_coverselect->priv->file_contents = NULL;
        shell_coverselect->priv->cover_uris = NULL;
}

static void
shell_coverselect_finalize (GObject *object)
{
        LOG_FUNCTION_START
        ShellCoverselect *shell_coverselect;

        g_return_if_fail (object != NULL);
        g_return_if_fail (IS_SHELL_COVERSELECT (object));

        shell_coverselect = SHELL_COVERSELECT (object);

        g_return_if_fail (shell_coverselect->priv != NULL);

        if (shell_coverselect->priv->file_size)
                g_array_free (shell_coverselect->priv->file_size, TRUE);
        g_list_foreach (shell_coverselect->priv->file_contents, (GFunc) g_free, NULL);
        g_list_free (shell_coverselect->priv->file_contents);
        g_list_foreach (shell_coverselect->priv->cover_uris, (GFunc) g_free, NULL);
        g_list_free (shell_coverselect->priv->cover_uris);
        g_free (shell_coverselect->priv);

        G_OBJECT_CLASS (parent_class)->finalize (object);
}

static GObject *
shell_coverselect_constructor (GType type, guint n_construct_properties,
                                     GObjectConstructParam *construct_properties)
{
        LOG_FUNCTION_START
        ShellCoverselect *shell_coverselect;
        ShellCoverselectClass *klass;
        GObjectClass *parent_class;
        GladeXML *xml = NULL;
        GtkWidget *vbox;

        GtkTreeViewColumn *column;
        GtkCellRenderer *cell_renderer;

        klass = SHELL_COVERSELECT_CLASS (g_type_class_peek (TYPE_SHELL_COVERSELECT));

        parent_class = G_OBJECT_CLASS (g_type_class_peek_parent (klass));
        shell_coverselect = SHELL_COVERSELECT (parent_class->constructor (type, n_construct_properties,
                                                                           construct_properties));

        xml = rb_glade_xml_new (GLADE_PATH "cover-select.glade", "vbox", NULL);
        vbox = glade_xml_get_widget (xml, "vbox");
        shell_coverselect->priv->artist_label =
                glade_xml_get_widget (xml, "artist_label");
        shell_coverselect->priv->album_label =
                glade_xml_get_widget (xml, "album_label");
        shell_coverselect->priv->notebook =
                glade_xml_get_widget (xml, "notebook");
        shell_coverselect->priv->amazon_artist_entry =
                glade_xml_get_widget (xml, "amazon_artist_entry");
        shell_coverselect->priv->amazon_album_entry =
                glade_xml_get_widget (xml, "amazon_album_entry");
        shell_coverselect->priv->get_amazon_covers_button =
                glade_xml_get_widget (xml, "search_button");
        shell_coverselect->priv->current_cover =
                glade_xml_get_widget (xml, "current_cover");
        shell_coverselect->priv->listview =
                glade_xml_get_widget (xml, "listview");
        shell_coverselect->priv->option_small =
                glade_xml_get_widget (xml, "option_small");
        shell_coverselect->priv->option_medium =
                glade_xml_get_widget (xml, "option_medium");
        shell_coverselect->priv->option_large =
                glade_xml_get_widget (xml, "option_large");
        shell_coverselect->priv->local_file_entry =
                glade_xml_get_widget (xml, "local_file_entry");
        shell_coverselect->priv->local_open_button =
                glade_xml_get_widget (xml, "local_open_button");

        g_object_unref (G_OBJECT (xml));

        gtk_container_add (GTK_CONTAINER (GTK_DIALOG (shell_coverselect)->vbox), 
                           vbox);

        gtk_window_set_title (GTK_WINDOW (shell_coverselect), _("Music Player Cover Download"));
        gtk_window_set_default_size (GTK_WINDOW (shell_coverselect), 520, 620);
        gtk_dialog_add_button (GTK_DIALOG (shell_coverselect),
                               GTK_STOCK_CANCEL,
                               GTK_RESPONSE_CANCEL);
        gtk_dialog_add_button (GTK_DIALOG (shell_coverselect),
                               GTK_STOCK_OK,
                               GTK_RESPONSE_OK);
        gtk_dialog_set_default_response (GTK_DIALOG (shell_coverselect),
                                         GTK_RESPONSE_OK);

        cell_renderer = gtk_cell_renderer_pixbuf_new ();
        column = gtk_tree_view_column_new_with_attributes ("Covers", 
                                                          cell_renderer, 
                                                          "pixbuf", 
                                                          BMP_COLUMN, NULL);
        gtk_tree_view_append_column (GTK_TREE_VIEW (shell_coverselect->priv->listview), 
                                    column);

        gtk_tree_view_set_model (GTK_TREE_VIEW (shell_coverselect->priv->listview),
                                      GTK_TREE_MODEL (shell_coverselect->priv->liststore));

        switch (shell_coverselect->priv->coversize) {
        case AMAZON_SMALL_COVER:
                gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (shell_coverselect->priv->option_small), TRUE);
                break;
        case AMAZON_MEDIUM_COVER:
                gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (shell_coverselect->priv->option_medium), TRUE);
                break;
        case AMAZON_LARGE_COVER:
                gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (shell_coverselect->priv->option_large), TRUE);
                break;
        default:
                g_assert_not_reached ();
                break;
        }

        g_signal_connect_object  (G_OBJECT (shell_coverselect),
                                 "delete_event",
                                 G_CALLBACK (shell_coverselect_window_delete_cb),
                                 shell_coverselect, 0);
        g_signal_connect_object (G_OBJECT (shell_coverselect),
                                 "response",
                                 G_CALLBACK (shell_coverselect_response_cb),
                                 shell_coverselect, 0);                                         
        g_signal_connect (G_OBJECT (shell_coverselect->priv->get_amazon_covers_button),
                         "clicked", G_CALLBACK (shell_coverselect_get_amazon_covers_cb),
                         shell_coverselect);
        g_signal_connect (G_OBJECT (shell_coverselect->priv->option_small), 
                         "clicked", G_CALLBACK (shell_coverselect_option_small_cb),
                         shell_coverselect);
        g_signal_connect (G_OBJECT (shell_coverselect->priv->option_medium), 
                         "clicked", G_CALLBACK (shell_coverselect_option_medium_cb),
                         shell_coverselect);
        g_signal_connect (G_OBJECT (shell_coverselect->priv->option_large), 
                         "clicked", G_CALLBACK (shell_coverselect_option_large_cb),
                         shell_coverselect);
        g_signal_connect (G_OBJECT (shell_coverselect->priv->local_open_button), 
                         "clicked", G_CALLBACK (shell_coverselect_local_open_button_cb),
                         shell_coverselect);

        return G_OBJECT (shell_coverselect);
}

GtkWidget *
shell_coverselect_new (const char *artist,
                       const char *album)
{
        LOG_FUNCTION_START
        ShellCoverselect *shell_coverselect;

        shell_coverselect = g_object_new (TYPE_SHELL_COVERSELECT,
                                          NULL);

        shell_coverselect->priv->file_artist = artist;        
        shell_coverselect->priv->file_album = album;

        shell_coverselect_set_current_cover (shell_coverselect);

        gtk_entry_set_text (GTK_ENTRY (shell_coverselect->priv->amazon_artist_entry), 
                            shell_coverselect->priv->file_artist);
        gtk_entry_set_text (GTK_ENTRY (shell_coverselect->priv->amazon_album_entry), 
                            shell_coverselect->priv->file_album);

        gtk_label_set_label (GTK_LABEL (shell_coverselect->priv->artist_label), 
                            shell_coverselect->priv->file_artist);
        gtk_label_set_label (GTK_LABEL (shell_coverselect->priv->album_label), 
                            shell_coverselect->priv->file_album);

        g_return_val_if_fail (shell_coverselect->priv != NULL, NULL);
        
        return GTK_WIDGET (shell_coverselect);
}

static gboolean
shell_coverselect_window_delete_cb (GtkWidget *window,
                                       GdkEventAny *event,
                                       ShellCoverselect *shell_coverselect)
{
        LOG_FUNCTION_START
        gtk_widget_hide (GTK_WIDGET (shell_coverselect));
        return FALSE;
}

static void
shell_coverselect_response_cb (GtkDialog *dialog,
                                  int response_id,
                                  ShellCoverselect *shell_coverselect)
{
        LOG_FUNCTION_START
        if (response_id == GTK_RESPONSE_OK) {
                shell_coverselect_save_cover (shell_coverselect);
                gtk_widget_hide (GTK_WIDGET (shell_coverselect));
        }

        if (response_id == GTK_RESPONSE_CANCEL)
                gtk_widget_hide (GTK_WIDGET (shell_coverselect));
}

static void
shell_coverselect_option_small_cb (GtkWidget *widget,
                                      ShellCoverselect *shell_coverselect)
{
        LOG_FUNCTION_START
        shell_coverselect->priv->coversize = AMAZON_SMALL_COVER;
}

static void
shell_coverselect_option_medium_cb (GtkWidget *widget,
                                       ShellCoverselect *shell_coverselect)
{
        LOG_FUNCTION_START
        shell_coverselect->priv->coversize = AMAZON_MEDIUM_COVER;
}

static void
shell_coverselect_option_large_cb (GtkWidget *widget,
                                      ShellCoverselect *shell_coverselect)
{
        LOG_FUNCTION_START
        shell_coverselect->priv->coversize = AMAZON_LARGE_COVER;
}

static void
shell_coverselect_local_open_button_cb (GtkWidget *widget,
                                        ShellCoverselect *shell_coverselect)
{
        LOG_FUNCTION_START
        GtkWidget *fileselection;

        fileselection = gtk_file_selection_new ("Select a file");
           gtk_window_set_modal (GTK_WINDOW (fileselection), TRUE);

        gtk_file_selection_set_filename (GTK_FILE_SELECTION (fileselection),
                                         g_get_home_dir ());
                                         
        if (gtk_dialog_run (GTK_DIALOG (fileselection)) == GTK_RESPONSE_OK)
                gtk_entry_set_text (GTK_ENTRY (shell_coverselect->priv->local_file_entry), 
                                    gtk_file_selection_get_filename (GTK_FILE_SELECTION (fileselection)));

        gtk_widget_destroy (fileselection);
}

static void
shell_coverselect_set_sensitive (ShellCoverselect *shell_coverselect,
                                    gboolean sensitive)
{
        LOG_FUNCTION_START
        gtk_dialog_set_response_sensitive (GTK_DIALOG (shell_coverselect),
                                           GTK_RESPONSE_CLOSE,
                                           sensitive);
        gtk_widget_set_sensitive (GTK_WIDGET (shell_coverselect->priv->amazon_artist_entry), sensitive);
        gtk_widget_set_sensitive (GTK_WIDGET (shell_coverselect->priv->amazon_album_entry), sensitive);
        gtk_widget_set_sensitive (GTK_WIDGET (shell_coverselect->priv->get_amazon_covers_button), sensitive);
        gtk_widget_set_sensitive (GTK_WIDGET (shell_coverselect->priv->listview), sensitive);

        while (gtk_events_pending ())
                gtk_main_iteration ();
}

static void 
shell_coverselect_get_amazon_covers_cb (GtkWidget *widget,
                                           ShellCoverselect *shell_coverselect)
{
        LOG_FUNCTION_START
        gchar *artist;
        gchar *album;
        GnomeVFSResult result;

        shell_coverselect_set_sensitive (shell_coverselect, FALSE);

        artist = gtk_editable_get_chars (GTK_EDITABLE (shell_coverselect->priv->amazon_artist_entry), 0, -1);
        album = gtk_editable_get_chars (GTK_EDITABLE (shell_coverselect->priv->amazon_album_entry), 0, -1);

        if (shell_coverselect->priv->file_size)
                g_array_free (shell_coverselect->priv->file_size, TRUE);
        g_list_foreach (shell_coverselect->priv->file_contents, (GFunc) g_free, NULL);
        g_list_free (shell_coverselect->priv->file_contents);
        shell_coverselect->priv->file_contents = NULL;
        g_list_foreach (shell_coverselect->priv->cover_uris, (GFunc) g_free, NULL);
        g_list_free (shell_coverselect->priv->cover_uris);
        shell_coverselect->priv->cover_uris = NULL;

        shell_coverselect->priv->file_size = g_array_new (TRUE, TRUE, sizeof (int));

        result = cover_load_amazon_covers (artist,
                                              album,
                                              &shell_coverselect->priv->cover_uris,
                                              &shell_coverselect->priv->file_size,
                                              &shell_coverselect->priv->file_contents,
                                              GET_ALL_COVERS,
                                              shell_coverselect->priv->coversize);
        g_free (artist);
        g_free (album);

        shell_coverselect_show_covers (shell_coverselect);

        shell_coverselect_set_sensitive (shell_coverselect, TRUE);
}

static void 
shell_coverselect_show_covers (ShellCoverselect *shell_coverselect)
{
        LOG_FUNCTION_START
        GtkTreeIter iter;
        int i = 0;
        GList *temp;
        GdkPixbuf *pixbuf;
        GtkTreePath *tree_path;
        GdkPixbufLoader *loader;

        gtk_list_store_clear (shell_coverselect->priv->liststore);

        if (!shell_coverselect->priv->file_contents)
                return;

        temp = shell_coverselect->priv->file_contents;
        while (g_array_index (shell_coverselect->priv->file_size, int, i) != 0) {
                loader = gdk_pixbuf_loader_new ();
                if (gdk_pixbuf_loader_write (loader, 
                                             temp->data,
                                             g_array_index (shell_coverselect->priv->file_size, int, i),
                                             NULL)) {
                        gdk_pixbuf_loader_close (loader, NULL);
                        pixbuf = gdk_pixbuf_loader_get_pixbuf (loader);

                        gtk_list_store_append(shell_coverselect->priv->liststore, 
                                              &iter);
                        gtk_list_store_set (shell_coverselect->priv->liststore, 
                                            &iter, 
                                            BMP_COLUMN, 
                                            pixbuf, -1);

                        g_object_unref (G_OBJECT (pixbuf));
                }
                temp = g_list_next (temp);
                i++;
        }

        tree_path = gtk_tree_path_new_from_indices (0, -1);
        gtk_tree_selection_select_path (gtk_tree_view_get_selection (GTK_TREE_VIEW (shell_coverselect->priv->listview)), tree_path);
}

static void 
shell_coverselect_save_cover (ShellCoverselect *shell_coverselect)
{        
        LOG_FUNCTION_START
        GtkWidget *dialog;
        
        GtkTreeSelection *selection;
        GtkTreeIter iter;
        GtkTreePath *tree_path;
        gint *indice;
        gchar *data;
        const gchar *local_file;
        gint size;
        GnomeVFSResult result;


        switch (gtk_notebook_get_current_page (GTK_NOTEBOOK (shell_coverselect->priv->notebook))) {
        case AMAZON_PAGE:
                selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (shell_coverselect->priv->listview));
                
                if (!gtk_tree_selection_get_selected (selection, NULL, &iter))
                        return;
                tree_path = gtk_tree_model_get_path (GTK_TREE_MODEL (shell_coverselect->priv->liststore), 
                                                     &iter);

                indice = gtk_tree_path_get_indices (tree_path);

                result = cover_save_cover (shell_coverselect->priv->file_artist,
                                           shell_coverselect->priv->file_album,
                                           g_list_nth_data (shell_coverselect->priv->file_contents, indice[0]),
                                           g_array_index (shell_coverselect->priv->file_size, int, indice[0]),
                                           OVERWRITE_MODE_ASK);
                gtk_tree_path_free (tree_path);
                break;
        case LOCAL_PAGE:
                local_file = gtk_entry_get_text (GTK_ENTRY (shell_coverselect->priv->local_file_entry));
                if (!local_file)
                        return;

                if (!strcmp (local_file, ""))
                        return;

                result = gnome_vfs_read_entire_file (local_file,
                                                     &size,
                                                     &data);
                if (result != GNOME_VFS_OK) {
                        dialog = gtk_message_dialog_new(NULL,
                                                        GTK_DIALOG_MODAL,
                                                        GTK_MESSAGE_ERROR,
                                                        GTK_BUTTONS_OK,
                                                        "Error reading file");
                        gtk_dialog_run (GTK_DIALOG (dialog));
                        gtk_widget_destroy (dialog);
                        return;
                }

                result = cover_save_cover (shell_coverselect->priv->file_artist,
                                           shell_coverselect->priv->file_album,
                                           data,
                                           size,
                                           OVERWRITE_MODE_ASK);
                g_free (data);
                break;
        default:
                return;
                break;
        }

        switch (result) {
        case GNOME_VFS_OK:
                break;
        case GNOME_VFS_ERROR_FILE_EXISTS:
                break;
        default:
                dialog = gtk_message_dialog_new(NULL,
                                                GTK_DIALOG_MODAL,
                                                GTK_MESSAGE_ERROR,
                                                GTK_BUTTONS_OK,
                                                "Error saving file");
                gtk_dialog_run(GTK_DIALOG(dialog));
                gtk_widget_destroy(dialog);
        }
}

static void
shell_coverselect_set_current_cover (ShellCoverselect *shell_coverselect)
{
        LOG_FUNCTION_START
        GdkPixbuf *pixbuf;
        gchar *cover_path;

        if (cover_cover_exists (shell_coverselect->priv->file_artist, shell_coverselect->priv->file_album)) {
                cover_path = cover_make_cover_path (shell_coverselect->priv->file_artist,
                                                       shell_coverselect->priv->file_album,
                                                       NORMAL_COVER);
                gtk_widget_show_all (shell_coverselect->priv->current_cover);
                pixbuf = gdk_pixbuf_new_from_file_at_size (cover_path, CURRENT_COVER_SIZE, CURRENT_COVER_SIZE, NULL);
                g_free (cover_path);
                gtk_image_set_from_pixbuf (GTK_IMAGE (shell_coverselect->priv->current_cover),
                                           pixbuf);
                g_object_unref (pixbuf);
        } else {
                gtk_widget_hide_all (shell_coverselect->priv->current_cover);
        }
}
