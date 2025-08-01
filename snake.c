#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/select.h>
#include <stdlib.h>
#include <termios.h>

#define COLS 60
#define ROWS 30

#define hide_cursor "\x1b[?25l"
#define show_cursor "\x1b[?25h"
#define cursor_to_top "\x1b[%iA"
#define cursor_to_top2 "\x1b[%iF"
#define game_over_str "\x1b[%iB\x1b[%iC Game Over! "
#define tail_str "\x1b[%iB\x1b[%iC·"
#define head_str "\x1b[%iB\x1b[%iC▓"
#define apple_str "\x1b[%iB\x1b[%iCA"

static struct termios old_t, new_t;
static char buf[COLS * ROWS + 1];
static long x[1024], y[1024];
static long xdir, ydir;
static long head, tail;
static long apple_x, apple_y;
static struct timeval tv;

void init(void)
{
  printf(hide_cursor);
  fflush(NULL);

  /* switch to console mode, disable echo */
  tcgetattr(0, &old_t);
  memcpy(&new_t, &old_t, sizeof(struct termios));
  new_t.c_lflag &= ~(ICANON | ECHO);
  tcsetattr(0, TCSANOW, &new_t);
}

void exit_game(void)
{
  printf(show_cursor);
  fflush(NULL);

  /* restore terminal mode */
  tcsetattr(0, TCSANOW, &old_t);
  exit(0);
}

void render_table(void)
{
  char *p = buf;

  /* Store ┌ (UTF-8 is 3 bytes)*/
  memcpy(p, "┌", sizeof("┌") - 1);
  p += sizeof("┌") - 1;

  /* Loop COLS times to draw horizontal lines */
  for(size_t i = 0; i < COLS; i++) {
    memcpy(p, "─", 3); /* UTF-8: 3 bytes */
    p += 3;
  }

  /* Add top-right corner */
  memcpy(p, "┐", 3);
  p += 3;

  /* add newline */
  *p++ = '\n';

  /* mid line */
  for(size_t row = 0; row < ROWS; row++) {

    /* left border */
    memcpy(p, "│", 3);
    p += 3;
    for(size_t col = 0; col < COLS; col++) {
      memcpy(p, "·", 2);
      p += 2; /* fill with middle dot */
    }

    /* right border */
    memcpy(p, "│", 3);
    p += 3;

    *p++ = '\n';
  }

  /* bottom line*/
  memcpy(p, "└", 3);
  p += 3;

  for(size_t col = 0; col < COLS; col++) {
    memcpy(p, "─", 3);
    p += 3;
  }

  memcpy(p, "┘", 3);
  p += 3;

  /* add newline */
  *p++ = '\n';
  *p = '\0';

  printf("%s", buf);
  printf(cursor_to_top, ROWS + 2);
}

void main_loop(void)
{
  render_table();

  tail = 0;
  head = 0;
  x[0] = COLS / 2;
  y[0] = ROWS / 2;
  xdir = 1; /* moving right */
  ydir = 0;
  apple_x = -1; /* triggers apple spawn later */

  while(1) {
    /* spawn apple if needed */
    if(apple_x < 0) {
      /* create new apple */
      apple_x = rand() % COLS;
      apple_y = rand() % ROWS;

      /* check if apple spawns on the snack */
      size_t i = tail;
      while(i != head) {
        if(x[i] == apple_x && y[i] == apple_y) {
          apple_x = -1;
          break;
        }
        i = (i + 1) & 1023;
      }
    }

    /* draw apple */
    if(apple_x >= 0) {
      printf(apple_str, (int)apple_y + 1, (int)apple_x + 1);
      printf(cursor_to_top2, (int)apple_y + 1);
    }

    /* clear tail */
    size_t t = tail;
    printf(tail_str, (int)y[t] + 1, (int)x[t] + 1);
    printf(cursor_to_top2, (int)y[tail] + 1);

    /* check if apple eaten */
    if(x[head] == apple_x && y[head] == apple_y) {
      apple_x = -1;
    }
    else {
      tail = (tail + 1) & 1023;
    }

    /* move head */
    size_t new_h = (head + 1) & 1023;
    long new_x = (x[head] + xdir + COLS) % COLS;
    long new_y = (y[head] + ydir + ROWS) % ROWS;

    x[new_h] = new_x;
    y[new_h] = new_y;
    head = new_h;

    /* check game over */
    size_t i = tail;
    while(i != head)
    {
      if(x[i] == new_x && y[i] == new_y) {
        printf(game_over_str, ROWS / 2, COLS / 2 - 5);
        fflush(NULL);
        printf(cursor_to_top2, ROWS / 2);
        getchar();
        main_loop();
      }
      i = (i + 1) & 1023;
    }

    /* draw new head */
    printf(head_str, (int)y[head] + 1, (int)x[head] + 1);
    printf(cursor_to_top2, (int)y[head] + 1);
    fflush(NULL);

    /* delay */
    usleep(5 * 1000000 / 60);

    /* read keyboard */
    fd_set readfds;
    FD_ZERO(&readfds);
    FD_SET(0, &readfds);
    tv.tv_sec = 0;
    tv.tv_usec = 0;

    if(select(1, &readfds, NULL, NULL, &tv) > 0) {
      int c = getchar();
      if(c == 27 || c == 'q') exit_game();
      if(c == 'h' && xdir != 1) { xdir = -1; ydir = 0; }
      if(c == 'l' && xdir != -1) { xdir = 1; ydir = 0; }
      if(c == 'j' && ydir != -1) { xdir = 0; ydir = 1; }
      if(c == 'k' && ydir != 1) { xdir = 0; ydir = -1; }
    }
  }
}

int main(void)
{
  init();
  main_loop();
  return 0;
}
