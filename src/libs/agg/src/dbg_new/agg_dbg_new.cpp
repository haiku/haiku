//----------------------------------------------------------------------------
// Anti-Grain Geometry - Version 2.2
// Copyright (C) 2002-2004 Maxim Shemanarev (http://www.antigrain.com)
//
// Permission to copy, use, modify, sell and distribute this software 
// is granted provided this copyright notice appears in all copies. 
// This software is provided "as is" without express or implied
// warranty, and with no claim as to its suitability for any purpose.
//
//----------------------------------------------------------------------------
// Contact: mcseem@antigrain.com
//          mcseemagg@yahoo.com
//          http://www.antigrain.com
//----------------------------------------------------------------------------
//
// Debuging stuff for catching memory leaks and corruptions
//
//----------------------------------------------------------------------------
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include "dbg_new/agg_dbg_new.h"

namespace agg
{

    int dbg_new_level = -1;

    struct dbg_new
    {
        unsigned new_count;
        unsigned del_count;
        unsigned new_bytes;
        unsigned del_bytes;
        unsigned max_count;
        unsigned max_bytes;
        unsigned corrupted_count;
        char     file_name[512];
        int      line;
        bool     report_all;
    };

    dbg_new dbg_new_info[max_dbg_new_level] = 
    {
        { 0, 0, 0, 0, 0, 0, 0, "", 0, false },
    };

#ifdef AGG_DBG_NEW_CHECK_ADDR
    void* dbg_new_allocations[max_allocations] = 
    {
       0,
    };
    unsigned dbg_new_prev_word[max_allocations] = 
    {
       0,
    };
       
       
    unsigned dbg_new_max_count = 0;
#endif


    //------------------------------------------------------------------------
    void add_allocated_addr(void* ptr)
    {
#ifdef AGG_DBG_NEW_CHECK_ADDR
        if(dbg_new_max_count >= max_allocations)
        {
            printf("ADDR_CHECK: limit exceeded\n");
        }
        dbg_new_allocations[dbg_new_max_count] = ptr;
        memcpy(dbg_new_prev_word + dbg_new_max_count,
               ((char*)ptr) - sizeof(unsigned),
               sizeof(unsigned));
        dbg_new_max_count++;
#endif
    }


    //------------------------------------------------------------------------
    bool check_and_remove_allocated_addr(void* ptr)
    {
#ifdef AGG_DBG_NEW_CHECK_ADDR
        unsigned i; 
        for(i = 0; i < dbg_new_max_count; i++)
        {
            if(dbg_new_allocations[i] == ptr)
            {
                unsigned prev;
                memcpy(&prev,
                       ((char*)ptr) - sizeof(unsigned),
                       sizeof(unsigned));
                
                if(prev != dbg_new_prev_word[i])
                {
                    printf("MEMORY CORRUPTION AT %08x prev=%08x cur=%08x\n", ptr, prev, dbg_new_prev_word[i]);
                    //return false;
                }
                
                if(i < dbg_new_max_count - 1)
                {
                   memmove(dbg_new_allocations + i, 
                           dbg_new_allocations + i + 1,
                           sizeof(void*) * dbg_new_max_count - i - 1);
                }
                dbg_new_max_count--;
                //printf("free ok\n");
                return true;
            }
        }
        printf("ATTEMPT TO FREE BAD ADDRESS %08x\n", ptr);
        return false;
#else
        return true;
#endif
    }


    //------------------------------------------------------------------------
    watchdoggy::watchdoggy(const char* file, int line, bool report_all)
    {
        #ifdef _WIN32
        if(dbg_new_level == -1)
        {
           FILE* fd = fopen("stdout.txt", "w");
           fclose(fd);
        }
        #endif

        dbg_new_level++;
        ::memset(dbg_new_info + dbg_new_level, 0, sizeof(dbg_new));
        if(file == 0) file = "";
        int len = strlen(file);
        if(len > 511) len = 511;
        ::memcpy(dbg_new_info[dbg_new_level].file_name, file, len);
        dbg_new_info[dbg_new_level].file_name[len] = 0;
        dbg_new_info[dbg_new_level].line = line;
        dbg_new_info[dbg_new_level].report_all = report_all;
        printf("wd%d started. File:%s line:%d\n", 
                dbg_new_level, 
                file, 
                line);
    }


