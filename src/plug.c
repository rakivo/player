#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>

#include <raylib.h>

#include "plug.h"

/* TODO:
    1. If key that regulates volume / seeks is pressed, maybe it's better to
       repeat its action every 0.3 or 0.5 second than user gonna spam that key.
       Convenience above all eventually.

    2. Even smarter shuffle system.
*/

#ifdef _WIN32
#   define DELIM '\\'
#else
#   define DELIM '/'
#endif

#define TEXT_CAP 1024

#define WAITING_MESSAGE   "Drag & Drop Music Here"
#define NAME_TEXT_MESSAGE "Song name: "
#define TIME_TEXT_MESSAGE "Time played: "

#define DEFAULT_MUSIC_VOLUME .3
#define DEFAULT_MUSIC_SEEK_STEP 5.f
#define DEFAULT_MUSIC_VOLUME_STEP .1

#define POPUP_MSG_DURATION 0.5

#define SUPPORTED_FORMATS_CAP 6

#define DA_INIT_CAP 256

#define DA_PUSH(vec, x) do {                                                         \
    assert((vec).count >= 0  && "Count can't be negative");                          \
    if ((vec).count >= (vec).cap) {                                                  \
        (vec).cap = (vec).cap == 0 ? DA_INIT_CAP : (vec).cap*2;                      \
        (vec).songs = realloc((vec).songs, (vec).cap*sizeof(*(vec).songs));          \
        assert((vec).songs != NULL && "Buy more RAM lol");                           \
    }                                                                                \
    (vec).songs[(vec).count++] = (x);                                                \
} while (0)

#define DA_LEN(vec) (sizeof(vec)/sizeof(vec[0]))

#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define MIN(a, b) ((a) < (b) ? (a) : (b))

#define PL_RAND(max) (((size_t) rand() << 32) | rand()) % (max)

#define TEXTURE(name)                                                                \
    Texture_Label name##_t;                                                          \
    bool name##_texture_loaded                                                       \

#define INIT_TEXTURE(texture_, path)                                                 \
    if (!plug->texture_##_texture_loaded) {                                          \
        plug->texture_##_t.texture = LoadTexture(path);                              \
        plug->texture_##_texture_loaded = true;                                      \
    }                                                                                \
    plug->texture_##_t.position = position;                                          \
    plug->texture_##_t.rotation = rotation;                                          \
    plug->texture_##_t.scale = scale;                                                \
    plug->texture_##_t.color = color;

#define UNLOAD_TEXTURE(name)                                                         \
    if (plug->name##_texture_loaded) {                                               \
        UnloadTexture(plug->name##_t.texture);                                       \
        plug->name##_texture_loaded = false;                                         \
    }

#define INIT_CONSTANT_TEXT_LABEL(label_)                                             \
    plug->label_.text_size = MeasureTextEx(plug->font, plug->label_.text,            \
                                           plug->font_size, plug->font_spacing);     \
    plug->label_.text_pos = center_text(plug->label_.text_size);

#define INIT_TEXT_LABEL(label_, msg, margin)                                         \
    if (cpydef) {                                                                    \
        strcpy(plug->label_.text, msg);                                              \
        plug->label_.text_size = MeasureTextEx(plug->font, plug->label_.text,        \
                                               plug->font_size, plug->font_spacing); \
    }                                                                                \
    plug->label_.text_pos = center_text(plug->label_.text_size);                     \
    plug->label_.text_pos.x = plug->label_.text_pos.x / 35;                          \
    plug->label_.text_pos.y = GetScreenHeight() - margin;                            \

#define DRAW_TEXT_EX(name, color) do {                                               \
    DrawTextEx(plug->font,                                                           \
           plug->name.text,                                                          \
           plug->name.text_pos,                                                      \
           plug->font_size,                                                          \
           plug->font_spacing,                                                       \
           color);                                                                   \
} while (0)                                                                          \

