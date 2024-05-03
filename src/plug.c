#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <raylib.h>

#include "plug.h"
#include "main.h"

const char* SUPPORTED_FORMATS[SUPPORTED_FORMATS_CAP] = {
    ".wav",
    ".ogg",
    ".mp3",
    ".qoa",
    ".xm",
    ".mod"/*,
    ".flac"*/
};

void plug_init(Plug* plug)
{
    plug->font_size = 50.f;
    plug->font_spacing = 2.f;

    plug->font = LoadFontEx(FONT_PATH, plug->font_size, 0, 0);
    GenTextureMipmaps(&plug->font.texture);
    SetTextureFilter(plug->font.texture, TEXTURE_FILTER_BILINEAR);

    plug_init_track(&plug->seek_track);
    plug_init_song_name(plug, &plug->song_name);
    plug_init_song_time(plug, &plug->song_time);

    plug->music.time_music_played = 0.f;
    plug->music.time_music_check = 0.f;

    plug->music.music_paused = false;
    plug->music.music_loaded = false;

    plug->music.music = (Music) {0};
}

void plug_frame(Plug* plug)
{
    if (IsWindowResized()) plug_reinit(plug);

    if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
        Vector2 mouse_pos = GetMousePosition();

        if (is_mouse_on_track(mouse_pos, plug->seek_track)) {
            TraceLog(LOG_INFO, "Track Start pos x: %f, y: %f", plug->seek_track.start_pos.x, plug->seek_track.start_pos.y);
            TraceLog(LOG_INFO, "Track End pos x: %f, y: %f", plug->seek_track.end_pos.x, plug->seek_track.end_pos.y);
            TraceLog(LOG_INFO, "Mouse clicked on the track at x: %f, y: %f", mouse_pos.x, mouse_pos.y);
        }
    }

    if (IsFileDropped()) {
        FilePathList files = LoadDroppedFiles();
        if (is_music(files.paths[0])) plug_load_music(plug, files.paths[0]);
        UnloadDroppedFiles(files);
    }

    if (IsKeyPressed(KEY_SPACE)) {
        plug->music.music_paused = !plug->music.music_paused;
        if (plug->music.music_paused) PauseMusicStream(plug->music.music);
        else                               ResumeMusicStream(plug->music.music);
    } else if (IsKeyPressed(KEY_LEFT) && IsMusicStreamPlaying(plug->music.music)) {
        float curr_pos = GetMusicTimePlayed(plug->music.music);
        float future_pos = MAX(curr_pos - DEFAULT_MUSIC_SEEK_STEP, 0.0);
        SeekMusicStream(plug->music.music, future_pos);
    } else if (IsKeyPressed(KEY_RIGHT) && IsMusicStreamPlaying(plug->music.music)) {
        float curr_pos = GetMusicTimePlayed(plug->music.music);
        float future_pos = MIN(curr_pos + DEFAULT_MUSIC_SEEK_STEP, GetMusicTimeLength(plug->music.music));
        SeekMusicStream(plug->music.music, future_pos);
    } else if (IsKeyPressed(KEY_UP) && IsMusicStreamPlaying(plug->music.music)) {
        float future_volume = MIN(plug->music.music_volume + DEFAULT_MUSIC_VOLUME_STEP, 1.f);
        SetMusicVolume(plug->music.music, future_volume);
    } else if (IsKeyPressed(KEY_DOWN) && IsMusicStreamPlaying(plug->music.music)) {
        float future_volume = MAX(plug->music.music_volume - DEFAULT_MUSIC_VOLUME_STEP, 0.f);
        SetMusicVolume(plug->music.music, future_volume);
    }
    
    if (plug->music.music_loaded) {
        UpdateMusicStream(plug->music.music);
        plug->music.time_music_played = GetMusicTimePlayed(plug->music.music);
        snprintf(plug->song_time.text, TEXT_CAP, "Time played: %.1f seconds", plug->music.time_music_played);

        plug->music.time_music_check = plug->music.time_music_played /
                                            GetMusicTimeLength(plug->music.music);

        if (plug->music.time_music_check > 1.0f) plug->music.time_music_check = 1.0f;
    }

    BeginDrawing();
        ClearBackground(BACKGROUND_COLOR);
        DrawTextEx(plug->font, plug->song_name.text,
                   plug->song_name.text_pos, plug->font_size,
                   plug->font_spacing, RAYWHITE);

        DrawTextEx(plug->font, plug->song_time.text,
                   plug->song_time.text_pos, plug->font_size,
                   plug->font_spacing, RAYWHITE);

        DrawLineEx(plug->seek_track.start_pos,
                   plug->seek_track.end_pos,
                   plug->seek_track.track_thickness,
                   plug->seek_track.track_color);

        DrawCircleSector(plug->seek_track.track_cursor.center,
                         plug->seek_track.track_cursor.radius,
                         plug->seek_track.track_cursor.start_angle,
                         plug->seek_track.track_cursor.end_angle,
                         plug->seek_track.track_cursor.segments,
                         plug->seek_track.track_cursor.color);
    EndDrawing();
}

