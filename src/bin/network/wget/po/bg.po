# Bulgarian messages for GNU Wget.
# Copyright (C) 1998, 1999, 2000, 2001, 2002 Free Software Foundation, Inc.
# Vesselin Markov <vemarkov@yahoo.com>, 2002
# Части от преводите на Павел Михайлов и Ясен Русев също са използувани.
# Ако имате идеи за подобряване на превода, ни пратете поща на 
# bg-team@bash.info
msgid ""
msgstr ""
"Project-Id-Version: wget 1.8.1\n"
"POT-Creation-Date: 2001-12-17 16:30+0100\n"
"PO-Revision-Date: 2002-03-18 03:11\n"
"Last-Translator: Yassen Roussev <slona@bulgaria.com>\n"
"Language-Team: Bulgarian <bg@bulgaria.com>\n"
"MIME-Version: 1.0\n"
"Content-Type: text/plain; charset=utf-8\n"
"Content-Transfer-Encoding: 8bit\n"

#: src/connect.c:94
#, c-format
msgid "Connecting to %s[%s]:%hu... "
msgstr "Установяване на контакт с %s[%s]:%hu... "

#: src/connect.c:97
#, c-format
msgid "Connecting to %s:%hu... "
msgstr "Установяване на контакт с %s:%hu... "

#: src/connect.c:131
msgid "connected.\n"
msgstr "успешно свързване.\n"

#: src/cookies.c:595
#, c-format
msgid "Error in Set-Cookie, field `%s'"
msgstr "Грешка в Set-Cookie, поле `%s'"

#: src/cookies.c:619
#, c-format
msgid "Syntax error in Set-Cookie at character `%c'.\n"
msgstr "Синтактична грешка при операция Set-Cookie, при `%c'.\n"

#: src/cookies.c:627
msgid "Syntax error in Set-Cookie: premature end of string.\n"
msgstr "Синтактична грешка при операция Set-Cookie: неправилен низ.\n"

#: src/cookies.c:1329
#, c-format
msgid "Cannot open cookies file `%s': %s\n"
msgstr "Не мога да отворя cookies файла \"cookies\", `%s': %s\n"

#: src/cookies.c:1341
#, c-format
msgid "Error writing to `%s': %s\n"
msgstr "Грешка при запис на `%s': %s\n"

#: src/cookies.c:1345
#, c-format
msgid "Error closing `%s': %s\n"
msgstr "Грешка при затваряне на `%s': %s\n"

# ^ msgstr "Грешка при затваряне на `%s': %s\n"
#: src/ftp-ls.c:802
msgid "Unsupported listing type, trying Unix listing parser.\n"
msgstr "Неподдържан вид листинг, пробвам с друг Unix листинг превождач.\n"

#: src/ftp-ls.c:847 src/ftp-ls.c:849
#, c-format
msgid "Index of /%s on %s:%d"
msgstr "Индекс от /%s върху %s:%d"

#: src/ftp-ls.c:871
msgid "time unknown       "
msgstr "неизвестно време   "

#: src/ftp-ls.c:875
msgid "File        "
msgstr "Файл        "

#: src/ftp-ls.c:878
msgid "Directory   "
msgstr "Директория     "

#: src/ftp-ls.c:881
msgid "Link        "
msgstr "Линк      "

#: src/ftp-ls.c:884
msgid "Not sure    "
msgstr "Не съм сигурен    "

#: src/ftp-ls.c:902
#, c-format
msgid " (%s bytes)"
msgstr " (%s байта)"

#. Second: Login with proper USER/PASS sequence.
#: src/ftp.c:179
#, c-format
msgid "Logging in as %s ... "
msgstr "Логвам се като %s ... "

#: src/ftp.c:188 src/ftp.c:241 src/ftp.c:272 src/ftp.c:326 src/ftp.c:419 src/ftp.c:470 src/ftp.c:500 src/ftp.c:564 src/ftp.c:628 src/ftp.c:689 src/ftp.c:737
msgid "Error in server response, closing control connection.\n"
msgstr "Сървърът праща съобщение за грешка, спирам управляващата връзка.\n"

#: src/ftp.c:196
msgid "Error in server greeting.\n"
msgstr "Грешка при ръкуването със сървъра.\n"

#: src/ftp.c:204 src/ftp.c:335 src/ftp.c:428 src/ftp.c:509 src/ftp.c:574 src/ftp.c:638 src/ftp.c:699 src/ftp.c:747
msgid "Write failed, closing control connection.\n"
msgstr "Писането се провали, прекъсвам управляващата връзка.\n"

#: src/ftp.c:211
msgid "The server refuses login.\n"
msgstr "Сървърът не приема логин.\n"

#: src/ftp.c:218
msgid "Login incorrect.\n"
msgstr "Неправилен логин.\n"

#: src/ftp.c:225
msgid "Logged in!\n"
msgstr "Успешно логване!\n"

#: src/ftp.c:250
msgid "Server error, can't determine system type.\n"
msgstr "Грешка при сървъра, не мога да определя вида система .\n"

#: src/ftp.c:260 src/ftp.c:549 src/ftp.c:612 src/ftp.c:669
msgid "done.    "
msgstr "готово.    "

#: src/ftp.c:314 src/ftp.c:449 src/ftp.c:484 src/ftp.c:720 src/ftp.c:768 src/host.c:283
msgid "done.\n"
msgstr "готово.\n"

