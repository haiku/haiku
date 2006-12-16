# Wget translation file: Serbian language, cyrillic script
# Copyright (C) 2003 Free Software Foundation, Inc.
# This file is distributed under the same license as the wget package.
# Filip Miletić <f.miletic@ewi.tudelft.nl>, 2003.
# -----------------------------------------
# NOTE: External translation submission.
# The true last translator is: Filip Miletić <f.miletic@ewi.tudelft.nl>
#
msgid ""
msgstr ""
"Project-Id-Version: wget 1.9.1\n"
"POT-Creation-Date: 2003-10-11 16:21+0200\n"
"PO-Revision-Date: 2004-01-13 10:07-0500\n"
"Last-Translator: Aleksandar Jelenak <jelenak@netlinkplus.net>\n"
"Language-Team: Serbian <sr@li.org>\n"
"MIME-Version: 1.0\n"
"Content-Type: text/plain; charset=UTF-8\n"
"Content-Transfer-Encoding: 8bit\n"

#: src/connect.c:88
#, c-format
msgid "Unable to convert `%s' to a bind address.  Reverting to ANY.\n"
msgstr "„%s‟ се не може претворити у адресу за повезивање.  Покушавам ANY.\n"

#: src/connect.c:165
#, c-format
msgid "Connecting to %s[%s]:%hu... "
msgstr "Повезујем се са %s[%s]:%hu... "

#: src/connect.c:168
#, c-format
msgid "Connecting to %s:%hu... "
msgstr "Повезујем се са %s:%hu... "

#: src/connect.c:222
msgid "connected.\n"
msgstr "повезано.\n"

#: src/convert.c:171
#, c-format
msgid "Converted %d files in %.2f seconds.\n"
msgstr "Број промењених датотека: %d, време: %.2fs.\n"

#: src/convert.c:197
#, c-format
msgid "Converting %s... "
msgstr "Мењам %s... "

#: src/convert.c:210
msgid "nothing to do.\n"
msgstr "Нема посла.\n"

#: src/convert.c:218 src/convert.c:242
#, c-format
msgid "Cannot convert links in %s: %s\n"
msgstr "Везе у %s се нису могле мењати: %s\n"

#: src/convert.c:233
#, c-format
msgid "Unable to delete `%s': %s\n"
msgstr "Не могу да обришем „%s‟: %s\n"

#: src/convert.c:439
#, c-format
msgid "Cannot back up %s as %s: %s\n"
msgstr "Не може се снимити резерва %s као %s: %s\n"

#: src/cookies.c:606
#, c-format
msgid "Error in Set-Cookie, field `%s'"
msgstr "Грешка са Set-Cookie, поље „%s‟"

#: src/cookies.c:629
#, c-format
msgid "Syntax error in Set-Cookie: %s at position %d.\n"
msgstr "Грешка са Set-Cookie: %s на месту %d.\n"

#: src/cookies.c:1426
#, c-format
msgid "Cannot open cookies file `%s': %s\n"
msgstr "Не могу да отворим датотеку са колачићима „%s‟: %s\n"

#: src/cookies.c:1438
#, c-format
msgid "Error writing to `%s': %s\n"
msgstr "Грешка при упису у „%s‟: %s\n"

#: src/cookies.c:1442
#, c-format
msgid "Error closing `%s': %s\n"
msgstr "Грешка при затварању „%s‟: %s\n"

#: src/ftp-ls.c:812
msgid "Unsupported listing type, trying Unix listing parser.\n"
msgstr "Тип исписа није подржан, пробам парсер за Unix спискове.\n"

#: src/ftp-ls.c:857 src/ftp-ls.c:859
#, c-format
msgid "Index of /%s on %s:%d"
msgstr "Списак за /%s на %s:%d"

#: src/ftp-ls.c:882
msgid "time unknown       "
msgstr "непознато време    "

#: src/ftp-ls.c:886
msgid "File        "
msgstr "Датотека    "

#: src/ftp-ls.c:889
msgid "Directory   "
msgstr "Каталог     "

#: src/ftp-ls.c:892
msgid "Link        "
msgstr "Веза        "

#: src/ftp-ls.c:895
msgid "Not sure    "
msgstr "Није сигурно"

#: src/ftp-ls.c:913
#, c-format
msgid " (%s bytes)"
msgstr " (%s бајт(ов)(а))"

#. Second: Login with proper USER/PASS sequence.
#: src/ftp.c:202
#, c-format
msgid "Logging in as %s ... "
msgstr "Пријављујем се као %s ... "

#: src/ftp.c:215 src/ftp.c:268 src/ftp.c:299 src/ftp.c:353 src/ftp.c:468
#: src/ftp.c:519 src/ftp.c:551 src/ftp.c:611 src/ftp.c:675 src/ftp.c:748
#: src/ftp.c:796
msgid "Error in server response, closing control connection.\n"
msgstr "Грешка у одговору са сервера, затварам контролну везу.\n"

#: src/ftp.c:223
msgid "Error in server greeting.\n"
msgstr "Грешка у поздравној поруци са сервера.\n"

#: src/ftp.c:231 src/ftp.c:362 src/ftp.c:477 src/ftp.c:560 src/ftp.c:621
#: src/ftp.c:685 src/ftp.c:758 src/ftp.c:806
msgid "Write failed, closing control connection.\n"
msgstr "Упис није успео, затварам контролну везу.\n"

#: src/ftp.c:238
msgid "The server refuses login.\n"
msgstr "Сервер не дозвољава пријаву.\n"

#: src/ftp.c:245
msgid "Login incorrect.\n"
msgstr "Пријава није исправна.\n"

#: src/ftp.c:252
msgid "Logged in!\n"
msgstr "Пријављен!\n"

#: src/ftp.c:277
msgid "Server error, can't determine system type.\n"
msgstr "Грешка на серверу, не може се утврдити тип система.\n"

#: src/ftp.c:287 src/ftp.c:596 src/ftp.c:659 src/ftp.c:716
msgid "done.    "
msgstr "обављено."

#: src/ftp.c:341 src/ftp.c:498 src/ftp.c:533 src/ftp.c:779 src/ftp.c:827
msgid "done.\n"
msgstr "готово.\n"

