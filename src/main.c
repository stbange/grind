#include "grind_core/grind_core.h"
#include "grind_app/grind_input.h"
#include "grind_app/grind_renderer.h"

#include <SDL2/SDL.h>
#include <stdio.h>
#include <string.h>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#include <emscripten/html5.h>
EM_JS(void, grind_web_set_tablet_open, (int open), {
    Module.tabletOpen = !!open;
})
#endif

typedef struct {
    SDL_Window *window;
    SDL_GLContext gl_ctx;
    SDL_AudioDeviceID audio_dev;
    float sfx_freq_hz;
    float sfx_phase;
    int sfx_samples_remaining;
    GrindWorld world;
    GrindInputState input;
    GrindRenderer renderer;
    uint32_t last_ticks;
    bool mouse_captured;
    int max_frames;
    int frame_count;
} GrindApp;

/* Zero-Asset Procedural Audio Synthesizer Callback (Coins, Sales Chimes, Crafting & Luxury Upgrades) */
static void audio_synth_callback(void *userdata, Uint8 *stream, int len) {
    GrindApp *app = (GrindApp *)userdata;
    int16_t *samples = (int16_t *)stream;
    int count = len / (int)sizeof(int16_t);
    for (int i = 0; i < count; ++i) {
        if (app->sfx_samples_remaining > 0) {
            float env = (float)app->sfx_samples_remaining / 6000.0f;
            if (env > 1.0f) env = 1.0f;
            float wave = sinf(app->sfx_phase * 2.0f * GRIND_PI);
            /* Add harmonic overtone for bright cash-register / coin chime */
            wave = 0.65f * wave + 0.35f * sinf(app->sfx_phase * 4.0f * GRIND_PI);
            samples[i] = (int16_t)(wave * env * 4200.0f);
            app->sfx_phase += app->sfx_freq_hz / 44100.0f;
            if (app->sfx_phase > 1.0f) app->sfx_phase -= 1.0f;
            app->sfx_samples_remaining--;
        } else {
            samples[i] = 0;
        }
    }
}