#: src/ftp.c:343
#, c-format
msgid "Unknown type `%c', closing control connection.\n"
msgstr "Непознат тип `%c', спирам управляващата връзка.\n"

#: src/ftp.c:356
msgid "done.  "
msgstr "готово.  "

#: src/ftp.c:362
msgid "==> CWD not needed.\n"
msgstr "==> CWD не е необходимо.\n"

#: src/ftp.c:435
#, c-format
msgid ""
"No such directory `%s'.\n"
"\n"
msgstr ""
"Няма такава директория `%s'.\n"
"\n"

#. do not CWD
#: src/ftp.c:453
msgid "==> CWD not required.\n"
msgstr "==> CWD не е необходимо.\n"

#: src/ftp.c:516
msgid "Cannot initiate PASV transfer.\n"
msgstr "Не мога да започна пасивен трансфер.\n"

#: src/ftp.c:520
msgid "Cannot parse PASV response.\n"
msgstr "Не мога да разбера PASV отговора.\n"

#: src/ftp.c:541
#, c-format
msgid "couldn't connect to %s:%hu: %s\n"
msgstr "немога да се свържа към %s:%hu: %s\n"

#: src/ftp.c:591
#, c-format
msgid "Bind error (%s).\n"
msgstr "Грешка при свързване (%s).\n"

#: src/ftp.c:598
msgid "Invalid PORT.\n"
msgstr "Невалиден порт.\n"

#: src/ftp.c:651
#, c-format
msgid ""
"\n"
"REST failed; will not truncate `%s'.\n"
msgstr ""
"\n"
"Грешка при REST; няма да прекъсна `%s'.\n"

#: src/ftp.c:658
msgid ""
"\n"
"REST failed, starting from scratch.\n"
msgstr ""
"\n"
"Грешка при REST, започвам отначало.\n"

#: src/ftp.c:707
#, c-format
msgid ""
"No such file `%s'.\n"
"\n"
msgstr ""
"Няма такъв файл `%s'.\n"
"\n"

#: src/ftp.c:755
#, c-format
msgid ""
"No such file or directory `%s'.\n"
"\n"
msgstr ""
"Няма такъв файл или директория `%s'.\n"
"\n"

#: src/ftp.c:839 src/ftp.c:847
#, c-format
msgid "Length: %s"
msgstr "Дължина: %s"

#: src/ftp.c:841 src/ftp.c:849
#, c-format
msgid " [%s to go]"
msgstr " [Остават %s]"

#: src/ftp.c:851
msgid " (unauthoritative)\n"
msgstr " (недостоверно)\n"

#: src/ftp.c:877
#, c-format
msgid "%s: %s, closing control connection.\n"
msgstr "%s: %s, спирам управляващата връзка.\n"

#: src/ftp.c:885
#, c-format
msgid "%s (%s) - Data connection: %s; "
msgstr "%s (%s) - Връзка за данни: %s: "

#: src/ftp.c:902
msgid "Control connection closed.\n"
msgstr "Основната връзка бе затворена.\n"

#: src/ftp.c:920
msgid "Data transfer aborted.\n"
msgstr "Трансферът бе прекъснат.\n"

#: src/ftp.c:984
#, c-format
msgid "File `%s' already there, not retrieving.\n"
msgstr "Файлът `%s' е вече тук, няма да го тегля.\n"

#: src/ftp.c:1054 src/http.c:1527
#, c-format
msgid "(try:%2d)"
msgstr "(опит:%2d)"

#: src/ftp.c:1118 src/http.c:1786
#, c-format
msgid ""
"%s (%s) - `%s' saved [%ld]\n"
"\n"
msgstr ""
"%s (%s) - `%s' записан [%ld]\n"
"\n"

#: src/ftp.c:1160 src/main.c:819 src/recur.c:349 src/retr.c:587
#, c-format
msgid "Removing %s.\n"
msgstr "Премахвам %s.\n"

#: src/ftp.c:1202
#, c-format
msgid "Using `%s' as listing tmp file.\n"
msgstr "Ползвам `%s' като временен списък файл.\n"

#: src/ftp.c:1217
#, c-format
msgid "Removed `%s'.\n"
msgstr "Премахвам `%s'.\n"

#: src/ftp.c:1252
#, c-format
msgid "Recursion depth %d exceeded max. depth %d.\n"
msgstr "Дълбочина на рекурсията %d надвишава макс. дълбочина %d.\n"

#. Remote file is older, file sizes can be compared and
#. are both equal.
#: src/ftp.c:1317
#, c-format
msgid "Remote file no newer than local file `%s' -- not retrieving.\n"
msgstr "Файлът от сървъра не е по-нов от местния `%s' -- не продължавам.\n"

#. Remote file is newer or sizes cannot be matched
#: src/ftp.c:1324
#, c-format
msgid ""
"Remote file is newer than local file `%s' -- retrieving.\n"
"\n"
msgstr "Файлът на сървъра е по-нов от местния `%s' -- започвам да тeгля.\n"

#. Sizes do not match
#: src/ftp.c:1331
#, c-format
msgid ""
"The sizes do not match (local %ld) -- retrieving.\n"
"\n"
msgstr "Големината не съвпада (местен %ld) -- започвам да тeгля.\n"

#: src/ftp.c:1348
msgid "Invalid name of the symlink, skipping.\n"
msgstr "Невалидно име на символична връзка, пропускам.\n"

