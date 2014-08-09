/* Initially, this is a whole C command-line program, while I test it.
   Later on, it will become a pebble program.
   I may at some stage spin off an ncurses version of it.
 */

/* fixme: getting stuck on moving up the tree to the top level */
/* todo: do something different on reaching a leaf node */
/* todo: tag-based selection */
/* todo: date/time based selection */
/* todo: changing states, and sending them back */

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>

typedef enum zorg_mode {
  top_level_chooser,
  tree,
  date,
  tag
} zorg_mode;

typedef struct zorg_context {
  zorg_mode mode;
  unsigned int parent;
  unsigned int parent_level;
  unsigned int level;
  int start;
  int end;
  int old_start;
  int old_end;
  unsigned int n_lines;
  char **lines;
  unsigned int display_n_lines;
  int *display_lines;
  int cursor;
} zorg_context;

void
zorg_middle_button(zorg_context *context)
{
  unsigned int scan;

  context->parent = context->display_lines[context->cursor];
  context->parent_level = context->lines[context->parent][0];
  /* Look for a new level; we don't assume it's parent_level+1,
     because the user might be an undisciplined wreck who has
     jumped a level ;-) */
  printf("select/show: current=%d becomes new parent, new parent level=%c, looking for next lower level\n", context->parent, context->parent_level);
  for (scan = context->parent + 1; scan <= context->n_lines; scan++) {
    char margin_char = context->lines[scan][0];
    printf("  considering %s\n", context->lines[scan]);
    if (margin_char < '0' || margin_char > '9') {
      printf("Skipping non-heading %d: %s\n", scan, context->lines[scan]);
      continue;		/* not a heading line */
    }
    if (margin_char > context->parent_level) {
      context->level = margin_char;
      context->old_start = context->start;
      context->old_end = context->end;
      printf("Found new level=%c, remembering old start=%d old end=%d\n", context->level, context->old_start, context->old_end);
      context->start = context->end = -1;
      break;
    }
  }
}

void
zorg_back_button(zorg_context *context)
{
  int scan;
  printf("going up; level=%c parent_level=%c cursor=%d parent=%d\n", context->level, context->parent_level, context->cursor, context->parent);
  for (scan = context->parent; scan >= 0; scan--) {
    char margin_char = context->lines[scan][0];
    printf("Trying %d, level is %c, looking for level < %c: %s\n", scan, margin_char, context->parent_level, context->lines[scan]);
    if (margin_char < '0' || margin_char > '9') {
      printf("Skipping non-heading %d: %s\n", scan, context->lines[scan]);
      continue;		/* not a heading line */
    }
    if (margin_char < context->parent_level) {
      printf("Got it\n");
      context->parent = scan;
      context->parent_level = margin_char;
      printf("Got new parent level %c\n", context->parent_level);
      context->level = context->parent_level+1; /* todo: should scan instead of assuming +1 */
      context->old_start = context->start;
      context->old_end = context->end;
      context->start = context->end = -1;
      break;
    }
  }
}

void
zorg_pebble_rescan(zorg_context *context)
{
  int scan;

  printf("rescanning for start and end, parent=%d\n", context->parent);
  if (context->parent >= context->end) {
    printf("seem to have gone off end\n");
  }
  context->display_n_lines = 0;

  for (scan = context->parent; scan < context->n_lines; scan++) {
    char margin_char;
    if (context->lines[scan] == NULL) {
      break;
    }
    margin_char = context->lines[scan][0];
    printf("  scan %d: %s (level %c, want %c)\n", scan, context->lines[scan], margin_char, context->level);
    if (margin_char < '0' || margin_char > '9') {
      printf("  Skipping non-heading %d: %s\n", scan, context->lines[scan]);
      continue;		/* not a heading line */
    }
    if (margin_char == context->level) {
      printf("  got one at our level: line %d is display line %d\n", scan, context->display_n_lines);
      context->display_lines[context->display_n_lines++] = scan;
      if (context->start == -1) {
	printf("Got start %d: %s\n", scan, context->lines[scan]);
	context->start = scan;
      }
    }
    if ((margin_char < context->level) && (scan != context->parent)) {
      printf("Gone out a level (on %c, outside %c), stopping scan at %d: %s\n", margin_char, context->level, scan, context->lines[scan]);
      break;		/* gone out a level */
    }
    context->end = scan;		/* trails one behind */
  }
  context->display_lines[context->display_n_lines] = -1;
  context->cursor = 0;
}

char *chooser_entries[] = {
  "Tree",
  "Date",
  "Tag"
};

