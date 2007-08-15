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

#ifndef __DEBUG_H
#define __DEBUG_H

//#define DEBUG

#ifdef DEBUG
#define LOG_DBG(x,args...) {printf("[debug](%s:%d) %s : ", __FILE__, __LINE__, __FUNCTION__); printf(x, ##args);printf("\n");}
#define LOG_FUNCTION_START      LOG_DBG("Function start")
#define LOG_FUNCTION_END        LOG_DBG("Function end")
#else
#define LOG_DBG(x,args...)    
#define LOG_FUNCTION_START       
#define LOG_FUNCTION_END         
#endif

#endif /* __DEBUG_H */
