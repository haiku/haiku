" vm: set com=b\:\" tw=79 fo=croq2 :
"
"		File manager macros for Vim 5	(UNIX only)
"
"	 Author:  Ra'ul Segura Acevedo <raul@turing.iquimica.unam.mx>
" Last modified:  991221  (1999 Dec 21)
"    Suggestion:  Add the command "so filename" in your ~/.vimrc
"		  (where filename is the name of this file).
"
"
" To get the last version send a mail to <raul@turing.iquimica.unam.mx> with
" SUBJECT: "send file manager macros 5 uuencoded".
" or if you are sure your mail system doesn't split long lines use:
" SUBJECT: "send file manager macros 5".
" To get the version for vim4.5 and vim 4.6 use:
" SUBJECT: "send file manager macros uuencoded".
" SUBJECT: "send file manager macros".
"
" User commands from any buffer:
" _ls	loads vim's current directory.
" _LS	loads the current file's directory. (it doesn't work on a [No File]).
"
" or 'edit' the directory ("vim /tmp", or from vim with ":e .", ":sp ~/bin",
"
" Magic Keys:
"
" In normal mode:
"
"   _H		Show keys/functions table.
"   <Return>	On a file entry.  Loads the file/directory.
"   <Return>	On a line starting with ":" executes the line as a :-command.
"   <Return>	On any other line.  Executes the 'listing command' (such as
"		":r!/bin/ls -lLa /home/user", displayed on line 1)
"   <C-N>	Moves the cursor to the next file entry.
"		([count]<C-N> also works:  9<C-N>)
"   <C-P>	Moves the cursor to the previous file entry.
"		([count]<C-P> also works:  9<C-P>)
"   o		Same as <Return> but it splits the window first.
"   U		Loads the parent directory. (The current buffer is not deleted).
"   R		Refresh (executes the 'listing command').
"   Q		Quit.
"
"   %		(Un)Tag a file entry.  It can be used alone on the current line,
"		or from the visual to toggle the tag of the highlighted lines.
"		The easiest way to untag all the files is pressing "R" (refresh)
"
" The next commands create a new window which prompts the user to execute a
" command on the "selected files".  Where "selected files" means:
"	1) The highlighted files under visual mode.
"	2) If you are not in visual mode, the files  tagged with % (a % is
"	   showed at the beginning of the lines)
"	3) If there are no tagged files, the file entry in the current line
"	4) If there is no file entry in the current line either, then the
"	   macros' working directory.
" To execute the command type "done", if no messages (errors) are returned then
" the window is deleted, otherwise the window displays those messages and "u"
" (undo) can be used to get back the previous contents of the buffer.
" "Q" can be used to quit without executing the command.
"
"   _CP		Copy the file(s) to another place.
"   _MV		Move the file(s) to another place.
"   _LN		Link the file(s) to another place.
"   _SLN	(Symbolic) Link the file(s) to another place.
"   _MK		Make directories (the given files are used as a template)
"   _CM		Change the file(s) mode (chmod).
"   _CO		Change the file(s) owner (chown).
"   _CG		Change the file(s) group (chgrp).
"   _RM		Remove the file(s) (directories have to be empty)
"   _RF		Remove (force) the file(s) (rm -rf is used, directories are
"		removed recursively).  BE CAREFUL with this one.
"   _SH		This macro just list the given files/directories and the
"		macros' working directory, so you can use those names with any
"		command you wish, say tar, gzip, or whatever.
"
" In insert mode:
"   <Return>	Exits insert mode and does the same as <Return> in normal mode.
"   <Tab>	Expands filenames.
"		(These commands are useful to edit the listing-command.)
"
" In the GUI version:
"   <2-LeftMouse>	Same as "o" (Split window and open file/dir)
"   <C-LeftMouse>	Same as "%" (Togle Tag)
"   <C-LeftDrag>	Toggle-Tag files as the mouse pass over them.
" All the commands can be executed from the "LS" menu.
"
""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""

