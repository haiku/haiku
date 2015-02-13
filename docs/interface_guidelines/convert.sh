#!/bin/bash
if [ -z "$1" ]; then
	echo "Usage: $0 <format>"
	exit 1
fi
xmlto $1 index.xml --skip-validation
