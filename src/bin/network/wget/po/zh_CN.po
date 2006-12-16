# translation of wget-1.9-b5.po to zh_CN
# Copyright (C) 2003 Free Software Foundation, Inc.
# Rongjun Mu <elanmu@sina.com>, 2003.
# Liu Songhe <jackliu9999@263.net>, 2003.
# Zong Yaotang <zong@cosix.com.cn>, 2003.
#
msgid ""
msgstr ""
"Project-Id-Version: wget 1.9-b5\n"
"POT-Creation-Date: 2003-10-11 16:21+0200\n"
"PO-Revision-Date: 2003-10-14 23:28+0800\n"
"Last-Translator: Rongjun Mu <elanmu@sina.com>\n"
"Language-Team: Chinese (simplified) <i18n-translation@lists.linux.net.cn>\n"
"MIME-Version: 1.0\n"
"Content-Type: text/plain; charset=UTF-8\n"
"Content-Transfer-Encoding: 8bit\n"
"X-Generator: KBabel 1.0.2\n"

#: src/connect.c:88
#, c-format
msgid "Unable to convert `%s' to a bind address.  Reverting to ANY.\n"
msgstr "无法转换 “%s”为绑定地址，正在恢复为 ANY。\n"

#: src/connect.c:165
#, c-format
msgid "Connecting to %s[%s]:%hu... "
msgstr "正在连接 %s[%s]:%hu... "

#: src/connect.c:168
#, c-format
msgid "Connecting to %s:%hu... "
msgstr "正在连接 %s:%hu... "

#: src/connect.c:222
msgid "connected.\n"
msgstr "已连接。\n"

#: src/convert.c:171
#, c-format
msgid "Converted %d files in %.2f seconds.\n"
msgstr "已转换 %d 个文件(在 %.2f 秒之内)。\n"

#: src/convert.c:197
#, c-format
msgid "Converting %s... "
msgstr "正在转换 %s... "

#: src/convert.c:210
msgid "nothing to do.\n"
msgstr "不需进行任何操作。\n"

#: src/convert.c:218 src/convert.c:242
#, c-format
msgid "Cannot convert links in %s: %s\n"
msgstr "无法转换 %s 中的链接：%s\n"

#: src/convert.c:233
#, c-format
msgid "Unable to delete `%s': %s\n"
msgstr "无法删除“%s”：%s\n"

#: src/convert.c:439
#, c-format
msgid "Cannot back up %s as %s: %s\n"
msgstr "无法将 %s 备份成 %s：%s\n"

#: src/cookies.c:606
#, c-format
msgid "Error in Set-Cookie, field `%s'"
msgstr "在 Set-Cookie 的中出现错误，字段“%s”"

#: src/cookies.c:629
#, c-format
msgid "Syntax error in Set-Cookie: %s at position %d.\n"
msgstr "在 Set-Cookie 中出现语法错误：%s 在位置 %d 处。\n"

#: src/cookies.c:1426
#, c-format
msgid "Cannot open cookies file `%s': %s\n"
msgstr "无法打开 cookie 文件“%s”：%s\n"

#: src/cookies.c:1438
#, c-format
msgid "Error writing to `%s': %s\n"
msgstr "写入“%s”时发生错误：%s\n"

#: src/cookies.c:1442
#, c-format
msgid "Error closing `%s': %s\n"
msgstr "关闭“%s”时发生错误：%s\n"

#: src/ftp-ls.c:812
msgid "Unsupported listing type, trying Unix listing parser.\n"
msgstr "不支持的文件列表类型，试用 Unix 格式的列表来分析。\n"

#: src/ftp-ls.c:857 src/ftp-ls.c:859
#, c-format
msgid "Index of /%s on %s:%d"
msgstr "/%s 的索引在 %s:%d"

#: src/ftp-ls.c:882
msgid "time unknown       "
msgstr "未知的时间         "

#: src/ftp-ls.c:886
msgid "File        "
msgstr "文件        "

#: src/ftp-ls.c:889
msgid "Directory   "
msgstr "目录        "

#: src/ftp-ls.c:892
msgid "Link        "
msgstr "链接        "

#: src/ftp-ls.c:895
msgid "Not sure    "
msgstr "不确定    "

#: src/ftp-ls.c:913
#, c-format
msgid " (%s bytes)"
msgstr " (%s 字节)"

#. Second: Login with proper USER/PASS sequence.
#: src/ftp.c:202
#, c-format
msgid "Logging in as %s ... "
msgstr "正在以 %s 登录 ... "

#: src/ftp.c:215 src/ftp.c:268 src/ftp.c:299 src/ftp.c:353 src/ftp.c:468
#: src/ftp.c:519 src/ftp.c:551 src/ftp.c:611 src/ftp.c:675 src/ftp.c:748
#: src/ftp.c:796
msgid "Error in server response, closing control connection.\n"
msgstr "服务器响应时发生错误，正在关闭控制连接。\n"

#: src/ftp.c:223
msgid "Error in server greeting.\n"
msgstr "服务器消息出现错误。\n"

