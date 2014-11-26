" Coding guidelines check for Haiku.
" Copyright 2010-2014 Haiku, Inc.
" Distributed under the terms of the MIT licence.
"
" Insert this into your vimrc or as some autoloaded file. It will register
" several matchadd regular expressions to try to catch common style violations:
" lines longer than 80 chars, missing space around operators or after keywords,
" indentation with spaces instead of tabs, and so on. Potential problems are
" highlighted with a beautiful red background.
"
" This regex-based method is not perfect: there may be some false positive and
" some cases are not checked. Feel free to improve on this.
"
" The matches are only enabled when starting vim from /Donnees/Dev/Haiku/haiku
" or a subdirectory of it. This way it doesn't get in the way when working on
" other projects. FuncHaikuCheck() can also be called manually to enable the
" matches in other directories.

:highlight Style ctermbg=red guibg=red
:fu FuncHaikuCheck()
	call matchadd('Style', '\%>80v.\+', -1) " line over 80 char
	call matchadd('Style', '^\s* \s*', -1)  " spaces instead of tabs
	call matchadd('Style', '[\t ]\(for\|if\|select\|switch\|while\|catch\)(', -1)
		"missing space after control statement
	call matchadd('Style', '^\(\(?!\/\/\|\/\*\).\)*//\S', -1)
		" Missing space at comment start

	call matchadd('Style', '^\(\(?!\/\/\|\/\*\).\)*\w[,=>+\-*;]\w', -1)
	call matchadd('Style', '^\(\(?!\/\/\|\/\*\).\)*\w\(<<\|>>\)\w', -1)
		"operator without space around it (without false positive on
		"templated<type>)
	call matchadd('Style', '^[^#]^\(\(?!\/\/\|\/\*\).\)*[^<]\zs\w*/\w', -1)
		"operator without space around it (without false positive on
		"#include <dir/file.h>)
	call matchadd('Style', '^[^/]\{2}.*\zs[^*][=/+\-< ]$', -1)
		"operator at end of line (without false positives on /* and */, nor
		"char*\nClass::method())
	call matchadd('Style', '^[^#].*\zs[^<]>$', -1)
		" > operator at EOL (without false positive on #include <file.h>)
	call matchadd('Style', '){', -1) " Missing space after method header
	call matchadd('Style', '}\n\s*else', -1) " Malformed else
	call matchadd('Style', '}\n\s*catch', -1) " Malformed catch
	call matchadd('Style', '\s$', -1) "Spaces at end of line
	call matchadd('Style', ',\S', -1) " Missing space after comma
	call matchadd('Style', '^}\n\{1,2}\S', -1)
		" Less than 2 lines between functions
	call matchadd('Style', '^}\n\{4,}\S', -1)
		" More than 2 lines between functions
:endfu

if stridx(getcwd(), '/Donnees/Dev/Haiku/haiku') == 0
	" Webkit indentation rules
	call FuncHaikuCheck()
endif
