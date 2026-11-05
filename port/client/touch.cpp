#include "touch.h"

#include <SDL.h>

#include "imgui_impl_sdl2.h"
#include "PsyX/PsyX_logger.h"

#if defined(__SWITCH__)
#include <switch.h>

#include "imgui.h"

/* The console's own input state. SDL2 on Switch may expose neither the built-in
 * pads nor the touch screen, so drive Dear ImGui from libnx directly. */
extern "C" void lain_switch_input_feed(float display_w, float display_h) {
    static PadState pad;
    static bool pad_ready = false;
    static bool touch_down = false;
    static float last_x = 0.0f, last_y = 0.0f;
    if (!pad_ready) {
        padInitializeDefault(&pad);
        hidInitializeTouchScreen();
        pad_ready = true;
        lain_log("input: libnx pad + touch screen ready");
    }
    padUpdate(&pad);
    const u64 k = padGetButtons(&pad);

    ImGuiIO &io = ImGui::GetIO();
    if (padIsConnected(&pad)) {
        /* ImGui maps a pad through SDL's positional names (SDL's A is the bottom
         * button, i.e. the Switch's B), which flips confirm and cancel. Drive
         * the keyboard navigation instead and keep SDL's pad mapping out of it:
         * A is Enter (confirm), B is Escape (cancel), the D-pad and left stick
         * move. SDL's backend also feeds gamepad keys every NewFrame, so the
         * flag is cleared here, after it ran and before ImGui::NewFrame reads it. */
        io.BackendFlags &= ~ImGuiBackendFlags_HasGamepad;

        static bool logged = false;
        if (!logged) {
            logged = true;
            lain_log("input: pad via libnx (A confirm, B cancel)");
        }

        const bool up = (k & (HidNpadButton_Up | HidNpadButton_StickLUp)) != 0;
        const bool down_b = (k & (HidNpadButton_Down | HidNpadButton_StickLDown)) != 0;
        const bool left = (k & (HidNpadButton_Left | HidNpadButton_StickLLeft)) != 0;
        const bool right = (k & (HidNpadButton_Right | HidNpadButton_StickLRight)) != 0;
        const bool a = (k & HidNpadButton_A) != 0;
        const bool b = (k & HidNpadButton_B) != 0;

        io.AddKeyEvent(ImGuiKey_UpArrow, up);
        io.AddKeyEvent(ImGuiKey_DownArrow, down_b);
        io.AddKeyEvent(ImGuiKey_LeftArrow, left);
        io.AddKeyEvent(ImGuiKey_RightArrow, right);
        io.AddKeyEvent(ImGuiKey_Enter, a || (k & HidNpadButton_Plus) != 0);
        io.AddKeyEvent(ImGuiKey_Escape, b);
    }

    /* Read the recent history: a quick tap can begin and end between two
     * frames, and sampling only the newest state would drop it (which feels
     * like the screen ignoring taps until the finger is held). */
    HidTouchScreenState st[8];
    const size_t n = hidGetTouchScreenStates(st, 8);
    bool down = false;
    const float sx = display_w > 0.0f ? display_w / 1280.0f : 1.0f;
    const float sy = display_h > 0.0f ? display_h / 720.0f : 1.0f;
    for (size_t i = 0; i < n; i++) {
        if (st[i].count > 0 && (st[i].touches[0].attributes & 1)) {
            down = true;
            // libnx reports the panel's native 1280x720 pixels.
            last_x = (float)st[i].touches[0].x * sx;
            last_y = (float)st[i].touches[0].y * sy;
        }
    }
    if (down) {
        io.AddMousePosEvent(last_x, last_y);
    }
    if (down != touch_down) {
        touch_down = down;
        if (down) {
            io.AddMousePosEvent(last_x, last_y);
        }
        io.AddMouseButtonEvent(0, down);
    }
}
#else
extern "C" void lain_switch_input_feed(float display_w, float display_h) {
    (void)display_w;
    (void)display_h;
}
#endif

extern "C" void lain_touch_to_imgui(const SDL_Event *ev, float display_w, float display_h) {
    if (!ev || display_w <= 0.0f || display_h <= 0.0f) {
        return;
    }
    /* SDL reports touch in 0..1 window coordinates. Some ports report pixels;
     * accept both. */
    float x = ev->tfinger.x;
    float y = ev->tfinger.y;
    if (x <= 1.0f && y <= 1.0f) {
        x *= display_w;
        y *= display_h;
    }

    SDL_Event me;
    SDL_zero(me);
    me.type = SDL_MOUSEMOTION;
    me.motion.which = SDL_TOUCH_MOUSEID;
    me.motion.x = (Sint32)x;
    me.motion.y = (Sint32)y;
    me.motion.state = ev->type == SDL_FINGERUP ? 0 : SDL_BUTTON_LMASK;
    ImGui_ImplSDL2_ProcessEvent(&me);

    if (ev->type == SDL_FINGERDOWN || ev->type == SDL_FINGERUP) {
        SDL_zero(me);
        me.type = ev->type == SDL_FINGERDOWN ? SDL_MOUSEBUTTONDOWN : SDL_MOUSEBUTTONUP;
        me.button.which = SDL_TOUCH_MOUSEID;
        me.button.button = SDL_BUTTON_LEFT;
        me.button.clicks = 1;
        me.button.x = (Sint32)x;
        me.button.y = (Sint32)y;
        ImGui_ImplSDL2_ProcessEvent(&me);
    }
}
