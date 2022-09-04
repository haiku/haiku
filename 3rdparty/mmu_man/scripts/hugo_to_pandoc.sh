#!/bin/sh

# This script tries to convert a hugo markdown source into a markdown that pandoc can read, to convert to ODT for example.

# First filter turns inline links into footnotes with italic link text and monospace URL.
# Second filter turns img tags into regular markdown images
# Third filter converts enough of the Hugo header into pandoc's own header format.

sed -r 's/\[([^\[]*)\]\(([^()]*)\)/*\1*^[`\2`]/g' | \
sed -r 's,<img src=\"([^"]*)\".*title=\"([^"]*)\".*>,![\2](static\1),g' | \
sed -r '/^\+\+\+$/{c\
---
};/^tags = \[.*$/d;1,5s/(.*) = "(.*)"/\1: \2/'
