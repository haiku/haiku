#!/bin/sh

cd "$(dirname "$0")"

if pwd | grep " ";
  then alert "ディレクトリ名に空白を含めないでください。
インストールが正常にできません。
/boot/home/mydirectoryのように
空白を含まないディレクトリに
内容を移動してください。";
  exit 1;
fi

RETURN=`alert "Mail Daemon Replacementインストール

BeOSのメールデーモンは
システムに一つしか存在できません。
（理由はREADMEをお読みください）

「Purge」：
　保存しません
「Backup」：
　MDRを初めてインストールする時に、
　古いメールデーモン
　(mail_daemon/libmail.so/BeMail/E-mail)を
　保存します。
　（2回目以降は選択しないでください）
「Don't do anything!」：
　インストールを中止します。" "Purge" "Backup" "Abort - Don't do anything!"`

if [[ $RETURN = Purge ]]
then
	# note: we don't remove libmail.so, because it doesn't matter here, and there may be symlinks and things
	query -a 'BEOS:APP_SIG == "application/x-vnd.Be-POST" || BEOS:APP_SIG == "application/x-vnd.Be-mprf" || BEOS:APP_SIG == "application/x-vnd.Be-MAIL" || BEOS:APP_SIG == "application/x-vnd.agmsmith.AGMSBayesianSpamServer"' | grep -v "`/bin/pwd`" | xargs rm -f
elif [[ $RETURN = Backup ]]
then
	query -a 'BEOS:APP_SIG == "application/x-vnd.Be-POST" || BEOS:APP_SIG == "application/x-vnd.Be-mprf" || BEOS:APP_SIG == "application/x-vnd.Be-MAIL" || BEOS:APP_SIG == "application/x-vnd.agmsmith.AGMSBayesianSpamServer" || name == libmail.so' | grep -v "`/bin/pwd`" | xargs zip -ym /boot/home/maildaemon.zip
else
	alert "Mail Daemon Replacementインストール
	
　バックアップを作成しませんでした。
　現在のバージョンが唯一のメールデーモンとなります。"
	exit -1
fi

# This should be translated
if [[ `uname -m` == BePC ]] && test ! -e ~/config/lib/libssl.so; then

RETURN=`alert "You don't seem to have OpenSSL installed, which is required for IMAP." "Get OpenSSL" "I Don't Care"`

if [[ $RETURN = "Get OpenSSL" ]]
then
	NetPositive http://www.bebits.com/app/1020 &
fi

fi

quit "application/x-vnd.Be-POST"
quit "application/x-vnd.Be-TSKB"
quit "application/x-vnd.agmsmith.AGMSBayesianSpamServer"

