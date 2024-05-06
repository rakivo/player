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

#define WAITING_MESSAGE   "Drop music file to play music"
#define NAME_TEXT_MESSAGE "Song name: "
#define TIME_TEXT_MESSAGE "Time played: "

#define DEFAULT_MUSIC_SEEK_STEP 5.f
#define DEFAULT_MUSIC_VOLUME_STEP .1
#define DEFAULT_MUSIC_VOLUME .1

#define TEXT_CAP 512
#define SUPPORTED_FORMATS_CAP 6

#define SONG_VOLUME_MSG_DURATION 1.f

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
    int track_margin_sides;

    float thickness;

    Color color;

    Track_Cursor cursor;

    Vector2 start_pos;
    Vector2 end_pos;
} Seek_Track;

#define DA_INIT_CAP 256

#define DA_FOREACH(item, vec)                                                   \
    for(int keep = 1,                                                           \
            count = 0,                                                          \
            size = vec.count;                                                   \
        keep && count != size;                                                  \
        keep = !keep, count++)                                                  \
    for(item = (vec.paths) + count; keep; keep = !keep)

#define DA_PUSH(vec, x) do {                                                    \
    assert((vec).count >= 0  && "Count can't be negative");                     \
    if ((vec).count >= (vec).cap) {                                             \
        (vec).cap = (vec).cap == 0 ? DA_INIT_CAP : (vec).cap*2;                 \
        (vec).paths = realloc((vec).paths, (vec).cap*sizeof(*(vec).paths));     \
        assert((vec).paths != NULL && "Buy more RAM lol");                      \
    }                                                                           \
    strcpy((vec).paths[(vec).count++].str, (x));                                \
} while (0)

#define DA_LEN(vec) (sizeof(vec)/sizeof(vec[0]))

typedef struct {
    char str[TEXT_CAP];
} Str_Wrapper;

typedef struct {
    Str_Wrapper* paths;
    size_t cap;
    size_t count;
} Paths;

typedef struct {
    Paths list;

    size_t curr;

    float length;

    float time_check;
    float time_played;
} Playlist;

enum App_State {
    WAITING_FOR_FILE,
    MAIN_SCREEN
};

typedef struct {
    enum App_State app_state;

    Color background_color;

    float font_size;
    float font_spacing;

    Font font;

    Seek_Track seek_track;

    Text_Label waiting_for_file_msg;
    Text_Label song_name;
    Text_Label song_time;
    Text_Label popup_msg;

    bool show_popup_msg;
    float popup_msg_start_time;

    bool music_loaded;
    bool music_paused;

    Music curr_music;

    float music_volume;

    Playlist pl;
} Plug;

bool is_music(const char*);
bool is_mouse_on_track(const Vector2, Seek_Track);

void get_song_name(const char*, char*, const size_t);

Vector2 center_text(const Vector2);

// < =======  + + + ======= >

void plug_handle_keys(Plug*);
void plug_handle_buttons(Plug*);
void plug_handle_dropped_files(Plug*);

void plug_draw_main_screen(Plug*);
void plug_draw_waiting_for_file_screen(Plug*);

bool plug_load_music(Plug*, const char*);

char* plug_get_curr_music(Plug*);
char* plug_get_nth_music(Plug*, const size_t);

void plug_reinit(Plug*);

void plug_init_popup_msg(Plug*);
void plug_init_track(Plug*, bool);
void plug_init_song_name(Plug*, bool);
void plug_init_song_time(Plug*, bool);
void plug_init_waiting_for_file_msg(Plug*);

typedef void (*plug_init_t)(Plug*);
typedef void (*plug_free_t)(Plug*);
typedef void (*plug_frame_t)(Plug*);

#endif // PLUG_H
