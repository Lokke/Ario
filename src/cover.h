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


#ifndef __COVER_H
#define __COVER_H

#include <libgnomevfs/gnome-vfs.h>

#define COVER_SIZE 70

G_BEGIN_DECLS

typedef enum
{
        GET_FIRST_COVER,
        GET_ALL_COVERS
}CoverAmazonOperation;

typedef enum
{
        AMAZON_SMALL_COVER,
        AMAZON_MEDIUM_COVER,
        AMAZON_LARGE_COVER
}CoverAmazonCoversSize;

typedef enum
{
        SMALL_COVER,
        NORMAL_COVER
}CoverHomeCoversSize;

typedef enum
{
        OVERWRITE_MODE_ASK,
        OVERWRITE_MODE_REPLACE,
        OVERWRITE_MODE_SKIP
}CoverOverwriteMode;

int                          cover_load_amazon_covers   (const char *artist,
                                                         const char *album,
                                                         GList **cover_uris,
                                                         GArray **file_size,
                                                         GList **file_contents,
                                                         CoverAmazonOperation operation,
                                                         CoverAmazonCoversSize cover_size);
GnomeVFSResult               cover_save_cover           (const gchar *artist,
                                                         const gchar *album,
                                                         char *data,
                                                         int size,
                                                         CoverOverwriteMode overwrite_mode);
void                         cover_remove_cover         (const gchar *artist,
                                                         const gchar *album);
gboolean                     cover_size_is_valid        (int size);

gboolean                     cover_cover_exists         (const gchar *artist,
                                                         const gchar *album);
gchar*                       cover_make_cover_path      (const gchar *artist,
                                                         const gchar *album,
                                                         CoverHomeCoversSize cover_size);

G_END_DECLS

#endif /* __COVER_H */
