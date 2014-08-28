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
Their line starts are stored in `keywords_line' and `tags_line', and the
arrays of pointers into them are stored in `keywords' up to
`n_keywords', and `tags' up to `n_tags'.

Lines beginning with a space are continuation (body) lines, and will
only be displayed on leaf nodes.

Heading lines (with depths) have a space character after the depth.
* TODO: drop the space *
They may have a `!' character after that, after which there is a
number which is an index into the keywords array.

After the number, there may be ':' character, beginning a
colon-separated list of indices into the tags array.  This list is
space-terminated.

After that comes the actual heading data to display.

The line containing the keywords begins with the '!' character.  The
last such line is taken as definitive, at least for now, but really
there should only be one of them.

The line containing the tags begins with the ':' character.  The last
such line is taken as definitive, at least for now, but really there
should only be one of them.

A line beginning with a '#' character defines the size of the file.
This should be the first line.  From the '#' character up to the first
space is ignored, to make it usable as a shebang.  Following that is
the number of lines and the number of characters.  These may be larger
than their real values, but should not be smaller, as they are used
for allocation.  They're not currently used when reading a file, as
the program can find the size of the file by calling `stat' anyway;
they're for reading from a stream of packets sent from the phone to
the Pebble.

A line beginning with the @ character contains a comma-separated list
of dates.

Non-file-data display
---------------------

The top-level chooser and the file selector are coded separately, and
don't use the file data at all.

The tag chooser displays data from the tag line of the file.

Plans
=====

Push data from phone
--------------------

The phone will be able to send a file, line by line.

Return updates to phone
-----------------------

Changing state (keywords, e.g. TODO --> DONE) will use the Pebble
Logging facility to queue them up, sending org "paths" to tell the
companion app what to update.

Push display choice from phone
------------------------------

The phone will be able to tell the watch app to switch to the display
for a particular tag or time (tags being chosen geographically
according to GPS, for example).

Display on the phone
--------------------

Since there'll be a companion app on the phone, that has to be able to
read the file and send it as lines, I might extend that into being a
proper zorg-mode browser map in its own right.

I might make it read and write actual org files, too, as it won't have
the tight space constraints of the watch app.
