#pragma once

#include <unistd.h>
#include <stdarg.h>
#include <sys/fsuid.h>

int fake_printf(const char* format, ...);
void fake_exit(int c);

#define printf fake_printf
#define exit fake_exit
#define main real_main
// do not drop privileges or we will not gain them back
#define setgid setfsgid
#define setuid setfsuid