#: src/ftp.c:231 src/ftp.c:362 src/ftp.c:477 src/ftp.c:560 src/ftp.c:621
#: src/ftp.c:685 src/ftp.c:758 src/ftp.c:806
msgid "Write failed, closing control connection.\n"
msgstr "写入失败，正在关闭控制连接。\n"

#: src/ftp.c:238
msgid "The server refuses login.\n"
msgstr "服务器拒绝登录。\n"

#: src/ftp.c:245
msgid "Login incorrect.\n"
msgstr "登录不正确。\n"

#: src/ftp.c:252
msgid "Logged in!\n"
msgstr "登录成功！\n"

#: src/ftp.c:277
msgid "Server error, can't determine system type.\n"
msgstr "服务器错误，无法确定操作系统的类型。\n"

#: src/ftp.c:287 src/ftp.c:596 src/ftp.c:659 src/ftp.c:716
msgid "done.    "
msgstr "完成。    "

#: src/ftp.c:341 src/ftp.c:498 src/ftp.c:533 src/ftp.c:779 src/ftp.c:827
msgid "done.\n"
msgstr "完成。\n"

#: src/ftp.c:370
#, c-format
msgid "Unknown type `%c', closing control connection.\n"
msgstr "未知的类别“%c”，正在关闭控制连接。\n"

#: src/ftp.c:383
msgid "done.  "
msgstr "完成。  "

#: src/ftp.c:389
msgid "==> CWD not needed.\n"
msgstr "==> 不需要 CWD。\n"

#: src/ftp.c:484
#, c-format
msgid ""
"No such directory `%s'.\n"
"\n"
msgstr ""
"目录“%s”不存在。\n"
"\n"

#. do not CWD
#: src/ftp.c:502
msgid "==> CWD not required.\n"
msgstr "==> 不需要 CWD。\n"

#: src/ftp.c:567
msgid "Cannot initiate PASV transfer.\n"
msgstr "无法启动 PASV 传输。\n"

#: src/ftp.c:571
msgid "Cannot parse PASV response.\n"
msgstr "无法解析 PASV 响应内容。\n"

#: src/ftp.c:588
#, c-format
msgid "couldn't connect to %s:%hu: %s\n"
msgstr "无法连接到 %s:%hu ：%s\n"

#: src/ftp.c:638
#, c-format
msgid "Bind error (%s).\n"
msgstr "Bind 错误(%s)。\n"

#: src/ftp.c:645
msgid "Invalid PORT.\n"
msgstr "无效的 PORT(端口)。\n"

#: src/ftp.c:698
#, c-format
msgid ""
"\n"
"REST failed; will not truncate `%s'.\n"
msgstr ""
"\n"
"重置 (REST) 失败；不会截短‘%s’。\n"

#: src/ftp.c:705
msgid ""
"\n"
"REST failed, starting from scratch.\n"
msgstr ""
"\n"
"重置 (REST) 失败，重新开始发送。\n"

#: src/ftp.c:766
#, c-format
msgid ""
"No such file `%s'.\n"
"\n"
msgstr ""
"文件“%s”不存在。\n"
"\n"

#: src/ftp.c:814
#, c-format
msgid ""
"No such file or directory `%s'.\n"
"\n"
msgstr ""
"文件或目录“%s”不存在。\n"
"\n"

#: src/ftp.c:898 src/ftp.c:906
#, c-format
msgid "Length: %s"
msgstr "长度：%s"

#: src/ftp.c:900 src/ftp.c:908
#, c-format
msgid " [%s to go]"
msgstr " [尚有 %s]"

#: src/ftp.c:910
msgid " (unauthoritative)\n"
msgstr " (非正式数据)\n"

#: src/ftp.c:936
#, c-format
msgid "%s: %s, closing control connection.\n"
msgstr "%s：%s，正在关闭控制连接。\n"

#: src/ftp.c:944
#, c-format
msgid "%s (%s) - Data connection: %s; "
msgstr "%s (%s) - 数据连接：%s；"

#: src/ftp.c:961
msgid "Control connection closed.\n"
msgstr "已关闭控制连接。\n"

#: src/ftp.c:979
msgid "Data transfer aborted.\n"
msgstr "数据传输已被中止。\n"

#: src/ftp.c:1044
#, c-format
msgid "File `%s' already there, not retrieving.\n"
msgstr "文件“%s”已经存在，不取回。\n"

#: src/ftp.c:1114 src/http.c:1716
#, c-format
msgid "(try:%2d)"
msgstr "(尝试次数：%2d)"

#: src/ftp.c:1180 src/http.c:1975
#, c-format
msgid ""
"%s (%s) - `%s' saved [%ld]\n"
"\n"
msgstr ""
"%s (%s) - 已保存‘%s’[%ld]\n"
"\n"

#: src/ftp.c:1222 src/main.c:890 src/recur.c:377 src/retr.c:596
#, c-format
msgid "Removing %s.\n"
msgstr "正在删除 %s。\n"

#: src/ftp.c:1264
#, c-format
msgid "Using `%s' as listing tmp file.\n"
msgstr "使用“%s”作为列表临时文件。\n"

#: src/ftp.c:1279
#, c-format
msgid "Removed `%s'.\n"
msgstr "已删除“%s”。\n"

