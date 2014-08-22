#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "zorg.h"

static char *test_data[] = {
  "1 First top-level item",
  "2 First second-level item within first top-level item",
  "2 Second second-level item within first top-level item",
  "2 Third second-level item within first top-level item",
  "1 Second top-level item",
  "2 First second-level item within second top-level item",
  "2 Second second-level item within second top-level item",
  "2 Third second-level item within second top-level item",
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

  exit(0);
}
