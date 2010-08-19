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

#ifdef __cplusplus
extern "C" {
#endif

#define FIFO_SIZE (512 * 1024)
#define ORIGIN_Z (-500.0)

typedef enum {
	BUSY_READY = 0,
	BUSY_WAITDRAWDONE,
	BUSY_WAITVSYNC
} _busy_t;

typedef struct {
	gfx_colorop_t op;
	GXColor c1;
	GXColor c2;
} _colorop_t;

static struct {
	GXRModeObj vm;
	u8 overscan;
	bool doublestrike;

	u32 *fb[2];
	u32 fb_size;
	u8 fb_active;

	bool viewport_enabled;
	u16 underscan_x;
	u16 underscan_y;
	f32 ar;
	bool pillarboxing;

	u8 *fifo;
	Mtx view;
	_busy_t busy;

	GXTexObj *tex_loaded;
	_colorop_t colorop;
} _gfx;

// Standard, DS
static GXRModeObj *mode_table[5][2] = {
	{ &TVNtsc480Prog, &TVNtsc240Ds },
	{ &TVNtsc480IntDf, &TVNtsc240Ds },
	{ &TVPal574IntDfScale, &TVPal264Ds },
	{ &TVEurgb60Hz480IntDf, &TVEurgb60Hz240Ds },
	{ &TVMpal480IntDf, &TVMpal240Ds }
};

static void _retrace_cb(u32 count) {
	(void) count;

	if (_gfx.busy == BUSY_WAITVSYNC) {
		VIDEO_SetNextFramebuffer(_gfx.fb[_gfx.fb_active]);
		VIDEO_Flush();

		_gfx.fb_active ^= 1;
		_gfx.busy = BUSY_READY;
	}
}

static void _drawdone_cb(void) {
	if (_gfx.busy == BUSY_WAITDRAWDONE)
		_gfx.busy = BUSY_WAITVSYNC;
}

static void _update_viewport(void) {
	f32 ar;
	u16 correction;
	u16 usy;
	u16 x1, y1, x2, y2;

	if (_gfx.viewport_enabled) {
		usy = _gfx.underscan_y;

		if (!_gfx.doublestrike)
			usy *= 2;

		x1 = _gfx.underscan_x * 2;
		y1 = usy;
		x2 = _gfx.vm.fbWidth - _gfx.underscan_x * 4;
		y2 = _gfx.vm.efbHeight - usy * 2;

		if (_gfx.pillarboxing)
			ar = 16.0 / 9.0;
		else
			ar = 4.0 / 3.0;

		if (fabs(ar - _gfx.ar) > 0.05) {
			if (ar > _gfx.ar) {
				correction = _gfx.vm.viWidth - 
							(u16) round((f32) _gfx.vm.viWidth * _gfx.ar / ar);

				x1 += correction / 2;
				x2 -= correction;
			} else {
				correction = _gfx.vm.efbHeight - 
							(u16) round((f32) _gfx.vm.efbHeight * ar / _gfx.ar);

				if (_gfx.doublestrike)
					correction /= 2;

				y1 += correction / 2;
				y2 -= correction;
			}
		}
	} else {
		x1 = 0;
		y1 = 0;
		x2 = _gfx.vm.fbWidth;
		y2 = _gfx.vm.efbHeight;
	}

	GX_SetViewport(x1, y1, x2, y2, 0, 1);
	GX_SetScissor(x1, y1, x2, y2);
}

gfx_video_standard_t gfx_video_get_standard(void) {
	gfx_video_standard_t standard;

#ifdef HW_RVL
	if ((CONF_GetProgressiveScan() > 0) && VIDEO_HaveComponentCable()) {
		standard = GFX_STANDARD_PROGRESSIVE;
	} else {
		switch (CONF_GetVideo()) {
		case CONF_VIDEO_PAL:
			if (CONF_GetEuRGB60() > 0)
				standard = GFX_STANDARD_EURGB60;
			else
				standard = GFX_STANDARD_PAL;
			break;

		case CONF_VIDEO_MPAL:
			standard = GFX_STANDARD_MPAL;
			break;

		default:
			standard = GFX_STANDARD_NTSC;
			break;
		}
	}
#else
	switch (VIDEO_GetCurrentTvMode()) {
	case VI_PAL:
		standard = GFX_STANDARD_PAL;
		break;
	case VI_MPAL:
		standard = GFX_STANDARD_MPAL;
		break;
	default:
		standard = GFX_STANDARD_NTSC;
		break;
	}
#endif

	return standard;
}

void gfx_video_get_modeobj(GXRModeObj *obj, gfx_video_standard_t standard,
							gfx_video_mode_t mode) {
	if (standard == GFX_STANDARD_AUTO)
		standard = gfx_video_get_standard();

	memcpy(obj, mode_table[standard][mode], sizeof(GXRModeObj));
}

void gfx_video_set_overscan(u8 overscan) {
	if (overscan > 80)
		overscan = 80;

	_gfx.overscan = overscan;
}

void gfx_video_init(GXRModeObj *obj) {
	static bool inited = false;
	u32 fb_size;
	u8 i;

	if (!inited) {
		memset(&_gfx, 0, sizeof(_gfx));

		_gfx.overscan = 32;
		_gfx.viewport_enabled = true;
		_gfx.ar = 4.0 / 3.0;
	}

	if (obj)
		memcpy(&_gfx.vm, obj, sizeof(GXRModeObj));
	else
		gfx_video_get_modeobj(&_gfx.vm, GFX_STANDARD_AUTO, GFX_MODE_DEFAULT);

	_gfx.vm.viWidth = 640 + _gfx.overscan;
	_gfx.vm.viXOrigin = (VI_MAX_WIDTH_NTSC - _gfx.vm.viWidth) / 2;

	_gfx.doublestrike = _gfx.vm.viHeight == 2 * _gfx.vm.xfbHeight;

	VIDEO_SetNextFramebuffer(NULL);

	if (inited)
		VIDEO_WaitVSync();

	inited = true;

	VIDEO_Configure(&_gfx.vm);
	VIDEO_Flush();

	fb_size = VIDEO_GetFrameBufferSize(&_gfx.vm);

	if (_gfx.fb_size != fb_size) {
		if (_gfx.fb[0])
			free(MEM_K1_TO_K0(_gfx.fb[0]));
		if (_gfx.fb[1])
			free(MEM_K1_TO_K0(_gfx.fb[1]));

		_gfx.fb[0] = (u32 *) MEM_K0_TO_K1(SYS_AllocateFramebuffer(&_gfx.vm));
		_gfx.fb[1] = (u32 *) MEM_K0_TO_K1(SYS_AllocateFramebuffer(&_gfx.vm));

		_gfx.fb_size = fb_size;
	}

	VIDEO_ClearFrameBuffer(&_gfx.vm, _gfx.fb[0], COLOR_BLACK);
	VIDEO_ClearFrameBuffer(&_gfx.vm, _gfx.fb[1], COLOR_BLACK);

	VIDEO_SetNextFramebuffer(_gfx.fb[_gfx.fb_active]);
	VIDEO_SetBlack(FALSE);
	VIDEO_Flush();

	for (i = 0; i < 4; ++i)
		VIDEO_WaitVSync();
}

void gfx_video_deinit(void) {
	u8 i;

	VIDEO_WaitVSync();
	VIDEO_SetBlack(TRUE);
	VIDEO_SetNextFramebuffer(NULL);
	VIDEO_Flush();

	for (i = 0; i < 4; ++i)
		VIDEO_WaitVSync();

	if (_gfx.fb[0]) {
		free(MEM_K1_TO_K0(_gfx.fb[0]));
		_gfx.fb[0] = NULL;
	}

	if (_gfx.fb[1]) {
		free(MEM_K1_TO_K0(_gfx.fb[1]));
		_gfx.fb[1] = NULL;
	}
}

u16 gfx_video_get_width(void) {
	return _gfx.vm.fbWidth;
}

u16 gfx_video_get_height(void) {
	return _gfx.vm.efbHeight;
}

void gfx_init(void) {
	Mtx44 p;
	f32 yscale;
	u32 xfbHeight;

	GX_AbortFrame();

	if (!_gfx.fifo)
		_gfx.fifo = (u8 *) memalign(32, FIFO_SIZE);

	memset(_gfx.fifo, 0, FIFO_SIZE);
	GX_Init(_gfx.fifo, FIFO_SIZE);

	GX_SetCopyClear(gfx_color_black, 0x00ffffff);

	yscale = GX_GetYScaleFactor(_gfx.vm.efbHeight, _gfx.vm.xfbHeight);
	xfbHeight = GX_SetDispCopyYScale(yscale);
	GX_SetDispCopySrc(0, 0, _gfx.vm.fbWidth, _gfx.vm.efbHeight);
	GX_SetDispCopyDst(_gfx.vm.fbWidth, xfbHeight);
	GX_SetCopyFilter(_gfx.vm.aa, _gfx.vm.sample_pattern, GX_TRUE,
					_gfx.vm.vfilter);

	if (_gfx.doublestrike)
		GX_SetFieldMode(_gfx.vm.field_rendering, GX_ENABLE);
	else
		GX_SetFieldMode(_gfx.vm.field_rendering, GX_DISABLE);

	if (_gfx.vm.aa)
		GX_SetPixelFmt(GX_PF_RGB565_Z16, GX_ZC_LINEAR);
	else
		GX_SetPixelFmt(GX_PF_RGB8_Z24, GX_ZC_LINEAR);

	GX_SetCullMode(GX_CULL_NONE);
	GX_SetDispCopyGamma(GX_GM_1_0);

	GX_ClearVtxDesc();
	GX_SetVtxDesc(GX_VA_POS, GX_DIRECT);
	GX_SetVtxDesc(GX_VA_CLR0, GX_NONE);
	GX_SetVtxDesc(GX_VA_TEX0, GX_DIRECT);

	GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_POS, GX_POS_XY, GX_F32, 0);
	GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_CLR0, GX_CLR_RGBA, GX_RGBA8, 0);
	GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_TEX0, GX_TEX_ST, GX_F32, 0);

	GX_InvVtxCache();
	GX_InvalidateTexAll();

	memset(&_gfx.view, 0, sizeof(Mtx));
	guMtxIdentity(_gfx.view);

	guOrtho(p, 0, _gfx.vm.efbHeight, 0, _gfx.vm.fbWidth, 100, 1000);
	GX_LoadProjectionMtx (p, GX_ORTHOGRAPHIC);
	_update_viewport();

	GX_Flush();

	_gfx.busy = BUSY_READY;
	VIDEO_SetPreRetraceCallback(_retrace_cb);
	GX_SetDrawDoneCallback(_drawdone_cb);
}

