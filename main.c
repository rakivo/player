#include <stdio.h>
#include <dlfcn.h>

#include <raylib.h>

#include "plug.h"

#define LIB_PLUG_PATH "build/libplug.so"

void* libplug = NULL;

Plug plug = {0};

plug_init_t plug_init = NULL;
plug_update_t plug_update = NULL;
plug_free_t plug_free = NULL;
plug_frame_t plug_frame = NULL;

bool plug_reload(void)
{
    if (libplug) {
        printf("Closing libplug\n");
        if (dlclose(libplug) != 0) {
            fprintf(stderr, "dlclose failed: %s\n", dlerror());
            return false;
        }
    }

    libplug = dlopen(LIB_PLUG_PATH, RTLD_NOW);
    if (!libplug) {
        fprintf(stderr, "%s\n", dlerror());
        return false;
    }
    dlerror();

    *(void**) (&plug_init)   = dlsym(libplug, "plug_init");
    *(void**) (&plug_update) = dlsym(libplug, "plug_update");
    *(void**) (&plug_free)   = dlsym(libplug, "plug_free");
    *(void**) (&plug_frame)  = dlsym(libplug, "plug_frame");

    if (!plug_init || !plug_update || !plug_free || !plug_frame) {
        fprintf(stderr, "Failed to find functions in libplug\n");
        return false;
    }
    
    printf("Loaded libplug successfully\n");

    return true;
}

int main(void)
{
    SetTargetFPS(60);
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Play");
    InitAudioDevice();
    SetExitKey(KEY_Q);

    if (!plug_reload()) return 1;
    plug_init(&plug);

    while (!WindowShouldClose()) {
        if (IsKeyPressed(KEY_R)) {
            if (!plug_reload()) return 1;
            plug_update(&plug);
        }
        plug_frame(&plug);
    }

    plug_free(&plug);
    CloseAudioDevice();
    CloseWindow();

    return 0;
}