#: src/ftp.c:1365
#, c-format
msgid ""
"Already have correct symlink %s -> %s\n"
"\n"
msgstr ""
"Символичната връзка е вече поправена %s -> %s.\n"
"\n"

#: src/ftp.c:1373
#, c-format
msgid "Creating symlink %s -> %s\n"
msgstr "Създавам символична връзка %s -> %s\n"

#: src/ftp.c:1384
#, c-format
msgid "Symlinks not supported, skipping symlink `%s'.\n"
msgstr "Символичните връзки не са поддържат, пропускам `%s'.\n"

#: src/ftp.c:1396
#, c-format
msgid "Skipping directory `%s'.\n"
msgstr "Пропускам директория `%s'.\n"

#: src/ftp.c:1405
#, c-format
msgid "%s: unknown/unsupported file type.\n"
msgstr "%s: неизвестен/неподдържан вид файл.\n"

#: src/ftp.c:1432
#, c-format
msgid "%s: corrupt time-stamp.\n"
msgstr "%s: недостоверен времеви печат.\n"

#: src/ftp.c:1457
#, c-format
msgid "Will not retrieve dirs since depth is %d (max %d).\n"
msgstr "Няма да тегля директории, защото дълбочината е %d (максимум %d).\n"

#: src/ftp.c:1507
#, c-format
msgid "Not descending to `%s' as it is excluded/not-included.\n"
msgstr "Не влизам в `%s', тъй като тя е изключенa/не е включенa.\n"

#: src/ftp.c:1561
#, c-format
msgid "Rejecting `%s'.\n"
msgstr "Отказвам `%s'.\n"

#. No luck.
#. #### This message SUCKS.  We should see what was the
#. reason that nothing was retrieved.
#: src/ftp.c:1608
#, c-format
msgid "No matches on pattern `%s'.\n"
msgstr "Няма съвпадения за пример `%s'.\n"

#: src/ftp.c:1673
#, c-format
msgid "Wrote HTML-ized index to `%s' [%ld].\n"
msgstr "Записах HTML-изиран индекс в `%s' [%ld].\n"

#: src/ftp.c:1678
#, c-format
msgid "Wrote HTML-ized index to `%s'.\n"
msgstr "Записах HTML-изиран индекс в `%s'.\n"

#: src/gen_sslfunc.c:109
msgid "Could not seed OpenSSL PRNG; disabling SSL.\n"
msgstr "Не мога да намеря OpenSSL PRNG; продължавам без SSL.\n"

#: src/getopt.c:454
#, c-format
msgid "%s: option `%s' is ambiguous\n"
msgstr "%s: опцията `%s' е многозначна\n"

#: src/getopt.c:478
#, c-format
msgid "%s: option `--%s' doesn't allow an argument\n"
msgstr "%s: опцията `--%s' не позволява аргумент\n"

#: src/getopt.c:483
#, c-format
msgid "%s: option `%c%s' doesn't allow an argument\n"
msgstr "%s: опцията `%c%s' не позволява аргумент\n"

#: src/getopt.c:498
#, c-format
msgid "%s: option `%s' requires an argument\n"
msgstr "%s: опцията `%s' изисква аргумент\n"

#. --option
#: src/getopt.c:528
#, c-format
msgid "%s: unrecognized option `--%s'\n"
msgstr "%s: неразпозната опция `--%s'\n"

#. +option or -option
#: src/getopt.c:532
#, c-format
msgid "%s: unrecognized option `%c%s'\n"
msgstr "%s: неразпозната опция `%c%s'\n"

#. 1003.2 specifies the format of this message.
#: src/getopt.c:563
#, c-format
msgid "%s: illegal option -- %c\n"
msgstr "%s: невалидна опция -- %c\n"

#. 1003.2 specifies the format of this message.
#: src/getopt.c:602
#, c-format
msgid "%s: option requires an argument -- %c\n"
msgstr "%s: опцията изисква аргумент -- %c\n"

#: src/host.c:271
#, c-format
msgid "Resolving %s... "
msgstr "Преобразувам %s... "

#: src/host.c:278
#, c-format
msgid "failed: %s.\n"
msgstr "неуспя: %s.\n"

#: src/host.c:348
msgid "Host not found"
msgstr "Хостът не бе открит"

#: src/host.c:350
msgid "Unknown error"
msgstr "Непозната грешка"

#: src/html-url.c:336
#, c-format
msgid "%s: Cannot resolve incomplete link %s.\n"
msgstr "%s: Не мога да изасня несъвършенния линк %s.\n"

#. this is fatal
#: src/http.c:573
msgid "Failed to set up an SSL context\n"
msgstr "Неуспех при установяване на SSL контекст\n"

#: src/http.c:579
#, c-format
msgid "Failed to load certificates from %s\n"
msgstr "Неуспех при зареждане на сертификати от %s\n"

#: src/http.c:583 src/http.c:591
msgid "Trying without the specified certificate\n"
msgstr "Опитвам без указаният сертификат\n"

#: src/http.c:587
#, c-format
msgid "Failed to get certificate key from %s\n"
msgstr "Неуспех при взимане на ключа към сертификата от %s\n"

#: src/http.c:657 src/http.c:1620
msgid "Unable to establish SSL connection.\n"
msgstr "Немога да установя SSL връзка.\n"

#: src/http.c:666
#, c-format
msgid "Reusing connection to %s:%hu.\n"
msgstr "Използване на вече установена връзка към %s:%hu.\n"

#: src/http.c:868
#, c-format
msgid "Failed writing HTTP request: %s.\n"
msgstr "Неуспех при запис на HTTP искане: %s.\n"

