#include "web_shared.h"
#include <stddef.h>

_Static_assert(sizeof(MeleeWebPad) == 16, "MeleeWebPad size");
_Static_assert(offsetof(MeleeWebPad, stick_x) == 0, "MeleeWebPad.stick_x");
_Static_assert(offsetof(MeleeWebPad, stick_y) == 2, "MeleeWebPad.stick_y");
_Static_assert(offsetof(MeleeWebPad, cstick_x) == 4, "MeleeWebPad.cstick_x");
_Static_assert(offsetof(MeleeWebPad, cstick_y) == 6, "MeleeWebPad.cstick_y");
_Static_assert(offsetof(MeleeWebPad, trigger_l) == 8, "MeleeWebPad.trigger_l");
_Static_assert(offsetof(MeleeWebPad, trigger_r) == 9, "MeleeWebPad.trigger_r");
_Static_assert(offsetof(MeleeWebPad, buttons) == 10, "MeleeWebPad.buttons");
_Static_assert(offsetof(MeleeWebPad, connected) == 12, "MeleeWebPad.connected");

_Static_assert(offsetof(MeleeWebShared, magic) == 0, "MeleeWebShared.magic");
_Static_assert(offsetof(MeleeWebShared, version) == 4, "MeleeWebShared.version");
_Static_assert(offsetof(MeleeWebShared, flags) == 8, "MeleeWebShared.flags");
_Static_assert(offsetof(MeleeWebShared, frame_counter) == 12, "MeleeWebShared.frame_counter");
_Static_assert(offsetof(MeleeWebShared, keys_down) == 16, "MeleeWebShared.keys_down");
_Static_assert(offsetof(MeleeWebShared, key_press_seq) == 80, "MeleeWebShared.key_press_seq");
_Static_assert(offsetof(MeleeWebShared, pads) == 592, "MeleeWebShared.pads");
_Static_assert(offsetof(MeleeWebShared, audio_write_pos) == 656, "MeleeWebShared.audio_write_pos");
_Static_assert(offsetof(MeleeWebShared, audio_read_pos) == 660, "MeleeWebShared.audio_read_pos");
_Static_assert(offsetof(MeleeWebShared, audio_underruns) == 664, "MeleeWebShared.audio_underruns");
_Static_assert(offsetof(MeleeWebShared, audio_target_frames) == 668, "MeleeWebShared.audio_target_frames");
_Static_assert(offsetof(MeleeWebShared, audio_ring) == 672, "MeleeWebShared.audio_ring");
_Static_assert(sizeof(MeleeWebShared) == 33440, "MeleeWebShared size");
_Static_assert((MELEE_WEB_AUDIO_RING_FRAMES & (MELEE_WEB_AUDIO_RING_FRAMES - 1)) == 0,
               "Audio ring length must be a power of two");

static MeleeWebShared g_shared = {
    .magic = MELEE_WEB_MAGIC,
    .version = MELEE_WEB_VERSION,
    .audio_target_frames = MELEE_WEB_AUDIO_TARGET_FRAMES,
};

EMSCRIPTEN_KEEPALIVE MeleeWebShared* melee_web_shared_block(void) { return &g_shared; }