char *
zorg_pebble_display_line(zorg_context *context, unsigned int index)
{
  switch (context->mode) {
  case top_level_chooser:
    return chooser_entries[index];
  case tree:
    return context->lines[context->display_lines[index]];
  case date:
    return "date mode not implemented";
  case tag:
    return "tag mode not implemented";
  }
}

int
zorg_pebble_display_n_lines(zorg_context *context)
{
  switch (context->mode) {
  case top_level_chooser:
    return 2;			/* fixme: there's a sizeof-like thing I can use, I think */
  case tree:
    return context->display_n_lines;
  case date:
    return 0;
  case tag:
    return 0;
  }
}

int
main(int argc, char **argv, char **env)
{
  struct stat stat_buf;
  char *file_name;
  unsigned int file_size;
  int fd;
  char *file_data;
  unsigned int i, j;
  zorg_context context;

  context.mode = tree;
  context.n_lines = 2;		/* for initial line, and final line */

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
      context.n_lines++;
      // fprintf(stderr, "line %d\n", context.n_lines);
    }
  }

  context.lines = (char**)malloc(context.n_lines * sizeof(char*));
  if (context.lines == NULL) {
    fprintf(stderr, "Could not allocate lines array\n");
    exit(1);
  }

  context.display_lines = (int*)malloc(context.n_lines * sizeof(int));
  if (context.display_lines == NULL) {
    fprintf(stderr, "Could not allocate displayed lines array\n");
    exit(1);
  }

  printf("allocated display_lines=%p, lines is %p\n", context.display_lines, context.lines);

  context.lines[0] = file_data;

  for (j = 1, i = 1; i < file_size; i++) {
    if (file_data[i] == '\0') {
      // fprintf(stderr, "recording line %d at offset %d (%p)\n", j, i, &(file_data[i+1]));
      context.lines[j++] = &(file_data[i+1]);
    }
  }

  fprintf(stderr, "Setting end marker in %d\n", j-1);
  context.lines[j-1] = NULL;

#if 1
  for (i = 0; i < context.n_lines; i++) {
    printf("Line %d: %s\n", i, context.lines[i]);
  }
#endif
  printf("about to start main block, display_lines=%p lines=%p\n", context.display_lines, context.lines);

  {
    int running;
    int first = 1;

    context.parent = 0; /* we display the children of this */
    context.parent_level = '0';
    context.start = -1;		/* -1 for unset */
    context.end = -1;		/* -1 for unset */
    context.old_start = context.start;
    context.old_end = context.end;

    for (running = 1; running;) {
      char command = first ? ' ' : getchar();
      unsigned int scan;
      first = 0;
      context.level = context.parent_level + 1;
      if ((command != ' ') && (command != '\n')) {
	printf("\n\ncommand='%c'; parent=%d; start=%d; cursor=%d; end=%d; parent_level=%c; level=%c\n",
	       command, context.parent, context.start, context.cursor, context.end, context.parent_level, context.level);
      }

      if ((context.start == -1) || (context.end == -1)) {
	zorg_pebble_rescan(&context);
      }

      if (context.start == -1) {
	printf("failed to set start\n");
	/* todo: handle leaf node */
	context.start = context.old_start;
	context.end = context.old_end;
      }

      {
	int i;
	for (i = 0; i < zorg_pebble_display_n_lines(&context); i++) {
	  printf("%s display % 3d (line % 3d): %s\n",
		 (i == context.cursor) ? "==>" : "   ",
		 i, context.display_lines[i],
		 zorg_pebble_display_line(&context, i));
	}

      }

      switch (command) {
      case 'q':			/* quit; on pebble, this will be "back" button when at top level */
	running = 0;
	break;

      case 'f':			/* forward; on pebble, "down" button */
	printf("forward from %d, limit %d, level=%c\n", context.cursor, context.end, context.level);
	{
	  int max_lines =  zorg_pebble_display_n_lines(&context);
	  context.cursor++;
	  if (context.cursor >= max_lines ) {
	    context.cursor = max_lines - 1;
	  }
	}
	printf("cursor now %d\n", context.cursor);
	break;

      case 'b':			/* backward; on pebble, "up" button */
	printf("backward from %d, limit %d, level=%c\n", context.cursor, context.end, context.level);
	context.cursor--;
	if (context.cursor < 0) {
	  context.cursor = 0;
	}
	printf("cursor now %d\n", context.cursor);
	break;

      case 's':			/* select: show, or down a level; on pebble, "select" button */
	zorg_middle_button(&context);
	break;

      case 'u':			/* up a level; on pebble, "back" button */
	zorg_back_button(&context);
	break;
      }
    }
  }

  free(context.lines);
  free(context.display_lines);
  free(file_data);

  exit(0);
}