#: src/http.c:873
#, c-format
msgid "%s request sent, awaiting response... "
msgstr "%s изпратено искане, чакам отговор... "

#: src/http.c:917
msgid "End of file while parsing headers.\n"
msgstr "Край на файла (EOF), докато превеждах заглавките.\n"

#: src/http.c:927
#, c-format
msgid "Read error (%s) in headers.\n"
msgstr "Грешка при четене (%s) в заглавките.\n"

#: src/http.c:962
msgid "No data received"
msgstr "Не се получават данни"

#: src/http.c:964
msgid "Malformed status line"
msgstr "Деформиран статус"

#: src/http.c:969
msgid "(no description)"
msgstr "(без описание)"

#: src/http.c:1101
msgid "Authorization failed.\n"
msgstr "Грешка при удостоверяване.\n"

#: src/http.c:1108
msgid "Unknown authentication scheme.\n"
msgstr "Непознат начин на удостоверение.\n"

#: src/http.c:1148
#, c-format
msgid "Location: %s%s\n"
msgstr "Адрес: %s%s\n"

#: src/http.c:1149 src/http.c:1282
msgid "unspecified"
msgstr "неопределен"

#: src/http.c:1150
msgid " [following]"
msgstr " [следва]"

#: src/http.c:1213
msgid ""
"\n"
"    The file is already fully retrieved; nothing to do.\n"
"\n"
msgstr ""
"\n"
"    Файлът е вече изтеглен; няма друга задача.\n"
"\n"

#: src/http.c:1229
#, c-format
msgid ""
"\n"
"Continued download failed on this file, which conflicts with `-c'.\n"
"Refusing to truncate existing file `%s'.\n"
"\n"
msgstr ""
"\n"
"Продълженият трансфер на този файл неуспя, конфликт с `-c'.\n"
"Отказвам да презапиша съществуващият файл `%s'.\n"
"\n"

#. No need to print this output if the body won't be
#. downloaded at all, or if the original server response is
#. printed.
#: src/http.c:1272
msgid "Length: "
msgstr "Дължина: "

#: src/http.c:1277
#, c-format
msgid " (%s to go)"
msgstr " (остават %s)"

#: src/http.c:1282
msgid "ignored"
msgstr "игнориран"

#: src/http.c:1413
msgid "Warning: wildcards not supported in HTTP.\n"
msgstr "Внимание: уайлдкардс не се поддържат в HTTP.\n"

#. If opt.noclobber is turned on and file already exists, do not
#. retrieve the file
#: src/http.c:1443
#, c-format
msgid "File `%s' already there, will not retrieve.\n"
msgstr "Файлът `%s' вече съществува, няма нов запис.\n"

#: src/http.c:1611
#, c-format
msgid "Cannot write to `%s' (%s).\n"
msgstr "Немога да запиша върху `%s' (%s).\n"

#: src/http.c:1630
#, c-format
msgid "ERROR: Redirection (%d) without location.\n"
msgstr "ГРЕШКА: Пре-адресация (%d) без установен адрес.\n"

#: src/http.c:1662
#, c-format
msgid "%s ERROR %d: %s.\n"
msgstr "%s ГРЕШКА: %d: %s.\n"

#: src/http.c:1675
msgid "Last-modified header missing -- time-stamps turned off.\n"
msgstr "Заглавката съдържаща информация относно последна промяна липсва -- полето за дата се изключва.\n"

#: src/http.c:1683
msgid "Last-modified header invalid -- time-stamp ignored.\n"
msgstr "Заглавката съдържаща информация относно последна промяна е невалиднa -- полето за дата се игнорира.\n"

#: src/http.c:1706
#, c-format
msgid ""
"Server file no newer than local file `%s' -- not retrieving.\n"
"\n"
msgstr "Файлът на сървъра не е по-нов от този на диска `%s' -- спирам.\n"

#: src/http.c:1714
#, c-format
msgid "The sizes do not match (local %ld) -- retrieving.\n"
msgstr "Големините не съвпадат (местен %ld) -- продължавам.\n"

#: src/http.c:1718
msgid "Remote file is newer, retrieving.\n"
msgstr "Файлът на сървъра е по-нов, продължавам.\n"

#: src/http.c:1759
#, c-format
msgid ""
"%s (%s) - `%s' saved [%ld/%ld]\n"
"\n"
msgstr ""
"%s (%s) - `%s' записан [%ld/%ld]\n"
"\n"

#: src/http.c:1809
#, c-format
msgid "%s (%s) - Connection closed at byte %ld. "
msgstr "%s (%s) - Връзката бе преустановена при байт %ld. "

#: src/http.c:1818
#, c-format
msgid ""
"%s (%s) - `%s' saved [%ld/%ld])\n"
"\n"
msgstr ""
"%s (%s) - `%s' записан [%ld/%ld])\n"
"\n"

#: src/http.c:1839
#, c-format
msgid "%s (%s) - Connection closed at byte %ld/%ld. "
msgstr "%s (%s) - Връзката бе преустановена при байт %ld/%ld. "

#: src/http.c:1851
#, c-format
msgid "%s (%s) - Read error at byte %ld (%s)."
msgstr "%s (%s) - Грешка при четене, байт %ld (%s)."

#: src/http.c:1860
#, c-format
msgid "%s (%s) - Read error at byte %ld/%ld (%s). "
msgstr "%s (%s) - Грешка при четене, байт %ld/%ld (%s). "

