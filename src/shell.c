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
#include <gdk/gdk.h>
#include <config.h>
#include <i18n.h>


#include "shell.h"
#include "mpd.h"
#include "browser.h"
#include "playlist.h"
#include "header.h"
#include "tray-icon.h"
#include "status-bar.h"
#include "preferences.h"
#include "eel/eel-gconf-extensions.h"
#include "shell-coverdownloader.h"
#include "debug.h"

static void shell_class_init (ShellClass *klass);
static void shell_init (Shell *shell);
static void shell_finalize (GObject *object);
static void shell_set_property (GObject *object,
                                guint prop_id,
                                const GValue *value,
                                GParamSpec *pspec);
static void shell_get_property (GObject *object,
                                guint prop_id,
                                GValue *value,
                                GParamSpec *pspec);
static void shell_cmd_quit (GtkAction *action,
                            Shell *shell);
static void shell_cmd_preferences (GtkAction *action,
                                   Shell *shell);
static void shell_cmd_covers (GtkAction *action,
                              Shell *shell);
static void shell_cmd_about (GtkAction *action,
                             Shell *shell);
static void shell_vpaned_size_allocate_cb (GtkWidget *widget,
                                           GtkAllocation *allocation,
                                           Shell *shell);
static void shell_paned_changed_cb (GConfClient *client,
                                    guint cnxn_id,
                                    GConfEntry *entry,
                                    Shell *shell);
static gboolean shell_window_state_cb (GtkWidget *widget,
                                       GdkEvent *event,
                                       Shell *shell);
static void shell_sync_window_state (Shell *shell);
static void shell_sync_paned (Shell *shell);

enum
{
        PROP_NONE,
        PROP_UI_MANAGER
};

struct ShellPrivate
{
        GtkWidget *window;

        Mpd *mpd;
        GtkWidget *header;
        GtkWidget *vpaned;
        GtkWidget *browser;
        GtkWidget *playlist;
        GtkWidget *status_bar;

        GtkUIManager *ui_manager;
        GtkActionGroup *actiongroup;

        TrayIcon *tray_icon;
};

static GtkActionEntry shell_actions [] =
{
        { "File", NULL, N_("_File") },
        { "Edit", NULL, N_("_Edit") },
        { "Tool", NULL, N_("_Tool") },
        { "Help", NULL, N_("_Help") },

        { "FileQuit", GTK_STOCK_QUIT, N_("_Quit"), "<control>Q",
          N_("Quit"),
          G_CALLBACK (shell_cmd_quit) },
        { "EditPreferences", GTK_STOCK_PREFERENCES, N_("Prefere_nces"), NULL,
          N_("Edit music player preferences"),
          G_CALLBACK (shell_cmd_preferences) },
        { "ToolCover", GTK_STOCK_EXECUTE, N_("Download album _covers"), NULL,
          N_("Download covers form amazon"),
          G_CALLBACK (shell_cmd_covers) },
        { "HelpAbout", GTK_STOCK_ABOUT, N_("_About"), NULL,
          N_("Show information about the music player"),
          G_CALLBACK (shell_cmd_about) }
};

static guint shell_n_actions = G_N_ELEMENTS (shell_actions);

static GObjectClass *parent_class;

GType
shell_get_type (void)
{
        LOG_FUNCTION_START
        static GType type = 0;
                                                                              
        if (type == 0)
        { 
                static GTypeInfo info =
                {
                        sizeof (ShellClass),
                        NULL, 
                        NULL,
                        (GClassInitFunc) shell_class_init, 
                        NULL,
                        NULL, 
                        sizeof (Shell),
                        0,
                        (GInstanceInitFunc) shell_init
                };

                type = g_type_register_static (G_TYPE_OBJECT,
                                               "Shell",
                                               &info, 0);
        }

        return type;
}

static void
shell_class_init (ShellClass *klass)
{
        LOG_FUNCTION_START
        GObjectClass *object_class = (GObjectClass *) klass;

        parent_class = g_type_class_peek_parent (klass);

        object_class->set_property = shell_set_property;
        object_class->get_property = shell_get_property;
        object_class->finalize = shell_finalize;

        g_object_class_install_property (object_class,
                                         PROP_UI_MANAGER,
                                         g_param_spec_object ("ui-manager", 
                                                              "GtkUIManager", 
                                                              "GtkUIManager object", 
                                                              GTK_TYPE_UI_MANAGER,
                                                               G_PARAM_READABLE));
}

static void
shell_init (Shell *shell) 
{
        LOG_FUNCTION_START
        shell->priv = g_new0 (ShellPrivate, 1);
}

static void
shell_set_property (GObject *object,
                    guint prop_id,
                    const GValue *value,
                    GParamSpec *pspec)
{
        LOG_FUNCTION_START
        /*Shell *shell = SHELL (object);*/

        switch (prop_id)
        {
        default:
                G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
                break;
        }
}

