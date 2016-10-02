/* 
 * Gamecube/Wii VIDEO/GX subsystem wrapper
 *
 * Copyright (C) 2008, 2009		Andre Heider "dhewg" <dhewg@wiibrew.org>
 *
 * This code is licensed to you under the terms of the GNU GPL, version 2;
 * see file COPYING or http://www.gnu.org/licenses/old-licenses/gpl-2.0.txt
 *
 */

#include <malloc.h>
#include <string.h>
#include <math.h>

#include "gfx.h"

void gfx_coords(gfx_screen_coords_t *coords, gfx_tex_t *tex, gfx_coord_t type) {
	f32 ar_efb, ar_tex;
	u16 sw, sh;

	if (!coords || !tex)
		return;

	sw = gfx_video_get_width();
	sh = gfx_video_get_height();

	switch(type) {
	case GFX_COORD_FULLSCREEN:
		coords->x = 0.0;
		coords->y = 0.0;
		coords->w = sw;
		coords->h = sh;
		break;

	case GFX_COORD_FULLSCREEN_AR:
		ar_efb = (f32) sw / (f32) sh;
		ar_tex = (f32) tex->width / (f32) tex->height;

		if (ar_tex > ar_efb) {
			coords->w = sw;
			coords->h = (u16) round((f32) sh * ar_efb / ar_tex);
			coords->x = 0.0;
			coords->y = (sh - coords->h) / 2;
		} else {
			coords->w = (u16) round((f32) sw * ar_tex / ar_efb);
			coords->h = sh;
			coords->x = (sw - coords->w) / 2;
			coords->y = 0.0;
		}
		break;

	case GFX_COORD_CENTER:
		coords->x = (sw - tex->width) / 2;
		coords->y = (sh - tex->height) / 2;
		coords->w = tex->width;
		coords->h = tex->height;
		break;

	default:
		break;
	}
}

bool gfx_tex_init(gfx_tex_t *tex, gfx_tex_format_t format, u32 tlut_name,
					u16 width, u16 height) {
	u8 bpp;
	u8 fmt_tex;
	bool tlut;
	u8 fmt_tlut;
	u32 ax, ay;
	u32 memsize;

	if (!tex)
		return false;

	switch(format) {
	case GFX_TF_RGB565:
		bpp = 16;
		fmt_tex = GX_TF_RGB565;
		tlut = false;
		fmt_tlut = 0;
		ax = 3;
		ay = 3;
		break;

	case GFX_TF_RGB5A3:
		bpp = 16;
		fmt_tex = GX_TF_RGB5A3;
		tlut = false;
		fmt_tlut = 0;
		ax = 3;
		ay = 3;
		break;

	case GFX_TF_I4:
		bpp = 4;
		fmt_tex = GX_TF_I4;
		tlut = false;
		fmt_tlut = 0;
		ax = 7;
		ay = 7;
		break;

	case GFX_TF_PALETTE_RGB565:
		bpp = 8;
		fmt_tex = GX_TF_CI8;
		tlut = true;
		fmt_tlut = GX_TL_RGB565;
		ax = 7;
		ay = 3;
		break;

	case GFX_TF_PALETTE_RGB5A3:
		bpp = 8;
		fmt_tex = GX_TF_CI8;
		tlut = true;
		fmt_tlut = GX_TL_RGB5A3;
		ax = 7;
		ay = 3;
		break;

	default:
		gfx_tex_deinit(tex);
		return false;
	}

	if ((width & ax) || (height & ay)) {
		gfx_tex_deinit(tex);
		return false;
	}

	if (tlut) {
		if (!tex->palette) {
			tex->palette = (u16 *) memalign(32, 256 * 2);
			if (!tex->palette) {
				gfx_tex_deinit(tex);
				return false;
			}

			memset(tex->palette, 0, 256 * 2);
			DCFlushRange(tex->palette, 256 * 2);
		}

		tex->tlut_name = tlut_name;
		GX_InitTlutObj(&tex->tlut, tex->palette, fmt_tlut, 256);
	} else {
		tex->tlut_name = 0;
		free(tex->palette);
		tex->palette = NULL;
	}

	if (!tex->pixels || (width != tex->width) || (height != tex->height ||
			bpp != tex->bpp)) {
		free(tex->pixels);

		memsize = (width * height * bpp) >> 3;
		tex->pixels = memalign(32, memsize);

		if (!tex->pixels) {
			gfx_tex_deinit(tex);
			return false;
		}
	
		memset(tex->pixels, 0, memsize);
		DCFlushRange(tex->pixels, memsize);
	}

	tex->format = format;
	tex->width = width;
	tex->height = height;
	tex->bpp = bpp;

	if (tlut) {
		GX_LoadTlut(&tex->tlut, tlut_name);
		GX_InitTexObjCI(&tex->obj, tex->pixels, width, height, fmt_tex,
						GX_CLAMP, GX_CLAMP, GX_FALSE, tlut_name);
	} else {
		GX_InitTexObj(&tex->obj, tex->pixels, width, height, fmt_tex,
						GX_CLAMP, GX_CLAMP, GX_FALSE);
	}

	return true;
}

