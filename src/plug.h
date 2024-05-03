#ifndef PLUG_H
#define PLUG_H

#include <raylib.h>

#include "main.h"

typedef struct {
    char text[TEXT_CAP];
    Vector2 text_size;
    Vector2 text_pos;
} Text_Label;

typedef struct {
    Vector2 center;

    float radius;

    float start_angle;
    float end_angle;

    int segments;

    Color color;
} Track_Cursor;

typedef struct {
    int track_margin_bottom;

    float track_thickness;

    Color track_color;

    Track_Cursor track_cursor;

    Vector2 start_pos;
    Vector2 end_pos;
} Seek_Track;

typedef struct {
    bool music_loaded;
    bool music_paused;

    float music_volume;

    float time_music_played;
    float time_music_check;

    Music music;
} Plug_Music;

typedef struct {
    float font_size;
    float font_spacing;

    Font font;

    Seek_Track seek_track;

    Text_Label song_name;
    Text_Label song_time;

    Plug_Music music;
} Plug;

void plug_reinit(Plug*);
void plug_init_track(Seek_Track*);
bool plug_load_music(Plug*, const char*);
void plug_init_song_name(Plug*, Text_Label*);
void plug_init_song_time(Plug*, Text_Label*);
bool is_mouse_on_track(const Vector2, Seek_Track);

typedef void (*plug_init_t)(Plug* plug);
typedef void (*plug_free_t)(Plug* plug);
typedef void (*plug_frame_t)(Plug* plug);

#endif // PLUG_H
