#ifndef PLAY_H
#define PLAY_H

#include <raylib.h>
#include <stddef.h>
#include <stdbool.h>

bool is_music(const char* path);
void supported_extensions(void);
Vector2 center_text(const Vector2 text_size);
void get_song_name(const char* input, char* output, const size_t output_size);

#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define MIN(a, b) ((a) < (b) ? (a) : (b))

#ifdef _WIN32
#   define DELIM '\\'
#else
#   define DELIM '/'
#endif

#define FONT_PATH "../resources/Alegreya-Regular.ttf"

#define WINDOW_WIDTH 1000
#define WINDOW_HEIGHT 600

#define NAME_TEXT_MESSAGE "Song name: "
#define TIME_TEXT_MESSAGE "Time played: "

#define DEFAULT_MUSIC_SEEK_STEP 5.f
#define DEFAULT_MUSIC_VOLUME_STEP .1
#define DEFAULT_MUSIC_VOLUME 0.1

#define TEXT_CAP 512
#define SUPPORTED_FORMATS_CAP 6

extern const char* SUPPORTED_FORMATS[SUPPORTED_FORMATS_CAP];

#define X_CENTER (GetScreenWidth() / 2)
#define Y_CENTER (GetScreenHeight() / 2)
#define BACKGROUND_COLOR ((Color) {24, 24, 24, 255})

#endif // PLAY_H
