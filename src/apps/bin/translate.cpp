// translate - for OpenBeOS
//
// authors - in order of contribution
// jonas.sundstrom@kirilla.com
// 
// bugs & issues
// -- type code doesn't work, needs MIME format in argv[3]
// 
// libs: be, root, translation
// 

#include <stdio.h>
#include <string.h>

#include <TranslationKit.h>
#include <Application.h>
#include <String.h>
#include <File.h>
#include <Mime.h>
	
class TranslateApp : public BApplication
{
	public:
						TranslateApp	(void);

		virtual void	ReadyToRun		(void);
		virtual void	ArgvReceived	(int32 argc, char **argv);

	private:
		
		void			PrintUsage					(void);
		void			ListTranslators				(void);
		uint32			GetTypecodeForOutputMime	(const char * a_mime);
		void			RemoveOutputFile			(char * path);

		bool					m_got_arguments;
		BTranslatorRoster	*	translator_roster;
		int32					translators_total;
		translator_id	*		translator_array; 
};

int main()
{
	new TranslateApp ();
	be_app->Run();	

	return B_OK;
}

TranslateApp::TranslateApp (void)	:	
	BApplication 		("application/x-vnd.obos-translate"),
	m_got_arguments		(false),
	translator_roster	(BTranslatorRoster::Default()),
	translators_total	(0),
	translator_array	(NULL)
{
	translator_roster->GetAllTranslators (& translator_array, & translators_total); 
}

void TranslateApp::ArgvReceived (int32 argc, char ** argv)
{
	status_t	status	=	B_OK;

	BString first_arg	=	argv[1];
	first_arg.ToLower();

	// --help
	if (first_arg.IFindFirst("--help") == 0)
	{
		m_got_arguments	=	true;
		PrintUsage();
		return;
	}
	
	// --list
	if (first_arg.IFindFirst("--list") == 0)
	{
		m_got_arguments	=	true;
		ListTranslators();
		return;
	}

	if (argc != 4)
		return;

	m_got_arguments	=	true;

	// input file
	BFile		in_file		(argv[1], B_READ_ONLY);
	status	=	in_file.InitCheck();

	if (status != B_OK)
	{
		printf("translate: %s : %s\n", argv[1], strerror(status));
		exit (-1);
	}	

	// find input translator
	translator_info		best_suited_translator;
	status	=	translator_roster->Identify(& in_file, NULL, & best_suited_translator);

	// get typecode of output format
	uint32		out_format	=	0;
	BMimeType	mime 	(argv[3]);

	if (mime.IsValid() && (! mime.IsSupertypeOnly()))			// MIME-string
	{
		out_format	=	GetTypecodeForOutputMime(argv[3]);
	}
	else														// 4-byte code
	{
		if (strlen(argv[3]) == 4)
			out_format	=	(uint32) argv[3];
		else
			out_format = 0;
	}
	
	if (out_format == 0)
	{
		printf("bad format: %s\nformat is 4-byte type code or full MIME type\n", argv[3]);
		exit (-1);
	}
	
	// output file
	BFile	out_file	(argv[2], B_READ_WRITE | B_CREATE_FILE | B_FAIL_IF_EXISTS);
	status	=	out_file.InitCheck();

	if (status != B_OK)
	{
		printf("translate: %s : %s\n", argv[2], strerror(status));
		exit (-1);
	}		

	// translate
	BMallocIO	*	malloc_io	=	new BMallocIO;

	status = translator_roster->Translate(& in_file, NULL, NULL, malloc_io, best_suited_translator.group);
	
	if (status != B_OK)
	{
		printf("translate: translation of %s to base format: %s\n", argv[1], strerror(status));
		delete malloc_io;
		RemoveOutputFile(argv[2]);
		exit (-1);
	}
	
	in_file.Unset();
	
	status = translator_roster->Translate(malloc_io, NULL, NULL, & out_file, out_format);
	
	if (status != B_OK)
	{
		printf("translate: translation from base format to %s: %s\n", argv[3], strerror(status));
		delete malloc_io;
		RemoveOutputFile(argv[2]);
		exit (-1);
	}
	
	delete malloc_io;
	out_file.Unset();
	
	// add filetype attribute
	update_mime_info(argv[2], 0, 1, 0);
}

void TranslateApp::ReadyToRun (void)
{
	if (m_got_arguments == false)
		PrintUsage();
	
	PostMessage(B_QUIT_REQUESTED);
}

void TranslateApp::PrintUsage (void)
{
	printf ("usage: translate { --list | input output format }\n");
}

void TranslateApp::ListTranslators (void)
{
	const char *			translator_name		=	NULL;
	const char *			translator_info		=	NULL; 
	int32					translator_version	=	0;
	
	for (int32 i = 0;  i < translators_total;  i++) 
	{ 
		translator_roster->GetTranslatorInfo(translator_array[i], & translator_name, & translator_info, & translator_version); 
		printf("name: %s\ninfo: %s\nversion: %.2f\n", translator_name, translator_info, translator_version/100.); 

		const translation_format *	format		=	NULL;
		int32 						format_num	=	0;
		
		translator_roster->GetInputFormats(translator_array[i], & format, & format_num);

		for (int32 ii = 0;  ii < format_num;  ii++)
		{
			printf("input:\t");
		
			printf("'%c", (short) (format[ii].type >> 24));
			printf("%c", (short) ((format[ii].type << 8) >> 24));
			printf("%c", (short) ((format[ii].type << 16) >> 24));
			printf("%c' ", (short) ((format[ii].type << 24) >> 24));

			printf("(%c", (short) (format[ii].group >> 24));
			printf("%c", (short) ((format[ii].group << 8) >> 24));
			printf("%c", (short) ((format[ii].group << 16) >> 24));
			printf("%c) ", (short) ((format[ii].group << 24) >> 24));
		
			printf("%.1f ", format[ii].quality);
			printf("%.1f ", format[ii].capability);
		
			printf("%s ; ", format[ii].MIME);
			printf("%s\n", format[ii].name);
		}
		
		translator_roster->GetOutputFormats(translator_array[i], & format, & format_num);

		for (int32 ii = 0;  ii < format_num;  ii++)
		{
			printf("output:\t");
		
			printf("'%c", (short) (format[ii].type >> 24));
			printf("%c", (short) ((format[ii].type << 8) >> 24));
			printf("%c", (short) ((format[ii].type << 16) >> 24));
			printf("%c' ", (short) ((format[ii].type << 24) >> 24));

			printf("(%c", (short) (format[ii].group >> 24));
			printf("%c", (short) ((format[ii].group << 8) >> 24));
			printf("%c", (short) ((format[ii].group << 16) >> 24));
			printf("%c) ", (short) ((format[ii].group << 24) >> 24));
		
			printf("%.1f ", format[ii].quality);
			printf("%.1f ", format[ii].capability);
		
			printf("%s ; ", format[ii].MIME);
			printf("%s\n", format[ii].name);
		}
		
		printf("\n");
	}
}

uint32 TranslateApp::GetTypecodeForOutputMime (const char * a_mime)
{
	for (int32 i = 0;  i < translators_total;  i++)
	{ 
		const translation_format *	format				=	NULL;
		int32 						format_num			=	0;
		
		translator_roster->GetOutputFormats(translator_array[i], & format, & format_num);

		for (int32 ii = 0;  ii < format_num;  ii++)
		{
			if (! strcmp(a_mime, format[ii].MIME))
				return (format[ii].type);
		}
	}
	
	return 0;
}

void TranslateApp::RemoveOutputFile (char * path)
{
	BEntry	entry	(path);
	entry.Remove();
}
