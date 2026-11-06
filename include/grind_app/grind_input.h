#ifndef GRIND_INPUT_H
#define GRIND_INPUT_H

#include "grind_core/grind_core.h"

/* Camera view modes as chosen by user: First-Person 3D + toggleable Top-Down Management Map */
typedef enum {
    CAMERA_MODE_FIRST_PERSON = 0,
    CAMERA_MODE_TOP_DOWN_MAP = 1
} GrindCameraMode;

/* Virtual Touch Button IDs for Web / Small-Device On-Screen Controls */
typedef enum {
    TOUCH_BTN_INTERACT = 0, /* [ACT / E] */
    TOUCH_BTN_BIZ_MENU = 1, /* [BIZ / B] */
    TOUCH_BTN_MAP_VIEW = 2, /* [MAP / M] */
    TOUCH_BTN_SLEEP    = 3, /* [REST / R] */
    TOUCH_BTN_COUNT
} GrindTouchButtonId;

typedef struct {
    float cx, cy; /* Normalized screen coordinates [0.0, 1.0] */
    float radius; /* Normalized radius */
    const char *label;
    bool pressed;
    bool just_pressed;
} GrindTouchButton;

typedef struct {
    int viewport_w;
    int viewport_h;
    bool force_touch_ui;
    bool touch_detected;
    bool touch_controls_active; /* True when viewport < 900x600 or touch event received */

    GrindCameraMode camera_mode;
    float map_transition_t; /* 0.0 = First-Person, 1.0 = Top-Down 3D Management Map */

    /* Continuous axes [-1.0, +1.0] */
    float move_forward;
    float move_strafe;
    float look_yaw_delta;
    float look_pitch_delta;

    /* Virtual Left Thumbstick state (normalized [0,1] coords) */
    bool joy_active;
    int64_t joy_finger_id;
    float joy_origin_x, joy_origin_y;
    float joy_current_x, joy_current_y;
    float joy_radius;

    /* Right-side touch look drag state */
    bool look_touch_active;
    int64_t look_finger_id;
    float look_last_x, look_last_y;

    /* On-Screen Action Buttons */
    GrindTouchButton buttons[TOUCH_BTN_COUNT];

    /* Business quick-menu drawer toggle & selected tablet action row (1..6, 0=none) */
    bool biz_drawer_open;
    int tablet_action_selected;
    bool quit_requested;
} GrindInputState;

void grind_input_init(GrindInputState *in, int viewport_w, int viewport_h, bool force_touch_ui);
void grind_input_resize(GrindInputState *in, int viewport_w, int viewport_h);
void grind_input_begin_frame(GrindInputState *in);

/* Feed synthetic or SDL normalized touch/pointer events (x, y in [0.0, 1.0]) */
void grind_input_on_touch_down(GrindInputState *in, int64_t finger_id, float norm_x, float norm_y);
void grind_input_on_touch_motion(GrindInputState *in, int64_t finger_id, float norm_x, float norm_y, float dx, float dy);
void grind_input_on_touch_up(GrindInputState *in, int64_t finger_id, float norm_x, float norm_y);

/* Apply input state to the GrindWorld simulation & compute 3D View/Projection matrices */
void grind_input_apply_to_world(GrindInputState *in, GrindWorld *world, float dt_sec);
Mat4 grind_input_compute_view_matrix(const GrindInputState *in, const GrindWorld *world);
Mat4 grind_input_compute_proj_matrix(const GrindInputState *in);

#endif /* GRIND_INPUT_H */
