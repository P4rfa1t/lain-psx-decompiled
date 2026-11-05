#include "touch.h"

#include <SDL.h>

#include "imgui_impl_sdl2.h"

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
