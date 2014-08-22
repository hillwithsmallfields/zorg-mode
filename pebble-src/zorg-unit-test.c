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

int
main(int argc, char **argv, char **env)
{
  char **p;

  for (p = test_data[0];
       *p != NULL;
       p++) {
    printf("Got test line: %s\n", *p);
  }

  exit(0);
}