#define DRAW_TEXTURE_EX(name)                                                        \
    DrawTextureEx(plug->name##_t.texture,                                            \
                  plug->name##_t.position,                                           \
                  plug->name##_t.rotation,                                           \
                  plug->name##_t.scale,                                              \
                  plug->name##_t.color)

#define DRAW_LINE_EX(name)                                                           \
    DrawLineEx(plug->name.start_pos,                                                 \
               plug->name.end_pos,                                                   \
               plug->name.thickness,                                                 \
               plug->name.color);                                                    \

#define DRAW_RECTANGLE_ROUNDED(name)                                                 \
    DrawRectangleRounded(plug->name.rect,                                            \
               plug->name.roundness,                                                 \
               plug->name.segments,                                                  \
               plug->name.color);                                                    \


#define UPDATE_POPUP_MSG(type)                                                       \
    plug->show_popup_msg = true;                                                     \
    plug->popup_msg_start_time = GetTime();                                          \
    plug->popup_msg_type = type                                                      \

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
bool is_mouse_on_track(Vector2, Seek_Track);
Vector2 center_text(Vector2);
Song new_song(const char*, size_t);
void get_song_name(const char*, char*, size_t);

bool plug_load_music(Song*);
bool plug_play_next_song(void);

Song* plug_get_curr_song(void);
Song* plug_get_nth_song(size_t);

size_t plug_pull_next_song(void);
size_t plug_pull_prev_song(void);

void plug_print_songs(void);
void plug_load_all(void);
void plug_unload_all(void);
void plug_unload_music(void);
void plug_handle_keys(void);
void plug_handle_buttons(void);
void plug_handle_dropped_files(void);
void plug_draw_main_screen(void);
void plug_reinit(void);
void plug_init_track(bool);
void plug_init_textures(void);
void plug_init_text_labels(bool);
void plug_init_constant_text_labels(void);

const char* SUPPORTED_FORMATS[SUPPORTED_FORMATS_CAP] = {".xm", ".wav", ".ogg", ".mp3", ".qoa", ".mod"};

static Plug* plug = NULL;

void plug_init(void)
{
    plug = malloc(sizeof(*plug));
    assert(plug != NULL && "Buy more RAM lol");
    memset(plug, 0, sizeof(*plug));

    plug->background_color = (Color) {24, 24, 24, 255};

    plug->font_size = 50.f;
    plug->font_spacing = 2.f;

    plug_load_all();

    plug_init_textures();
    plug_init_track(true);
    plug_init_text_labels(true);
    plug_init_constant_text_labels();
    plug->app_state = WAITING_FOR_FILE;
    plug->music_volume = DEFAULT_MUSIC_VOLUME;
}

void* plug_pre_reload(void)
{
    plug_unload_all();
    return (void*) plug;
}

void plug_post_reload(void* pplug)
{
    plug = (Plug*) pplug;
    plug_load_all();
}

void plug_load_all(void)
{
    TraceLog(LOG_INFO, "LOADING ALL");
    plug->font = LoadFontEx(FONT_PATH, plug->font_size, 0, 0);
    plug->font_loaded = true;
    GenTextureMipmaps(&plug->font.texture);
    SetTextureFilter(plug->font.texture, TEXTURE_FILTER_BILINEAR);
    plug_init_textures();
    Song* curr_song = plug_get_curr_song();
    TraceLog(LOG_INFO, "LOADING MUSIC STREAM");
    if (curr_song && plug_load_music(curr_song)) {
        SetMusicVolume(plug->curr_music, plug->music_volume);
        SeekMusicStream(plug->curr_music, plug->pl.time_played);
    }
}

void plug_unload_music(void)
{
    TraceLog(LOG_INFO, "UNLOADING MUSIC STREAM");
    StopMusicStream(plug->curr_music);
    plug->music_loaded = false;
    UnloadMusicStream(plug->curr_music);
}

void plug_unload_all(void)
{
    TraceLog(LOG_INFO, "UNLOADING ALL");
    if (IsMusicStreamPlaying(plug->curr_music) && plug->music_loaded) plug_unload_music();
    UNLOAD_TEXTURE(muted);
    UNLOAD_TEXTURE(unmuted);
    UNLOAD_TEXTURE(shuffle);
    UNLOAD_TEXTURE(crossed_shuffle);
    if (plug->font_loaded) {
        UnloadFont(plug->font);
        plug->font_loaded = false;
    }
    TraceLog(LOG_INFO, "UNLOADED ALL SUCCESSFULLY");
}

void plug_free(void)
{
    plug_unload_all();
    free(plug->pl.songs);
    TraceLog(LOG_INFO, "FREED ALLOCATED SONGS");
}

void plug_reinit(void)
{
    plug_init_textures();
    plug_init_track(false);
    plug_init_text_labels(false);
    plug_init_constant_text_labels();
}

void plug_frame(void)
{
    if (IsWindowResized()) plug_reinit();

    plug_handle_dropped_files();

    if (plug->app_state == MAIN_SCREEN) {
        plug_handle_keys();
        plug_handle_buttons();
    }

    if (plug->music_loaded && !plug->music_paused && plug->app_state == MAIN_SCREEN) {
        UpdateMusicStream(plug->curr_music);
        plug->pl.time_played = GetMusicTimePlayed(plug->curr_music);

        if (plug->pl.time_played >= plug->pl.length - 0.035) {
            TraceLog(LOG_INFO, "Song ended, playing next one");
            plug_play_next_song();
        }

        snprintf(plug->song_time.text, TEXT_CAP, "Time played: %.1f / %.1f seconds",
                 plug->pl.time_played, plug->pl.length);

        // Update seek track cursor
        {
            const float position = plug->pl.time_played
                / plug->pl.length
                * (plug->seek_track.end_pos.x
                -  plug->seek_track.start_pos.x);

            plug->seek_track.cursor.rect.x = position + plug->seek_track.start_pos.x;
        }

        plug->pl.time_check = plug->pl.time_played
            / plug->pl.length;

        if (plug->pl.time_check > 1.f) plug->pl.time_check = 1.f;
    }

    BeginDrawing();
        ClearBackground(plug->background_color);
        if (plug->app_state == WAITING_FOR_FILE) DRAW_TEXT_EX(waiting_for_file_msg, RAYWHITE);
        else if (plug->app_state == MAIN_SCREEN) plug_draw_main_screen();
    EndDrawing();
}

void plug_draw_main_screen(void)
{
    DRAW_TEXT_EX(song_name, RAYWHITE);
    DRAW_TEXT_EX(song_time, RAYWHITE);

    if (plug->show_popup_msg) {
        if (GetTime() - plug->popup_msg_start_time < POPUP_MSG_DURATION) {
            switch (plug->popup_msg_type) {
            case SEEK_FORWARD: snprintf(plug->popup_msg.text, TEXT_CAP, "+ %.1f  ", DEFAULT_MUSIC_SEEK_STEP); break;
            case SEEK_BACKWARD: snprintf(plug->popup_msg.text, TEXT_CAP, "- %.1f  ", DEFAULT_MUSIC_SEEK_STEP); break;

            case VOLUME_UP: snprintf(plug->popup_msg.text, TEXT_CAP, "+ %.1f  ", DEFAULT_MUSIC_VOLUME_STEP); break;
            case VOLUME_DOWN: snprintf(plug->popup_msg.text, TEXT_CAP, "- %.1f  ", DEFAULT_MUSIC_VOLUME_STEP); break;

            case NEXT_SONG: strcpy(plug->popup_msg.text, ">"); break;
            case PREV_SONG: strcpy(plug->popup_msg.text, "<"); break;

            case ENABLE_SHUFFLE_MODE: DRAW_TEXTURE_EX(shuffle); break;

            case DISABLE_SHUFFLE_MODE: DRAW_TEXTURE_EX(crossed_shuffle); break;

            case MUTE_MUSIC: DRAW_TEXTURE_EX(muted); break;
            case UNMUTE_MUSIC: DRAW_TEXTURE_EX(unmuted); break;

            default: assert(NULL && "Unexpected case");
            }
            if (plug->popup_msg_type != ENABLE_SHUFFLE_MODE
            &&  plug->popup_msg_type != DISABLE_SHUFFLE_MODE
            &&  plug->popup_msg_type != MUTE_MUSIC
            &&  plug->popup_msg_type != UNMUTE_MUSIC)
                DRAW_TEXT_EX(popup_msg, RAYWHITE);
        } else plug->show_popup_msg = false;
    }

    DRAW_LINE_EX(seek_track);
    DRAW_RECTANGLE_ROUNDED(seek_track.cursor);
}

void plug_handle_dropped_files(void)
{
    if (IsFileDropped()) {
        FilePathList files = LoadDroppedFiles();
        for (size_t i = 0; i < files.count; ++i) {
            if (!is_music(files.paths[i]))
                TraceLog(LOG_ERROR, "Couldn't load music from file: %s", files.paths[i]);
            else {
                const Song song = new_song(files.paths[i], 0);
                DA_PUSH(plug->pl, song);
#ifdef DEBUG
                TraceLog(LOG_INFO, "Pushed into the playlist this one: %s", files.paths[i]);
                TraceLog(LOG_INFO, "Music count in the vm array: %zu\n", plug->pl.count);
#endif
                plug_print_songs();
            }
        }

        if (!plug->music_loaded) plug_load_music(&plug->pl.songs[0]);

#ifdef DEBUG
        TraceLog(LOG_INFO, "Curr: %zu, prev: %zu, count: %zu\n", plug->pl.curr, plug->pl.prev, plug->pl.count);
#endif

        UnloadDroppedFiles(files);
    }
}

void plug_handle_buttons(void)
{
    if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
        Vector2 mouse_pos = GetMousePosition();

        if (is_mouse_on_track(mouse_pos, plug->seek_track) && IsMusicStreamPlaying(plug->curr_music)) {
            plug->seek_track.cursor.rect.x = mouse_pos.x;

            const float position = (mouse_pos.x
                - plug->seek_track.start_pos.x)
                / (plug->seek_track.end_pos.x - plug->seek_track.start_pos.x)
                * plug->pl.length;

            SeekMusicStream(plug->curr_music, position);
        }
    }
}