#: src/ftp.c:1314
#, c-format
msgid "Recursion depth %d exceeded max. depth %d.\n"
msgstr "链接递归深度 %d 超过最大值 %d。\n"

#. Remote file is older, file sizes can be compared and
#. are both equal.
#: src/ftp.c:1384
#, c-format
msgid "Remote file no newer than local file `%s' -- not retrieving.\n"
msgstr "远程文件不比本地文件“%s”新 -- 不取回。\n"

#. Remote file is newer or sizes cannot be matched
#: src/ftp.c:1391
#, c-format
msgid ""
"Remote file is newer than local file `%s' -- retrieving.\n"
"\n"
msgstr "远程文件较本地文件“%s”新 -- 取回。\n"

#. Sizes do not match
#: src/ftp.c:1398
#, c-format
msgid ""
"The sizes do not match (local %ld) -- retrieving.\n"
"\n"
msgstr ""
"文件大小不符(本地文件 %ld) -- 取回。\n"
"\n"

#: src/ftp.c:1415
msgid "Invalid name of the symlink, skipping.\n"
msgstr "无效的符号连接名，跳过。\n"

#: src/ftp.c:1432
#, c-format
msgid ""
"Already have correct symlink %s -> %s\n"
"\n"
msgstr ""
"已经存在正确的符号连接 %s -> %s\n"
"\n"

#: src/ftp.c:1440
#, c-format
msgid "Creating symlink %s -> %s\n"
msgstr "正在创建符号链接 %s -> %s\n"

#: src/ftp.c:1451
#, c-format
msgid "Symlinks not supported, skipping symlink `%s'.\n"
msgstr "不支持符号连接，正在跳过符号连接“%s”。\n"

#: src/ftp.c:1463
#, c-format
msgid "Skipping directory `%s'.\n"
msgstr "正在跳过目录“%s”。\n"

#: src/ftp.c:1472
#, c-format
msgid "%s: unknown/unsupported file type.\n"
msgstr "%s：未知的/不支持的文件类型。\n"

#: src/ftp.c:1499
#, c-format
msgid "%s: corrupt time-stamp.\n"
msgstr "%s：错误的时间戳标记。\n"

#: src/ftp.c:1524
#, c-format
msgid "Will not retrieve dirs since depth is %d (max %d).\n"
msgstr "因为深度为 %d(最大值为 %d)，所以不取回。\n"

#: src/ftp.c:1574
#, c-format
msgid "Not descending to `%s' as it is excluded/not-included.\n"
msgstr "不进入“%s”目录因为已被排除或未被包含进来。\n"

#: src/ftp.c:1639 src/ftp.c:1652
#, c-format
msgid "Rejecting `%s'.\n"
msgstr "拒绝“%s”。\n"

#. No luck.
#. #### This message SUCKS.  We should see what was the
#. reason that nothing was retrieved.
#: src/ftp.c:1698
#, c-format
msgid "No matches on pattern `%s'.\n"
msgstr "没有与模式“%s”相符合的。\n"

#: src/ftp.c:1764
#, c-format
msgid "Wrote HTML-ized index to `%s' [%ld].\n"
msgstr "已经将 HTML 格式的索引写入到“%s” [%ld]。\n"

#: src/ftp.c:1769
#, c-format
msgid "Wrote HTML-ized index to `%s'.\n"
msgstr "已经将 HTML 格式的索引写入到“%s”。\n"

#: src/gen_sslfunc.c:117
msgid "Could not seed OpenSSL PRNG; disabling SSL.\n"
msgstr "无法产生 OpenSSL 随机数产生程序(PRNG)使用的种子；禁用 SSL。\n"

#: src/getopt.c:675
#, c-format
msgid "%s: option `%s' is ambiguous\n"
msgstr "%s：选项“%s”不明确\n"

#: src/getopt.c:700
#, c-format
msgid "%s: option `--%s' doesn't allow an argument\n"
msgstr "%s：选项“--%s”不允许有参数\n"

#: src/getopt.c:705
#, c-format
msgid "%s: option `%c%s' doesn't allow an argument\n"
msgstr "%s：选项“%c%s”不允许有参数\n"

#: src/getopt.c:723 src/getopt.c:896
#, c-format
msgid "%s: option `%s' requires an argument\n"
msgstr "%s：选项“%s”需要参数\n"

#. --option
#: src/getopt.c:752
#, c-format
msgid "%s: unrecognized option `--%s'\n"
msgstr "%s：无法识别的选项“--%s”\n"

#. +option or -option
#: src/getopt.c:756
#, c-format
msgid "%s: unrecognized option `%c%s'\n"
msgstr "%s：无法识别的选项“%c%s”\n"

#. 1003.2 specifies the format of this message.
#: src/getopt.c:782
#, c-format
msgid "%s: illegal option -- %c\n"
msgstr "%s：非法选项 -- %c\n"

#: src/getopt.c:785
#, c-format
msgid "%s: invalid option -- %c\n"
msgstr "%s：无效选项 -- %c\n"

#. 1003.2 specifies the format of this message.
#: src/getopt.c:815 src/getopt.c:945
#, c-format
msgid "%s: option requires an argument -- %c\n"
msgstr "%s：选项需要参数 -- %c\n"