void gfx_deinit(void) {
	VIDEO_SetPreRetraceCallback(NULL);
	GX_SetDrawDoneCallback(NULL);
	GX_AbortFrame();

	free(_gfx.fifo);
	_gfx.fifo = NULL;
}

bool gfx_enable_viewport(bool enable) {
	bool old = _gfx.viewport_enabled;

	_gfx.viewport_enabled = enable;
	_update_viewport();

	return old;
}

void gfx_set_underscan(u16 underscan_x, u16 underscan_y) {
	_gfx.underscan_x = underscan_x;
	_gfx.underscan_y = underscan_y;

	if (_gfx.underscan_x > 32)
		_gfx.underscan_x = 32;

	if (_gfx.underscan_y > 32)
		_gfx.underscan_y = 32;

	_update_viewport();
}

bool gfx_set_pillarboxing(bool enable) {
	bool old = _gfx.pillarboxing;
	_gfx.pillarboxing = enable;

	_update_viewport();

	return old;
}

f32 gfx_set_ar(f32 ar) {
	f32 old = _gfx.ar;

	_gfx.ar = ar;

	if (ar < 16.0 / 480.0)
		ar = 16.0 / 480.0;

	if (ar > 640.0 / 16.0)
		ar = 640.0 / 16.0;

	_update_viewport();

	return old;
}

