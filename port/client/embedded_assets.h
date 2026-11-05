/* Fonts, language packs and the launcher artwork built into the executable.
 *
 * They are written to the client's data folder on first use: the font and the
 * artwork before the launcher appears, a subtitle pack only when the game is
 * about to start (the launcher shows a loading message meanwhile). The paths
 * stay editable afterwards. */
#pragma once

#ifdef __cplusplus
extern "C" {
#endif

/* Fonts, launcher artwork and the pack names shown in the language pickers.
 * Cheap enough for start-up: a handful of files, no subtitle tables. */
int lain_assets_extract_core(void);

/* Everything under lang/<code>/ (a subtitle pack). */
int lain_assets_extract_lang(const char *code);

/* Every built-in file. */
int lain_assets_extract(void);

#ifdef __cplusplus
}
#endif