#: src/getopt.c:862
#, c-format
msgid "%s: option `-W %s' is ambiguous\n"
msgstr "%s：选项“-W %s”不明确\n"

#: src/getopt.c:880
#, c-format
msgid "%s: option `-W %s' doesn't allow an argument\n"
msgstr "%s：选项“-W %s”不允许有参数\n"

#: src/host.c:636
#, c-format
msgid "Resolving %s... "
msgstr "正在解析主机 %s... "

#: src/host.c:656 src/host.c:672
#, c-format
msgid "failed: %s.\n"
msgstr "失败：%s。\n"

#: src/host.c:674
msgid "failed: timed out.\n"
msgstr "失败：超时。\n"

#: src/host.c:762
msgid "Host not found"
msgstr "找不到主机"

#: src/host.c:764
msgid "Unknown error"
msgstr "未知的错误"

#: src/html-url.c:293
#, c-format
msgid "%s: Cannot resolve incomplete link %s.\n"
msgstr "%s：无法解析不完整的链接 %s。\n"

#. this is fatal
#: src/http.c:674
msgid "Failed to set up an SSL context\n"
msgstr "无法创建 SSL context\n"

#: src/http.c:680
#, c-format
msgid "Failed to load certificates from %s\n"
msgstr "无法从 %s 载入证书 (certificate)\n"

#: src/http.c:684 src/http.c:692
msgid "Trying without the specified certificate\n"
msgstr "尝试不载入指定的证书 (certificate)\n"

#: src/http.c:688
#, c-format
msgid "Failed to get certificate key from %s\n"
msgstr "无法从 %s 获取证书密钥\n"

#: src/http.c:761 src/http.c:1809
msgid "Unable to establish SSL connection.\n"
msgstr "无法建立 SSL 连接。\n"

#: src/http.c:770
#, c-format
msgid "Reusing connection to %s:%hu.\n"
msgstr "再使用到 %s:%hu 的连接。\n"

#: src/http.c:1034
#, c-format
msgid "Failed writing HTTP request: %s.\n"
msgstr "无法写入 HTTP 请求：%s。\n"

#: src/http.c:1039
#, c-format
msgid "%s request sent, awaiting response... "
msgstr "已发出 %s 请求，正在等待回应... "

#: src/http.c:1083
msgid "End of file while parsing headers.\n"
msgstr "正在分析文件头时，文件已结束。\n"

#: src/http.c:1093
#, c-format
msgid "Read error (%s) in headers.\n"
msgstr "读取文件头错误 (%s)。\n"

#: src/http.c:1128
msgid "No data received"
msgstr "没有接收到数据"

#: src/http.c:1130
msgid "Malformed status line"
msgstr "不正常的状态行"

#: src/http.c:1135
msgid "(no description)"
msgstr "(没有描述)"

#: src/http.c:1267
msgid "Authorization failed.\n"
msgstr "验证失败。\n"

#: src/http.c:1274
msgid "Unknown authentication scheme.\n"
msgstr "未知的验证方式。\n"

#: src/http.c:1314
#, c-format
msgid "Location: %s%s\n"
msgstr "位置：%s%s\n"

#: src/http.c:1315 src/http.c:1454
msgid "unspecified"
msgstr "未指定"

#: src/http.c:1316
msgid " [following]"
msgstr " [跟随至新的 URL]"

#: src/http.c:1383
msgid ""
"\n"
"    The file is already fully retrieved; nothing to do.\n"
"\n"
msgstr ""
"\n"
"    文件已下载完成；不会进行任何操作。\n"
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
"无法续传此文件，与“-c”选项的意义冲突。\n"
"不会截短已存在的文件“%s”。\n"
"\n"

#. No need to print this output if the body won't be
#. downloaded at all, or if the original server response is
#. printed.
#: src/http.c:1444
msgid "Length: "
msgstr "长度："

#: src/http.c:1449
#, c-format
msgid " (%s to go)"
msgstr " (尚有 %s)"

#: src/http.c:1454
msgid "ignored"
msgstr "已忽略"

#: src/http.c:1598
msgid "Warning: wildcards not supported in HTTP.\n"
msgstr "警告：HTTP 不支持通配符。\n"

#. If opt.noclobber is turned on and file already exists, do not
#. retrieve the file
#: src/http.c:1628
#, c-format
msgid "File `%s' already there, will not retrieve.\n"
msgstr "文件“%s”已经存在，不会取回。\n"

#: src/http.c:1800
#, c-format
msgid "Cannot write to `%s' (%s).\n"
msgstr "无法写到“%s”(%s)。\n"

#: src/http.c:1819
#, c-format
msgid "ERROR: Redirection (%d) without location.\n"
msgstr "错误：重定向 (%d) 但没有指定位置。\n"

#: src/http.c:1851
#, c-format
msgid "%s ERROR %d: %s.\n"
msgstr "%s 错误 %d：%s。\n"

#: src/http.c:1864
msgid "Last-modified header missing -- time-stamps turned off.\n"
msgstr "缺少“Last-modified”文件头-- 关闭时间戳标记。\n"

#: src/http.c:1872
msgid "Last-modified header invalid -- time-stamp ignored.\n"
msgstr "无效的“Last-modified”文件头 -- 忽略时间戳标记。\n"

