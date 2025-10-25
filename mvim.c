#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <termios.h>
#include <unistd.h>

// This is the original terminal settings before raw mode. Let's assume: 0100.
// But after the changes in the raw before exist. **atexit() function it'll
// overwrite 0100 value again.
struct termios original_termios;
/*
 * This function restores the value to it's original mode atexit.
 * */
void disableRawMode() { tcsetattr(STDIN_FILENO, TCSAFLUSH, &original_termios); }
/*
 * 1. settings the current terminal settings to original_termios so we can
 * restore it when we want.
 *
 * 2. setting the atexit function run disableRawMode when exit as it restores
 * the terminal settings.
 *
 * 3. in raw we're assigning the original_termios which we'll going to
 * manipulate.
 *
 * 4. setting the c_iflag (how input bytes are handled) as it'll wipe out (&= ~,
 * meaning doing a NOT operation on the existing binary and doing an AND
 * operation on it which will make it 0. Assume: 0100, do NOT you are getting
 * 1011. do AND you'll get 0000),
 *      - IXEN = Setting the IXON which refers to the CTRL+S AND CTRL+Q issue.
 * As it freezes the output and CTRL+Q unfreezes it. We're stopping it.
 *      - ICRNL = ICRNL refers to the CTRL+M issue where CTRL+M means to \r
 * which generates similar ascii value of RETURN (10). So disabling it making
 * it 13.
 *
 * 5. setting the c_lflag (how local bytes are handled) as it'll wipe out,
 *      - ECHO = Shows us whatever we're typing. Disabling it.
 *      - ICANON = Canonical mode. Not making it line by line otherwise making
 * it bytes by bytes. Disabling the line by line option.
 *      - ISIG = SIGTERM or SIGINT related things. Disabling it.
 *      - IEXTEN = type Ctrl-V, the terminal waits for you to type another
 * character and then sends that character literally. Disabling it.
 *
 * 6. setting the terminal settings in raw.
 * */
void enableRawMode() {
  tcgetattr(STDIN_FILENO, &original_termios);
  atexit(disableRawMode);
  struct termios raw = original_termios;
  raw.c_iflag &= ~(IXON | ICRNL);
  raw.c_lflag &= ~(ECHO | ICANON | ISIG | IEXTEN);
  tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
}

/*
 * Enabling the raw mode.
 * Initialzing character which we're reading from the user as an input and if
 * the character is 'q' and user enters it'll stop the program and it'll display
 * the output and it's corresponding ascii characters
 *
 * */
int main(int argc, char *argv[]) {
  enableRawMode();
  char c;
  while (read(STDIN_FILENO, &c, 1) == 1 && c != 'q') {
    if (iscntrl(c)) {
      printf("%d", c);
    } else {
      printf("%d(%c)", c, c);
    }
  }
  return EXIT_SUCCESS;
}
