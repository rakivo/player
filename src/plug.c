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

    plug_init_popup_msg();
    plug_init_track(true);
    plug_init_song_name(true);
    plug_init_song_time(true);
    plug_init_waiting_for_file_msg();

    plug->music_volume = DEFAULT_MUSIC_VOLUME;
    plug->app_state = WAITING_FOR_FILE;
}

Plug* plug_pre_reload(void)
{
    plug_unload_all();
    return plug;
}

void plug_post_reload(Plug* pplug)
{
    plug = pplug;
    plug_load_all();
}

void plug_load_all(void)
{
    TraceLog(LOG_INFO, "LOADING ALL");
    plug->font = LoadFontEx(FONT_PATH, plug->font_size, 0, 0);
    plug->font_loaded = true;
    GenTextureMipmaps(&plug->font.texture);
    SetTextureFilter(plug->font.texture, TEXTURE_FILTER_BILINEAR);
    plug_init_muted_texture();
    plug_init_unmuted_texture();
    plug_init_shuffle_texture();
    plug_init_crossed_shuffle_texture();
    Song* curr_song = plug_get_curr_song();
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
    if (plug->shuffle_texture_loaded) {
        UnloadTexture(plug->shuffle_t.texture);
        plug->shuffle_texture_loaded = false;        
    }
    if (plug->crossed_shuffle_texture_loaded) {
        UnloadTexture(plug->crossed_shuffle_t.texture);
        plug->crossed_shuffle_texture_loaded = false;        
    }
    if (plug->muted_texture_loaded) {
        UnloadTexture(plug->muted_t.texture);
        plug->muted_texture_loaded = false;        
    }
    if (plug->unmuted_texture_loaded) {
        UnloadTexture(plug->unmuted_t.texture);
        plug->unmuted_texture_loaded = false;        
    }
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
    TraceLog(LOG_INFO, "Freed allocated songs");
}

void plug_reinit(void)
{
    plug_init_popup_msg();
    plug_init_track(false);
    plug_init_song_name(false);
    plug_init_song_time(false);
    plug_init_muted_texture();
    plug_init_unmuted_texture();
    plug_init_shuffle_texture();
    plug_init_waiting_for_file_msg();
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
        if (plug->app_state == WAITING_FOR_FILE) plug_draw_waiting_for_file_screen();
        else if (plug->app_state == MAIN_SCREEN) plug_draw_main_screen();
    EndDrawing();
}

void plug_draw_waiting_for_file_screen(void)
{
    DrawTextEx(plug->font,
               plug->waiting_for_file_msg.text,
               plug->waiting_for_file_msg.text_pos,
               plug->font_size,
               plug->font_spacing,
               RAYWHITE);
}

