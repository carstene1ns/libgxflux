/* 
 * Gamecube/Wii VIDEO/GX subsystem wrapper
 *
 * Copyright (C) 2008, 2009		Andre Heider "dhewg" <dhewg@wiibrew.org>
 *
 * This code is licensed to you under the terms of the GNU GPL, version 2;
 * see file COPYING or http://www.gnu.org/licenses/old-licenses/gpl-2.0.txt
 *
 */

#ifndef __GFX_COLOR_H__
#define __GFX_COLOR_H__

#include "gfx_types.h"

#ifdef __cplusplus
extern "C" {
#endif

extern const GXColor gfx_color_none;
extern const GXColor gfx_color_black;
extern const GXColor gfx_color_red;
extern const GXColor gfx_color_green;
extern const GXColor gfx_color_blue;
extern const GXColor gfx_color_white;

static inline bool gfx_color_cmp(const GXColor a, const GXColor b) {
	return (a.r == b.r) && (a.g == b.g) && (a.b == b.b) && (a.a == b.a);
}

static inline u32 gfx_color_u32(const GXColor c) {
	return (c.r << 24) | (c.g << 16) | (c.b << 8) | c.a;
}

#ifdef __cplusplus
}
#endif

#endif

