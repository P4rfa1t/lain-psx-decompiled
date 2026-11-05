#include "PsyX/PsyX_logger.h"

#include <stdarg.h>
#include <stdio.h>

#include <SDL.h>

static Uint32 s_logStart;
static int s_logStarted;

void lain_log(const char *fmt, ...)
{
	va_list ap;
	Uint32 now = SDL_GetTicks();
	Uint32 t;

	if (!s_logStarted)
	{
		s_logStart = now;
		s_logStarted = 1;
	}
	t = now - s_logStart;

	fprintf(stderr, "[%6u.%03u] ", t / 1000u, t % 1000u);
	va_start(ap, fmt);
	vfprintf(stderr, fmt, ap);
	va_end(ap);
	fputc('\n', stderr);
	fflush(stderr);
}
