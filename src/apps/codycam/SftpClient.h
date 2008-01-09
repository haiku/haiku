#ifndef SFTP_CLIENT_H
#define SFTP_CLIENT_H


#include <stdio.h>
#include <string>

#include <File.h>
#include "SpawningUploadClient.h"

using std::string;


class SftpClient : public SpawningUploadClient {
	public:
		SftpClient();
		~SftpClient();

virtual bool	Connect(const string& server, const string& login,
			const string& passwd);

		bool PutFile(const string& local, const string& remote,
			ftp_mode mode = binary_mode);

		bool GetFile(const string& remote, const string& local,
			ftp_mode mode = binary_mode);

		bool MoveFile(const string& oldPath, const string& newPath);
		bool ChangeDir(const string& dir);
		bool PrintWorkingDir(string& dir);
		bool ListDirContents(string& listing);
		bool Chmod(const string& path, const string& mod);

		void SetPassive(bool on);
	
	protected:

};

#endif	// SFTP_CLIENT_H
