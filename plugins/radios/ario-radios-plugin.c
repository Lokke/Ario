/*
 * Copyright (C) 2002-2005 - Paolo Maggi
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <string.h> /* For strlen */

#include "ario-radios-plugin.h"

#include <glib/gi18n-lib.h>
#include <gtk/gtk.h>

#include <ario-debug.h>
#include <ario-shell.h>
#include <ario-source-manager.h>
#include "ario-radio.h"

#define ARIO_RADIOS_PLUGIN_GET_PRIVATE(object)(G_TYPE_INSTANCE_GET_PRIVATE ((object), ARIO_TYPE_RADIOS_PLUGIN, ArioRadiosPluginPrivate))

#ifdef WIN32
#undef UI_PATH 
#define UI_PATH "plugins\\"
#endif

struct _ArioRadiosPluginPrivate
{
        guint ui_merge_id;
        GtkWidget *source;
};

ARIO_PLUGIN_REGISTER_TYPE(ArioRadiosPlugin, ario_radios_plugin)

static void
ario_radios_plugin_init (ArioRadiosPlugin *plugin)
{
        plugin->priv = ARIO_RADIOS_PLUGIN_GET_PRIVATE (plugin);
}

static void
ario_radios_plugin_finalize (GObject *object)
{
        G_OBJECT_CLASS (ario_radios_plugin_parent_class)->finalize (object);
}

static void
impl_activate (ArioPlugin *plugin,
               ArioShell *shell)
{
        GtkUIManager *uimanager;
        GtkActionGroup *actiongroup;
        ArioMpd *mpd;
        ArioRadiosPlugin *pi = ARIO_RADIOS_PLUGIN (plugin);
        ArioSourceManager *sourcemanager;

        g_object_get (shell, "ui-manager", &uimanager, NULL);
        g_object_get (shell, "action-group", &actiongroup, NULL);
        g_object_get (shell, "mpd", &mpd, NULL);

        pi->priv->source = ario_radio_new (uimanager,
                                           actiongroup,
                                           mpd,
                                           ARIO_PLAYLIST (ario_shell_get_playlist (shell)));
        g_return_if_fail (IS_ARIO_RADIO (pi->priv->source));
        pi->priv->ui_merge_id = gtk_ui_manager_add_ui_from_file (uimanager,
                                                                 UI_PATH "radios-ui.xml", NULL);

        g_object_unref (uimanager);
        g_object_unref (actiongroup);
        g_object_unref (mpd);

        sourcemanager = ARIO_SOURCEMANAGER (ario_shell_get_sourcemanager (shell));
        ario_sourcemanager_append (sourcemanager,
                                   ARIO_SOURCE (pi->priv->source));
        ario_sourcemanager_reorder (sourcemanager);
}

static void
impl_deactivate (ArioPlugin *plugin,
                 ArioShell *shell)
{
        GtkUIManager *uimanager;

        ArioRadiosPlugin *pi = ARIO_RADIOS_PLUGIN (plugin);

        g_object_get (shell, "ui-manager", &uimanager, NULL);
        gtk_ui_manager_remove_ui (uimanager, pi->priv->ui_merge_id);
        g_object_unref (uimanager);

        ario_sourcemanager_remove (ARIO_SOURCEMANAGER (ario_shell_get_sourcemanager (shell)),
                                   ARIO_SOURCE (pi->priv->source));
}

static void
ario_radios_plugin_class_init (ArioRadiosPluginClass *klass)
{
        GObjectClass *object_class = G_OBJECT_CLASS (klass);
        ArioPluginClass *plugin_class = ARIO_PLUGIN_CLASS (klass);

        object_class->finalize = ario_radios_plugin_finalize;

        plugin_class->activate = impl_activate;
        plugin_class->deactivate = impl_deactivate;

        g_type_class_add_private (object_class, sizeof (ArioRadiosPluginPrivate));
}