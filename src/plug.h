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

#define WAITING_MESSAGE   "Drag & Drop Music Here"
#define NAME_TEXT_MESSAGE "Song name: "
#define TIME_TEXT_MESSAGE "Time played: "

#define DEFAULT_MUSIC_SEEK_STEP 5.f
#define DEFAULT_MUSIC_VOLUME_STEP .1
#define DEFAULT_MUSIC_VOLUME .3

#define POPUP_MSG_DURATION 0.5

#define TEXT_CAP 1024

#define SUPPORTED_FORMATS_CAP 6

extern const char* SUPPORTED_FORMATS[SUPPORTED_FORMATS_CAP];

#define DA_INIT_CAP 256

#define DA_PUSH(vec, x) do {                                                     \
    assert((vec).count >= 0  && "Count can't be negative");                      \
    if ((vec).count >= (vec).cap) {                                              \
        (vec).cap = (vec).cap == 0 ? DA_INIT_CAP : (vec).cap*2;                  \
        (vec).songs = realloc((vec).songs, (vec).cap*sizeof(*(vec).songs));      \
        assert((vec).songs != NULL && "Buy more RAM lol");                       \
    }                                                                            \
    (vec).songs[(vec).count++] = (x);                                            \
} while (0)

#define DA_LEN(vec) (sizeof(vec)/sizeof(vec[0]))

#define PL_RAND(max) (((size_t) rand() << 32) | rand()) % (max)

#define TEXTURE(name)           \
    Texture_Label name##_t;     \
    bool name##_texture_loaded  \

#define INIT_TEXTURE(texture_, path)                                             \
    if (!plug->texture_##_texture_loaded) {                                      \
        plug->texture_##_t.texture = LoadTexture(path);                          \
        plug->texture_##_texture_loaded = true;                                  \
    }                                                                            \
    plug->texture_##_t.position = position;                                      \
    plug->texture_##_t.rotation = rotation;                                      \
    plug->texture_##_t.scale = scale;                                            \
    plug->texture_##_t.color = color;

#define INIT_CONSTANT_TEXT_LABEL(label_)                                         \
    plug->label_.text_size = MeasureTextEx(plug->font, plug->label_.text,        \
                                           plug->font_size, plug->font_spacing); \
    plug->label_.text_pos = center_text(plug->label_.text_size);

#define INIT_TEXT_LABEL(label_, msg, margin)                                          \
    if (cpydef) {                                                                     \
        strcpy(plug->label_.text, msg);                                               \
        plug->label_.text_size = MeasureTextEx(plug->font, plug->label_.text,         \
                                               plug->font_size, plug->font_spacing);  \
    }                                                                                 \
    plug->label_.text_pos = center_text(plug->label_.text_size);                      \
    plug->label_.text_pos.x = plug->label_.text_pos.x / 35;                           \
    plug->label_.text_pos.y = GetScreenHeight() - margin;                             \

#define UNLOAD_TEXTURE(name)                   \
    if (plug->name##_texture_loaded) {         \
        UnloadTexture(plug->name##_t.texture); \
        plug->name##_texture_loaded = false;   \
    }

#define DRAW_TEXT_EX(name, color) do {     \
    DrawTextEx(plug->font,                 \
           plug->name.text,                \
           plug->name.text_pos,            \
           plug->font_size,                \
           plug->font_spacing,             \
           color);                         \
} while (0)                                \

#define DRAW_TEXTURE_EX(name)              \
    DrawTextureEx(plug->name##_t.texture,  \
                  plug->name##_t.position, \
                  plug->name##_t.rotation, \
                  plug->name##_t.scale,    \
                  plug->name##_t.color)

#define DRAW_LINE_EX(name)                 \
    DrawLineEx(plug->name.start_pos,       \
               plug->name.end_pos,         \
               plug->name.thickness,       \
               plug->name.color);          \

#define DRAW_RECTANGLE_ROUNDED(name)       \
    DrawRectangleRounded(plug->name.rect,  \
               plug->name.roundness,       \
               plug->name.segments,        \
               plug->name.color);          \

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
    Rectangle rect;

    float roundness;

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
    DISABLE_SHUFFLE_MODE,

    MUTE_MUSIC,
    UNMUTE_MUSIC,
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

    bool font_loaded;

    bool music_muted;
    bool music_loaded;
    bool music_paused;

    bool shuffle_mode;
 
    TEXTURE(muted);
    TEXTURE(unmuted);
    TEXTURE(shuffle);
    TEXTURE(crossed_shuffle);

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

size_t plug_pull_next_song(void);
size_t plug_pull_prev_song(void);

void plug_handle_keys(void);
void plug_handle_buttons(void);
void plug_handle_dropped_files(void);

void plug_draw_main_screen(void);

bool plug_load_music(Song*);

bool plug_play_next_song(void);

void plug_print_songs(void);

Song* plug_get_curr_song(void);
Song* plug_get_nth_song(const size_t);

void plug_load_all(void);
void plug_unload_all(void);
void plug_unload_music(void);

void plug_reinit(void);

void plug_init_textures(void);

void plug_init_text_labels(bool);
void plug_init_constant_text_labels(void);

void plug_init_track(bool);

typedef void  (*plug_init_t)(void);
typedef void  (*plug_free_t)(void);
typedef void  (*plug_frame_t)(void);
typedef Plug* (*plug_pre_reload_t)(void);
typedef void  (*plug_post_reload_t)(Plug*);

#endif // PLUG_H
