/* 
 * Gamecube/Wii VIDEO/GX subsystem wrapper
 *
 * Copyright (C) 2008, 2009		Andre Heider "dhewg" <dhewg@wiibrew.org>
 *
 * This code is licensed to you under the terms of the GNU GPL, version 2;
 * see file COPYING or http://www.gnu.org/licenses/old-licenses/gpl-2.0.txt
 *
 */

#ifndef GFX_TEX_H
#define GFX_TEX_H

#include "gfx_types.h"

#ifdef __cplusplus
extern "C" {
#endif

void gfx_coords(gfx_screen_coords_t *coords, gfx_tex_t *tex, gfx_coord_t type);

bool gfx_tex_init(gfx_tex_t *tex, gfx_tex_format_t format, u32 tlut_name,
					u16 width, u16 height);
void gfx_tex_deinit(gfx_tex_t *tex);

bool gfx_tex_set_bilinear_filter(gfx_tex_t *tex, bool enable);
bool gfx_tex_flush_texture(gfx_tex_t *tex);
bool gfx_tex_flush_palette(gfx_tex_t *tex);
bool gfx_tex_clear_palette(gfx_tex_t *tex);

bool gfx_tex_convert(gfx_tex_t *tex, const void *src);

bool gfx_tiles_init(gfx_tiles_t *tiles, gfx_tex_t *tex, u8 cols, u8 rows);
void gfx_tiles_deinit(gfx_tiles_t *tiles);

#ifdef __cplusplus
}
#endif

#endif
