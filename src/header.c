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
#include <config.h>
#include <i18n.h>
#include <string.h>

#include "header.h"
#include "util.h"
#include "volume.h"
#include "debug.h"
#include "cover.h"

static void header_class_init (HeaderClass *klass);
static void header_init (Header *header);
static GObject *header_constructor (GType type, guint n_construct_properties,
                                    GObjectConstructParam *construct_properties);
static void header_finalize (GObject *object);
static void header_set_property (GObject *object,
                                 guint prop_id,
                                 const GValue *value,
                                 GParamSpec *pspec);
static void header_get_property (GObject *object,
                                 guint prop_id,
                                 GValue *value,
                                 GParamSpec *pspec);
static gboolean slider_press_callback (GtkWidget *widget,
                                       GdkEventButton *event,
                                       Header *header);
static gboolean slider_release_callback (GtkWidget *widget,
                                         GdkEventButton *event,
                                         Header *header);
static void header_song_changed_cb (Mpd *mpd,
                                    Header *header);
static void header_state_changed_cb (Mpd *mpd,
                                     Header *header);
static void header_elapsed_changed_cb (Mpd *mpd,
                                       Header *header);
static void header_random_changed_cb (Mpd *mpd,
                                      Header *header);
static void header_repeat_changed_cb (Mpd *mpd,
                                      Header *header);
static void header_do_random (Header *header);
static void header_do_repeat (Header *header);
static void header_cmd_playpause (GtkAction *action,
                                   Header *header);
static void header_cmd_stop (GtkAction *action,
                             Header *header);
static void header_cmd_next (GtkAction *action,
                             Header *header);
static void header_cmd_previous (GtkAction *action,
                                 Header *header);

struct HeaderPrivate
{
        Mpd *mpd;

        GtkTooltips *tooltips;
        GtkWidget *prev_button;
        GtkWidget *play_pause_button;
        GtkWidget *random_button;
        GtkWidget *repeat_button;

        GtkWidget *stop_button;
        GtkWidget *next_button;

        GtkWidget *play_image;
        GtkWidget *pause_image;

        GtkWidget *image;

        GtkWidget *song;
        GtkWidget *artist_album;

        GtkWidget *scale;
        GtkAdjustment *adjustment;

        GtkWidget *elapsed;
        GtkWidget *total;

        GtkWidget *volume_button;

        int total_time;

        gboolean slider_dragging;

        GtkActionGroup *actiongroup;

        gint image_width;
        gint image_height;
};

static GtkActionEntry header_actions [] =
{
        { "ControlPlayPause", GTK_STOCK_MEDIA_PLAY, N_("_Play/Pause"), "<control>space",
          N_("Start playback"),
          G_CALLBACK (header_cmd_playpause) },
        { "ControlStop", GTK_STOCK_MEDIA_STOP, N_("_Stop"), NULL,
          N_("Stop playback"),
          G_CALLBACK (header_cmd_stop) },
        { "ControlNext", GTK_STOCK_MEDIA_NEXT, N_("_Next"), "<control>Right",
          N_("Start playing the next song"),
          G_CALLBACK (header_cmd_next) },
        { "ControlPrevious", GTK_STOCK_MEDIA_PREVIOUS, N_("P_revious"), "<control>Left",
          N_("Start playing the previous song"),
          G_CALLBACK (header_cmd_previous) },
};

static guint header_n_actions = G_N_ELEMENTS (header_actions);

enum
{
        PROP_0,
        PROP_MPD,
        PROP_ACTION_GROUP,
};

static GObjectClass *parent_class = NULL;

#define SONG_MARKUP(xSONG) g_strdup_printf ("<big><b>%s</b></big>", xSONG);
#define FROM_MARKUP(xALBUM, xARTIST) g_strdup_printf (_("from %s by %s"), xALBUM, xARTIST);