#: src/ftp.c:370
#, c-format
msgid "Unknown type `%c', closing control connection.\n"
msgstr "Непознат тип `%c', затварам контролну везу.\n"

#: src/ftp.c:383
msgid "done.  "
msgstr "обављено.  "

#: src/ftp.c:389
msgid "==> CWD not needed.\n"
msgstr "==> CWD није потребан.\n"

#: src/ftp.c:484
#, c-format
msgid ""
"No such directory `%s'.\n"
"\n"
msgstr ""
"Не постоји директоријум „%s‟.\n"
"\n"

#. do not CWD
#: src/ftp.c:502
msgid "==> CWD not required.\n"
msgstr "==> CWD није потребан.\n"

#: src/ftp.c:567
msgid "Cannot initiate PASV transfer.\n"
msgstr "Не може се покренути PASV пренос.\n"

#: src/ftp.c:571
msgid "Cannot parse PASV response.\n"
msgstr "Одговор на PASV команду је нечитљив.\n"

#: src/ftp.c:588
#, c-format
msgid "couldn't connect to %s:%hu: %s\n"
msgstr "не могу да се повежем на %s:%hu: %s\n"

#: src/ftp.c:638
#, c-format
msgid "Bind error (%s).\n"
msgstr "Грешка при повезивању (%s).\n"

#: src/ftp.c:645
msgid "Invalid PORT.\n"
msgstr "Неисправан PORT.\n"

#: src/ftp.c:698
#, c-format
msgid ""
"\n"
"REST failed; will not truncate `%s'.\n"
msgstr ""
"\n"
"REST није успео; „%s‟ неће бити одсечен.\n"

#: src/ftp.c:705
msgid ""
"\n"
"REST failed, starting from scratch.\n"
msgstr ""
"\n"
"REST није успео, почињем из почетка.\n"

#: src/ftp.c:766
#, c-format
msgid ""
"No such file `%s'.\n"
"\n"
msgstr ""
"Не постоји датотека „%s‟.\n"
"\n"

#: src/ftp.c:814
#, c-format
msgid ""
"No such file or directory `%s'.\n"
"\n"
msgstr ""
"Не постоји датотека или каталог „%s‟.\n"
"\n"

#: src/ftp.c:898 src/ftp.c:906
#, c-format
msgid "Length: %s"
msgstr "Дужина: %s"

#: src/ftp.c:900 src/ftp.c:908
#, c-format
msgid " [%s to go]"
msgstr " [%s преостало]"

#: src/ftp.c:910
msgid " (unauthoritative)\n"
msgstr " (није поуздано)\n"

#: src/ftp.c:936
#, c-format
msgid "%s: %s, closing control connection.\n"
msgstr "%s: %s, затварам контролну везу.\n"

#: src/ftp.c:944
#, c-format
msgid "%s (%s) - Data connection: %s; "
msgstr "%s (%s) - Веза за податке: %s; "

#: src/ftp.c:961
msgid "Control connection closed.\n"
msgstr "Затворена је контролна веза.\n"

#: src/ftp.c:979
msgid "Data transfer aborted.\n"
msgstr "Пренос обустављен.\n"

#: src/ftp.c:1044
#, c-format
msgid "File `%s' already there, not retrieving.\n"
msgstr "Датотека „%s‟ већ постоји, не преузимам поново.\n"

#: src/ftp.c:1114 src/http.c:1716
#, c-format
msgid "(try:%2d)"
msgstr "(пробајте:%2d)"

#: src/ftp.c:1180 src/http.c:1975
#, c-format
msgid ""
"%s (%s) - `%s' saved [%ld]\n"
"\n"
msgstr ""
"%s (%s) - „%s‟ снимљен [%ld]\n"
"\n"

#: src/ftp.c:1222 src/main.c:890 src/recur.c:377 src/retr.c:596
#, c-format
msgid "Removing %s.\n"
msgstr "Уклањам %s.\n"

#: src/ftp.c:1264
#, c-format
msgid "Using `%s' as listing tmp file.\n"
msgstr "Користим „%s‟ као привремену датотеку за списак.\n"

#: src/ftp.c:1279
#, c-format
msgid "Removed `%s'.\n"
msgstr "Уклоњен „%s‟.\n"

#: src/ftp.c:1314
#, c-format
msgid "Recursion depth %d exceeded max. depth %d.\n"
msgstr "Дубина рекурзије %d је већа од максималне дубине: %d.\n"

#. Remote file is older, file sizes can be compared and
#. are both equal.
#: src/ftp.c:1384
#, c-format
msgid "Remote file no newer than local file `%s' -- not retrieving.\n"
msgstr "Удаљена датотека није новија од локалане „%s‟ -- не преузимам.\n"

#. Remote file is newer or sizes cannot be matched
#: src/ftp.c:1391
#, c-format
msgid ""
"Remote file is newer than local file `%s' -- retrieving.\n"
"\n"
msgstr ""
"Удаљена датотека је новија од локалне „%s‟ -- преузимам.\n"
"\n"

#. Sizes do not match
#: src/ftp.c:1398
#, c-format
msgid ""
"The sizes do not match (local %ld) -- retrieving.\n"
"\n"
msgstr ""
"Величине се не поклапају (локална %ld) -- преузимам.\n"
"\n"

#: src/ftp.c:1415
msgid "Invalid name of the symlink, skipping.\n"
msgstr "Неисправно име симболичке везе, прескачем.\n"

#: src/ftp.c:1432
#, c-format
msgid ""
"Already have correct symlink %s -> %s\n"
"\n"
msgstr ""
"Већ имам исправну везу %s -> %s\n"
"\n"

#: src/ftp.c:1440
#, c-format
msgid "Creating symlink %s -> %s\n"
msgstr "Правим везу %s -> %s\n"

#: src/ftp.c:1451
#, c-format
msgid "Symlinks not supported, skipping symlink `%s'.\n"
msgstr "Симболичке везе нису подржане, прескачем везу „%s‟.\n"

#: src/ftp.c:1463
#, c-format
msgid "Skipping directory `%s'.\n"
msgstr "Прескачем директоријум „%s‟.\n"

#: src/ftp.c:1472
#, c-format
msgid "%s: unknown/unsupported file type.\n"
msgstr "%s: тип датотеке је непознат или није подржан.\n"

