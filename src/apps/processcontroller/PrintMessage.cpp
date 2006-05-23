/*
	PrintMessage.cpp
	
	ProcessController
	(c) 2000, Georges-Edouard Berenger, All Rights Reserved.
	Copyright (C) 2004 beunited.org 

	This library is free software; you can redistribute it and/or 
	modify it under the terms of the GNU Lesser General Public 
	License as published by the Free Software Foundation; either 
	version 2.1 of the License, or (at your option) any later version. 

	This library is distributed in the hope that it will be useful, 
	but WITHOUT ANY WARRANTY; without even the implied warranty of 
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU 
	Lesser General Public License for more details. 

	You should have received a copy of the GNU Lesser General Public 
	License along with this library; if not, write to the Free Software 
	Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA	
	
*/

#include "PrintMessage.h"
#include "ReadableLong.h"

#include <ctype.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <AppFileInfo.h>
#include <Path.h>
#include <SymLink.h>
#include <fs_attr.h>
#include <Application.h>
#include <Resources.h>
#include <PropertyInfo.h>
#include <Roster.h>

int32	gHexalinesPref=9;

void do_print_message(pchar & texte, BMessage* message);
void print_type(pchar & texte, ulong type, void* pvoid, ssize_t size);
void tab_to(pchar & texte, int n);
void print_hex(pchar & texte, void* pvoid, ssize_t size, int extra_space);
void ajoute(pchar & texte, const char* mot, long length=0);
inline void ajoute(pchar & texte, const char c) { *texte++=c; }

const char*	specifiers[] = {
	"B_NO_SPECIFIER",
	"B_DIRECT_SPECIFIER",
	"B_INDEX_SPECIFIER",
	"B_REVERSE_INDEX_SPECIFIER",
	"B_RANGE_SPECIFIER",
	"B_REVERSE_RANGE_SPECIFIER",
	"B_NAME_SPECIFIER",
	"B_ID_SPECIFIER",
	NULL };
	
const int kTabType=22;
const int kTabName=25;
const int kTabNum=12;

const char* indent1=" | ";
const char* indent2=" > ";
const char* indent3=" \" ";
const int	maxprefix=1000;

char	gPrefix[maxprefix];

void print_message(pchar & texte, BMessage* message, bool dig)
{
	gPrefix[0]=0;
	if (dig) {
#if B_BEOS_VERSION > B_BEOS_VERSION_5
		const char*	name;
#else
		char *		name;
#endif
		ulong		type;
		off_t		size;
		long		m;
		long		count, k;
		entry_ref	ref;
		bool		found=false;
		for (k=0; message->GetInfo(B_REF_TYPE, k, &name, &type, &count)==B_OK; k++)
			for (m=0; m<count; m++)
				if (message->FindRef(name, m, &ref)==B_OK) {
					BEntry	entry(&ref, true);
					if (entry.InitCheck()==B_OK && entry.IsFile()) {
						found=true;
						ajoute(texte, "File");
						BPath	path;
						entry.GetPath(&path);
						texte+=sprintf(texte, ": %s\n", path.Path());
						entry.GetSize(&size);
						if (size==0)
							ajoute(texte, " Empty file.\n");
						else {
							BFile		file(&entry, B_READ_ONLY);
							BResources	rsrc;
							if (rsrc.SetTo(&file)==B_OK) {
								ajoute(texte, " Resource file:\n");
								// file has resource file
								int32		index=0;
								type_code	typeFound;
								int32		idFound;
								const char*	nameFound;
								size_t		lengthFound;
								while (rsrc.GetResourceInfo(index++, &typeFound, &idFound, &nameFound, &lengthFound)) {
									void*	resource=rsrc.FindResource(typeFound, idFound, &lengthFound);
									if (resource) {
										texte+=sprintf(texte, " %d", int(idFound));
										tab_to(texte, 5);
										ajoute(texte, long_to_be_string(typeFound));
										tab_to(texte, 1+kTabType);
										texte+=sprintf(texte, "\"%s\"", nameFound);
										tab_to(texte, 1+kTabType+kTabName);
										strcpy(gPrefix, " ");
										print_type(texte, typeFound, resource, lengthFound);
										free(resource);
									} else
										texte+=sprintf(texte, "Could not read resource of type %s with id=%d\n", long_to_be_string(typeFound), int(idFound));
								}
							} else {
								char*	content=new char[size];
								if (file.Read(content, size)==size) {
									BMessage	inmessage;
									strcpy(gPrefix, " > ");
									if (inmessage.Unflatten(content)==B_OK) {
										ajoute(texte, " BMessage file:\n");
										do_print_message(texte, &inmessage);
									} else {
										ajoute(texte, " Not a BMessage, not a resource file.\n");
										print_hex(texte, content, size, 0);
										//print_type(texte, B_RAW_TYPE, content, size);
									}
								} else
									ajoute(texte, " The file's content could not be read.\n");
								delete[] content;
							}
						}
					}
				}
		if (!found)
			ajoute(texte, "(No file referenced in the BMessage)");
	} else
		do_print_message(texte, message);
	*texte=0;
}