GType
header_get_type (void)
{
        LOG_FUNCTION_START
        static GType header_type = 0;

        if (header_type == 0) {
                static const GTypeInfo our_info =
                {
                        sizeof (HeaderClass),
                        NULL,
                        NULL,
                        (GClassInitFunc) header_class_init,
                        NULL,
                        NULL,
                        sizeof (Header),
                        0,
                        (GInstanceInitFunc) header_init
                };

                header_type = g_type_register_static (GTK_TYPE_HBOX,
                                                      "Header",
                                                      &our_info, 0);
        }

        return header_type;
}

static void
header_class_init (HeaderClass *klass)
{
        LOG_FUNCTION_START
        GObjectClass *object_class = G_OBJECT_CLASS (klass);

        parent_class = g_type_class_peek_parent (klass);

        object_class->finalize = header_finalize;
        object_class->set_property = header_set_property;
        object_class->get_property = header_get_property;
        object_class->constructor = header_constructor;

        g_object_class_install_property (object_class,
                                         PROP_MPD,
                                         g_param_spec_object ("mpd",
                                                              "Mpd",
                                                              "Mpd object",
                                                              TYPE_MPD,
                                                              G_PARAM_READWRITE));
        g_object_class_install_property (object_class,
                                         PROP_ACTION_GROUP,
                                         g_param_spec_object ("action-group",
                                                              "GtkActionGroup",
                                                              "GtkActionGroup object",
                                                              GTK_TYPE_ACTION_GROUP,
                                                              G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
}

static void
header_init (Header *header)
{
        LOG_FUNCTION_START
        header->priv = g_new0 (HeaderPrivate, 1);
}

static GObject *
header_constructor (GType type, guint n_construct_properties,
                    GObjectConstructParam *construct_properties)
{
        LOG_FUNCTION_START
        Header *header;
        HeaderClass *klass;
        GObjectClass *parent_class;

        GtkWidget *image, *hbox, *vbox, *alignment, *label;

        klass = HEADER_CLASS (g_type_class_peek (TYPE_HEADER));

        parent_class = G_OBJECT_CLASS (g_type_class_peek_parent (klass));
        header = HEADER (parent_class->constructor (type, n_construct_properties,
                                                     construct_properties));


        gtk_box_set_spacing (GTK_BOX (header), 12);

        header->priv->tooltips = gtk_tooltips_new ();
        gtk_tooltips_enable (header->priv->tooltips);

        /* Previous button */
        image = gtk_image_new_from_stock (GTK_STOCK_MEDIA_PREVIOUS,
                                          GTK_ICON_SIZE_LARGE_TOOLBAR);

        header->priv->prev_button = gtk_button_new ();
        gtk_container_add (GTK_CONTAINER (header->priv->prev_button), image);
        g_signal_connect_swapped (G_OBJECT (header->priv->prev_button),
                                  "clicked", G_CALLBACK (header_do_previous), header);
        gtk_tooltips_set_tip (GTK_TOOLTIPS (header->priv->tooltips), 
                              GTK_WIDGET (header->priv->prev_button), 
                              _("Play previous song"), NULL);


        /* Button images */
        header->priv->play_image = gtk_image_new_from_stock (GTK_STOCK_MEDIA_PLAY,
                                                             GTK_ICON_SIZE_LARGE_TOOLBAR);
        g_object_ref (header->priv->play_image);
        gtk_widget_show (header->priv->play_image);
        header->priv->pause_image = gtk_image_new_from_stock (GTK_STOCK_MEDIA_PAUSE,
                                                              GTK_ICON_SIZE_LARGE_TOOLBAR);
        g_object_ref (header->priv->pause_image);

        header->priv->play_pause_button = gtk_button_new ();

        gtk_container_add (GTK_CONTAINER (header->priv->play_pause_button), header->priv->pause_image);

        g_signal_connect_swapped (G_OBJECT (header->priv->play_pause_button),
                                  "clicked", G_CALLBACK (header_playpause), header);
        gtk_tooltips_set_tip (GTK_TOOLTIPS (header->priv->tooltips), 
                              GTK_WIDGET (header->priv->play_pause_button), 
                              _("Play/Pause the music"), NULL);

        /* Stop button */
        image = gtk_image_new_from_stock (GTK_STOCK_MEDIA_STOP,
                                          GTK_ICON_SIZE_LARGE_TOOLBAR);
        header->priv->stop_button = gtk_button_new ();
        gtk_container_add (GTK_CONTAINER (header->priv->stop_button), image);
        g_signal_connect_swapped (G_OBJECT (header->priv->stop_button),
                                  "clicked", G_CALLBACK (header_stop), header);
        gtk_tooltips_set_tip (GTK_TOOLTIPS (header->priv->tooltips), 
                              GTK_WIDGET (header->priv->stop_button), 
                              _("Stop the music"), NULL);

        /* Next button */
        image = gtk_image_new_from_stock (GTK_STOCK_MEDIA_NEXT,
                                          GTK_ICON_SIZE_LARGE_TOOLBAR);
        header->priv->next_button = gtk_button_new ();
        gtk_container_add (GTK_CONTAINER (header->priv->next_button), image);
        g_signal_connect_swapped (G_OBJECT (header->priv->next_button),
                                  "clicked", G_CALLBACK (header_do_next), header);
        gtk_tooltips_set_tip (GTK_TOOLTIPS (header->priv->tooltips), 
                              GTK_WIDGET (header->priv->next_button), 
                              _("Play next song"), NULL);

        /* Command Buttons Container */
        hbox = gtk_hbox_new (FALSE, 5);
        gtk_container_set_border_width (GTK_CONTAINER (hbox), 5);

        gtk_box_pack_start (GTK_BOX (hbox), header->priv->prev_button, FALSE, TRUE, 0);
        gtk_box_pack_start (GTK_BOX (hbox), header->priv->play_pause_button, FALSE, TRUE, 0);
        gtk_box_pack_start (GTK_BOX (hbox), header->priv->stop_button, FALSE, TRUE, 0);
        gtk_box_pack_start (GTK_BOX (hbox), header->priv->next_button, FALSE, TRUE, 0);

        alignment = gtk_alignment_new (0.0, 0.5, 1.0, 0.0);
        gtk_container_add (GTK_CONTAINER (alignment), hbox);
        gtk_box_pack_start (GTK_BOX (header), alignment, FALSE, TRUE, 0);

        /* Construct the cover display */
        header->priv->image = gtk_image_new ();
        gtk_box_pack_start (GTK_BOX (header), header->priv->image, FALSE, TRUE, 0);
        gtk_icon_size_lookup(GTK_ICON_SIZE_LARGE_TOOLBAR, &header->priv->image_width, &header->priv->image_height);
        header->priv->image_width += 18;
        header->priv->image_height += 18;

        /* Construct the Song/Artist/Album display */
        header->priv->song = gtk_label_new ("");
        gtk_label_set_ellipsize (GTK_LABEL (header->priv->song), PANGO_ELLIPSIZE_END);
        gtk_label_set_use_markup (GTK_LABEL (header->priv->song), TRUE);
        gtk_misc_set_alignment (GTK_MISC (header->priv->song), 0, 0);

        header->priv->artist_album = gtk_label_new ("");
        gtk_misc_set_alignment (GTK_MISC (header->priv->artist_album), 0, 0);

        vbox = gtk_vbox_new (FALSE, 0);
        gtk_box_pack_start (GTK_BOX (vbox), header->priv->song, FALSE, TRUE, 0);
        gtk_box_pack_start (GTK_BOX (vbox), header->priv->artist_album, FALSE, TRUE, 0);

        alignment = gtk_alignment_new (0.0, 0.5, 1.0, 0.0);
        gtk_container_add (GTK_CONTAINER (alignment), vbox);
        gtk_box_pack_start (GTK_BOX (header), alignment, FALSE, TRUE, 0);

        /* Construct the time slider and display */
        header->priv->adjustment = GTK_ADJUSTMENT (gtk_adjustment_new (0.0, 0.0, 10.0, 1.0, 10.0, 0.0));
        header->priv->scale = gtk_hscale_new (header->priv->adjustment);

        g_signal_connect_object (G_OBJECT (header->priv->scale),
                                 "button_press_event",
                                 G_CALLBACK (slider_press_callback),
                                 header, 0);
        g_signal_connect_object (G_OBJECT (header->priv->scale),
                                 "button_release_event",
                                 G_CALLBACK (slider_release_callback),
                                 header, 0);

        gtk_scale_set_draw_value (GTK_SCALE (header->priv->scale), FALSE);
        gtk_widget_set_size_request (header->priv->scale, 150, -1);

        header->priv->elapsed = gtk_label_new ("0:00");
        label = gtk_label_new (_(" of "));
        header->priv->total = gtk_label_new ("0:00");

        vbox = gtk_vbox_new (FALSE, 0);
        hbox = gtk_hbox_new (FALSE, 0);

        gtk_box_pack_start (GTK_BOX (hbox), header->priv->elapsed, FALSE, TRUE, 0);
        gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, TRUE, 0);
        gtk_box_pack_start (GTK_BOX (hbox), header->priv->total, FALSE, TRUE, 0);

        gtk_box_pack_start (GTK_BOX (vbox), header->priv->scale, FALSE, TRUE, 0);
        gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, TRUE, 0);

        alignment = gtk_alignment_new (1.0, 0.5, 0.0, 0.0);

        gtk_container_add (GTK_CONTAINER (alignment), vbox);
        gtk_box_pack_start (GTK_BOX (header), alignment, TRUE, TRUE, 0);

        /* Random button */
        image = gtk_image_new_from_stock ("random",
                                          GTK_ICON_SIZE_LARGE_TOOLBAR);
        header->priv->random_button = gtk_toggle_button_new ();
        gtk_container_add (GTK_CONTAINER (header->priv->random_button), image);
        g_signal_connect_swapped (G_OBJECT (header->priv->random_button),
                                  "clicked", G_CALLBACK (header_do_random), header);
        gtk_tooltips_set_tip (GTK_TOOLTIPS (header->priv->tooltips), 
                              GTK_WIDGET (header->priv->random_button), 
                              _("Toggle random on/off"), NULL);

        /* Repeat button */
        image = gtk_image_new_from_stock ("repeat",
                                          GTK_ICON_SIZE_LARGE_TOOLBAR);
        header->priv->repeat_button = gtk_toggle_button_new ();
        gtk_container_add (GTK_CONTAINER (header->priv->repeat_button), image);
        g_signal_connect_swapped (G_OBJECT (header->priv->repeat_button),
                                  "clicked", G_CALLBACK (header_do_repeat), header);
        gtk_tooltips_set_tip (GTK_TOOLTIPS (header->priv->tooltips), 
                              GTK_WIDGET (header->priv->repeat_button), 
                              _("Toggle repeat on/off"), NULL);

        /* Buttons Container */
        hbox = gtk_hbox_new (FALSE, 5);
        gtk_box_pack_start (GTK_BOX (hbox), header->priv->random_button, FALSE, TRUE, 0);
        gtk_box_pack_start (GTK_BOX (hbox), header->priv->repeat_button, FALSE, TRUE, 0);
        alignment = gtk_alignment_new (0.0, 0.5, 1.0, 0.0);
        gtk_container_add (GTK_CONTAINER (alignment), hbox);
        gtk_box_pack_start (GTK_BOX (header), alignment, FALSE, TRUE, 0);

        return G_OBJECT (header);
}

