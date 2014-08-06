/* Initially, this is a whole C command-line program, while I test it.
   Later on, it will become a pebble program. */

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>

int
main(int argc, char **argv, char **env)
{
  struct stat stat_buf;
  char *file_name;
  unsigned int file_size;
  int fd;
  char *file_data;
  unsigned int i, j;
  unsigned int n_lines = 2;	/* initial line, and null line at end */
  char **lines;

  if (argc != 2) {
    fprintf(stderr, "Usage: zorg-pebble file\n");
    exit(1);
  }

  file_name = argv[1];
  if (stat(file_name, &stat_buf) != 0) {
    fprintf(stderr, "Could not stat %s\n", file_name);
    exit(1);
  }

  file_size = stat_buf.st_size;

  file_data = (char*)malloc(file_size+1);

  if (file_data == NULL) {
    fprintf(stderr, "Could not malloc buffer of %d bytes\n", file_size+1);
    exit(1);
  }

  fd = open(file_name, O_RDONLY);
  
  if (fd == -1) {
    fprintf(stderr, "Could not open file %s\n", file_name);
    exit(1);
  }

  if (read(fd, file_data, file_size) != file_size) {
    fprintf(stderr, "Could not read whole file %s (%d bytes)\n", file_name, file_size);
    exit(1);
  }

  close(fd);

  fprintf(stderr, "Read file of %d bytes\n", file_size);

  file_data[file_size] = '\0';

  /* split buffer into lines */
  for (i = 0; i < file_size; i++) {
    if (file_data[i] == '\n') {
      file_data[i] = '\0';
      n_lines++;
      fprintf(stderr, "line %d\n", n_lines);
    }
  }

  lines = (char**)malloc(n_lines * sizeof(char*));
  if (lines == NULL) {
    fprintf(stderr, "Could not allocate lines array\n");
    exit(1);
  }
  
  lines[0] = file_data;

  for (j = 0, i = 1; i < file_size; i++) {
    if (file_data[i] == '\0') {
      fprintf(stderr, "recording line %d at offset %d (%p)\n", j, i, &(file_data[i+1]));
      lines[j++] = &(file_data[i+1]);
    }
  }
  
  fprintf(stderr, "Setting end marker in %d\n", j-1);
  lines[j-1] = NULL;

  for (i = 1; i < n_lines; i++) {
    printf("Line %d: %s\n", i, lines[i]);
  }

  {
    int running;
    unsigned int start = 0;
    unsigned int cursor = start;
    unsigned int level = lines[start][0];

    for (running = 1; running;) {
      char command = getchar();
      unsigned int scan;
      printf("command %c; start %d; level %c\n", command, start, level);
      printf("start line: %s%s\n", lines[start], (start == cursor) ? "<==" : "");
      for (scan = start; scan < n_lines; scan++) {
	if (lines[scan][0] < level) {
	  break;
	}
	if (lines[scan][0] == level) {
	  printf("sibling line: %s%s\n", lines[scan], (start == cursor) ? "<==" : "");
	}
      }
      switch (char) {
      case 'q':			/* quit; on pebble, this will be "back" button when at top level */
	running = 0;
	break;
      case 'f':			/* forward; on pebble, "down" button */
	/* todo: move cursor to next at same level, stopping when level goes out */
	break;
      case 'b':			/* backward; on pebble, "up" button */
	/* todo: move cursor to previous at same level, stopping when level goes out */
	break;
      case 'd':			/* down a level; on pebble, "select" button */
	break;
      case 'u':			/* up a level; on pebble, "back" button */
	/* todo: go out a level, or if already at top level, quit */
	break;
      }
    }
  }

  free(lines);
  free(file_data);

  exit(0);
}
