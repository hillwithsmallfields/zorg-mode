#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "zorg.h"

char filter_search_string[FILTER_SEARCH_STRING_MAX];
unsigned int filter_search_string_length = 0;

zorg_mode mode;

data_source_type data_source;

char *currently_selected_file = NULL;

char *error_message = NULL;

/* The whole data read from file or connection.
   This is a file as prepared by the companion emacs-lisp code.
*/
char *file_data = NULL;
char *data_filling_point = NULL;
unsigned int file_size = 0;
unsigned int allocated_file_size = 0;
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
unsigned int n_lines = 0;
unsigned int allocated_n_lines = 0;
char **lines = NULL;

/* Variables for the tree-mode display. */
unsigned int parent = 0;
unsigned int parent_level = 0;
unsigned int level = 0;
int start = -1;
int end = -1;
int old_start = -1;
int old_end = -1;

/* The keywords line is split into an array of strings. */
char *keywords_line = NULL;
unsigned int n_keywords = 0;
char **keywords = NULL;
int current_keyword = -1;
int original_keyword = -1;
#define KEYWORD_PROXY -42

/* The tags lines is split into an array of strings. */
char *tags_line = NULL;
unsigned int n_tags = 0;
char **tags = NULL;
int chosen_tag = -1;

/* The dates line is split into an array of strings. */
char *dates_line = NULL;
unsigned int n_dates = 0;
char **dates = NULL;
int chosen_date = -1;

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
  { tag_chooser, "Tags" },
  { date_chooser, "Date" },
  { live_data, "Live data" },
  { settings, "Settings" },
  { top_level_chooser, NULL }};

// #define debug_general 1

#ifdef debug_general
char *
mode_name(zorg_mode mode_code)
{
  switch (mode_code) {
  case top_level_chooser: return "top_level_chooser";
  case file_chooser: return "file_chooser";
  case tree: return "tree";
  case date: return "date";
  case tag: return "tag";
  case leaf: return "leaf";
  case tag_chooser: return "tag_chooser";
  case date_chooser: return "date_chooser";
  case live_data: return "live_data";
  case settings: return "settings";
  default: return "bad mode";
  }
}

char *
data_source_name(data_source_type source_type)
{
  switch(source_type) {
  case local_file: return "local_file";
  case remote_stream: return "remote_stream";
  case none: return "none";
  default: return "bad data source mode";
  }
}

void
show_status(char *label)
{
  printf("%s in mode %s\n", label, mode_name(mode));
  printf("  data source=%s; file_data=%p; n_lines=%d(%d displayed), filename=%s\n", data_source_name(data_source), file_data, n_lines, display_n_lines, currently_selected_file);
}
#endif

#define debug_scan_tags 1

void
zorg_pebble_scan_tags()
{
  int scan;
#ifdef debug
  printf("in zorg_pebble_scan_tags(%s)\n", filter_search_string);
#endif
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
#ifdef debug_scan_tags
	printf("comparing %s with %s (%c(%02x) with %c(%02x))\n", q-1, p-1, d, d, c, c);
#endif
	while (c == d) {
	  c = *p++;
	  d = *q++;
#ifdef debug_scan_tags
	  printf("comparing %c(%02x) with %c(%02x)\n", d, d, c, c);
#endif
	}
	if ((d == '\0') && ((c == ' ') || (c == ':'))) {
	  hit = 1;
#ifdef debug_scan_tags
	  // printf("matched!\n");
#endif
	  break;
	} else {
	  /* back off one, to re-scan the : */
	  p--;
	}
      }
    }
    if (hit) {
      display_lines[display_n_lines++] = scan;
#ifdef debug_scan_tags
      printf("Added line %d to display (now %d lines)\n", scan, display_n_lines);
#endif
    }
  }
}

void
zorg_pebble_scan_dates()
{
  int scan;
#ifdef debug
  printf("in zorg_pebble_scan_dates(%s)\n", filter_search_string);
#endif
  display_n_lines = 0;

  for (scan = 0; scan < n_lines; scan++) {
    char *p;
    int hit = 0;

    for (p = lines[scan]; *p != 0; p++) {

      char c = *p;

      if (c == '<') {
	if (strncmp(p+1,
		    filter_search_string,
		    filter_search_string_length) == 0) {
	  hit = 1;
	  break;
	}
      }
    }

    if (hit) {
      display_lines[display_n_lines++] = scan;
    }
  }
}

