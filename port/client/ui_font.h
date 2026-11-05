/* ImGui UI font selection. The default ProggyClean has no CJK glyphs, so a TTF
 * with Chinese coverage is loaded as the UI font when one can be found. */
#pragma once

#ifdef __cplusplus
extern "C" {
#endif

struct ImFont;
/* Adds the UI font to the current ImGui context's atlas and returns it.
 * Search order: $LAIN_UI_FONT, <data>/fonts/ui.ttf|otf|ttc, system CJK fonts,
 * ImGui's default font. */
struct ImFont *ui_font_load(float size_px);

#ifdef __cplusplus
}
#endif