static void
shell_get_property (GObject *object,
                    guint prop_id,
                    GValue *value,
                    GParamSpec *pspec)
{
        LOG_FUNCTION_START
        Shell *shell = SHELL (object);

        switch (prop_id)
        {
        case PROP_UI_MANAGER:
                g_value_set_object (value, shell->priv->ui_manager);
                break;
        default:
                G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
                break;
        }
}

static void
shell_finalize (GObject *object)
{
        LOG_FUNCTION_START
        Shell *shell = SHELL (object);

        gtk_widget_hide (shell->priv->window);
        gtk_widget_destroy (shell->priv->window);
        
        g_free (shell->priv);

        parent_class->finalize (G_OBJECT (shell));
}

Shell *
shell_new (void)
{
        LOG_FUNCTION_START
        Shell *s;

        s = g_object_new (TYPE_SHELL, NULL);

        return s;
}

static gboolean
shell_window_delete_cb (GtkWidget *win,
                        GdkEventAny *event,
                        Shell *shell)
{
        LOG_FUNCTION_START
        gtk_main_quit ();
        return TRUE;
};

void
shell_construct (Shell *shell)
{
        LOG_FUNCTION_START
        GtkWindow *win;
        GtkWidget *menubar;
        GtkWidget *vbox;
        GtkWidget *icon;
        GdkPixbuf *pixbuf;
        GError *error = NULL;

        g_return_if_fail (IS_SHELL (shell));

        /* initialize UI */
        win = GTK_WINDOW (gtk_window_new (GTK_WINDOW_TOPLEVEL));
        gtk_window_set_title (win, "Ario");
        pixbuf = gdk_pixbuf_new_from_file (PIXMAP_PATH "icon.png", NULL);
        gtk_window_set_default_icon (pixbuf);

        shell->priv->window = GTK_WIDGET (win);

        g_signal_connect_object (G_OBJECT (win), "delete_event",
                                 G_CALLBACK (shell_window_delete_cb),
                                 shell, 0);
  
        shell->priv->ui_manager = gtk_ui_manager_new ();
        shell->priv->actiongroup = gtk_action_group_new ("MainActions");

        gtk_action_group_set_translation_domain (shell->priv->actiongroup,
                                                 GETTEXT_PACKAGE);

        /* initialize shell services */
        vbox = gtk_vbox_new (FALSE, 0);

        shell->priv->mpd = mpd_new ();
        shell->priv->header = header_new (shell->priv->actiongroup, shell->priv->mpd);
        shell->priv->playlist = playlist_new (shell->priv->ui_manager, shell->priv->actiongroup, shell->priv->mpd);
        shell->priv->browser = browser_new (shell->priv->ui_manager, shell->priv->actiongroup, shell->priv->mpd, PLAYLIST (shell->priv->playlist));

        gtk_action_group_add_actions (shell->priv->actiongroup,
                                      shell_actions,
                                      shell_n_actions, shell);
        gtk_ui_manager_insert_action_group (shell->priv->ui_manager,
                                            shell->priv->actiongroup, 0);
        gtk_ui_manager_add_ui_from_file (shell->priv->ui_manager,
                                         UI_PATH "ario-ui.xml", &error);
        gtk_window_add_accel_group (GTK_WINDOW (shell->priv->window),
                                    gtk_ui_manager_get_accel_group (shell->priv->ui_manager));

        menubar = gtk_ui_manager_get_widget (shell->priv->ui_manager, "/MenuBar");
        shell->priv->vpaned = gtk_vpaned_new ();
        shell->priv->status_bar = status_bar_new (shell->priv->mpd);


        gtk_paned_add1 (GTK_PANED (shell->priv->vpaned),
                        shell->priv->browser);

        gtk_paned_add2 (GTK_PANED (shell->priv->vpaned),
                        shell->priv->playlist);

        gtk_box_pack_start (GTK_BOX (vbox),
                            menubar,
                            FALSE, FALSE, 0);

        gtk_box_pack_start (GTK_BOX (vbox),
                            shell->priv->header,
                            FALSE, FALSE, 0);

        gtk_box_pack_start (GTK_BOX (vbox),
                            shell->priv->vpaned,
                            TRUE, TRUE, 0);

        gtk_box_pack_start (GTK_BOX (vbox),
                            shell->priv->status_bar,
                            FALSE, FALSE, 0);

        gtk_container_add (GTK_CONTAINER (win), vbox);

        shell_sync_window_state (shell);
        gtk_widget_show_all (GTK_WIDGET (win));
        shell_sync_paned (shell);

        /* initialize tray icon */
        shell->priv->tray_icon = tray_icon_new (shell->priv->ui_manager,
                                                win,
                                                shell->priv->mpd);
        gtk_widget_show_all (GTK_WIDGET (shell->priv->tray_icon));

        eel_gconf_notification_add (STATE_VPANED_POSITION,
                                    (GConfClientNotifyFunc) shell_paned_changed_cb,
                                    shell);

        g_signal_connect_object (G_OBJECT (win), "window-state-event",
                                 G_CALLBACK (shell_window_state_cb),
                                 shell, 0);

        g_signal_connect_object (G_OBJECT (win), "configure-event",
                                 G_CALLBACK (shell_window_state_cb),
                                 shell, 0);

        g_signal_connect_object (G_OBJECT (shell->priv->playlist),
                                 "size_allocate",
                                 G_CALLBACK (shell_vpaned_size_allocate_cb),
                                 shell, 0);

        g_timeout_add (500, (GSourceFunc) mpd_update_status, shell->priv->mpd);
}