bool gfx_frame_start(void) {
	if (_gfx.busy != BUSY_READY)
		return false;

	GX_InvVtxCache();
	GX_InvalidateTexAll();
	_gfx.tex_loaded = NULL;

	gfx_set_colorop(COLOROP_NONE, gfx_color_none, gfx_color_none);

	GX_SetBlendMode(GX_BM_BLEND, GX_BL_SRCALPHA, GX_BL_INVSRCALPHA, GX_LO_NOOP);

	return true;
}

void gfx_frame_end(void) {
	GX_SetZMode(GX_TRUE, GX_LEQUAL, GX_TRUE);
	GX_SetColorUpdate(GX_TRUE);

	GX_CopyDisp(_gfx.fb[_gfx.fb_active], GX_TRUE);

	_gfx.busy = BUSY_WAITDRAWDONE;
	GX_SetDrawDone();
}

void gfx_frame_abort(void) {
	GX_AbortFrame();
	_gfx.busy = BUSY_READY;
}

// TEV formula for OP ADD and SUB: (a * (1.0 - c) + b * c) OP d
void gfx_set_colorop(gfx_colorop_t op, const GXColor c1, const GXColor c2) {
	bool skip = false;

	if (_gfx.colorop.op == op) {
		switch (op) {
		case COLOROP_MODULATE_FGBG:
			if (!gfx_color_cmp(_gfx.colorop.c1, c1))
				GX_SetTevKColor(GX_KCOLOR0, c1);
			if (!gfx_color_cmp(_gfx.colorop.c2, c2))
				GX_SetTevKColor(GX_KCOLOR1, c2);
			break;

		default:
			break;
		}

		skip = true;
	}

	_gfx.colorop.op = op;
	_gfx.colorop.c1 = c1;
	_gfx.colorop.c2 = c2;

	if (skip)
		return;

	GX_SetNumChans(0);
	GX_SetNumTexGens(1);
	GX_SetTexCoordGen(GX_TEXCOORD0, GX_TG_MTX2x4, GX_TG_TEX0, GX_IDENTITY);

	switch (op) {
	case COLOROP_SIMPLEFADE:
		GX_SetNumTevStages(1);
		GX_SetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD0, GX_TEXMAP0, GX_COLORZERO);

		GX_SetTevColorIn(GX_TEVSTAGE0, GX_CC_ZERO, GX_CC_ZERO,
							GX_CC_ZERO, GX_CC_TEXC);
		GX_SetTevColorOp(GX_TEVSTAGE0, GX_TEV_ADD, GX_TB_ZERO,
							GX_CS_DIVIDE_2, GX_TRUE, GX_TEVPREV);
		GX_SetTevAlphaIn(GX_TEVSTAGE0, GX_CA_ZERO, GX_CA_ZERO,
							GX_CA_ZERO, GX_CA_TEXA);
		GX_SetTevAlphaOp(GX_TEVSTAGE0, GX_TEV_ADD, GX_TB_ZERO,
							GX_CS_SCALE_1, GX_TRUE, GX_TEVPREV);
		break;

	case COLOROP_MODULATE_FGBG:
		GX_SetNumTevStages(2);
		GX_SetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD0, GX_TEXMAP0, GX_COLORZERO);

		GX_SetTevKColor(GX_KCOLOR0, c1);
		GX_SetTevKColor(GX_KCOLOR1, c2);
		GX_SetTevKColorSel(GX_TEVSTAGE0, GX_TEV_KCSEL_K0);
		GX_SetTevKAlphaSel(GX_TEVSTAGE0, GX_TEV_KCSEL_K0_A);

		GX_SetTevColorIn(GX_TEVSTAGE0, GX_CC_ZERO, GX_CC_ZERO,
							GX_CC_ZERO, GX_CC_KONST);
		GX_SetTevColorOp(GX_TEVSTAGE0, GX_TEV_ADD, GX_TB_ZERO,
							GX_CS_SCALE_1, GX_TRUE, GX_TEVREG0);

		GX_SetTevAlphaIn(GX_TEVSTAGE0, GX_CA_ZERO, GX_CA_ZERO,
							GX_CA_ZERO, GX_CA_KONST);
		GX_SetTevAlphaOp(GX_TEVSTAGE0, GX_TEV_ADD, GX_TB_ZERO,
							GX_CS_SCALE_1, GX_TRUE, GX_TEVREG0);

		GX_SetTevOrder(GX_TEVSTAGE1, GX_TEXCOORD0, GX_TEXMAP0, GX_COLORZERO);

		GX_SetTevKColorSel(GX_TEVSTAGE1, GX_TEV_KCSEL_K1);
		GX_SetTevKAlphaSel(GX_TEVSTAGE1, GX_TEV_KCSEL_K1_A);

		GX_SetTevColorIn(GX_TEVSTAGE1, GX_CC_KONST, GX_CC_C0,
							GX_CC_TEXC, GX_CC_ZERO);
		GX_SetTevColorOp(GX_TEVSTAGE1, GX_TEV_ADD, GX_TB_ZERO,
							GX_CS_SCALE_1, GX_TRUE, GX_TEVPREV);

		GX_SetTevAlphaIn(GX_TEVSTAGE1, GX_CA_KONST, GX_CA_A0,
							GX_CA_TEXA, GX_CA_ZERO);
		GX_SetTevAlphaOp(GX_TEVSTAGE1, GX_TEV_ADD, GX_TB_ZERO,
							GX_CS_SCALE_1, GX_TRUE, GX_TEVPREV);
		break;

	default:
		GX_SetNumTevStages(1);
		GX_SetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD0, GX_TEXMAP0, GX_COLORZERO);

		GX_SetTevOp(GX_TEVSTAGE0, GX_REPLACE);
		break;
	}
}

