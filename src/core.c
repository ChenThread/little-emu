#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include "littleemu.h"

// TODO: make cores more separable
struct EmuGlobal *lemu_core_global_new(const char *fname, const void *data, size_t len);
void lemu_core_global_free(struct EmuGlobal *G);
void lemu_core_state_init(struct EmuGlobal *G, void *state);
void lemu_core_run_frame(struct EmuGlobal *G, void *sms, bool no_draw);
void lemu_core_audio_callback(struct EmuGlobal *G, void *state, uint8_t *stream, int len);
void lemu_core_video_callback(struct EmuGlobal *G, struct EmuSurface *S);
void lemu_core_surface_configure(struct EmuGlobal *G, struct EmuSurface *S);
void lemu_core_handle_input(struct EmuGlobal *G, void *state, int player_id, int input_id, bool down);

uint64_t time_now(void)
{
	struct timeval ts;
	gettimeofday(&ts, NULL);

	uint64_t sec = ts.tv_sec;
	uint64_t usec = ts.tv_usec;
	sec *= 1000000ULL;
	usec += sec;
	return usec;
}

struct EmuGlobal *lemu_global_new(const char *fname, const void *data, size_t len)
{
	return lemu_core_global_new(fname, data, len);
}

void lemu_global_free(struct EmuGlobal *G)
{
	lemu_core_global_free(G);
}

void lemu_state_init(struct EmuGlobal *G, void *state)
{
	lemu_core_state_init(G, state);
}

void lemu_run_frame(struct EmuGlobal *G, void *sms, bool no_draw)
{
	lemu_core_run_frame(G, sms, no_draw);
}

void lemu_copy(struct EmuGlobal *G, void *dest_state, void *src_state)
{
	memcpy(dest_state, src_state, G->state_len);
}

void lemu_audio_callback(struct EmuGlobal *G, void *state, uint8_t *stream, int len)
{
	lemu_core_audio_callback(G, state, stream, len);
}

struct EmuSurface *lemu_surface_new(struct EmuGlobal *G)
{
	struct EmuSurface *surface;
	surface = (struct EmuSurface *) malloc(sizeof(struct EmuSurface));

	lemu_core_surface_configure(G, surface);
	return surface;
}

void lemu_surface_free(struct EmuSurface* S) {
	free(S->pixels);
	free(S);
}

void lemu_video_callback(struct EmuGlobal *G, struct EmuSurface* S) {
	lemu_core_video_callback(G, S);
}

void lemu_handle_input(struct EmuGlobal *G, void *state, int player_id, int input_id, bool down) {
	lemu_core_handle_input(G, state, player_id, input_id, down);
}