void plug_handle_keys(void)
{
    if (IsKeyPressed(KEY_SPACE)) {
        plug->music_paused = !plug->music_paused;
        if (plug->music_paused) PauseMusicStream(plug->curr_music);
        else                    ResumeMusicStream(plug->curr_music);
    } else if (IsKeyPressed(KEY_LEFT)) {
        if (IsMusicStreamPlaying(plug->curr_music)) {
            float curr_pos = GetMusicTimePlayed(plug->curr_music);
            float future_pos = MAX(curr_pos - DEFAULT_MUSIC_SEEK_STEP, 0.0);
            SeekMusicStream(plug->curr_music, future_pos);
            UPDATE_POPUP_MSG(SEEK_FORWARD);
        }
    } else if (IsKeyPressed(KEY_RIGHT) && IsMusicStreamPlaying(plug->curr_music)) {
        float curr_pos = GetMusicTimePlayed(plug->curr_music);
        float future_pos = MIN(curr_pos + DEFAULT_MUSIC_SEEK_STEP, GetMusicTimeLength(plug->curr_music));
        SeekMusicStream(plug->curr_music, future_pos);
        UPDATE_POPUP_MSG(SEEK_BACKWARD);
    } else if (IsKeyPressed(KEY_UP) && IsMusicStreamPlaying(plug->curr_music)) {
        UPDATE_POPUP_MSG(VOLUME_UP);
        plug->music_volume = MIN(plug->music_volume + DEFAULT_MUSIC_VOLUME_STEP, 1.f);
        SetMusicVolume(plug->curr_music, plug->music_volume);
    } else if (IsKeyPressed(KEY_DOWN) && IsMusicStreamPlaying(plug->curr_music)) {
        UPDATE_POPUP_MSG(VOLUME_DOWN);
        plug->music_volume = MAX(plug->music_volume - DEFAULT_MUSIC_VOLUME_STEP, 0.f);
        SetMusicVolume(plug->curr_music, plug->music_volume);
    } else if (IsKeyPressed(KEY_N)) {
        UPDATE_POPUP_MSG(NEXT_SONG);

        size_t next_index = plug_pull_next_song();
        Song* song = plug_get_nth_song(next_index);

#ifdef DEBUG
        if (!song) TraceLog(LOG_ERROR, "Next song is NULL, curr: %zu", next_index);
#endif

        if (song && plug_load_music(song)) {
            plug->pl.prev = plug->pl.curr;
            TraceLog(LOG_INFO, "Set curr to: %zu", plug->pl.curr = next_index);
            PlayMusicStream(plug->curr_music);
        }
    } else if (IsKeyPressed(KEY_P)) {
        UPDATE_POPUP_MSG(PREV_SONG);

        size_t next_index = plug_pull_prev_song();

        Song* song = plug_get_nth_song(next_index);

#ifdef DEBUG
        if (!song) TraceLog(LOG_ERROR, "Prev song is NULL, curr: %zu", next_index);
#endif

        if (song && plug_load_music(song)) {
            plug->pl.prev = next_index;
            TraceLog(LOG_INFO, "Set curr to: %zu", plug->pl.curr = next_index);
            PlayMusicStream(plug->curr_music);
        }
    } else if (IsKeyPressed(KEY_S)) {
        plug->shuffle_mode = !plug->shuffle_mode;
        if (plug->shuffle_mode) {
            UPDATE_POPUP_MSG(DISABLE_SHUFFLE_MODE);
            TraceLog(LOG_INFO, "Shuffle mode disabled");
        } else {
            UPDATE_POPUP_MSG(ENABLE_SHUFFLE_MODE);
            TraceLog(LOG_INFO, "Shuffle mode enabled");
        }
    } else if (IsKeyPressed(KEY_M) && IsMusicStreamPlaying(plug->curr_music)) {
        plug->music_muted = !plug->music_muted;
        if (!plug->music_muted) {
            SetMusicVolume(plug->curr_music, plug->music_volume);
            UPDATE_POPUP_MSG(UNMUTE_MUSIC);
            TraceLog(LOG_INFO, "Music has been muted");
        } else {
            SetMusicVolume(plug->curr_music, 0.0);
            UPDATE_POPUP_MSG(MUTE_MUSIC);
            TraceLog(LOG_INFO, "Music has been unmuted");
        }
    }
}

