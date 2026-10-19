#pragma once
/* Shared-memory block between the game (wasm) and the page/AudioWorklet.
 * Byte offsets are part of the contract with browser-smash/src/game/shared.ts;
 * web_shared.c asserts every one of them. */
#include <stdint.h>
#include <emscripten/emscripten.h>

#define MELEE_WEB_MAGIC 0x4D4C4545u
#define MELEE_WEB_VERSION 1u
#define MELEE_WEB_KEY_WORDS 16                 /* 512 SDL scancodes */
#define MELEE_WEB_KEY_COUNT 512
#define MELEE_WEB_AUDIO_RING_FRAMES 8192       /* power of two */
#define MELEE_WEB_AUDIO_TARGET_FRAMES 1024

#define MELEE_WEB_FLAG_HIDDEN 0x1u             /* page hidden: pause rendering */
#define MELEE_WEB_FLAG_FOCUS_LOST 0x2u         /* page lost focus: release keys */
#define MELEE_WEB_FLAG_EXIT_REQUESTED 0x4u     /* page asked the game to exit */

#ifdef __cplusplus
extern "C" {
#endif

typedef struct MeleeWebPad {
    int16_t stick_x, stick_y, cstick_x, cstick_y; /* -80..80, up = positive Y */
    uint8_t trigger_l, trigger_r;                 /* 0..255 */
    uint16_t buttons;                             /* PAD_BUTTON_* / PAD_TRIGGER_* bits */
    uint8_t connected, pad_[3];
} MeleeWebPad; /* 16 B */

typedef struct MeleeWebShared {
    uint32_t magic;                 /* MELEE_WEB_MAGIC */
    uint32_t version;               /* MELEE_WEB_VERSION */
    uint32_t flags;                 /* MELEE_WEB_FLAG_* (page) */
    uint32_t frame_counter;         /* game: ++ after each present */
    uint32_t keys_down[MELEE_WEB_KEY_WORDS];       /* page: bitset over SDL scancodes */
    uint8_t key_press_seq[MELEE_WEB_KEY_COUNT];    /* page: ++ on keydown */
    MeleeWebPad pads[4];            /* page */
    uint32_t audio_write_pos;       /* frames, monotonic, mixer thread */
    uint32_t audio_read_pos;        /* frames, monotonic, worklet */
    uint32_t audio_underruns;       /* worklet */
    uint32_t audio_target_frames;   /* mixer keeps write - read <= this */
    int16_t audio_ring[MELEE_WEB_AUDIO_RING_FRAMES * 2]; /* interleaved stereo, 32 kHz */
} MeleeWebShared;

/* Returns the static block (fixed address inside the initial memory). */
EMSCRIPTEN_KEEPALIVE MeleeWebShared* melee_web_shared_block(void);

#ifdef __cplusplus
}
#endif
