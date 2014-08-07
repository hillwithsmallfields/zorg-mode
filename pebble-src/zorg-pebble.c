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
      // fprintf(stderr, "line %d\n", n_lines);
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
      // fprintf(stderr, "recording line %d at offset %d (%p)\n", j, i, &(file_data[i+1]));
      lines[j++] = &(file_data[i+1]);
    }
  }

  fprintf(stderr, "Setting end marker in %d\n", j-1);
  lines[j-1] = NULL;

#if 0
  for (i = 1; i < n_lines; i++) {
    printf("Line %d: %s\n", i, lines[i]);
  }
#endif

  {
    int running;
    int start = -1;		/* -1 for unset */
    int end = -1;		/* -1 for unset */
    unsigned int cursor = start;
    unsigned int parent = 0; /* we display the children of this */
    unsigned int parent_level = '0';
    unsigned int level;
    int first = 1;

    for (running = 1; running;) {
      char command = first ? ' ' : getchar();
      unsigned int scan;
      first = 0;
      level = parent_level + 1;
      printf("\n\ncommand='%c'; parent=%d; start=%d; cursor=%d; end=%d; parent_level=%c; level=%c\n",
	     command, parent, start, cursor, end, parent_level, level);
      if ((start == -1) || (end == -1)) {
	printf("rescanning for start and end\n");
	if (parent >= end) {
	  printf("seem to have gone off end\n");
	}
	for (scan = parent + 1; scan < n_lines; scan++) {
	  char margin_char;
	  if (lines[scan] == NULL) {
	    break;
	  }
	  printf("scan %d: %s\n", scan, lines[scan]);
	  margin_char = lines[scan][0];
	  if (margin_char < '0' || margin_char > '9') {
	    printf("Skipping non-heading %d: %s\n", scan, lines[scan]);
	    continue;		/* not a heading line */
	  }
	  if ((start == -1) && (margin_char == level)) {
	    printf("Got start %d: %s\n", scan, lines[scan]);
	    cursor = start = scan;
	  }
	  if (margin_char < level) {
	    printf("Gone out a level, stopping scan at %d: %s\n", scan, lines[scan]);
	    break;		/* gone out a level */
	  }
	  end = scan;		/* trails one behind */
	}
      }

      if (start == -1) {
	printf("failed to set start\n");
	/* todo: not sure! */
	exit(2);
      }

      printf("parent line: %s\n", lines[parent]);
      for (scan = start; scan <= end; scan++) {
	if (lines[scan][0] == level) {
	  printf("sibling line: %s%s\n", lines[scan], (scan == cursor) ? "<==" : "");
	}
      }

      switch (command) {
      case 'q':			/* quit; on pebble, this will be "back" button when at top level */
	running = 0;
	break;

      case 'f':			/* forward; on pebble, "down" button */
	for (; cursor <= end; cursor++) {
	  char margin_char = lines[cursor][0];
	  if (margin_char < '0' || margin_char > '9') {
	    printf("Skipping non-heading %d: %s\n", scan, lines[scan]);
	    continue;		/* not a heading line */
	  }
	  if (margin_char == level) {
	    break;
	  }
	}
	break;

      case 'b':			/* backward; on pebble, "up" button */
	for (; cursor >= start; cursor--) {
	  char margin_char = lines[cursor][0];
	  if (margin_char < '0' || margin_char > '9') {
	    printf("Skipping non-heading %d: %s\n", scan, lines[scan]);
	    continue;		/* not a heading line */
	  }
	  if (margin_char == level) {
	    break;
	  }
	}
	break;

      case 's':			/* select: show, or down a level; on pebble, "select" button */
	parent = cursor;
	parent_level = level;

	for (; cursor <= end; cursor++) {
	  char margin_char = lines[cursor][0];

	  if (margin_char < '0' || margin_char > '9') {
	    printf("Skipping non-heading %d: %s\n", scan, lines[scan]);
	    continue;		/* not a heading line */
	  }
	  if (margin_char > level) {
	    level = margin_char;
	    start = end = -1;
	    break;
	  }
	}
	break;

      case 'u':			/* up a level; on pebble, "back" button */
	for (cursor = start; cursor >= 0; cursor--) {
	  char margin_char = lines[cursor][0];
	  if (margin_char < '0' || margin_char > '9') {
	    printf("Skipping non-heading %d: %s\n", scan, lines[scan]);
	    continue;		/* not a heading line */
	  }
	  if (margin_char < level) {
	    parent = cursor;
	    parent_level = margin_char;
	    start = end = -1;
	    break;
	  }
	}
	break;
      }
    }
  }

  free(lines);
  free(file_data);

  exit(0);
}
