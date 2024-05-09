#include <stdio.h>
#include <stdlib.h>

#include <time.h>
#include <dlfcn.h>

#include <raylib.h>

#include "plug.h"

void* libplug;

plug_init_t plug_init;
plug_free_t plug_free;
plug_frame_t plug_frame;
plug_pre_reload_t plug_pre_reload;
plug_post_reload_t plug_post_reload;

bool plug_reload(void)
{
    if (libplug) dlclose(libplug);

    libplug = dlopen(LIB_PLUG_PATH, RTLD_NOW);
    if (!libplug) {
        TraceLog(LOG_ERROR, "HOTRELOAD: could not load %s: %s", LIB_PLUG_PATH, dlerror());
        return false;
    }
    dlerror();

    *(void**) (&plug_init)  = dlsym(libplug, "plug_init");
    *(void**) (&plug_free)  = dlsym(libplug, "plug_free");
    *(void**) (&plug_frame) = dlsym(libplug, "plug_frame");
    *(void**) (&plug_pre_reload) = dlsym(libplug, "plug_pre_reload");
    *(void**) (&plug_post_reload) = dlsym(libplug, "plug_post_reload");

    if (!plug_init || !plug_free || !plug_frame) {
        TraceLog(LOG_ERROR, "Failed to find functions in %s: %s", LIB_PLUG_PATH, dlerror());
        return false;
    }
    
    TraceLog(LOG_INFO, "Reloaded libplug successfully");

    return true;
}

int main(void)
{
    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    SetTargetFPS(60);
    InitWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "Player");
    InitAudioDevice();
    SetExitKey(KEY_Q);
    SetTargetFPS(GetMonitorRefreshRate(GetCurrentMonitor()));

    if (!plug_reload()) return 1;
    plug_init();

    srand(time(NULL));

    while (!WindowShouldClose()) {
        if (IsKeyPressed(KEY_R)) {
            Plug* plug = plug_pre_reload();
            if (!plug_reload()) return 1;
            plug_post_reload(plug);
        }

        plug_frame();
    }

    plug_free();
    CloseAudioDevice();
    CloseWindow();

    return 0;
}