#: src/http.c:1895
#, c-format
msgid ""
"Server file no newer than local file `%s' -- not retrieving.\n"
"\n"
msgstr ""
"远程文件不比本地文件‘%s’新 -- 不取回。\n"
"\n"

#: src/http.c:1903
#, c-format
msgid "The sizes do not match (local %ld) -- retrieving.\n"
msgstr "文件大小不符 (本地文件 %ld) -- 取回。\n"

#: src/http.c:1907
msgid "Remote file is newer, retrieving.\n"
msgstr "远程文件较新，取回。\n"

#: src/http.c:1948
#, c-format
msgid ""
"%s (%s) - `%s' saved [%ld/%ld]\n"
"\n"
msgstr ""
"%s (%s)-- 已保存“%s”[%ld/%ld])\n"
"\n"

#: src/http.c:1998
#, c-format
msgid "%s (%s) - Connection closed at byte %ld. "
msgstr "%s (%s) - 连接在 %ld 字节时被关闭。"

#: src/http.c:2007
#, c-format
msgid ""
"%s (%s) - `%s' saved [%ld/%ld])\n"
"\n"
msgstr ""
"%s (%s) -- 已保存“%s”[%ld/%ld])\n"
"\n"

#: src/http.c:2028
#, c-format
msgid "%s (%s) - Connection closed at byte %ld/%ld. "
msgstr "%s (%s) - 连接在 %ld/%ld 字节时被关闭。"

#: src/http.c:2040
#, c-format
msgid "%s (%s) - Read error at byte %ld (%s)."
msgstr "%s (%s) - 于 %ld 字节处发生读取错误 (%s)。"

#: src/http.c:2049
#, c-format
msgid "%s (%s) - Read error at byte %ld/%ld (%s). "
msgstr "%s (%s) - 于 %ld/%ld 字节处发生读取错误 (%s)。"

#: src/init.c:342
#, c-format
msgid "%s: WGETRC points to %s, which doesn't exist.\n"
msgstr "%s: WGETRC指向 %s，但它并不存在。\n"

#: src/init.c:398 src/netrc.c:276
#, c-format
msgid "%s: Cannot read %s (%s).\n"
msgstr "%s：无法读取 %s (%s)。\n"

#: src/init.c:416 src/init.c:422
#, c-format
msgid "%s: Error in %s at line %d.\n"
msgstr "%1$s：错误发生于第 %3$d 行的 %2$s。\n"

#: src/init.c:454
#, c-format
msgid "%s: Warning: Both system and user wgetrc point to `%s'.\n"
msgstr "%s：警告：系统与用户的 wgetrc 都指向“%s”。\n"

#: src/init.c:594
#, c-format
msgid "%s: Invalid --execute command `%s'\n"
msgstr "%s：无效 -- 执行命令“%s”\n"

#: src/init.c:630
#, c-format
msgid "%s: %s: Invalid boolean `%s', use `on' or `off'.\n"
msgstr "%s：%s：无效的布尔值“%s”，请使用 on 或 off。\n"

#: src/init.c:673
#, c-format
msgid "%s: %s: Invalid boolean `%s', use always, on, off, or never.\n"
msgstr "%s：%s：无效的布尔值“%s”，请使用 always、on、off 或 never。\n"

#: src/init.c:691
#, c-format
msgid "%s: %s: Invalid number `%s'.\n"
msgstr "%s：%s：无效数字“%s”。\n"

#: src/init.c:930 src/init.c:949
#, c-format
msgid "%s: %s: Invalid byte value `%s'\n"
msgstr "%s：%s：无效的字节数值“%s”。\n"

#: src/init.c:974
#, c-format
msgid "%s: %s: Invalid time period `%s'\n"
msgstr "%s：%s：无效的时间周期“%s”。\n"

#: src/init.c:1051
#, c-format
msgid "%s: %s: Invalid header `%s'.\n"
msgstr "%s：%s：无效的文件头“%s”。\n"

#: src/init.c:1106
#, c-format
msgid "%s: %s: Invalid progress type `%s'.\n"
msgstr "%s：%s：无效的进度指示方式“%s”。\n"

#: src/init.c:1157
#, c-format
msgid "%s: %s: Invalid restriction `%s', use `unix' or `windows'.\n"
msgstr "%s：%s：无效的限定项“%s”，请使用 unix 或者 windows。\n"

#: src/init.c:1198
#, c-format
msgid "%s: %s: Invalid value `%s'.\n"
msgstr "%s：%s：无效的值“%s”。\n"

#: src/log.c:636
#, c-format
msgid ""
"\n"
"%s received, redirecting output to `%s'.\n"
msgstr ""
"\n"
"接收 %s 完毕，正在把输出重定向至“%s”。\n"

#. Eek!  Opening the alternate log file has failed.  Nothing we
#. can do but disable printing completely.
#: src/log.c:643
#, c-format
msgid "%s: %s; disabling logging.\n"
msgstr "%s：%s；禁用日志记录。\n"

#: src/main.c:127
#, c-format
msgid "Usage: %s [OPTION]... [URL]...\n"
msgstr "用法： %s [选项]... [URL]...\n"

