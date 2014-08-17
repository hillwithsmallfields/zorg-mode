How the pebble app works
========================

Structure and functions
-----------------------

The program uses menus to display a hierarchical tree of data.

The main data comes from a zorg file (from the phone) but the higher
levels of the tree are generated separately.

What is displayed is controlled by the mode, which is selected by the
function `set_mode'.

Each line to display is returned by the function
`zorg_pebble_display_line', which takes a line number as its argument.
The maximum line number to display is returned by the function
`zorg_pebble_display_n_lines'.

Data structures
---------------

For displaying data from the file, the data is held as an array of
lines, in the C variable `lines', occupied up to `n_lines'.  The
selective tree display (by level, tag, or date, and perhaps later by
priority too) is done by putting the line numbers to display (i.e. the
indices into `lines') in the C variable `display_lines', up to max of
`display_n_lines'.

Each line is null-terminated; at the start of each line there is some
metadata.  The first thing on each line is either a space, or a digit
representing the tree depth of the line, or another character
indicating that it is a special line.

The special lines, declaring an array of keywords and a array of tags,
are split up into their individual entries using null characters.
The line starts are stored in `keywords_line' and `tags_line', and the
arrays of pointers into them are stored in `keywords' up to
`n_keywords', and `tags' up to `n_tags'.

Lines beginning with a space are continuation (body) lines, and will
only be displayed on leaf nodes.

Heading lines (with depths) have a space character after the depth.
They may have a `!' character after that, after which there is a
number which is an index into the keywords array.

After the number, there may be ':' character, beginning a
colon-separated list of indices into the tags array.  This list is
space-terminated.

After that comes the actual heading data to display.

Non-file-data display
---------------------

The top-level chooser and the file selector are coded separately, and
don't use the file data at all.

The tag chooser displays data from the tag line of the file.