void plug_init_constant_text_labels(void)
{
    strcpy(plug->waiting_for_file_msg.text, WAITING_MESSAGE);
    strcpy(plug->popup_msg.text, "       ");

    INIT_CONSTANT_TEXT_LABEL(waiting_for_file_msg);
    INIT_CONSTANT_TEXT_LABEL(popup_msg);
}

void plug_init_text_labels(bool cpydef)
{
    const int song_name_margin = GetScreenHeight() / 1.01;
    const int song_time_margin = GetScreenHeight() / 1.1;

    INIT_TEXT_LABEL(song_name, NAME_TEXT_MESSAGE, song_name_margin);
    INIT_TEXT_LABEL(song_time, TIME_TEXT_MESSAGE, song_time_margin);
}

void plug_init_textures(void)
{
    const Vector2 position = { // Basically sizes of all of the textures are equal: 256x256
        .x = (GetScreenWidth() - 256 / 2) / 2,
        .y = (GetScreenHeight() - 256 / 2) / 2,
    };
    const float rotation = 0.f;
    const float scale = 0.5;
    const Color color = WHITE;

    INIT_TEXTURE(muted, MUTED_PATH);
    INIT_TEXTURE(unmuted, UNMUTED_PATH);
    INIT_TEXTURE(shuffle, SHUFFLE_PATH);
    INIT_TEXTURE(crossed_shuffle, CROSSED_SHUFFLE_PATH);
}

