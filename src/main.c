#include "grind_core/grind_core.h"
#include "grind_app/grind_input.h"
#include "grind_app/grind_renderer.h"

#include <SDL2/SDL.h>
#include <stdio.h>
#include <string.h>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#include <emscripten/html5.h>
#endif

typedef struct {
    SDL_Window *window;
    SDL_GLContext gl_ctx;
    GrindWorld world;
    GrindInputState input;
    GrindRenderer renderer;
    uint32_t last_ticks;
    bool mouse_captured;
    int max_frames;
    int frame_count;
} GrindApp;

static void process_events(GrindApp *app) {
    grind_input_begin_frame(&app->input);

    SDL_Event ev;
    while (SDL_PollEvent(&ev)) {
        switch (ev.type) {
            case SDL_QUIT:
                app->input.quit_requested = true;
                break;
            case SDL_WINDOWEVENT:
                if (ev.window.event == SDL_WINDOWEVENT_SIZE_CHANGED ||
                    ev.window.event == SDL_WINDOWEVENT_RESIZED) {
                    grind_input_resize(&app->input, ev.window.data1, ev.window.data2);
                }
                break;
            case SDL_FINGERDOWN:
                grind_input_on_touch_down(&app->input, (int64_t)ev.tfinger.fingerId, ev.tfinger.x, ev.tfinger.y);
                break;
            case SDL_FINGERMOTION:
                grind_input_on_touch_motion(&app->input, (int64_t)ev.tfinger.fingerId,
                                            ev.tfinger.x, ev.tfinger.y, ev.tfinger.dx, ev.tfinger.dy);
                break;
            case SDL_FINGERUP:
                grind_input_on_touch_up(&app->input, (int64_t)ev.tfinger.fingerId, ev.tfinger.x, ev.tfinger.y);
                break;
            case SDL_MOUSEBUTTONDOWN:
                if ((app->input.touch_controls_active || app->input.biz_drawer_open) && ev.button.button == SDL_BUTTON_LEFT) {
                    float nx = (float)ev.button.x / (float)app->input.viewport_w;
                    float ny = (float)ev.button.y / (float)app->input.viewport_h;
                    grind_input_on_touch_down(&app->input, 999, nx, ny);
                } else if (ev.button.button == SDL_BUTTON_RIGHT) {
                    app->mouse_captured = !app->mouse_captured;
                    SDL_SetRelativeMouseMode(app->mouse_captured ? SDL_TRUE : SDL_FALSE);
                }
                break;
            case SDL_MOUSEBUTTONUP:
                if (app->input.touch_controls_active && ev.button.button == SDL_BUTTON_LEFT) {
                    float nx = (float)ev.button.x / (float)app->input.viewport_w;
                    float ny = (float)ev.button.y / (float)app->input.viewport_h;
                    grind_input_on_touch_up(&app->input, 999, nx, ny);
                }
                break;
            case SDL_MOUSEMOTION:
                if (app->input.touch_controls_active && (ev.motion.state & SDL_BUTTON_LMASK)) {
                    float nx = (float)ev.motion.x / (float)app->input.viewport_w;
                    float ny = (float)ev.motion.y / (float)app->input.viewport_h;
                    float dx = (float)ev.motion.xrel / (float)app->input.viewport_w;
                    float dy = (float)ev.motion.yrel / (float)app->input.viewport_h;
                    grind_input_on_touch_motion(&app->input, 999, nx, ny, dx, dy);
                } else if (app->mouse_captured || (ev.motion.state & SDL_BUTTON_RMASK)) {
                    app->input.look_yaw_delta += (float)ev.motion.xrel * 0.0035f;
                    app->input.look_pitch_delta -= (float)ev.motion.yrel * 0.0030f;
                }
                break;
            case SDL_KEYDOWN:
                if (ev.key.repeat) break;
                switch (ev.key.keysym.sym) {
                    case SDLK_ESCAPE:
                        app->input.quit_requested = true;
                        break;
                    case SDLK_e:
                    case SDLK_SPACE:
                        app->input.buttons[TOUCH_BTN_INTERACT].just_pressed = true;
                        break;
                    case SDLK_b:
                        app->input.buttons[TOUCH_BTN_BIZ_MENU].just_pressed = true;
                        break;
                    case SDLK_m:
                    case SDLK_TAB:
                        app->input.buttons[TOUCH_BTN_MAP_VIEW].just_pressed = true;
                        break;
                    case SDLK_r:
                        app->input.buttons[TOUCH_BTN_SLEEP].just_pressed = true;
                        break;
                    case SDLK_1:
                        app->input.tablet_action_selected = 1;
                        break;
                    case SDLK_2:
                        app->input.tablet_action_selected = 2;
                        break;
                    case SDLK_3:
                        app->input.tablet_action_selected = 3;
                        break;
                    case SDLK_4:
                        app->input.tablet_action_selected = 4;
                        break;
                    case SDLK_5:
                        app->input.tablet_action_selected = 5;
                        break;
                    case SDLK_6:
                        app->input.tablet_action_selected = 6;
                        break;
                    default:
                        break;
                }
                break;
            default:
                break;
        }
    }

    /* Keyboard continuous movement axes when virtual joystick isn't held */
    if (!app->input.joy_active) {
        const Uint8 *keys = SDL_GetKeyboardState(NULL);
        float fwd = 0.0f;
        float str = 0.0f;
        if (keys[SDL_SCANCODE_W] || keys[SDL_SCANCODE_UP]) fwd += 1.0f;
        if (keys[SDL_SCANCODE_S] || keys[SDL_SCANCODE_DOWN]) fwd -= 1.0f;
        if (keys[SDL_SCANCODE_D]) str += 1.0f;
        if (keys[SDL_SCANCODE_A]) str -= 1.0f;
        if (keys[SDL_SCANCODE_LEFT]) app->input.look_yaw_delta -= 0.03f;
        if (keys[SDL_SCANCODE_RIGHT]) app->input.look_yaw_delta += 0.03f;
        app->input.move_forward = fwd;
        app->input.move_strafe = str;
    }
}

