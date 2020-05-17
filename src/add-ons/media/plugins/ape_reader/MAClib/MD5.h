/*
 * Copyright (C) 2020 Adrien Destugues <pulkomandy@pulkomandy.tk>
 *
 * Distributed under terms of the MIT license.
 */

#ifndef MD5_H
#define MD5_H


#include "md5.h"


class CMD5Helper
{
public:

    CMD5Helper()
    {
        MD5_Init(&m_MD5Context);
        m_nTotalBytes = 0;
    }

    inline void AddData(const void * pData, int nBytes)
    {
        MD5_Update(&m_MD5Context, (const unsigned char *) pData, nBytes);
        m_nTotalBytes += nBytes;
    }

    void GetResult(unsigned char cResult[16])
    {
        MD5_Final(cResult, &m_MD5Context);
    }

protected:

    MD5_CTX m_MD5Context;
    int m_nTotalBytes;
};


#endif /* !MD5_H */
