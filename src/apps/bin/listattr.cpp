// Author: Ryan Fleet
// Created: 11th October 2002
// Modified: 11th October 2002

#include <stdio.h>
#include <Node.h>
#include <fs_attr.h>

int main(int argc, char *argv[])
{
	if(argc < 2)
	{
		printf("usage: listattr 'filename' ['filename' ...]\n");
		return 1;
	}
	
	for(int i = 1; i < argc; ++i)
	{
		BNode node(argv[i]);
		if (B_OK != node.InitCheck())
		{
			printf("listattr: error for '%s' (Initialization failed)\n", argv[i]);
			return 0;
		}

		printf("file %s\n", argv[i]);
		printf("  Type         Size                 Name\n");
		printf("----------  ---------   -------------------------------\n");

		char szBuffer[B_ATTR_NAME_LENGTH];
		while(B_OK == node.GetNextAttrName(szBuffer))
		{
			attr_info attrInfo;
			node.GetAttrInfo(szBuffer, &attrInfo);
			switch(attrInfo.type)
			{
			case 0x4d494d53:
				printf("  MIME str");
				break;
			case 0x43535452:
				printf("      Text");
				break;
			case 0x4c4f4e47:
				printf("    Int-32");
				break;
			default:
				printf("0x%lx", attrInfo.type);
				break;
			};	
		
			printf("% 11lli", attrInfo.size);
			printf("%34s\n", szBuffer);
		}
	}
		
	return 0;
}
