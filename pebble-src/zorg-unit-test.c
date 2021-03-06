#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "zorg.h"

static char *test_data[] = {
  /* 0 */ "1 :1 First top-level item",
  /* 1 */ "2 :12 First second-level item within first top-level item <2014-12-24 Wed>",
  /* 2 */ "2 :3 Second second-level item within first top-level item",
  /* 3 */ "2 :8 Third second-level item within first top-level item",
  /* 4 */ "1 Second top-level item",
  /* 5 */ "2 First second-level item within second top-level item",
  /* 6 */ "2 Second second-level item within second top-level item <2014-12-25 Thu>",
  /* 7 */ "2 :8:12:9 Third second-level item within second top-level item",
  /* 8 */ "1 Third top-level item",
  /* 9 */ "2 First second-level item within third top-level item",
  /* 10 */ "3 First third-level item within first second-level item within third top-level item",
  /* 11 */ "3 4:12 Second third-level item within first second-level item within third top-level item",
  /* 12 */ "2 :12:7 Second second-level item within third top-level item <2014-12-25 Thu>",
  /* 13 */ "2 Third second-level item within third top-level item",
  /* 14 */ ":zero:one:two:three:four:five:six:seven:eight:nine:ten:eleven:twelve:thirteen:fourteen:fifteen:",
  NULL};

#define SPECIAL_MAX 256

void
log_changes(int *path, int path_len)
{
  /* todo: check the changes against a reference value */
  printf("Log change %d", current_keyword);
  if (data_source == local_file) {
    printf(":%s", currently_selected_file);
  }
  for (path_len--; path_len >= 0; path_len--) {
    printf(":%s", lines[path[path_len]]);
  }
  printf("\n");
}

int
main(int argc, char **argv, char **env)
{
  char **p;
  unsigned int lines = 0, chars = 0;
  char special[SPECIAL_MAX];

  for (p = test_data;
       *p != NULL;
       p++) {
    lines++;
    chars += strlen(*p) + 1;
  }

  snprintf(special, SPECIAL_MAX, "#!/usr/local/bin/zorg %d %d", lines, chars);
  add_line(special);

  for (p = test_data;
       *p != NULL;
       p++) {
    add_line(*p);
  }

  printf("Tree mode output (should be lines 0, 4, 8):\n");
  set_mode(tree);
  update_display_lines();
  scrollout_display_lines();

  printf("Tags output: (should be lines 1, 7, 11, 12)\n");
  strcpy(filter_search_string, "12");
  filter_search_string_length = strlen(filter_search_string);
  zorg_pebble_scan_tags();
  scrollout_display_lines();

  printf("Date output: (should be lines 6, 12)\n");
  strcpy(filter_search_string, "2014-12-25");
  filter_search_string_length = strlen(filter_search_string);
  zorg_pebble_scan_dates();
  scrollout_display_lines();
  
  exit(0);
}