#: src/main.c:135
#, c-format
msgid "GNU Wget %s, a non-interactive network retriever.\n"
msgstr "GNU Wget %s，非交互式的网络文件下载工具。\n"

#. Had to split this in parts, so the #@@#%# Ultrix compiler and cpp
#. don't bitch.  Also, it makes translation much easier.
#: src/main.c:140
msgid ""
"\n"
"Mandatory arguments to long options are mandatory for short options too.\n"
"\n"
msgstr ""
"\n"
"长选项必须用的参数在使用短选项时也是必须的。\n"
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
"启动：\n"
"  -V,  --version           显示 Wget 的版本并且退出。\n"
"  -h,  --help              打印此帮助。\n"
"  -b,  -background         启动后进入后台操作。\n"
"  -e,  -execute=COMMAND    运行‘.wgetrc’形式的命令。\n"
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
"日志记录及输入文件：\n"
"  -o,  --output-file=文件     将日志消息写入到指定文件中。\n"
"  -a,  --append-output=文件   将日志消息追加到指定文件的末端。\n"
"  -d,  --debug                打印调试输出。\n"
"  -q,  --quiet                安静模式(不输出信息)。\n"
"  -v,  --verbose               详细输出模式(默认)。\n"
"  -nv, --non-verbose          关闭详细输出模式，但不进入安静模式。\n"
"  -i,  --input-file=文件      下载从指定文件中找到的 URL。\n"
"  -F,  --force-html           以 HTML 方式处理输入文件。\n"
"  -B,  --base=URL             使用 -F -i 文件选项时，在相对链接前添加指定的 URL。\n"
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
"下载：\n"
"  -t,  --tries=次数             配置重试次数（0 表示无限）。\n"
"       --retry-connrefused      即使拒绝连接也重试。\n"
"  -O   --output-document=文件   将数据写入此文件中。\n"
"  -nc, --no-clobber             不更改已经存在的文件，也不使用在文件名后\n"
"                                添加 .#（# 为数字）的方法写入新的文件。\n"
"  -c,  --continue               继续接收已下载了一部分的文件。\n"
"       --progress=方式          选择下载进度的表示方式。\n"
"  -N,  --timestamping           除非远程文件较新，否则不再取回。\n"
"  -S,  --server-response        显示服务器回应消息。\n"
"       --spider                 不下载任何数据。\n"
"  -T,  --timeout=秒数           配置读取数据的超时时间 (秒数)。\n"
"  -w,  --wait=秒数              接收不同文件之间等待的秒数。\n"
"       --waitretry=秒数         在每次重试之间稍等一段时间 (由 1 秒至指定的 秒数不等)。\n"
"       --random-wait            接收不同文件之间稍等一段时间(由 0 秒至  2*WAIT 秒不等)。\n"
"  -Y,  --proxy=on/off           打开或关闭代理服务器。\n"
"  -Q,  --quota=大小             配置接收数据的限额大小。\n"
"       --bind-address=地址      使用本机的指定地址 (主机名称或 IP) 进行连接。\n"
"       --limit-rate=速率        限制下载的速率。\n"
"       --dns-cache=off          禁止查找存于高速缓存中的 DNS。\n"
"       --restrict-file-names=OS 限制文件名中的字符为指定的 OS (操作系统) 所允许的字符。\n"
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
"目录：\n"
"  -nd  --no-directories            不创建目录。\n"
"  -x,  --force-directories         强制创建目录。\n"
"  -nH, --no-host-directories       不创建含有远程主机名称的目录。\n"
"  -P,  --directory-prefix=名称     保存文件前先创建指定名称的目录。\n"
"       --cut-dirs=数目             忽略远程目录中指定数目的目录层。\n"
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
"HTTP 选项：\n"
"       --http-user=用户    配置 http 用户名。\n"
"       --http-passwd=密码    配置 http 用户密码。\n"
"  -C,  --cache=on/off        (不)使用服务器中的高速缓存中的数据 (默认是使用的)。\n"
"  -E,  --html-extension      将所有 MIME 类型为 text/html 的文件都加上 .html 扩展文件名。\n"
"       --ignore-length       忽略“Content-Length”文件头字段。\n"
"       --header=字符串         在文件头中添加指定字符串。\n"
"       --proxy-user=用户   配置代理服务器用户名。\n"
"       --proxy-passwd=密码   配置代理服务器用户密码。\n"
"       --referer=URL         在 HTTP 请求中包含“Referer：URL”头。\n"
"  -s,  --save-headers        将 HTTP 头存入文件。\n"
"  -U,  --user-agent=AGENT    标志为 AGENT 而不是 Wget/VERSION。\n"
"       --no-http-keep-alive  禁用 HTTP keep-alive（持久性连接）。\n"
"       --cookies=off         禁用 cookie。\n"
"       --load-cookies=文件   会话开始前由指定文件载入 cookie。\n"
"       --save-cookies=文件   会话结束后将 cookie 保存至指定文件。\n"
"       --post-data=字符串    使用 POST 方法，发送指定字符串。\n"
"       --post-file=文件      使用 POST 方法，发送指定文件中的内容。\n"
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
"HTTPS (SSL) 选项：\n"
"       --sslcertfile=文件     可选的客户段端证书。\n"
"       --sslcertkey=密钥文件  对此证书可选的“密钥文件”。\n"
"       --egd-file=文件        EGD socket 文件名。\n"
"       --sslcadir=目录         CA 散列表所在的目录。\n"
"       --sslcafile=文件      包含 CA 的文件。\n"
"       --sslcerttype=0/1      Client-Cert 类型 0=PEM (默认) / 1=ASN1 (DER)\n"
"       --sslcheckcert=0/1     根据提供的 CA 检查服务器的证书\n"
"       --sslprotocol=0-3      选择 SSL 协议；0=自动选择，\n"
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
"FTP 选项：\n"
"  -nr, --dont-remove-listing   不删除“.listing”文件。\n"
"  -g,  --glob=on/off           设置是否展开有通配符的文件名。\n"
"       --passive-ftp           使用“被动”传输模式。\n"
"       --retr-symlinks         在递归模式中，下载链接所指示的文件(连至目录\n"
"                               则例外）。\n"

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
"递归下载：\n"
"  -r,  --recursive          递归下载。\n"
"  -l,  --level=数字         最大递归深度(inf 或 0 表示无限)。\n"
"       --delete-after       删除下载后的文件。\n"
"  -k,  --convert-links      将绝对链接转换为相对链接。\n"
"  -K,  --backup-converted   转换文件 X 前先将其备份为 X.orig。\n"
"  -m,  --mirror             等效于 -r -N -l inf -nr 的选项。\n"
"  -p,  --page-requisites    下载所有显示完整网页所需的文件，例如图像。\n"
"       --strict-comments    打开对 HTML 备注的严格(SGML)处理选项。\n"
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
"递归下载时有关接受/拒绝的选项：\n"
"  -A,  --accept=列表                接受的文件样式列表，以逗号分隔。\n"
"  -R,  --reject=列表                排除的文件样式列表，以逗号分隔。\n"
"  -D,  --domains=列表               接受的域列表，以逗号分隔。\n"
"       --exclude-domains=列表       排除的域列表，以逗号分隔。\n"
"       --follow-ftp                 跟随 HTML 文件中的 FTP 链接。\n"
"       --follow-tags=列表           要跟随的 HTML 标记，以逗号分隔。\n"
"  -G,  --ignore-tags=列表           要忽略的 HTML 标记，以逗号分隔。\n"
"  -H,  --span-hosts                 递归时可进入其它主机。\n"
"  -L,  --relative                   只跟随相对链接。\n"
"  -I,  --include-directories=列表   要下载的目录列表。\n"
"  -X,  --exclude-directories=列表   要排除的目录列表。\n"
"  -np, --no-parent                  不搜索上层目录。\n"
"\n"

