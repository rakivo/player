#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include <raylib.h>

#include "play.h"

const char *SUPPORTED_FORMATS[SUPPORTED_FORMATS_CAP] = {
    ".wav",
    ".ogg",
    ".mp3",
    ".qoa",
    ".xm",
    ".mod"/*,
    ".flac"*/
};

int main()
{
    SetTargetFPS(30);
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Play");
    InitAudioDevice();
    SetExitKey(KEY_Q);

    Font font = LoadFontEx(FONT, 75, 0, 0);
    const float font_size = (float) font.baseSize;
    GenTextureMipmaps(&font.texture);
    SetTextureFilter(font.texture, TEXTURE_FILTER_BILINEAR);

    char name_text[NAME_TEXT_CAP] = NAME_TEXT_MESSAGE;
    Vector2 name_text_size = MeasureTextEx(font, name_text, font_size, FONT_SPACING);
    Vector2 name_text_pos = center_text(name_text_size);
    name_text_pos.y += 150;
    name_text_pos.x -= 470;

    char time_text[TIME_TEXT_CAP] = TIME_TEXT_MESSAGE;
    Vector2 time_text_size = MeasureTextEx(font, time_text, font_size, FONT_SPACING);
    Vector2 time_text_pos = center_text(time_text_size);
    time_text_pos.y += 230;
    time_text_pos.x -= 470;

    bool pause = false;
    bool music_loaded = false;

    float time_check = 0.f;
    float time_played = 0.f;

    Music music;

    while (!WindowShouldClose()) {
        if (IsFileDropped()) {
            FilePathList files = LoadDroppedFiles();

            if (is_music(files.paths[0])) {
                if (music_loaded) {
                    StopMusicStream(music);
                    UnloadMusicStream(music);
                }

                music = LoadMusicStream(files.paths[0]);
                SetMusicVolume(music, DEFAULT_MUSIC_VOLUME);

                if (music.frameCount != 0) {
                    music_loaded = true;

                    char song_name[SONG_NAME_CAP];
                    collect_from_end(files.paths[0], song_name, sizeof(song_name));
                    snprintf(name_text, NAME_TEXT_CAP, "Song name: %s", song_name);
                    PlayMusicStream(music);
                }
            }
            UnloadDroppedFiles(files);
        }

        if (IsKeyPressed(KEY_SPACE)) {
            pause = !pause;
            if (pause) PauseMusicStream(music);
            else       ResumeMusicStream(music);
        } else if (IsKeyPressed(KEY_LEFT)) {
            if (IsMusicStreamPlaying(music)) {
                float curr_pos = GetMusicTimePlayed(music);
                float future_pos = MAX(curr_pos - DEFAULT_MUSIC_STEP,
                                       0.0);
                SeekMusicStream(music, future_pos);
            }
        } else if (IsKeyPressed(KEY_RIGHT)) {
            if (IsMusicStreamPlaying(music)) {
                float curr_pos = GetMusicTimePlayed(music);
                float future_pos = MIN(curr_pos + DEFAULT_MUSIC_STEP,
                                       GetMusicTimeLength(music));
                SeekMusicStream(music, future_pos);
            }
        }
        
        if (music_loaded) {
            UpdateMusicStream(music);

            time_played = GetMusicTimePlayed(music);
            snprintf(time_text, TIME_TEXT_CAP, "Time played: %.1f seconds", time_played);
    
            time_check = time_played / GetMusicTimeLength(music);
            if (time_check > 1.0f) time_check = 1.0f;
        }

        BeginDrawing();
            ClearBackground(BACKGROUND_COLOR);
            DrawTextEx(font, time_text, time_text_pos, font_size, FONT_SPACING, RAYWHITE);
            DrawTextEx(font, name_text, name_text_pos, font_size, FONT_SPACING, RAYWHITE);
        EndDrawing();
    }

    UnloadMusicStream(music);
    CloseAudioDevice();
    UnloadFont(font);
    CloseWindow();

    return 0;
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

void supported_extensions()
{
    fprintf(stderr, "Supported extensions:\n %s", SUPPORTED_FORMATS[0]);
    for (size_t i = 1; i < SUPPORTED_FORMATS_CAP; ++i)
        fprintf(stderr, ", %s", SUPPORTED_FORMATS[i]);
}

Vector2 center_text(Vector2 text_size)
{
    return (Vector2) {
        .x = (X_CENTER - (text_size.x / 230)),
        .y = (Y_CENTER - (text_size.y / 2))
    };
}

void collect_from_end(const char *input, char *output, size_t output_size)
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
