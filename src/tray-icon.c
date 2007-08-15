/*
 *  Copyright (C) 2005 Marc Pavot <marc.pavot@gmail.com>
 *
 *   Based on:
 *   Implementation of Rhythmbox tray icon object
 *   Copyright (C) 2003,2004 Colin Walters <walters@rhythmbox.org>
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
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <i18n.h>
#include <libgnomevfs/gnome-vfs.h>
#include <libgnomevfs/gnome-vfs-mime-utils.h>

#include "tray-icon.h"
#include "preferences.h"
#include "eel/eel-gconf-extensions.h"
#include "debug.h"

static void tray_icon_class_init (TrayIconClass *klass);
static void tray_icon_init (TrayIcon *shell_player);
static GObject *tray_icon_constructor (GType type, guint n_construct_properties,
                                       GObjectConstructParam *construct_properties);
static void tray_icon_finalize (GObject *object);
static void tray_icon_set_property (GObject *object,
                                    guint prop_id,
                                    const GValue *value,
                                    GParamSpec *pspec);
static void tray_icon_get_property (GObject *object,
                                    guint prop_id,
                                    GValue *value,
                                    GParamSpec *pspec);
static void tray_icon_set_visibility (TrayIcon *tray, int state);
static void tray_icon_button_press_event_cb (GtkWidget *ebox, GdkEventButton *event,
                                             TrayIcon *icon);
static void tray_icon_song_changed_cb (Mpd *mpd,
                                       TrayIcon *icon);
static void tray_icon_state_changed_cb (Mpd *mpd,
                                        TrayIcon *icon);

struct TrayIconPrivate
{
        GtkTooltips *tooltips;

        GtkUIManager *ui_manager;

        GtkWindow *main_window;
        GtkWidget *ebox;

        gboolean maximized;
        int window_x;
        int window_y;
        int window_w;
        int window_h;
        gboolean visible;

        Mpd *mpd;
};

enum
{
        PROP_0,
        PROP_UI_MANAGER,
        PROP_WINDOW,
        PROP_MPD
};

enum
{
        VISIBILITY_HIDDEN,
        VISIBILITY_VISIBLE,
        VISIBILITY_TOGGLE
};

enum
{
        LAST_SIGNAL,
};

static GObjectClass *parent_class = NULL;

GType
tray_icon_get_type (void)
{
        LOG_FUNCTION_START
        static GType tray_icon_type = 0;

        if (tray_icon_type == 0)
        {
                static const GTypeInfo our_info =
                {
                        sizeof (TrayIconClass),
                        NULL,
                        NULL,
                        (GClassInitFunc) tray_icon_class_init,
                        NULL,
                        NULL,
                        sizeof (TrayIcon),
                        0,
                        (GInstanceInitFunc) tray_icon_init
                };

                tray_icon_type = g_type_register_static (EGG_TYPE_TRAY_ICON,
                                                         "TrayIcon",
                                                         &our_info, 0);
        }

        return tray_icon_type;
}

static void
tray_icon_class_init (TrayIconClass *klass)
{
        LOG_FUNCTION_START
        GObjectClass *object_class = G_OBJECT_CLASS (klass);

        parent_class = g_type_class_peek_parent (klass);

        object_class->finalize = tray_icon_finalize;
        object_class->constructor = tray_icon_constructor;

        object_class->set_property = tray_icon_set_property;
        object_class->get_property = tray_icon_get_property;

        g_object_class_install_property (object_class,
                                         PROP_WINDOW,
                                         g_param_spec_object ("window",
                                                              "GtkWindow",
                                                              "main GtkWindo",
                                                              GTK_TYPE_WINDOW,
                                                              G_PARAM_READWRITE));
        g_object_class_install_property (object_class,
                                         PROP_UI_MANAGER,
                                         g_param_spec_object ("ui-manager",
                                                              "GtkUIManager",
                                                              "GtkUIManager object",
                                                              GTK_TYPE_UI_MANAGER,
                                                              G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
        g_object_class_install_property (object_class,
                                         PROP_MPD,
                                         g_param_spec_object ("mpd",
                                                              "mpd",
                                                              "mpd",
                                                              TYPE_MPD,
                                                              G_PARAM_READWRITE));
}

static void
tray_icon_init (TrayIcon *icon)
{
        LOG_FUNCTION_START
        GtkWidget *image;

        icon->priv = g_new0 (TrayIconPrivate, 1);

        icon->priv->tooltips = gtk_tooltips_new ();

        gtk_tooltips_set_tip (icon->priv->tooltips, GTK_WIDGET (icon),
                              _("Not playing"), NULL);

        icon->priv->ebox = gtk_event_box_new ();
        g_signal_connect_object (G_OBJECT (icon->priv->ebox),
                                 "button_press_event",
                                 G_CALLBACK (tray_icon_button_press_event_cb),
                                 icon, 0);

        image = gtk_image_new_from_stock ("volume-max",
                                          GTK_ICON_SIZE_SMALL_TOOLBAR);
        gtk_container_add (GTK_CONTAINER (icon->priv->ebox), image);
        
        icon->priv->window_x = -1;
        icon->priv->window_y = -1;
        icon->priv->window_w = -1;
        icon->priv->window_h = -1;
        icon->priv->visible = TRUE;

        gtk_container_add (GTK_CONTAINER (icon), icon->priv->ebox);
        gtk_widget_show_all (GTK_WIDGET (icon->priv->ebox));
}

static GObject *
tray_icon_constructor (GType type, guint n_construct_properties,
                          GObjectConstructParam *construct_properties)
{
        LOG_FUNCTION_START
        TrayIcon *tray;
        TrayIconClass *klass;
        GObjectClass *parent_class;  

        klass = TRAY_ICON_CLASS (g_type_class_peek (TYPE_TRAY_ICON));

        parent_class = G_OBJECT_CLASS (g_type_class_peek_parent (klass));
        tray = TRAY_ICON (parent_class->constructor (type, n_construct_properties,
                                                        construct_properties));

        tray_icon_set_visibility (tray, VISIBILITY_VISIBLE);
        return G_OBJECT (tray);
}

static void
tray_icon_finalize (GObject *object)
{
        LOG_FUNCTION_START
        TrayIcon *tray;

        g_return_if_fail (object != NULL);
        g_return_if_fail (IS_TRAY_ICON (object));

        tray = TRAY_ICON (object);

        g_return_if_fail (tray->priv != NULL);
        
        gtk_object_destroy (GTK_OBJECT (tray->priv->tooltips));

        g_free (tray->priv);

        G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
tray_icon_set_property (GObject *object,
                              guint prop_id,
                              const GValue *value,
                              GParamSpec *pspec)
{
        LOG_FUNCTION_START
        TrayIcon *tray = TRAY_ICON (object);

        switch (prop_id)
        {
        case PROP_WINDOW:
                tray->priv->main_window = g_value_get_object (value);
                break;
        case PROP_UI_MANAGER:
                tray->priv->ui_manager = g_value_get_object (value);
                break;
        case PROP_MPD:
                tray->priv->mpd = g_value_get_object (value);
                g_signal_connect_object (G_OBJECT (tray->priv->mpd),
                                         "song_changed", G_CALLBACK (tray_icon_song_changed_cb),
                                         tray, 0);
                g_signal_connect_object (G_OBJECT (tray->priv->mpd),
                                         "state_changed", G_CALLBACK (tray_icon_state_changed_cb),
                                         tray, 0);
                break;
        default:
                G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
                break;
        }
}

static void 
tray_icon_get_property (GObject *object,
                              guint prop_id,
                              GValue *value,
                              GParamSpec *pspec)
{
        LOG_FUNCTION_START
        TrayIcon *tray = TRAY_ICON (object);

        switch (prop_id)
        {
        case PROP_WINDOW:
                g_value_set_object (value, tray->priv->main_window);
                break;
        case PROP_UI_MANAGER:
                g_value_set_object (value, tray->priv->ui_manager);
                break;
        case PROP_MPD:
                g_value_set_object (value, tray->priv->mpd);
                break;
        default:
                G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
                break;
        }
}

TrayIcon *
tray_icon_new (GtkUIManager *mgr,
               GtkWindow *window,
               Mpd *mpd)
{
        LOG_FUNCTION_START
        return g_object_new (TYPE_TRAY_ICON,
                             "title", _("Ario tray icon"),
                             "ui-manager", mgr,
                             "window", window,
                             "mpd", mpd,
                             NULL);
}

static void
tray_icon_button_press_event_cb (GtkWidget *ebox, GdkEventButton *event,
                                 TrayIcon *icon)
{
        LOG_FUNCTION_START
        /* filter out double, triple clicks */
        if (event->type != GDK_BUTTON_PRESS)
                return;

        switch (event->button) {
        case 1:
                tray_icon_set_visibility (icon, VISIBILITY_TOGGLE);
                break;

        case 3:
        {
                GtkWidget *popup;
                popup = gtk_ui_manager_get_widget (GTK_UI_MANAGER (icon->priv->ui_manager),
                                                   "/TrayPopup");
                gtk_menu_set_screen (GTK_MENU (popup), gtk_widget_get_screen (GTK_WIDGET (icon)));
                gtk_menu_popup (GTK_MENU (popup), NULL, NULL,
                                NULL, NULL, 2,
                                gtk_get_current_event_time ());
        }
        break;
        default:
                break;
        }
}

