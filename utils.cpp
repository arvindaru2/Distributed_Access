
#include <cstdarg>
#include <cstdio>
#include "utils.hpp"

// From:
// https://stackoverflow.com/questions/150543/forward-an-invocation-of-a-variadic-function-in-c
void grep_log(const char *fmt, ...)
{
  va_list argp;
  va_start(argp, fmt);
  vfprintf(stderr, fmt, argp);
  va_end(argp);
}