static void
header_finalize (GObject *object)
{
        LOG_FUNCTION_START
        Header *header;

        g_return_if_fail (object != NULL);
        g_return_if_fail (IS_HEADER (object));

        header = HEADER (object);

        g_return_if_fail (header->priv != NULL);

        g_free (header->priv);

        G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
header_set_property (GObject *object,
                        guint prop_id,
                        const GValue *value,
                        GParamSpec *pspec)
{
        LOG_FUNCTION_START
        Header *header = HEADER (object);

        switch (prop_id) {
        case PROP_MPD:
                header->priv->mpd = g_value_get_object (value);

                /* Signals to synchronize the header with mpd */
                g_signal_connect_object (G_OBJECT (header->priv->mpd),
                                         "song_changed", G_CALLBACK (header_song_changed_cb),
                                         header, 0);
                g_signal_connect_object (G_OBJECT (header->priv->mpd),
                                         "state_changed", G_CALLBACK (header_state_changed_cb),
                                         header, 0);
                g_signal_connect_object (G_OBJECT (header->priv->mpd),
                                         "elapsed_changed", G_CALLBACK (header_elapsed_changed_cb),
                                         header, 0);
                g_signal_connect_object (G_OBJECT (header->priv->mpd),
                                         "random_changed", G_CALLBACK (header_random_changed_cb),
                                         header, 0);
                g_signal_connect_object (G_OBJECT (header->priv->mpd),
                                         "repeat_changed", G_CALLBACK (header_repeat_changed_cb),
                                         header, 0);
                break;
        case PROP_ACTION_GROUP:
                header->priv->actiongroup = g_value_get_object (value);
                gtk_action_group_add_actions (header->priv->actiongroup,
                                              header_actions,
                                              header_n_actions, header);
                break;
        default:
                G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
                break;
        }
}

static void
header_get_property (GObject *object,
                        guint prop_id,
                        GValue *value,
                        GParamSpec *pspec)
{
        LOG_FUNCTION_START
        Header *header = HEADER (object);

        switch (prop_id) {
        case PROP_MPD:
                g_value_set_object (value, header->priv->mpd);
                break;
        case PROP_ACTION_GROUP:
                g_value_set_object (value, header->priv->actiongroup);
                break;
        default:
                G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
                break;
        }
}

GtkWidget *
header_new (GtkActionGroup *group, Mpd *mpd)
{
        LOG_FUNCTION_START
        Header *header;
        GtkWidget *alignment;

        header = HEADER (g_object_new (TYPE_HEADER,
                                       "action-group", group,
                                       "mpd", mpd,
                                       NULL));

        /* Construct the volume button */
        header->priv->volume_button = GTK_WIDGET (volume_new (header->priv->mpd));
        gtk_tooltips_set_tip (GTK_TOOLTIPS (header->priv->tooltips), 
                              GTK_WIDGET (header->priv->volume_button), 
                              _("Change the music volume"), NULL);

        alignment = gtk_alignment_new (0.0, 0.5, 1.0, 0.0);
        gtk_container_add (GTK_CONTAINER (alignment), header->priv->volume_button);
        gtk_box_pack_end (GTK_BOX (header), alignment, FALSE, TRUE, 5);

        g_return_val_if_fail (header->priv != NULL, NULL);

        return GTK_WIDGET (header);
}

static void
header_change_total_time (Header *header)
{
        LOG_FUNCTION_START
        char *tmp;

        if (mpd_is_connected (header->priv->mpd))
                header->priv->total_time = mpd_get_current_total_time (header->priv->mpd);
        else
                header->priv->total_time = 0;

        tmp = util_format_time (header->priv->total_time);
        gtk_label_set_text (GTK_LABEL (header->priv->total), tmp);
        g_free (tmp);

        header->priv->adjustment->upper = header->priv->total_time;
}

static void
header_change_labels (Header *header)
{
        LOG_FUNCTION_START
        char *title;
        char *artist;
        char *album;
        char *tmp;
        gchar *cover_path;
        GdkPixbuf *cover;

        switch (mpd_get_current_state (header->priv->mpd)) {
        case MPD_STATUS_STATE_PLAY:
        case MPD_STATUS_STATE_PAUSE:
                title = mpd_get_current_title (header->priv->mpd);
                artist = mpd_get_current_artist (header->priv->mpd);
                album = mpd_get_current_album (header->priv->mpd);

                if (!title)
                        title = _("Unknown");

                if (!album)
                        album = _("Unknown");

                if (!artist)
                        artist = _("Unknown");

                cover_path = cover_make_cover_path (artist, album, SMALL_COVER);
                cover = gdk_pixbuf_new_from_file_at_size (cover_path, header->priv->image_width, header->priv->image_height, NULL);
                g_free (cover_path);
                gtk_image_set_from_pixbuf(GTK_IMAGE(header->priv->image), cover);

                tmp = SONG_MARKUP (title);
                gtk_label_set_markup (GTK_LABEL (header->priv->song), tmp);
                g_free (tmp);

                tmp = FROM_MARKUP (album, artist);
                gtk_label_set_label (GTK_LABEL (header->priv->artist_album), tmp);
                g_free (tmp);
                break;
        case MPD_STATUS_STATE_UNKNOWN:
        case MPD_STATUS_STATE_STOP:
        default:
                gtk_label_set_label (GTK_LABEL (header->priv->song), "");
                gtk_label_set_label (GTK_LABEL (header->priv->artist_album), "");
                gtk_image_set_from_pixbuf(GTK_IMAGE(header->priv->image), NULL);
                break;
        }
}

static void
header_song_changed_cb (Mpd *mpd,
                        Header *header)
{
        LOG_FUNCTION_START
        header_change_labels (header);
        header_change_total_time (header);
}

static void
header_state_changed_cb (Mpd *mpd,
                         Header *header)
{
        LOG_FUNCTION_START
        if (!mpd_is_connected (mpd)) {
                gtk_widget_set_sensitive (header->priv->prev_button, FALSE);
                gtk_widget_set_sensitive (header->priv->play_pause_button, FALSE);

                gtk_widget_set_sensitive (header->priv->random_button, FALSE);
                gtk_widget_set_sensitive (header->priv->repeat_button, FALSE);

                gtk_widget_set_sensitive (header->priv->stop_button, FALSE);
                gtk_widget_set_sensitive (header->priv->next_button, FALSE);

                gtk_widget_set_sensitive (header->priv->scale, FALSE);

                gtk_widget_set_sensitive (header->priv->volume_button, FALSE);
        } else {
                gtk_widget_set_sensitive (header->priv->prev_button, TRUE);
                gtk_widget_set_sensitive (header->priv->play_pause_button, TRUE);

                gtk_widget_set_sensitive (header->priv->random_button, TRUE);
                gtk_widget_set_sensitive (header->priv->repeat_button, TRUE);

                gtk_widget_set_sensitive (header->priv->stop_button, TRUE);
                gtk_widget_set_sensitive (header->priv->next_button, TRUE);

                gtk_widget_set_sensitive (header->priv->scale, TRUE);

                gtk_widget_set_sensitive (header->priv->volume_button, TRUE);
        }

        header_change_labels (header);
        header_change_total_time (header);

        gtk_container_remove (GTK_CONTAINER (header->priv->play_pause_button),
                              gtk_bin_get_child (GTK_BIN (header->priv->play_pause_button)));

        if (mpd_is_paused (mpd))
                gtk_container_add (GTK_CONTAINER (header->priv->play_pause_button),
                                   header->priv->play_image);
        else
                gtk_container_add (GTK_CONTAINER (header->priv->play_pause_button),
                                   header->priv->pause_image);
}

static void
header_elapsed_changed_cb (Mpd *mpd,
                        Header *header)
{
        LOG_FUNCTION_START
        int elapsed;
        char *tmp;

        if (header->priv->slider_dragging)
                return;

        elapsed = mpd_get_current_elapsed (mpd);

        tmp = util_format_time (elapsed);
        gtk_label_set_text (GTK_LABEL (header->priv->elapsed), tmp);
        g_free (tmp);

        gtk_adjustment_set_value (header->priv->adjustment, (gdouble) elapsed);
}

static void
header_random_changed_cb (Mpd *mpd,
                          Header *header)
{
        LOG_FUNCTION_START
        gboolean random;

        random = mpd_get_current_random (mpd);
        g_signal_handlers_disconnect_by_func (G_OBJECT (header->priv->random_button),
                                              G_CALLBACK (header_do_random),
                                              header);
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (header->priv->random_button),
                                      random);
        g_signal_connect_swapped (G_OBJECT (header->priv->random_button),
                                  "clicked", G_CALLBACK (header_do_random), header);
}

