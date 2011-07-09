/* 
 * Gamecube/Wii VIDEO/GX subsystem wrapper
 *
 * Copyright (C) 2008, 2009		Andre Heider "dhewg" <dhewg@wiibrew.org>
 *
 * This code is licensed to you under the terms of the GNU GPL, version 2;
 * see file COPYING or http://www.gnu.org/licenses/old-licenses/gpl-2.0.txt
 *
 */

#include <ogc/machine/processor.h>
#include <ogc/usbgecko.h>

#include "gfx.h"
#include "gfx_con.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

#include <sys/iosupport.h>

#define USBGECKO_CHANNEL 1

#define TEX_WIDTH 160
#define TEX_HEIGHT 144
#define TEX_TILES_X 16
#define TEX_TILES_Y 9
#define TEX_CHAR_FIRST 0x20
#define TEX_CHAR_LAST 0xb0
#define TEX_CHAR_WIDTH (TEX_WIDTH / TEX_TILES_X)
#define TEX_CHAR_HEIGHT (TEX_HEIGHT / TEX_TILES_Y)

#define TABWIDTH 4

extern unsigned int gfx_con_font_len;
extern unsigned char gfx_con_font[];

#ifdef __cplusplus
extern "C" {
#endif

static ssize_t _con_write(struct _reent *r, int fd, const char *ptr,
							size_t len);
static ssize_t _con_read(struct _reent *r, int fd, char *ptr, size_t len);

static const devoptab_t _dt_stdio = {
	"stdio",			// device name
	0,					// size of file structure
	NULL,				// device open
	NULL,				// device close
	_con_write,			// device write
	_con_read,			// device read
	NULL,				// device seek
	NULL,				// device fstat
	NULL,				// device stat
	NULL,				// device link
	NULL,				// device unlink
	NULL,				// device chdir
	NULL,				// device rename
	NULL,				// device mkdir
	0,					// dirStateSize
	NULL,				// device diropen_r
	NULL,				// device dirreset_r
	NULL,				// device dirnext_r
	NULL,				// device dirclose_r
	NULL,				// device statvfs_r
	NULL,				// device ftrunctate_r
	NULL,				// device fsync_r
	NULL,				// deviceData;
	NULL,				// device chmod_r
	NULL				// device fchmod_r
};

static const GXColor _con_colors[] = {
	{ 0x00, 0x00, 0x00, 0xff },
	{ 0xe0, 0x10, 0x10, 0xff },
	{ 0x20, 0xad, 0x20, 0xff },
	{ 0xd4, 0xc2, 0x4f, 0xff },
	{ 0x23, 0x1b, 0xb8, 0xff },
	{ 0x9c, 0x38, 0x85, 0xff },
	{ 0x1d, 0xbd, 0xb8, 0xff },
	{ 0xfe, 0xfe, 0xfe, 0xff },
	{ 0x0a, 0x0a, 0x0a, 0xff },
	{ 0xe8, 0x3a, 0x3d, 0xff },
	{ 0x35, 0xe9, 0x56, 0xff },
	{ 0xff, 0xff, 0x2f, 0xff },
	{ 0x3a, 0x53, 0xf0, 0xff },
	{ 0xe6, 0x28, 0xba, 0xff },
	{ 0x1c, 0xf5, 0xf5, 0xff },
	{ 0xff, 0xff, 0xff, 0xff }
};

static struct {
	bool usbgecko;
	gfx_tex_t tex;
	gfx_tiles_t tiles;
	f32 x, y;
	u8 cols, rows;

	u8 col, row;
	u16 *buf;
	u16 *end;
	u16 *top;
	u16 *pos;
	u8 fg, bg;
	u8 alpha_fg, alpha_bg;

	u8 saved_col, saved_row;
	u8 saved_fg, saved_bg;
} _con;

static void _con_reset(void) {
	_con.pos = _con.buf;
	_con.top = _con.buf;
	_con.col = 0;
	_con.row = 0;
	_con.fg = 7;
	_con.bg = 0;
	_con.alpha_fg = 0xff;
	_con.alpha_bg = 0xff;
	_con.saved_col = 0;
	_con.saved_row = 0;
	_con.saved_fg = 7;
	_con.saved_bg = 0;
	memset(_con.buf, 0, _con.cols * _con.rows * sizeof(u16));
}

static void _con_clear(void) {
	_con.pos = _con.buf;
	_con.top = _con.buf;
	_con.col = 0;
	_con.row = 0;
	memset(_con.buf, 0, _con.cols * _con.rows * sizeof(u16));
}

static void _con_lf(void) {
	_con.pos += _con.cols - _con.col;
	if (_con.pos > _con.end)
		_con.pos = _con.buf;
	_con.col = 0;

	if (_con.row == _con.rows - 1) {
		_con.top += _con.cols;
		if (_con.top > _con.end)
			_con.top = _con.buf;
		memset(_con.pos, 0, _con.cols * sizeof(u16));
	} else {
		_con.row++;
	}
}

static void _con_setpos(void) {
	_con.pos = _con.top + _con.row * _con.cols + _con.col;
	if (_con.pos > _con.end)
		_con.pos = _con.buf + (ptrdiff_t) (_con.pos - _con.end) - 1;
}

static size_t _con_esc(const char *ptr, size_t len) {
	char c;
	size_t ret = 0;
	s16 parms[3];
	u8 num = 0, count = 0;
	u8 color;
	s16 tmp;

	parms[0] = -1;
	parms[1] = -1;
	parms[2] = -1;

	c = *ptr++;
	ret++;

	switch (c) {
	case 'c': // Reset Device
		_con_reset();
		break;

	case '7': // Save Cursor & Attrs
		_con.saved_col = _con.col;
		_con.saved_row = _con.row;
		_con.saved_fg = _con.fg;
		_con.saved_bg = _con.bg;
		break;

	case '8': // Restore Cursor & Attrs
		_con.col = _con.saved_col;
		_con.row = _con.saved_row;
		_con.fg = _con.saved_fg;
		_con.bg = _con.saved_bg;
		_con_setpos();
		break;

	case '[':
		while (len) {
			c = *ptr;

			if ((c >= '0') && (c <= '9')) {
				if (parms[num] == -1) {
					count++;
					parms[num] = c - '0';
				} else {
					parms[num] = parms[num] * 10 + c - '0';
				}
			} else if (c == ';')
				num++;
			else
				break;

			if (num > 2)
				break;

			ptr++;
			ret++;
			len--;
		}

		c = *ptr++;
		ret++;

		switch (c) {
		case 'H': // Cursor Home
		case 'f': // Force Cursor Position
			_con.row = 0;
			_con.col = 0;
			if (count > 0) {
				tmp = parms[0] - 1;
				if (tmp < 0)
					tmp = 0;
				if (tmp > _con.rows - 1)
					tmp = _con.rows - 1;
				_con.row = tmp;
			}

			if (count > 1) {
				tmp = parms[1] - 1;
				if (tmp < 0)
					tmp = 0;
				if (tmp > _con.cols - 1)
					tmp = _con.cols - 1;
				_con.col = tmp;
			}

			_con_setpos();
			break;

		case 'A': // Cursor Up
			if (count)
				tmp = parms[0];
			else
				tmp = 1;
			if (tmp > _con.row)
				_con.row = 0;
			else
				_con.row -= tmp;
			_con_setpos();
			break;

		case 'B': // Cursor Down
			if (count)
				tmp = parms[0];
			else
				tmp = 1;
			if (_con.row + tmp > _con.rows - 1)
				_con.row = _con.rows - 1;
			else
				_con.row += tmp;
			_con_setpos();
			break;

		case 'C': // Cursor Forward
			if (count)
				tmp = parms[0];
			else
				tmp = 1;
			if (tmp > _con.col)
				_con.col = 0;
			else
				_con.col -= tmp;
			_con_setpos();
			break;

		case 'D': // Cursor Backward
			if (count)
				tmp = parms[0];
			else
				tmp = 1;
			if (_con.col + tmp > _con.cols - 1)
				_con.col = _con.cols - 1;
			else
				_con.col += tmp;
			_con_setpos();
			break;

		case 's': // Save Cursor
			_con.saved_col = _con.col;
			_con.saved_row = _con.row;
			break;

		case 'u': // Unsave Cursor
			_con.col = _con.saved_col;
			_con.row = _con.saved_row;
			_con_setpos();
			break;

		case 'J':
			switch (parms[0]) {
			case 2: // Erase Screen
				_con_clear();
				break;

			default:
				break;
			}

		case 'm': // Set Attribute Mode
			if (!count)
				break;

			if (!parms[0]) {
				_con.fg = 7;
				_con.bg = 0;
			} else if ((parms[0] >= 30) && (parms[0] <= 37)) {
				color = parms[0] - 30;

				if (parms[1] == 1)
					color += 8;

				_con.fg = color;
			} else if ((parms[0] >= 40) && parms[0] <= 47) {
				color = parms[0] - 40;

				if (parms[1] == 1)
					color += 8;

				_con.bg = color;
			}
			break;

		default:
			break;
		}

		break;

	default:
		break;
	}

	return ret;
}

static ssize_t _con_write(struct _reent *r, int fd, const char *ptr,
							size_t len) {
	u32 level;
	ssize_t ret = 0;
	u16 tmp;

	(void) r;
	(void) fd;

	_CPU_ISR_Disable(level);

	if (_con.usbgecko)
		usb_sendbuffer(USBGECKO_CHANNEL, ptr, len);

	while (len) {
		switch (*ptr) {
		case '\b':
			if (_con.col > 0) {
				_con.pos--;
				_con.col--;
			}
			break;

		case 0x1b: // \e
			if (len > 1) {
				tmp = _con_esc(ptr + 1, len - 1);
				ptr += tmp;
				ret += tmp;
				len -= tmp;
			}
			break;

		case '\f':
			tmp = _con.col;
			_con_lf();
			_con.pos += tmp;
			_con.col = tmp;
			break;

		case '\n':
			_con_lf();
			break;

		case '\r':
			_con.pos -= _con.col;
			_con.col = 0;
			break;

		case '\t':
			tmp = (_con.col + 1) % TABWIDTH;
			if (!tmp)
				tmp = TABWIDTH;
			if (_con.col + tmp > _con.cols - 1)
				_con_lf();
			_con.pos += tmp;
			_con.col += tmp;
			break;

		default:
			tmp = *ptr;

			if (tmp < 0x20)
				break;

			tmp |= (_con.fg & 0xf) << 12;
			tmp |= (_con.bg & 0xf) << 8;

			*_con.pos = tmp;
			if (_con.col == _con.cols - 1) {
				_con_lf();
			} else {
				_con.pos++;
				_con.col++;
			}
			break;
		}

		ptr++;
		ret++;
		len--;
	}

	_CPU_ISR_Restore(level);

	return ret;
}

static ssize_t _con_read(struct _reent *r, int fd, char *ptr, size_t len) {
	(void) r;
	(void) fd;
	(void) ptr;
	(void) len;

	// TODO implement me

	return -EINVAL;
}

bool gfx_con_init(gfx_screen_coords_t *coords) {
	static bool inited = false;
	u32 level;

	if (!inited) {
		memset(&_con, 0, sizeof(_con));

		if (!gfx_tex_init(&_con.tex, GFX_TF_I4, 0, TEX_WIDTH, TEX_HEIGHT)) 
			return false;

		// TODO new function to init a tex from an existing buffer
		memcpy(_con.tex.pixels, gfx_con_font, gfx_con_font_len);
		gfx_tex_flush_texture(&_con.tex);

		if (!gfx_tiles_init(&_con.tiles, &_con.tex, TEX_TILES_X, TEX_TILES_Y)) {
			gfx_tex_deinit(&_con.tex);
			return false;
		}

		inited = true;
	}

	_CPU_ISR_Disable(level);
	devoptab_list[STD_IN] = &_dt_stdio;
	devoptab_list[STD_OUT] = &_dt_stdio;
	devoptab_list[STD_ERR] = &_dt_stdio;
	_CPU_ISR_Restore(level);

	setvbuf(stdin, NULL, _IONBF, 0);
	setvbuf(stdout, NULL, _IONBF, 0);
	setvbuf(stderr, NULL, _IONBF, 0);

	clearerr(stdin);
	clearerr(stdout);
	clearerr(stderr);

	_con.usbgecko = usb_isgeckoalive(USBGECKO_CHANNEL);

	if (coords) {
		_con.x = coords->x;
		_con.y = coords->y;
		_con.cols = (u16) coords->w / TEX_CHAR_WIDTH;
		_con.rows = (u16) coords->h / TEX_CHAR_HEIGHT;
	} else {
		_con.x = 48;
		_con.y = 48;
		_con.cols = (gfx_video_get_width() - 96) / TEX_CHAR_WIDTH;
		_con.rows = (gfx_video_get_height() - 96) / TEX_CHAR_HEIGHT;
	}

	free(_con.buf);
	_con.buf = (u16 *) malloc(_con.cols * _con.rows * sizeof(u16));
	if (!_con.buf)
		return false;

	_con.end = _con.buf + _con.cols * _con.rows - 1;
	_con_reset();

	return true;
}

void gfx_con_deinit(void) {
	u32 level;

	_CPU_ISR_Disable(level);
	devoptab_list[STD_IN] = NULL;
	devoptab_list[STD_OUT] = NULL;
	devoptab_list[STD_ERR] = NULL;

	if (_con.usbgecko)
		usb_sendbuffer(USBGECKO_CHANNEL, CON_COLRESET, strlen(CON_COLRESET));

	_CPU_ISR_Restore(level);

	free(_con.buf);
	gfx_tiles_deinit(&_con.tiles);
	gfx_tex_deinit(&_con.tex);
	memset(&_con, 0, sizeof(_con));
}

u8 gfx_con_get_columns(void) {
	return _con.cols;
}

u8 gfx_con_get_rows(void) {
	return _con.rows;
}

void gfx_con_set_alpha(u8 foreground, u8 background) {
	_con.alpha_fg = foreground;
	_con.alpha_bg = background;
}

void gfx_con_draw(void) {
	u16 *p;
	gfx_screen_coords_t coords;
	bool vp;
	u16 x, y;
	u16 val;
	u8 c, fg, bg;
	GXColor c_fg, c_bg;

	p = _con.top;

	coords.y = _con.y;
	coords.w = TEX_CHAR_WIDTH;
	coords.h = TEX_CHAR_HEIGHT;

	vp = gfx_enable_viewport(false);

	for (y = 0; y < _con.rows; ++y) {
		coords.x = _con.x;

		for (x = 0; x < _con.cols; ++x) {
			val = *p;
			c = val & 0xff;
			fg = val >> 12;
			bg = (val >> 8) & 0xf;

			if ((c >= TEX_CHAR_FIRST) && (c <= TEX_CHAR_LAST)) {
				c_fg = _con_colors[fg];
				c_bg = _con_colors[bg];
				c_fg.a = _con.alpha_fg;

				if (bg)
					c_bg.a = _con.alpha_bg;
				else
					c_bg.a = 0;

				gfx_set_colorop(COLOROP_MODULATE_FGBG, c_fg, c_bg);

				c -= TEX_CHAR_FIRST;
				gfx_draw_tile_by_index(&_con.tiles, &coords, c);
			}

			if (p == _con.end)
				p = _con.buf;
			else
				p++;

			coords.x += TEX_CHAR_WIDTH;
		}

		coords.y += TEX_CHAR_HEIGHT;
	}

	gfx_set_colorop(COLOROP_NONE, gfx_color_none, gfx_color_none);
	gfx_enable_viewport(vp);
}

#ifdef __cplusplus
}
#endif

