#include "grind_app/grind_input.h"
#include <string.h>

static void layout_touch_controls(GrindInputState *in) {
    /* Automatic switch to on-screen controls on small viewports (<900px width or <600px height) or touch */
    bool small_screen = (in->viewport_w < 900 || in->viewport_h < 600);
    in->touch_controls_active = in->force_touch_ui || in->touch_detected || small_screen;

    in->joy_origin_x = 0.16f;
    in->joy_origin_y = 0.76f;
    if (!in->joy_active) {
        in->joy_current_x = in->joy_origin_x;
        in->joy_current_y = in->joy_origin_y;
    }
    in->joy_radius = 0.10f;

    /* Ergonomic right-thumb button diamond */
    in->buttons[TOUCH_BTN_INTERACT] = (GrindTouchButton){0.86f, 0.78f, 0.065f, "ACT [E]", false, false};
    in->buttons[TOUCH_BTN_BIZ_MENU] = (GrindTouchButton){0.74f, 0.84f, 0.055f, "BIZ [B]", false, false};
    in->buttons[TOUCH_BTN_MAP_VIEW] = (GrindTouchButton){0.86f, 0.62f, 0.055f, "MAP [M]", false, false};
    in->buttons[TOUCH_BTN_SLEEP]    = (GrindTouchButton){0.74f, 0.68f, 0.055f, "REST[R]", false, false};
}

void grind_input_init(GrindInputState *in, int viewport_w, int viewport_h, bool force_touch_ui) {
    memset(in, 0, sizeof(*in));
    in->viewport_w = viewport_w > 0 ? viewport_w : 1280;
    in->viewport_h = viewport_h > 0 ? viewport_h : 720;
    in->force_touch_ui = force_touch_ui;
    in->camera_mode = CAMERA_MODE_FIRST_PERSON;
    in->map_transition_t = 0.0f;
    in->joy_finger_id = -1;
    in->look_finger_id = -1;
    layout_touch_controls(in);
}

void grind_input_resize(GrindInputState *in, int viewport_w, int viewport_h) {
    in->viewport_w = viewport_w > 0 ? viewport_w : 1;
    in->viewport_h = viewport_h > 0 ? viewport_h : 1;
    layout_touch_controls(in);
}

void grind_input_begin_frame(GrindInputState *in) {
    in->look_yaw_delta = 0.0f;
    in->look_pitch_delta = 0.0f;
    for (int i = 0; i < TOUCH_BTN_COUNT; ++i) {
        in->buttons[i].just_pressed = false;
    }
    if (!in->joy_active) {
        in->move_forward = 0.0f;
        in->move_strafe = 0.0f;
    }
}

void grind_input_on_touch_down(GrindInputState *in, int64_t finger_id, float norm_x, float norm_y) {
    in->touch_detected = true;
    in->touch_controls_active = true;

    /* 1. Check right-hand action buttons first */
    float aspect = (float)in->viewport_w / (float)(in->viewport_h > 0 ? in->viewport_h : 1);
    for (int i = 0; i < TOUCH_BTN_COUNT; ++i) {
        GrindTouchButton *b = &in->buttons[i];
        float dx = (norm_x - b->cx) * (aspect > 1.0f ? 1.0f : aspect);
        float dy = (norm_y - b->cy);
        if (dx * dx + dy * dy <= (b->radius * 1.25f) * (b->radius * 1.25f)) {
            b->pressed = true;
            b->just_pressed = true;
            return;
        }
    }

    /* 2. Left half of screen controls the virtual movement thumbstick */
    if (norm_x < 0.48f && !in->joy_active) {
        in->joy_active = true;
        in->joy_finger_id = finger_id;
        in->joy_current_x = norm_x;
        in->joy_current_y = norm_y;

        float dx = (norm_x - in->joy_origin_x) / in->joy_radius;
        float dy = (norm_y - in->joy_origin_y) / in->joy_radius;
        in->move_strafe = grind_clampf(dx, -1.0f, 1.0f);
        in->move_forward = grind_clampf(-dy, -1.0f, 1.0f);
        return;
    }

    /* 3. Right half of screen controls first-person look yaw/pitch */
    if (!in->look_touch_active) {
        in->look_touch_active = true;
        in->look_finger_id = finger_id;
        in->look_last_x = norm_x;
        in->look_last_y = norm_y;
    }
}

void grind_input_on_touch_motion(GrindInputState *in, int64_t finger_id, float norm_x, float norm_y, float dx, float dy) {
    in->touch_detected = true;
    in->touch_controls_active = true;

    if (in->joy_active && finger_id == in->joy_finger_id) {
        in->joy_current_x = norm_x;
        in->joy_current_y = norm_y;
        float jdx = (norm_x - in->joy_origin_x) / in->joy_radius;
        float jdy = (norm_y - in->joy_origin_y) / in->joy_radius;
        float len = sqrtf(jdx * jdx + jdy * jdy);
        if (len > 1.0f) {
            jdx /= len;
            jdy /= len;
            in->joy_current_x = in->joy_origin_x + jdx * in->joy_radius;
            in->joy_current_y = in->joy_origin_y + jdy * in->joy_radius;
        }
        in->move_strafe = grind_clampf(jdx, -1.0f, 1.0f);
        in->move_forward = grind_clampf(-jdy, -1.0f, 1.0f);
    } else if (in->look_touch_active && finger_id == in->look_finger_id) {
        in->look_yaw_delta += dx * 3.2f;
        in->look_pitch_delta -= dy * 2.4f;
        in->look_last_x = norm_x;
        in->look_last_y = norm_y;
    }
}

