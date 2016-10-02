/* 
 * Gamecube/Wii VIDEO/GX subsystem wrapper
 *
 * Copyright (C) 2008, 2009		Andre Heider "dhewg" <dhewg@wiibrew.org>
 *
 * This code is licensed to you under the terms of the GNU GPL, version 2;
 * see file COPYING or http://www.gnu.org/licenses/old-licenses/gpl-2.0.txt
 *
 */

#ifndef GFX_CON_H
#define GFX_CON_H

#include <stdio.h>

#include "gfx_types.h"
#include "gfx_con_esc.h"

#ifdef __cplusplus
extern "C" {
#endif

bool gfx_con_init(gfx_screen_coords_t *coords);
void gfx_con_deinit(void);

u8 gfx_con_get_columns(void);
u8 gfx_con_get_rows(void);

void gfx_con_set_alpha(u8 foreground, u8 background);

void gfx_con_draw(void);

static inline void gfx_con_reset(void) {
	printf(CON_RESET);
}

static inline void gfx_con_clear(void) {
	printf(CON_CLEAR);
}

static inline void gfx_con_set_pos(u8 row, u8 column) {
	printf(CON_ESC "%u;%uH", row, column);
}

static inline void gfx_con_cur_up(u8 rows) {
	printf(CON_ESC "%uA", rows);
}

static inline void gfx_con_cur_down(u8 rows) {
	printf(CON_ESC "%uB", rows);
}

static inline void gfx_con_cur_forward(u8 columns) {
	printf(CON_ESC "%uC", columns);
}

static inline void gfx_con_cur_backward(u8 columns) {
	printf(CON_ESC "%uD", columns);
}

static inline void gfx_con_save_pos(void) {
	printf(CON_SAVEPOS);
}

static inline void gfx_con_restore_pos(void) {
	printf(CON_RESTOREPOS);
}

static inline void gfx_con_save_attr(void) {
	printf(CON_SAVEATTR);
}

static inline void gfx_con_restore_attr(void) {
	printf(CON_RESTOREATTR);
}

static inline void gfx_con_set_foreground_color(u8 index, bool bold) {
	if (bold)
		printf(CON_ESC "%u;1m", 30 + index);
	else
		printf(CON_ESC "%um", 30 + index);
}

static inline void gfx_con_set_background_color(u8 index, bool bold) {
	if (bold)
		printf(CON_ESC "%u;1m", 40 + index);
	else
		printf(CON_ESC "%um", 40 + index);
}

static inline void gfx_con_reset_colors(void) {
	printf(CON_COLRESET);
}

#ifdef __cplusplus
}
#endif

#endif
