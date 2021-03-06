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

#ifndef __ARIO_DEBUG_H
#define __ARIO_DEBUG_H

#include <config.h>
#include <stdio.h>

/* Macro used to log an error */
#define ARIO_LOG_ERROR(x,args...) {printf("[ERROR](%s:%d) %s : " x "\n", __FILE__, __LINE__, __FUNCTION__, ##args);}

/* Macro used to log an information */
#define ARIO_LOG_INFO(x,args...) {printf("[info]" x "\n", ##args);}

#ifdef DEBUG
/* Macro used to log a debug information */
#define ARIO_LOG_DBG(x,args...) {printf("[debug](%s:%d) %s : " x "\n", __FILE__, __LINE__, __FUNCTION__, ##args);}
/* Macro used to log the start of a function */
#define ARIO_LOG_FUNCTION_START      ARIO_LOG_DBG("Function start")
#else
/* If DEBUG is not activated we don't log debug info */
#define ARIO_LOG_DBG(x,args...) {}
#define ARIO_LOG_FUNCTION_START
#endif

#endif /* __ARIO_DEBUG_H */