static void
shell_cmd_quit (GtkAction *action,
                Shell *shell)
{
        LOG_FUNCTION_START
        gtk_main_quit ();
}

static void
shell_cmd_preferences (GtkAction *action,
                       Shell *shell)
{
        LOG_FUNCTION_START
        GtkWidget *prefs;

        prefs = preferences_new (shell->priv->mpd);

        gtk_window_set_transient_for (GTK_WINDOW (prefs),
                                      GTK_WINDOW (shell->priv->window));

        gtk_widget_show_all (prefs);
}

static void
shell_cmd_about (GtkAction *action,
                 Shell *shell)
{
        LOG_FUNCTION_START
        const char *authors[] = {
                "",
#include "AUTHORS"
                "",
                NULL
        };

        gtk_show_about_dialog (GTK_WINDOW (shell->priv->window),
                               "name", "Ario",
                               "version", VERSION,
                               "copyright", "Copyright \xc2\xa9 2005-2007 Marc Pavot",
                               "comments", "Music player and browser for MPD",
                               "authors", (const char **) authors,
                               NULL);
}

static void
shell_cmd_covers (GtkAction *action,
                  Shell *shell)
{
        LOG_FUNCTION_START
        GtkWidget *coverdownloader;

        coverdownloader = shell_coverdownloader_new (shell->priv->mpd);

        shell_coverdownloader_get_covers (SHELL_COVERDOWNLOADER (coverdownloader),
                                          GET_AMAZON_COVERS);

        gtk_widget_destroy (coverdownloader);
}

static void
shell_sync_paned (Shell *shell)
{
        LOG_FUNCTION_START
        int pos;

        pos = eel_gconf_get_integer (STATE_VPANED_POSITION);
        if (pos > 0)
                gtk_paned_set_position (GTK_PANED (shell->priv->vpaned),
                                        pos);
}

static void
shell_paned_changed_cb (GConfClient *client,
                        guint cnxn_id,
                        GConfEntry *entry,
                        Shell *shell)
{
        LOG_FUNCTION_START
        shell_sync_paned (shell);
}

static void
shell_vpaned_size_allocate_cb (GtkWidget *widget,
                               GtkAllocation *allocation,
                               Shell *shell)
{
        LOG_FUNCTION_START
        eel_gconf_set_integer (STATE_VPANED_POSITION,
                               gtk_paned_get_position (GTK_PANED (shell->priv->vpaned)));
}

static gboolean
shell_window_state_cb (GtkWidget *widget,
                       GdkEvent *event,
                       Shell *shell)
{
        LOG_FUNCTION_START
        g_return_val_if_fail (widget != NULL, FALSE);

        switch (event->type)
        {
        case GDK_WINDOW_STATE:
                eel_gconf_set_boolean (CONF_STATE_WINDOW_MAXIMIZED,
                                       event->window_state.new_window_state &
                                       GDK_WINDOW_STATE_MAXIMIZED);
                break;
        case GDK_CONFIGURE:
                if (!eel_gconf_get_boolean (CONF_STATE_WINDOW_MAXIMIZED)) {
                        eel_gconf_set_integer (CONF_STATE_WINDOW_WIDTH, event->configure.width);
                        eel_gconf_set_integer (CONF_STATE_WINDOW_HEIGHT, event->configure.height);
                }
                break;
        default:
                break;
        }

        return FALSE;
}

static void
shell_sync_window_state (Shell *shell)
{
        LOG_FUNCTION_START
        int width = eel_gconf_get_integer (CONF_STATE_WINDOW_WIDTH); 
        int height = eel_gconf_get_integer (CONF_STATE_WINDOW_HEIGHT);
        gboolean maximized = eel_gconf_get_boolean (CONF_STATE_WINDOW_MAXIMIZED);
        GdkGeometry hints;

        gtk_window_set_default_size (GTK_WINDOW (shell->priv->window),
                                     width, height);
        gtk_window_resize (GTK_WINDOW (shell->priv->window),
                           width, height);
        gtk_window_set_geometry_hints (GTK_WINDOW (shell->priv->window),
                                        NULL,
                                        &hints,
                                        0);

        if (maximized)
                gtk_window_maximize (GTK_WINDOW (shell->priv->window));
        else
                gtk_window_unmaximize (GTK_WINDOW (shell->priv->window));
}
