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


#include "status-bar.h"
#include "util.h"
#include <i18n.h>
#include "debug.h"

static void status_bar_class_init (StatusBarClass *klass);
static void status_bar_init (StatusBar *shell_player);
static void status_bar_finalize (GObject *object);
static void status_bar_set_property (GObject *object,
                                    guint prop_id,
                                    const GValue *value,
                                    GParamSpec *pspec);
static void status_bar_get_property (GObject *object,
                                    guint prop_id,
                                    GValue *value,
                                    GParamSpec *pspec);
static void status_bar_playlist_changed_cb (Mpd *mpd,
                                           StatusBar *status_bar);

struct StatusBarPrivate
{
        guint playlist_context_id;

        Mpd *mpd;
};

enum
{
        PROP_NONE,
        PROP_MPD
};

static GObjectClass *parent_class = NULL;

GType
status_bar_get_type (void)
{
        LOG_FUNCTION_START
        static GType status_bar_type = 0;

        if (status_bar_type == 0)
        {
                static const GTypeInfo our_info =
                {
                        sizeof (StatusBarClass),
                        NULL,
                        NULL,
                        (GClassInitFunc) status_bar_class_init,
                        NULL,
                        NULL,
                        sizeof (StatusBar),
                        0,
                        (GInstanceInitFunc) status_bar_init
                };

                status_bar_type = g_type_register_static (GTK_TYPE_STATUSBAR,
                                                         "StatusBar",
                                                         &our_info, 0);
        }

        return status_bar_type;
}

static void
status_bar_class_init (StatusBarClass *klass)
{
        LOG_FUNCTION_START
        GObjectClass *object_class = G_OBJECT_CLASS (klass);

        parent_class = g_type_class_peek_parent (klass);

        object_class->finalize = status_bar_finalize;

        object_class->set_property = status_bar_set_property;
        object_class->get_property = status_bar_get_property;

        g_object_class_install_property (object_class,
                                         PROP_MPD,
                                         g_param_spec_object ("mpd",
                                                              "mpd",
                                                              "mpd",
                                                              TYPE_MPD,
                                                              G_PARAM_READWRITE));
}

static void
status_bar_init (StatusBar *status_bar)
{
        LOG_FUNCTION_START
        status_bar->priv = g_new0 (StatusBarPrivate, 1);
        status_bar->priv->playlist_context_id = gtk_statusbar_get_context_id (GTK_STATUSBAR (status_bar), "PlaylistMsg");
}

static void
status_bar_finalize (GObject *object)
{
        LOG_FUNCTION_START
        StatusBar *status_bar;

        g_return_if_fail (object != NULL);
        g_return_if_fail (IS_STATUS_BAR (object));

        status_bar = STATUS_BAR (object);

        g_return_if_fail (status_bar->priv != NULL);
        g_free (status_bar->priv);

        G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
status_bar_set_property (GObject *object,
                              guint prop_id,
                              const GValue *value,
                              GParamSpec *pspec)
{
        LOG_FUNCTION_START
        StatusBar *status_bar = STATUS_BAR (object);

        switch (prop_id)
        {
        case PROP_MPD:
                status_bar->priv->mpd = g_value_get_object (value);
                g_signal_connect_object (G_OBJECT (status_bar->priv->mpd),
                                         "playlist_changed", G_CALLBACK (status_bar_playlist_changed_cb),
                                         status_bar, 0);
                break;
        default:
                G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
                break;
        }
}

static void 
status_bar_get_property (GObject *object,
                              guint prop_id,
                              GValue *value,
                              GParamSpec *pspec)
{
        LOG_FUNCTION_START
        StatusBar *status_bar = STATUS_BAR (object);

        switch (prop_id)
        {
        case PROP_MPD:
                g_value_set_object (value, status_bar->priv->mpd);
                break;
        default:
                G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
                break;
        }
}

GtkWidget *
status_bar_new (Mpd *mpd)
{
        LOG_FUNCTION_START
        StatusBar *status_bar;

        status_bar = g_object_new (TYPE_STATUS_BAR,
                                   "mpd", mpd,
                                   NULL);

        g_return_val_if_fail (status_bar->priv != NULL, NULL);

        return GTK_WIDGET (status_bar);
}

static void
status_bar_playlist_changed_cb (Mpd *mpd,
                           StatusBar *status_bar)
{
        LOG_FUNCTION_START
        gchar *msg;
        gchar *formated_total_time;
        int playlist_length;
        int playlist_total_time;

        playlist_length = mpd_get_current_playlist_length (mpd);

        playlist_total_time = mpd_get_current_playlist_total_time (mpd);
        formated_total_time = util_format_total_time (playlist_total_time);

        msg = g_strdup_printf (_("%d Songs - %s"), playlist_length, formated_total_time);
        g_free (formated_total_time);

        gtk_statusbar_pop (GTK_STATUSBAR(status_bar), status_bar->priv->playlist_context_id);
        gtk_statusbar_push (GTK_STATUSBAR (status_bar), status_bar->priv->playlist_context_id, msg);

        g_free (msg);
}
