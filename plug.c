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

    plug->name_text_size = MeasureTextEx(plug->font, plug->name_text, plug->font_size, plug->font_spacing);
    plug->name_text_pos = center_text(plug->name_text_size);
    plug->name_text_pos.y += 150;
    plug->name_text_pos.x -= 450;

    plug->time_text_size = MeasureTextEx(plug->font, plug->time_text, plug->font_size, plug->font_spacing);
    plug->time_text_pos = center_text(plug->time_text_size);
    plug->time_text_pos.y += 230;
    plug->time_text_pos.x -= 450;

    plug->time_played = 0.f;
    plug->time_check = 0.f;

    strcpy(plug->name_text, NAME_TEXT_MESSAGE);
    strcpy(plug->time_text, TIME_TEXT_MESSAGE);

    plug->pause = false;
    plug->music_loaded = false;

    plug->music = (Music) {0};
}

void plug_update(Plug* plug)
{
    printf("`plug_update` called in file: %s\n", __FILE__);
    plug->font_size = 50.f;
    printf("`plug_update` successed\n");
}

void plug_free(Plug* plug)
{
    UnloadFont(plug->font);
    if (IsMusicStreamPlaying(plug->music)) UnloadMusicStream(plug->music);
    plug->music_loaded = false;
}

void plug_frame(Plug* plug)
{
    if (IsFileDropped()) {
        FilePathList files = LoadDroppedFiles();
        if (is_music(files.paths[0])) {
            if (plug->music_loaded) {
                StopMusicStream(plug->music);
                UnloadMusicStream(plug->music);
            }

            plug->music = LoadMusicStream(files.paths[0]);
            SetMusicVolume(plug->music, DEFAULT_MUSIC_VOLUME);

            if (plug->music.frameCount != 0) {
                plug->music_loaded = true;

                char song_name[SONG_NAME_CAP];
                get_song_name(files.paths[0], song_name, SONG_NAME_CAP);
                snprintf(plug->name_text, NAME_TEXT_CAP, "Song name: %s", song_name);
                PlayMusicStream(plug->music);
            }
        }
        UnloadDroppedFiles(files);
    }

    if (IsKeyPressed(KEY_SPACE)) {
        plug->pause = !plug->pause;
        if (plug->pause) PauseMusicStream(plug->music);
        else       ResumeMusicStream(plug->music);
    } else if (IsKeyPressed(KEY_LEFT) && IsMusicStreamPlaying(plug->music)) {
        float curr_pos = GetMusicTimePlayed(plug->music);
        float future_pos = MAX(curr_pos - DEFAULT_MUSIC_STEP,
                               0.0);
        SeekMusicStream(plug->music, future_pos);
    } else if (IsKeyPressed(KEY_RIGHT) && IsMusicStreamPlaying(plug->music)) {
        float curr_pos = GetMusicTimePlayed(plug->music);
        float future_pos = MIN(curr_pos + DEFAULT_MUSIC_STEP,
                               GetMusicTimeLength(plug->music));
        SeekMusicStream(plug->music, future_pos);
    }
    
    if (plug->music_loaded) {
        UpdateMusicStream(plug->music);
        plug->time_played = GetMusicTimePlayed(plug->music);
        snprintf(plug->time_text, TIME_TEXT_CAP, "Time played: %.1f seconds", plug->time_played);

        plug->time_check = plug->time_played / GetMusicTimeLength(plug->music);
        if (plug->time_check > 1.0f) plug->time_check = 1.0f;
    }

    BeginDrawing();
        ClearBackground(BACKGROUND_COLOR);
        DrawTextEx(plug->font, plug->time_text, plug->time_text_pos, plug->font_size, plug->font_spacing, RAYWHITE);
        DrawTextEx(plug->font, plug->name_text, plug->name_text_pos, plug->font_size, plug->font_spacing, RAYWHITE);
    EndDrawing();
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

Color rgba(const char* hex)
{
    Color ret = {0, 0, 0, 255};

    ret.r = (strtol(hex + 1, NULL, 16) >> 16) & 0xFF;
    ret.g = (strtol(hex + 1, NULL, 16) >> 8)  & 0xFF;
    ret.b = (strtol(hex + 1, NULL, 16) >> 0)  & 0xFF;

    return ret;
}

bool is_music(const char* path)
{
    for (size_t i = 0; i < SUPPORTED_FORMATS_CAP; ++i)
        if (IsFileExtension(path, SUPPORTED_FORMATS[i]))
            return true;

    return false;
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
