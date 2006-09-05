#!/bin/bash
mydir=$(readlink -f $(dirname $0))
$mydir/install.sh --test
