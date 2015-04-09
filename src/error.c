#include "error.h"
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>

extern int disable_warnings;
extern int colored_diagnostics;
extern unsigned warning_count, error_count;

void emit_error(int fatal, char *file, int line, int column, char *fmt, ...)
{
	va_list args;

    if (fatal)
        fprintf(stderr, "An unrecoverable error occurred\n");

    if (colored_diagnostics)
        fprintf(stderr, INFO_COLOR "%s:%d:%d: " ERROR_COLOR "error: " RESET_ATTR, file, line, column);
    else
        fprintf(stderr, "%s:%d:%d: error: ", file, line, column);

	va_start(args, fmt);
	vfprintf(stderr, fmt, args);
	va_end(args);

	fprintf(stderr, "\n");

    if (fatal)
        exit(EXIT_FAILURE);

    ++error_count;
}

void emit_warning(char *file, int line, int column, char *fmt, ...)
{
	va_list args;

    if (disable_warnings)
        return;

    if (colored_diagnostics)
        fprintf(stderr, INFO_COLOR "%s:%d:%d: " WARNING_COLOR "warning: " RESET_ATTR, file, line, column);
    else
        fprintf(stderr, "%s:%d:%d: warning: ", file, line, column);

	va_start(args, fmt);
	vfprintf(stderr, fmt, args);
	va_end(args);

	fprintf(stderr, "\n");

    ++warning_count;
}