#: src/ftp.c:1499
#, c-format
msgid "%s: corrupt time-stamp.\n"
msgstr "%s: неисправно време.\n"

#: src/ftp.c:1524
#, c-format
msgid "Will not retrieve dirs since depth is %d (max %d).\n"
msgstr "Нећу преузети директоријуме пошто је дубина %d (највише %d).\n"

#: src/ftp.c:1574
#, c-format
msgid "Not descending to `%s' as it is excluded/not-included.\n"
msgstr "Не спуштам се у „%s‟ пошто је занемарен.\n"

#: src/ftp.c:1639 src/ftp.c:1652
#, c-format
msgid "Rejecting `%s'.\n"
msgstr "Одбијам „%s‟.\n"

#. No luck.
#. #### This message SUCKS.  We should see what was the
#. reason that nothing was retrieved.
#: src/ftp.c:1698
#, c-format
msgid "No matches on pattern `%s'.\n"
msgstr "Ниједна датотека не одговара шаблону „%s‟.\n"

#: src/ftp.c:1764
#, c-format
msgid "Wrote HTML-ized index to `%s' [%ld].\n"
msgstr "Списак је пребачен у HTML и записан у „%s‟ [%ld].\n"

#: src/ftp.c:1769
#, c-format
msgid "Wrote HTML-ized index to `%s'.\n"
msgstr "Списак је пребачен у HTML и записан у „%s‟.\n"

#: src/gen_sslfunc.c:117
msgid "Could not seed OpenSSL PRNG; disabling SSL.\n"
msgstr "OpenSSL PRNG seed није постављен. Искључујем SSL.\n"

#: src/getopt.c:675
#, c-format
msgid "%s: option `%s' is ambiguous\n"
msgstr "%s: Избор „%s‟ је двосмислен\n"

#: src/getopt.c:700
#, c-format
msgid "%s: option `--%s' doesn't allow an argument\n"
msgstr "%s: Избор „--%s‟ се задаје без додатних аргумената\n"

#: src/getopt.c:705
#, c-format
msgid "%s: option `%c%s' doesn't allow an argument\n"
msgstr "%s: Избор „%c%s‟ се задаје без додатних аргумената\n"

#: src/getopt.c:723 src/getopt.c:896
#, c-format
msgid "%s: option `%s' requires an argument\n"
msgstr "%s: За избор „%s‟ потребан је додатни аргумент\n"

#. --option
#: src/getopt.c:752
#, c-format
msgid "%s: unrecognized option `--%s'\n"
msgstr "%s: Избор није препознат: „--%s‟\n"

#. +option or -option
#: src/getopt.c:756
#, c-format
msgid "%s: unrecognized option `%c%s'\n"
msgstr "%s: Избор није препознат: `%c%s'\n"

#. 1003.2 specifies the format of this message.
#: src/getopt.c:782
#, c-format
msgid "%s: illegal option -- %c\n"
msgstr "%s: неисправан избор -- %c\n"

#: src/getopt.c:785
#, c-format
msgid "%s: invalid option -- %c\n"
msgstr "%s: непостојећи избор -- %c\n"

#. 1003.2 specifies the format of this message.
#: src/getopt.c:815 src/getopt.c:945
#, c-format
msgid "%s: option requires an argument -- %c\n"
msgstr "%s: избор захтева аргумент -- %c\n"

#: src/getopt.c:862
#, c-format
msgid "%s: option `-W %s' is ambiguous\n"
msgstr "%s: избор `-W %s' је двосмислен\n"

#: src/getopt.c:880
#, c-format
msgid "%s: option `-W %s' doesn't allow an argument\n"
msgstr "%s: избор `-W %s' не захтева аргумент\n"

#: src/host.c:636
#, c-format
msgid "Resolving %s... "
msgstr "Тражим %s... "

#: src/host.c:656 src/host.c:672
#, c-format
msgid "failed: %s.\n"
msgstr "није успело: %s.\n"

#: src/host.c:674
msgid "failed: timed out.\n"
msgstr "није успело: време је истекло.\n"

#: src/host.c:762
msgid "Host not found"
msgstr "Рачунар није пронађен"

#: src/host.c:764
msgid "Unknown error"
msgstr "Непозната грешка"

#: src/html-url.c:293
#, c-format
msgid "%s: Cannot resolve incomplete link %s.\n"
msgstr "%s: Не може се утврдити шта значи непотпуна веза %s.\n"

#. this is fatal
#: src/http.c:674
msgid "Failed to set up an SSL context\n"
msgstr "Нисам успео да подесим SSL контекст\n"

#: src/http.c:680
#, c-format
msgid "Failed to load certificates from %s\n"
msgstr "Није успело учитавање сертификата из %s\n"

#: src/http.c:684 src/http.c:692
msgid "Trying without the specified certificate\n"
msgstr "Покушавам приступ без потребног сертификата\n"

#: src/http.c:688
#, c-format
msgid "Failed to get certificate key from %s\n"
msgstr "Не могу да преузмем кључ сертификата са %s\n"

#: src/http.c:761 src/http.c:1809
msgid "Unable to establish SSL connection.\n"
msgstr "Не могу да успоставим SSL везу.\n"

#: src/http.c:770
#, c-format
msgid "Reusing connection to %s:%hu.\n"
msgstr "Поново користим везу са %s:%hu.\n"

#: src/http.c:1034
#, c-format
msgid "Failed writing HTTP request: %s.\n"
msgstr "HTTP захтев није успео: %s.\n"

#: src/http.c:1039
#, c-format
msgid "%s request sent, awaiting response... "
msgstr "%s захтев је послат, чека се одговор... "

#: src/http.c:1083
msgid "End of file while parsing headers.\n"
msgstr "Крај датотеке приликом читања заглавља.\n"

#: src/http.c:1093
#, c-format
msgid "Read error (%s) in headers.\n"
msgstr "Грешка у читању (%s) у заглављима.\n"

#: src/http.c:1128
msgid "No data received"
msgstr "Подаци нису примљени"

#: src/http.c:1130
msgid "Malformed status line"
msgstr "Неисправна статусна линија"

#: src/http.c:1135
msgid "(no description)"
msgstr "(нема описа)"

#: src/http.c:1267
msgid "Authorization failed.\n"
msgstr "Пријава није успела.\n"