static void dispatch_sound_events(GrindApp *app) {
    uint32_t mask = app->world.sound_events_mask;
    if (mask == 0) return;
    app->world.sound_events_mask = 0;
    if (app->audio_dev == 0) return;

    SDL_LockAudioDevice(app->audio_dev);
    if (mask & GRIND_SFX_UPGRADE) {
        app->sfx_freq_hz = 880.0f; /* High A5 fanfare chime */
        app->sfx_samples_remaining = 9000;
    } else if (mask & (GRIND_SFX_COIN | GRIND_SFX_SALE)) {
        app->sfx_freq_hz = 1046.5f; /* C6 cash-register chime */
        app->sfx_samples_remaining = 5500;
    } else if (mask & GRIND_SFX_CRAFT) {
        app->sfx_freq_hz = 523.25f; /* C5 craft pour tone */
        app->sfx_samples_remaining = 4000;
    }
    SDL_UnlockAudioDevice(app->audio_dev);
}

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
                } else if (ev.button.button == SDL_BUTTON_LEFT || ev.button.button == SDL_BUTTON_RIGHT) {
                    if (ev.button.button == SDL_BUTTON_RIGHT) {
                        app->mouse_captured = !app->mouse_captured;
                    } else {
                        app->mouse_captured = true;
                    }
                    SDL_SetRelativeMouseMode(app->mouse_captured ? SDL_TRUE : SDL_FALSE);
#ifdef __EMSCRIPTEN__
                    if (app->mouse_captured) {
                        emscripten_request_pointerlock("#canvas", 1);
                    } else {
                        emscripten_exit_pointerlock();
                    }
#endif
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
                } else if (app->mouse_captured || (ev.motion.state & (SDL_BUTTON_LMASK | SDL_BUTTON_RMASK))) {
                    app->input.look_yaw_delta += (float)ev.motion.xrel * 0.0035f;
                    app->input.look_pitch_delta -= (float)ev.motion.yrel * 0.0030f;
                }
                break;
            case SDL_KEYDOWN:
                if (ev.key.repeat) break;
                switch (ev.key.keysym.sym) {
                    case SDLK_ESCAPE:
#ifdef __EMSCRIPTEN__
                        app->mouse_captured = false;
                        app->input.biz_drawer_open = false;
                        SDL_SetRelativeMouseMode(SDL_FALSE);
                        emscripten_exit_pointerlock();
#else
                        app->input.quit_requested = true;
#endif
                        break;
                    case SDLK_e:
                    case SDLK_SPACE:
                        app->input.buttons[TOUCH_BTN_INTERACT].just_pressed = true;
                        break;
                    case SDLK_b:
                        app->input.buttons[TOUCH_BTN_BIZ_MENU].just_pressed = true;
                        if (!app->input.biz_drawer_open) {
                            app->mouse_captured = false;
                            SDL_SetRelativeMouseMode(SDL_FALSE);
#ifdef __EMSCRIPTEN__
                            emscripten_exit_pointerlock();
#endif
                        }
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
#ifdef __EMSCRIPTEN__
    grind_web_set_tablet_open(app->input.biz_drawer_open ? 1 : 0);
#endif

    /* 1 real second = 8 simulated minutes during interactive play */
    grind_world_tick(&app->world, dt_sec * 8.0f);
    dispatch_sound_events(app);

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

static GrindApp g_app;

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

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER | SDL_INIT_EVENTS) != 0) {
        /* Fallback to video-only if headless runner has no audio device */
        if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_EVENTS) != 0) {
            fprintf(stderr, "SDL_Init failed: %s\n", SDL_GetError());
            return 1;
        }
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

    memset(&g_app, 0, sizeof(g_app));
    g_app.max_frames = max_frames;

    g_app.window = SDL_CreateWindow(
        "The Grind - 3D First-Principles Business & Life Sim",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        win_w, win_h,
        SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_SHOWN
    );
    if (!g_app.window) {
        fprintf(stderr, "SDL_CreateWindow failed: %s\n", SDL_GetError());
        SDL_Quit();
        return 1;
    }

    g_app.gl_ctx = SDL_GL_CreateContext(g_app.window);
    if (!g_app.gl_ctx) {
        fprintf(stderr, "SDL_GL_CreateContext failed: %s\n", SDL_GetError());
        SDL_DestroyWindow(g_app.window);
        SDL_Quit();
        return 1;
    }

#ifndef __EMSCRIPTEN__
    SDL_GL_SetSwapInterval(1);
#endif
    grind_world_init(&g_app.world, 0xC0FFEE123456ULL);
    grind_input_init(&g_app.input, win_w, win_h, force_touch);
    g_app.mouse_captured = !g_app.input.touch_controls_active;
    grind_renderer_init(&g_app.renderer);

    SDL_AudioSpec want = {0}, have = {0};
    want.freq = 44100;
    want.format = AUDIO_S16SYS;
    want.channels = 1;
    want.samples = 1024;
    want.callback = audio_synth_callback;
    want.userdata = &g_app;
    g_app.audio_dev = SDL_OpenAudioDevice(NULL, 0, &want, &have, 0);
    if (g_app.audio_dev != 0) {
        SDL_PauseAudioDevice(g_app.audio_dev, 0);
    }

    g_app.last_ticks = SDL_GetTicks();

#ifdef __EMSCRIPTEN__
    emscripten_set_main_loop_arg(step_frame, &g_app, 0, 1);
#else
    while (!g_app.input.quit_requested) {
        step_frame(&g_app);
    }
#endif

    if (g_app.audio_dev != 0) {
        SDL_CloseAudioDevice(g_app.audio_dev);
    }
    grind_renderer_shutdown(&g_app.renderer);
    SDL_GL_DeleteContext(g_app.gl_ctx);
    SDL_DestroyWindow(g_app.window);
    SDL_Quit();
    return 0;
}
