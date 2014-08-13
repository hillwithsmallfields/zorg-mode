/* Initially, this is a whole C command-line program, while I test it.
   Later on, it will become a pebble program.
   I may at some stage spin off an ncurses version of it.
 */

/* todo: do something different on reaching a leaf node */
/* todo: tag-based selection */
/* todo: date/time based selection */
/* todo: changing states, and sending them back using pebble's DataLogging */
/* todo: live data */
/* todo: settings */

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <dirent.h>
#include <string.h>

typedef enum zorg_mode {
  top_level_chooser,
  file_chooser,
  tree,
  date,
  tag,
  tag_chooser,			/* used in the lead-in to tag mode */
  live_data,
  settings
} zorg_mode;

typedef enum data_source_type {
  local_file,
  remote_stream,
  none
} data_source_type;

typedef struct zorg_top_level_item {
  zorg_mode mode;
  char *label;
} zorg_top_level_item;

struct zorg_top_level_item top_level_items[] = {
  { tree, "Tree" },
  { file_chooser, "Files" },
  { date, "Date" },
  { tag_chooser, "Tags" },
  { live_data, "Live data" },
  { settings, "Settings" },
  { top_level_chooser, NULL }};

zorg_mode mode;

data_source_type data_source;

char *currently_selected_file = NULL;

/* The whole data read from file or connection.
   This is a file as prepared by the companion emacs-lisp code.
*/
char *file_data;
unsigned int file_size;

/* The data parsed into an array of lines.
   The data gets split in place, by poking null characters into it.
   Heading lines (in the org-mode sense) begin with a digit, which indicates the tree level.
   The level digit is followed by some optional keyword and tag data.
   Then comes the actual heading text.
   Lines beginning with a space are body lines (shown in the leaf-mode display only).
   A line beginning with a '!' character holds the array of keywords.
   A line beginning with a ':' character holds the array of tags.
 */
unsigned int n_lines;
char **lines;

/* Variables for the tree-mode display. */
unsigned int parent;
unsigned int parent_level;
unsigned int level;
int start;
int end;
int old_start;
int old_end;

/* The keywords line is split into an array of strings. */
char *keywords_line = NULL;
unsigned int n_keywords = 0;
char **keywords = NULL;

/* The tags lines is split into an array of strings. */
char *tags_line = NULL;
unsigned int n_tags = 0;
char **tags = NULL;
int chosen_tag = -1;

/* The lines which are actually displayed, as indices into the main "lines" array. */
unsigned int display_n_lines;
int *display_lines;
int cursor;

char *zorg_dir_name = "/tmp";

static void unload_data();

static void
set_mode(zorg_mode new_mode)
{
  /* undo anything from the old mode */
  switch (mode) {
  case live_data:
    /* todo: unregister event handlers */
    break;
  }

  mode = new_mode;
  cursor = 0;			/* jump back to the top */
  switch (new_mode) {
  case top_level_chooser:
    display_n_lines = (sizeof(top_level_items) / sizeof(top_level_items[0])) - 1;
    /* doesn't use display_lines, as zorg_pebble_display_line returns
       a string from top_level_items directly */
    display_lines = NULL;
    break;
  case tree:
    parent = 0;
    parent_level = '0';
    level = parent_level + 1;
    start = end = -1;
    break;
  case file_chooser:
    {
      struct dirent *dir_buf;
      DIR *dir = opendir(zorg_dir_name);
      unsigned int i;
      char *p;
      n_lines = 0;
      file_size = 0;
      unload_data();
      while (dir_buf = readdir(dir)) {
	char *name = dir_buf->d_name;
	int name_len = strlen(name);
	if (strncmp(name+name_len-5, ".zorg", 5) == 0) {
	  printf("name = %s\n", name);
	  n_lines++;
	  file_size += name_len + 1;
	}
      }
      rewinddir(dir);
      
      printf("%d matches, %d bytes\n", n_lines, file_size);
      lines = (char**)malloc(n_lines*sizeof(char*));
      file_data = (char*)malloc(file_size);
      display_lines = NULL;	/* not using this */

      i = 0; p = file_data;
      while (dir_buf = readdir(dir)) {
	char *name = dir_buf->d_name;
	int name_len = strlen(name);
	if (strncmp(name+name_len-5, ".zorg", 5) == 0) {
	  strcpy(p, name);
	  lines[i] = p;
	  printf("[%d] p = %p = %s\n", i, lines[i], lines[i]);
	  p += name_len + 1;
	  i++;
	  printf("name = %s\n", name);
	}
      }
      closedir(dir);
    }
    {
      unsigned int i;
      for (i = 0; i < n_lines; i++) {
	printf("Line %d: %p %s\n", i, lines[i], lines[i]);
      }
    }
    break;
  case date:
    printf("date mode not implemented\n");
    set_mode(top_level_chooser);
    break;
  case tag_chooser:
    chosen_tag = -1;
    break;
  case tag:
    printf("tags mode not implemented\n");
    set_mode(top_level_chooser);
    break;
  case live_data:
    /* todo: any live data initialization (register event handlers etc) */
    break;
  case settings:
    printf("settings mode not implemented\n");
    set_mode(top_level_chooser);
    break;
  }
 }

