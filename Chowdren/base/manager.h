#ifndef CHOWDREN_MANAGER_H
#define CHOWDREN_MANAGER_H

#include "globals.h"
#include "color.h"
#include "fpslimit.h"
#include "include_gl.h"

class Frame;
class Media;

class GameManager
{
public:
    Frame * frame;
    GlobalValues * values;
    GlobalStrings * strings;
    Media * media;
    FPSLimiter fps_limit;
    bool window_created;
    bool fullscreen;
    int off_x, off_y, x_size, y_size;
    int mouse_x, mouse_y;
    GLuint screen_texture;
    GLuint screen_fbo;
    Color fade_color;
    float fade_dir;
    float fade_value;

    GameManager();
    void on_key(int key, bool state);
    void on_mouse(int key, bool state);
    int update();
    void draw();
    void set_frame(int index);
    void set_framerate(int framerate);
    void set_window(bool fullscreen);
    bool is_fullscreen();
    void run();
    void reset_globals();
    void set_fade(const Color & color, float dir);
};

extern GameManager * global_manager;

#endif // CHOWDREN_MANAGER_H
