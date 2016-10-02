/* 
 * Gamecube/Wii VIDEO/GX subsystem wrapper
 *
 * Copyright (C) 2008, 2009		Andre Heider "dhewg" <dhewg@wiibrew.org>
 *
 * This code is licensed to you under the terms of the GNU GPL, version 2;
 * see file COPYING or http://www.gnu.org/licenses/old-licenses/gpl-2.0.txt
 *
 */

#ifndef GFX_H
#define GFX_H

#include "gfx_types.h"
#include "gfx_color.h"
#include "gfx_tex.h"

#ifdef __cplusplus
extern "C" {
#endif

gfx_video_standard_t gfx_video_get_standard(void);
void gfx_video_get_modeobj(GXRModeObj *obj, gfx_video_standard_t standard,
							gfx_video_mode_t mode);

void gfx_video_set_overscan(u8 overscan);

void gfx_video_init(GXRModeObj *obj);
void gfx_video_deinit(void);

u16 gfx_video_get_width(void);
u16 gfx_video_get_height(void);

void gfx_init(void);
void gfx_deinit(void);

bool gfx_enable_viewport(bool enable);
void gfx_set_underscan(u16 underscan_x, u16 underscan_y);
bool gfx_set_pillarboxing(bool enable);
f32 gfx_set_ar(f32 ar);

bool gfx_frame_start(void);
void gfx_frame_end(void);
void gfx_frame_abort(void);

void gfx_set_colorop(gfx_colorop_t op, const GXColor c1, const GXColor c2);

void gfx_draw_tex(gfx_tex_t *tex, gfx_screen_coords_t *coords);
void gfx_draw_tile(gfx_tiles_t *tiles, gfx_screen_coords_t *coords,
					u8 col, u8 row);
void gfx_draw_tile_by_index(gfx_tiles_t *tiles, gfx_screen_coords_t *coords,
							u16 index);

#ifdef __cplusplus
}
#endif

#endif