void do_print_message(pchar & texte, BMessage* message)
{
#if B_BEOS_VERSION > B_BEOS_VERSION_5
	const char*	name;
#else
	char *		name;
#endif
	ulong		type;
	void*		data;
	ssize_t		size;
	long		m;
	long		count, k;
	int			lPrefix=strlen(gPrefix);

	ajoute(texte, gPrefix);
	ajoute(texte, "What=");
	ajoute(texte, long_to_be_string(message->what));
	ajoute(texte, "\n");
	for (k=0; message->GetInfo(B_ANY_TYPE, k, &name, &type, &count)==B_OK; k++) {
		for (m=0; m<count; m++) {
			if (message->FindData(name, type, m, (const void**) &data, &size)==B_OK) {
				ajoute(texte, gPrefix);
				ajoute(texte, long_to_be_string(type));
				tab_to(texte, lPrefix+kTabType);
				if (count>1)
					texte+=sprintf(texte, "\"%s\" #%d", name, (int) m+1);
				else
					texte+=sprintf(texte, "\"%s\"", name);
				tab_to(texte, lPrefix+kTabType+kTabName);
				if (type==B_REF_TYPE) {
					entry_ref	ref;
					message->FindRef(name, m, &ref);
					print_type(texte, type, &ref, sizeof(ref));
				} else if (type==B_MESSENGER_TYPE) {
					BMessenger messenger;
					message->FindMessenger(name, m, &messenger);
					print_type(texte, type, &messenger, sizeof(messenger));
				} else
					print_type(texte, type, data, size);
			} else
				texte+=sprintf(texte, "%sError retreiving data.\n", gPrefix);
		}
	}
}

