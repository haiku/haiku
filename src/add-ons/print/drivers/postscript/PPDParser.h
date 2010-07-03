#ifndef PPDPARSER_H
#define PPDPARSER_H


#include <String.h>


class BPath;
class BFile;
class BDirectory;


class PPDParser
{
public:
						PPDParser(const BDirectory& dir, const char* fname);
						PPDParser(const BPath& path);
						PPDParser(BFile& file);
						~PPDParser();

			status_t	InitCheck() const
						{
							return fInitErr;
						}

			BString		GetParameter(const BString& param);

private:
			status_t	InitData(BFile& file);

			BString		fContent;
			status_t	fInitErr;
};

#endif // PPDPARSER_H
