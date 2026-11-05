/* Maps SDL touch events to the mouse events Dear ImGui understands, so the
 * launcher and the F1 menu can be used on the Switch's touch screen. */
#pragma once

#ifdef __cplusplus
extern "C" {
#endif

union SDL_Event;
void lain_touch_to_imgui(const union SDL_Event *ev, float display_w, float display_h);

/* Feeds the Switch's pads and touch panel to Dear ImGui. No-op off the console. */
void lain_switch_input_feed(float display_w, float display_h);

#ifdef __cplusplus
}
#endif
