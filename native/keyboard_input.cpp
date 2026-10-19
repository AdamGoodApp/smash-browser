#include <SDL3/SDL.h>
#include <dolphin/pad.h>
#include <cstdint>
#include <array>
#include <cstdlib>
#include <cstdio>
#include <fstream>
#include <string>
extern "C" void MeleeNativeSetKeyboard(u16, s8, s8, s8, s8);
#ifdef __EMSCRIPTEN__
#include "platform/web/web_shared.h"
// The page publishes key state in shared memory; a changed press sequence
// stands in for a keydown event seen since the previous sample.
static std::array<uint8_t, MELEE_WEB_KEY_COUNT> last_seq;
#else
static std::array<bool, SDL_SCANCODE_COUNT> pressed;
#endif
static int ready_scene=-1;
extern "C" void MeleeNativeRenderCheckScene(int);
extern "C" void MeleeNativeMatrixScene(int);
extern "C" void MeleeNativeInputScene(int scene) {
    ready_scene=scene;
    if(std::getenv("MELEE_TRACE_INPUT")) std::fprintf(stderr,"[input] ready scene %d\n",scene);
    MeleeNativeRenderCheckScene(scene);
    MeleeNativeMatrixScene(scene);
    if(std::getenv("MELEE_INPUT_SCRIPT")) std::fprintf(stderr,"[input-test] ready scene %d\n",scene);
}
// Opt-in integration-test input, sampled at the same VI boundary as real keys.
// Each line is "frame_count keys" (e.g. "2 X", "20 W", "60 NONE").
// Timing is in game retraces, independent of CUA/OS key injection latency.
static bool replayInput() {
    static const char* path=std::getenv("MELEE_INPUT_SCRIPT");
    if(!path) return false;
    static std::ifstream script(path);
    static bool initialized=false, finished=false;
    static unsigned remaining=0, line_number=0;
    static std::streampos resume=0;
    static u16 buttons=0;
    static s8 x=0,y=0,cx=0,cy=0;
    static int wait_scene=-1;
    if(!initialized) {
        initialized=true;
        if(!script) {std::fprintf(stderr,"Cannot open input script: %s\n",path);std::abort();}
    }
    if(!remaining) {
        if(finished) {
            // Editors may replace the file atomically when appending a segment.
            script.close();script.open(path);script.seekg(resume);
        }
        std::string line;
        bool found=false;
        while(std::getline(script,line)) {
            resume=script.tellg();
            if(resume==std::streampos(-1)) {
                script.clear();script.seekg(0,std::ios::end);resume=script.tellg();
            }
            ++line_number;
            if(line.empty()||line[0]=='#') continue;
            char token[64],extra;
            if(std::sscanf(line.c_str(),"%u %63s %c",&remaining,token,&extra)!=2||remaining==0||remaining>36000) {
                std::fprintf(stderr,"Invalid input script line %u\n",line_number);std::abort();
            }
            const std::string key=token;
            buttons=0;x=0;y=0;cx=0;cy=0;wait_scene=-1;
            if(key=="SCENE_TITLE") wait_scene=0;
            else if(key=="SCENE_MENU") wait_scene=1;
            else if(key=="SCENE_MATCH") wait_scene=2;
            else if(key=="SCENE_RESULTS") wait_scene=5;
            else if(key=="SCENE_CSS") wait_scene=8;
            else if(key=="SCENE_SSS") wait_scene=9;
            else if(key=="SCENE_MOVIE") wait_scene=28;
            else {
                size_t start=0;
                do {
                    const auto end=key.find('+',start);
                    const auto part=key.substr(start,end-start);
                    if(part=="X") buttons|=PAD_BUTTON_A;
                    else if(part=="Z") buttons|=PAD_BUTTON_B;
                    else if(part=="C") buttons|=PAD_BUTTON_X;
                    else if(part=="V") buttons|=PAD_BUTTON_Y;
                    else if(part=="Q") buttons|=PAD_TRIGGER_L;
                    else if(part=="E") buttons|=PAD_TRIGGER_R;
                    else if(part=="R") buttons|=PAD_TRIGGER_Z;
                    else if(part=="START") buttons|=PAD_BUTTON_START;
                    else if(part=="W") y=80;
                    else if(part=="S") y=-80;
                    else if(part=="A") x=-80;
                    else if(part=="D") x=80;
                    else if(part=="I") cy=80;
                    else if(part=="K") cy=-80;
                    else if(part=="J") cx=-80;
                    else if(part=="L") cx=80;
                    else if(part!="NONE") {std::fprintf(stderr,"Unknown test key: %s\n",part.c_str());std::abort();}
                    if(end==std::string::npos) break;
                    start=end+1;
                } while(true);
            }
            std::fprintf(stderr,"[input-test] line %u: %u frames %s\n",line_number,remaining,key.c_str());
            finished=false;found=true;break;
        }
        if(!found) {
            if(!finished) std::fprintf(stderr,"[input-test] script finished; manual control restored\n");
            finished=true;buttons=0;x=0;y=0;
        }
    }
    if(finished) return false;
    if(wait_scene>=0) {
        MeleeNativeSetKeyboard(0,0,0,0,0);
        if(ready_scene==wait_scene) {remaining=0;wait_scene=-1;}
        else if(--remaining==0) {std::fprintf(stderr,"[input-test] timed out waiting for scene %d, current %d\n",wait_scene,ready_scene);std::abort();}
        return true;
    }
    MeleeNativeSetKeyboard(buttons,x,y,cx,cy);--remaining;return true;
}
extern "C" void MeleeNativeKeyboardEvent(const SDL_Event* event) {
#ifdef __EMSCRIPTEN__
    (void)event;
#else
    if (std::getenv("MELEE_TRACE_INPUT") && (event->type == SDL_EVENT_KEY_DOWN || event->type == SDL_EVENT_KEY_UP))
        std::fprintf(stderr, "[keyboard] %s scancode=%d\n", event->type == SDL_EVENT_KEY_DOWN ? "down" : "up", int(event->key.scancode));
    if(event->type==SDL_EVENT_KEY_DOWN && event->key.scancode>SDL_SCANCODE_UNKNOWN && event->key.scancode<SDL_SCANCODE_COUNT)
        pressed[event->key.scancode]=true;
    if(event->type==SDL_EVENT_WINDOW_FOCUS_LOST) pressed.fill(false);
#endif
}
extern "C" void MeleeNativeSampleKeyboard(void) {
#ifdef __EMSCRIPTEN__
    auto* shared=melee_web_shared_block();
    std::array<uint8_t, MELEE_WEB_KEY_COUNT> seq;
    for(unsigned i=0;i<seq.size();++i) seq[i]=__atomic_load_n(&shared->key_press_seq[i],__ATOMIC_RELAXED);
    if(replayInput()) {last_seq=seq;return;}
    const bool focused=!(__atomic_load_n(&shared->flags,__ATOMIC_RELAXED)&MELEE_WEB_FLAG_FOCUS_LOST);
    const auto down=[&](SDL_Scancode code) {
        const unsigned c=unsigned(code);
        return focused&&(((__atomic_load_n(&shared->keys_down[c>>5],__ATOMIC_RELAXED)>>(c&31))&1)||seq[c]!=last_seq[c]);
    };
#else
    if(replayInput()) {pressed.fill(false);return;}
    const bool* keys=SDL_GetKeyboardState(nullptr);
    const bool focused=SDL_GetKeyboardFocus()!=nullptr;
    const auto down=[&](SDL_Scancode code) {return focused&&(keys[code]||pressed[code]);};
#endif
    u16 buttons=0;
    if(down(SDL_SCANCODE_X)) buttons|=PAD_BUTTON_A;
    if(down(SDL_SCANCODE_Z)) buttons|=PAD_BUTTON_B;
    if(down(SDL_SCANCODE_C)) buttons|=PAD_BUTTON_X;
    if(down(SDL_SCANCODE_V)) buttons|=PAD_BUTTON_Y;
    if(down(SDL_SCANCODE_RETURN)) buttons|=PAD_BUTTON_START;
    if(down(SDL_SCANCODE_Q)) buttons|=PAD_TRIGGER_L;
    if(down(SDL_SCANCODE_E)) buttons|=PAD_TRIGGER_R;
    if(down(SDL_SCANCODE_R)) buttons|=PAD_TRIGGER_Z;
    if(down(SDL_SCANCODE_LEFT)) buttons|=PAD_BUTTON_LEFT;
    if(down(SDL_SCANCODE_RIGHT)) buttons|=PAD_BUTTON_RIGHT;
    if(down(SDL_SCANCODE_UP)) buttons|=PAD_BUTTON_UP;
    if(down(SDL_SCANCODE_DOWN)) buttons|=PAD_BUTTON_DOWN;
    MeleeNativeSetKeyboard(buttons,80*(down(SDL_SCANCODE_D)-down(SDL_SCANCODE_A)),
        80*(down(SDL_SCANCODE_W)-down(SDL_SCANCODE_S)),
        80*(down(SDL_SCANCODE_L)-down(SDL_SCANCODE_J)),
        80*(down(SDL_SCANCODE_I)-down(SDL_SCANCODE_K)));
#ifdef __EMSCRIPTEN__
    last_seq=seq;
#else
    pressed.fill(false);
#endif
}
