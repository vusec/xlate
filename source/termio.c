#include <stdio.h>

#include <termios.h>

#include <xlate/termio.h>

void await_key_press(void)
{
	struct termios prev, info;

	tcgetattr(0, &info);
	prev = info;

	info.c_lflag &= ~ICANON;
	info.c_cc[VMIN] = 1;
	info.c_cc[VTIME] = 0;

	tcsetattr(0, TCSANOW, &info);
	getchar();
	tcsetattr(0, TCSANOW, &prev);
}
