/* zorg.h */

typedef enum zorg_mode {
  top_level_chooser,
  file_chooser,
  tree,
  date,
  tag,
  leaf,
  tag_chooser,			/* used in the lead-in to tag mode */
  date_chooser,			/* used in the lead-in to date mode */
  live_data,
  settings
} zorg_mode;

typedef enum data_source_type {
  local_file,
  remote_stream,
  none
} data_source_type;

typedef struct zorg_top_level_item {
  zorg_mode mode;
  char *label;
} zorg_top_level_item;

#define FILTER_SEARCH_STRING_MAX 16

extern char filter_search_string[FILTER_SEARCH_STRING_MAX];
extern unsigned int filter_search_string_length;

extern struct zorg_top_level_item top_level_items[];

extern zorg_mode mode;

extern data_source_type data_source;

extern char *currently_selected_file;

/* The whole data read from file or connection.
   This is a file as prepared by the companion emacs-lisp code.
*/
extern char *file_data;
extern char *data_filling_point;
extern unsigned int file_size;
extern unsigned int allocated_file_size;
extern int file_changed;

/* The data parsed into an array of lines.
   The data gets split in place, by poking null characters into it.
   Heading lines (in the org-mode sense) begin with a digit, which indicates the tree level.
   The level digit is followed by some optional keyword and tag data.
   Then comes the actual heading text.
   Lines beginning with a space are body lines (shown in the leaf-mode display only).
   A line beginning with a '!' character holds the array of keywords.
   A line beginning with a ':' character holds the array of tags.
 */
extern unsigned int n_lines;
extern unsigned int allocated_n_lines;
extern char **lines;

/* Variables for the tree-mode display. */
extern unsigned int parent;
extern unsigned int parent_level;
extern unsigned int level;
extern int start;
extern int end;
extern int old_start;
extern int old_end;

/* The keywords line is split into an array of strings. */
extern char *keywords_line;
extern unsigned int n_keywords;
extern char **keywords;
extern int current_keyword;
extern int original_keyword;

/* The tags lines is split into an array of strings. */
extern char *tags_line;
extern unsigned int n_tags;
extern char **tags;
extern int chosen_tag;

/* The dates lines is split into an array of strings. */
extern char *dates_line;
extern unsigned int n_dates;
extern char **dates;
extern int chosen_date;

/* The lines which are actually displayed, as indices into the main "lines" array. */
extern unsigned int display_n_lines;
extern int *display_lines;
extern int cursor;

/* Another set of line, for the directory display.  This is stored
   separately, so that if we go into the directory display and then
   back out of it without selecting a file, the original file can be
   left undisturbed. */

extern char *directory_data;
extern unsigned int directory_data_size;
extern unsigned int n_directory_lines;
extern char **directory_lines;

extern char *zorg_dir_name;

/* zorg-actions.c */

extern void set_mode(zorg_mode new_mode);
extern void load_local_file(char *filename);
extern void zorg_middle_button();
extern void zorg_back_button();

/* zorg-data.c */
extern void zorg_pebble_scan_tags();
extern void zorg_pebble_scan_dates();
extern char *zorg_pebble_display_line(unsigned int index);
extern int zorg_pebble_display_n_lines();
extern void zorg_pebble_rescan_tree_level();
extern void construct_leaf_display();
extern void parse_line(char *line);
extern int *parse_data(char *data_buffer, unsigned int data_size, unsigned int *line_count_p);
extern void load_data();
extern void unload_data();
extern void update_display_lines();
extern void scrollout_display_lines();

/* zorg-files.c */
extern char *read_local_file(char *file_name, unsigned int *file_size_result);
extern void load_local_file(char *filename);
extern void unload_local_file();

/* zorg-stream.c */
extern void add_line(char *line);
extern void load_remote_stream(char *stream_name);
extern void unload_remote_stream();


/* zorg.h ends here */