#: src/http.c:1274
msgid "Unknown authentication scheme.\n"
msgstr "Начин пријаве није познат.\n"

#: src/http.c:1314
#, c-format
msgid "Location: %s%s\n"
msgstr "Место: %s%s\n"

#: src/http.c:1315 src/http.c:1454
msgid "unspecified"
msgstr "није наведено"

#: src/http.c:1316
msgid " [following]"
msgstr " [пратим]"

#: src/http.c:1383
msgid ""
"\n"
"    The file is already fully retrieved; nothing to do.\n"
"\n"
msgstr ""
"\n"
"    Датотека је већ преузета у целини; неће бити поново преузета.\n"
"\n"

#: src/http.c:1401
#, c-format
msgid ""
"\n"
"Continued download failed on this file, which conflicts with `-c'.\n"
"Refusing to truncate existing file `%s'.\n"
"\n"
msgstr ""
"\n"
"Наставак преузимања није успео за ову датотеку, а то је у супротности са\n"
"избором `-c'. Датотека `%s' неће бити скраћена.\n"
"\n"

#. No need to print this output if the body won't be
#. downloaded at all, or if the original server response is
#. printed.
#: src/http.c:1444
msgid "Length: "
msgstr "Дужина: "

#: src/http.c:1449
#, c-format
msgid " (%s to go)"
msgstr " (још %s)"

#: src/http.c:1454
msgid "ignored"
msgstr "занемарено"

#: src/http.c:1598
msgid "Warning: wildcards not supported in HTTP.\n"
msgstr "Упозорење: џокер знаци се не користе за HTTP.\n"

#. If opt.noclobber is turned on and file already exists, do not
#. retrieve the file
#: src/http.c:1628
#, c-format
msgid "File `%s' already there, will not retrieve.\n"
msgstr "Датотека `%s' је већ ту, не преузима се поново.\n"

#: src/http.c:1800
#, c-format
msgid "Cannot write to `%s' (%s).\n"
msgstr "Не може се писати у `%s' (%s).\n"

#: src/http.c:1819
#, c-format
msgid "ERROR: Redirection (%d) without location.\n"
msgstr "ГРЕШКА: Преусмерење (%d) нема одредиште.\n"

#: src/http.c:1851
#, c-format
msgid "%s ERROR %d: %s.\n"
msgstr "%s ГРЕШКА %d: %s.\n"

#: src/http.c:1864
msgid "Last-modified header missing -- time-stamps turned off.\n"
msgstr "Заглавље са датумом последње измене недостаје -- искључено бележење времена.\n"

#: src/http.c:1872
msgid "Last-modified header invalid -- time-stamp ignored.\n"
msgstr "Заглавље са датумом последње измене је неисправно -- искључено бележење времена.\n"

#: src/http.c:1895
#, c-format
msgid ""
"Server file no newer than local file `%s' -- not retrieving.\n"
"\n"
msgstr ""
"Датотека на серверу није новија од локалне датотеке `%s' -- не преузимам.\n"
"\n"

#: src/http.c:1903
#, c-format
msgid "The sizes do not match (local %ld) -- retrieving.\n"
msgstr "Величине се не поклапају (овде је: %ld) -- преузимам.\n"

#: src/http.c:1907
msgid "Remote file is newer, retrieving.\n"
msgstr "Удаљена датотека је новија, преузимам.\n"

#: src/http.c:1948
#, c-format
msgid ""
"%s (%s) - `%s' saved [%ld/%ld]\n"
"\n"
msgstr ""
"%s (%s) - `%s' снимљено [%ld/%ld]\n"
"\n"

#: src/http.c:1998
#, c-format
msgid "%s (%s) - Connection closed at byte %ld. "
msgstr "%s (%s) - Веза је прекинута при преносу бајта %ld. "

#: src/http.c:2007
#, c-format
msgid ""
"%s (%s) - `%s' saved [%ld/%ld])\n"
"\n"
msgstr ""
"%s (%s) - `%s' снимљено [%ld/%ld])\n"
"\n"

#: src/http.c:2028
#, c-format
msgid "%s (%s) - Connection closed at byte %ld/%ld. "
msgstr "%s (%s) - Веза је прекинута при преносу бајта %ld/%ld. "

#: src/http.c:2040
#, c-format
msgid "%s (%s) - Read error at byte %ld (%s)."
msgstr "%s (%s) - Грешка при читању бајта %ld (%s)."

#: src/http.c:2049
#, c-format
msgid "%s (%s) - Read error at byte %ld/%ld (%s). "
msgstr "%s (%s) - Грешка при читању бајта %ld/%ld (%s). "

#: src/init.c:342
#, c-format
msgid "%s: WGETRC points to %s, which doesn't exist.\n"
msgstr "%s: WGETRC помиње датотеку %s која не постоји.\n"

#: src/init.c:398 src/netrc.c:276
#, c-format
msgid "%s: Cannot read %s (%s).\n"
msgstr "%s: Не може се прочитати %s (%s).\n"

#: src/init.c:416 src/init.c:422
#, c-format
msgid "%s: Error in %s at line %d.\n"
msgstr "%s: Грешка у %s на линији %d.\n"

#: src/init.c:454
#, c-format
msgid "%s: Warning: Both system and user wgetrc point to `%s'.\n"
msgstr "%s: Упозорење: И системски и корисников wgetrc показују на `%s'.\n"

#: src/init.c:594
#, c-format
msgid "%s: Invalid --execute command `%s'\n"
msgstr "%s: Команда --execute није препозната: `%s'\n"

#: src/init.c:630
#, c-format
msgid "%s: %s: Invalid boolean `%s', use `on' or `off'.\n"
msgstr "%s: %s: Неисправна Булова вредност `%s', користите `on' или `off'.\n"

#: src/init.c:673
#, c-format
msgid "%s: %s: Invalid boolean `%s', use always, on, off, or never.\n"
msgstr "%s: %s: Неисправна Булова вредност `%s', користите always, on, off, или never.\n"

#: src/init.c:691
#, c-format
msgid "%s: %s: Invalid number `%s'.\n"
msgstr "%s: %s: Неисправан број `%s'.\n"

#: src/init.c:930 src/init.c:949
#, c-format
msgid "%s: %s: Invalid byte value `%s'\n"
msgstr "%s: %s: Неисправна вредност бајта `%s'\n"