rm -rf ~/config/add-ons/mail_daemon/*
rm -f /system/servers/mail_daemon /boot/beos/preferences/E-mail /boot/beos/apps/BeMail
rm -f ~/config/lib/libmail2.so ~/config/lib/libnumail2.so /system/lib/libmail.so
rm -f "${HOME}/config/settings/Mail/New E-mail"
rm -f "${HOME}/config/settings/Mail/Menu Links/T) 本日のメール"

mkdir -p ~/config/add-ons/mail_daemon

copyattr -d -m bin/libmail.so /boot/beos/system/lib/libmail.so
copyattr -d -m bin/libcrypto.so /boot/home/config/lib/libcrypto.so
copyattr -d -m bin/libssl.so /boot/home/config/lib/libssl.so
copyattr -d -m -r bin/addons/* ~/config/add-ons/mail_daemon
copyattr -d -m bin/mail_daemon /system/servers/mail_daemon
copyattr -d -m bin/E-mail /boot/beos/preferences/E-mail
copyattr -d -m bin/BeMail /boot/beos/apps/BeMail

# Set up the files in the Menu Links directory.  They will appear in the pop-up
# menu in the deskbar.  Overwrite existing ones, and remove ones in the wrong
# language (left over from previous installs).  But don't delete any other ones
# that the user may have custom made.
mkdir -p ~/config/settings/Mail/Menu\ Links
unzip -o bin/ExtraMenuLinksForR5Tracker.zip -d ~/config/settings/Mail/Menu\ Links
rm bin/ExtraMenuLinksForR5Tracker.zip
mv -f -v "${HOME}/config/settings/Mail/Menu Links/E-Mail From…" "${HOME}/config/settings/Mail/Menu Links/F) 送信者を検索…"
mv -f -v "${HOME}/config/settings/Mail/Menu Links/Open Draft" "${HOME}/config/settings/Mail/Menu Links/D) ドラフトを開く…"
mv -f -v "${HOME}/config/settings/Mail/Menu Links/Open Inbox Folder" "${HOME}/config/settings/Mail/Menu Links/I) 受信箱を開く"
mv -f -v "${HOME}/config/settings/Mail/Menu Links/Open Mail Folder" "${HOME}/config/settings/Mail/Menu Links/M) 郵便箱を開く"
mv -f -v "${HOME}/config/settings/Mail/Menu Links/Subject Contains…" "${HOME}/config/settings/Mail/Menu Links/S) 見出しで検索…"
mv -f -v "${HOME}/config/settings/Mail/Menu Links/Received Today" "${HOME}/config/settings/Mail/Menu Links/R) 本日受信したメール"
mv -f -v "${HOME}/config/settings/Mail/Menu Links/Sent Today" "${HOME}/config/settings/Mail/Menu Links/T) 本日送信したメール"

# Set up the spam classifier server.
copyattr -d -m bin/AGMSBayesianSpamServer ~/config/bin/AGMSBayesianSpamServer
mkdir -p ~/config/settings/AGMSBayesianSpam
if test ! -e "${HOME}/config/settings/AGMSBayesianSpam/AGMSBayesianSpam Database"; then
    cp bin/SampleSpamDatabase "${HOME}/config/settings/AGMSBayesianSpam/AGMSBayesianSpam Database";
fi
copyattr -d -m bin/SoundGenuine "${HOME}/config/settings/AGMSBayesianSpam/SoundGenuine"
copyattr -d -m bin/SoundSpam "${HOME}/config/settings/AGMSBayesianSpam/SoundSpam"
copyattr -d -m bin/SoundUncertain "${HOME}/config/settings/AGMSBayesianSpam/SoundUncertain"

# Patch up things that need Japanese names.  Do it here before the daemon is
# started.  A link (rather than renaming) is done so that saved chain settings,
# which use the file names of the add-ons, will still work if they were made in
# English mode.  Also done in the English version, in case someone was using
# Japanese and switched back to English.
ln -f -s -v "${HOME}/config/add-ons/mail_daemon/system_filters/Inbox" "${HOME}/config/add-ons/mail_daemon/system_filters/受信箱"
ln -f -s -v "${HOME}/config/add-ons/mail_daemon/system_filters/Outbox" "${HOME}/config/add-ons/mail_daemon/system_filters/送信箱"
ln -f -s -v "${HOME}/config/add-ons/mail_daemon/system_filters/New Mail Notification" "${HOME}/config/add-ons/mail_daemon/system_filters/着信通知方法"

# Various fixups...

# The word index files sometimes go corrupt.  Since they are regenerated if
# they aren't there, this is fixed by just deleting the whole annoying
# directory.
rm -r /boot/beos/etc/word_index/

# The old installer used to delete the developer's link to the libmail.so
# library, so put it back.  Do for x86 and ppc, will do nothing if you don't
# have that development system installed (ln will fail harmlessly).  Won't work
# if you have both PPC and x86 development systems.
if test -e "/boot/develop/lib/x86/libmail.so";	then
	echo "/boot/develop/lib/x86/libmail.so already exists, no need to fix.";
else
	ln -s /boot/beos/system/lib/libmail.so /boot/develop/lib/x86/libmail.so
fi
if test -e "/boot/develop/lib/ppc/libmail.so";	then
	echo "/boot/develop/lib/ppc/libmail.so already exists, no need to fix.";
else
	ln -s /boot/beos/system/lib/libmail.so /boot/develop/lib/ppc/libmail.so
fi

# Force the MIME database to reload the icons and other application info, since
# it doesn't do it automatically and we sometimes add new icons.
setmime -remove application/x-vnd.Be-POST; mimeset /system/servers/mail_daemon
setmime -remove application/x-vnd.Be-MAIL; mimeset /boot/beos/apps/BeMail
mimeset ~/config/bin/AGMSBayesianSpamServer

setmime -set text/x-email -preferredApp /boot/beos/apps/BeMail -preferredAppSig application/x-vnd.Be-MAIL
setmime -set text/x-vnd.be-maildraft -preferredApp /boot/beos/apps/BeMail -preferredAppSig application/x-vnd.Be-MAIL
setmime -set text/x-partial-email -preferredApp /boot/beos/system/servers/mail_daemon -preferredAppSig application/x-vnd.Be-POST

sleep 1
/system/Deskbar &
sleep 1
/system/servers/mail_daemon -M &
sleep 2

# Create the MIME types and indices needed by the spam server.  Note that it
# modifies the e-mail type by adding attributes to it, so we have to wait until
# the daemon has started and created the text/x-email MIME type entry.
~/config/bin/AGMSBayesianSpamServer InstallThings

sync
alert "Mail Daemon Replacementインストール

　インストールが完了しました。
　再インストールする際は再度
　パッケージの解凍から行ってください。"

# Launch prefs if this is a new install of MDR
if test !  -e "/boot/home/config/settings/Mail/chains";	then
	/boot/beos/preferences/E-mail &
fi

# Prevent a second attempt to install since all our source files have been moved / deleted.
echo "This one use install script will now self destruct..."
rm install.japanese.sh
