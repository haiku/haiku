#include <Application.h>
#include <BufferGroup.h>
#include <Buffer.h>
#include <stdio.h>

int main()
{
	// app_server connection (no need to run it)
	BApplication app("application/x-vnd-test"); 
	
	BBufferGroup * group;
	status_t s;
	int32 count;
	BBuffer *buffer;

/*
	printf("using default constructor:\n");
	group = new BBufferGroup();


	s = group->InitCheck();
	printf("InitCheck: status = %ld\n",s);
	
	s = group->CountBuffers(&count);
	printf("CountBuffers: count = %ld, status = %ld\n",count,s);
	
	delete group;
*/
	printf("\n");
	printf("using size = 1234 constructor:\n");
	group = new BBufferGroup(1234);

	s = group->InitCheck();
	printf("InitCheck: status = %ld\n",s);
	
	s = group->CountBuffers(&count);
	printf("CountBuffers: count = %ld, status = %ld\n",count,s);

	s = group->GetBufferList(1,&buffer);
	printf("GetBufferList: status = %ld\n",s);

	printf("Buffer->Data:  = %08x\n",(int)buffer->Data());

	printf("Buffer->ID:  = %d\n",(int)buffer->ID());

	printf("Buffer->Size:  = %ld\n",buffer->Size());

	printf("Buffer->SizeAvailable:  = %ld\n",buffer->SizeAvailable());

	printf("Buffer->SizeUsed:  = %ld\n",buffer->SizeUsed());

	printf("\n");

	media_buffer_id id = buffer->ID();
	BBufferGroup * group2 = new BBufferGroup(1,&id);
	printf("creating second group with a buffer from first group:\n");

	s = group2->InitCheck();
	printf("InitCheck: status = %ld\n",s);

	s = group2->CountBuffers(&count);
	printf("CountBuffers: count = %ld, status = %ld\n",count,s);

	buffer = 0;
	s = group2->GetBufferList(1,&buffer);
	printf("GetBufferList: status = %ld\n",s);

	printf("Buffer->Data:  = %08x\n",(int)buffer->Data());

	printf("Buffer->ID:  = %d\n",(int)buffer->ID());

	printf("Buffer->Size:  = %ld\n",buffer->Size());

	printf("Buffer->SizeAvailable:  = %ld\n",buffer->SizeAvailable());

	printf("Buffer->SizeUsed:  = %ld\n",buffer->SizeUsed());

	delete group;
	delete group2;

	printf("\n");
/*
	printf("creating a BSmallBuffer:\n");
	BSmallBuffer * sb = new BSmallBuffer;

	printf("sb->Data:  = %08x\n",(int)sb->Data());

	printf("sb->ID:  = %d\n",(int)sb->ID());

	printf("sb->Size:  = %ld\n",sb->Size());

	printf("sb->SizeAvailable:  = %ld\n",sb->SizeAvailable());

	printf("sb->SizeUsed:  = %ld\n",sb->SizeUsed());

	printf("sb->SmallBufferSizeLimit:  = %ld\n",sb->SmallBufferSizeLimit());

	delete sb;
*/
	return 0;
}
