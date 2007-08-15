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
#include "mpd.h"

#ifndef __PREFERENCES_H
#define __PREFERENCES_H

G_BEGIN_DECLS

#define TYPE_PREFERENCES         (preferences_get_type ())
#define PREFERENCES(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), TYPE_PREFERENCES, Preferences))
#define PREFERENCES_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST((k), TYPE_PREFERENCES, PreferencesClass))
#define IS_PREFERENCES(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), TYPE_PREFERENCES))
#define IS_PREFERENCES_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), TYPE_PREFERENCES))
#define PREFERENCES_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), TYPE_PREFERENCES, PreferencesClass))

#define CONF_HOST                        "/apps/ario/host"
#define CONF_PORT                        "/apps/ario/port"
#define CONF_TIMEOUT                        "/apps/ario/timeout"
#define CONF_AUTOCONNECT                "/apps/ario/autoconnect"
#define CONF_AUTH                        "/apps/ario/state/auth"
#define CONF_PASSWORD                        "/apps/ario/password"
#define CONF_COVER_TREE_HIDDEN                "/apps/ario/cover_tree_hidden"
#define CONF_COVER_AMAZON_COUNTRY        "/apps/ario/cover_amazon_country"
#define CONF_STATE_WINDOW_WIDTH                "/apps/ario/state/window_width"
#define CONF_STATE_WINDOW_HEIGHT        "/apps/ario/state/window_height"
#define CONF_STATE_WINDOW_MAXIMIZED        "/apps/ario/state/window_maximized"
#define STATE_VPANED_POSITION                "/apps/ario/state/vpaned_position"

typedef struct PreferencesPrivate PreferencesPrivate;

typedef struct
{
        GtkDialog parent;

        PreferencesPrivate *priv;
} Preferences;

typedef struct
{
        GtkDialogClass parent_class;
} PreferencesClass;

GType              preferences_get_type         (void);

GtkWidget *        preferences_new              (Mpd *mpd);

gboolean           preferences_update_mpd       (Preferences *preferences);

G_END_DECLS

#endif /* __PREFERENCES_H */