void grind_input_on_touch_up(GrindInputState *in, int64_t finger_id, float norm_x, float norm_y) {
    (void)norm_x;
    (void)norm_y;
    if (in->joy_active && finger_id == in->joy_finger_id) {
        in->joy_active = false;
        in->joy_finger_id = -1;
        in->joy_current_x = in->joy_origin_x;
        in->joy_current_y = in->joy_origin_y;
        in->move_forward = 0.0f;
        in->move_strafe = 0.0f;
    }
    if (in->look_touch_active && finger_id == in->look_finger_id) {
        in->look_touch_active = false;
        in->look_finger_id = -1;
    }
    for (int i = 0; i < TOUCH_BTN_COUNT; ++i) {
        in->buttons[i].pressed = false;
    }
}

void grind_input_apply_to_world(GrindInputState *in, GrindWorld *world, float dt_sec) {
    GrindPlayer *p = &world->player;

    /* Update camera yaw and pitch */
    p->yaw_rad += in->look_yaw_delta;
    p->pitch_rad = grind_clampf(p->pitch_rad + in->look_pitch_delta, GRIND_DEG2RAD(-72.0f), GRIND_DEG2RAD(72.0f));

    /* Smooth transition between First-Person 3D and Top-Down Management Map */
    float target_t = (in->camera_mode == CAMERA_MODE_TOP_DOWN_MAP) ? 1.0f : 0.0f;
    in->map_transition_t += (target_t - in->map_transition_t) * grind_clampf(dt_sec * 8.0f, 0.0f, 1.0f);

    /* Apply player movement */
    grind_player_move(world, in->move_forward, in->move_strafe, dt_sec);

    /* Process contextual buttons */
    if (in->buttons[TOUCH_BTN_INTERACT].just_pressed) {
        grind_action_context_interact(world);
    }
    if (in->buttons[TOUCH_BTN_MAP_VIEW].just_pressed) {
        in->camera_mode = (in->camera_mode == CAMERA_MODE_FIRST_PERSON)
            ? CAMERA_MODE_TOP_DOWN_MAP
            : CAMERA_MODE_FIRST_PERSON;
    }
    if (in->buttons[TOUCH_BTN_SLEEP].just_pressed) {
        grind_action_sleep_overnight(world);
    }
    if (in->buttons[TOUCH_BTN_BIZ_MENU].just_pressed) {
        in->biz_drawer_open = !in->biz_drawer_open;
        /* In quick-action mode, execute next smart business milestone if affordable */
        if (p->stand_deployed && world->employee_count == 0 && p->cash >= 45.0f) {
            grind_action_hire_employee(world, EMP_ROLE_STAND_OPERATOR);
        } else if (world->employee_count == 1 && p->cash >= 65.0f) {
            grind_action_hire_employee(world, EMP_ROLE_COURIER);
        } else if (p->marketing_awareness < 0.5f && p->cash >= 50.0f) {
            grind_action_launch_marketing_campaign(world, 1);
        }
    }
}

Mat4 grind_input_compute_view_matrix(const GrindInputState *in, const GrindWorld *world) {
    const GrindPlayer *p = &world->player;

    /* First-Person eye position with subtle stride head-bob */
    float bob = sinf((p->pos.x + p->pos.z) * 1.4f) * 0.04f;
    float eye_h = (p->vehicle == VEHICLE_BICYCLE || p->vehicle == VEHICLE_CARGO_EBIKE) ? 1.85f : 1.68f;
    Vec3 fp_eye = vec3_make(p->pos.x, p->pos.y + eye_h + bob, p->pos.z);

    float cos_p = cosf(p->pitch_rad);
    Vec3 fp_dir = vec3_make(
        sinf(p->yaw_rad) * cos_p,
        sinf(p->pitch_rad),
        -cosf(p->yaw_rad) * cos_p
    );
    Vec3 fp_center = vec3_add(fp_eye, fp_dir);

    /* Top-Down 3D Tactical Management Map camera */
    Vec3 map_eye = vec3_make(p->pos.x * 0.35f, 92.0f, p->pos.z * 0.35f + 28.0f);
    Vec3 map_center = vec3_make(p->pos.x * 0.35f, 0.0f, p->pos.z * 0.35f);

    float t = in->map_transition_t;
    Vec3 eye = vec3_make(
        grind_lerpf(fp_eye.x, map_eye.x, t),
        grind_lerpf(fp_eye.y, map_eye.y, t),
        grind_lerpf(fp_eye.z, map_eye.z, t)
    );
    Vec3 center = vec3_make(
        grind_lerpf(fp_center.x, map_center.x, t),
        grind_lerpf(fp_center.y, map_center.y, t),
        grind_lerpf(fp_center.z, map_center.z, t)
    );

    return mat4_look_at(eye, center, vec3_make(0.0f, 1.0f, 0.0f));
}

Mat4 grind_input_compute_proj_matrix(const GrindInputState *in) {
    float aspect = (float)in->viewport_w / (float)(in->viewport_h > 0 ? in->viewport_h : 1);
    float fov = grind_lerpf(GRIND_DEG2RAD(68.0f), GRIND_DEG2RAD(52.0f), in->map_transition_t);
    return mat4_perspective(fov, aspect, 0.2f, 260.0f);
}
