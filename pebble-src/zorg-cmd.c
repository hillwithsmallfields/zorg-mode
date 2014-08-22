/* Initially, this is a whole C command-line program, while I test it.
   Later on, it will become a pebble program.
   I may at some stage spin off an ncurses version of it.
 */

/* todo: do something different on reaching a leaf node */
/* todo: don't unload file data until loading another file */
/* todo: tag-based selection */
/* todo: date/time based selection */
/* todo: changing states, and sending them back using pebble's DataLogging */
/* todo: live data */
/* todo: loading data from stream */
/* todo: settings */
/* todo: commands from phone to bring up particular agendas according to context e.g. location, time */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "zorg.h"

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
      first = 0;
      level = parent_level + 1;
      if ((command != ' ') && (command != '\n')) {
	printf("\n\ncommand='%c'; parent=%d; start=%d; cursor=%d; end=%d; parent_level=%c; level=%c\n",
	       command, parent, start, cursor, end, parent_level, level);
      }

      update_display_lines();

      scrollout_display_lines();

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
