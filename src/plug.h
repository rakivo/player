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

#define SHUFFLE_PATH "../resources/shuffle.png"
#define CROSSED_SHUFFLE_PATH "../resources/crossed_shuffle.png"
#define FONT_PATH "../resources/Alegreya-Regular.ttf"

#define WAITING_MESSAGE   "Drag & Drop Music Here"
#define NAME_TEXT_MESSAGE "Song name: "
#define TIME_TEXT_MESSAGE "Time played: "

#define DEFAULT_MUSIC_SEEK_STEP 5.f
#define DEFAULT_MUSIC_VOLUME_STEP .1
#define DEFAULT_MUSIC_VOLUME .1

#define POPUP_MSG_DURATION 0.5

#define TEXT_CAP 1024

#define SUPPORTED_FORMATS_CAP 6

extern const char* SUPPORTED_FORMATS[SUPPORTED_FORMATS_CAP];

#define DA_INIT_CAP 256

#define DA_PUSH(vec, x) do {                                                    \
    assert((vec).count >= 0  && "Count can't be negative");                     \
    if ((vec).count >= (vec).cap) {                                             \
        (vec).cap = (vec).cap == 0 ? DA_INIT_CAP : (vec).cap*2;                 \
        (vec).songs = realloc((vec).songs, (vec).cap*sizeof(*(vec).songs));     \
        assert((vec).songs != NULL && "Buy more RAM lol");                      \
    }                                                                           \
    (vec).songs[(vec).count++] = (x);                                           \
} while (0)

#define DA_LEN(vec) (sizeof(vec)/sizeof(vec[0]))

typedef struct {
    char text[TEXT_CAP];
    Vector2 text_size;
    Vector2 text_pos;
} Text_Label;

typedef struct {
    Texture2D texture;

    Vector2 position;

    float rotation;
    float scale;

    Color color;
} Texture_Label;

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

typedef struct {
    char path[TEXT_CAP];

    size_t times_played;
    // ...
} Song;

typedef struct {
    Song* songs;

    size_t cap;
    size_t curr;
    size_t prev;
    size_t count;

    Song prev_song;

    float length;

    float time_check;
    float time_played;
} Playlist;

enum App_State {
    WAITING_FOR_FILE,
    MAIN_SCREEN
};

enum Popup_Msg {
    SEEK_FORWARD,
    SEEK_BACKWARD,

    VOLUME_UP,
    VOLUME_DOWN,

    NEXT_SONG,
    PREV_SONG,
    
    ENABLE_SHUFFLE_MODE,
    DISABLE_SHUFFLE_MODE
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

    enum Popup_Msg popup_msg_type;

    bool music_loaded;
    bool music_paused;

    bool shuffle_mode;
    bool shuffle_texture_loaded;
    bool crossed_shuffle_texture_loaded;

    Texture_Label shuffle_t;
    Texture_Label crossed_shuffle_t;

    Music curr_music;

    float music_volume;

    Playlist pl;
} Plug;

bool is_music(const char*);
bool is_mouse_on_track(const Vector2, Seek_Track);

void get_song_name(const char*, char*, const size_t);

Song new_song(const char*, const size_t);

Vector2 center_text(const Vector2);

// < =======  + + + ======= >

size_t plug_pull_next_song(Plug* plug);

void plug_handle_keys(Plug*);
void plug_handle_buttons(Plug*);
void plug_handle_dropped_files(Plug*);

void plug_draw_main_screen(Plug*);
void plug_draw_waiting_for_file_screen(Plug*);

bool plug_load_music(Plug*, Song*);

bool plug_next_song(Plug*);

void plug_print_songs(Plug*);

Song* plug_get_curr_song(Plug*);
Song* plug_get_nth_song(Plug*, const size_t);

void plug_reinit(Plug*);

void plug_init_shuffle_texture(Plug*);
void plug_init_crossed_shuffle_texture(Plug*q);

void plug_init_popup_msg(Plug*);
void plug_init_track(Plug*, bool);
void plug_init_song_name(Plug*, bool);
void plug_init_song_time(Plug*, bool);
void plug_init_waiting_for_file_msg(Plug*);

typedef void (*plug_init_t)(Plug*);
typedef void (*plug_free_t)(Plug*);
typedef void (*plug_frame_t)(Plug*);

#endif // PLUG_H
