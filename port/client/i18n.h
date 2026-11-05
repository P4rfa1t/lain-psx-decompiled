/* Client UI localization. The game's own text comes from the player's discs and
 * is drawn by the game's bitmap font; this covers the launcher, the F1 menu and
 * the client's own messages. */
#pragma once

#ifdef __cplusplus
extern "C" {
#endif

/* "auto" (the system locale, Chinese when it cannot be detected), "en", "zh". */
void i18n_set_language(const char *code);
const char *i18n_language(void);
int i18n_is_zh(void);

/* Translate a UI string. Strings without a translation are returned unchanged,
 * so callers can keep using the English literal. */
const char *tr(const char *en);

#ifdef __cplusplus
}
#endif