void gfx_tex_deinit(gfx_tex_t *tex) {
	if (!tex)
		return;

	free(tex->pixels);
	free(tex->palette);
	memset(tex, 0, sizeof(gfx_tex_t));
}

bool gfx_tex_set_bilinear_filter(gfx_tex_t *tex, bool enable) {
	if (!tex)
		return false;

	if (enable) {
		if (tex->palette)
			GX_InitTexObjFilterMode(&tex->obj, GX_LIN_MIP_NEAR, GX_LINEAR);
		else
			GX_InitTexObjFilterMode(&tex->obj, GX_LIN_MIP_LIN, GX_LINEAR);
	} else {
		GX_InitTexObjFilterMode(&tex->obj, GX_NEAR, GX_NEAR);
	}

	return true;
}

bool gfx_tex_flush_texture(gfx_tex_t *tex) {
	if (!tex)
		return false;

	DCFlushRange(tex->pixels, (tex->width * tex->height * tex->bpp) >> 3);
	return true;
}

bool gfx_tex_clear_palette(gfx_tex_t *tex) {
	if (!tex || !tex->palette)
		return false;

	memset(tex->palette, 0, 256 * 2);
	DCFlushRange(tex->palette, 256 * 2);
	GX_LoadTlut(&tex->tlut, tex->tlut_name);

	return true;
}

bool gfx_tex_flush_palette(gfx_tex_t *tex) {
	if (!tex || !tex->palette)
		return false;

	DCFlushRange(tex->palette, 256 * 2);
	GX_LoadTlut(&tex->tlut, tex->tlut_name);

	return true;
}

bool gfx_tex_convert(gfx_tex_t *tex, const void *src) {
	bool ret;
	u16 w, h, x, y;
	u64 *dst, *src1, *src2, *src3, *src4;
	u16 rowpitch;
	u16 pitch;
	const u8 *s = (const u8 *) src;

	if (!tex)
		return false;

	ret = false;
	w = tex->width;
	h = tex->height;

	switch(tex->format) {
	case GFX_TF_RGB565:
	case GFX_TF_RGB5A3:
		pitch = w * 2;
		dst = (u64 *) tex->pixels;
		src1 = (u64 *) s;
		src2 = (u64 *) (s + pitch);
		src3 = (u64 *) (s + (pitch * 2));
		src4 = (u64 *) (s + (pitch * 3));
		rowpitch = (pitch >> 3) * 3 + pitch % 8;

		for (y = 0; y < h; y += 4) {
			for (x = 0; x < (w >> 2); x++) {
				*dst++ = *src1++;
				*dst++ = *src2++;
				*dst++ = *src3++;
				*dst++ = *src4++;
			}

			src1 += rowpitch;
			src2 += rowpitch;
			src3 += rowpitch;
			src4 += rowpitch;
		}

		ret = true;
		break;

	case GFX_TF_PALETTE_RGB565:
	case GFX_TF_PALETTE_RGB5A3:
		pitch = w;
		dst = (u64 *) tex->pixels;
		src1 = (u64 *) s;
		src2 = (u64 *) (s + pitch);
		src3 = (u64 *) (s + (pitch * 2));
		src4 = (u64 *) (s + (pitch * 3));
		rowpitch = (pitch >> 3) * 3 + pitch % 8;

		for (y = 0; y < h; y += 4) {
			for (x = 0; x < (w >> 3); x++) {
				*dst++ = *src1++;
				*dst++ = *src2++;
				*dst++ = *src3++;
				*dst++ = *src4++;
			}

			src1 += rowpitch;
			src2 += rowpitch;
			src3 += rowpitch;
			src4 += rowpitch;
		}

		ret = true;
		break;

	default:
		break;
	}

	if (ret)
		DCFlushRange(tex->pixels, (w * h * tex->bpp) >> 3);

	return ret;
}

bool gfx_tiles_init(gfx_tiles_t *tiles, gfx_tex_t *tex, u8 cols, u8 rows) {
	f32 tw, th;
	u8 x, y;
	gfx_tex_coord_t *tile;

	if (!tiles || !tex)
		return false;

	if ((tex->width % cols) || (tex->height % rows))
		return false;

	memset(tiles, 0, sizeof(gfx_tiles_t));
	tiles->tiles = (gfx_tex_coord_t *) malloc(rows * cols *
												sizeof(gfx_tex_coord_t));
	if (!tiles->tiles)
		return false;

	tiles->tex = tex;
	tiles->cols = cols;
	tiles->rows = rows;
	tiles->width = tex->width / cols;
	tiles->height = tex->height / rows;

	tw = 1.0 / cols;
	th = 1.0 / rows;

	tile = tiles->tiles;
	for (y = 0; y < rows; ++y) {
		for (x = 0; x < cols; ++x) {
			tile->s1 = tw * x;
			tile->t1 = th * y;
			tile->s2 = tile->s1 + tw;
			tile->t2 = tile->t1 + th;

			tile++;
		}
	}

	return true;
}

void gfx_tiles_deinit(gfx_tiles_t *tiles) {
	if (!tiles)
		return;

	free(tiles->tiles);
	memset(tiles, 0, sizeof(gfx_tiles_t));
}