    //------------------------------------------------------------------------
    watchdoggy::~watchdoggy()
    {
        if(dbg_new_level >= 0)
        {
            printf("wd%d stopped. File:%s line:%d\n", 
                    dbg_new_level, 
                    dbg_new_info[dbg_new_level].file_name, 
                    dbg_new_info[dbg_new_level].line);
            printf("new_count=%u del_count=%u max_count=%u balance=%d\n"
                   "new_bytes=%u del_bytes=%u max_bytes=%u balance=%d "
                   "corrupted_count=%u\n",
                    dbg_new_info[dbg_new_level].new_count,
                    dbg_new_info[dbg_new_level].del_count,
                    dbg_new_info[dbg_new_level].max_count,
                    int(dbg_new_info[dbg_new_level].new_count - 
                        dbg_new_info[dbg_new_level].del_count),
                    dbg_new_info[dbg_new_level].new_bytes,
                    dbg_new_info[dbg_new_level].del_bytes,
                    dbg_new_info[dbg_new_level].max_bytes,
                    int(dbg_new_info[dbg_new_level].new_bytes - 
                        dbg_new_info[dbg_new_level].del_bytes),
                    dbg_new_info[dbg_new_level].corrupted_count
            );
            dbg_new_level--;
        }
    }


    //------------------------------------------------------------------------
    //
    // This code implements the AUTODIN II polynomial
    // The variable corresponding to the macro argument "crc" should
    // be an unsigned long.
    // Oroginal code  by Spencer Garrett <srg@quick.com>
    //
    // generated using the AUTODIN II polynomial
    //	x^32 + x^26 + x^23 + x^22 + x^16 +
    //	x^12 + x^11 + x^10 + x^8 + x^7 + x^5 + x^4 + x^2 + x^1 + 1
    //
    //------------------------------------------------------------------------
    static const unsigned crc32tab[256] = 
    {
	    0x00000000, 0x77073096, 0xee0e612c, 0x990951ba,
	    0x076dc419, 0x706af48f, 0xe963a535, 0x9e6495a3,
	    0x0edb8832, 0x79dcb8a4, 0xe0d5e91e, 0x97d2d988,
	    0x09b64c2b, 0x7eb17cbd, 0xe7b82d07, 0x90bf1d91,
	    0x1db71064, 0x6ab020f2, 0xf3b97148, 0x84be41de,
	    0x1adad47d, 0x6ddde4eb, 0xf4d4b551, 0x83d385c7,
	    0x136c9856, 0x646ba8c0, 0xfd62f97a, 0x8a65c9ec,
	    0x14015c4f, 0x63066cd9, 0xfa0f3d63, 0x8d080df5,
	    0x3b6e20c8, 0x4c69105e, 0xd56041e4, 0xa2677172,
	    0x3c03e4d1, 0x4b04d447, 0xd20d85fd, 0xa50ab56b,
	    0x35b5a8fa, 0x42b2986c, 0xdbbbc9d6, 0xacbcf940,
	    0x32d86ce3, 0x45df5c75, 0xdcd60dcf, 0xabd13d59,
	    0x26d930ac, 0x51de003a, 0xc8d75180, 0xbfd06116,
	    0x21b4f4b5, 0x56b3c423, 0xcfba9599, 0xb8bda50f,
	    0x2802b89e, 0x5f058808, 0xc60cd9b2, 0xb10be924,
	    0x2f6f7c87, 0x58684c11, 0xc1611dab, 0xb6662d3d,
        
	    0x76dc4190, 0x01db7106, 0x98d220bc, 0xefd5102a,
	    0x71b18589, 0x06b6b51f, 0x9fbfe4a5, 0xe8b8d433,
	    0x7807c9a2, 0x0f00f934, 0x9609a88e, 0xe10e9818,
	    0x7f6a0dbb, 0x086d3d2d, 0x91646c97, 0xe6635c01,
	    0x6b6b51f4, 0x1c6c6162, 0x856530d8, 0xf262004e,
	    0x6c0695ed, 0x1b01a57b, 0x8208f4c1, 0xf50fc457,
	    0x65b0d9c6, 0x12b7e950, 0x8bbeb8ea, 0xfcb9887c,
	    0x62dd1ddf, 0x15da2d49, 0x8cd37cf3, 0xfbd44c65,
	    0x4db26158, 0x3ab551ce, 0xa3bc0074, 0xd4bb30e2,
	    0x4adfa541, 0x3dd895d7, 0xa4d1c46d, 0xd3d6f4fb,
	    0x4369e96a, 0x346ed9fc, 0xad678846, 0xda60b8d0,
	    0x44042d73, 0x33031de5, 0xaa0a4c5f, 0xdd0d7cc9,
	    0x5005713c, 0x270241aa, 0xbe0b1010, 0xc90c2086,
	    0x5768b525, 0x206f85b3, 0xb966d409, 0xce61e49f,
	    0x5edef90e, 0x29d9c998, 0xb0d09822, 0xc7d7a8b4,
	    0x59b33d17, 0x2eb40d81, 0xb7bd5c3b, 0xc0ba6cad,
        
	    0xedb88320, 0x9abfb3b6, 0x03b6e20c, 0x74b1d29a,
	    0xead54739, 0x9dd277af, 0x04db2615, 0x73dc1683,
	    0xe3630b12, 0x94643b84, 0x0d6d6a3e, 0x7a6a5aa8,
	    0xe40ecf0b, 0x9309ff9d, 0x0a00ae27, 0x7d079eb1,
	    0xf00f9344, 0x8708a3d2, 0x1e01f268, 0x6906c2fe,
	    0xf762575d, 0x806567cb, 0x196c3671, 0x6e6b06e7,
	    0xfed41b76, 0x89d32be0, 0x10da7a5a, 0x67dd4acc,
	    0xf9b9df6f, 0x8ebeeff9, 0x17b7be43, 0x60b08ed5,
	    0xd6d6a3e8, 0xa1d1937e, 0x38d8c2c4, 0x4fdff252,
	    0xd1bb67f1, 0xa6bc5767, 0x3fb506dd, 0x48b2364b,
	    0xd80d2bda, 0xaf0a1b4c, 0x36034af6, 0x41047a60,
	    0xdf60efc3, 0xa867df55, 0x316e8eef, 0x4669be79,
	    0xcb61b38c, 0xbc66831a, 0x256fd2a0, 0x5268e236,
	    0xcc0c7795, 0xbb0b4703, 0x220216b9, 0x5505262f,
	    0xc5ba3bbe, 0xb2bd0b28, 0x2bb45a92, 0x5cb36a04,
	    0xc2d7ffa7, 0xb5d0cf31, 0x2cd99e8b, 0x5bdeae1d,
        
	    0x9b64c2b0, 0xec63f226, 0x756aa39c, 0x026d930a,
	    0x9c0906a9, 0xeb0e363f, 0x72076785, 0x05005713,
	    0x95bf4a82, 0xe2b87a14, 0x7bb12bae, 0x0cb61b38,
	    0x92d28e9b, 0xe5d5be0d, 0x7cdcefb7, 0x0bdbdf21,
	    0x86d3d2d4, 0xf1d4e242, 0x68ddb3f8, 0x1fda836e,
	    0x81be16cd, 0xf6b9265b, 0x6fb077e1, 0x18b74777,
	    0x88085ae6, 0xff0f6a70, 0x66063bca, 0x11010b5c,
	    0x8f659eff, 0xf862ae69, 0x616bffd3, 0x166ccf45,
	    0xa00ae278, 0xd70dd2ee, 0x4e048354, 0x3903b3c2,
	    0xa7672661, 0xd06016f7, 0x4969474d, 0x3e6e77db,
	    0xaed16a4a, 0xd9d65adc, 0x40df0b66, 0x37d83bf0,
	    0xa9bcae53, 0xdebb9ec5, 0x47b2cf7f, 0x30b5ffe9,
	    0xbdbdf21c, 0xcabac28a, 0x53b39330, 0x24b4a3a6,
	    0xbad03605, 0xcdd70693, 0x54de5729, 0x23d967bf,
	    0xb3667a2e, 0xc4614ab8, 0x5d681b02, 0x2a6f2b94,
	    0xb40bbe37, 0xc30c8ea1, 0x5a05df1b, 0x2d02ef8d
    };


