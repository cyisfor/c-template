#include <stdarg.h>

#ifdef DO_MAIN
void apply_template(int dest, int source);
#else
void apply_template(int dest, int source, va_list args);
#endif
