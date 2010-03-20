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


#ifndef __ARIO_LYRICS_LYRC_H
#define __ARIO_LYRICS_LYRC_H

#include "lyrics/ario-lyrics-provider.h"

G_BEGIN_DECLS

#define TYPE_ARIO_LYRICS_LYRC         (ario_lyrics_lyrc_get_type ())
#define ARIO_LYRICS_LYRC(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), TYPE_ARIO_LYRICS_LYRC, ArioLyricsLyrc))
#define ARIO_LYRICS_LYRC_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST((k), TYPE_ARIO_LYRICS_LYRC, ArioLyricsLyrcClass))
#define IS_ARIO_LYRICS_LYRC(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), TYPE_ARIO_LYRICS_LYRC))
#define IS_ARIO_LYRICS_LYRC_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), TYPE_ARIO_LYRICS_LYRC))
#define ARIO_LYRICS_LYRC_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), TYPE_ARIO_LYRICS_LYRC, ArioLyricsLyrcClass))

typedef struct
{
        ArioLyricsProvider parent;
} ArioLyricsLyrc;

typedef struct
{
        ArioLyricsProviderClass parent;
} ArioLyricsLyrcClass;

GType                   ario_lyrics_lyrc_get_type      (void) G_GNUC_CONST;

ArioLyricsProvider*     ario_lyrics_lyrc_new           (void);

G_END_DECLS

#endif /* __ARIO_LYRICS_LYRC_H */
