#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include "zorg.h"

char*
read_local_file(char *file_name, unsigned int *file_size_result)
{
  struct stat stat_buf;
  int fd;
  int data_size;
  char *data_buffer;

  if (stat(file_name, &stat_buf) != 0) {
    fprintf(stderr, "Could not stat %s\n", file_name);
    exit(1);
  }

  data_size = stat_buf.st_size;

  data_buffer = (char*)malloc(data_size+1);

  if (data_buffer == NULL) {
    fprintf(stderr, "Could not malloc buffer of %d bytes\n", data_size+1);
    exit(1);
  }

  fd = open(file_name, O_RDONLY);

  if (fd == -1) {
    fprintf(stderr, "Could not open file %s\n", file_name);
    exit(1);
  }

  if (read(fd, data_buffer, data_size) != data_size) {
    fprintf(stderr, "Could not read whole file %s (%d bytes)\n", file_name, data_size);
    exit(1);
  }

  close(fd);

  fprintf(stderr, "Read file of %d bytes\n", data_size);

  data_buffer[data_size] = '\0';

  *file_size_result = data_size;
  return data_buffer;
}

void
load_local_file(char *filename)
{
  printf("Loading data from local file %s\n", filename);
  file_data = read_local_file(filename, &file_size);

  display_lines = parse_data(file_data, file_size, &n_lines);

  data_source = local_file;
}

void
unload_local_file()
{
  if (file_data != NULL) {
    free(file_data);
    file_data = NULL;
    file_size = 0;
    allocated_file_size = 0;
  }
  file_size = 0;
  if (lines != NULL) {
    free(lines);
    lines = NULL;
  }
  n_lines = 0;
  allocated_n_lines = 0;
  if (display_lines != NULL) {
    free(display_lines);
    display_lines = NULL;
  }
  display_n_lines = 0;

  tags_line = NULL;
  keywords_line = NULL;

  if (keywords != NULL) {
    free(keywords);
    n_keywords = 0;
    keywords = NULL;
  }

  if (tags != NULL) {
    free(tags);
    n_tags = 0;
    tags = NULL;
  }

  data_source = none;
}
