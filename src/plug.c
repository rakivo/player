#include <stdio.h>
#include <assert.h>
#include <string.h>

#include <stdlib.h>
#include <raylib.h>

#include "plug.h"

/* TODO:
    I've just found out that in raylib you gotta unload
    your already loaded music stream(s) to play another one.

    To make playlist system work, i need to change dyn. array
    of Music to dyn. array of const char* to obtain possibility
    of loading (unloading) music stream(s) whenever i need.
*/

const char* SUPPORTED_FORMATS[SUPPORTED_FORMATS_CAP] = {
    ".xm",
    ".wav",
    ".ogg",
    ".mp3",
    ".qoa",
    ".mod"
};

void plug_init(Plug* plug)
{
    plug->background_color = (Color) {24, 24, 24, 255};

    plug->font_size = 50.f;
    plug->font_spacing = 2.f;

    plug->font = LoadFontEx(FONT_PATH, plug->font_size, 0, 0);
    GenTextureMipmaps(&plug->font.texture);
    SetTextureFilter(plug->font.texture, TEXTURE_FILTER_BILINEAR);

    plug_init_waiting_for_file_msg(plug);
    plug_init_track(plug, true);
    plug_init_song_name(plug, true);
    plug_init_song_time(plug, true);
    plug_init_popup_msg(plug);

    plug->app_state = WAITING_FOR_FILE;

    memset(&plug->pl, 0, sizeof(plug->pl));
}

void plug_frame(Plug* plug)
{
    if (IsWindowResized()) plug_reinit(plug);

    plug_handle_dropped_files(plug);

    if (plug->app_state == MAIN_SCREEN) {
        plug_handle_keys(plug);
        plug_handle_buttons(plug);
    }

    if (plug->music_loaded && plug->app_state == MAIN_SCREEN) {
        const Music* m = plug_get_curr_music(plug);
        if (m) {
            UpdateMusicStream(*m);
            plug->pl.time_played = GetMusicTimePlayed(*m);
    
            snprintf(plug->song_time.text, TEXT_CAP, "Time played: %.1f / %.1f seconds",
                     plug->pl.time_played, plug->pl.length);
    
            // Seek track cursor
            {
                const float position = plug->pl.time_played
                    / plug->pl.length
                    * (plug->seek_track.end_pos.x
                    -  plug->seek_track.start_pos.x);
        
                plug->seek_track.cursor.center.x = position + plug->seek_track.start_pos.x;
            }
    
            plug->pl.time_check = plug->pl.time_played
                / plug->pl.length;
    
            if (plug->pl.time_check > 1.f) plug->pl.time_check = 1.f;
        }
    }

    BeginDrawing();
        ClearBackground(plug->background_color);
        if (plug->app_state == WAITING_FOR_FILE) plug_draw_waiting_for_file_screen(plug);
        else if (plug->app_state == MAIN_SCREEN) plug_draw_main_screen(plug);
    EndDrawing();
}

void plug_draw_waiting_for_file_screen(Plug* plug)
{
    DrawTextEx(plug->font,
               plug->waiting_for_file_msg.text,
               plug->waiting_for_file_msg.text_pos,
               plug->font_size,
               plug->font_spacing,
               RAYWHITE);
}

void plug_draw_main_screen(Plug* plug)
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
        if (GetTime() - plug->popup_msg_start_time < SONG_VOLUME_MSG_DURATION) {
            DrawTextEx(plug->font,
                       plug->popup_msg.text,
                       plug->popup_msg.text_pos,
                       plug->font_size,
                       plug->font_spacing,
                       RAYWHITE);
        } else plug->show_popup_msg = false;
    }

    DrawLineEx(plug->seek_track.start_pos,
               plug->seek_track.end_pos,
               plug->seek_track.thickness,
               plug->seek_track.color);
  
    DrawCircleSector(plug->seek_track.cursor.center,
                     plug->seek_track.cursor.radius,
                     plug->seek_track.cursor.start_angle,
                     plug->seek_track.cursor.end_angle,
                     plug->seek_track.cursor.segments,
                     plug->seek_track.cursor.color);
}

void plug_handle_dropped_files(Plug* plug)
{
    if (IsFileDropped()) {
        FilePathList files = LoadDroppedFiles();
        for (size_t i = 0; i < files.count; ++i) {
            Music music = LoadMusicStream(files.paths[i]);

            if (!plug_load_music(plug, &music, files.paths[i]))
                TraceLog(LOG_ERROR, "Couldn't play music from file: %s", files.paths[i]);
            else {
                VM_PUSH(&plug->pl.list, music);
                TraceLog(LOG_INFO, "Pushed into the playlist this one: %s", files.paths[i]);
                printf("Music count in the vm array: %zu\n", plug->pl.list.count);
                VM_FOREACH(Music* i, plug->pl.list)
                    printf("Length of all music: %f\n", GetMusicTimeLength(*i));
            }
        }
        
        UnloadDroppedFiles(files);
    }
}