static void load_local_file(char *filename);

static void
zorg_middle_button()
{
  unsigned int scan;

  switch (mode) {
  case top_level_chooser:
    set_mode(top_level_items[cursor].mode);
    break;
  case tree:
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
  case file_chooser:
    /* todo: change the current file */
    {
      printf("Selecting file %s\n", lines[cursor]);
      char *new_file = lines[cursor];
      currently_selected_file = (char*)malloc(strlen(new_file) + strlen(zorg_dir_name) + 1);
      sprintf(currently_selected_file, "%s/%s", zorg_dir_name, new_file);
      unload_data();
      data_source = local_file;
      load_local_file(currently_selected_file);
      set_mode(tree);
      free(currently_selected_file);
      currently_selected_file = NULL;
    }
    break;
  case date:
    break;
  case tag_chooser:
    chosen_tag = cursor;
    printf("entering tags mode with chosen tag %d = %s\n", chosen_tag, tags[chosen_tag]);
    set_mode(tag);
    break;
  case tag:
    break;
  case live_data:
    /* maybe update, but it should probably do that on ticks anyway */
    break;
  case settings:
    break;
  }
}

static void
zorg_back_button()
{
  int scan;
  switch (mode) {
  case top_level_chooser:
    printf("this would quit\n");
    break;
  case file_chooser:
    set_mode(top_level_chooser);
    break;
  case tree:
    printf("going up; level=%c parent_level=%c cursor=%d parent=%d\n", level, parent_level, cursor, parent);
    if (parent_level == '0') {
      set_mode(top_level_chooser);
    } else {
      for (scan = parent; scan >= 0; scan--) {
	char margin_char = lines[scan][0];
	printf("Trying %d, level is %c, looking for level < %c: %s\n", scan, margin_char, parent_level, lines[scan]);
	if (margin_char < '0' || margin_char > '9') {
	  printf("Skipping non-heading %d: %s\n", scan, lines[scan]);
	  continue;		/* not a heading line */
	}
	if (parent_level == '1') {
	  /* go to the top level now; there's no actual parent node for
	     this, so recognize it explictly */
	  level = '1';
	  parent_level = '0';
	  parent = 0;
	} else {
	  if (margin_char < parent_level) {
	    level = parent_level;	/* new level is the old parent level */
	    parent = scan;
	    parent_level = (scan == 0) ? '0' : margin_char;
	    printf("Got new parent level %c; new current level is %c\n", parent_level, level);
	    break;
	  }
	}
	old_start = start;
	old_end = end;
	start = end = -1;
      }
    }
    break;
  case date:
  case tag_chooser:
  case tag:
  case live_data:
    set_mode(top_level_chooser);
    break;
  case settings:
    set_mode(tag_chooser);
    break;
  }
}

static void
zorg_pebble_rescan_tree_level()
{
  int scan;

  printf("rescanning for start and end, parent=%d\n", parent);
  if (parent >= end) {
    printf("seem to have gone off end\n");
  }
  display_n_lines = 0;

  for (scan = parent; scan < n_lines; scan++) {
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
    if ((margin_char < level) && (scan != parent)) {
      printf("Gone out a level (on %c, outside %c), stopping scan at %d: %s\n", margin_char, level, scan, lines[scan]);
      break;		/* gone out a level */
    }
    end = scan;		/* trails one behind */
  }
  display_lines[display_n_lines] = -1;
  cursor = 0;
  if (start == -1) {
    printf("failed to set start\n");
    /* todo: handle leaf node */
    start = old_start;
    end = old_end;
  }
}

static char *
zorg_pebble_display_line(unsigned int index)
{
  switch (mode) {
  case top_level_chooser:
    return top_level_items[index].label;
  case file_chooser:
    return lines[index];
  case tree:
    {
      char *line = lines[display_lines[index]];
      for (line++; *line == ' '; line++); /* skip the level, and following whitespace */
      if (*line == '!') {
	for (; *line != ' '; line++); /* skip the keyword */
	for (; *line == ' '; line++); /* skip the space after the keyword */
      }
      if (*line == ':') {
	for (; *line != ' '; line++); /* skip the tags */
	for (; *line == ' '; line++); /* skip the space after the tags */
      }
      return line;
    }
  case date:
    return "date mode not implemented";
  case tag_chooser:
    return tags[index];
  case tag:
    return "tag mode not implemented";
  case live_data:
    switch (index) {
    case 0: return "Time will go here";
    case 1: return "Battery level will go here";
    case 2: return "File name will go here";
    case 3: return "Memory usage will go here";
    default: return "Gone off end!";
    }
  case settings:
    return "settings not implemented";
  }
}

