#ifndef TERMINAL_H
#define TERMINAL_H
#include <cstdint>
#include <termios.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>

#define TERM_OUT 0xFFFFFF00
#define TERM_IN 0xFFFFFF04


void init_terminal();
void close_terminal();

#endif