void plug_init_track(bool cpydef)
{
    plug->seek_track.track_margin_bottom = GetScreenHeight() / 13;
    plug->seek_track.thickness = 5.f;
    plug->seek_track.color = (Color) { 86, 205, 234, 255 };
    plug->seek_track.start_pos = (Vector2) {
        .x = GetScreenWidth() / 20,
        .y = GetScreenHeight() - plug->seek_track.track_margin_bottom
    };
    plug->seek_track.end_pos = (Vector2) {
        .x = GetScreenWidth() - GetScreenWidth() / 20,
        .y = GetScreenHeight() - plug->seek_track.track_margin_bottom
    };
    if (cpydef) {
        plug->seek_track.cursor = (Track_Cursor) {
            .rect = (Rectangle) {
                .x = plug->seek_track.start_pos.x,
                .y = plug->seek_track.start_pos.y - GetScreenHeight() * 0.0082,
                .width = GetScreenWidth() * 0.01,
                .height = GetScreenHeight() * 0.015
            },
            .roundness = 1.f,
            .segments = 30,
            .color = WHITE,
        };
    } else plug->seek_track.cursor.rect = (Rectangle) {
        .y = plug->seek_track.start_pos.y - GetScreenHeight() * 0.0082,
        .width = GetScreenWidth() * 0.01,
        .height = GetScreenHeight() * 0.015,
    };
}

Song* plug_get_curr_song(void)
{
    if (plug->pl.curr >= 0 && plug->pl.curr < plug->pl.count)
        return &plug->pl.songs[plug->pl.curr];
    else return NULL;
}

Song* plug_get_nth_song(size_t n)
{
    if (n >= 0 && n < plug->pl.count)
        return &plug->pl.songs[n];
    else return NULL;
}

size_t plug_pull_next_song(void)
{
    if (plug->shuffle_mode) {
        size_t ret = PL_RAND(plug->pl.count);
        while (ret == plug->pl.prev) ret = PL_RAND(plug->pl.count);
        return ret;
    } else if (!plug->shuffle_mode && plug->pl.curr + 1 >= plug->pl.count)
        return 0;
    else return MIN(plug->pl.curr + 1, plug->pl.count - 1);
}

