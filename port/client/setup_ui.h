/* First-run / settings window: disc import, display and language options. */
#pragma once
#include "settings.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Reads the game data from the discs while the launcher can still show a
 * "loading" screen. Return 0 on success. May be NULL. */
typedef int (*SetupPreloadFn)(Settings *settings, const char *disc1, const char *disc2);

/* Shows the setup window until the player presses Play (returns 1) or quits (0).
 * Creates and destroys its own SDL window; SDL video must be initialised.
 * `preload` runs with the launcher still on screen. */
int setup_ui_run(Settings *settings, SetupPreloadFn preload);

#ifdef __cplusplus
}
#endif