void plug_draw_main_screen(void)
{
    DrawTextEx(plug->font,
               plug->song_name.text,
               plug->song_name.text_pos,
               plug->font_size,
               plug->font_spacing,
               RAYWHITE);

    DrawTextEx(plug->font,
               plug->song_time.text,
               plug->song_time.text_pos,
               plug->font_size,
               plug->font_spacing,
               RAYWHITE);

    if (plug->show_popup_msg) {
        if (GetTime() - plug->popup_msg_start_time < POPUP_MSG_DURATION) {
            switch (plug->popup_msg_type) {
            case SEEK_FORWARD: snprintf(plug->popup_msg.text, TEXT_CAP, "+ %.1f  ", DEFAULT_MUSIC_SEEK_STEP); break;
            case SEEK_BACKWARD: snprintf(plug->popup_msg.text, TEXT_CAP, "- %.1f  ", DEFAULT_MUSIC_SEEK_STEP); break;

            case VOLUME_UP: snprintf(plug->popup_msg.text, TEXT_CAP, "+ %.1f  ", DEFAULT_MUSIC_VOLUME_STEP); break;
            case VOLUME_DOWN: snprintf(plug->popup_msg.text, TEXT_CAP, "- %.1f  ", DEFAULT_MUSIC_VOLUME_STEP); break;

            case NEXT_SONG: strcpy(plug->popup_msg.text, ">"); break;
            case PREV_SONG: strcpy(plug->popup_msg.text, "<"); break;

            case ENABLE_SHUFFLE_MODE:
                DrawTextureEx(plug->shuffle_t.texture,
                              plug->shuffle_t.position,
                              plug->shuffle_t.rotation,
                              plug->shuffle_t.scale,
                              plug->shuffle_t.color); break;

            case DISABLE_SHUFFLE_MODE:
                DrawTextureEx(plug->crossed_shuffle_t.texture,
                              plug->crossed_shuffle_t.position,
                              plug->crossed_shuffle_t.rotation,
                              plug->crossed_shuffle_t.scale,
                              plug->crossed_shuffle_t.color); break;


            case MUTE_MUSIC: DrawTextureEx(plug->muted_t.texture,
                                           plug->muted_t.position,
                                           plug->muted_t.rotation,
                                           plug->muted_t.scale,
                                           plug->muted_t.color);  break;


            case UNMUTE_MUSIC: DrawTextureEx(plug->unmuted_t.texture,
                                             plug->unmuted_t.position,
                                             plug->unmuted_t.rotation,
                                             plug->unmuted_t.scale,
                                             plug->unmuted_t.color);  break;

            default: assert(NULL && "Unexpected case");
            }
            if (plug->popup_msg_type != ENABLE_SHUFFLE_MODE
            &&  plug->popup_msg_type != DISABLE_SHUFFLE_MODE) {
                DrawTextEx(plug->font,
                       plug->popup_msg.text,
                       plug->popup_msg.text_pos,
                       plug->font_size,
                       plug->font_spacing,
                       RAYWHITE);
            }
        } else plug->show_popup_msg = false;
    }

    DrawLineEx(plug->seek_track.start_pos,
               plug->seek_track.end_pos,
               plug->seek_track.thickness,
               plug->seek_track.color);

    DrawRectangleRounded(plug->seek_track.cursor.rect,
                         plug->seek_track.cursor.roundness,
                         plug->seek_track.cursor.segments,
                         plug->seek_track.cursor.color);
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

        for (size_t i = plug->pl.count - 1; i >= 0; --i) {
#ifdef DEBUG
            TraceLog(LOG_INFO, "plug->pl.songs[%zu] file_path: %s", i, plug->pl.songs[i].path);
#endif
            if (plug_load_music(&plug->pl.songs[i])) {
                plug->pl.curr = i;
                break;
            }
        }

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

        if (is_mouse_on_track(mouse_pos, plug->seek_track)) {

#ifdef DEBUG
            TraceLog(LOG_INFO, "Track Start pos x: %f, y: %f", plug->seek_track.start_pos.x, plug->seek_track.start_pos.y);
            TraceLog(LOG_INFO, "Track End pos x: %f, y: %f", plug->seek_track.end_pos.x, plug->seek_track.end_pos.y);
            TraceLog(LOG_INFO, "Mouse clicked on the track at x: %f, y: %f", mouse_pos.x, mouse_pos.y);
#endif

            if (IsMusicStreamPlaying(plug->curr_music)) {
                plug->seek_track.cursor.rect.x = mouse_pos.x;

                const float position = (mouse_pos.x
                    - plug->seek_track.start_pos.x)
                    / (plug->seek_track.end_pos.x - plug->seek_track.start_pos.x)
                    * plug->pl.length;

                SeekMusicStream(plug->curr_music, position);
            }
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
            plug->show_popup_msg = true;
            plug->popup_msg_start_time = GetTime();
            plug->popup_msg_type = SEEK_FORWARD;
        }
    } else if (IsKeyPressed(KEY_RIGHT) && IsMusicStreamPlaying(plug->curr_music)) {
        float curr_pos = GetMusicTimePlayed(plug->curr_music);
        float future_pos = MIN(curr_pos + DEFAULT_MUSIC_SEEK_STEP, GetMusicTimeLength(plug->curr_music));
        SeekMusicStream(plug->curr_music, future_pos);
        plug->show_popup_msg = true;
        plug->popup_msg_start_time = GetTime();
        plug->popup_msg_type = SEEK_BACKWARD;
    } else if (IsKeyPressed(KEY_UP) && IsMusicStreamPlaying(plug->curr_music)) {
        plug->popup_msg_type = VOLUME_UP;
        plug->show_popup_msg = true;
        plug->popup_msg_start_time = GetTime();
        plug->music_volume = MIN(plug->music_volume + DEFAULT_MUSIC_VOLUME_STEP, 1.f);
        SetMusicVolume(plug->curr_music, plug->music_volume);
    } else if (IsKeyPressed(KEY_DOWN)) {
        if (IsMusicStreamPlaying(plug->curr_music)) {
            plug->popup_msg_type = VOLUME_DOWN;
            plug->show_popup_msg = true;
            plug->popup_msg_start_time = GetTime();
            plug->music_volume = MAX(plug->music_volume - DEFAULT_MUSIC_VOLUME_STEP, 0.f);
            SetMusicVolume(plug->curr_music, plug->music_volume);
        }
    } else if (IsKeyPressed(KEY_N)) {
        plug->popup_msg_type = NEXT_SONG;
        plug->show_popup_msg = true;
        plug->popup_msg_start_time = GetTime();

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
        plug->popup_msg_type = PREV_SONG;
        plug->show_popup_msg = true;
        plug->popup_msg_start_time = GetTime();

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
        if (plug->shuffle_mode) {
            plug->popup_msg_type = DISABLE_SHUFFLE_MODE;
            TraceLog(LOG_INFO, "Shuffle mode disabled");
        } else {
            plug->popup_msg_type = ENABLE_SHUFFLE_MODE;
            TraceLog(LOG_INFO, "Shuffle mode enabled");
        }

        plug->shuffle_mode = !plug->shuffle_mode;
        plug->show_popup_msg = true;
        plug->popup_msg_start_time = GetTime();
        strcpy(plug->popup_msg.text, "shuffling mode enabled");
    } else if (IsKeyPressed(KEY_M) && IsMusicStreamPlaying(plug->curr_music)) {
        if (plug->music_muted) {
            SetMusicVolume(plug->curr_music, plug->music_volume);
            plug->popup_msg_type = UNMUTE_MUSIC;
            TraceLog(LOG_INFO, "Music has been muted");
        } else {
            SetMusicVolume(plug->curr_music, 0.0);
            plug->popup_msg_type = MUTE_MUSIC;
            TraceLog(LOG_INFO, "Music has been unmuted");
        }
        plug->show_popup_msg = true;
        plug->popup_msg_start_time = GetTime();
        plug->music_muted = !plug->music_muted;
    }
}

