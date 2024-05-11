#include <stdio.h>
#include <stdlib.h>

#include <time.h>
#include <dlfcn.h>

#include <raylib.h>

#include "plug.h"

void* libplug;

FN(plug_init);
FN(plug_free);
FN(plug_frame);
FN(plug_pre_reload);
FN(plug_post_reload);

bool plug_reload(void)
{
    if (libplug) dlclose(libplug);

    libplug = dlopen(LIB_PLUG_PATH, RTLD_NOW);
    if (!libplug) {
        TraceLog(LOG_ERROR, "HOTRELOAD: could not load %s: %s", LIB_PLUG_PATH, dlerror());
        return false;
    }
    dlerror();

    FN_SYM(plug_init, libplug, return false);
    FN_SYM(plug_free, libplug, return false);
    FN_SYM(plug_frame, libplug, return false);
    FN_SYM(plug_pre_reload, libplug, return false);
    FN_SYM(plug_post_reload, libplug, return false);
    
    TraceLog(LOG_INFO, "Reloaded libplug successfully");

    return true;
}

int main(void)
{
    SetTargetFPS(60);
    SetConfigFlags(FLAG_WINDOW_RESIZABLE);

    InitWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "Player");
    InitAudioDevice();

    SetExitKey(KEY_Q);
    SetTargetFPS(GetMonitorRefreshRate(GetCurrentMonitor()));

    if (!plug_reload()) return 1;
    plug_init();

    srand(time(NULL));

    for (; !WindowShouldClose(); plug_frame()) {
        if (IsKeyPressed(KEY_R)) {
            void* plug = plug_pre_reload();
            if (!plug_reload()) return 1;
            plug_post_reload(plug);
        }
    }

    plug_free();
    CloseAudioDevice();
    CloseWindow();

    return 0;
}
