#include <ctype.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <termios.h>
#include <unistd.h>
#include <locale.h>
#include <wchar.h>

#if defined(__AVR__)
#include <avr/eeprom.h>
#else
#include "../compat/avr/eeprom.h"
#include "../compat/avr/pgmspace.h"
uint8_t nk_sim_eeprom[1024];
#endif

#include "editor_utils.h"

/*
 * ────────────────────────────────────────────────────────────────────
 * vini.c — VI Nano Implementation
 * --------------------------------------------------------------------
 * A compact vi-like editor.  The entire file resides in memory and the
 * screen is redrawn after each keystroke.  Only a subset of vi is
 * implemented but the basic modal workflow should feel familiar.
 *
 * Command mode
 *   h j k l  move cursor
 *   i        enter insert mode
 *   x        delete character
 *   o        open line below
 *   dd       delete line
 *   :w/:q    write/quit
 *   /text    search forward
 *
 * Insert mode
 *   ESC      return to command mode
 *   Backspace delete char before cursor
 *
 * Lines starting with '#' or '//' are drawn in yellow.  Digits appear in
 * cyan.  The intent is educational rather than feature parity with real
 * vi.
 * ────────────────────────────────────────────────────────────────────
 */

#define MAX_LINES 14
#define MAX_LINE_LEN 64

struct Buffer {
  char lines[MAX_LINES][MAX_LINE_LEN];
  uint8_t count;
};

static struct termios orig_term;

static void enable_raw(void) {
  struct termios raw;
  tcgetattr(STDIN_FILENO, &orig_term);
  raw = orig_term;
  raw.c_lflag &= ~(ICANON | ECHO);
  tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
}

static void disable_raw(void) {
  tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_term);
}

static void buf_init(struct Buffer *b) { memset(b, 0, sizeof *b); }

static void buf_free(struct Buffer *b) { (void)b; }

static void insert_line(struct Buffer *b, uint8_t idx, const char *text) {
  if (b->count >= MAX_LINES)
    return;
  if (idx > b->count)
    idx = b->count;
  for (uint8_t i = b->count; i > idx; --i)
    memcpy(b->lines[i], b->lines[i - 1], MAX_LINE_LEN);
  strncpy(b->lines[idx], text, MAX_LINE_LEN - 1);
  b->lines[idx][MAX_LINE_LEN - 1] = '\0';
  ++b->count;
}

static void delete_line(struct Buffer *b, uint8_t idx) {
  if (idx >= b->count)
    return;
  for (uint8_t i = idx; i + 1 < b->count; ++i)
    memcpy(b->lines[i], b->lines[i + 1], MAX_LINE_LEN);
  --b->count;
  if (b->count == 0) {
    strncpy(b->lines[0], "\n", MAX_LINE_LEN);
    b->count = 1;
  }
}

static void load_file(struct Buffer *b, const char *path) {
  FILE *f = fopen(path, "r");
  if (!f) {
    perror("open");
    strncpy(b->lines[0], "\n", MAX_LINE_LEN);
    b->count = 1;
    return;
  }
  char tmp[MAX_LINE_LEN];
  while (fgets(tmp, sizeof tmp, f) && b->count < MAX_LINES)
    insert_line(b, b->count, tmp);
  fclose(f);
}

static void save_file(const struct Buffer *b, const char *path) {
  FILE *f = fopen(path, "w");
  if (!f) {
    perror("open");
    return;
  }
  for (uint8_t i = 0; i < b->count; ++i)
    fputs(b->lines[i], f);
  fclose(f);
}

static uint8_t EEMEM ee_buf[1 + MAX_LINES * MAX_LINE_LEN];

static void eeprom_save(const struct Buffer *b) {
  eeprom_update_byte(&ee_buf[0], b->count);
  for (uint8_t i = 0; i < b->count; ++i)
    for (uint8_t j = 0; j < MAX_LINE_LEN; ++j)
      eeprom_update_byte(&ee_buf[1 + i * MAX_LINE_LEN + j],
                         (uint8_t)b->lines[i][j]);
}

static void eeprom_load(struct Buffer *b) {
  b->count = eeprom_read_byte(&ee_buf[0]);
  if (b->count > MAX_LINES)
    b->count = MAX_LINES;
  for (uint8_t i = 0; i < b->count; ++i)
    for (uint8_t j = 0; j < MAX_LINE_LEN; ++j)
      b->lines[i][j] =
          (char)eeprom_read_byte(&ee_buf[1 + i * MAX_LINE_LEN + j]);
}

static char status_msg[64] = "";

static void set_status_message(const char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  vsnprintf(status_msg, sizeof status_msg, fmt, ap);
  va_end(ap);
}

static int display_width(const char *s, size_t byte_offset) {
  int width = 0;
  size_t i = 0;
  setlocale(LC_CTYPE, "");
  while (i < byte_offset && s[i]) {
    if (s[i] == '\t') {
      width += 8 - (width % 8);
      ++i;
    } else {
      wchar_t wc;
      int len = mbtowc(&wc, s + i, MB_CUR_MAX);
      if (len <= 0) {
        ++width;
        ++i;
      } else {
        int w = wcwidth(wc);
        width += (w > 0) ? w : 1;
        i += len;
      }
    }
  }
  return width;
}


static const char ins_str[] PROGMEM = "INSERT";
static const char cmd_str[] PROGMEM = "COMMAND";

