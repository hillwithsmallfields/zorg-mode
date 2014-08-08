/* Initially, this is a whole C command-line program, while I test it.
   Later on, it will become a pebble program.
   I may at some stage spin off an ncurses version of it.
 */

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
  unsigned int display_n_lines;
  int *display_lines;

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

  display_lines = (int*)malloc(n_lines * sizeof(int));
  if (display_lines == NULL) {
    fprintf(stderr, "Could not allocate displayed lines array\n");
    exit(1);
  }

  lines[0] = file_data;

  for (j = 1, i = 1; i < file_size; i++) {
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
    int old_start = start;
    int old_end = end;

    unsigned int cursor;
    unsigned int parent = 0; /* we display the children of this */
    unsigned int parent_level = '0';
    unsigned int level;
    int first = 1;

    for (running = 1; running;) {
      char command = first ? ' ' : getchar();
      unsigned int scan;
      first = 0;
      level = parent_level + 1;
      if ((command != ' ') && (command != '\n')) {
	printf("\n\ncommand='%c'; parent=%d; start=%d; cursor=%d; end=%d; parent_level=%c; level=%c\n",
	       command, parent, start, cursor, end, parent_level, level);
      }

      if ((start == -1) || (end == -1)) {
	printf("rescanning for start and end, parent=%d\n", parent);
	if (parent >= end) {
	  printf("seem to have gone off end\n");
	}
	display_n_lines = 0;

	for (scan = parent+1; scan < n_lines; scan++) {
	  char margin_char;
	  if (lines[scan] == NULL) {
	    break;
	  }
	  margin_char = lines[scan][0];
	  printf("  scan %d: %s (level %c, want %c)\n", scan, lines[scan], margin_char, level);
	  if (margin_char < '0' || margin_char > '9') {
	    printf("  Skipping non-heading %d: %s\n", scan, lines[scan]);
	    continue;		/* not a heading line */
	  }
	  if (margin_char == level) {
	    printf("  got one at our level: line %d is display line %d\n", scan, display_n_lines);
	    display_lines[display_n_lines++] = scan;
	    if (start == -1) {
	      printf("Got start %d: %s\n", scan, lines[scan]);
	      start = scan;
	    }
	  }
	  if (margin_char < level) {
	    printf("Gone out a level (on %c, outside %c), stopping scan at %d: %s\n", margin_char, level, scan, lines[scan]);
	    break;		/* gone out a level */
	  }
	  end = scan;		/* trails one behind */
	}
	display_lines[display_n_lines] = -1;
	cursor = 0;
      }


      if (start == -1) {
	printf("failed to set start\n");
	/* todo: not sure! */
	start = old_start;
	end = old_end;
      }

      {
	int i;
	for (i = 0; i < display_n_lines; i++) {
	  printf("%s display % 3d (line % 3d): %s\n",
		 (i == cursor) ? "==>" : "   ",
		 i, display_lines[i],
		 lines[display_lines[i]]);
	}

      }

      switch (command) {
      case 'q':			/* quit; on pebble, this will be "back" button when at top level */
	running = 0;
	break;

      case 'f':			/* forward; on pebble, "down" button */
	printf("forward from %d, limit %d, level=%c\n", cursor, end, level);
	cursor++;
	if (cursor >= display_n_lines) {
	  cursor = display_n_lines;
	}
	break;

      case 'b':			/* backward; on pebble, "up" button */
	printf("backward from %d, limit %d, level=%c\n", cursor, end, level);
	cursor--;
	if (cursor < 0) {
	  cursor = 0;
	}
	break;

      case 's':			/* select: show, or down a level; on pebble, "select" button */
	parent = display_lines[cursor];
	parent_level = lines[parent][0];
	/* Look for a new level; we don't assume it's parent_level+1,
	   because the user might be an undisciplined wreck who has
	   jumped a level ;-) */
	printf("select/show: current=%d becomes new parent, new parent level=%c, looking for next lower level\n", parent, parent_level);
	for (scan = parent + 1; scan <= n_lines; scan++) {
	  char margin_char = lines[scan][0];
	  printf("  considering %s\n", lines[scan]);
	  if (margin_char < '0' || margin_char > '9') {
	    printf("Skipping non-heading %d: %s\n", scan, lines[scan]);
	    continue;		/* not a heading line */
	  }
	  if (margin_char > parent_level) {
	    level = margin_char;
	    old_start = start;
	    old_end = end;
	    printf("Found new level=%c, remembering old start=%d old end=%d\n", level, old_start, old_end);
	    start = end = -1;
	    break;
	  }
	}
	break;

      case 'u':			/* up a level; on pebble, "back" button */
	printf("going up; level=%c cursor=%d\n", level, cursor);
	for (scan = parent; scan >= 0; scan--) {
	  char margin_char = lines[scan][0];
	  if (margin_char < '0' || margin_char > '9') {
	    printf("Skipping non-heading %d: %s\n", scan, lines[scan]);
	    continue;		/* not a heading line */
	  }
	  printf("Trying %d: %s\n", scan, lines[scan]);
	  if (margin_char < parent_level) {
	    printf("Got it\n");
	    parent = scan;
	    parent_level = margin_char;
	    level = parent_level+1;
	    old_start = start;
	    old_end = end;
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
