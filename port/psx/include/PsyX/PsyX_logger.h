/* Timestamped diagnostic log shared by the emulator, the client and the game.
 *
 * Writes one line per call to stderr; on the Switch main() redirects stderr to
 * sdmc:/switch/lain/lain.log, so a black screen or a crash leaves a trail. */
#pragma once

#ifdef __cplusplus
extern "C" {
#endif

void lain_log(const char *fmt, ...);

#ifdef __cplusplus
}
#endif
