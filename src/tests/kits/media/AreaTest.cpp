#include <OS.h> 
#include <stdio.h> 

int main() 
{ 
        int * ptr = new int[1]; 
        char *adr; 
        area_id id; 
        int offset; 
        

        area_info info; 
        id = area_for(ptr); 
        get_area_info(id, &info); 
        adr = (char *)info.address; 
        offset = (uint32)ptr - (uint32)adr; 

        
        char * adrclone1; 
        char * adrclone2; 
        int * ptrclone1; 
        int * ptrclone2; 
        area_id idclone1; 
        area_id idclone2; 
        
        idclone1 = clone_area("clone 1", (void **)&adrclone1, B_ANY_ADDRESS,B_READ_AREA | B_WRITE_AREA,id); 
        idclone2 = clone_area("clone 2", (void **)&adrclone2, B_ANY_ADDRESS,B_READ_AREA | B_WRITE_AREA,id); 
        
        ptrclone1 = (int *)(adrclone1 + offset); 
        ptrclone2 = (int *)(adrclone2 + offset); 
        
        printf("offset      = 0x%08x\n",(int)offset); 
        printf("id          = 0x%08x\n",(int)id); 
        printf("id  clone 1 = 0x%08x\n",(int)idclone1); 
        printf("id  clone 2 = 0x%08x\n",(int)idclone2); 
        printf("adr         = 0x%08x\n",(int)adr); 
        printf("adr clone 1 = 0x%08x\n",(int)adrclone1); 
        printf("adr clone 2 = 0x%08x\n",(int)adrclone2); 
        printf("ptr         = 0x%08x\n",(int)ptr); 
        printf("ptr clone 1 = 0x%08x\n",(int)ptrclone1); 
        printf("ptr clone 2 = 0x%08x\n",(int)ptrclone2); 

        ptr[0] = 0x12345678;    

        printf("ptr[0]         = 0x%08x\n",(int)ptr[0]); 
        printf("ptr clone 1[0] = 0x%08x\n",(int)ptrclone1[0]); 
        printf("ptr clone 2[0] = 0x%08x\n",(int)ptrclone2[0]); 

}
