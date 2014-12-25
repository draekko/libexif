#!/bin/bash

## 
# update svn_version_h when revision changes
##

svn_version_h=svn_version.h
svn_version=$(svn info .. 2>/dev/null | grep ^Revision | cut -f 2 -d' ')
if [ -z "$svn_version"   ]; then svn_version=0 ; fi

##
# report svn_version to output
set | grep svn_version

##
# sniff svn_version in svn_version.h and delete the file if incorrect
if [ -e "$svn_version_h" ]; then
	old=$(cut -f 3 -d' ' "$svn_version_h")
	if [ "$old" != "$svn_version" ]; then
		rm -rf "$svn_version_h"
	fi
fi

##
# write svn_version to file if necessary
if [ ! -e "$svn_version_h" ]; then
	echo "#ifndef SVN_VERSION"                > "$svn_version_h"
	echo "#define SVN_VERSION $svn_version"  >> "$svn_version_h"
	echo "#endif"                            >> "$svn_version_h"
fi

# That's all Folks!
##
