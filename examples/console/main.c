#include <stdio.h>
#include <unistd.h>
#include <string.h>

#include <ogcsys.h>

#include <gfx/gfx.h>
#include <gfx/gfx_con.h>

static bool quit = false;

void stmcb(void) {
	quit = true;
}

// TODO this is just my console test app, clean up this mess!
int main(int argc, char *argv[]) {
	(void) argc;
	(void) argv;

	VIDEO_Init();
	PAD_Init();

	SYS_SetResetCallback(stmcb);
	SYS_SetPowerCallback(stmcb);

	gfx_video_init(GFX_STANDARD_AUTO, GFX_MODE_DEFAULT);
	gfx_init();
	gfx_con_init(NULL);

	printf("startup\n");

	gfx_tex_t tex;
	memset(&tex, 0, sizeof(gfx_tex_t));
	if (!gfx_tex_init(&tex, GFX_TF_RGB565, 0, 16, 16)) {
		printf("failed to init tex!\n");
		return 1;
	}
	memset(tex.pixels, 0xe070, 16 * 16 * 2);
	gfx_tex_flush_texture(&tex);

	gfx_screen_coords_t coords_bg;
	gfx_coords(&coords_bg, &tex, GFX_COORD_FULLSCREEN);

	u32 frame = 0;
	u16 b;
	bool pf = false;
	u32 retries;
	u8 fg = 7, bg = 0;

	while (!quit) {
		if (pf)
			printf("frame: %u\n", frame);

		b = 0;
		if (PAD_ScanPads() & 1)
			b = PAD_ButtonsDown(0);

		if (b & PAD_BUTTON_A)
			quit = true;

		if (b & PAD_BUTTON_B)
			pf = !pf;

		if (b & PAD_BUTTON_X)
			printf(S_RED("Hello") " " S_BLUE("world") "!\n");

		if (b & PAD_BUTTON_Y)
			printf(CON_CLEAR);

		if (b & PAD_TRIGGER_Z) {
			printf(CON_RESET);
			fg = 7;
			bg = 0;
		}

		if (b & 15) {
			if (b & PAD_BUTTON_LEFT)
				fg = (fg - 1) % 8;
			if (b & PAD_BUTTON_RIGHT)
				fg = (fg + 1) % 8;
			if (b & PAD_BUTTON_UP)
				bg = (bg - 1) % 8;
			if (b & PAD_BUTTON_DOWN)
				bg = (bg + 1) % 8;

			printf(CON_ESC "%um" CON_ESC "%umnew color selected: %u %u\n", 30 + fg, 40 + bg, fg, bg);
		}

		printf(CON_SAVEATTR CON_POS(0, 20) S_WHITE("frame: %u") CON_RESTOREATTR, frame);

		retries = 0;
		while (!gfx_frame_start()) {
			retries++;
			if (retries > 1000) {
				printf("gx hates you\n");
				GX_AbortFrame();
				return -1;
			}

			usleep(50);
		}

		if (pf)
			gfx_set_colorop(COLOROP_SIMPLEFADE, NULL, NULL);

		gfx_draw_tex(&tex, &coords_bg);
		gfx_con_draw();

		gfx_frame_end();

		frame++;
	}

	printf("shutdown\n");

	gfx_tex_deinit(&tex);
	gfx_con_deinit();
	gfx_deinit();
	gfx_video_deinit();

	return 0;
}