static int
zorg_pebble_display_n_lines()
{
  switch (mode) {
  case top_level_chooser:
    return (sizeof(top_level_items) / sizeof(top_level_items[0])) - 1;
  case tree:
    return display_n_lines;
  case file_chooser:
    return n_lines;
  case date:
    return 0;
  case tag_chooser:
    return n_tags;
  case tag:
    return 0;
  case live_data:
    return 4;
  case settings:
    return 0;
  }
}

static char*
read_local_file(char *file_name, unsigned int *file_size_result)
{
  struct stat stat_buf;
  int fd;
  int data_size;
  char *data_buffer;

  if (stat(file_name, &stat_buf) != 0) {
    fprintf(stderr, "Could not stat %s\n", file_name);
    exit(1);
  }

  data_size = stat_buf.st_size;

  data_buffer = (char*)malloc(data_size+1);

  if (data_buffer == NULL) {
    fprintf(stderr, "Could not malloc buffer of %d bytes\n", data_size+1);
    exit(1);
  }

  fd = open(file_name, O_RDONLY);

  if (fd == -1) {
    fprintf(stderr, "Could not open file %s\n", file_name);
    exit(1);
  }

  if (read(fd, data_buffer, data_size) != data_size) {
    fprintf(stderr, "Could not read whole file %s (%d bytes)\n", file_name, data_size);
    exit(1);
  }

  close(fd);

  fprintf(stderr, "Read file of %d bytes\n", data_size);

  data_buffer[data_size] = '\0';

  *file_size_result = data_size;
  return data_buffer;
}

static int*
parse_data(char *data_buffer, unsigned int data_size, int *line_count_p)
{
  unsigned int i, j;
  char *p;
  unsigned int line_count = 0;
  int *displayed_lines_indices;

  /* split buffer into lines */
  for (i = 0; i < data_size; i++) {
    if (data_buffer[i] == '\n') {
      data_buffer[i] = '\0';
      line_count++;
    }
  }

  lines = (char**)malloc((line_count) * sizeof(char*));
  if (lines == NULL) {
    fprintf(stderr, "Could not allocate lines array\n");
    exit(1);
  }

  displayed_lines_indices = (int*)malloc((line_count) * sizeof(int));
  if (displayed_lines_indices == NULL) {
    fprintf(stderr, "Could not allocate displayed lines array\n");
    exit(1);
  }

  // printf("allocated displayed_lines_indices=%p, lines is %p\n", displayed_lines_indices, lines);

  lines[0] = data_buffer;

  for (j = 1, i = 1; i < data_size; i++) {
    if (data_buffer[i] == '\0') {
      // fprintf(stderr, "recording line %d at offset %d (%p)\n", j, i, &(data_buffer[i+1]));
      lines[j++] = &(data_buffer[i+1]);
    }
  }

  // fprintf(stderr, "Setting end marker in %d\n", j-1);
  lines[j-1] = NULL;

#if 1
  for (i = 0; i < line_count; i++) {
    printf("Line %d: %s\n", i, lines[i]);
  }
#endif
  for (i = 0; i < line_count; i++) {
    // printf("looking at line %d: %s\n", i, lines[i]);
    if (lines[i] != NULL) {
      switch (lines[i][0]) {
      case '!':
	keywords_line = lines[i];
	for (p = keywords_line; *p != '\0'; p++) {
	  if (*p == ' ') {
	    n_keywords++;
	  }
	}
	keywords = (char**)malloc(sizeof(char*) * (n_keywords+1));
	if (keywords == NULL) {
	  printf("Could not allocate keywords array\n");
	  exit(1);
	}
	j = 0;
	for (p = keywords_line; *p != '\0'; p++) {
	  if (*p == ' ') {
	    *p = '\0';
	    printf("Recorded keyword %d as being start of %s\n", j, p+1);
	    keywords[j++] = p+1;
	  }
	}
	break;
      case ':':
	tags_line = lines[i];
	for (p = tags_line; *p != '\0'; p++) {
	  if (*p == ':') {
	    n_tags++;
	  }
	}
	tags = (char**)malloc(sizeof(char*) * (n_tags+1));
	if (tags == NULL) {
	  printf("Could not allocate tags array\n");
	  exit(1);
	}
	j = 0;
	for (p = tags_line; *p != '\0'; p++) {
	  if (*p == ':') {
	    *p = '\0';
	    tags[j++] = p+1;
	  }
	}
	break;
      default:
	break;
      }
    }
  }
#if 1
  if (keywords != NULL) {
    printf("Keywords[%d]:\n", n_keywords);
    for (i = 0; i < n_keywords; i++) {
      printf("  %d: %s\n", i, keywords[i]);
    }
  }
  if (tags != NULL) {
    printf("Tags:\n");
    for (i = 0; i < n_tags; i++) {
      printf("  %d: %s\n", i, tags[i]);
    }
  }
#endif

  *line_count_p = line_count;
  return displayed_lines_indices;
}

