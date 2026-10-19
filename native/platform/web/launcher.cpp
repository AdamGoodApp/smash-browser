// Browser launcher: the page imports the disc into OPFS before main() runs and
// Aurora's web DVD backend reads it from Module.auroraDisc, so there is no
// picker here. Setup errors go back to the page as {type:'error'} messages.
#include "platform_launcher.h"
#include <emscripten/emscripten.h>
#include <cstdio>
#include <sys/stat.h>

namespace {
constexpr const char* kDiscPath = "opfs:disc";
constexpr const char* kConfigPath = "/melee-user";
constexpr const char* kCachePath = "/melee-cache";
}

EM_JS_DEPS(melee_web_launcher, "$UTF8ToString");
EM_JS(void, melee_web_post_error, (const char* message), {
    globalThis.postMessage({type: 'error', message: UTF8ToString(message)});
});

// MEMFS: console settings and bindings reset on reload (no persistence on web).
std::string MeleeConfigPath() { return kConfigPath; }
std::string MeleeCachePath() { return kCachePath; }

void MeleePrepareAppLogging() {
    // stderr already reaches the worker console; only create the MEMFS dirs.
    mkdir(kConfigPath, 0755);
    mkdir(kCachePath, 0755);
}

void MeleeShowLaunchError(const std::string& error) {
    std::fprintf(stderr, "[launch-error] %s\n", error.c_str());
    melee_web_post_error(error.c_str());
}

std::string MeleeChooseDisc(const std::string&) { return {}; }

std::string MeleeLaunchDisc(MeleeDiscValidator validate, bool) {
    const auto error = validate(kDiscPath);
    if (!error.empty()) {
        MeleeShowLaunchError(error);
        return {};
    }
    return kDiscPath;
}
