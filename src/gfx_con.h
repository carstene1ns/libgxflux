/* 
 * Gamecube/Wii VIDEO/GX subsystem wrapper
 *
 * Copyright (C) 2008, 2009		Andre Heider "dhewg" <dhewg@wiibrew.org>
 *
 * This code is licensed to you under the terms of the GNU GPL, version 2;
 * see file COPYING or http://www.gnu.org/licenses/old-licenses/gpl-2.0.txt
 *
 */

#ifndef __GFX_CON_H__
#define __GFX_CON_H__

#include "gfx_types.h"
#include "gfx_con_esc.h"

#ifdef __cplusplus
extern "C" {
#endif

bool gfx_con_init(gfx_screen_coords_t *coords);
void gfx_con_deinit(void);
void gfx_con_draw(void);

#ifdef __cplusplus
}
#endif

#endif

