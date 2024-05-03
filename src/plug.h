#ifndef PLUG_H
#define PLUG_H

#include <stdlib.h>
#include <raylib.h>

#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define MIN(a, b) ((a) < (b) ? (a) : (b))

#ifdef _WIN32
#   define DELIM '\\'
#else
#   define DELIM '/'
#endif

#define WINDOW_WIDTH 1000
#define WINDOW_HEIGHT 600

#define FONT_PATH "../resources/Alegreya-Regular.ttf"

#define NAME_TEXT_MESSAGE "Song name: "
#define TIME_TEXT_MESSAGE "Time played: "

#define DEFAULT_MUSIC_SEEK_STEP 5.f
#define DEFAULT_MUSIC_VOLUME_STEP .1
#define DEFAULT_MUSIC_VOLUME 0.1

#define TEXT_CAP 512
#define SUPPORTED_FORMATS_CAP 6

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
    Color background_color;

    float font_size;
    float font_spacing;

    Font font;

    Seek_Track seek_track;

    Text_Label song_name;
    Text_Label song_time;

    Plug_Music music;
} Plug;

bool is_music(const char*);
bool is_mouse_on_track(const Vector2, Seek_Track);

void get_song_name(const char*, char*, const size_t);

Vector2 center_text(const Vector2);

// < =======  + + + ======= >

void plug_handle_keys(Plug*);
void plug_handle_dropped_files(Plug* plug);

void plug_draw_main_screen(Plug*);

bool plug_load_music(Plug*, const char*);

void plug_reinit(Plug*);

void plug_init_track(Seek_Track*);
void plug_init_song_name(Plug*, Text_Label*);
void plug_init_song_time(Plug*, Text_Label*);

typedef void (*plug_init_t)(Plug* plug);
typedef void (*plug_free_t)(Plug* plug);
typedef void (*plug_frame_t)(Plug* plug);

#endif // PLUG_H
