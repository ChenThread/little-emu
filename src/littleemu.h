#include <stdbool.h>
#include <stdint.h>
#include <sys/types.h>

#define TIME_IN_ORDER(t0, t1) (((t0) - (t1)) > ((t1) - (t0)))

// structure must not be packed
// this does not need to be serialised at all
// actually, please don't serialise this
struct EmuGlobal {
	// Pointers and lengths
	const char *core_name; // MUST point to lemu_core_name
	size_t common_header_len;
	size_t full_global_len;
	size_t state_len;

	size_t ram_count;
	struct EmuRamHead {
		size_t len;
		void *ptr;
		uintptr_t flags; // only low 32 bits used
		char *name;
	} *ram_ptrs;

	size_t rom_count;
	struct EmuRomHead {
		size_t len;
		void *ptr;
		uintptr_t flags; // only low 32 bits used
		char *name;
	} *rom_ptrs;

	size_t chicken_pointer_count;
	void **chicken_pointers;

	size_t input_button_count;
	size_t player_count;

	// Common data
	void *current_state; // packed fixed-size structure with no pointers in it
	uint64_t twait;
	bool no_draw;

	// Core-specific data
	uint8_t extra_data[];
};

// structure MUST be packed
struct EmuState {
	uint64_t timestamp;
	uint64_t timestamp_end;
} __attribute__((__packed__));

enum EmuSurfaceFormat {
	EMU_SURFACE_FORMAT_BGRA_32
};

struct EmuSurface {
	int width, height, pitch;
	enum EmuSurfaceFormat format;
	void* pixels;
};

// core.c
uint64_t time_now(void);
struct EmuGlobal *lemu_global_new(const char *fname, const void *data, size_t len);
void lemu_global_free(struct EmuGlobal *G);
void lemu_state_init(struct EmuGlobal *G, void *state);
void lemu_run_frame(struct EmuGlobal *G, void *state, bool no_draw);
void lemu_copy(struct EmuGlobal *G, void *dest_state, void *src_state);
void lemu_handle_input(struct EmuGlobal *G, void *state, int player_id, int input_id, bool down);

struct EmuSurface *lemu_surface_new(struct EmuGlobal *G);
void lemu_surface_free(struct EmuSurface* surface);
void lemu_video_callback(struct EmuGlobal *G, struct EmuSurface *S);
void lemu_audio_callback(struct EmuGlobal *G, void *state, uint8_t *stream, int len);