#: src/init.c:355 src/netrc.c:265
#, c-format
msgid "%s: Cannot read %s (%s).\n"
msgstr "%s: Немога да прочета %s (%s).\n"

#: src/init.c:373 src/init.c:379
#, c-format
msgid "%s: Error in %s at line %d.\n"
msgstr "%s: Грешка при %s в ред %d.\n"

#: src/init.c:411
#, c-format
msgid "%s: Warning: Both system and user wgetrc point to `%s'.\n"
msgstr "%s: Внимание: Системният wgetrc и личният сочат към `%s'.\n"

#: src/init.c:503
#, c-format
msgid "%s: BUG: unknown command `%s', value `%s'.\n"
msgstr "%s: БЪГ: непозната команда `%s', стойност `%s'.\n"

#: src/init.c:537
#, c-format
msgid "%s: %s: Cannot convert `%s' to an IP address.\n"
msgstr "%s: %s: Не мога да преобразувам `%s' в IP адрес.\n"

#: src/init.c:570
#, c-format
msgid "%s: %s: Please specify on or off.\n"
msgstr "%s: %s: Моля определете on или off.\n"

#: src/init.c:614
#, c-format
msgid "%s: %s: Please specify always, on, off, or never.\n"
msgstr "%s: %s: Моля определете always, on, off или never.\n"

#: src/init.c:633 src/init.c:900 src/init.c:981
#, c-format
msgid "%s: %s: Invalid specification `%s'.\n"
msgstr "%s: %s: Невалидна спецификация `%s'.\n"

#: src/init.c:789 src/init.c:811 src/init.c:833 src/init.c:859
#, c-format
msgid "%s: Invalid specification `%s'\n"
msgstr "%s: Невалидна спецификация `%s'\n"

#: src/init.c:949
#, c-format
msgid "%s: %s: Invalid progress type `%s'.\n"
msgstr "%s: %s: Невалиден вид напредък `%s'.\n"

#: src/log.c:641
#, fuzzy, c-format
msgid ""
"\n"
"%s received, redirecting output to `%s'.\n"
msgstr ""
"\n"
"%s получени, пре-адресиране на резултата към `%%s'.\n"

#. Eek!  Opening the alternate log file has failed.  Nothing we
#. can do but disable printing completely.
#: src/log.c:648
#, c-format
msgid "%s: %s; disabling logging.\n"
msgstr "%s: %s; спирам записването.\n"

#: src/main.c:116
#, c-format
msgid "Usage: %s [OPTION]... [URL]...\n"
msgstr "Употреба: %s [ОПЦИЯ]... [УРЛ]...\n"

#: src/main.c:124
#, c-format
msgid "GNU Wget %s, a non-interactive network retriever.\n"
msgstr "GNU Wget %s, не-интерактивен мрежов софтуеър за трансфер.\n"

#. Had to split this in parts, so the #@@#%# Ultrix compiler and cpp
#. don't bitch.  Also, it makes translation much easier.
#: src/main.c:129
msgid ""
"\n"
"Mandatory arguments to long options are mandatory for short options too.\n"
"\n"
msgstr ""
"\n"
"Задължителните аргументи за опции в дълъг вид, са задължителни и за тези в опростен вид.\n"
"\n"

#: src/main.c:133
msgid ""
"Startup:\n"
"  -V,  --version           display the version of Wget and exit.\n"
"  -h,  --help              print this help.\n"
"  -b,  --background        go to background after startup.\n"
"  -e,  --execute=COMMAND   execute a `.wgetrc'-style command.\n"
"\n"
msgstr ""
"Пускане:\n"
"  -V,  --version           показва версията на Wget и излиза.\n"
"  -h,  --help              показва тeзи помощни редове.\n"
"  -b,  --background        преминава в заден план.\n"
"  -e,  --execute=КОМАНДА   изпълнява `.wgetrc'-тип команда.\n"
"\n"

#: src/main.c:140
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
"       --sslcertfile=FILE     optional client certificate.\n"
"       --sslcertkey=KEYFILE   optional keyfile for this certificate.\n"
"       --egd-file=FILE        file name of the EGD socket.\n"
"\n"
msgstr ""
"Записване и входови файл:\n"
"  -o,  --output-file=ФАЙЛ     записва съобщенията във ФАЙЛ.\n"
"  -a,  --append-output=ФАЙЛ   добавя съобщенията във ФАЙЛ.\n"
"  -d,  --debug                показва debug резултат.\n"
"  -q,  --quiet                \"тих\" режим (без output).\n"
"  -v,  --verbose              многословно (поначало).\n"
"  -nv, --non-verbose          без многословност (не \"тих\" режим).\n"
"  -i,  --input-file=ФАЙЛ      запис на УРЛ във ФАЙЛ.\n"
"  -F,  --force-html           разглежда входния файл като HTML.\n"
"  -B,  --base=УРЛ             добавя URL към отнасящи се линкове (-F -i файл).\n"
"       --sslcertfile=ФАЙЛ     незадължителен клиентски сертификат -F -i.\n"
"       --sslcertkey=КЛЮЧ      незадължителен ключ към този сертификат.\n"
"       --egd-file=ФАЙЛ        име на файла от EGD сокет.\n"
"\n"

