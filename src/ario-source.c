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
#include <string.h>
#include <config.h>
#include "ario-source.h"
#include "ario-browser.h"
#include "ario-radio.h"
#include "ario-preferences.h"
#include "ario-debug.h"

static void ario_source_class_init (ArioSourceClass *klass);
static void ario_source_init (ArioSource *source);
static void ario_source_finalize (GObject *object);

struct ArioSourcePrivate
{        
        GtkWidget *browser;
        GtkWidget *radio;
};

static GObjectClass *parent_class = NULL;

GType
ario_source_get_type (void)
{
        ARIO_LOG_FUNCTION_START
        static GType type = 0;

        if (!type) {
                static const GTypeInfo our_info =
                {
                        sizeof (ArioSourceClass),
                        NULL,
                        NULL,
                        (GClassInitFunc) ario_source_class_init,
                        NULL,
                        NULL,
                        sizeof (ArioSource),
                        0,
                        (GInstanceInitFunc) ario_source_init
                };

                type = g_type_register_static (GTK_TYPE_HBOX,
                                               "ArioSource",
                                                &our_info, 0);
        }
        return type;
}

static void
ario_source_class_init (ArioSourceClass *klass)
{
        ARIO_LOG_FUNCTION_START
        GObjectClass *object_class = G_OBJECT_CLASS (klass);

        parent_class = g_type_class_peek_parent (klass);

        object_class->finalize = ario_source_finalize;
}

static void
ario_source_init (ArioSource *source)
{
        ARIO_LOG_FUNCTION_START

        source->priv = g_new0 (ArioSourcePrivate, 1);
}

static void
ario_source_finalize (GObject *object)
{
        ARIO_LOG_FUNCTION_START
        ArioSource *source;

        g_return_if_fail (object != NULL);
        g_return_if_fail (IS_ARIO_SOURCE (object));

        source = ARIO_SOURCE (object);

        g_return_if_fail (source->priv != NULL);
        g_free (source->priv->browser);
        g_free (source->priv->radio);
        g_free (source->priv);

        G_OBJECT_CLASS (parent_class)->finalize (object);
}

GtkWidget *
ario_source_new (GtkUIManager *mgr,
                 GtkActionGroup *group,
                 ArioMpd *mpd,
                 ArioPlaylist *playlist)
{
        ARIO_LOG_FUNCTION_START
        ArioSource *source;

        source = g_object_new (TYPE_ARIO_SOURCE,
                               NULL);

        source->priv->browser = ario_browser_new (mgr,
                                                  group,
                                                  mpd,
                                                  playlist);
        gtk_box_pack_start (GTK_BOX (source), source->priv->browser, TRUE, TRUE, 0);
#ifdef ENABLE_RADIOS
        source->priv->radio = ario_radio_new (mgr,
                                              group,
                                              mpd,
                                              playlist);
        gtk_box_pack_start (GTK_BOX (source), source->priv->radio, TRUE, TRUE, 0);
#endif  /* ENABLE_RADIOS */
        g_return_val_if_fail (source->priv != NULL, NULL);

        return GTK_WIDGET (source);
}

void
ario_source_set_source (ArioSource *source,
                        ArioSourceType source_type)
{
        ARIO_LOG_FUNCTION_START
#ifdef ENABLE_RADIOS
        if (source_type == ARIO_SOURCE_RADIO) {
                gtk_widget_show_all (GTK_WIDGET (source->priv->radio));
                gtk_widget_hide_all (GTK_WIDGET (source->priv->browser));
        } else {
                gtk_widget_show_all (GTK_WIDGET (source->priv->browser));
                gtk_widget_hide_all (GTK_WIDGET (source->priv->radio));
        }
#endif  /* ENABLE_RADIOS */
}