static void
header_repeat_changed_cb (Mpd *mpd,
                          Header *header)
{
        LOG_FUNCTION_START
        gboolean repeat;

        repeat = mpd_get_current_repeat (mpd);
        g_signal_handlers_disconnect_by_func (G_OBJECT (header->priv->repeat_button),
                                              G_CALLBACK (header_do_repeat),
                                              header);
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (header->priv->repeat_button),
                                      repeat);
        g_signal_connect_swapped (G_OBJECT (header->priv->repeat_button),
                                  "clicked", G_CALLBACK (header_do_repeat), header);
}

static gboolean
slider_press_callback (GtkWidget *widget,
                       GdkEventButton *event,
                       Header *header)
{
        LOG_FUNCTION_START
        header->priv->slider_dragging = TRUE;
        return FALSE;
}

static gboolean
slider_release_callback (GtkWidget *widget,
                         GdkEventButton *event,
                         Header *header)
{
        LOG_FUNCTION_START
        header->priv->slider_dragging = FALSE;
        mpd_set_current_elapsed (header->priv->mpd, 
                                 (int) gtk_range_get_value (GTK_RANGE (header->priv->scale)));
        return FALSE;
}

void
header_do_next (Header *header)
{
        LOG_FUNCTION_START
        g_return_if_fail (IS_HEADER (header));
        mpd_do_next (header->priv->mpd);
}

