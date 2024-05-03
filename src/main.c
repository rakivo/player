#include <stdio.h>
#include <dlfcn.h>

#include <raylib.h>

#include "plug.h"

#define LIB_PLUG_PATH "libplug.so"

#define REINIT

Plug plug = {0};

void* libplug           = NULL;
plug_init_t plug_init   = NULL;
plug_free_t plug_free   = NULL;
plug_frame_t plug_frame = NULL;

bool plug_reload(void)
{
    if (libplug) dlclose(libplug);

    libplug = dlopen(LIB_PLUG_PATH, RTLD_NOW);
    if (!libplug) {
        TraceLog(LOG_ERROR, "HOTRELOAD: could not load %s: %s", LIB_PLUG_PATH, dlerror());
        return false;
    }

    dlerror();

    *(void**) (&plug_init)   = dlsym(libplug, "plug_init");
    *(void**) (&plug_free)   = dlsym(libplug, "plug_free");
    *(void**) (&plug_frame)  = dlsym(libplug, "plug_frame");

    if (!plug_init || !plug_free || !plug_frame) {
        TraceLog(LOG_ERROR, "Failed to find functions in libplug", LIB_PLUG_PATH, dlerror());
        return false;
    }
    
    TraceLog(LOG_INFO, "Loaded libplug successfully", LIB_PLUG_PATH, dlerror());

    return true;
}

int main(void)
{
    SetTargetFPS(60);
    InitWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "Play");
    InitAudioDevice();
    SetExitKey(KEY_Q);

    if (!plug_reload()) return 1;
    plug_init(&plug);

    while (!WindowShouldClose()) {
        if (IsKeyPressed(KEY_R)) {
            if (!plug_reload()) return 1;
#ifdef REINIT
            plug_init(&plug);
#endif
        }
        plug_frame(&plug);
    }

    plug_free(&plug);
    CloseAudioDevice();
    CloseWindow();

    return 0;
}
