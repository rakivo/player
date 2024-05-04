#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <raylib.h>

#include "plug.h"

/* TODO:
    Implement playlist (queue) system.
    Work on color palette in the app.
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

    plug->music.time_played = 0.f;
    plug->music.time_check = 0.f;
}

void plug_frame(Plug* plug)
{
    if (IsWindowResized()) plug_reinit(plug);

    plug_handle_dropped_files(plug);

    if (plug->app_state == MAIN_SCREEN) {
        plug_handle_keys(plug);
        plug_handle_buttons(plug);
    }

    if (plug->music.loaded && plug->app_state == MAIN_SCREEN) {
        UpdateMusicStream(plug->music.music);

        plug->music.time_played = GetMusicTimePlayed(plug->music.music);

        snprintf(plug->song_time.text, TEXT_CAP, "Time played: %.1f / %.1f seconds",
                 plug->music.time_played, plug->music.length);

        // Seek track cursor
        {
            const float position = plug->music.time_played
                / plug->music.length
                * (plug->seek_track.end_pos.x
                 - plug->seek_track.start_pos.x);
    
            plug->seek_track.cursor.center.x = position + plug->seek_track.start_pos.x;
        }

        plug->music.time_check = plug->music.time_played
            / plug->music.length;

        if (plug->music.time_check > 1.f) plug->music.time_check = 1.f;
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
        const char* file_path = files.paths[0];
        if (!plug_load_music(plug, file_path))
            TraceLog(LOG_ERROR, "Couldn't play music from file: %s", file_path);
        
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

            if (IsMusicStreamPlaying(plug->music.music)) {
                plug->seek_track.cursor.center.x = mouse_pos.x;

                const float position = (mouse_pos.x
                    - plug->seek_track.start_pos.x)
                    / (plug->seek_track.end_pos.x - plug->seek_track.start_pos.x)
                    * plug->music.length;

                SeekMusicStream(plug->music.music, position);
            }
        }
    }
}

void plug_handle_keys(Plug* plug)
{
    if (IsKeyPressed(KEY_SPACE)) {
        plug->music.paused = !plug->music.paused;
        if (plug->music.paused) PauseMusicStream(plug->music.music);
        else                          ResumeMusicStream(plug->music.music);
    } else if (IsKeyPressed(KEY_LEFT) && IsMusicStreamPlaying(plug->music.music)) {
        float curr_pos = GetMusicTimePlayed(plug->music.music);
        float future_pos = MAX(curr_pos - DEFAULT_MUSIC_SEEK_STEP, 0.0);
        plug->show_popup_msg = true;
        plug->popup_msg_start_time = GetTime();
        snprintf(plug->popup_msg.text, TEXT_CAP, "- %.1f  ", DEFAULT_MUSIC_SEEK_STEP);
        SeekMusicStream(plug->music.music, future_pos);
    } else if (IsKeyPressed(KEY_RIGHT) && IsMusicStreamPlaying(plug->music.music)) {
        float curr_pos = GetMusicTimePlayed(plug->music.music);
        float future_pos = MIN(curr_pos + DEFAULT_MUSIC_SEEK_STEP, GetMusicTimeLength(plug->music.music));
        plug->show_popup_msg = true;
        plug->popup_msg_start_time = GetTime();
        snprintf(plug->popup_msg.text, TEXT_CAP, "+ %.1f  ", DEFAULT_MUSIC_SEEK_STEP);
        SeekMusicStream(plug->music.music, future_pos);
    } else if (IsKeyPressed(KEY_UP) && IsMusicStreamPlaying(plug->music.music)) {
        plug->show_popup_msg = true;
        plug->popup_msg_start_time = GetTime();
        plug->music.volume = MIN(plug->music.volume + DEFAULT_MUSIC_VOLUME_STEP, 1.f);
        snprintf(plug->popup_msg.text, TEXT_CAP, "+ %.1f  ", DEFAULT_MUSIC_VOLUME_STEP);
        SetMusicVolume(plug->music.music, plug->music.volume);
    } else if (IsKeyPressed(KEY_DOWN) && IsMusicStreamPlaying(plug->music.music)) {
        plug->show_popup_msg = true;
        plug->popup_msg_start_time = GetTime();
        plug->music.volume = MAX(plug->music.volume - DEFAULT_MUSIC_VOLUME_STEP, 0.f);
        snprintf(plug->popup_msg.text, TEXT_CAP, "- %.1f  ", DEFAULT_MUSIC_VOLUME_STEP);
        SetMusicVolume(plug->music.music, plug->music.volume);
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
    }
}

bool plug_load_music(Plug* plug, const char* file_path)
{
    if (!is_music(file_path)) return false;
    if (IsMusicStreamPlaying(plug->music.music)) {
        StopMusicStream(plug->music.music);
        UnloadMusicStream(plug->music.music);
    }

    plug->music.music = LoadMusicStream(file_path);
    SetMusicVolume(plug->music.music, DEFAULT_MUSIC_VOLUME);

    if (plug->music.music.frameCount != 0) {
        plug->app_state = MAIN_SCREEN;

        plug->music.length = GetMusicTimeLength(plug->music.music);
        plug->music.loaded = true;

        char song_name[256];
        get_song_name(file_path, song_name, TEXT_CAP);

        snprintf(plug->song_name.text, TEXT_CAP, "Song name: %s", song_name);
        TraceLog(LOG_INFO, "Assigned song_name successfully: %s", plug->song_name.text);
        PlayMusicStream(plug->music.music);
        return true;
    } else return false;
}

void plug_free(Plug* plug)
{
    UnloadFont(plug->font);
    if (IsMusicStreamPlaying(plug->music.music)) UnloadMusicStream(plug->music.music);
    plug->music.loaded = false;
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
