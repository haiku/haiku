#include "rar.hpp"

int ToPercent(Int64 N1,Int64 N2)
{
  if (N2<N1)
    return(100);
  return(ToPercentUnlim(N1,N2));
}

/* allows percent larger than 100 */
int ToPercentUnlim(Int64 N1,Int64 N2)
{
  if (N2==0)
    return(0);
  return(int64to32(N1*100/N2));
}


void RARInitData()
{
  InitCRC();
  ErrHandler.Clean();
}