#: src/main.c:155
msgid ""
"Download:\n"
"       --bind-address=ADDRESS   bind to ADDRESS (hostname or IP) on local host.\n"
"  -t,  --tries=NUMBER           set number of retries to NUMBER (0 unlimits).\n"
"  -O   --output-document=FILE   write documents to FILE.\n"
"  -nc, --no-clobber             don't clobber existing files or use .# suffixes.\n"
"  -c,  --continue               resume getting a partially-downloaded file.\n"
"       --progress=TYPE          select progress gauge type.\n"
"  -N,  --timestamping           don't re-retrieve files unless newer than local.\n"
"  -S,  --server-response        print server response.\n"
"       --spider                 don't download anything.\n"
"  -T,  --timeout=SECONDS        set the read timeout to SECONDS.\n"
"  -w,  --wait=SECONDS           wait SECONDS between retrievals.\n"
"       --waitretry=SECONDS      wait 1...SECONDS between retries of a retrieval.\n"
"       --random-wait            wait from 0...2*WAIT secs between retrievals.\n"
"  -Y,  --proxy=on/off           turn proxy on or off.\n"
"  -Q,  --quota=NUMBER           set retrieval quota to NUMBER.\n"
"       --limit-rate=RATE        limit download rate to RATE.\n"
"\n"
msgstr ""
"Запис:\n"
"       --bind-address=АДРЕС     закачване към АДРЕС (име на хост или IP) на местна машина.\n"
"  -t,  --tries=НОМЕР            определя броя опити (0 -- безкарайно).\n"
"  -O   --output-document=ФАЙЛ   записва документите във ФАЙЛ.\n"
"  -nc, --no-clobber             не презаписва вече изтеглени файлове.\n"
"  -c,  --continue               продължава тегленето на файл (при прекъснало състояние).\n"
"       --progress=ВИД           определя вида напредване.\n"
"  -N,  --timestamping           не тегли файлове ако са по-стари от вече съществуващите.\n"
"  -S,  --server-response        показва съобщенията от сървъра.\n"
"       --spider                 не тегли нищо.\n"
"  -T,  --timeout=СЕКУНДИ        ограничава времето за теглене (в секунди).\n"
"  -w,  --wait=СЕКУНДИ           време за изчакване между файлове (в секунди).\n"
"       --waitretry=СЕКУНДИ      време за изчакване между нови опити за теглене (в секунди).\n"
"       --random-wait            изчакване от 0...2 -- ИЗЧАКВАНЕ в секунди между тегления.\n"
"  -Y,  --proxy=on/off           включва/изключва прокси.\n"
"  -Q,  --quota=НОМЕР            ограничава сборния обем за автоматично теглене.\n"
"       --limit-rate=СКОРОСТ     ограничава скоростта на теглене.\n"
"\n"

#: src/main.c:174
msgid ""
"Directories:\n"
"  -nd  --no-directories            don't create directories.\n"
"  -x,  --force-directories         force creation of directories.\n"
"  -nH, --no-host-directories       don't create host directories.\n"
"  -P,  --directory-prefix=PREFIX   save files to PREFIX/...\n"
"       --cut-dirs=NUMBER           ignore NUMBER remote directory components.\n"
"\n"
msgstr ""
"Директории:\n"
"  -nd  --no-directories            не създава директории.\n"
"  -x,  --force-directories         задължава създаването на директории.\n"
"  -nH, --no-host-directories       не създава директории с името на хоста.\n"
"  -P,  --directory-prefix=ПРЕФИКС  записва файловете в ПРЕФИКС/...\n"
"       --cut-dirs=НОМЕР            игнорира НОМЕР на компоненти от страна на сървъра.\n"
"\n"

#: src/main.c:182
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
"\n"
msgstr ""
"HTTP опции:\n"
"       --http-user=ИМЕ       определя http ИМЕ.\n"
"       --http-passwd=ПАРОЛА   определя парола http ПАРОЛА.\n"
"  -C,  --cache=on/off         не/позволява използване на вече кеширана информация от сървъра.\n"
"  -E,  --html-extension       записва всички текстови файлове с .html наставка .\n"
"       --ignore-length        игнорира заглавката `Content-Length'.\n"
"       --header=НИЗ           слага НИЗ в заглавките.\n"
"       --proxy-user=ИМЕ      определя ИМЕ за прокси сървър.\n"
"       --proxy-passwd=ПАРОЛА  определя ПАРОЛА за прокси сървър.\n"
"       --referer=УРЛ          включва `Referer: URL' заглавка в HTTP искането.\n"
"  -s,  --save-headers         записва HTTP заглавките във ФАЙЛ.\n"
"  -U,  --user-agent=АГЕНТ     идентифицира се като АГЕНТ вместо Wget/Версия.\n"
"       --no-http-keep-alive   спира HTTP keep-alive.\n"
"       --cookies=off          не използва бисквитки.\n"
"       --load-cookies=ФАЙЛ    зарежда бисквитките от ФАЙЛ (преди сесия).\n"
"       --save-cookies=ФАЙЛ    записва бисквитките във ФАЙЛ (след сесия).\n"
"\n"

#: src/main.c:200
msgid ""
"FTP options:\n"
"  -nr, --dont-remove-listing   don't remove `.listing' files.\n"
"  -g,  --glob=on/off           turn file name globbing on or off.\n"
"       --passive-ftp           use the \"passive\" transfer mode.\n"
"       --retr-symlinks         when recursing, get linked-to files (not dirs).\n"
"\n"
msgstr ""
"FTP опции:\n"
"  -nr, --dont-remove-listing   не премахва `.listing' файлове.\n"
"  -g,  --glob=on/off           включва/изключва търсенето за схема (от файл).\n"
"       --passive-ftp           използва пасивен модел на трансфер.\n"
"       --retr-symlinks         при рекурсивност, използва самите линкнати файлове (не директории).\n"
"\n"