void
zorg_pebble_rescan_tree_level()
{
  int scan;

#ifdef debug
  printf("rescanning for start and end, parent=%d\n", parent);
  if (parent >= end) {
    printf("seem to have gone off end at zorg_pebble_rescan_tree_level\n");
  }
#endif
  display_n_lines = 0;
  start = end = -1;

  for (scan = parent; scan < n_lines; scan++) {
    char margin_char;
    if (lines[scan] == NULL) {
      break;
    }
    margin_char = lines[scan][0];
#ifdef debug_scan
    printf("  scan %d: %s (level %c, want %c)\n", scan, lines[scan], margin_char, level);
#endif
    if (margin_char < '0' || margin_char > '9') {
#ifdef debug_scan
      printf("  Skipping non-heading %d: %s\n", scan, lines[scan]);
#endif
      continue;		/* not a heading line */
    }
    if (margin_char == level) {
#ifdef debug_scan
      printf("  got one at our level: line %d is display line %d\n", scan, display_n_lines);
#endif
      display_lines[display_n_lines++] = scan;
      if (start == -1) {
#ifdef debug_scan
	printf("Got start %d: %s\n", scan, lines[scan]);
#endif
	start = scan;
      }
    }
    if ((margin_char < level) && (scan != parent)) {
#ifdef debug_scan
      printf("Gone out a level (on %c, outside %c), stopping scan at %d: %s\n", margin_char, level, scan, lines[scan]);
#endif
      break;		/* gone out a level */
    }
    end = scan;		/* trails one behind */
  }
  display_lines[display_n_lines] = -1;
  cursor = 0;
  if (start == -1) {
#ifdef printf
    printf("failed to set start in zorg_pebble_rescan_tree_level\n");
#endif
    /* todo: handle leaf node */
    /* todo: is this path ever triggered now? gets to leaf by another route now */
    start = old_start;
    end = old_end;
  }
}

