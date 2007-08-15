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

#ifndef __BROWSER_H
#define __BROWSER_H

#include <gtk/gtkhbox.h>
#include "mpd.h"
#include "playlist.h"

G_BEGIN_DECLS

#define TYPE_BROWSER         (browser_get_type ())
#define BROWSER(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), TYPE_BROWSER, Browser))
#define BROWSER_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST((k), TYPE_BROWSER, BrowserClass))
#define IS_BROWSER(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), TYPE_BROWSER))
#define IS_BROWSER_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), TYPE_BROWSER))
#define BROWSER_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), TYPE_BROWSER, BrowserClass))

typedef struct BrowserPrivate BrowserPrivate;

typedef struct
{
        GtkHBox parent;

        BrowserPrivate *priv;
} Browser;

typedef struct
{
        GtkHBoxClass parent;
} BrowserClass;

GType                        browser_get_type        (void);

GtkWidget *                 browser_new                (GtkUIManager *mgr,
                                                 GtkActionGroup *group,
                                                 Mpd *mpd,
                                                 Playlist *playlist);

G_END_DECLS

#endif /* __BROWSER_H */