void print_type(pchar & texte, ulong type, void* pvoid, ssize_t size)
{
	union Data {
		char		c;
		double		d;
		float		f;
		int8		i8;
		uint8		ui8;
		int16		i16;
		uint16		ui16;
		int32		i32;
		uint32		ui32;
		int64		i64;
		uint64		ui64;
		time_t		time;
		rgb_color	rgb;
		version_info	vinfos;
	};

	Data*		data=(Data*) pvoid;
	char*		txtdata=(char*) pvoid;
	BMessage	local;
	bool		cr=true;
	int			lPrefix=strlen(gPrefix);
	int			max=maxprefix-lPrefix-1;

	long_to_be_string(type);
	switch (type) {
	case B_BOOL_TYPE:
		ajoute(texte, '0'+data->c);
		break;

	case B_CHAR_TYPE:
		ajoute(texte, data->c);
		break;

	case B_DOUBLE_TYPE:
		texte+=sprintf(texte, "%f",  data->d);
		break;

	case B_FLOAT_TYPE:
		texte+=sprintf(texte, "%f", data->f);
		break;

	case B_UINT8_TYPE:
		texte+=sprintf(texte, "%u (0x%.2hX)", data->ui8, data->ui8);
		break;

	case B_INT8_TYPE:
		texte+=sprintf(texte, "%d (0x%.2hX)", data->i8, data->ui8);
		break;

	case B_INT16_TYPE:
		texte+=sprintf(texte, "%hd (0x%.4hX)", data->i16, data->i16);
		break;

	case B_UINT16_TYPE:
		texte+=sprintf(texte, "%hu (0x%.4hX)", data->ui16, data->ui16);
		break;

	case B_INT32_TYPE:
	case B_SSIZE_T_TYPE:
		texte+=sprintf(texte, "%ld (%s", data->i32, long_to_be_string(data->ui32));
		if (data->i32<=B_ERRORS_END)
			texte+=sprintf(texte, " %s", strerror(data->i32));
		ajoute(texte, ')');
		break;

	case B_UINT32_TYPE:
	case B_SIZE_T_TYPE:
		texte+=sprintf(texte, "%lu (%s)", data->ui32, long_to_be_string(data->ui32));
		break;

	case B_INT64_TYPE:
	case B_OFF_T_TYPE:
		texte+=sprintf(texte, "%Ld (0x%.16LX)", data->i64, data->i64);
		break;

	case B_UINT64_TYPE:
		texte+=sprintf(texte, "%Lu (0x%.16LX)", data->ui64, data->ui64);
		break;

	case B_ASCII_TYPE:
	case B_STRING_TYPE:
	case B_MIME_TYPE:
	case B_MIME_STRING_TYPE:
	case 'MSIG':
		{
		long	tt=0;
		while (txtdata[tt]!=0 && txtdata[tt]!='\n')
			tt++;
		if (txtdata[tt]!=0) {
			ajoute(texte, "Multiline texte follows:\n");
			ajoute(texte, gPrefix);
			ajoute(texte, indent3);
			tt=0;
			while (txtdata[tt]!=0 && tt<size) {
				ajoute(texte, txtdata[tt]);
				if (txtdata[tt++]=='\n') {
					ajoute(texte, gPrefix);
					ajoute(texte, indent3);
				}
			}
		} else {
			ajoute(texte, '"');
			tt=0;
			while (txtdata[tt]!=0 && tt<size)
				ajoute(texte, txtdata[tt++]);
			ajoute(texte, '"');
		}
		}
		break;

	case B_POINTER_TYPE:
		texte+=sprintf(texte, "%p", pvoid);
		break;

	case B_POINT_TYPE:
		{
			BPoint	*point=(BPoint*) pvoid;
			texte+=sprintf(texte, "x=%.2f", point->x);
			tab_to(texte, lPrefix+kTabType+kTabName+kTabNum);
			texte+=sprintf(texte, "y=%.2f", point->y);
		}
		break;

	case B_RECT_TYPE:
		{
			BRect	*rect=(BRect*) pvoid;
			texte+=sprintf(texte, "l=%.2f", rect->left);
			tab_to(texte, lPrefix+kTabType+kTabName+kTabNum);
			texte+=sprintf(texte, "t=%.2f", rect->top);
			tab_to(texte, lPrefix+kTabType+kTabName+2*kTabNum);
			texte+=sprintf(texte, "r=%.2f", rect->right);
			tab_to(texte, lPrefix+kTabType+kTabName+3*kTabNum);
			texte+=sprintf(texte, "b=%.2f", rect->bottom);
		}
		break;

	case B_REF_TYPE:
		{
			entry_ref	*ref=(entry_ref*) pvoid;
			off_t	size;
			texte+=sprintf(texte, "\n%s ", gPrefix);
			BEntry	entry(ref, false);
			if (entry.InitCheck()==B_OK) {
				if (entry.IsFile())
					ajoute(texte, "File");
				else if (entry.IsDirectory())
					ajoute(texte, "Directory");
				else if (entry.IsSymLink())
					ajoute(texte, "Link");
				else ajoute(texte, "The entry points to an unexisting object");
				BPath	path;
				entry.GetPath(&path);
				texte+=sprintf(texte, ": %s\n", path.Path());
				BNode	node(&entry);
				if (entry.IsSymLink()) {
					texte+=sprintf(texte, "%s ", gPrefix);
					BEntry target(ref, true);
					if (target.GetPath(&path)==B_OK)
						texte+=sprintf(texte, "Valid target: ");
					else
						texte+=sprintf(texte, "Invalid target: ");
					char	linkto[B_PATH_NAME_LENGTH+1];
					BSymLink link(ref);
					if (link.InitCheck()==B_OK) {
						linkto[link.ReadLink(linkto, B_PATH_NAME_LENGTH)]=0;
						texte+=sprintf(texte, "%s\n", linkto);
					} else
						ajoute(texte, "Unreadable link.\n");
				} else if (entry.IsFile()) {
					node.GetSize(&size);
					texte+=sprintf(texte, "%s Size: %Ld byte", gPrefix, size);
					if (size!=1)
						ajoute(texte, 's');
					ajoute(texte, ".\n");
				}
				char	attribute[B_ATTR_NAME_LENGTH];
				cr=false;
				strncat(gPrefix, indent2, max);
				while (node.GetNextAttrName(attribute)==B_OK) {
					attr_info	infos;
					size_t		length;
					node.GetAttrInfo(attribute, &infos);
					ajoute(texte, gPrefix);
					ajoute(texte, long_to_be_string(infos.type));
					tab_to(texte, strlen(gPrefix)+kTabType);
					texte+=sprintf(texte, "\"%s\"", attribute);
					tab_to(texte, strlen(gPrefix)+kTabType+kTabName);
					char*		buffeur=new char[infos.size+1];
					length=node.ReadAttr(attribute, infos.type, 0, buffeur, infos.size);
					if (length==infos.size) {
						buffeur[length]=0;
						print_type(texte, infos.type, buffeur, infos.size);
					} else
						texte+=sprintf(texte, "[attribute not read properly (%s)]\n", strerror(length));
					delete[] buffeur;
				}
				gPrefix[lPrefix]=0;
			} else
				ajoute(texte, "Bad ref.");
		}
		break;					

	case B_MESSAGE_TYPE:
		if (local.Unflatten(txtdata)==B_OK) {
			ajoute(texte, '\n');
			strncat(gPrefix, indent1, max);
			do_print_message(texte, &local);
			gPrefix[lPrefix]=0;
			cr=false;
		} else
			ajoute(texte, "[invalid message]");
		break;

	case B_TIME_TYPE:
		{
			char*	t=ctime(&data->time);
			if (t) {
				ajoute(texte, t);
				cr=false;
			}
		}
		break;

	case B_RGB_32_BIT_TYPE:
	case B_RGB_COLOR_TYPE:
		texte+=sprintf(texte, "r=%u", data->rgb.red);
		tab_to(texte, lPrefix+kTabType+kTabName+kTabNum);
		texte+=sprintf(texte, "g=%u", data->rgb.green);
		tab_to(texte, lPrefix+kTabType+kTabName+2*kTabNum);
		texte+=sprintf(texte, "b=%u", data->rgb.blue);
		tab_to(texte, lPrefix+kTabType+kTabName+3*kTabNum);
		texte+=sprintf(texte, "a=%u", data->rgb.alpha);
		break;
		
	case B_PROPERTY_INFO_TYPE:
		{
			BPropertyInfo pinfo;
			if (pinfo.Unflatten(B_PROPERTY_INFO_TYPE, pvoid, size)==B_OK) {
				const property_info *infos=pinfo.Properties();
				if (infos) {
					int	k=0;
					int count=pinfo.CountProperties();
					while (count-->0) {
						texte+=sprintf(texte, "\n%s Name[%d]=%s\n%s ", gPrefix, k+1, infos[k].name, gPrefix);
						int j=0;
						do {
							ajoute(texte, ' ');
							if (infos[k].commands[j]==0)
								ajoute(texte, "All possible commands.");
							else
								ajoute(texte, long_to_be_string(infos[k].commands[j]));
						} while (j<9 && infos[k].commands[j]!=0 && infos[k].commands[++j]!=0);
						texte+=sprintf(texte, "\n%s ", gPrefix);
						j=0;
						do {
							if (infos[k].specifiers[j]==0)
								ajoute(texte, " All possible specifiers.");
							else {
								ajoute(texte, ' ');
								uint32 s=0;
								while (specifiers[s]!=NULL && s!=infos[k].specifiers[j])
									s++;
								if (specifiers[s])
									ajoute(texte, specifiers[s]);
								else
									texte+=sprintf(texte, "%d", (int) infos[k].specifiers[j]);
							}
						} while (j<9 && infos[k].specifiers[j]!=0 && infos[k].specifiers[++j]!=0);
						if (infos[k].usage) {
							texte+=sprintf(texte, "\n%s  Usage=", gPrefix);
							if (infos[k].usage)
								ajoute(texte, infos[k].usage);
						}
						if (infos[k].extra_data)
							texte+=sprintf(texte, "\n%s  Extra Data: %lu", gPrefix, infos[k].extra_data);
						k++;
					}
				} else
					ajoute(texte, "Empty.");
			} else
				ajoute(texte, "Invalid.");
		}
		break;

	case 'APPV':
		{
			struct version_info	*vinfos=(struct version_info*) pvoid;
			const char*	variety[]= { "Development", "Alpha", "Beta", "Gamma", "Golden Master", "Final" };
			for (int32 v=0; (unsigned)size>=sizeof(version_info); size-=sizeof(version_info), v++, vinfos++) {
				texte+=sprintf(texte, "\n%s%s", gPrefix, indent1);
				if (v==B_APP_VERSION_KIND)
					ajoute(texte, "Application");
				else if (v==B_SYSTEM_VERSION_KIND)
					ajoute(texte, "System");
				else
					ajoute(texte, "Extra");
				texte+=sprintf(texte, " Version: %lu.%lu.%lu ", vinfos->major, vinfos->middle, vinfos->minor);
				if (vinfos->variety<6)
					ajoute(texte, variety[vinfos->variety]);
				else
					texte+=sprintf(texte, "Unknown Variety (%lu)", vinfos->variety);
				texte+=sprintf(texte, " #%lu", vinfos->internal);
				vinfos->short_info[63]=0;
				texte+=sprintf(texte, "\n%s%s Short Info: %s", gPrefix, indent1, vinfos->short_info);
				vinfos->long_info[255]=0;
				texte+=sprintf(texte, "\n%s%s Long Info:  %s", gPrefix, indent1, vinfos->long_info);
			}
			if (size>0)
				texte+=sprintf(texte, "\n%s with %lu extra byte(s)", gPrefix, size);
		}
		break;

	case B_MESSENGER_TYPE:
		{
			BMessenger	*messenger=(BMessenger*) pvoid;
			if (!messenger->IsValid())
				ajoute(texte, "Invalid Messenger");
			else {
				if (messenger->IsTargetLocal()) {
					BLooper *looper;
					BHandler *handler=messenger->Target(&looper);
					if (!handler) {
						team_info	tminfo;
						texte+=sprintf(texte, "\n%s%sLooper: ", gPrefix, indent1);
						if (get_team_info(looper->Team(), &tminfo)==B_OK)
							texte+=sprintf(texte, "\n%s%s Team #%d", gPrefix, indent1, (int) looper->Team());
						else
							texte+=sprintf(texte, "\n%s%s Team #%d (not found)", gPrefix, indent1, (int) looper->Team());
						thread_info	thinfo;
						if (get_thread_info(looper->Thread(), &thinfo)==B_OK)
							texte+=sprintf(texte, "\n%s%s Thread #%d %s", gPrefix, indent1, (int) looper->Thread(), thinfo.name);
						else
							texte+=sprintf(texte, "\n%s%s Thread #%d (not found)", gPrefix, indent1, (int) looper->Thread());
						handler=looper;
					}
					texte+=sprintf(texte, "\n%s%sHandler: %s", gPrefix, indent1, handler->Name());
				} else {
					app_info info;
					ajoute(texte, "Remote Looper:");
					if (be_roster->GetRunningAppInfo(messenger->Team(), &info)==B_OK) {
						BEntry		entry(&info.ref);
						char		appname[B_FILE_NAME_LENGTH];
						entry.GetName(appname);
						texte+=sprintf(texte, " Team #%d \"%s\" (%s)", (int) messenger->Team(), appname, info.signature);
					} else
						texte+=sprintf(texte, " Team #%d (team not found)", (int) messenger->Team());
				}
			}
		}
		break;

	case 'ICON':
	case 'MICN':
		texte+=sprintf(texte, "[%d bytes of icon]", (int) size);
		break;
	
	case B_ANY_TYPE:
	case B_COLOR_8_BIT_TYPE:
	case B_GRAYSCALE_8_BIT_TYPE:
	case B_MONOCHROME_1_BIT_TYPE:
	case B_OBJECT_TYPE:
	case B_PATTERN_TYPE:
	case B_RAW_TYPE:
	default:
//		if (size==1)
//			ajoute(texte, "[Unsupported (one byte)]");
//		else
//			texte+=sprintf(texte, "[Unsupported (%d bytes)]", size);
		// Type is not known. Try to interpret it as an archived object...
		BMessage	archive;
		if (archive.Unflatten(txtdata)==B_OK) {
			ajoute(texte, "Is an archived object in a BMessage\n");
			strncat(gPrefix, indent2, max);
			do_print_message(texte, &archive);
			gPrefix[lPrefix]=0;
			cr=false;
		} else {
			print_hex(texte, pvoid, size, kTabType+kTabName);
//			int32	*pint=(int32 *) pvoid;
//			long	done=0;
//			int		tab=lPrefix+kTabType+kTabName;
//			while (done<size && (done<gHexalinesPref*16 || size-done<=16)) {
//				texte+=sprintf(texte, "%.4lX - ", done);
//				int	todo=size-done;
//				if (todo>=16) {
//					todo=16;
//					int32	*cp=&pint[done/4];
//					texte+=sprintf(texte, "%.8lX %.8lX %.8lX %.8lX  ", B_HOST_TO_BENDIAN_INT32(*cp), B_HOST_TO_BENDIAN_INT32(cp[1]),
//							B_HOST_TO_BENDIAN_INT32(cp[2]), B_HOST_TO_BENDIAN_INT32(cp[3]));
//				} else {
//					int32	intbuff[4];
//					memcpy(intbuff, &txtdata[done], size-done);
//					texte+=sprintf(texte, "%.8lX %.8lX %.8lX %.8lX  ", B_HOST_TO_BENDIAN_INT32(*intbuff), B_HOST_TO_BENDIAN_INT32(intbuff[1]),
//							B_HOST_TO_BENDIAN_INT32(intbuff[2]), B_HOST_TO_BENDIAN_INT32(intbuff[3]));
//					int	efface=(16-(size-done))*2;
//					char	*e=texte;
//					while (efface>0) {
//						e--;
//						if (*e!=' ') {
//							*e=' ';
//							efface--;
//						}
//					}
//				}
//				for (int n=0; n<todo; n++, done) {
//					char	c=txtdata[done++];
//					if (!isprint(c))
//						c='.';
//					ajoute(texte, c);
//	//				if (done % 8 == 0)
//	//					ajoute(texte, ' ');
//				}
//				if (done<size) {
//					texte+=sprintf(texte, "\n%s", gPrefix);
//					tab_to(texte, tab);
//				}
//			}
//			if (done<size)
//				texte+=sprintf(texte, "       [%d bytes remaining of %d bytes]", (int) (size-done), (int) size);
		}
		break;
	}
	if (cr)
		ajoute(texte, '\n');
}