#: src/init.c:974
#, c-format
msgid "%s: %s: Invalid time period `%s'\n"
msgstr "%s: %s: Неисправна ознака за период `%s'\n"

#: src/init.c:1051
#, c-format
msgid "%s: %s: Invalid header `%s'.\n"
msgstr "%s: %s: Неисправно заглавље `%s'.\n"

#: src/init.c:1106
#, c-format
msgid "%s: %s: Invalid progress type `%s'.\n"
msgstr "%s: %s: Неисправан тип индикатора напретка `%s'.\n"

#: src/init.c:1157
#, c-format
msgid "%s: %s: Invalid restriction `%s', use `unix' or `windows'.\n"
msgstr "%s: %s: Неисправна ознака ограничења `%s', користите `unix' или `windows'.\n"

#: src/init.c:1198
#, c-format
msgid "%s: %s: Invalid value `%s'.\n"
msgstr "%s: %s: Неисправна вредност `%s'.\n"

#: src/log.c:636
#, c-format
msgid ""
"\n"
"%s received, redirecting output to `%s'.\n"
msgstr ""
"\n"
"%s примљено, излаз преусмерен у `%s'.\n"

#. Eek!  Opening the alternate log file has failed.  Nothing we
#. can do but disable printing completely.
#: src/log.c:643
#, c-format
msgid "%s: %s; disabling logging.\n"
msgstr "%s: %s; искључујем дневник.\n"

#: src/main.c:127
#, c-format
msgid "Usage: %s [OPTION]... [URL]...\n"
msgstr "Употреба: %s [ОПЦИЈА]... [URL]...\n"

#: src/main.c:135
#, c-format
msgid "GNU Wget %s, a non-interactive network retriever.\n"
msgstr "GNU Wget %s, програм за не-интерактивно преузимање датотека.\n"

#. Had to split this in parts, so the #@@#%# Ultrix compiler and cpp
#. don't bitch.  Also, it makes translation much easier.
#: src/main.c:140
msgid ""
"\n"
"Mandatory arguments to long options are mandatory for short options too.\n"
"\n"
msgstr ""
"\n"
"Аргументи који су обавезни за дугачке опције су обавезни и за кратке опције.\n"
"\n"

#: src/main.c:144
msgid ""
"Startup:\n"
"  -V,  --version           display the version of Wget and exit.\n"
"  -h,  --help              print this help.\n"
"  -b,  --background        go to background after startup.\n"
"  -e,  --execute=COMMAND   execute a `.wgetrc'-style command.\n"
"\n"
msgstr ""
"Startup:\n"
"  -V,  --version           исписује ознаку верзије програма wget.\n"
"  -h,  --help              исписује ову помоћну поруку.\n"
"  -b,  --background        пребацује се у позадину после покретања.\n"
"  -e,  --execute=КОМАНДА   изврши команду као да је уписана у `.wgetrc'.\n"
"\n"

#: src/main.c:151
msgid ""
"Logging and input file:\n"
"  -o,  --output-file=FILE     log messages to FILE.\n"
"  -a,  --append-output=FILE   append messages to FILE.\n"
"  -d,  --debug                print debug output.\n"
"  -q,  --quiet                quiet (no output).\n"
"  -v,  --verbose              be verbose (this is the default).\n"
"  -nv, --non-verbose          turn off verboseness, without being quiet.\n"
"  -i,  --input-file=FILE      download URLs found in FILE.\n"
"  -F,  --force-html           treat input file as HTML.\n"
"  -B,  --base=URL             prepends URL to relative links in -F -i file.\n"
"\n"
msgstr ""
"Дневник и улазна датотека:\n"
"  -o,  --output-file=ДАТОТЕКА   запиши поруке у ДАТОТЕКУ.\n"
"  -a,  --append-output=ДАТОТЕКА надовежи поруке на ДАТОТЕКУ.\n"
"  -d,  --debug                  исписуј поруке за дебагирање.\n"
"  -q,  --quiet                  тишина (ништа не исписуј).\n"
"  -v,  --verbose                детаљи (подразумевана вредност).\n"
"  -nv, --non-verbose            не исписуј баш све детаље.\n"
"  -i,  --input-file=ДАТОТЕКА    преузимај са URL-ова из ДАТОТЕКЕ.\n"
"  -F,  --force-html             сматрај да је улаз у HTML.\n"
"  -B,  --base=URL               додаје URL на релативне везе у -F -i датотеци.\n"
"\n"

