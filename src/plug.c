#include <stdio.h>
#include <assert.h>
#include <string.h>

#include <stdlib.h>
#include <raylib.h>

#include "plug.h"

/* TODO:
    gonna think about to do tomorrow (2:36:00 AM), but I'm finally implemented playlist system.
    n -> next song, p -> prev song
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
    memset(plug, 0, sizeof(*plug));

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
        UpdateMusicStream(plug->curr_music);
        plug->pl.time_played = GetMusicTimePlayed(plug->curr_music);

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
            if (!is_music(files.paths[i]))
                TraceLog(LOG_ERROR, "Couldn't load music from file: %s", files.paths[i]);
            else {
                DA_PUSH(plug->pl.list, files.paths[i]);
                TraceLog(LOG_INFO, "Pushed into the playlist this one: %s", files.paths[i]);
                printf("Music count in the vm array: %zu\n", plug->pl.list.count);
                for (size_t i = 0; i < plug->pl.list.count; ++i)
                    printf("Names of music in playlist: %s\n", plug->pl.list.paths[i].str);
            }
        }

        printf("Curr: %zu, count: %zu\n", plug->pl.curr, plug->pl.list.count);
        if (!plug->music_loaded)
            for (size_t i = 0; i < plug->pl.list.count; ++i)
                if (plug_load_music(plug, plug->pl.list.paths[i].str)) break;
        
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

            if (IsMusicStreamPlaying(plug->curr_music)) {
                plug->seek_track.cursor.center.x = mouse_pos.x;

                const float position = (mouse_pos.x
                    - plug->seek_track.start_pos.x)
                    / (plug->seek_track.end_pos.x - plug->seek_track.start_pos.x)
                    * plug->pl.length;

                SeekMusicStream(plug->curr_music, position);
            }
        }
    }
}

void plug_handle_keys(Plug* plug)
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
            snprintf(plug->popup_msg.text, TEXT_CAP, "- %.1f  ", DEFAULT_MUSIC_SEEK_STEP);
        }
    } else if (IsKeyPressed(KEY_RIGHT)) {
        if (IsMusicStreamPlaying(plug->curr_music)) {
            float curr_pos = GetMusicTimePlayed(plug->curr_music);
            float future_pos = MIN(curr_pos + DEFAULT_MUSIC_SEEK_STEP, GetMusicTimeLength(plug->curr_music));
            SeekMusicStream(plug->curr_music, future_pos);
            plug->show_popup_msg = true;
            plug->popup_msg_start_time = GetTime();
            snprintf(plug->popup_msg.text, TEXT_CAP, "+ %.1f  ", DEFAULT_MUSIC_SEEK_STEP);
        }
    } else if (IsKeyPressed(KEY_UP)) {
        if (IsMusicStreamPlaying(plug->curr_music)) {
            plug->show_popup_msg = true;
            plug->popup_msg_start_time = GetTime();
            plug->music_volume = MIN(plug->music_volume + DEFAULT_MUSIC_VOLUME_STEP, 1.f);
            SetMusicVolume(plug->curr_music, plug->music_volume);
            snprintf(plug->popup_msg.text, TEXT_CAP, "+ %.1f  ", DEFAULT_MUSIC_VOLUME_STEP);
        }
    } else if (IsKeyPressed(KEY_DOWN)) {
        if (IsMusicStreamPlaying(plug->curr_music)) {
            plug->show_popup_msg = true;
            plug->popup_msg_start_time = GetTime();
            plug->music_volume = MAX(plug->music_volume - DEFAULT_MUSIC_VOLUME_STEP, 0.f);
            SetMusicVolume(plug->curr_music, plug->music_volume);
            snprintf(plug->popup_msg.text, TEXT_CAP, "- %.1f  ", DEFAULT_MUSIC_VOLUME_STEP);
        }
    } else if (IsKeyPressed(KEY_N)) {
        const char* file_path = plug_get_nth_music(plug, plug->pl.curr + 1);
        if (file_path && plug_load_music(plug, file_path)) {
            TraceLog(LOG_INFO, "Increased curr: %zu", plug->pl.curr++);
            PlayMusicStream(plug->curr_music);
        }
    } else if (IsKeyPressed(KEY_P)) {
        for (size_t i = 0; i < plug->pl.list.count; ++i) printf("list[%zu] = %s\n", i, plug->pl.list.paths[i].str);
        const char* file_path = plug_get_nth_music(plug, plug->pl.curr - 1);
        if (file_path && plug_load_music(plug, file_path)) {
            TraceLog(LOG_INFO, "Decreased curr: %zu", plug->pl.curr--);
            PlayMusicStream(plug->curr_music);
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

char* plug_get_curr_music(Plug* plug)
{
    if (plug->pl.curr >= 0 && plug->pl.curr < plug->pl.list.count)
         return plug->pl.list.paths[plug->pl.curr].str;
    else return NULL;
}

char* plug_get_nth_music(Plug* plug, const size_t n)
{
    if (n >= 0 && n < plug->pl.list.count)
         return plug->pl.list.paths[n].str;
    else return NULL;
}

bool plug_load_music(Plug* plug, const char* file_path)
{
    if (!is_music(file_path)) return false;

    Music m = LoadMusicStream(file_path);

    if (m.frameCount != 0) {
        SetMusicVolume(m, DEFAULT_MUSIC_VOLUME);

        if (IsMusicStreamPlaying(plug->curr_music)) {
            StopMusicStream(plug->curr_music);
            UnloadMusicStream(plug->curr_music);
        }

        plug->app_state = MAIN_SCREEN;
        plug->pl.length = GetMusicTimeLength(m);
        plug->music_loaded = true;
        plug->music_paused = false;

        char song_name[256];
        get_song_name(file_path, song_name, TEXT_CAP);

        snprintf(plug->song_name.text, TEXT_CAP, "Song name: %s", song_name);
        TraceLog(LOG_INFO, "Assigned song_name successfully: %s", plug->song_name.text);
        plug->curr_music = m;
        PlayMusicStream(m);
        return true;
    } else {
        UnloadMusicStream(m);
        return false;
    }
}

void plug_free(Plug* plug)
{
    UnloadFont(plug->font);
    if (IsMusicStreamPlaying(plug->curr_music)) UnloadMusicStream(plug->curr_music);
    free(plug->pl.list.paths);
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
