#ifndef MAIN_H
#define MAIN_H

void supported_extensions();
Color rgba(const char* hex);
bool is_music(const char* path);
Vector2 center_text(Vector2 text_size);
void get_song_name(const char *input, char *output, size_t output_size);

#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define MIN(a, b) ((a) < (b) ? (a) : (b))

#ifdef _WIN32
#   define DELIM '\\'
#else
#   define DELIM '/'
#endif

#define FONT "resources/Alegreya-Regular.ttf"
#define FONT_SIZE 50.f
#define FONT_SPACING 2.f

#define SCREEN_WIDTH 1000
#define SCREEN_HEIGHT 600

#define NAME_TEXT_MESSAGE "Song name: "
#define TIME_TEXT_MESSAGE "Time played: "

#define DEFAULT_MUSIC_STEP 5.0
#define DEFAULT_MUSIC_VOLUME 0.1
#define DEFAULT_MUSIC_STEP 5.0

#define TIME_TEXT_CAP 128
#define SONG_NAME_CAP 256
#define NAME_TEXT_CAP 512
#define SUPPORTED_FORMATS_CAP 6

#define X_CENTER (GetScreenWidth() / 2)
#define Y_CENTER (GetScreenHeight() / 2)
#define BACKGROUND_COLOR (rgba("#181818"))

#endif // MAIN_H