void plug_handle_buttons(Plug* plug)
{
    if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
        Vector2 mouse_pos = GetMousePosition();

        if (is_mouse_on_track(mouse_pos, plug->seek_track)) {
            TraceLog(LOG_INFO, "Track Start pos x: %f, y: %f", plug->seek_track.start_pos.x, plug->seek_track.start_pos.y);
            TraceLog(LOG_INFO, "Track End pos x: %f, y: %f", plug->seek_track.end_pos.x, plug->seek_track.end_pos.y);
            TraceLog(LOG_INFO, "Mouse clicked on the track at x: %f, y: %f", mouse_pos.x, mouse_pos.y);

            const Music* m = plug_get_curr_music(plug);
            if (m && IsMusicStreamPlaying(*m)) {
                plug->seek_track.cursor.center.x = mouse_pos.x;

                const float position = (mouse_pos.x
                    - plug->seek_track.start_pos.x)
                    / (plug->seek_track.end_pos.x - plug->seek_track.start_pos.x)
                    * plug->pl.length;

                SeekMusicStream(*m, position);
            }
        }
    }
}

void plug_handle_keys(Plug* plug)
{
    if (IsKeyPressed(KEY_SPACE)) {
        plug->music_paused = !plug->music_paused;
        const Music* m = plug_get_curr_music(plug);
        if (m) {
            if (plug->music_paused) PauseMusicStream(*m);
            else                    ResumeMusicStream(*m);
        }
    } else if (IsKeyPressed(KEY_LEFT)) {
        const Music* m = plug_get_curr_music(plug);
        if (m && IsMusicStreamPlaying(*m)) {
            float curr_pos = GetMusicTimePlayed(*m);
            float future_pos = MAX(curr_pos - DEFAULT_MUSIC_SEEK_STEP, 0.0);
            SeekMusicStream(*m, future_pos);
            plug->show_popup_msg = true;
            plug->popup_msg_start_time = GetTime();
            snprintf(plug->popup_msg.text, TEXT_CAP, "- %.1f  ", DEFAULT_MUSIC_SEEK_STEP);
        }
    } else if (IsKeyPressed(KEY_RIGHT)) {
        const Music* m = plug_get_curr_music(plug);
        if (m && IsMusicStreamPlaying(*m)) {
            float curr_pos = GetMusicTimePlayed(*m);
            float future_pos = MIN(curr_pos + DEFAULT_MUSIC_SEEK_STEP, GetMusicTimeLength(*m));
            SeekMusicStream(*m, future_pos);
            plug->show_popup_msg = true;
            plug->popup_msg_start_time = GetTime();
            snprintf(plug->popup_msg.text, TEXT_CAP, "+ %.1f  ", DEFAULT_MUSIC_SEEK_STEP);
        }
    } else if (IsKeyPressed(KEY_UP)) {
        const Music* m = plug_get_curr_music(plug);
        if (m && IsMusicStreamPlaying(*m)) {
            plug->show_popup_msg = true;
            plug->popup_msg_start_time = GetTime();
            plug->music_volume = MIN(plug->music_volume + DEFAULT_MUSIC_VOLUME_STEP, 1.f);
            SetMusicVolume(*m, plug->music_volume);
            snprintf(plug->popup_msg.text, TEXT_CAP, "+ %.1f  ", DEFAULT_MUSIC_VOLUME_STEP);
        }
    } else if (IsKeyPressed(KEY_DOWN)) {
        const Music* m = plug_get_curr_music(plug);
        if (m && IsMusicStreamPlaying(*m)) {
            plug->show_popup_msg = true;
            plug->popup_msg_start_time = GetTime();
            plug->music_volume = MAX(plug->music_volume - DEFAULT_MUSIC_VOLUME_STEP, 0.f);
            SetMusicVolume(*m, plug->music_volume);
            snprintf(plug->popup_msg.text, TEXT_CAP, "- %.1f  ", DEFAULT_MUSIC_VOLUME_STEP);
        }
    } else if (IsKeyPressed(KEY_N)) {
        const Music* m = plug_get_nth_music(plug, plug->pl.curr + 1);
        if (m) {
            if (IsMusicStreamPlaying(*m)) {
                StopMusicStream(*m);
                UnloadMusicStream(*m);
            }
            plug->pl.curr++;
            PlayMusicStream(*m);
        }
    } else if (IsKeyPressed(KEY_P)) {
        const Music* m = plug_get_nth_music(plug, plug->pl.curr - 1);
        const Music* currm = plug_get_curr_music(plug);
        if (m && currm) {
            if (IsMusicStreamPlaying(*currm)) {
                StopMusicStream(*currm);
                UnloadMusicStream(*currm);
            }
            PlayMusicStream(*m);
        }
    }
}