static void step_frame(void *userdata) {
    GrindApp *app = (GrindApp *)userdata;
    uint32_t now = SDL_GetTicks();
    float dt_sec = (float)(now - app->last_ticks) * 0.001f;
    if (dt_sec <= 0.0f || dt_sec > 0.25f) dt_sec = 0.016f;
    app->last_ticks = now;

    process_events(app);
    grind_input_apply_to_world(&app->input, &app->world, dt_sec);

    /* 1 real second = 8 simulated minutes during interactive play */
    grind_world_tick(&app->world, dt_sec * 8.0f);

    grind_renderer_draw_frame(&app->renderer, &app->world, &app->input);
    SDL_GL_SwapWindow(app->window);

    app->frame_count++;
    if (app->max_frames > 0 && app->frame_count >= app->max_frames) {
        app->input.quit_requested = true;
    }
#ifdef __EMSCRIPTEN__
    if (app->input.quit_requested) {
        emscripten_cancel_main_loop();
    }
#endif
}

int main(int argc, char **argv) {
    bool force_touch = false;
    int win_w = 1280;
    int win_h = 720;
    int max_frames = 0;

    for (int i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "--touch-ui") == 0) {
            force_touch = true;
        } else if (strcmp(argv[i], "--small-screen") == 0) {
            win_w = 412;
            win_h = 840;
        } else if (strcmp(argv[i], "--smoke-frames") == 0 && i + 1 < argc) {
            max_frames = atoi(argv[++i]);
        }
    }

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_EVENTS) != 0) {
        fprintf(stderr, "SDL_Init failed: %s\n", SDL_GetError());
        return 1;
    }

#ifdef __EMSCRIPTEN__
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
#else
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
#endif
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);

    GrindApp app;
    memset(&app, 0, sizeof(app));
    app.max_frames = max_frames;

    app.window = SDL_CreateWindow(
        "The Grind - 3D First-Principles Business & Life Sim",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        win_w, win_h,
        SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_SHOWN
    );
    if (!app.window) {
        fprintf(stderr, "SDL_CreateWindow failed: %s\n", SDL_GetError());
        SDL_Quit();
        return 1;
    }

    app.gl_ctx = SDL_GL_CreateContext(app.window);
    if (!app.gl_ctx) {
        fprintf(stderr, "SDL_GL_CreateContext failed: %s\n", SDL_GetError());
        SDL_DestroyWindow(app.window);
        SDL_Quit();
        return 1;
    }

    SDL_GL_SetSwapInterval(1);
    grind_world_init(&app.world, 0xC0FFEE123456ULL);
    grind_input_init(&app.input, win_w, win_h, force_touch);
    grind_renderer_init(&app.renderer);
    app.last_ticks = SDL_GetTicks();

#ifdef __EMSCRIPTEN__
    emscripten_set_main_loop_arg(step_frame, &app, 0, 1);
#else
    while (!app.input.quit_requested) {
        step_frame(&app);
    }
#endif

    grind_renderer_shutdown(&app.renderer);
    SDL_GL_DeleteContext(app.gl_ctx);
    SDL_DestroyWindow(app.window);
    SDL_Quit();
    return 0;
}
