#include <dolphin/pad.h>
#include <array>
#include <chrono>
#include <cstring>
#include <mutex>
#ifdef __EMSCRIPTEN__
#include "platform/web/web_shared.h"
#endif

namespace {
// Keep Melee's 12-byte input ABI separate from Aurora's 16-byte extension.
struct MeleePadStatus {
    u16 button;
    s8 stickX, stickY, substickX, substickY;
    u8 triggerLeft, triggerRight, analogA, analogB;
    s8 err;
};
static_assert(sizeof(MeleePadStatus) == 12);
static_assert(sizeof(PADStatus) == 16);
static_assert(offsetof(MeleePadStatus, err) == offsetof(PADStatus, err));
std::mutex pad_mutex;
std::chrono::milliseconds sample_period(0);
std::chrono::steady_clock::time_point last_sample;
std::array<PADStatus, 4> latest;
u32 motor_mask;
MeleePadStatus keyboard;
void copy_to_game(MeleePadStatus* game, const PADStatus* native) {
    for (unsigned i = 0; i < 4; ++i) {
        game[i] = {};
        std::memcpy(&game[i], &native[i], offsetof(MeleePadStatus, err)+1);
    }
}
}
extern "C" u32 MeleeNativePADRead(MeleePadStatus* output) {
    std::lock_guard lock(pad_mutex);
    auto now = std::chrono::steady_clock::now();
    if (last_sample.time_since_epoch().count() == 0 || now-last_sample >= sample_period) {
#ifdef __EMSCRIPTEN__
        // The page publishes gamepads in shared memory; Aurora's PADRead is idle on web.
        auto* shared = melee_web_shared_block();
        for (unsigned i = 0; i < 4; ++i) {
            const MeleeWebPad& pad = shared->pads[i];
            PADStatus& status = latest[i];
            status = {};
            status.button = __atomic_load_n(&pad.buttons, __ATOMIC_RELAXED);
            status.stickX = static_cast<s8>(__atomic_load_n(&pad.stick_x, __ATOMIC_RELAXED));
            status.stickY = static_cast<s8>(__atomic_load_n(&pad.stick_y, __ATOMIC_RELAXED));
            status.substickX = static_cast<s8>(__atomic_load_n(&pad.cstick_x, __ATOMIC_RELAXED));
            status.substickY = static_cast<s8>(__atomic_load_n(&pad.cstick_y, __ATOMIC_RELAXED));
            status.triggerLeft = __atomic_load_n(&pad.trigger_l, __ATOMIC_RELAXED);
            status.triggerRight = __atomic_load_n(&pad.trigger_r, __ATOMIC_RELAXED);
            status.err = __atomic_load_n(&pad.connected, __ATOMIC_RELAXED) ? PAD_ERR_NONE : PAD_ERR_NO_CONTROLLER;
        }
        motor_mask = 0; last_sample = now;
#else
        motor_mask = PADRead(latest.data()); last_sample = now;
#endif
    }
    copy_to_game(output, latest.data());
    auto& player=output[0];
    if(player.err!=PAD_ERR_NONE) player={};
    player.button|=keyboard.button;
    if(keyboard.stickX) player.stickX=keyboard.stickX;
    if(keyboard.stickY) player.stickY=keyboard.stickY;
    if(keyboard.substickX) player.substickX=keyboard.substickX;
    if(keyboard.substickY) player.substickY=keyboard.substickY;
    if(keyboard.button&PAD_TRIGGER_L) player.triggerLeft=255;
    if(keyboard.button&PAD_TRIGGER_R) player.triggerRight=255;
    return motor_mask;
}
extern "C" void MeleeNativePADClamp(MeleePadStatus* data) {
    std::array<PADStatus, 4> native{};
    for (unsigned i = 0; i < 4; ++i)
        std::memcpy(&native[i], &data[i], offsetof(MeleePadStatus, err)+1);
    PADClamp(native.data()); copy_to_game(data, native.data());
}
extern "C" void PADSetSamplingRate(u32 milliseconds) {
    std::lock_guard lock(pad_mutex);
    sample_period = std::chrono::milliseconds(milliseconds);
}

extern "C" void MeleeNativeSetKeyboard(u16 buttons,s8 x,s8 y,s8 cx,s8 cy) {
    std::lock_guard lock(pad_mutex);
    keyboard={}; keyboard.button=buttons; keyboard.stickX=x; keyboard.stickY=y;
    keyboard.substickX=cx; keyboard.substickY=cy;
}