char *
skip_line_preamble(char *line)
{
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

void
construct_leaf_display()
{
  char *p;
  int scan = start + 1;
  current_keyword = -1;
  for (p = lines[start]; *p != '\0'; p++) {
    if (*p == '!') {
      original_keyword = current_keyword = atoi(p+1);
    }
  }
#ifdef printf
  printf("construct_leaf_display start=%d end=%d level=%c parent=%d parent_level=%c\n", start, end, level, parent, parent_level);
#endif
  display_n_lines = 0;
  if (current_keyword >= 0) {
    display_lines[display_n_lines++] = KEYWORD_PROXY;
  }
  display_lines[display_n_lines++] = start;
  while ((scan < n_lines) && lines[scan][0] == ' ') {
    display_lines[display_n_lines++] = scan;
    scan++;
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
  case leaf:
    if (index == ((current_keyword >= 0) ? 1 : 0)) {
      // printf("returning leaf header line index=%d display_lines[%d]=%d\n", index, index, display_lines[index]);
      return skip_line_preamble(lines[display_lines[index]]);
    } else {
      int x = display_lines[index];
      if ((x == KEYWORD_PROXY) && (current_keyword != -1)) {
	return keywords[current_keyword];
      }
      return lines[x]+1;
    }
  case tree:
  case tag:
  case date:
    return skip_line_preamble(lines[display_lines[index]]);
  case tag_chooser:
    return (tags == NULL) ? NULL : tags[index];
  case date_chooser:
    return (dates == NULL) ? NULL : dates[index];
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
  case error:
    if (index > 0 && error_message != NULL) {
      return error_message;
    }
    return "There has been an error";
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
  case leaf:
  case tag:
  case date:
    return display_n_lines;
  case file_chooser:
    return n_directory_lines;
  case tag_chooser:
    return n_tags;
  case date_chooser:
    return n_dates;
  case live_data:
    return 4;
  case settings:
    return 0;
  case error:
    return error_message != NULL ? 2 : 1;
  }
  printf("Bad mode!");
  exit(1);
}

void
parse_line(char *line)
{
  char *p;
  unsigned int j;

  if (line != NULL) {
    switch (line[0]) {
    case '!':
      keywords_line = line;
      for (p = keywords_line; *p != '\0'; p++) {
	if (*p == ' ') {
	  n_keywords++;
	}
      }
      keywords = (char**)malloc(sizeof(char*) * (n_keywords+1));
      if (keywords == NULL) {
#ifdef debug_parse
	printf("Could not allocate keywords array\n");
#endif
	exit(1);
      }
      j = 0;
      for (p = keywords_line; *p != '\0'; p++) {
	if (*p == ' ') {
	  *p = '\0';
	  // printf("Recorded keyword %d as being start of %s\n", j, p+1);
	  keywords[j++] = p+1;
	}
      }
      break;
    case ':':
      tags_line = line;
      for (p = tags_line; *p != '\0'; p++) {
	if (*p == ':') {
	  n_tags++;
	}
      }
      tags = (char**)malloc(sizeof(char*) * (n_tags+1));
      if (tags == NULL) {
#ifdef debug_parse
	printf("Could not allocate tags array\n");
#endif
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
    case '@':
      dates_line = line;
      for (p = dates_line; *p != '\0'; p++) {
	if (*p == ',') {
	  n_dates++;
	}
      }
      dates = (char**)malloc(sizeof(char*) * (n_dates+1));
      if (dates == NULL) {
#ifdef debug_parse
	printf("Could not allocate dates array\n");
#endif
	exit(1);
      }
      j = 0;
      for (p = dates_line; *p != '\0'; p++) {
	if (*p == ',') {
	  *p = '\0';
	  dates[j++] = p+1;
	}
      }
      break;
    case '#':
      {
	char shebang[256];
	if (sscanf(line, "%255s %d %d", shebang, &allocated_n_lines, &file_size) == 3) {
#ifdef debug_parse
	  printf("Reallocating data storage: %d %d\n", allocated_n_lines, file_size);
#endif
	  if (file_data != NULL) {
	    free(file_data);
	  }
	  file_data = (char*)malloc(file_size);
	  data_filling_point = file_data;
	  if (lines != NULL) {
	    free(lines);
	  }
	  lines = (char**)malloc((allocated_n_lines) * sizeof(char*));
	  if (display_lines != NULL) {
	    free(display_lines);
	  }
	  display_lines = (int*)malloc((allocated_n_lines) * sizeof(int));
	  display_n_lines = 0;
	  n_lines = 0;
	  keywords_line = NULL;
	  n_keywords = 0;
	  tags_line = NULL;
	  n_tags = 0;
	  start = end = old_start = old_end = -1;
	  parent = parent_level = level = 0;
	}
      }
    default:
      break;
    }
  }
}

int*
parse_data(char *data_buffer, unsigned int data_size, unsigned int *line_count_p)
{
  unsigned int i, j;
  unsigned int line_count = 0;
  int *displayed_lines_indices;

  /* split buffer into lines */
  for (i = 0; i < data_size; i++) {
    if (data_buffer[i] == '\n') {
      data_buffer[i] = '\0';
      line_count++;
    }
  }

  lines = (char**)malloc((line_count+1) * sizeof(char*));
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

#ifdef debug_parse
  for (i = 0; i < line_count; i++) {
    printf("Line %d: %s\n", i, lines[i]);
  }
#endif
  for (i = 0; i < line_count; i++) {
    parse_line(lines[i]);
  }
#ifdef debug_parse
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
  allocated_n_lines = line_count;
  return displayed_lines_indices;
}

void
load_data()
{
#ifdef debug_general
  show_status("load_data");
#endif
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
#ifdef debug_general
  show_status("unload_data");
#endif
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

void
update_display_lines()
{
#ifdef debug_general
  show_status("update_display_lines");
#endif
  /* pick up any mode change, or any changes within a mode */
  load_data();
  switch (mode) {
  case top_level_chooser:
    break;
  case file_chooser:
    break;
  case tree:
    /* fixme: I think this may be a bit confused, probably should have loaded the data by now */
#if 1
    if (file_changed) {
      file_changed = 0;
      unload_data();
    }
    load_data();
#endif
    if ((start == -1) || (end == -1)) {
      zorg_pebble_rescan_tree_level();
    }
    break;
  case leaf:
    break;
  case date:
    break;
  case tag_chooser:
    break;
  case date_chooser:
    break;
  case tag:
    break;
  case live_data:
    break;
  case settings:
    break;
  }
}

void
scrollout_display_lines()
{
  int i;
#ifdef debug_general
  show_status("scrollout_display_lines");
#endif
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