#: src/main.c:207
msgid ""
"Recursive retrieval:\n"
"  -r,  --recursive          recursive web-suck -- use with care!\n"
"  -l,  --level=NUMBER       maximum recursion depth (inf or 0 for infinite).\n"
"       --delete-after       delete files locally after downloading them.\n"
"  -k,  --convert-links      convert non-relative links to relative.\n"
"  -K,  --backup-converted   before converting file X, back up as X.orig.\n"
"  -m,  --mirror             shortcut option equivalent to -r -N -l inf -nr.\n"
"  -p,  --page-requisites    get all images, etc. needed to display HTML page.\n"
"\n"
msgstr ""
"Рекурсивен трансфер:\n"
"  -r,  --recursive             рекурсивен \"web-suck\" -- използвайте внимателно! .\n"
"  -l,  --level=НОМЕР           максимална \"дълбочина\" при \"web-suck\" (inf/0 за безкрайна).\n"
"       --delete-after          изтриване на файлове след като са изтеглени (местно).\n"
"  -k,  --convert-links         преобразува несвързани линкове в свързани.\n"
"  -K,  --backup-converted      преди да преобразува файл, осигурява (файл.orig).\n"
"  -m,  --mirror                опцията е по-къс еквивалент на -r -N -l inf -nr.\n"
"  -p,  --page-requisites       изтегля всички графични файлове (и т.н.), за пълна HTML станица.\n"
"\n"

#: src/main.c:217
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
"Рекурсивно приемане/отхвърляне:\n"
"  -A,  --accept=СПИСЪК              списък на разрешени окончания.\n"
"  -R,  --reject=СПИСЪК              списък на забранени окончания.\n"
"  -D,  --domains=СПИСЪК             списък на разрешени домейни.\n"
"       --exclude-domains=СПИСЪК     списък на забранени домейни.\n"
"       --follow-ftp                 следва FTP линкове от HTML документи.\n"
"       --follow-tags=СПИСЪК         списък на HTML тагове които следвам.\n"
"  -G,  --ignore-tags=СПИСЪК         списък на HTML тагове които игнорирам..\n"
"  -H,  --span-hosts                 използва други хостове при рекурсия.\n"
"  -L,  --relative                   \tизползва само свързани линкове.\n"
"  -I,  --include-directories=СПИСЪК  \tсписък на всички позволени директории.\n"
"  -X,  --exclude-directories=СПИСЪК  \tсписък на всички забранени директории.\n"
"  -np, --no-parent               \tне се изкачва към родителската директория.\n"
"\n"

#: src/main.c:232
msgid "Mail bug reports and suggestions to <bug-wget@gnu.org>.\n"
msgstr "Изпращайте съобщения за грешки и предложения до <bug-wget@gnu.org>.\n"

#: src/main.c:420
#, c-format
msgid "%s: debug support not compiled in.\n"
msgstr "%s: поддръжката на \"debug\" не е компилирана.\n"

#: src/main.c:472
msgid "Copyright (C) 1995, 1996, 1997, 1998, 2000, 2001 Free Software Foundation, Inc.\n"
msgstr "Запазени авторски права (C) 1995-2001 Free Software Foundation, Inc.\n"

#: src/main.c:474
msgid ""
"This program is distributed in the hope that it will be useful,\n"
"but WITHOUT ANY WARRANTY; without even the implied warranty of\n"
"MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the\n"
"GNU General Public License for more details.\n"
msgstr ""
"Тази програма се разпространява с надеждата че ще бъде полезна,\n"
"но БЕЗ КАКВАТО И ДА Е БИЛА ГАРАНЦИЯ; дори за ТЪРГОВСКА СТОЙНОСТ\n"
"или ГОДНОСТ ЗА ДАДЕНА ЦЕЛ.  Отнесете се към GNU General Public License\n"
"за повече информация.\n"

#: src/main.c:479
msgid ""
"\n"
"Originally written by Hrvoje Niksic <hniksic@arsdigita.com>.\n"
msgstr ""
"\n"
"Първонаписана от Hrvoje Niksic <hniksic@arsdigita.com>.\n"

#: src/main.c:578
#, c-format
msgid "%s: %s: invalid command\n"
msgstr "%s: %s: невалидна команда\n"

#: src/main.c:631
#, c-format
msgid "%s: illegal option -- `-n%c'\n"
msgstr "%s: невалидна опция -- `-n%c'\n"

#. #### Something nicer should be printed here -- similar to the
#. pre-1.5 `--help' page.
#: src/main.c:634 src/main.c:676 src/main.c:722
#, c-format
msgid "Try `%s --help' for more options.\n"
msgstr "Опитайте `%s --help' за повече опции.\n"

#: src/main.c:702
msgid "Can't be verbose and quiet at the same time.\n"
msgstr "Не може да бъде \"многословен\" и \"тих\" едновременно.\n"

#: src/main.c:708
msgid "Can't timestamp and not clobber old files at the same time.\n"
msgstr "Не мога да сложа дата, но и да не презапиша едновременно\n"

#. No URL specified.
#: src/main.c:717
#, c-format
msgid "%s: missing URL\n"
msgstr "%s: УРЛ не е указан\n"