void plug_init_waiting_for_file_msg(void)
{
    strcpy(plug->waiting_for_file_msg.text, WAITING_MESSAGE);
    plug->waiting_for_file_msg.text_size = MeasureTextEx(plug->font,
                                                         plug->waiting_for_file_msg.text,
                                                         plug->font_size, plug->font_spacing);

    plug->waiting_for_file_msg.text_pos = center_text(plug->waiting_for_file_msg.text_size);
}

void plug_init_song_name(bool cpydef)
{
    if (cpydef) {
        strcpy(plug->song_name.text, NAME_TEXT_MESSAGE);
        plug->song_name.text_size = MeasureTextEx(plug->font, plug->song_name.text,
                                                  plug->font_size, plug->font_spacing);
    }

    const int margin_top = GetScreenHeight() / 1.01;

    plug->song_name.text_pos = center_text(plug->song_name.text_size);
    plug->song_name.text_pos.x = plug->song_name.text_pos.x / 35;
    plug->song_name.text_pos.y = GetScreenHeight() - margin_top;
}

void plug_init_song_time(bool cpydef)
{
    if (cpydef) {
        strcpy(plug->song_time.text, TIME_TEXT_MESSAGE);
        plug->song_time.text_size = MeasureTextEx(plug->font, plug->song_time.text,
                                                  plug->font_size, plug->font_spacing);
    }

    const int margin_top = GetScreenHeight() / 1.1;

    plug->song_time.text_pos = center_text(plug->song_time.text_size);
    plug->song_time.text_pos.x = plug->song_time.text_pos.x / 35;
    plug->song_time.text_pos.y = GetScreenHeight() - margin_top;
}

