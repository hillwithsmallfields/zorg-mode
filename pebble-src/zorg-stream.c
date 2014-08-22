#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "zorg.h"

void
add_line(char *line)
{
  unsigned int llen = strlen(line);
  if (line[0] == '#') {
    /* We don't store this line, as we don't yet have anywhere to
       store it; it contains the information needed to set the lines
       storage up. */
    parse_line(line);
  } else {
    // printf("adding line: %s to data filling point at %p\n", line, data_filling_point);
    strcpy(data_filling_point, line);
    lines[n_lines++] = data_filling_point;
    parse_line(data_filling_point);
    data_filling_point += (llen + 1);
  }
}

void
load_remote_stream(char *stream_name)
{
  /* todo:
     request stream_name from the phone
     get a number of lines
     read all the lines, and parse them
  */

}

void
unload_remote_stream()
{
}