#: src/main.c:263
msgid "Mail bug reports and suggestions to <bug-wget@gnu.org>.\n"
msgstr "请将错误报告或建议寄给 <bug-wget@gnu.org>。\n"

#: src/main.c:465
#, c-format
msgid "%s: debug support not compiled in.\n"
msgstr "%s：未将调试支持编译到程序中。\n"

#: src/main.c:517
msgid "Copyright (C) 2003 Free Software Foundation, Inc.\n"
msgstr "版权所有 (C) 2003 Free Software Foundation, Inc.\n"

#: src/main.c:519
msgid ""
"This program is distributed in the hope that it will be useful,\n"
"but WITHOUT ANY WARRANTY; without even the implied warranty of\n"
"MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the\n"
"GNU General Public License for more details.\n"
msgstr ""
"此程序发布的目的是希望它会有用，但它不作任何保证；\n"
"甚至没有可售性或适用于特定目的的保证。\n"
"详情请查看 GNU General Public License。\n"

#: src/main.c:524
msgid ""
"\n"
"Originally written by Hrvoje Niksic <hniksic@xemacs.org>.\n"
msgstr ""
"\n"
"最初由 Hrvoje Niksic <hniksic@xemacs.org> 编写。\n"

#: src/main.c:703
#, c-format
msgid "%s: illegal option -- `-n%c'\n"
msgstr "%s：非法的选项 -- “-n%c”\n"

#. #### Something nicer should be printed here -- similar to the
#. pre-1.5 `--help' page.
#: src/main.c:706 src/main.c:748 src/main.c:794
#, c-format
msgid "Try `%s --help' for more options.\n"
msgstr "请尝试使用“%s --help”查看更多的选项。\n"

#: src/main.c:774
msgid "Can't be verbose and quiet at the same time.\n"
msgstr "无法同时使用详细输出模式和安静模式。\n"

#: src/main.c:780
msgid "Can't timestamp and not clobber old files at the same time.\n"
msgstr "无法修改时间戳标记而不更改本地文件。\n"

#. No URL specified.
#: src/main.c:789
#, c-format
msgid "%s: missing URL\n"
msgstr "%s：未指定 URL\n"

#: src/main.c:905
#, c-format
msgid "No URLs found in %s.\n"
msgstr "在 %s 中找不到 URL。\n"