static void draw(const struct Buffer *b, uint8_t row, uint8_t col, int mode) {
  printf("\x1b[2J\x1b[H");
  for (uint8_t i = 0; i < b->count; ++i) {
    if (i == row)
      printf("> %3zu ", i + 1);
    else
      printf("  %3zu ", i + 1);
    highlight(b->lines[i]);
    if (i == row && col < strlen(b->lines[i])) {
      int caret_pos = display_width(b->lines[i], col);
      printf("%*s^", caret_pos + 1, "");
    }
    putchar('\n');
  }
  fputs("-- ", stdout);
  if (mode)
    pgm_print(
#ifdef __AVR__
        ins_str
#else
        "INSERT"
#endif
    );
  else
    pgm_print(
#ifdef __AVR__
        cmd_str
#else
        "COMMAND"
#endif
    );
  puts(" --");
  if (status_msg[0])
    puts(status_msg);
  else
    putchar('\n');
}

static char yank[MAX_LINE_LEN] = "";

static void command_loop(struct Buffer *b, const char *path) {
  uint8_t row = 0, col = 0;
  int mode = 0; /* 0=cmd 1=ins */
  int ch;

  enable_raw();
  draw(b, row, col, mode);

  while ((ch = getchar()) != EOF) {
    if (ch == '\x1b') { /* arrow keys */
      char seq[2];
      if (read(STDIN_FILENO, seq, 2) == 2 && seq[0] == '[') {
        if (seq[1] == 'A')
          ch = 'k';
        else if (seq[1] == 'B')
          ch = 'j';
        else if (seq[1] == 'C')
          ch = 'l';
        else if (seq[1] == 'D')
          ch = 'h';
      }
    }

    if (!mode) { /* command */
      static int prev = 0;
      if (ch == 'i') {
        mode = 1;
      } else if (ch == 'h' && col)
        --col;
      else if (ch == 'l' && col + 1 < strlen(b->lines[row]))
        ++col;
      else if (ch == 'j' && row + 1 < b->count) {
        ++row;
        if (col >= strlen(b->lines[row]))
          col = strlen(b->lines[row]);
      } else if (ch == 'k' && row) {
        --row;
        if (col >= strlen(b->lines[row]))
          col = strlen(b->lines[row]);
      } else if (ch == 'x' && col < strlen(b->lines[row])) {
        memmove(&b->lines[row][col], &b->lines[row][col + 1],
                strlen(b->lines[row]) - col);
      } else if (ch == 'o') {
        insert_line(b, row + 1, "\n");
        row++;
        col = 0;
        mode = 1;
      } else if (ch == 'd' && prev == 'd') {
        delete_line(b, row);
        if (row >= b->count)
          row = b->count - 1;
        prev = 0;
      } else if (ch == 'd') {
        prev = 'd';
      } else if (ch == 'y' && prev == 'y') {
        strncpy(yank, b->lines[row], sizeof yank - 1);
        prev = 0;
      } else if (ch == 'y') {
        prev = 'y';
      } else if (ch == 'p' && yank[0]) {
        insert_line(b, row + 1, yank);
      } else if (ch == 'E') {
        eeprom_save(b);
      } else if (ch == 'L') {
        eeprom_load(b);
      } else if (ch == ':') {
        char cmd[32];
        fgets(cmd, sizeof cmd, stdin);
        if (strncmp(cmd, "wq", 2) == 0) {
          save_file(b, path);
          eeprom_save(b);
          break;
        }
        if (cmd[0] == 'w') {
          save_file(b, path);
          eeprom_save(b);
        }
        if (cmd[0] == 'q')
          break;
      } else if (ch == '/') {
        char term[32];
        fgets(term, sizeof term, stdin);
        for (uint8_t i = row + 1; i < b->count; ++i)
          if (strstr(b->lines[i], term)) {
            row = i;
            col = 0;
            break;
          }
      } else {
        prev = 0;
      }
    } else { /* insert mode */
      if (ch == 27) {
        mode = 0;
        continue;
      }
      if (ch == 127 && col) { /* backspace */
        memmove(&b->lines[row][col - 1], &b->lines[row][col],
                strlen(b->lines[row]) - col + 1);
        --col;
        continue;
      }
      if (ch == '\n') {
        char tail[MAX_LINE_LEN];
        strncpy(tail, b->lines[row] + col, MAX_LINE_LEN - 1);
        tail[MAX_LINE_LEN - 1] = '\0';
        b->lines[row][col] = '\n';
        b->lines[row][col + 1] = '\0';
        insert_line(b, row + 1, tail);
        row++;
        col = 0;
        continue;
      }
      uint8_t len = strlen(b->lines[row]);
      if ((uint8_t)(len + 1) < MAX_LINE_LEN) {
        memmove(&b->lines[row][col + 1], &b->lines[row][col], len - col + 1);
        b->lines[row][col] = ch;
        ++col;
      } else {
        set_status_message("Line length limit reached");
      }
    }

    draw(b, row, col, mode);
  }

  disable_raw();
}

int main(int argc, char **argv) {
  setlocale(LC_CTYPE, "");
  if (argc < 2) {
    fprintf(stderr, "Usage: %s <file>\n", argv[0]);
    return 1;
  }

  const char *path = argv[1];
  struct Buffer buf;
  buf_init(&buf);
  load_file(&buf, path);

  command_loop(&buf, path);

  save_file(&buf, path);
  buf_free(&buf);
  return 0;
}