#: src/main.c:163
msgid ""
"Download:\n"
"  -t,  --tries=NUMBER           set number of retries to NUMBER (0 unlimits).\n"
"       --retry-connrefused      retry even if connection is refused.\n"
"  -O   --output-document=FILE   write documents to FILE.\n"
"  -nc, --no-clobber             don't clobber existing files or use .# suffixes.\n"
"  -c,  --continue               resume getting a partially-downloaded file.\n"
"       --progress=TYPE          select progress gauge type.\n"
"  -N,  --timestamping           don't re-retrieve files unless newer than local.\n"
"  -S,  --server-response        print server response.\n"
"       --spider                 don't download anything.\n"
"  -T,  --timeout=SECONDS        set all timeout values to SECONDS.\n"
"       --dns-timeout=SECS       set the DNS lookup timeout to SECS.\n"
"       --connect-timeout=SECS   set the connect timeout to SECS.\n"
"       --read-timeout=SECS      set the read timeout to SECS.\n"
"  -w,  --wait=SECONDS           wait SECONDS between retrievals.\n"
"       --waitretry=SECONDS      wait 1...SECONDS between retries of a retrieval.\n"
"       --random-wait            wait from 0...2*WAIT secs between retrievals.\n"
"  -Y,  --proxy=on/off           turn proxy on or off.\n"
"  -Q,  --quota=NUMBER           set retrieval quota to NUMBER.\n"
"       --bind-address=ADDRESS   bind to ADDRESS (hostname or IP) on local host.\n"
"       --limit-rate=RATE        limit download rate to RATE.\n"
"       --dns-cache=off          disable caching DNS lookups.\n"
"       --restrict-file-names=OS restrict chars in file names to ones OS allows.\n"
"\n"
msgstr ""
"Преузимање:\n"
"  -t,  --tries=БРОЈ             поставља број покушаја на БРОЈ (0=бесконачно).\n"
"       --retry-connrefused      покушај опет чак и ако је веза одбијена.\n"
"  -O   --output-document=ДАТ    запиши документе у датотеку ДАТ.\n"
"  -nc, --no-clobber             не преписуј датотеке које већ постоје као ни .# суфиксе.\n"
"  -c,  --continue               настави делимично преузете датотеке.\n"
"       --progress=ВРСТА         изабери врсту мерача напретка.\n"
"  -N,  --timestamping           не преузимај уколико су датотеке старије.\n"
"  -S,  --server-response        исписуј одговоре са сервера.\n"
"       --spider                 не преузимај ништа.\n"
"  -T,  --timeout=СЕКУНДЕ        све временске границе постави на СЕКУНДЕ.\n"
"       --dns-timeout=СЕКУНДЕ    време за одговор од DNS-а.\n"
"       --connect-timeout=СЕКУНДЕ време за повезивање.\n"
"       --read-timeout=СЕКУНДЕ   време за читање.\n"
"  -w,  --wait=СЕКУНДЕ           чекај неколико СЕКУНДИ пре преузимања\n"
"       --waitretry=СЕКУНДЕ      чекај најмање 1 а највише СЕКУНДИ пре поновног поушаја.\n"
"       --random-wait            wait from 0...2*WAIT secs between retrievals.\n"
"  -Y,  --proxy=on/off           укључи или искључи приступ преко заступника.\n"
"  -Q,  --quota=БРОЈ             постави границу за преузимање на БРОЈ.\n"
"       --bind-address=АДРЕСА    повежи се на АДРЕСУ (име или IP) у локалу.\n"
"       --limit-rate=ПРОТОК      ограничи проток на ПРОТОК.\n"
"       --dns-cache=off          не чувај DNS упите.\n"
"       --restrict-file-names=OS у имену датотека легални су само знаци које дозвољава оперативни систем OS.\n"
"\n"

#: src/main.c:188
msgid ""
"Directories:\n"
"  -nd, --no-directories            don't create directories.\n"
"  -x,  --force-directories         force creation of directories.\n"
"  -nH, --no-host-directories       don't create host directories.\n"
"  -P,  --directory-prefix=PREFIX   save files to PREFIX/...\n"
"       --cut-dirs=NUMBER           ignore NUMBER remote directory components.\n"
"\n"
msgstr ""
"Директоријуми:\n"
"  -nd, --no-directories            не прави директоријуме.\n"
"  -x,  --force-directories         увек прави директоријуме.\n"
"  -nH, --no-host-directories       не прави директоријуме за хост.\n"
"  -P,  --directory-prefix=ПРЕФИКС  снимај датотеке у ПРЕФИКС/...\n"
"       --cut-dirs=БРОЈ             игнориши БРОЈ компоненти имена директоријума.\n"
"\n"

#: src/main.c:196
msgid ""
"HTTP options:\n"
"       --http-user=USER      set http user to USER.\n"
"       --http-passwd=PASS    set http password to PASS.\n"
"  -C,  --cache=on/off        (dis)allow server-cached data (normally allowed).\n"
"  -E,  --html-extension      save all text/html documents with .html extension.\n"
"       --ignore-length       ignore `Content-Length' header field.\n"
"       --header=STRING       insert STRING among the headers.\n"
"       --proxy-user=USER     set USER as proxy username.\n"
"       --proxy-passwd=PASS   set PASS as proxy password.\n"
"       --referer=URL         include `Referer: URL' header in HTTP request.\n"
"  -s,  --save-headers        save the HTTP headers to file.\n"
"  -U,  --user-agent=AGENT    identify as AGENT instead of Wget/VERSION.\n"
"       --no-http-keep-alive  disable HTTP keep-alive (persistent connections).\n"
"       --cookies=off         don't use cookies.\n"
"       --load-cookies=FILE   load cookies from FILE before session.\n"
"       --save-cookies=FILE   save cookies to FILE after session.\n"
"       --post-data=STRING    use the POST method; send STRING as the data.\n"
"       --post-file=FILE      use the POST method; send contents of FILE.\n"
"\n"
msgstr ""
"HTTP избори:\n"
"       --http-user=USER      постави корисничко име на USER.\n"
"       --http-passwd=PASS    постави лозинку на PASS.\n"
"  -C,  --cache=on/off        да ли је дозвољено кеширање (подразумевано: on).\n"
"  -E,  --html-extension      све документе сними са .html екстензијом.\n"
"       --ignore-length       не користи заглавље `Content-Length'.\n"
"       --header=STRING       убаци STRING у заглавља.\n"
"       --proxy-user=USER     стави име USER при пријави заступнику.\n"
"       --proxy-passwd=PASS   стави лозинку PASS при пријави заступнику.\n"
"       --referer=URL         убаци `Referer: URL' заглавље у HTTP захтев.\n"
"  -s,  --save-headers        сними HTTP заглавља у датотеку.\n"
"  -U,  --user-agent=AGENT    пријави се као AGENT уместо Wget/Верзија.\n"
"       --no-http-keep-alive  искључи одржавање HTTP везе (трајне везе).\n"
"       --cookies=off         не користи колачиће.\n"
"       --load-cookies=FILE   учитај колачиће из датотеке FILE пре преноса.\n"
"       --save-cookies=FILE   сними колачиће у FILE после преноса.\n"
"       --post-data=STRING    користи POST методу; шаљи STRING као податке.\n"
"       --post-file=FILE      користи POST методу; шаљи садржај датотеке FILE.\n"
"\n"

