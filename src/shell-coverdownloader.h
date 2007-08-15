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



#ifndef __SHELL_COVERDOWNLOADER_H
#define __SHELL_COVERDOWNLOADER_H

#include "mpd.h"
#include <libgnomevfs/gnome-vfs.h>

G_BEGIN_DECLS

#define TYPE_SHELL_COVERDOWNLOADER         (shell_coverdownloader_get_type ())
#define SHELL_COVERDOWNLOADER(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), TYPE_SHELL_COVERDOWNLOADER, ShellCoverdownloader))
#define SHELL_COVERDOWNLOADER_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST((k), TYPE_SHELL_COVERDOWNLOADER, ShellCoverdownloaderClass))
#define IS_SHELL_COVERDOWNLOADER(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), TYPE_SHELL_COVERDOWNLOADER))
#define IS_SHELL_COVERDOWNLOADER_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), TYPE_SHELL_COVERDOWNLOADER))
#define SHELL_COVERDOWNLOADER_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), TYPE_SHELL_COVERDOWNLOADER, ShellCoverdownloaderClass))

typedef struct ShellCoverdownloaderPrivate ShellCoverdownloaderPrivate;

typedef struct
{
        GtkWindow parent;

        ShellCoverdownloaderPrivate *priv;
} ShellCoverdownloader;

typedef struct
{
        GtkWindowClass parent_class;
} ShellCoverdownloaderClass;

typedef enum
{
        GET_LOCAL_COVERS,
        GET_AMAZON_COVERS,
        REMOVE_COVERS
} ShellCoverdownloaderOperation;

GType           shell_coverdownloader_get_type                  (void);

GtkWidget *     shell_coverdownloader_new                       (Mpd *mpd);

void            shell_coverdownloader_get_covers                (ShellCoverdownloader *shell_coverdownloader,
                                                                 ShellCoverdownloaderOperation operation);
void            shell_coverdownloader_get_covers_from_artists   (ShellCoverdownloader *shell_coverdownloader,
                                                                 GList *artists,
                                                                 ShellCoverdownloaderOperation operation);
void            shell_coverdownloader_get_covers_from_albums    (ShellCoverdownloader *shell_coverdownloader,
                                                                 GList *albums,
                                                                 ShellCoverdownloaderOperation operation);
void            shell_coverdownloader_amazon_xml_loaded_cb      (GnomeVFSResult result,
                                                                 GnomeVFSFileSize file_size,
                                                                 char *file_contents,
                                                                 gpointer callback_data);
void            shell_coverdownloader_amazon_cover_loaded_cb    (GnomeVFSResult result,
                                                                 GnomeVFSFileSize file_size,
                                                                 char *file_contents,
                                                                 gpointer callback_data);

G_END_DECLS

#endif /* __SHELL_COVERDOWNLOADER_H */