void plug_init_popup_msg(void)
{
    plug->popup_msg.text_size = MeasureTextEx(plug->font, "       ",
                                             plug->font_size, plug->font_spacing);
    plug->popup_msg.text_pos = center_text(plug->popup_msg.text_size);
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

void plug_init_shuffle_texture(void)
{
    if (!plug->shuffle_texture_loaded) {
        plug->shuffle_t.texture = LoadTexture(SHUFFLE_PATH);
        plug->shuffle_texture_loaded = true;
    }
    plug->shuffle_t.position = (Vector2) {
        .x = (GetScreenWidth() - plug->shuffle_t.texture.width / 2) / 2,
        .y = (GetScreenHeight() - plug->shuffle_t.texture.height / 2) / 2,
    };
    plug->shuffle_t.rotation = 0.f;
    plug->shuffle_t.scale = 0.5;
    plug->shuffle_t.color = WHITE;
    plug->shuffle_texture_loaded = true;
    SetTextureFilter(plug->shuffle_t.texture, TEXTURE_FILTER_BILINEAR);
}

void plug_init_crossed_shuffle_texture(void)
{
    if (!plug->crossed_shuffle_texture_loaded) {
        plug->crossed_shuffle_t.texture = LoadTexture(CROSSED_SHUFFLE_PATH);
        plug->crossed_shuffle_texture_loaded = true;
    }
    plug->crossed_shuffle_t.position = (Vector2) {
        .x = (GetScreenWidth() - plug->crossed_shuffle_t.texture.width / 2) / 2,
        .y = (GetScreenHeight() - plug->crossed_shuffle_t.texture.height / 2) / 2,
    };
    plug->crossed_shuffle_t.rotation = 0.f;
    plug->crossed_shuffle_t.scale = 0.5;
    plug->crossed_shuffle_t.color = WHITE;
    SetTextureFilter(plug->crossed_shuffle_t.texture, TEXTURE_FILTER_BILINEAR);
}

void plug_init_muted_texture(void)
{
    if (!plug->muted_texture_loaded) {
        plug->muted_t.texture = LoadTexture(MUTED_PATH);
        plug->muted_texture_loaded = true;
    }
    plug->muted_t.position = (Vector2) {
        .x = (GetScreenWidth() - plug->muted_t.texture.width / 2) / 2,
        .y = (GetScreenHeight() - plug->muted_t.texture.height / 2) / 2,
    };
    plug->muted_t.rotation = 0.f;
    plug->muted_t.scale = 0.5;
    plug->muted_t.color = WHITE;
    SetTextureFilter(plug->muted_t.texture, TEXTURE_FILTER_BILINEAR);
}

void plug_init_unmuted_texture(void)
{
    if (!plug->unmuted_texture_loaded) {
        plug->unmuted_t.texture = LoadTexture(UNMUTED_PATH);
        plug->unmuted_texture_loaded = true;
    }
    plug->unmuted_t.position = (Vector2) {
        .x = (GetScreenWidth() - plug->unmuted_t.texture.width / 2) / 2,
        .y = (GetScreenHeight() - plug->unmuted_t.texture.height / 2) / 2,
    };
    plug->unmuted_t.rotation = 0.f;
    plug->unmuted_t.scale = 0.5;
    plug->unmuted_t.color = WHITE;
    SetTextureFilter(plug->unmuted_t.texture, TEXTURE_FILTER_BILINEAR);
}

Song* plug_get_curr_song(void)
{
    if (plug->pl.curr >= 0 && plug->pl.curr < plug->pl.count)
        return &plug->pl.songs[plug->pl.curr];
    else return NULL;
}

Song* plug_get_nth_song(const size_t n)
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

Song new_song(const char* file_path, const size_t times_played)
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

bool is_mouse_on_track(const Vector2 mouse_pos, Seek_Track seek_track)
{
    const float track_padding = seek_track.thickness * 2;
    return (mouse_pos.x >= seek_track.start_pos.x) && (mouse_pos.x <= seek_track.end_pos.x) &&
           (mouse_pos.y >= (seek_track.start_pos.y - seek_track.thickness - track_padding)) &&
           (mouse_pos.y <= (seek_track.end_pos.y + seek_track.thickness + track_padding));
}

Vector2 center_text(const Vector2 text_size)
{
    return (Vector2) {
        .x = (GetScreenWidth() - text_size.x) / 2,
        .y = (GetScreenHeight() - text_size.y) / 2,
    };
}

void get_song_name(const char* input, char* output, const size_t output_size)
{
    size_t input_len = strlen(input);
    size_t out_len = 0;

    for (size_t i = input_len - 1; i > 0 && input[i] != DELIM; --i)
        if (out_len < output_size - 1)
            output[out_len++] = input[i];

    output[out_len] = '\0';
    for (size_t i = 0; i < out_len / 2; i++) {
        char temp = output[i];
        output[i] = output[out_len - i - 1];
        output[out_len - i - 1] = temp;
    }
}
