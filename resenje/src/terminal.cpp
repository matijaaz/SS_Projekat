#include "../inc/terminal.hpp"

termios oldSettings;
int flags;

void init_terminal() {
  termios newSettings;
	tcgetattr(STDIN_FILENO, &oldSettings);
	newSettings = oldSettings;
	newSettings.c_lflag &= ~(ICANON | ECHO );
	tcsetattr(STDIN_FILENO, TCSANOW, &newSettings);
	flags = fcntl(STDIN_FILENO, F_GETFL, 0);
  fcntl(STDIN_FILENO, F_SETFL, flags | O_NONBLOCK);
}

void close_terminal() {
  fcntl(STDIN_FILENO, F_SETFL, flags);
	tcsetattr(STDIN_FILENO, TCSANOW, &oldSettings);
}