#: src/main.c:834
#, c-format
msgid "No URLs found in %s.\n"
msgstr "УРЛ не е открит в %s.\n"

#: src/main.c:843
#, c-format
msgid ""
"\n"
"FINISHED --%s--\n"
"Downloaded: %s bytes in %d files\n"
msgstr ""
"\n"
"ГОТОВО --%s--\n"
"Изтеглени: %s байта в %d файла\n"

#: src/main.c:851
#, c-format
msgid "Download quota (%s bytes) EXCEEDED!\n"
msgstr "Квотата от (%s байта) бе ПРЕВИШЕНА!\n"

#: src/mswindows.c:120
msgid "Continuing in background.\n"
msgstr "Продължавам на заден план.\n"

#: src/mswindows.c:122 src/utils.c:457
#, c-format
msgid "Output will be written to `%s'.\n"
msgstr "Резултатът ще бъде записван в `%s'.\n"

#: src/mswindows.c:202
#, c-format
msgid "Starting WinHelp %s\n"
msgstr "Стартиране на WinHelp %s\n"

#: src/mswindows.c:229 src/mswindows.c:236
#, c-format
msgid "%s: Couldn't find usable socket driver.\n"
msgstr "%s: Немога да намеря подходящ TCP/IP драйвер.\n"

#: src/netrc.c:365
#, c-format
msgid "%s: %s:%d: warning: \"%s\" token appears before any machine name\n"
msgstr "%s: %s:%d: внимание: \"%s\" има символ преди името на машината\n"

#: src/netrc.c:396
#, c-format
msgid "%s: %s:%d: unknown token \"%s\"\n"
msgstr "%s: %s:%d: непознат символ \"%s\"\n"

#: src/netrc.c:460
#, c-format
msgid "Usage: %s NETRC [HOSTNAME]\n"
msgstr "Употреба: %s NETRC [ИМЕ НА ХОСТ]\n"

#: src/netrc.c:470
#, c-format
msgid "%s: cannot stat %s: %s\n"
msgstr "%s: непълен формат %s: %s\n"

#. Align the [ skipping ... ] line with the dots.  To do
#. that, insert the number of spaces equal to the number of
#. digits in the skipped amount in K.
#: src/progress.c:224
#, c-format
msgid ""
"\n"
"%*s[ skipping %dK ]"
msgstr ""
"\n"
"%*s[ пропускам %dK ]"

#: src/progress.c:391
#, c-format
msgid "Invalid dot style specification `%s'; leaving unchanged.\n"
msgstr "Невалидна точкова спецификация `%s'; оставам непроменено.\n"

#: src/recur.c:350
#, c-format
msgid "Removing %s since it should be rejected.\n"
msgstr "Премахване на %s, след като той би трябвало да бъде отхвърлен.\n"

#: src/recur.c:935
#, c-format
msgid "Converted %d files in %.2f seconds.\n"
msgstr "Преобразувах %d файла за %.2f секунди.\n"

#: src/res.c:540
msgid "Loading robots.txt; please ignore errors.\n"
msgstr "Зареждам robots.txt; моля игнорирайте грешките.\n"

#: src/retr.c:363
msgid "Could not find proxy host.\n"
msgstr "Немога да намеря прокси хоста.\n"

#: src/retr.c:375
#, c-format
msgid "Error parsing proxy URL %s: %s.\n"
msgstr "Грешка при транслирането на прокси УРЛ %s: %s\n"

#: src/retr.c:384
#, c-format
msgid "Error in proxy URL %s: Must be HTTP.\n"
msgstr "Грешка при прокси УРЛ %s: Трябва да е HTTP.\n"

#: src/retr.c:476
#, c-format
msgid "%d redirections exceeded.\n"
msgstr "%d пре-адресациите бяха твърде много.\n"

#: src/retr.c:491
#, c-format
msgid "%s: Redirection cycle detected.\n"
msgstr "%s: Установено зацикляне при пре-адресация.\n"

#: src/retr.c:608
msgid ""
"Giving up.\n"
"\n"
msgstr ""
"Отказвам се.\n"
"\n"

#: src/retr.c:608
msgid ""
"Retrying.\n"
"\n"
msgstr ""
"Продължавам.\n"
"\n"

#: src/url.c:1875
#, c-format
msgid "Converting %s... "
msgstr "Преобразувам %s... "

#: src/url.c:1888
msgid "nothing to do.\n"
msgstr "няма друга задача.\n"

#: src/url.c:1896 src/url.c:1920
#, c-format
msgid "Cannot convert links in %s: %s\n"
msgstr "Немога да преобразувам линковете в %s: %s\n"

#: src/url.c:1911
#, c-format
msgid "Unable to delete `%s': %s\n"
msgstr "Немога да изтрия `%s': %s\n"

#: src/url.c:2117
#, c-format
msgid "Cannot back up %s as %s: %s\n"
msgstr "Немога да подсигуря %s като %s: %s\n"

#: src/utils.c:90
#, c-format
msgid "%s: %s: Not enough memory.\n"
msgstr "%s: %s: Недостиг на памет.\n"

#. parent, no error
#: src/utils.c:455
#, c-format
msgid "Continuing in background, pid %d.\n"
msgstr "Продължавам на заден план, pid %d.\n"

#: src/utils.c:499
#, c-format
msgid "Failed to unlink symlink `%s': %s\n"
msgstr "Грешка при изтриване на символична връзка `%s': %s\n"
