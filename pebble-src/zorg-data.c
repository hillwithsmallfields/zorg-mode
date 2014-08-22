#include <stdlib.h>
#include <stdio.h>

#include "zorg.h"

char filter_search_string[FILTER_SEARCH_STRING_MAX];

zorg_mode mode;

data_source_type data_source;

char *currently_selected_file = NULL;

/* The whole data read from file or connection.
   This is a file as prepared by the companion emacs-lisp code.
*/
char *file_data;
unsigned int file_size;
int file_changed = 0;

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

/* Another set of line, for the directory display.  This is stored
   separately, so that if we go into the directory display and then
   back out of it without selecting a file, the original file can be
   left undisturbed. */

char *directory_data;
unsigned int directory_data_size;
unsigned int n_directory_lines;
char **directory_lines;

char *zorg_dir_name = "/home/jcgs/tmp";

/* This forms the root node of the displayed tree */
struct zorg_top_level_item top_level_items[] = {
  { tree, "Tree" },
  { file_chooser, "Files" },
  { date, "Date" },
  { tag_chooser, "Tags" },
  { live_data, "Live data" },
  { settings, "Settings" },
  { top_level_chooser, NULL }};

void
zorg_pebble_scan_tags()
{
  int scan;
  printf("in zorg_pebble_scan_tags(%s)\n", filter_search_string);
  display_n_lines = 0;

  for (scan = 0; scan < n_lines; scan++) {
    char *p;
    int hit = 0;

    // printf("Looking for tag %s in line %d: %s\n", filter_search_string, scan, lines[scan]);

    for (p = lines[scan]; *p != 0; p++) {
      char c = *p;

      if (c == ':') {
	char *q = filter_search_string;
	char d = *q++;
	p++;			/* skip the ':' */
	c = *p++;
	// printf("comparing %s with %s (%c with %c)\n", q-1, p-1, d, c);
	while (c == d) {
	  c = *p++;
	  d = *q++;
	  /* printf("comparing %c with %c\n", d, c); */
	}
	if (d == '\0') {
	  hit = 1;
	  // printf("matched!\n");
	  break;
	}
      }
    }
    if (hit) {
      display_lines[display_n_lines++] = scan;
      printf("Added line %d to display (now %d lines)\n", scan, display_n_lines);
    }
  }
}

void
zorg_pebble_scan_dates()
{
  int scan;
  printf("in zorg_pebble_scan_dates(%s)\n", filter_search_string);
  display_n_lines = 0;

  for (scan = 0; scan < n_lines; scan++) {
    char *p;
    int hit = 0;

    // printf("Looking for tag %s in line %d: %s\n", filter_search_string, scan, lines[scan]);

    for (p = lines[scan]; *p != 0; p++) {


      /* todo: fill display_lines with just one of each date, in order */

#if 0
      /* from zorg_pebble_scan_tags */
      char c = *p;

      if (c == ':') {
	char *q = filter_search_string;
	char d = *q++;
	p++;			/* skip the ':' */
	c = *p++;
	// printf("comparing %s with %s (%c with %c)\n", q-1, p-1, d, c);
	while (c == d) {
	  c = *p++;
	  d = *q++;
	  /* printf("comparing %c with %c\n", d, c); */
	}
	if (d == '\0') {
	  hit = 1;
	  // printf("matched!\n");
	  break;
	}
      }
#endif
    }
    if (hit) {
      display_lines[display_n_lines++] = scan;
      printf("Added line %d to display (now %d lines)\n", scan, display_n_lines);
    }
  }
}

void
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

char *
zorg_pebble_display_line(unsigned int index)
{
  switch (mode) {
  case top_level_chooser:
    return top_level_items[index].label;
  case file_chooser:
    return directory_lines[index];
  case tree:
  case tag:
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
  printf("Bad mode!");
  exit(1);
}

int
zorg_pebble_display_n_lines()
{
  switch (mode) {
  case top_level_chooser:
    return (sizeof(top_level_items) / sizeof(top_level_items[0])) - 1;
  case tree:
  case tag:
    return display_n_lines;
  case file_chooser:
    return n_directory_lines;
  case date:
    return 0;
  case tag_chooser:
    return n_tags;
  case live_data:
    return 4;
  case settings:
    return 0;
  }
  printf("Bad mode!");
  exit(1);
}

int*
parse_data(char *data_buffer, unsigned int data_size, unsigned int *line_count_p)
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
      case '#':
	/* todo: skip a shebang, read the number of lines and the number of characters */
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

void
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
    case none:
      break;
    }
  }
}

void
unload_data()
{
  switch (data_source) {
  case local_file:
    unload_local_file();
    break;
  case remote_stream:
    unload_remote_stream();
    break;
  case none:
    break;
  }
}
