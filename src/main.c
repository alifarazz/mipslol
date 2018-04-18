#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#define MAXN 1000
#define MAXMEM 1000
#define FILE_BUFFER_SIZE 1000

int32_t g_regfile[32];
int32_t g_instructions[MAXN];
int8_t *g_mainmem;

int32_t g_instructions_len;

char g_char_buf[FILE_BUFFER_SIZE];

int32_t g_prog_counter;

/**
 * C++ version 0.4 char* style "itoa":
 * Written by Lukás Chmela
 * Released under GPLv3.

 */
char* itoa(int value, char* result, int base) {
  // check that the base if valid
  if (base < 2 || base > 36) { *result = '\0'; return result; }

  char* ptr = result, *ptr1 = result, tmp_char;
  int tmp_value;

  do {
    tmp_value = value;
    value /= base;
    *ptr++ = "zyxwvutsrqponmlkjihgfedcba9876543210123456789abcdefghijklmnopqrstuvwxyz" [35 + (tmp_value - value * base)];
  } while ( value );

  // Apply negative sign
  if (tmp_value < 0) *ptr++ = '-';
  *ptr-- = '\0';
  while(ptr1 < ptr) {
    tmp_char = *ptr;
    *ptr--= *ptr1;
    *ptr1++ = tmp_char;
  }
  return result;
}



void* lol_malloc(size_t length, size_t size)
{
  size_t x = length * size;
  if (length != 0 && x / length != size) {
    fprintf(stderr,
	    "malloc failed.\n%s\n\tlength: %lu, size: %lu\n",
	    "Reason: memory overflow", length, size);
    exit(EXIT_FAILURE);
  }
  void* mp = malloc(x);
  if (mp == NULL) {
    fprintf(stderr,
	    "malloc failed.\n%s\n\t length: %lu, size: %lu\n",
	    "malloc returned NULL", length, size);
    exit(EXIT_FAILURE);
  }
  return mp;
}

static int32_t
convert_str_bin(const char *g_char_buf)
{
  int32_t res = 0x0000;
  int i;

  for (i = 0; i < 8 * (int)sizeof(int32_t); i++) {
    res <<= 1;			/* add zero to right */
    if (g_char_buf[i] == '1')	/* toggle LSB bit */
      res |= 1;
  }
  return res;
}

    /* itoa(res, buffer, 2); */
    /* printf("%s\n", buffer); */

int main()
{
  FILE *fp;
  int i;

  g_mainmem = lol_malloc(MAXMEM, sizeof(int8_t));

  if ((fp = fopen("/home/luce/workspace/mipslol/src/ins.txt", "r")) == NULL) {
    perror("fopen() failed");
    exit(EXIT_FAILURE);
  }

  for (i = 0; fgets(g_char_buf, FILE_BUFFER_SIZE, fp) != NULL;) {
    if (g_char_buf[0] == '#')	/* it's a comment */
      continue;

    /* assume anything after 31'th char is comment */
    g_char_buf[32] = '\0';
    g_instructions[i++] = convert_str_bin(g_char_buf);
  } g_instructions_len = i;


  fclose(fp);

  free(g_mainmem);

  return EXIT_SUCCESS;
}