static void
tray_icon_sync_tooltip (TrayIcon *icon)
{
        LOG_FUNCTION_START
        gchar *tooltip;

        switch (mpd_get_current_state (icon->priv->mpd)) {
        case MPD_STATUS_STATE_PLAY:
        case MPD_STATUS_STATE_PAUSE:
                tooltip = g_strdup_printf (_("Artist: %s\nAlbum: %s\nTitle: %s"), 
                                            mpd_get_current_artist (icon->priv->mpd),
                                            mpd_get_current_album (icon->priv->mpd),
                                            mpd_get_current_title (icon->priv->mpd));
                break;
        default:
                tooltip = g_strdup (_("Not playing"));
                break;
        }

        gtk_tooltips_set_tip (icon->priv->tooltips,
                              GTK_WIDGET (icon),
                              tooltip, NULL);

        g_free (tooltip);
}

static void
tray_icon_song_changed_cb (Mpd *mpd,
                           TrayIcon *icon)
{
        LOG_FUNCTION_START
        tray_icon_sync_tooltip (icon);
}

static void
tray_icon_state_changed_cb (Mpd *mpd,
                            TrayIcon *icon)
{
        LOG_FUNCTION_START
        tray_icon_sync_tooltip (icon);
}

static void
tray_icon_restore_main_window (TrayIcon *icon)
{
        LOG_FUNCTION_START
        if ((icon->priv->window_x >= 0 && icon->priv->window_y >= 0) || (icon->priv->window_h >= 0 && icon->priv->window_w >=0 )) {
                gtk_widget_realize (GTK_WIDGET (icon->priv->main_window));
                gdk_flush ();

                if (icon->priv->window_x >= 0 && icon->priv->window_y >= 0) {
                        gtk_window_move (icon->priv->main_window,
                                         icon->priv->window_x,
                                         icon->priv->window_y);
                }
                if (icon->priv->window_w >= 0 && icon->priv->window_y >=0) {
                        gtk_window_resize (icon->priv->main_window,
                                           icon->priv->window_w,
                                           icon->priv->window_h);
                }
        }

        if (icon->priv->maximized)
                gtk_window_maximize (GTK_WINDOW (icon->priv->main_window));
}

static void
tray_icon_set_visibility (TrayIcon *icon, int state)
{
        LOG_FUNCTION_START
        switch (state)
        {
        case VISIBILITY_HIDDEN:
               case VISIBILITY_VISIBLE:
                if (icon->priv->visible != state)
                        tray_icon_set_visibility (icon, VISIBILITY_TOGGLE);
                break;
        case VISIBILITY_TOGGLE:
                icon->priv->visible = !icon->priv->visible;

                if (icon->priv->visible == TRUE) {
                        tray_icon_restore_main_window (icon);
                        gtk_widget_show (GTK_WIDGET (icon->priv->main_window));
                } else {
                        icon->priv->maximized = eel_gconf_get_boolean (CONF_STATE_WINDOW_MAXIMIZED);
                        gtk_window_get_position (icon->priv->main_window,
                                                 &icon->priv->window_x,
                                                 &icon->priv->window_y);
                        gtk_window_get_size (icon->priv->main_window,
                                             &icon->priv->window_w,
                                             &icon->priv->window_h);
                        gtk_widget_hide (GTK_WIDGET (icon->priv->main_window));
                }
        }
}
