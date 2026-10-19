// Web audio host: a mixer pthread renders AX blocks into the shared PCM ring,
// which the page's AudioWorklet drains (32 kHz interleaved stereo).
#include "audio_host.h"
#include "platform/web/web_shared.h"
#include <dolphin/ar.h>
#include <pthread.h>
#include <unistd.h>
#include <algorithm>
#include <atomic>
#include <cstdio>
#include <cstdlib>
#include <cstring>

namespace {
constexpr unsigned kBlockFrames = 160; // AX renders 160 samples per callback.
void (*renderer)(int16_t*, unsigned);
pthread_t mixer;
bool running;
std::atomic<bool> stop_requested;

void* mix(void*) {
    auto* shared = melee_web_shared_block();
    uint32_t write = __atomic_load_n(&shared->audio_write_pos, __ATOMIC_RELAXED);
    while (!stop_requested.load(std::memory_order_relaxed)) {
        for (;;) {
            const uint32_t read = __atomic_load_n(&shared->audio_read_pos, __ATOMIC_RELAXED);
            const uint32_t target = std::min<uint32_t>(
                __atomic_load_n(&shared->audio_target_frames, __ATOMIC_RELAXED), MELEE_WEB_AUDIO_RING_FRAMES);
            if (write - read + kBlockFrames > target) break;
            // The renderer takes OSDisableInterrupts internally, as on SDL's thread.
            int16_t block[kBlockFrames * 2];
            renderer(block, kBlockFrames);
            const unsigned at = write % MELEE_WEB_AUDIO_RING_FRAMES;
            const unsigned first = std::min(kBlockFrames, MELEE_WEB_AUDIO_RING_FRAMES - at);
            std::memcpy(&shared->audio_ring[at * 2], block, first * 2 * sizeof(int16_t));
            std::memcpy(&shared->audio_ring[0], block + first * 2, (kBlockFrames - first) * 2 * sizeof(int16_t));
            write += kBlockFrames;
            __atomic_store_n(&shared->audio_write_pos, write, __ATOMIC_RELEASE);
        }
        usleep(1000);
    }
    return nullptr;
}
}

extern "C" void MeleeNativeAudioOpen(void (*render)(int16_t*, unsigned)) {
    if (running) return;
    renderer = render;
    auto* shared = melee_web_shared_block();
    __atomic_store_n(&shared->audio_target_frames, MELEE_WEB_AUDIO_TARGET_FRAMES, __ATOMIC_RELAXED);
    stop_requested.store(false, std::memory_order_relaxed);
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setstacksize(&attr, 1u << 20);
    const int result = pthread_create(&mixer, &attr, mix, nullptr);
    pthread_attr_destroy(&attr);
    if (result) {
        std::fprintf(stderr, "Audio mixer thread failed: %d\n", result);
        std::abort();
    }
    running = true;
    std::fprintf(stderr, "[audio] driver=web-worklet ring_frames=%u target_frames=%u\n",
                 MELEE_WEB_AUDIO_RING_FRAMES, MELEE_WEB_AUDIO_TARGET_FRAMES);
}

extern "C" void MeleeNativeAudioClose(void) {
    if (!running) return;
    stop_requested.store(true, std::memory_order_relaxed);
    pthread_join(mixer, nullptr);
    running = false;
}

extern "C" void* MeleeNativeARAM(unsigned* size) {
    *size = ARGetSize();
    return ARGetStorageAddress();
}
