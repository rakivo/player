#ifndef PLUG_H
#define PLUG_H

#define WINDOW_WIDTH 1000
#define WINDOW_HEIGHT 600

#define DEBUG

#ifdef DEBUG
#   define FONT_PATH "../resources/Alegreya-Regular.ttf"
#   define MUTED_PATH "../resources/muted.png"
#   define UNMUTED_PATH "../resources/unmuted.png"
#   define SHUFFLE_PATH "../resources/shuffle.png"
#   define LIB_PLUG_PATH "../build/libplug.so"
#   define CROSSED_SHUFFLE_PATH "../resources/crossed_shuffle.png"
#else
#   define FONT_PATH "resources/Alegreya-Regular.ttf"
#   define MUTED_PATH "resources/muted.png"
#   define UNMUTED_PATH "resources/unmuted.png"
#   define SHUFFLE_PATH "resources/shuffle.png"
#   define LIB_PLUG_PATH "build/libplug.so"
#   define CROSSED_SHUFFLE_PATH "resources/crossed_shuffle.png"
#endif

#define FN(name) name##_t name

#define FN_SYM(name, lib, do_)                             \
    *(void**) (&name)  = dlsym(lib, #name);                \
    if (name == NULL) {                                    \
        TraceLog(LOG_ERROR, "Failed to find %s in %s: %s", \
                 #name, LIB_PLUG_PATH, dlerror());         \
        do_;                                               \
    }

typedef void  (*plug_init_t)(void);
typedef void  (*plug_free_t)(void);
typedef void  (*plug_frame_t)(void);
typedef void* (*plug_pre_reload_t)(void);
typedef void  (*plug_post_reload_t)(void*);

#endif // PLUG_H