#: src/main.c:914
#, c-format
msgid ""
"\n"
"FINISHED --%s--\n"
"Downloaded: %s bytes in %d files\n"
msgstr ""
"\n"
"下载完毕 --%s--\n"
"下载了：%s 字节，共 %d 个文件\n"

#: src/main.c:920
#, c-format
msgid "Download quota (%s bytes) EXCEEDED!\n"
msgstr "超过下载限额(%s 字节)！\n"

#: src/mswindows.c:147
msgid "Continuing in background.\n"
msgstr "继续在后台运行。\n"

#: src/mswindows.c:149 src/utils.c:487
#, c-format
msgid "Output will be written to `%s'.\n"
msgstr "将把输出写入至“%s”。\n"

#: src/mswindows.c:245
#, c-format
msgid "Starting WinHelp %s\n"
msgstr "正在启动 WinHelp %s\n"

#: src/mswindows.c:272 src/mswindows.c:279
#, c-format
msgid "%s: Couldn't find usable socket driver.\n"
msgstr "%s：找不到可用的 socket 驱动程序。\n"

#: src/netrc.c:380
#, c-format
msgid "%s: %s:%d: warning: \"%s\" token appears before any machine name\n"
msgstr "%s：%s:%d：警告：“%s”标记出现在机器名称前\n"

#: src/netrc.c:411
#, c-format
msgid "%s: %s:%d: unknown token \"%s\"\n"
msgstr "%s：%s:%d：未知的标记“%s”\n"

#: src/netrc.c:475
#, c-format
msgid "Usage: %s NETRC [HOSTNAME]\n"
msgstr "用法：%s NETRC [主机名]\n"

#: src/netrc.c:485
#, c-format
msgid "%s: cannot stat %s: %s\n"
msgstr "%s：无法 stat %s：%s\n"

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
"%*s[ 正跳过 %dK ]"

#: src/progress.c:401
#, c-format
msgid "Invalid dot style specification `%s'; leaving unchanged.\n"
msgstr "无效的进度指示方式“%s”；不会改变原来的方式。\n"

#: src/recur.c:378
#, c-format
msgid "Removing %s since it should be rejected.\n"
msgstr "正在删除 %s 因为它应该被指定了拒绝下载。\n"

#: src/res.c:549
msgid "Loading robots.txt; please ignore errors.\n"
msgstr "正在载入 robots.txt；请忽略错误消息。\n"

#: src/retr.c:400
#, c-format
msgid "Error parsing proxy URL %s: %s.\n"
msgstr "解析代理服务器 URL %s 时发生错误：%s。\n"

#: src/retr.c:408
#, c-format
msgid "Error in proxy URL %s: Must be HTTP.\n"
msgstr "代理服务器 URL %s 错误：必须是 HTTP。\n"

#: src/retr.c:493
#, c-format
msgid "%d redirections exceeded.\n"
msgstr "已超过 %d 次重定向。\n"

#: src/retr.c:617
msgid ""
"Giving up.\n"
"\n"
msgstr ""
"已放弃。\n"
"\n"

#: src/retr.c:617
msgid ""
"Retrying.\n"
"\n"
msgstr ""
"重试中。\n"
"\n"

#: src/url.c:621
msgid "No error"
msgstr "没有错误"

#: src/url.c:623
msgid "Unsupported scheme"
msgstr "不支持的主题"

#: src/url.c:625
msgid "Empty host"
msgstr "未指定主机"

#: src/url.c:627
msgid "Bad port number"
msgstr "端口号错误"

#: src/url.c:629
msgid "Invalid user name"
msgstr "无效的用户名"

#: src/url.c:631
msgid "Unterminated IPv6 numeric address"
msgstr "未结束的 IPv6 数字地址"

#: src/url.c:633
msgid "IPv6 addresses not supported"
msgstr "不支持 IPv6 地址"

#: src/url.c:635
msgid "Invalid IPv6 numeric address"
msgstr "无效的 IPv6 数字地址"

#: src/utils.c:120
#, c-format
msgid "%s: %s: Not enough memory.\n"
msgstr "%s：%s：内存不足。\n"

#. parent, no error
#: src/utils.c:485
#, c-format
msgid "Continuing in background, pid %d.\n"
msgstr "继续在后台运行，pid 为 %d。\n"

#: src/utils.c:529
#, c-format
msgid "Failed to unlink symlink `%s': %s\n"
msgstr "无法删除符号链接 '%s'：%s\n"

#~ msgid "Syntax error in Set-Cookie at character `%c'.\n"
#~ msgstr "在 Set-Cookie 中字符“%c”处出现语法错误。\n"

#~ msgid "%s: BUG: unknown command `%s', value `%s'.\n"
#~ msgstr "%s：BUG：未知的命令“%s”，值“%s”。\n"

#~ msgid "%s: %s: Cannot convert `%s' to an IP address.\n"
#~ msgstr "%s：%s：无法将“%s”转换为一个 IP 地址。\n"

#~ msgid "%s: %s: invalid command\n"
#~ msgstr "%s：%s：无效的命令\n"

#~ msgid "Could not find proxy host.\n"
#~ msgstr "找不到代理服务器主机。\n"

#~ msgid "%s: Redirection cycle detected.\n"
#~ msgstr "%s：重定向到自己。\n"