void ajoute(pchar & texte, const char* mot, long length)
{
	long k;
	if (length==0)
		while (mot[length])
			ajoute(texte, mot[length++]);
	else
		for (k=0; mot[k]!=0 && k<length; k++)
			ajoute(texte, mot[k]);
}

void tab_to(pchar & texte, int n)
{
	int p=0;
	while (texte[-p-1]!='\n')
		p++;
	if (p<n) {
		while (p++<n)
			ajoute(texte, ' ');
	} else
		if (texte[p-1]!=' ')
			ajoute(texte, ' ');
}

void print_hex(pchar & texte, void* pvoid, ssize_t size, int extra_space)
{
	int32	*pint=(int32 *) pvoid;
	char*	txtdata=(char*) pvoid;
	long	done=0;
	int		tab=strlen(gPrefix)+extra_space;
	if (extra_space == 0)
		ajoute (texte, gPrefix);
	while (done<size && (done<gHexalinesPref*16 || size-done<=16)) {
		texte+=sprintf(texte, "%04lX - ", done);
		int	todo=size-done;
		if (todo>=16) {
			todo=16;
			int32	*cp=&pint[done/4];
			texte+=sprintf(texte, "%.8lX %.8lX %.8lX %.8lX  ", B_HOST_TO_BENDIAN_INT32(*cp), B_HOST_TO_BENDIAN_INT32(cp[1]),
					B_HOST_TO_BENDIAN_INT32(cp[2]), B_HOST_TO_BENDIAN_INT32(cp[3]));
		} else {
			int32	intbuff[4];
			memcpy(intbuff, &txtdata[done], size-done);
			texte+=sprintf(texte, "%.8lX %.8lX %.8lX %.8lX  ", B_HOST_TO_BENDIAN_INT32(*intbuff), B_HOST_TO_BENDIAN_INT32(intbuff[1]),
					B_HOST_TO_BENDIAN_INT32(intbuff[2]), B_HOST_TO_BENDIAN_INT32(intbuff[3]));
			int	efface=(16-(size-done))*2;
			char	*e=texte;
			while (efface>0) {
				e--;
				if (*e!=' ') {
					*e=' ';
					efface--;
				}
			}
		}
		for (int n=0; n<todo; n++, done) {
			char	c=txtdata[done++];
			if (c >= 0 && c < ' ')
				c='.';
			ajoute(texte, c);
//				if (done % 8 == 0)
//					ajoute(texte, ' ');
		}
		if (done<size) {
			texte+=sprintf(texte, "\n%s", gPrefix);
			if (extra_space>0)
				tab_to(texte, tab);
		}
	}
	if (done<size)
		texte+=sprintf(texte, "       [%d bytes remaining of %d bytes]", (int) (size-done), (int) size);
}