void plug_reinit(Plug* plug)
{
    plug_init_song_name(plug, &plug->song_name);
    plug_init_song_time(plug, &plug->song_time);
    plug_init_track(&plug->seek_track);

    TraceLog(LOG_INFO, "Plug reinitialized successfully");;
}

void plug_init_song_name(Plug* plug, Text_Label* song_name)
{
    strcpy(song_name->text, NAME_TEXT_MESSAGE);
    song_name->text_size = MeasureTextEx(plug->font, song_name->text, plug->font_size, plug->font_spacing);
    
    const int margin_bottom = GetScreenHeight() / 4;

    song_name->text_pos = center_text(song_name->text_size);
    song_name->text_pos.y = GetScreenHeight() - margin_bottom;
    song_name->text_pos.x /= 15;
}

void plug_init_song_time(Plug* plug, Text_Label* song_time)
{
    strcpy(song_time->text, TIME_TEXT_MESSAGE);
    song_time->text_size = MeasureTextEx(plug->font, song_time->text, plug->font_size, plug->font_spacing);

    const int margin_bottom = GetScreenHeight() / 5.8;

    song_time->text_pos = center_text(song_time->text_size);
    song_time->text_pos.y = GetScreenHeight() - margin_bottom;
    song_time->text_pos.x /= 15;
}

void plug_init_track(Seek_Track* seek_track)
{
    seek_track->track_margin_bottom = GetScreenHeight() / 15;
    seek_track->track_thickness = 5.f;
    seek_track->track_color = RED;
    seek_track->start_pos = (Vector2) {
        .x = 0,
        .y = GetScreenHeight() - seek_track->track_margin_bottom
    };
    seek_track->end_pos = (Vector2) {
        .x = GetScreenWidth(),
        .y = GetScreenHeight() - seek_track->track_margin_bottom
    };
    seek_track->track_cursor = (Track_Cursor) {
        .center = seek_track->start_pos,
        .start_angle = 0,
        .end_angle = 365,
        .color = YELLOW,
        .radius = 6.f,
        .segments = 28
    };
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
        plug->music.music_loaded = true;

        char song_name[256];
        get_song_name(file_path, song_name, TEXT_CAP);

        TraceLog(LOG_INFO, "SONG_NAME: %s", song_name);
        snprintf(plug->song_name.text, TEXT_CAP, "Song name: %s", song_name);

        TraceLog(LOG_INFO, "Assigned song_name successfully");
        PlayMusicStream(plug->music.music);

        return true;
    } else return false;
}

void plug_free(Plug* plug)
{
    UnloadFont(plug->font);
    if (IsMusicStreamPlaying(plug->music.music)) UnloadMusicStream(plug->music.music);
    plug->music.music_loaded = false;
}

void get_song_name(const char* input, char* output, const size_t output_size)
{
    size_t input_len = strlen(input);
    size_t out_len = 0;

    for (size_t i = input_len - 1; i > 0 && input[i] != DELIM; --i) {
        if (out_len < output_size - 1) {
            output[out_len] = input[i];
            out_len++;
        }
    }

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
    const float track_padding = seek_track.track_thickness * 2;

    return (mouse_pos.x >= seek_track.start_pos.x) && (mouse_pos.x <= seek_track.end_pos.x) &&
           (mouse_pos.y >= (seek_track.start_pos.y - seek_track.track_thickness - track_padding)) && 
           (mouse_pos.y <= (seek_track.end_pos.y + seek_track.track_thickness + track_padding));
}

Vector2 center_text(const Vector2 text_size)
{
    return (Vector2) {
        .x = (X_CENTER - (text_size.x / 2)),
        .y = (Y_CENTER - (text_size.y / 2))
    };
}

void supported_extensions(void)
{
    fprintf(stderr, "Supported extensions:\n %s", SUPPORTED_FORMATS[0]);
    for (size_t i = 1; i < SUPPORTED_FORMATS_CAP; ++i)
        fprintf(stderr, ", %s", SUPPORTED_FORMATS[i]);
}
