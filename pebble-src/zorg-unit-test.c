#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "zorg.h"

static char *test_data[] = {
  "1 :1 First top-level item",
  "2 :12 First second-level item within first top-level item",
  "2 :3 Second second-level item within first top-level item",
  "2 :8 Third second-level item within first top-level item",
  "1 Second top-level item",
  "2 First second-level item within second top-level item",
  "2 Second second-level item within second top-level item",
  "2 :8:12:9 Third second-level item within second top-level item", /* isn't working when :12: isn't the first tag */
  "1 Third top-level item",
  "2 First second-level item within third top-level item",
  "3 First third-level item within first second-level item within third top-level item",
  "3 :12 Second third-level item within first second-level item within third top-level item",
  "2 Second second-level item within third top-level item",
  "2 Third second-level item within third top-level item",
  ":zero:one:two:three:four:five:six:seven:eight:nine:ten:eleven:twelve:thirteen:fourteen:fifteen:",
  NULL};

#define SPECIAL_MAX 256

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

  printf("Tree mode output:\n");
  set_mode(tree);
  update_display_lines();
  scrollout_display_lines();

  printf("Tags output:\n");
  strcpy(filter_search_string, "12");
  zorg_pebble_scan_tags();
  scrollout_display_lines();
  
  exit(0);
}