    //------------------------------------------------------------------------
    static unsigned long calcCRC32(const char* buf, unsigned size)
    {
       unsigned long crc = (unsigned long)~0;
       const char* p;
       unsigned len = 0; 
       unsigned nr = size;

       for (len += nr, p = buf; nr--; ++p) 
       {
          crc = (crc >> 8) ^ crc32tab[(crc ^ *p) & 0xff];
       }
       return ~crc;
    }





    //------------------------------------------------------------------------
    void* dbg_malloc(unsigned size, const char* file, int line)
    {
        if(dbg_new_level < 0) 
        {
            void* ptr = ::malloc(size);
            printf("%u allocated at %08x %s %d\n", size, ptr, file, line);
            add_allocated_addr(ptr);
            return ptr;
        }

        // The structure of the allocated memory:
        // 
        // [USER_SIZE:16 bytes]   //allows for keeping proper data alignment
        // [USER_MEM]
        // [CRC32_sum:long, actual size depends on architecture]
        // [test_bytes:256 bytes] //should be enough for checking non-fatal 
        //                        //corruptions with keeping the line number 
        //                        //and filename correct
        // [line:unsigned]
        // [strlen:unsigned]
        // [file_name:zero_end_string]

        unsigned fname_len = ::strlen(file) + 1;
        unsigned total_size = 16 +                    // size
                              size +                  // user_mem
                              sizeof(unsigned long) + // crc32
                              256 +                   // zero_bytes
                              sizeof(unsigned) +      // line
                              sizeof(unsigned) +      // strlen
                              fname_len;              // .cpp file_name

        char* inf_ptr = (char*)::malloc(total_size);
        char* ret_ptr = inf_ptr + 16;
        
        int i;
        for(i = 0; i < 16; i++)
        {
            inf_ptr[i] = (char)(15-i);
        }
        ::memcpy(inf_ptr, &size, sizeof(unsigned));
        
        unsigned long crc32_before = calcCRC32(inf_ptr, 16);
        
        inf_ptr += 16 + size;
        char* crc32_ptr = inf_ptr;
        inf_ptr += sizeof(unsigned long);

        for(i = 0; i < 256; i++)
        {
            *inf_ptr++ = (char)(255-i);
        }
        
        ::memcpy(inf_ptr, &line, sizeof(unsigned));
        inf_ptr += sizeof(unsigned);
        
        ::memcpy(inf_ptr, &fname_len, sizeof(unsigned));
        inf_ptr += sizeof(unsigned);
        
        ::strcpy(inf_ptr, file);

        unsigned long // long just in case
            crc32_sum = calcCRC32(crc32_ptr + sizeof(unsigned long), 
                                  256 + 
                                  sizeof(unsigned) + 
                                  sizeof(unsigned) + 
                                  fname_len);
        
        crc32_sum ^= crc32_before;

        ::memcpy(crc32_ptr, &crc32_sum, sizeof(unsigned long));

        dbg_new_info[dbg_new_level].new_count++;
        dbg_new_info[dbg_new_level].new_bytes += size;
        int balance = int(dbg_new_info[dbg_new_level].new_count - 
                          dbg_new_info[dbg_new_level].del_count); 

        if(balance > 0)
        {
            if(balance > int(dbg_new_info[dbg_new_level].max_count))
            {
                dbg_new_info[dbg_new_level].max_count = unsigned(balance);
            }
        }


        balance = int(dbg_new_info[dbg_new_level].new_bytes - 
                      dbg_new_info[dbg_new_level].del_bytes); 

        if(balance > 0)
        {
            if(balance > int(dbg_new_info[dbg_new_level].max_bytes))
            {
                dbg_new_info[dbg_new_level].max_bytes = unsigned(balance);
            }
        }


        if(dbg_new_info[dbg_new_level].report_all)
        {
            printf("wdl%d. %u allocated %08x. %s, %d\n",
                    dbg_new_level,
                    size,
                    ret_ptr,
                    file,
                    line
            );
        }

        add_allocated_addr(ret_ptr);
        return ret_ptr;
    }



