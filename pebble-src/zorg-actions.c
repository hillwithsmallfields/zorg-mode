#include <stdlib.h>
#include <stdio.h>
#include <dirent.h>
#include <string.h>
#include "zorg.h"

void
set_mode(zorg_mode new_mode)
{
  /* undo anything from the old mode */
  switch (mode) {
  case live_data:
    /* todo: unregister event handlers */
    break;
  default:
    break;
  }

  mode = new_mode;
  cursor = 0;			/* jump back to the top */
  switch (new_mode) {
  case top_level_chooser:
    /* doesn't use display_lines etc, as zorg_pebble_display_line
       returns a string from top_level_items directly */
    break;
  case tree:
    parent = 0;
    parent_level = '0';
    level = parent_level + 1;
    start = end = -1;
    break;
  case file_chooser:
    {
      if (directory_data == NULL) {
	struct dirent *dir_buf;
	DIR *dir = opendir(zorg_dir_name);
	unsigned int i;
	char *p;
	n_directory_lines = 0;
	directory_data_size = 0;
	while ((dir_buf = readdir(dir))) {
	  char *name = dir_buf->d_name;
	  int name_len = strlen(name);
	  if (strncmp(name+name_len-5, ".zorg", 5) == 0) {
	    printf("name = %s\n", name);
	    n_directory_lines++;
	    directory_data_size += name_len + 1;
	  }
	}
	rewinddir(dir);
      
	printf("%d matches, %d bytes\n", n_directory_lines, directory_data_size);
	directory_lines = (char**)malloc(n_directory_lines*sizeof(char*));
	directory_data = (char*)malloc(directory_data_size);

	i = 0; p = directory_data;
	while ((dir_buf = readdir(dir))) {
	  char *name = dir_buf->d_name;
	  int name_len = strlen(name);
	  if (strncmp(name+name_len-5, ".zorg", 5) == 0) {
	    strcpy(p, name);
	    directory_lines[i] = p;
	    printf("[%d] p = %p = %s\n", i, directory_lines[i], directory_lines[i]);
	    p += name_len + 1;
	    i++;
	    printf("name = %s\n", name);
	  }
	}
	closedir(dir);
      }
    }
    {
      unsigned int i;
      for (i = 0; i < n_directory_lines; i++) {
	printf("Line %d: %p %s\n", i, directory_lines[i], directory_lines[i]);
      }
    }
    break;
  case date:
    {
      zorg_pebble_scan_dates();
      printf("date mode not implemented\n");
      set_mode(top_level_chooser);
    }
    break;
  case tag_chooser:
    printf("Starting choosing a tag\n");
    chosen_tag = -1;
    break;
  case tag:
    /* todo: ensure a file is loaded, and filter to produce display_lines by tag */
    printf("selecting tag %d = %s\n", chosen_tag, tags[chosen_tag]);
    snprintf(filter_search_string, FILTER_SEARCH_STRING_MAX, "%d", chosen_tag);
    load_data();
    zorg_pebble_scan_tags();
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

void
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
      printf("Selecting file %s\n", directory_lines[cursor]);
      char *new_file = directory_lines[cursor];
      currently_selected_file = (char*)malloc(strlen(new_file) + strlen(zorg_dir_name) + 1);
      sprintf(currently_selected_file, "%s/%s", zorg_dir_name, new_file);
      unload_data();
      data_source = local_file;
      load_local_file(currently_selected_file);
      set_mode(tree);
      free(currently_selected_file);
      currently_selected_file = NULL;
      file_changed = 0;
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
    /* todo: display individual entry */
    break;
  case live_data:
    /* maybe update, but it should probably do that on ticks anyway */
    break;
  case settings:
    break;
  }
}

void
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
  case live_data:
  case settings:
    set_mode(top_level_chooser);
    break;
  case tag:
    set_mode(tag_chooser);
    break;
  }
}