void plug_reinit(Plug* plug)
{
    plug_init_popup_msg(plug);
    plug_init_song_name(plug, false);
    plug_init_song_time(plug, false);
    plug_init_track(plug, false);
}

void plug_init_waiting_for_file_msg(Plug* plug)
{
    strcpy(plug->waiting_for_file_msg.text, WAITING_MESSAGE);
    plug->waiting_for_file_msg.text_size = MeasureTextEx(plug->font,
                                                         plug->waiting_for_file_msg.text,
                                                         plug->font_size, plug->font_spacing);

    plug->waiting_for_file_msg.text_pos = center_text(plug->waiting_for_file_msg.text_size);
}

void plug_init_song_name(Plug* plug, bool cpydef)
{
    if (cpydef) strcpy(plug->song_name.text, NAME_TEXT_MESSAGE);
    plug->song_name.text_size = MeasureTextEx(plug->font, plug->song_name.text,
                                              plug->font_size, plug->font_spacing);
    
    const int margin_top = GetScreenHeight() / 1.01;

    plug->song_name.text_pos = center_text(plug->song_name.text_size);
    plug->song_name.text_pos.x = plug->song_name.text_pos.x / 25;
    plug->song_name.text_pos.y = GetScreenHeight() - margin_top;
}

void plug_init_song_time(Plug* plug, bool cpydef)
{
    if (cpydef) strcpy(plug->song_time.text, TIME_TEXT_MESSAGE);
    plug->song_time.text_size = MeasureTextEx(plug->font, plug->song_time.text,
                                              plug->font_size, plug->font_spacing);
    
    const int margin_top = GetScreenHeight() / 1.1;

    plug->song_time.text_pos = center_text(plug->song_time.text_size);
    plug->song_time.text_pos.x = plug->song_time.text_pos.x / 24;
    plug->song_time.text_pos.y = GetScreenHeight() - margin_top;
}

void plug_init_popup_msg(Plug* plug)
{
    plug->popup_msg.text_size = MeasureTextEx(plug->font, "- 0.1  ",
                                              plug->font_size, plug->font_spacing);

    plug->popup_msg.text_pos = center_text(plug->popup_msg.text_size);
}

void plug_init_track(Plug* plug, bool cpydef)
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
            .center = plug->seek_track.start_pos,
            .start_angle = 0,
            .end_angle = 365,
            .color = WHITE,
            .radius = 6.f,
            .segments = 30
        };
    } else plug->seek_track.cursor.center.y = plug->seek_track.start_pos.y;
}

Music* plug_get_curr_music(Plug* plug)
{
    if (plug->pl.curr >= 0 && plug->pl.curr < plug->pl.list.count) {
        return &plug->pl.list.items[plug->pl.curr];
    } else return NULL;
}

Music* plug_get_nth_music(Plug* plug, const size_t n)
{
    if (n > 0 && n < plug->pl.list.count) {
        return &plug->pl.list.items[n];
    } else return NULL;
}

Music* plug_load_music(Plug* plug, Music* music, const char* file_path)
{
    if (!is_music(file_path)) return NULL;

    SetMusicVolume(*music, DEFAULT_MUSIC_VOLUME);

    if ((*music).frameCount != 0) {
        const Music* m = plug_get_curr_music(plug);

        if (m && IsMusicStreamPlaying(*m)) {
            plug->pl.curr++;
            StopMusicStream(*m);
            UnloadMusicStream(*m);
        }

        plug->app_state = MAIN_SCREEN;

        plug->pl.length = GetMusicTimeLength((*music));
        plug->music_loaded = true;
        plug->music_paused = false;

        char song_name[256];
        get_song_name(file_path, song_name, TEXT_CAP);

        snprintf(plug->song_name.text, TEXT_CAP, "Song name: %s", song_name);
        TraceLog(LOG_INFO, "Assigned song_name successfully: %s", plug->song_name.text);
        PlayMusicStream(*music);
        return music;
    } else return NULL;
}

void plug_free(Plug* plug)
{
    UnloadFont(plug->font);
    const Music* m = plug_get_curr_music(plug);
    if (m && IsMusicStreamPlaying(*m)) UnloadMusicStream(*m);
    free(plug->pl.list.items);
    plug->music_loaded = false;
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
        .x = ((GetScreenWidth() / 2) - (text_size.x / 2)),
        .y = ((GetScreenHeight() / 2) - (text_size.y / 2))
    };
}