void
header_do_previous (Header *header)
{
        LOG_FUNCTION_START
        g_return_if_fail (IS_HEADER (header));
        mpd_do_prev (header->priv->mpd);
}

void
header_playpause (Header *header)
{
        LOG_FUNCTION_START
        g_return_if_fail (IS_HEADER (header));
        if (mpd_is_paused (header->priv->mpd))
                mpd_do_play (header->priv->mpd);
        else
                mpd_do_pause (header->priv->mpd);
}

void
header_stop (Header *header)
{
        LOG_FUNCTION_START
        g_return_if_fail (IS_HEADER (header));
        mpd_do_stop (header->priv->mpd);
}

static void
header_do_random (Header *header)
{
        LOG_FUNCTION_START
        g_return_if_fail (IS_HEADER (header));
        mpd_set_current_random (header->priv->mpd, !mpd_get_current_random (header->priv->mpd));
}

static void
header_do_repeat (Header *header)
{
        LOG_FUNCTION_START
        g_return_if_fail (IS_HEADER (header));
        mpd_set_current_repeat (header->priv->mpd, !mpd_get_current_repeat (header->priv->mpd));
}

static void
header_cmd_playpause (GtkAction *action,
                      Header *header)
{
        LOG_FUNCTION_START
        header_playpause (header);
}

static void
header_cmd_stop (GtkAction *action,
                 Header *header)
{
        LOG_FUNCTION_START
        header_stop (header);
}

static void
header_cmd_next (GtkAction *action,
                      Header *header)
{
        LOG_FUNCTION_START
        header_do_next (header);
}

static void
header_cmd_previous (GtkAction *action,
                      Header *header)
{
        LOG_FUNCTION_START
        header_do_previous (header);
}
