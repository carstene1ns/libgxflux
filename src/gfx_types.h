/* 
 * Gamecube/Wii VIDEO/GX subsystem wrapper
 *
 * Copyright (C) 2008, 2009		Andre Heider "dhewg" <dhewg@wiibrew.org>
 *
 * This code is licensed to you under the terms of the GNU GPL, version 2;
 * see file COPYING or http://www.gnu.org/licenses/old-licenses/gpl-2.0.txt
 *
 */

#ifndef __GFX_TYPES_H__
#define __GFX_TYPES_H__

#include <gccore.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
	GFX_STANDARD_AUTO = -1,
	GFX_STANDARD_PROGRESSIVE = 0,
	GFX_STANDARD_NTSC,
	GFX_STANDARD_PAL,
	GFX_STANDARD_EURGB60,
	GFX_STANDARD_MPAL
} gfx_video_standard_t;

typedef enum {
	GFX_MODE_DEFAULT = 0,
	GFX_MODE_DS,
} gfx_video_mode_t;

typedef enum {
	GFX_TF_RGB565 = 0,
	GFX_TF_RGB5A3,
	GFX_TF_I4,
	GFX_TF_PALETTE_RGB565,
	GFX_TF_PALETTE_RGB5A3
} gfx_tex_format_t;

typedef struct {
	void *pixels;
	u16 *palette;

	gfx_tex_format_t format;
	u16 width;
	u16 height;
	u8 bpp;
	GXTexObj obj;
	GXTlutObj tlut;
	u32 tlut_name;
} gfx_tex_t;

typedef enum {
	GFX_COORD_FULLSCREEN = 0,
	GFX_COORD_FULLSCREEN_AR,
	GFX_COORD_CENTER
} gfx_coord_t;

typedef struct {
	f32 x, y;
	f32 w, h;
} gfx_screen_coords_t;

typedef struct {
	f32 s1, t1;
	f32 s2, t2;
} gfx_tex_coord_t;

typedef struct {
	gfx_tex_t *tex;
	u8 cols;
	u8 rows;
	u16 width;
	u16 height;
	gfx_tex_coord_t *tiles;
} gfx_tiles_t;

typedef enum {
	COLOROP_NONE = 0,
	COLOROP_SIMPLEFADE,
	COLOROP_MODULATE_FGBG
} gfx_colorop_t;

#ifdef __cplusplus
}
#endif

#endif

