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
  case leaf:
    construct_leaf_display();
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
	    // printf("name = %s\n", name);
	    n_directory_lines++;
	    directory_data_size += name_len + 1;
	  }
	}
	rewinddir(dir);
      
	// printf("%d matches, %d bytes\n", n_directory_lines, directory_data_size);
	directory_lines = (char**)malloc(n_directory_lines*sizeof(char*));
	directory_data = (char*)malloc(directory_data_size);

	i = 0; p = directory_data;
	while ((dir_buf = readdir(dir))) {
	  char *name = dir_buf->d_name;
	  int name_len = strlen(name);
	  if (strncmp(name+name_len-5, ".zorg", 5) == 0) {
	    strcpy(p, name);
	    directory_lines[i] = p;
	    // printf("[%d] p = %p = %s\n", i, directory_lines[i], directory_lines[i]);
	    p += name_len + 1;
	    i++;
	    // printf("name = %s\n", name);
	  }
	}
	closedir(dir);
      }
    }
    break;
  case date:
    strcpy(filter_search_string, dates[chosen_date]);
    filter_search_string_length = strlen(filter_search_string);
    zorg_pebble_scan_dates();
    break;
  case tag_chooser:
    chosen_tag = -1;
    break;
  case date_chooser:
    chosen_date = -1;
    break;
  case tag:
    snprintf(filter_search_string, FILTER_SEARCH_STRING_MAX, "%d", chosen_tag);
    filter_search_string_length = strlen(filter_search_string);
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
    for (scan = parent + 1; scan < n_lines; scan++) {
      char margin_char = lines[scan][0];
      // printf("  considering %s\n", lines[scan]);
      if (margin_char < '0' || margin_char > '9') {
	continue;		/* not a heading line */
      }
      if (margin_char > parent_level) {
	level = margin_char;
	old_start = start;
	old_end = end;
	start = end = -1;
	break;
      } else {
	set_mode(leaf);
	break;
      }
    }
    break;
  case leaf:
    current_keyword++;
    if (current_keyword >= n_keywords) {
      current_keyword = 0;
    }
    if (strcmp(keywords[current_keyword], "|") == 0) {
      current_keyword++;
      if (current_keyword >= n_keywords) {
	current_keyword = 0;
      }
    } else {
      /* if at a group delimiter, rewind */
      /* todo: put this in the elisp too, when I find the input file format for it */
      if (strcmp(keywords[current_keyword], "#") == 0) {
	current_keyword--;
	while ((current_keyword > 0) && (strcmp(keywords[current_keyword], "#") != 0)) {
	  current_keyword--;
	}
	if (current_keyword > 0) {	/* we found the delimiter, move to the one after it */
	  current_keyword++;
	}
      }
    }
    break;
  case file_chooser:
    {
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
    set_mode(tag);
    break;
  case date_chooser:
    chosen_date = cursor;
    break;
  case file_chooser:
    set_mode(top_level_chooser);
    break;
  case leaf:
    /* can't do set_mode(tree) here as it goes to the root node */
    mode = tree;
    if (current_keyword != original_keyword) {
      int scan = start;
      int scan_level = level;
      int path[10];		/* number of levels is one digit */
      int ipath = 0;
      while (scan_level >= 1) {
	path[ipath++] = scan;
	while (scan >= 0) {
	  scan--;
	  if (lines[scan][0] < scan_level) {
	    scan_level = lines[scan][0];
	    break;
	  }
	}
      }
      printf("Log change %d", current_keyword);
      for (ipath--; ipath >= 0; ipath--) {
	printf(":%s", lines[path[ipath]]);
      }
      printf("\n");
      /* todo: send log change to Pebble logging (and equivalent on other devices) */
    }
    /* fallthrough */
  case tree:
    if (parent_level == '0') {
      set_mode(top_level_chooser);
    } else {
      for (scan = parent; scan >= 0; scan--) {
	char margin_char = lines[scan][0];
	if (margin_char < '0' || margin_char > '9') {
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
  case date_chooser:
  case live_data:
  case settings:
    set_mode(top_level_chooser);
    break;
  case tag:
    set_mode(tag_chooser);
    break;
  }
}
