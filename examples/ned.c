#include <ctype.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined(__AVR__)
#include <avr/eeprom.h>
#else
#include "../compat/avr/eeprom.h"
#include "../compat/avr/pgmspace.h"
uint8_t nk_sim_eeprom[1024];
#endif

/*
 * ────────────────────────────────────────────────────────────────────
 * ned.c — Nano ED
 * --------------------------------------------------------------------
 * A lightweight line oriented editor intended for small or embedded
 * systems.  The entire file is kept in memory and edited via simple
 * one-letter commands.
 *
 * Commands
 * --------
 *   w            write file
 *   q            quit (discard changes)
 *   wq           write and quit
 *   p            print buffer
 *   e N TEXT     replace line N with TEXT
 *   i N TEXT     insert TEXT before line N
 *   a TEXT       append TEXT at end of buffer
 *   d N          delete line N
 *   s TEXT       search for TEXT
 *   h            help
 *
 * Lines expand dynamically.  Printing performs extremely simple syntax
 * highlighting: lines starting with '#' or '//' appear in yellow and
 * digits are shown in cyan.
 * ────────────────────────────────────────────────────────────────────
 */

#define MAX_LINES 16
#define MAX_LINE_LEN 64

#include "editor_common.h"

struct Buffer {
  char lines[MAX_LINES][MAX_LINE_LEN];
  uint8_t count;
  char filename[32];
};

static void die(const char *msg) {
  perror(msg);
  exit(EXIT_FAILURE);
}

static void buffer_init(struct Buffer *b, const char *path) {
  memset(b, 0, sizeof *b);
  if (path)
    strncpy(b->filename, path, sizeof b->filename - 1);
}

static void buffer_free(struct Buffer *b) { (void)b; /* nothing to free */ }


static void replace_line(struct Buffer *b, uint8_t idx, const char *text) {
  if (idx >= b->count)
    return;
  strncpy(b->lines[idx], text, MAX_LINE_LEN - 1);
  b->lines[idx][MAX_LINE_LEN - 1] = '\0';
}

static void load_file(struct Buffer *b, const char *path) {
  FILE *f = fopen(path, "r");
  if (!f) {
    perror("open");
    return;
  }
  char tmp[MAX_LINE_LEN];
  while (fgets(tmp, sizeof tmp, f) && b->count < MAX_LINES)
    insert_line(b, b->count, tmp);
  fclose(f);
  strncpy(b->filename, path, sizeof b->filename - 1);
}

static void save_file(const struct Buffer *b) {
  if (!b->filename[0])
    return;
  FILE *f = fopen(b->filename, "w");
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


static void print_buffer(const struct Buffer *b) {
  for (uint8_t i = 0; i < b->count; ++i) {
    printf("%3zu: ", (size_t)i + 1);
    highlight(b->lines[i]);
  }
}

static void search(const struct Buffer *b, const char *term) {
  for (uint8_t i = 0; i < b->count; ++i)
    if (strstr(b->lines[i], term))
      printf("%3zu: %s", (size_t)i + 1, b->lines[i]);
}

static const char help_msg[] PROGMEM =
    "Commands: w q wq p e i a d s h E(save EEPROM) L(load EEPROM)\n";

static void help(void) { pgm_print(help_msg); }

int main(int argc, char **argv) {
  struct Buffer buf;
  buffer_init(&buf, argc > 1 ? argv[1] : NULL);
  if (argc > 1)
    load_file(&buf, argv[1]);

  char cmd[256];
  while (printf(": "), fgets(cmd, sizeof cmd, stdin)) {
    if (strncmp(cmd, "q", 1) == 0)
      break;
    if (strncmp(cmd, "wq", 2) == 0) {
      save_file(&buf);
      eeprom_save(&buf);
      break;
    }
    if (strncmp(cmd, "w", 1) == 0) {
      save_file(&buf);
      eeprom_save(&buf);
      continue;
    }
    if (strncmp(cmd, "p", 1) == 0) {
      print_buffer(&buf);
      continue;
    }
    if (cmd[0] == 'e') {
      unsigned int line;
      char text[200];
      if (sscanf(cmd + 1, "%u %[\x00-\xFF]", &line, text) == 2 && line) {
        if (strlen(text) >= MAX_LINE_LEN) {
          set_status_message("Line truncated to %d chars", MAX_LINE_LEN - 1);
          printf("%s\n", status_msg);
        }
        replace_line(&buf, (uint8_t)(line - 1), text);
      }
      continue;
    }
    if (cmd[0] == 'i') {
      unsigned int line;
      char text[200];
      if (sscanf(cmd + 1, "%u %[\x00-\xFF]", &line, text) == 2) {
        if (strlen(text) >= MAX_LINE_LEN) {
          set_status_message("Line truncated to %d chars", MAX_LINE_LEN - 1);
          printf("%s\n", status_msg);
        }
        insert_line(&buf, (uint8_t)(line - 1), text);
      }
      continue;
    }
    if (cmd[0] == 'a') {
      char text[200];
      if (sscanf(cmd + 1, " %[\x00-\xFF]", text) == 1) {
        if (strlen(text) >= MAX_LINE_LEN) {
          set_status_message("Line truncated to %d chars", MAX_LINE_LEN - 1);
          printf("%s\n", status_msg);
        }
        insert_line(&buf, buf.count, text);
      }
      continue;
    }
    if (cmd[0] == 'd') {
      unsigned int line;
      if (sscanf(cmd + 1, "%u", &line) == 1)
        delete_line(&buf, (uint8_t)(line - 1));
      continue;
    }
    if (cmd[0] == 's') {
      char term[64];
      if (sscanf(cmd + 1, " %[\x00-\xFF]", term) == 1)
        search(&buf, term);
      continue;
    }
    if (cmd[0] == 'E') {
      eeprom_save(&buf);
      continue;
    }
    if (cmd[0] == 'L') {
      eeprom_load(&buf);
      continue;
    }
    if (cmd[0] == 'h' || cmd[0] == '?') {
      help();
      continue;
    }
  }

  buffer_free(&buf);
  return 0;
}
