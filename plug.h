#ifndef PLUG_H
#define PLUG_H

#include <raylib.h>

#include "main.h"

typedef struct {
    bool pause;
    bool music_loaded;

    Music music;
} Plug;

typedef void (*plug_init_t)(Plug* plug);
typedef void (*plug_update_t)(Plug* plug);
typedef void (*plug_free_t)(Plug* plug);
typedef void (*plug_frame_t)(Plug* plug);

#endif // PLUG_H
