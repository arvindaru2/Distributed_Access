
#pragma once

#include <cstdarg>

#define _MPLOG(fmt, ...) do { \
  grep_log("%s:%d: " fmt "\n", \
      __FILE__, __LINE__ , ## __VA_ARGS__); \
} while (0)
#define MPLOG(fmt, ...) _MPLOG(fmt , ## __VA_ARGS__)

void grep_log(const char *fmt, ...);

