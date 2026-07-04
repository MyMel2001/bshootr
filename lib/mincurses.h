#pragma once
// mincurses.h — minimal viable ncurses replacement using POSIX termios + ANSI
// escape sequences.  Only the subset of the ncurses API actually used by
// bshootr is replicated here.
//
// Implemented:  initscr, endwin, cbreak, noecho, curs_set, keypad, nodelay,
//               clear, refresh, getch, getmaxx, getmaxy, mvaddch, mvprintw,
//               stdscr, WINDOW, TRUE, FALSE, ERR, OK

#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>

// ── ncurses-compatible type aliases and constants
// ─────────────────────────────

typedef struct _win_st WINDOW;
struct _win_st {
  int _maxy;
  int _maxx;
  int _delay; // 0 = non-blocking, -1 = blocking
};

#define TRUE 1
#define FALSE 0
#define ERR (-1)
#define OK 0

// ── Internal state ───────────────────────────────────────────────────────────

static WINDOW _stdscr;
static WINDOW *stdscr = &_stdscr;

static struct termios _orig_termios;
static bool _inited = false;

// Double-buffered screen: we write into _buf, then blast it to the terminal
// with refresh().  This avoids flicker and matches ncurses semantics.
static char *_buf = nullptr;   // flat buffer: rows * cols
static char *_dirty = nullptr; // 1 if cell differs from previous frame
static char *_prev = nullptr;  // previous frame content
static int _bufRows = 0;
static int _bufCols = 0;

// ── Helpers ──────────────────────────────────────────────────────────────────

static void _resizeBuf() {
  struct winsize ws;
  ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws);
  _stdscr._maxy = ws.ws_row;
  _stdscr._maxx = ws.ws_col;

  if (_bufRows != _stdscr._maxy || _bufCols != _stdscr._maxx) {
    free(_buf);
    free(_dirty);
    free(_prev);
    _bufRows = _stdscr._maxy;
    _bufCols = _stdscr._maxx;
    size_t n = (size_t)(_bufRows * _bufCols);
    _buf = (char *)malloc(n);
    memset(_buf, ' ', n);
    _dirty = (char *)malloc(n);
    memset(_dirty, 1, n); // all dirty on resize
    _prev = (char *)malloc(n);
    memset(_prev, 0, n);
  }
}

// ── Public API ───────────────────────────────────────────────────────────────

static WINDOW *initscr() {
  if (_inited)
    return stdscr;

  // Save original terminal settings
  tcgetattr(STDIN_FILENO, &_orig_termios);

  // Switch to raw-ish mode: no echo, no canonical, no signals
  struct termios raw = _orig_termios;
  raw.c_lflag &= ~(ECHO | ICANON | ISIG);
  raw.c_cc[VMIN] = 0;
  raw.c_cc[VTIME] = 0;
  tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);

  _resizeBuf();

  // Enter alternate screen buffer + hide cursor
  write(STDOUT_FILENO, "\033[?1049h\033[?25l", 14);

  _inited = true;
  return stdscr;
}

static int endwin() {
  if (!_inited)
    return OK;

  // Restore original terminal settings
  tcsetattr(STDIN_FILENO, TCSAFLUSH, &_orig_termios);

  // Leave alternate screen + show cursor
  write(STDOUT_FILENO, "\033[?1049l\033[?25h", 14);

  free(_buf);
  _buf = nullptr;
  free(_dirty);
  _dirty = nullptr;
  free(_prev);
  _prev = nullptr;
  _bufRows = 0;
  _bufCols = 0;

  _inited = false;
  return OK;
}

static int cbreak() { return OK; }      // already in raw mode from initscr
static int noecho() { return OK; }      // already suppressed echo
static int curs_set(int) { return OK; } // cursor hidden in initscr

static int keypad(WINDOW *, int) { return OK; } // no-op; we read raw bytes
static int nodelay(WINDOW *win, int bf) {
  win->_delay = bf ? 0 : -1;
  return OK;
}

static int clear() {
  if (!_inited)
    return ERR;
  _resizeBuf(); // pick up terminal resize
  size_t n = (size_t)(_bufRows * _bufCols);
  memset(_buf, ' ', n);
  memset(_dirty, 1, n);
  return OK;
}

static int refresh() {
  if (!_inited)
    return ERR;

  // Build a minimal diff output: only write cells that changed.
  // We move the cursor and write characters, grouping runs on the same row.
  for (int y = 0; y < _bufRows; y++) {
    int runStart = -1;
    for (int x = 0; x <= _bufCols; x++) {
      int idx = y * _bufCols + x;
      bool cellDirty = (x < _bufCols) ? (_dirty[idx] != 0) : false;

      if (cellDirty) {
        if (runStart < 0)
          runStart = x;
      } else {
        if (runStart >= 0) {
          // Emit ANSI cursor move + run of characters
          char hdr[32];
          int hdrlen =
              snprintf(hdr, sizeof(hdr), "\033[%d;%dH", y + 1, runStart + 1);
          write(STDOUT_FILENO, hdr, (size_t)hdrlen);

          int runLen = x - runStart;
          write(STDOUT_FILENO, &_buf[y * _bufCols + runStart], (size_t)runLen);

          // Mark clean
          for (int rx = runStart; rx < x; rx++) {
            int ri = y * _bufCols + rx;
            _prev[ri] = _buf[ri];
            _dirty[ri] = 0;
          }
          runStart = -1;
        }
      }
    }
  }
  return OK;
}

static int getch() {
  if (!_inited)
    return ERR;

  unsigned char c;
  ssize_t n = read(STDIN_FILENO, &c, 1);
  if (n <= 0) {
    if (_stdscr._delay == 0)
      return ERR; // non-blocking: no input
    // blocking: spin until we get something
    while (true) {
      n = read(STDIN_FILENO, &c, 1);
      if (n > 0)
        break;
      usleep(1000); // 1 ms poll
    }
  }
  return (int)c;
}

static int getmaxx(WINDOW *win) {
  if (!win)
    return 0;
  _resizeBuf(); // pick up resize
  return win->_maxx;
}

static int getmaxy(WINDOW *win) {
  if (!win)
    return 0;
  _resizeBuf(); // pick up resize
  return win->_maxy;
}

static int mvaddch(int y, int x, char ch) {
  if (!_inited)
    return ERR;
  if (y < 0 || y >= _bufRows || x < 0 || x >= _bufCols)
    return ERR;
  int idx = y * _bufCols + x;
  if (_buf[idx] != ch || _dirty[idx]) {
    _buf[idx] = ch;
    _dirty[idx] = 1;
  }
  return OK;
}

static int mvprintw(int y, int x, const char *fmt, ...) {
  if (!_inited)
    return ERR;
  if (y < 0 || y >= _bufRows || x < 0 || x >= _bufCols)
    return ERR;

  char tmp[1024];
  va_list ap;
  va_start(ap, fmt);
  vsnprintf(tmp, sizeof(tmp), fmt, ap);
  va_end(ap);

  for (int i = 0; tmp[i] && (x + i) < _bufCols; i++) {
    mvaddch(y, x + i, tmp[i]);
  }
  return OK;
}