size_t plug_pull_prev_song(void)
{
    if (plug->shuffle_mode) {
        size_t ret = PL_RAND(plug->pl.count);
        while (ret == plug->pl.prev) ret = PL_RAND(plug->pl.count);
        return ret;
    } else if (plug->pl.curr == 0)
        return plug->pl.count - 1;
    else return MAX(plug->pl.curr - 1, 0);
}

bool plug_play_next_song(void)
{
    plug_print_songs();

    size_t next_index = plug_pull_next_song();

    Song* next_song = plug_get_nth_song(next_index);
    if (next_song) {
        plug_unload_music();

        if (plug_load_music(next_song)) {
            plug->pl.prev = plug->pl.curr;
            plug->pl.curr = next_index;
        } else {
            TraceLog(LOG_ERROR, "Couldn't load music from file: %s", next_song);
            return false;
        }
    }

    return true;
}

bool plug_load_music(Song* song)
{
#ifdef DEBUG
    TraceLog(LOG_INFO, "Passed file format: %s", song->path);
#endif

    Music m = LoadMusicStream(song->path);

    if (m.frameCount != 0) {
        SetMusicVolume(m, plug->music_volume);

        if (IsMusicStreamPlaying(plug->curr_music) && plug->music_loaded)
            plug_unload_music();

        plug->app_state = MAIN_SCREEN;

        plug->pl.length = GetMusicTimeLength(m);

        plug->music_loaded = true;
        plug->music_paused = false;

        char song_name[256];
        get_song_name(song->path, song_name, TEXT_CAP);

        snprintf(plug->song_name.text, TEXT_CAP, "Song name: %s", song_name);

        plug->pl.prev_song = *plug_get_curr_song();
        song->times_played++;

#ifdef DEBUG
        TraceLog(LOG_INFO, "Assigned song_name successfully: %s", plug->song_name.text);
        TraceLog(LOG_INFO, "Previous song: %s", plug->pl.prev_song.path);
#endif

        plug->curr_music = m;
        PlayMusicStream(m);
        return true;
    } else {
        UnloadMusicStream(m);
        return false;
    }
}

void plug_print_songs(void)
{
    for (size_t i = 0; i < plug->pl.count; ++i)
        printf("playlist[%zu] = %s\n", i, plug->pl.songs[i].path);
}

Song new_song(const char* file_path, size_t times_played)
{
    Song new_song;
    new_song.times_played = times_played;

    if (strlen(file_path) > TEXT_CAP)
        TraceLog(LOG_ERROR, "file path length is greater than supported, truncating..");

    strncpy(new_song.path, file_path, TEXT_CAP);

#ifdef DEBUG
    TraceLog(LOG_INFO, "Copied to the path: %s", new_song.path);
#endif

    return new_song;
}

bool is_music(const char* path)
{
    for (size_t i = 0; i < SUPPORTED_FORMATS_CAP; ++i)
        if (IsFileExtension(path, SUPPORTED_FORMATS[i]))
            return true;

    return false;
}

bool is_mouse_on_track(Vector2 mouse_pos, Seek_Track seek_track)
{
    const float track_pad = seek_track.thickness*2;
    return (mouse_pos.x >= seek_track.start_pos.x) && (mouse_pos.x <= seek_track.end_pos.x) &&
           (mouse_pos.y >= (seek_track.start_pos.y - seek_track.thickness - track_pad)) &&
           (mouse_pos.y <= (seek_track.end_pos.y + seek_track.thickness + track_pad));
}

Vector2 center_text(Vector2 text_size)
{
    return (Vector2) {
        .x = (GetScreenWidth() - text_size.x) / 2,
        .y = (GetScreenHeight() - text_size.y) / 2,
    };
}

void get_song_name(const char* input, char* o, size_t o_n)
{
    size_t end = 0;
    size_t n = strlen(input);

    for (size_t i = n - 1; i > 0 && input[i] != DELIM; --i)
        if (end < o_n - 1)
            o[end++] = input[i];

    o[end] = '\0';
    for (size_t i = 0; i < end / 2; i++) {
        char t = o[i];
        o[i] = o[end - i - 1];
        o[end - i - 1] = t;
    }
}