#: src/main.c:217
msgid ""
"HTTPS (SSL) options:\n"
"       --sslcertfile=FILE     optional client certificate.\n"
"       --sslcertkey=KEYFILE   optional keyfile for this certificate.\n"
"       --egd-file=FILE        file name of the EGD socket.\n"
"       --sslcadir=DIR         dir where hash list of CA's are stored.\n"
"       --sslcafile=FILE       file with bundle of CA's\n"
"       --sslcerttype=0/1      Client-Cert type 0=PEM (default) / 1=ASN1 (DER)\n"
"       --sslcheckcert=0/1     Check the server cert agenst given CA\n"
"       --sslprotocol=0-3      choose SSL protocol; 0=automatic,\n"
"                              1=SSLv2 2=SSLv3 3=TLSv1\n"
"\n"
msgstr ""
"HTTPS (SSL) избори:\n"
"       --sslcertfile=FILE     опциони сертификат за клијента.\n"
"       --sslcertkey=KEYFILE   опциона датотека са кључевима.\n"
"       --egd-file=FILE        име датотеке са EGD утичницом.\n"
"       --sslcadir=DIR         директоријум где се чувају CA.\n"
"       --sslcafile=FILE       датотека са списком CA\n"
"       --sslcerttype=0/1      Тип сертификата 0=PEM (подраз.) / 1=ASN1 (DER)\n"
"       --sslcheckcert=0/1     Провери серверов сертификат уз помоћ CA\n"
"       --sslprotocol=0-3      одабери SSL протокол; 0=аутоматски,\n"
"                              1=SSLv2 2=SSLv3 3=TLSv1\n"
"\n"

#: src/main.c:230
msgid ""
"FTP options:\n"
"  -nr, --dont-remove-listing   don't remove `.listing' files.\n"
"  -g,  --glob=on/off           turn file name globbing on or off.\n"
"       --passive-ftp           use the \"passive\" transfer mode.\n"
"       --retr-symlinks         when recursing, get linked-to files (not dirs).\n"
"\n"
msgstr ""
"FTP избори:\n"
"  -nr, --dont-remove-listing   не уклањај датотеке `.listing'.\n"
"  -g,  --glob=on/off           укључи или искључи промену имена датотека.\n"
"       --passive-ftp           користи пасивни начин преноса.\n"
"       --retr-symlinks         при рекурзивном спусту, преузимај линковане датотеке (не директоријуме)\n"
"\n"

#: src/main.c:237
msgid ""
"Recursive retrieval:\n"
"  -r,  --recursive          recursive download.\n"
"  -l,  --level=NUMBER       maximum recursion depth (inf or 0 for infinite).\n"
"       --delete-after       delete files locally after downloading them.\n"
"  -k,  --convert-links      convert non-relative links to relative.\n"
"  -K,  --backup-converted   before converting file X, back up as X.orig.\n"
"  -m,  --mirror             shortcut option equivalent to -r -N -l inf -nr.\n"
"  -p,  --page-requisites    get all images, etc. needed to display HTML page.\n"
"       --strict-comments    turn on strict (SGML) handling of HTML comments.\n"
"\n"
msgstr ""
"Рекурзивни спуст:\n"
"  -r,  --recursive          рекурзивни спуст.\n"
"  -l,  --level=NUMBER       највећа дубина рекурзије (inf или 0 за бесконачну).\n"
"       --delete-after       избриши датотеке у локалу после преузимања.\n"
"  -k,  --convert-links      пребаци релативне везе у апсолутне.\n"
"  -K,  --backup-converted   пре пребацивања направи резервну копију датотеке X са именом X.orig\n"
"  -m,  --mirror             ради исто што и скуп избора -r -N -l inf -nr.\n"
"  -p,  --page-requisites    преузми све слике и остало потребно за приказ HTML стране.\n"
"       --strict-comments    укључи стриктну (SGML) обраду HTML-а.\n"
"\n"

#: src/main.c:248
msgid ""
"Recursive accept/reject:\n"
"  -A,  --accept=LIST                comma-separated list of accepted extensions.\n"
"  -R,  --reject=LIST                comma-separated list of rejected extensions.\n"
"  -D,  --domains=LIST               comma-separated list of accepted domains.\n"
"       --exclude-domains=LIST       comma-separated list of rejected domains.\n"
"       --follow-ftp                 follow FTP links from HTML documents.\n"
"       --follow-tags=LIST           comma-separated list of followed HTML tags.\n"
"  -G,  --ignore-tags=LIST           comma-separated list of ignored HTML tags.\n"
"  -H,  --span-hosts                 go to foreign hosts when recursive.\n"
"  -L,  --relative                   follow relative links only.\n"
"  -I,  --include-directories=LIST   list of allowed directories.\n"
"  -X,  --exclude-directories=LIST   list of excluded directories.\n"
"  -np, --no-parent                  don't ascend to the parent directory.\n"
"\n"
msgstr ""
"Рекурзивно прихватање и одбијање:\n"
"  -A,  --accept=LIST                списак наставака који се прихватају (раздвојени запетама)\n"
"  -R,  --reject=LIST                списак наставака који се одбијају (р.з.)\n"
"  -D,  --domains=LIST               списак домена који се прихватају (р.з.)\n"
"       --exclude-domains=LIST       списак домена који се одбијају (р.з.)\n"
"       --follow-ftp                 прати FTP везе из HTML докумената.\n"
"       --follow-tags=LIST           списак праћених HTML страна (р.з.)\n"
"  -G,  --ignore-tags=LIST           списак одбијених HTML ознака (р.з.)\n"
"  -H,  --span-hosts                 прелази на друге хостове при спусту\n"
"  -L,  --relative                   прати само релативне везе\n"
"  -I,  --include-directories=LIST   списак дозвољених директоријума\n"
"  -X,  --exclude-directories=LIST   списак нежељених директоријума\n"
"  -np, --no-parent                  не иди у родитељски директоријум\n"
"\n"

#: src/main.c:263
msgid "Mail bug reports and suggestions to <bug-wget@gnu.org>.\n"
msgstr "Предлоге и извештаје о грешкама шаљите на <bug-wget@gnu.org>.\n"

#: src/main.c:465
#, c-format
msgid "%s: debug support not compiled in.\n"
msgstr "%s: подршка за дебагирање није уграђена.\n"

#: src/main.c:517
msgid "Copyright (C) 2003 Free Software Foundation, Inc.\n"
msgstr "Copyright (C) 2003 Free Software Foundation, Inc.\n"

#: src/main.c:519
msgid ""
"This program is distributed in the hope that it will be useful,\n"
"but WITHOUT ANY WARRANTY; without even the implied warranty of\n"
"MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the\n"
"GNU General Public License for more details.\n"
msgstr ""
"This program is distributed in the hope that it will be useful,\n"
"but WITHOUT ANY WARRANTY; without even the implied warranty of\n"
"MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the\n"
"GNU General Public License for more details.\n"

