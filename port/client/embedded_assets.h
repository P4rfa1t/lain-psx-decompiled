/* Fonts and language packs built into the executable.
 *
 * The release is a single file: on first run these are written to the client's
 * data folder (fonts/ui.ttf, lang/<code>/...), where the normal pack and font
 * lookup already expects them. The paths stay editable afterwards. */
#pragma once

#ifdef __cplusplus
extern "C" {
#endif

/* Returns the number of files written (0 if everything is already there). */
int lain_assets_extract(void);

#ifdef __cplusplus
}
#endif