static inline void _tex_vert(f32 x, f32 y, f32 s, f32 t) {
	GX_Position2f32(x, y);
	GX_TexCoord2f32(s, t);
}

static void _quad(f32 x, f32 y, f32 w, f32 h, f32 s1, f32 t1, f32 s2, f32 t2) {
	Mtx m;
	Mtx mv;

	guMtxIdentity(m);
	guMtxTrans(m, x, y, ORIGIN_Z);
	guMtxConcat(_gfx.view, m, mv);
	GX_LoadPosMtxImm(mv, GX_PNMTX0);

	GX_Begin(GX_QUADS, GX_VTXFMT0, 4);
	_tex_vert(0.0, 0.0, s1, t1);
	_tex_vert(w, 0.0, s2, t1);
	_tex_vert(w, h, s2, t2);
	_tex_vert(0.0, h, s1, t2);
	GX_End();
}

void gfx_draw_tex(gfx_tex_t *tex, gfx_screen_coords_t *coords) {
	if (!tex || !coords)
		return;

	if (_gfx.tex_loaded != &tex->obj) {
		GX_LoadTexObj(&tex->obj, GX_TEXMAP0);
		_gfx.tex_loaded = &tex->obj;
	}

	_quad(coords->x, coords->y, coords->w, coords->h, 0.0, 0.0, 1.0, 1.0);
}

void gfx_draw_tile(gfx_tiles_t *tiles, gfx_screen_coords_t *coords,
					u8 col, u8 row) {
	gfx_draw_tile_by_index(tiles, coords, row * tiles->cols + col);
}

void gfx_draw_tile_by_index(gfx_tiles_t *tiles, gfx_screen_coords_t *coords,
							u16 index) {
	gfx_tex_coord_t *tile;

	if (!tiles)
		return;

	if (_gfx.tex_loaded != &tiles->tex->obj) {
		GX_LoadTexObj(&tiles->tex->obj, GX_TEXMAP0);
		_gfx.tex_loaded = &tiles->tex->obj;
	}

	tile = &tiles->tiles[index];
	_quad(coords->x, coords->y, coords->w, coords->h, tile->s1, tile->t1,
			tile->s2, tile->t2);
}

#ifdef __cplusplus
}
#endif