#: src/main.c:524
msgid ""
"\n"
"Originally written by Hrvoje Niksic <hniksic@xemacs.org>.\n"
msgstr ""
"\n"
"Први аутор је Хрвоје Никшић (Hrvoje Niksic) <hniksic@xemacs.org>.\n"

#: src/main.c:703
#, c-format
msgid "%s: illegal option -- `-n%c'\n"
msgstr "%s: неисправан избор -- `-n%c'\n"

#. #### Something nicer should be printed here -- similar to the
#. pre-1.5 `--help' page.
#: src/main.c:706 src/main.c:748 src/main.c:794
#, c-format
msgid "Try `%s --help' for more options.\n"
msgstr "Користите `%s --help' за више избора.\n"

#: src/main.c:774
msgid "Can't be verbose and quiet at the same time.\n"
msgstr "Не може се бити тих и детаљан у исто време.\n"

#: src/main.c:780
msgid "Can't timestamp and not clobber old files at the same time.\n"
msgstr "Не могу се мењати ознаке времена и истовремени не мењати старе датотеке.\n"

#. No URL specified.
#: src/main.c:789
#, c-format
msgid "%s: missing URL\n"
msgstr "%s: недостаје URL\n"

#: src/main.c:905
#, c-format
msgid "No URLs found in %s.\n"
msgstr "Ниједан URL није нађен у %s.\n"

#: src/main.c:914
#, c-format
msgid ""
"\n"
"FINISHED --%s--\n"
"Downloaded: %s bytes in %d files\n"
msgstr ""
"\n"
"ГОТОВО --%s--\n"
"Преузето: бајтова: %s, датотека: %d\n"

#: src/main.c:920
#, c-format
msgid "Download quota (%s bytes) EXCEEDED!\n"
msgstr "Прекорачен лимит за преузимање (бајтова: %s)!\n"

#: src/mswindows.c:147
msgid "Continuing in background.\n"
msgstr "Рад се наставља у позадини.\n"

#: src/mswindows.c:149 src/utils.c:487
#, c-format
msgid "Output will be written to `%s'.\n"
msgstr "Излаз ће бити записан у `%s'.\n"

#: src/mswindows.c:245
#, c-format
msgid "Starting WinHelp %s\n"
msgstr "Покрећем WinHelp %s\n"

#: src/mswindows.c:272 src/mswindows.c:279
#, c-format
msgid "%s: Couldn't find usable socket driver.\n"
msgstr "%s: Не постоји погодан уређај за утичницу.\n"

#: src/netrc.c:380
#, c-format
msgid "%s: %s:%d: warning: \"%s\" token appears before any machine name\n"
msgstr "%s: %s:%d: упозорење: текст \"%s\" појављује се пре било ког имена машине\n"

#: src/netrc.c:411
#, c-format
msgid "%s: %s:%d: unknown token \"%s\"\n"
msgstr "%s: %s:%d: ознака \"%s\" није препозната\n"

#: src/netrc.c:475
#, c-format
msgid "Usage: %s NETRC [HOSTNAME]\n"
msgstr "Употреба: %s NETRC [РАЧУНАР]\n"

#: src/netrc.c:485
#, c-format
msgid "%s: cannot stat %s: %s\n"
msgstr "%s: не могу се добити подаци %s: %s\n"

#. Align the [ skipping ... ] line with the dots.  To do
#. that, insert the number of spaces equal to the number of
#. digits in the skipped amount in K.
#: src/progress.c:234
#, c-format
msgid ""
"\n"
"%*s[ skipping %dK ]"
msgstr ""
"\n"
"%*s[ прескочено %dK ]"

#: src/progress.c:401
#, c-format
msgid "Invalid dot style specification `%s'; leaving unchanged.\n"
msgstr "Неисправна ознака са тачном `%s'; ништа се не мења.\n"

#: src/recur.c:378
#, c-format
msgid "Removing %s since it should be rejected.\n"
msgstr "Уклањам %s пошто је означен као нежељен.\n"

#: src/res.c:549
msgid "Loading robots.txt; please ignore errors.\n"
msgstr "Учитавам robots.txt; молим игноришите грешке ако се појаве.\n"

#: src/retr.c:400
#, c-format
msgid "Error parsing proxy URL %s: %s.\n"
msgstr "Грешка при очитавању заступниковог URL-а %s: %s.\n"

#: src/retr.c:408
#, c-format
msgid "Error in proxy URL %s: Must be HTTP.\n"
msgstr "Фрешка у заступниковом URL-у %s: мора бити HTTP.\n"

#: src/retr.c:493
#, c-format
msgid "%d redirections exceeded.\n"
msgstr "%d је превише преусмеравања.\n"

#: src/retr.c:617
msgid ""
"Giving up.\n"
"\n"
msgstr ""
"Одустајем.\n"
"\n"

#: src/retr.c:617
msgid ""
"Retrying.\n"
"\n"
msgstr ""
"Пробам поново.\n"
"\n"

#: src/url.c:621
msgid "No error"
msgstr "Нема грешке"

#: src/url.c:623
msgid "Unsupported scheme"
msgstr "Шаблон није подржан"

#: src/url.c:625
msgid "Empty host"
msgstr "Празна ознака рачунара"

#: src/url.c:627
msgid "Bad port number"
msgstr "Лоше наведен број порта"

#: src/url.c:629
msgid "Invalid user name"
msgstr "Лоше наведено корисничко име"

#: src/url.c:631
msgid "Unterminated IPv6 numeric address"
msgstr "IPv6 адреса није исправно наведена"

#: src/url.c:633
msgid "IPv6 addresses not supported"
msgstr "IPv6 адресе нису подржане"

#: src/url.c:635
msgid "Invalid IPv6 numeric address"
msgstr "Неисправна IPv6 нумеричка адреса"

#: src/utils.c:120
#, c-format
msgid "%s: %s: Not enough memory.\n"
msgstr "%s: %s: Нема довољно меморије.\n"

#. parent, no error
#: src/utils.c:485
#, c-format
msgid "Continuing in background, pid %d.\n"
msgstr "Настављам рад у позадини, ознака pid је %d.\n"

#: src/utils.c:529
#, c-format
msgid "Failed to unlink symlink `%s': %s\n"
msgstr "Неуспело брисање симболичке везе `%s': %s\n"