" Syntax highlighting
"
" In a directory window the filenames are highlighted depending on the type of
" file.  There are two sets of colors, one similar to linux's ls-color, and
" other to the default ls-color.
"
" To always use the linux set, uncomment the following line:
" let ls_syntax_linux=1
" To always use the normal set, uncomment the following line:
" let ls_syntax_linux=0
" If ls_syntax_linux is not set the linux colors are used if and only if there
" is an enviroment variable $OSTYPE set to "linux"
"
" Type			Linux Set	Normal Set
"
" Directory		blue		green
" Symbolic link		cyan		cyan
" Named pipe (FIFO)	brown		red
" Socket		magenta		yellow
" Block/Char. Device	yellow		white on a blue background
" Executable		green		magenta
""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""

" The internal macros start with "__" and end with "-".
"
" __CQ-  quotes all the shell metacharacters (see comment just above the macro
"	 to see exactely which chars. are quoted) in the given lines so each
"	 line can be seen as a single pathname. (normally used just in the
"	 current line: 1__CQ-) the line must start with a space.  cursor: ends
"	 in col. 1.  pattern: :-search (& therefore last-search too), replace.
"
" __LS-  the given lines have pathnames, it strips the first occurrence of
"	 the following "words": (anything might be before or after them)
"	 //something//../ ->	/   (// means at least 1 / in a row)
"	 //../		  ->	/   (//../ at the beginning of the pathname)
"				    (// means at least 1 / in a row)
"	 //./		  ->	/   (// means at least 1 / in a row)
"	 //		  ->	/   (// means at least 2 / in a row)
"	 cursor: same line, but might end on column 1.	pattern: all.
"
" __L1-  Adds the 'listing command' (:r!/bin/ls -lLa <dir>) in line 1, which is
"	 always needed to get the (macro's) working directory. <dir> is quoted
"	 with __CQ-.  cursor: line 1, just before the dir. name.  register:-,"
"
" __L1C- checks if the first line is a :-command, calls __L1- if not.	Calls
"	 __LS- to strip the dir. name.	(an aux. map is used: __L-)
"	 cursor: line 1, might end in col 1.  register: -," (iff _L1- called).
"	 pattern: search patterns changed.
"
" __LC-  this macro is executed on a line which starts with ":", assuming it
"	 is a :-command to list files, the line is *copied* (to ensure there
"	 are at least 2 lines in the buf.) to the first line, a message is
"	 printed in the second line and all the other lines are removed.  Then
"	 the :-command is executed.	Finally it adds a / to the directories
"	 names, inserts 2 spaces before every file entry, and :set's nomod.
"	 cursor: last file entry (last line).	pattern: all.	pattern: all.
"	 register: .(msg), 0 (listing com.), 1 and " (previous buf. contents)
"
" __LBD- It creates a normal-command to display the file whose name appears in
"	 the current line (it is inserted in the command line later using ^R-,
"	 this way special vim-char. wont be interpreted as commands, say ^M)
"	 and deletes the current buf.	register: .
"
" __LK-  It creates a normal-command to undo the changes and display the file
"	 whose name appears in the current line (it is inserted in the command
"	 line later using ^R-, this way special vim-char. wont be interpreted
"	 as commands, say ^M) The listing buffer is kept.	register: .
"
" __LF-  called when <CR> is typed in a line which looks like a file entry, it
"	 expects the filename in register 0 (yank), the listing command on line
"	 1 (to get the dir. name), and a macro __LFP- mapped to either __LBD-
"	 or __LK- depending on whether the current buf. is going to be deleted
"	 or kept.  First, it removes everything but the dirname from line 1, It
"	 quotes the filename, adds the dir. if needed, __LS-strips it, loads
"	 that file by executing the command generated by __LFP- and finally
"	 shows the file info (:f), note that it explicitly jumps to line 1 to
"	 avoid "Press return" msgs.	cursor: col 1. (not sure), line 1.
"	 register: ", - (loading command).	patterns: all.
"
" __LN-  looks forward for a filename, if the cursor is at the start of a line
"	 with a file entry, then this macro moves the cursor to the beginning
"	 of the filename, (note that 3E2l must not be replaced with 4W or the
"	 macro will get confused with filenames starting with spaces/tabs)
"	 cursor: beginning of a filename (hopefully).	pattern: last search.
"
" __LWh- sets [Wh]ich action will be done by the macro __LD- depending on
"	 whether the current line looks like a file entry, a :-command, or
"	 none (when "none", the listing command is executed) finally calls
"	 __LD-.	patterns: all.	registers: depend on __LD-
"
" _H	 Open a new window to display the key/function table.
"
" %	 it toggles the tag (leading % char.) of the selected lines (default
"	 just the current, but it can be used from the visual)
"	 cursor: below the end of the selected area.	patterns: all.
"
" __LSC- (shell command) marks the current line (y) so the cursor goes back to
"	 there just before leave the buffer.	Puts the current line, and all
"	 the tagged ones in register l, the listing command is yanked, then
"	 :splits another window, (named: /Command(from_%):_<command name from
"	 macro __L_C->, if it already exists then its previous contents are
"	 kept), pastes the register l as the first paragraph and the second
"	 paragraph starts with a white space followed by the macros' working
"	 directory (gotten from the listing command) this line is marked (also
"	 "y", but remember we R in a different buffer now) and yanked (without
"	 the trailing NL) and finally the macro __CI- is called.  After that
"	 the first paragraph has quoted full pathnames with a leading white
"	 space, the second paragraph is not modified by __CI-, instructions are
"	 inserted automatically at the end of the buffer when it is created.
"	 cursor: first line, column 1, of a new window.	pattern: all.
"	 registers: ", l, 0, -, ., %
"
" __LCM- This macroS (normal & visual) are called from _CP, _MV, etc. to map
"	 __L_C- to the name of the command to be displayed in the status line
"	 and to define __LG- to either ":g/^%/" or ":'<,'>g/^[ %]/" to be used
"	 later in __LSC- to pick the tagged files or the files in the visual
"	 respectively.
"
" _CP, _MV, _LN, _SLN
"	 these macros first call __LCM- and __LSC to create a new window which
"	 prompts the user to copy/move/link/symbolic-link the given files
"	 (files in the visual, if not called from the visual (normal then) the
"	 tagged files, if there are none, the file on the current line, if it
"	 doesn't list a file then the (macros') working directory:) to the
"	 (macro's) working directory.  The cursor is left at the end of working
"	 directory name so it can be easily edited.	registers: l, ", 1, -.
"	 patterns: all.
"
" _RF	 call __LSC to create a new window which prompts the user to delete
"	 the given files, and directories.	rm -rf is used!!
"	 directories are removed recursively.
" _RM	 same as _RF but it uses rm on files and rmdir on directories, (so
"	 directories must be empty)
"
" _MK	 call __LSC to create a new window which prompts the user to create
"	 directories (the given files are used as a template)
"
" _CM (_CHM), _CG (_CHG), _CO (_CHO)
"	 these macros call __LSC to create a new window which prompts the user
"	 to chmod/chgrp/chown the given files. The cursor is left between the
"	 command and the first filename where an argument must be inserted.
"
" _SH	 call __LSC to create a new window which shows the given files first,
"	 then the given directories (separated with an empty line) and finally
"	 the macros' working directory, so the user can pass those names to any
"	 command. (":.g/./" can be used to check if there are directories, and
"	 ":1g/./" to check if there are files) cursor: on the directories
"	 paragraph (or on the second empty line if there were no directories)
"
" <CR>	 in insert-mode goes to normal mode and then execute the normal-mode's
"	 macro <CR>.
" <Tab>  in insert mode, expands file names.
"
" <CR>	 normal-mode set __LFP- to __LBD-, and then calls __LWh- which means
"	 that a new file/directory is going to be loaded and the current
"	 buffer will be removed.
"    on a line with a file entry, that file (or directory) is loaded.
"    on a line starting with : that line is executed as a :-command.
"    on any other line the 'listing command' (normally on line 1) is executed.
"
" __L:- Shows in the command mode line the contents of the file from the cursor
"	position to the end of the line.  Used by <C-N>, <C-P> and BufEnter to
"	remove the garbage in that line showing the current filename entry.
"
" <C-N>	moves the cursor to the next line, and then calls __LN- from the
"	beginning of the line to jump to the next file name.	calls __L:-.
"	[count]<C-N> works too.
"
" <C-P>	moves the cursor to the previous line, and then calls __LN- from the
"	beginning of the line to jump to the next file name.	calls __L:-.
"	[count]<C-P> works too.
"
" o	set __LFP- to __LK- and then calls __LWh-, so works like <CR> but
"	doesn't destroy the current buffer.	It splits the window too.
"
" U	loads parent directory.	(current buffer is not deleted) it first
"	checks for the listing command and deletes everything after the last /
"	adds a ../ at the end of the line, strips the directory's name, and
"	executes the listing command, finally the buffer's name is set
"	accordingly to the directory's.	and the cursor is left in the first
"	file entry.
"
" Q	quit
"
" R	refresh.  before the refresh: if the current line has a file entry then
"	just the filename is used, in any case, the search-metacharacters in
"	the line are quoted, the line is yanked in register "l" (lower L) and
"	the changes undone, after the refresh the cursor is in the last line,
"	it is then moved to the last column where a backward search starts
"	looking for the former current line or the "^:" which always exists in
"	line 1, finally, a dummy ":" command is executed to clean the last
"	message.
"
" __+-	refresh.  call R and then sets mark `.	Called by the /Command*'s
"	macro "done" (m`__+-``).  This is the only internal macro without a L/C
"	in it's name.	That's because when "__+-" is interpreted the cursor
"	may or may not be in the listing window, so, it has to be harmless in a
"	"normal" buffer.
"
" The buffer's refresh is automatically done when it is empty.
" The autocommands "." also :set magic tw=0 wm=0 noai nosi nocin nolisp fo=
"
"
" in the buffers /Command*
" __CJ-	 joins the lines in a paragraph without adding or removing any space.
"	 and :set nomod.	cursor: first non-blank
" __CJD- same as __CJ- but also removes the next line.	cursor: last column.
" __CJj- same as __CJ- but also joins	the next line.	cursor: last column.
"
" done	 filters the buffer through $SHELL, if no messages (errors) are
"	 returned then the window is deleted, otherwise the window displays
"	 those messages and "u" (undo) can be used to get back the previous
"	 contents of the buffer.  Finally, "m`__+-``" is executed, it is
"	 harmless in a normal buffer, but in the listing window it refreshes
"	 the listing.	note that in a normal buffer, if the cursor is in the
"	 last line then m`__+-`` moves the cursor to the beginning of the line.
"
" Q	 quit.
" <Tab>  insert-mode expand filenames.
" <CR>	 insert-mode exits insert-mode.
"
" __CB-  prints a banner at the end of the buffer giving instructions.
"	 the cursor is left at the end of the buffer.
"
" __CI-  (called from __LSC-) it gets what was the listing buffer's current
"	 line in line 1, the tagged lines (if any) from line 2 on, the macros'
"	 working directory in the next paragraph (marked y) and in registers 0
"	 and ", and the cursor in line 1, then checks line 2, if there are
"	 tagged lines then the first line is removed, after that, if line 1
"	 doesn't look like a file entry then it is replaced with a dummy entry
"	 to the "./" file so the first paragraph has always a list of file
"	 entries which is then marked with the visual to execute 4 global
"	 commands on it, 1) remove everything but the filename, 2)quote (__CQ-)
"	 those filenames, 3) add the directory name if needed, or some blanks
"	 otherwise (from the yank buffer) and 4) strips (__LS-) the pathnames.
"	 patterns: all.	registers: -, 1, ".	cursor: in the first paragraph.
"
" The autocomands for /Command* also :set tw=0 wm=0 noai nosi nocin nolisp fo=
" and insert "instructions" in the last line if the last line is empty.
"
" The /File/manager/keys windows has just 2 maps:
" Q	delete the window.
" R	refresh: clears the window and inserts the key/function table.
" and :set tw=0 wm=0 noai nosi nocin nolisp fo=
"
""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""