static void
load_local_file(char *filename)
{
  printf("Loading data from local file %s\n", filename);
  file_data = read_local_file(filename, &file_size);

  display_lines = parse_data(file_data, file_size, &n_lines);

  data_source = local_file;
}

static void
unload_local_file()
{
  if (file_data != NULL) {
    free(file_data);
    file_data = NULL;
  }
  file_size = 0;
  if (lines != NULL) {
    free(lines);
    lines = NULL;
  }
  n_lines = 0;
  if (display_lines != NULL) {
    free(display_lines);
    display_lines = NULL;
  }
  display_n_lines = 0;

  data_source = none;
}

static void
load_remote_stream(char *stream_name)
{
  /* todo:
     request stream_name from the phone
     get a number of lines
     read all the lines, and parse them
  */

}

static void
unload_remote_stream()
{
}

static void
load_data()
{
  printf("load_data called, file_data=%p currently_selected_file=%s\n", file_data, currently_selected_file);
  if (file_data == NULL) {
    switch (data_source) {
    case local_file:
      load_local_file(currently_selected_file);
      break;
    case remote_stream:
      load_remote_stream(currently_selected_file);
      break;
    }
  }
}

static void
unload_data()
{
  switch (data_source) {
  case local_file:
    unload_local_file();
    break;
  case remote_stream:
    unload_remote_stream();
    break;
  }
}

static void
update_display_lines()
{
  /* pick up any mode change, or any changes within a mode */
  switch (mode) {
  case top_level_chooser:
    break;
  case file_chooser:
    break;
  case tree:
    load_data();
    if ((start == -1) || (end == -1)) {
      zorg_pebble_rescan_tree_level();
    }
    break;
  case date:
    break;
  case tag_chooser:
    break;
  case tag:
    break;
  case live_data:
    break;
  case settings:
    break;
  }
}

int
main(int argc, char **argv, char **env)
{
  mode = top_level_chooser;

  if (argc != 2) {
    fprintf(stderr, "Usage: zorg-pebble file\n");
    exit(1);
  }

  currently_selected_file = argv[1];
  data_source = local_file;

  printf("about to start main block, display_lines=%p lines=%p n_lines=%d\n", display_lines, lines, n_lines);

  {
    int running;
    int first = 1;

    parent = 0; /* we display the children of this */
    parent_level = '0';
    start = -1;		/* -1 for unset */
    end = -1;		/* -1 for unset */
    old_start = start;
    old_end = end;

    for (running = 1; running;) {
      char command = first ? ' ' : getchar();
      unsigned int scan;
      first = 0;
      level = parent_level + 1;
      if ((command != ' ') && (command != '\n')) {
	printf("\n\ncommand='%c'; parent=%d; start=%d; cursor=%d; end=%d; parent_level=%c; level=%c\n",
	       command, parent, start, cursor, end, parent_level, level);
      }

      update_display_lines();

      {
	int i;
	if (mode == tree) {
	  printf("parent %d:  %s\n", parent, lines[parent]);
	}
	for (i = 0; i < zorg_pebble_display_n_lines(); i++) {
	  printf("  %s display % 3d (line % 3d): %s\n",
		 (i == cursor) ? "==>" : "   ",
		 i, (display_lines != NULL) ? display_lines[i] : -1,
		 zorg_pebble_display_line(i));
	}

      }

      switch (command) {
      case 'q':			/* quit; on pebble, this will be "back" button when at top level */
	running = 0;
	break;

      case 'f':			/* forward; on pebble, "down" button */
	printf("forward from %d, limit %d, level=%c\n", cursor, end, level);
	{
	  int max_lines =  zorg_pebble_display_n_lines();
	  cursor++;
	  if (cursor >= max_lines ) {
	    cursor = max_lines - 1;
	  }
	}
	printf("cursor now %d\n", cursor);
	break;

      case 'b':			/* backward; on pebble, "up" button */
	printf("backward from %d, limit %d, level=%c\n", cursor, end, level);
	cursor--;
	if (cursor < 0) {
	  cursor = 0;
	}
	printf("cursor now %d\n", cursor);
	break;

      case 's':			/* select: show, or down a level; on pebble, "select" button */
	zorg_middle_button();
	break;

      case 'u':			/* up a level; on pebble, "back" button */
	zorg_back_button();
	break;
      }
    }
  }

  unload_data();

  exit(0);
}
