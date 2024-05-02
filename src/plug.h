#ifndef PLUG_H
#define PLUG_H

#include <raylib.h>

#include "main.h"

typedef struct {
    float font_size;
    float font_spacing;
    float time_played;
    float time_check;

    Font font;

    char name_text[NAME_TEXT_CAP];
    char time_text[TIME_TEXT_CAP];

    Vector2 name_text_size;
    Vector2 name_text_pos;

    Vector2 time_text_size;
    Vector2 time_text_pos;

    bool pause;
    bool music_loaded;

    Music music;
} Plug;

typedef void (*plug_init_t)(Plug* plug);
typedef void (*plug_update_t)(Plug* plug);
typedef void (*plug_free_t)(Plug* plug);
typedef void (*plug_frame_t)(Plug* plug);

#endif // PLUG_H