    //------------------------------------------------------------------------
    void dbg_free(void* ptr)
    {
        if(ptr == 0)
        {
           //printf("Null pointer free\n");
           return;
        }

        if(!check_and_remove_allocated_addr(ptr)) return;

        if(dbg_new_level < 0) 
        {
            printf("free at %08x\n", ptr);
            ::free(ptr);
            return;
        }

        char* free_ptr = (char*)ptr;
        char* inf_ptr = free_ptr;
        free_ptr -= 16;
        
        unsigned size;
        ::memcpy(&size, free_ptr, sizeof(unsigned));
        
        unsigned long crc32_before = calcCRC32(free_ptr, 16);
        
        inf_ptr += size;
        unsigned long crc32_sum;
        ::memcpy(&crc32_sum, inf_ptr, sizeof(unsigned long));
        inf_ptr += sizeof(unsigned long);
        char* crc32_ptr = inf_ptr;
        
        inf_ptr += 256;
        
        unsigned line;
        ::memcpy(&line, inf_ptr, sizeof(unsigned));
        inf_ptr += sizeof(unsigned);
        
        unsigned fname_len;
        ::memcpy(&fname_len, inf_ptr, sizeof(unsigned));
        inf_ptr += sizeof(unsigned);
        
        char file[512];
        if(fname_len > 511) fname_len = 511;
        ::memcpy(file, inf_ptr, fname_len);
        file[fname_len] = 0;

        if(crc32_sum != (calcCRC32(crc32_ptr, 
                                   256 +
                                   sizeof(unsigned) + 
                                   sizeof(unsigned) + 
                                   fname_len) ^ crc32_before))
        {
            printf("WD%d:MEMORY CORRUPTION AT %08x. Allocated %u bytes in %s, line %d\n",
                    dbg_new_level,
                    ptr,
                    size,                    
                    file,
                    line);
            dbg_new_info[dbg_new_level].corrupted_count++;
        }
        else
        {
            ::free(free_ptr);

            dbg_new_info[dbg_new_level].del_count++;
            dbg_new_info[dbg_new_level].del_bytes += size;
                        
            if(dbg_new_info[dbg_new_level].report_all)
            {
                printf("wdl%d. %u freed %08x. %s, %d\n",
                        dbg_new_level,
                        size,
                        free_ptr,
                        file,
                        line
                );
            }
        }
    }
}

#ifdef new
#undef new
#endif

//----------------------------------------------------------------------------
void* operator new (unsigned size, const char* file, int line)
{
    return agg::dbg_malloc(size, file, line);
}

//----------------------------------------------------------------------------
void* operator new [] (unsigned size, const char* file, int line)
{
    return agg::dbg_malloc(size, file, line);
}

//----------------------------------------------------------------------------
void  operator delete(void *ptr) throw()
{
    agg::dbg_free(ptr);
}

//----------------------------------------------------------------------------
void  operator delete [] (void *ptr) throw()
{
    agg::dbg_free(ptr);
}














/*
#include "dbg_new/agg_dbg_new.h"

int main()
{
    AGG_WATCHDOGGY(wd1, true);
    int* a = new int[100];
    a[100] = 0;

    delete [] a;
    return 0;
}
*/








