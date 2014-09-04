/* This curses-based program is a spinoff / developmental stage of getting zorg running on the Pebble.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <curses.h>

#include "zorg.h"

static int height, width;
#define MAXWIDTH 256

void
curses_display_lines()
{
  int lines_available = zorg_pebble_display_n_lines();
  int lines_to_display = lines_available;
  int topline = 0;
  int i;
  erase();
  if (lines_to_display > height) {
    lines_to_display = height;
    if (cursor > lines_to_display) {
      topline = cursor - lines_to_display/2;
    }
  }
  
  for (i = 0; i < lines_to_display; i++) {
    wmove(stdscr, i, 0);
    if (i == cursor) {
      attron(A_STANDOUT);
    }
    wprintw(stdscr, "%c %.*s", (i == cursor) ? '*' : ' ', MAXWIDTH - 3, zorg_pebble_display_line(i + topline));
    if (i == cursor) {
      attroff(A_STANDOUT);
    }
  }

  wrefresh(stdscr);
}

void
log_changes(int *path, int path_len)
{
  FILE *changes = fopen("/tmp/zorg-changes", "a");
  fprintf(changes, "Log change %s", keywords[current_keyword]);
  if (data_source == local_file) {
    fprintf(changes, ":%s", currently_selected_file);
  }
  for (path_len--; path_len >= 0; path_len--) {
    fprintf(changes, ":%s", lines[path[path_len]]);
  }
  fprintf(changes,"\n");
  fclose(changes);
}

int
main(int argc, char **argv, char **env)
{
  if (argc != 2) {
    fprintf(stderr, "Usage: zorg-curses file\n");
    exit(1);
  }

  initscr();
  cbreak();
  noecho();
  getmaxyx(stdscr, height, width);

  mode = top_level_chooser;
  currently_selected_file = argv[1];
  data_source = local_file;

  {
    int running;
    // int first = 1;

    parent = 0; /* we display the children of this */
    parent_level = '0';
    start = -1;		/* -1 for unset */
    end = -1;		/* -1 for unset */
    old_start = start;
    old_end = end;

    for (running = 1; running;) {
      char command;
      // first = 0;
      level = parent_level + 1;

      update_display_lines();

      curses_display_lines();

      command = // first ? ' ' :
	getch();

      switch (command) {
      case 'q':			/* quit; on pebble, this will be "back" button when at top level */
	running = 0;
	break;

      case 'f':			/* forward; on pebble, "down" button */
      case 'n':			/* next */
	{
	  int max_lines =  zorg_pebble_display_n_lines();
	  cursor++;
	  if (cursor >= max_lines ) {
	    cursor = max_lines - 1;
	  }
	}
	break;

      case 'b':			/* backward; on pebble, "up" button */
      case 'p':			/* previous */
	cursor--;
	if (cursor < 0) {
	  cursor = 0;
	}
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

  endwin();

  exit(0);
}
