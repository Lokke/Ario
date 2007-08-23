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

#include <gtk/gtkdialog.h>
#include "ario-mpd.h"

#ifndef __ARIO_PREFERENCES_H
#define __ARIO_PREFERENCES_H

G_BEGIN_DECLS

#define TYPE_ARIO_PREFERENCES         (ario_preferences_get_type ())
#define ARIO_PREFERENCES(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), TYPE_ARIO_PREFERENCES, ArioPreferences))
#define ARIO_PREFERENCES_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST((k), TYPE_ARIO_PREFERENCES, ArioPreferencesClass))
#define IS_ARIO_PREFERENCES(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), TYPE_ARIO_PREFERENCES))
#define IS_ARIO_PREFERENCES_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), TYPE_ARIO_PREFERENCES))
#define ARIO_PREFERENCES_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), TYPE_ARIO_PREFERENCES, ArioPreferencesClass))

#define CONF_HOST                       "/apps/ario/host"
#define CONF_PORT                       "/apps/ario/port"
#define CONF_TIMEOUT                    "/apps/ario/timeout"
#define CONF_AUTOCONNECT                "/apps/ario/autoconnect"
#define CONF_AUTH                       "/apps/ario/state/auth"
#define CONF_PASSWORD                   "/apps/ario/password"
#define CONF_COVER_TREE_HIDDEN          "/apps/ario/ario_cover_tree_hidden"
#define CONF_COVER_AMAZON_COUNTRY       "/apps/ario/ario_cover_amazon_country"
#define CONF_STATE_WINDOW_WIDTH         "/apps/ario/state/window_width"
#define CONF_STATE_WINDOW_HEIGHT        "/apps/ario/state/window_height"
#define CONF_STATE_WINDOW_MAXIMIZED     "/apps/ario/state/window_maximized"
#define STATE_VPANED_POSITION           "/apps/ario/state/vpaned_position"
#define CONF_USE_PROXY                  "/apps/ario/use_proxy"
#define CONF_PROXY_ADDRESS              "/apps/ario/proxy_address"
#define CONF_PROXY_PORT                 "/apps/ario/proxy_port"

typedef struct ArioPreferencesPrivate ArioPreferencesPrivate;

typedef struct
{
        GtkDialog parent;

        ArioPreferencesPrivate *priv;
} ArioPreferences;

typedef struct
{
        GtkDialogClass parent_class;
} ArioPreferencesClass;

GType              ario_preferences_get_type         (void);

GtkWidget *        ario_preferences_new              (ArioMpd *mpd);

G_END_DECLS

#endif /* __ARIO_PREFERENCES_H */