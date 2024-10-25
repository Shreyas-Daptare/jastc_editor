/* header files */
dfjasdkj
dsfjakldjfk
#include<ctype.h>
#include<errno.h>
#include<stdio.h>
#include<unistd.h>
#include<sys/ioctl.h>
#include<termios.h>
#include<stdlib.h>

/* defines */

#define CTRL_KEY(k) ((k) & 0x1f)

/* data */
struct editorConfig //Store editor state which will contain our terminal's height and width 
{
	int screenrows;
  	int screencols;	
	struct termios orig_termios;
};

struct editorConfig E;
/* terminal handling */

void die(const char *s)
{
	write(STDOUT_FILENO, "\x1b[2J", 4);
  	write(STDOUT_FILENO, "\x1b[H", 3);
	
	perror(s);
	exit(1);
}
void disableRawMode()
{
	if(tcsetattr(STDIN_FILENO, TCSAFLUSH, &E.orig_termios) == -1)
		die("tcsetattr");
}
void enableRawMode()
{ 	
	if(tcgetattr(STDIN_FILENO, &E.orig_termios) == -1)
		die("tcgetattr");
	atexit(disableRawMode);

	struct termios raw = E.orig_termios;
	/* Disable Ctrl-S and Ctrl-Q using IXON, Disable Ctrl-M using ICRNL, BRKINT, INCPK, ISTRIP are all other miscellaneous flags
	 * which are needed to set the terminal to raw mode */
	raw.c_iflag &= ~(BRKINT | IXON | ICRNL | INPCK | ISTRIP);
	/* Turn off all output processing like translation of \n to \r\n, from now on we will have to type \n\r manually*/
	raw.c_oflag &= ~(OPOST);
	/* Other miscellaneous flag needed to set the terminal in raw mode */
	raw.c_cflag |= (CS8);
	/* Disable showing output using ECHO, disable canonical mode using ICANON, disable Ctrl-C and Ctrl-Z using ISIG,
	 * Disable Ctrl-V using IEXTEN */
	raw.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);
	/* The VMIN value sets the minimum number of bytes of input needed before read() can return. */
	raw.c_cc[VMIN] = 0;
	/* The VTIME value sets the maximum amount of time to wait before read() returns. It is in tenths of a second i.e. 100 milliseconds. If read() times out, it will return 0, because its usual return value is the number of bytes read. */
  	raw.c_cc[VTIME] = 1;


	if(tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) == -1) 
		die("tcsetattr");
}

char editorReadKey() //read the keypress input
{
  int nread;
  char c;
  while ((nread = read(STDIN_FILENO, &c, 1)) != 1) 
  {
    if (nread == -1 && errno != EAGAIN)
	    die("read");
  }
  return c;
}

int getCursorPosition(int *rows, int *cols) 
{
	  if (write(STDOUT_FILENO, "\x1b[6n", 4) != 4) return -1;
  printf("\r\n");
  char c;
  while (read(STDIN_FILENO, &c, 1) == 1) {
    if (iscntrl(c)) {
      printf("%d\r\n", c);
    } else {
      printf("%d ('%c')\r\n", c, c);
    }
  }
  editorReadKey();
  return -1;
}


int getWindowSize(int *rows, int *cols) //To get window size using ioctl  
{
  struct winsize ws;

  if (1 || ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == -1 || ws.ws_col == 0) 
  {
	  if(write(STDOUT_FILENO, "\x1b[999C\x1b[999B", 12) != 12)
		  return -1;
	  return getCursorPosition(rows, cols);
  } 
  else 
  {
    *cols = ws.ws_col;
    *rows = ws.ws_row;
    return 0;
  }
}

/* output */
void editorDrawRows() // Draw tildes(~) in the windows 
{
  int y;
  for (y = 0; y < E.screenrows; y++) 
  {
    write(STDOUT_FILENO, "?\r\n", 3);
  }
}

void editorRefreshScreen()
{
	write(STDOUT_FILENO, "\x1b[2J", 4); //Clear the screen 
	write(STDOUT_FILENO, "\x1b[H", 3); //Position the cursor to the top of the screen

	editorDrawRows();

	write(STDOUT_FILENO, "\x1b[H", 3); //Reposition the cursor to the top of the screen after drawing the tildes(~)
}

/* input */

void editorProcessKeypress() //process user keypress and check for key combinations
{
  char c = editorReadKey();
  switch (c)
/* This mirrors what the Ctrl key does in the terminal: it strips bits 5 and 6 from whatever key you press in combination with Ctrl, and sends that. */
  {
	case CTRL_KEY('q'):
	    write(STDOUT_FILENO, "\x1b[2J", 4);
	    write(STDOUT_FILENO, "\x1b[H", 3);
      exit(0);
      break;
  }
}

/* start */

void initEditor() //Initialize the editor by fetching the windows size
{
  if(getWindowSize(&E.screenrows, &E.screencols) == -1) 
	  die("getWindowSize");
}

int main()
{
	enableRawMode();
	initEditor();

	while(1)
	{
	        editorRefreshScreen();
		editorProcessKeypress();
	}
	return 0;
}