aug LS
au!
" quote this groups of chars: <CR> <space>!"#$&'()* <Tab> :;<=>? [\]^ ` {|}~
au BufEnter .,/Command* no__CQ- :s/[ -*	:-?[-^`{-~]/\\&/g<CR>
au BufEnter .,/Command* no__LS- :g-\(/\+[^/]*\)\=/\+\.\./\<BAR>/\+\.\=/-s--/-<CR>
au BufEnter . nm__L1- O:r!/bin/ls -la /<Esc>hd0"=expand("%:p")<CR>p1__CQ-Plx
au BufEnter . nm__L1C- :nm__L- 1G<BAR>1v-^:-nm__L- __L1-<CR>__L-1__LS-
au BufEnter . no__LC- yy1GPA<NL>Type _H for a list of commands, see <sfile>:p for details.<Esc>jdG@0:g-^[ 0-9]*d-s-/*$-/<CR>:g/^[^:].*[0-9] \K..  \=[0-9]/s/^/  <CR>:se nomod<CR>
au BufEnter . nn__LBD- Cu:e<C-V><C-R>-<Bar>bd#<Esc>
au BufEnter . nn__LK- Cu:e<C-V><C-R>-<Esc>
au BufEnter . nm__LF- 03xdEdt/$p,D0D"0p1__CQ-:.v-^. /-norm2l"-P0<CR>2x1__LS-0__LFP-y1G@":f<CR>1G
au BufEnter . nn__LN- 0/[0-9] \K..  \=[0-9]<CR>3E2l
au BufEnter . nm__LWh- :nm__LD- __LC-1G__LN-<Bar>.g/^\([ %] [ 0-9]*l.*\) -> .*/s//\1<CR>:.g/^[ %]/nm__LD- __LF-<BAR>norm __LN-hy$<CR>:.g/^:/t0<CR>__L1C-__LD-
au BufEnter . nm_H :5sp!/File/manager/keys<CR>R<C-W>p|map<F1> _H

au BufEnter . nm% v%|vn% :g/^%/s//x<CR>:'<,'>g/^ /s//%<CR>:'<,'>g/^x/s// <CR>:se nomod<CR>:"toggle tag"<CR>`>j
au BufEnter . nm__LSC- myylpX:.g/^\([ %] [ 0-9]*l.*\) -> .*/s//\1<CR>"lyyuylpX__L1C-yy__LG-<BS> [ 0-9]*l/s/\(.*\) -> .*/\1/<CR>__LG-y L<CR>u`y:5new/Command/__L_C-<BAR>cu <C-V>__L_C-<CR>1G"0Pf!Et/d0yl3Pmy$pT/DyyP0y$D"lP__CI-1G0

au BufEnter . vn__LCM- :g/^[ %]/<Home>nn__LG- :<CR>:cno__L_C- |			nn__LCM- :nn__LG- :g/^%/<BAR>cno__L_C- 
au BufEnter . map_CP __LCM-cp<CR>__LSC-icp<Esc>__CJj-
au BufEnter . map_MV __LCM-mv<CR>__LSC-imv<Esc>__CJj-
au BufEnter . map_LN __LCM-ln<CR>__LSC-iln<Esc>__CJj-
au BufEnter . map_SLN __LCM-ln-s<CR>__LSC-iln -s<Esc>__CJj-
au BufEnter . map_RF __LCM-rm-rf<CR>__LSC-irm -rf<Esc>__CJD-
au BufEnter . map_RM __LCM-rm<CR>__LSC-'yD:1,-2g-/$-m'y-<CR>:.g--norm {jirmdir<C-V><ESC>__CJ-<CR>:1g/./norm 0irm<C-V><Esc>__CJ-<CR>$
au BufEnter . map_MK __LCM-mk<CR>__LSC-imkdir<Esc>__CJD-T/
au BufEnter . map_CM __LCM-cm<CR>__LSC-ichmod <Esc>__CJD-0e2l|map_CHM _CH
au BufEnter . map_CG __LCM-cg<CR>__LSC-ichgrp <Esc>__CJD-0e2l|map_CHG _CG
au BufEnter . map_CO __LCM-co<CR>__LSC-ichown <Esc>__CJD-0e2l|map_CHO _CO
au BufEnter . map_SH __LCM-sh<CR>__LSC-}yyp:1,-2g-/$-m'y-2<CR>0

au BufEnter . im<CR> <Esc><CR>|ino<TAB> <C-X><C-F>
au BufEnter . nm<CR> :nm__LFP- __LBD-<CR>__LWh-|nm o :sp<Bar>nm__LFP- __LK-<CR>__LWh-
au BufEnter . nn__L:- y$:"<C-R>""<CR>|nm<C-N> +__LN-__L:-|nm<C-P> -__LN-__L:-
au BufEnter . nm U __L1C-A../<Esc>2hdT/1__LS-__LC-1G0f!Ef/y$__LN-:f <C-R>"<CR>
au BufEnter . nn Q :bd!<CR>
au BufEnter . nm R :.g-^[% ]-norm__LN-d0<CR>__magic0"ly$u__L1C-__LC-$?^:\<Bar> <C-R>l$<CR>l:"refresh"<CR>
au BufEnter . se magic tw=0 wm=0 noai nosi nocin nolisp fo=
au BufEnter . nm__+- Rm`|nn__L- m`|nn__LFP- m`|nn__LG- m`
" [R]efresh only iff the buffer is empty
au BufEnter . let b:ls_done=1
au BufEnter . nn__LD- m`|1v/./nm__LD- __L1C-__LC-1G__LN-
au BufEnter . norm__LD-__L:-

au BufLeave . unm__CQ-|unm__LS-|unm__L1-|unm__L1C-|unm__LC-|unm__LBD-|		unm__LK-|unm__LF-|unm__LN-|unm__LWh-|unm__LD-|unm__L-|unm__LFP-|unm_H|		iun<CR>|iun<Tab>|nun<CR>|nun__L:-|nun<C-N>|nun<C-P>|nun o|nun U|nun Q|		nun R|nun__+-|unm_MK|unm_CP|unm_MV|unm_LN|unm_SLN|unm_RF|unm_RM|unm<F1>|	unm__LSC-|unm%|unm_CHM|unm_CHG|unm_CHO|unm_SH|unm__LCM-|unm__LG-|unm_CM|	unm_CG|unm_CO
au BufLeave . unlet b:ls_done

map_ls :sp %:p:h<CR>
map_LS _ls

au BufEnter /File/manager/keys nn Q :bd!<CR>|nn R :%d<CR>iRa'ul Segura Acevedo <raul@hplara.iquimica.unam.mx><CR>#CR> open	<Bar> #C-N> next	 <Bar> _MV	move   <Bar> _LN  link	<Bar> _CO chown<CR>o    split&open	<Bar> #C-P> previous <Bar> _CP	copy   <Bar> _SLN sym. link	<Bar> _CM chmod<CR>U    up dir.	<Bar> %	tag file <Bar> _RM	delete <Bar> _MK  make dir. <Bar> _CG chgrp<CR>R    refresh	<Bar> Q	*QUIT*   <Bar> _RF	rm -rf <Bar> _SH  shell free style<CR>Insert mode: #CR> enter  #Tab> expand. GUI: #2-clic> split&enter  <C-Clic> tag<Esc>:%s/#/</g<Bar>se nomod<CR>Gz-
au BufLeave /File/manager/keys nun Q|nun R

au BufEnter /Command*	nn__CJ- vap:j!<CR>:se nomod<CR>
au BufEnter /Command*	nn__CJD- vap:j!<CR>JD:se nomod<CR>
au BufEnter /Command*	nn__CJj- vapj:j!<CR>:se nomod<CR>$
au BufEnter /Command*	nm done :%!$SHELL<CR>:g/./"<CR>y$:if@"==""<Bar>norm Qm`__+-``<CR>end  "Command Output"<CR>|nn Q :bd!<CR>
au BufEnter /Command*	ino<TAB> <C-X><C-F>|ino<CR> <Esc>
au BufEnter /Command*	nn __CB- Gi# Normal mode: Type "done" to execute the command, "Q" to quit.<NL># Insert mode: Tab to expand filenames, Return go to Normal mode.<Esc><<<<

au BufEnter /Command*	nm__CI- :2g/./1d<CR>:1v-^[% ]-s-.*-6 Nul 6 6 ./<CR>vapk:g/[0-9] \K..  \=[0-9]/norm /<C-V><CR>3Eld0<CR>gv__CQ-:*s/..//<CR>:.g-^/-norm `y0yt/``<CR>gv:g/^/norm "0P<CR>gv__LS-
au BufEnter /Command*,/File/manager/keys se tw=0 wm=0 noai nosi nocin nolisp nolist isk=@,-,<,>,48-97 fo=
au BufEnter /Command*	nn__L- m`|$v/./nm__L- __CB-
au BufEnter /Command*	norm__L-
au BufLeave /Command*	unm__CQ-|unm__LS-|unm__CI-|unm__CB-|unm__CJ-|unm__CJj-|		unm__CJD-|unm__L-|iu<Tab>|iu<CR>|unm Q|unm done

" the next macro comes from my set of emacs-like incremental search macros
nn__magic	:.g/[[$^.*~\\/?]/s//\\&/g<CR>

"	GUI stuff
if has("gui")
  au BufEnter . nm<2-LeftMouse> :nm__L- <C-V><CR><CR>:.v-/$-nm__L- o<CR>__L-|	nm<C-LeftMouse> <LeftMouse>%|vm<C-LeftMouse> %|nm<C-LeftDrag> <RightDrag>%

  au BufEnter . ime LS.Expand\ name <Tab>|	ime LS.Next\ name <C-N>|	nme LS.Next\ name <C-N>|	ime LS.Prev\.\ name <C-P>|	nme LS.Prev\.\ name <C-P>|	nme LS.Open <CR>|	ime LS.Open <CR>|	nme LS.Split&Open o|	nme LS.Up\ dir U|	nme LS.Refresh R|	nme LS.Close Q|	men LS.\  \ |	men LS.Toggle\ tag %|	men LS.Tag\ all :g/^ /s//%<CR>:se nomod<CR>:"Tag all"<CR>``|	men LS.UnTag\ all :g/^%/s// <CR>:se nomod<CR>:"Untag all"<CR>``|	men LS.Move _MV|	men LS.Copy _CP|	men LS.Delete _RM|	men LS.Make\ dir _MK|	men LS.Link _LN|	men LS.Sym\.\ link _SLN|	men LS.ChOwn _CO|	men LS.ChMod _CM|	men LS.ChGrp _CG|	men LS.Shell _SH|men LS.\ \  \ |men LS.DelTree _RF

  au BufLeave . unm<C-LeftMouse>|nun<2-LeftMouse>|nun<C-LeftDrag>|unme LS|	 nme LS.File's\ dir _LS|nme LS.Vim's\ dir _ls|iunme LS
  nme LS.File's\ dir	_LS
  nme LS.Vim's\ dir	_ls

  au BufEnter /Command* nme Command.Close Q|nme Command.Execute done
  if has("gui_athena")
    au BufEnter /Command* nme Command.sh mm|unme Command.sh
    au BufLeave /Command* nme Command.sh :sp/Command/sh<Bar>se nomod<CR>|unme Command.Close|unme Command.Execute
  el
    au BufLeave /Command* unme Command
  en
endif
au BufEnter * if isdirectory(expand("%"))&&!exists("b:ls_done")|do BufEnter .|en
au BufLeave * if exists("b:ls_done")|do BufLeave .|en

if has("syntax")
  au BufEnter . if exists("syntax_on") && !has("syntax_items")
  au BufEnter . sy match LsAny	".*" contains=LsBody
  au BufEnter . sy match LsExe	"^. \S*x.*" contains=LsBody
  au BufEnter . sy match LsDir	"^. d.*" contains=LsBody
  au BufEnter . sy match LsSln	"^. l.* -> "he=e-4 contains=LsBody
  au BufEnter . sy match LsFifo	"^. p.*" contains=LsBody
  au BufEnter . sy match LsSpl	"^. [cb].*" contains=LsBody
  au BufEnter . sy match LsSock	"^. s.*" contains=LsBody
  au BufEnter . sy match LsBody	contained "^.*[0-9] \K\K\K [ 0-3][0-9] [ 0-9]\S* " contains=Tag
  au BufEnter . sy match Tag	contained "^%.*[0-9] \K\K\K [ 0-3][0-9] [ 0-9]\S*"
  au BufEnter . sy match Comment	"^Type _H.*"
  au BufEnter . sy match Operator	"^:.*"
  au BufEnter . en
  au BufEnter /File/manager/keys sy	match	Tag	"[<_%]\S*\|\<.\>"
  au BufEnter /File/manager/keys sy	match	Comment	"[IGN].\{-}:"
  au BufEnter /Command*	sy	match	Comment	"^#.*" contains=Tag
  au BufEnter /Command*	sy	keyword	Tag	contained done Q Tab Return
  if !exists("did_ls_syntax_inits")
    let did_ls_syntax_inits = 1
    if !exists("ls_syntax_linux")
      let ls_syntax_linux=$OSTYPE=="linux"
    en
    if ls_syntax_linux
      hi LsDir	start=<Esc>[1,34m stop=<Esc>[m ctermfg=blue guifg=blue
      hi LsSln	start=<Esc>[1,36m stop=<Esc>[m ctermfg=cyan guifg=cyan
      hi LsFifo	start=<Esc>[33m stop=<Esc>[m ctermfg=brown guifg=#ffff60
      hi LsSock	start=<Esc>[1,35m stop=<Esc>[m ctermfg=magenta guifg=Magenta
      hi LsSpl	start=<Esc>[1,33m stop=<Esc>[m ctermfg=yellow guifg=#ffff60 gui=bold
      hi LsExe	start=<Esc>[1,32m stop=<Esc>[m ctermfg=green guifg=green
    el
      hi LsDir	start=<Esc>[32m stop=<Esc>[m ctermfg=green guifg=green
      hi LsSln	start=<Esc>[36m stop=<Esc>[m ctermfg=cyan guifg=cyan
      hi LsFifo	start=<Esc>[31m stop=<Esc>[m ctermfg=red guifg=Red
      hi LsSock	start=<Esc>[33m stop=<Esc>[m ctermfg=yellow guifg=#ffff60
      hi LsSpl	start=<Esc>[44,37m stop=<Esc>[m ctermfg=white ctermbg=blue guifg=white guibg=blue
      hi LsExe	start=<Esc>[35m stop=<Esc>[m ctermfg=magenta guifg=Magenta
    en
  let b:current_syntax = "ls"
  en
endif
aug END
