#include <stdlib.h>
#include <stdio.h>

#include "zorg.h"

void
add_line(char *line)
{
  printf("adding line: %s\n", line);
  /* todo: make it add the result to the buffer and to the lines array */
  parse_line(line);
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
