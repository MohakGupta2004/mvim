/* Includes */
#include <asm-generic/errno-base.h>
#include <asm-generic/ioctls.h>
#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>

/* Defines */
#define CTRL_KEY(k) ((k) & 0x1f)

/* Data */

// This is the original terminal settings before raw mode. Let's assume: 0100.
// But after the changes in the raw before exist. **atexit() function it'll
// overwrite 0100 value again.
struct editorConfig {
  struct termios original_termios;
  int screenrows;
  int screencolumns;
};
struct editorConfig E;
/* Terminal */

/*
 * This fucntion is for error handling
 * */
void die(const char *s) {
  write(STDIN_FILENO, "\x1b[?1049l", 8);
  perror(s);
  exit(1);
}

/*
 * This function restores the value to it's original mode atexit. if error
 * happens it calls the die function.
 * */
void disableRawMode() {
  if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &E.original_termios) == -1) {
    die("tcsetattr");
  }
}
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
 *      - IXON = Setting the IXON which refers to the CTRL+S AND CTRL+Q issue.
 * As it freezes the output and CTRL+Q unfreezes it. We're stopping it.
 *      - ICRNL = ICRNL refers to the CTRL+M issue where CTRL+M means to \r
 * which generates similar ascii value of RETURN (10). So disabling it making
 * it 13.
 *      - BRKINT = Sometimes break condition can apply which can make it
 * generates a NULL byte which some terminal coz SIGINT. So making it without
 * it.
 *      - INPCK = Stands for Input Parity Checking.
 *      - ISTRIP = Causes the 8th bit to get stripped.
 * 5. setting the c_cflag as doing an BITWISE OR with CS8. It's not a flag it's
 * a bitmask which is set for char bytes to only has 8 bits as it strips the
 * last null byte.
 *
 * 6. setting the c_oflag to OPOST which means Ouptut post processing. It
 * actually turns off \n to  \r\n and which returns actual values of processed
 * keystroke.
 *
 * 7. set VMIN = 0 as VMIN means minimum number of bytes to read.
 * 8. set VTIME = 1 as VTIME means 1 x 0.1 seconds = 100 miliseconds.
 *
 * 9. setting the c_lflag (how local bytes are handled) as it'll wipe out,
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
  tcgetattr(STDIN_FILENO, &E.original_termios);
  atexit(disableRawMode);
  struct termios raw = E.original_termios;
  raw.c_iflag &= ~(IXON | ICRNL | BRKINT | INPCK | ISTRIP);
  raw.c_cflag |= (CS8);
  raw.c_oflag &= ~(OPOST);
  raw.c_lflag &= ~(ECHO | ICANON | ISIG | IEXTEN);
  raw.c_cc[VMIN] = 0;
  raw.c_cc[VTIME] = 1;
  if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) == -1) {
    die("tcsetattr");
  }
}

/*
 * Editor:
 * 1. switching to a alternate screen using the editorInit() function where [ =>
 * ESC or the control sequence, ?1049h hisdes the current screen and opens the
 * alternate screen and 1029l means it'll stop when exit means it'll return to
 * normal state or the previous screen before.
 * 2. drawRows just prints the rows
 * */
int getWindowSize(int *rows, int *cols) {
  struct winsize w;
  if (ioctl(STDIN_FILENO, TIOCGWINSZ, &w) == -1) {
    return -1;
  } else {
    *rows = w.ws_row;
    *cols = w.ws_col;
    return 0;
  }
}

void editorInit() {
  write(STDIN_FILENO, "\x1b[?1049h", 8); // Switch to alternate screen buffer
  if (getWindowSize(&E.screenrows, &E.screencolumns) == -1)
    die("getWindowSize");
}

void editorQuit() { write(STDIN_FILENO, "\x1b[?1049l", 8); }

void drawRows() {
  for (int i = 0; i < E.screenrows; i++) {
    write(STDIN_FILENO, "~\r\n", 3);
  }
}

/*
 * 1. editorReadKey is looping over the input signal and it'll capture every
 * input the user gives
 * 2. editorKeyPress takes the input and perform certain tasks. Right now it's
 * CTRL+Q for exit only.
 * */
char editorReadKey() {
  char c = '\0';
  while (read(STDIN_FILENO, &c, 1) == -1) {
    if (errno != EAGAIN)
      die("read");
  }
  return c;
}
/* Input */
void editorKeyPress() {
  char c = editorReadKey();
  switch (c) {
  case CTRL_KEY('q'):
    editorQuit();
    exit(0);
    break;
  }
}

/*
 * This function helps the clear the alternate screeen, make the cursor go to
 * the top right.
 * */

void editorRefreshScreen() {
  //  system("clear");
  write(STDIN_FILENO, "\x1b[2J", 4); // Clear that screen
  write(STDIN_FILENO, "\x1b[H", 3);  // Move cursor to top-left corner
  drawRows();
  write(STDIN_FILENO, "\x1b[H", 3); // Move
}

/* Init */

/*
 * 1. Enabling the raw mode.
 * 2. Initialzing character which we're reading from the user as an input and if
 * 3. the character is 'q' and user enters it'll stop the program and it'll
 * display the output and it's corresponding ascii characters
 * */
int main(int argc, char *argv[]) {
  enableRawMode();
  editorInit();

  while (1) {
    editorRefreshScreen();
    editorKeyPress();
  }
  return EXIT_SUCCESS;
}
