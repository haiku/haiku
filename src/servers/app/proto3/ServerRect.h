#ifndef _SERVER_RECT_H_
#define _SERVER_RECT_H_

class ServerRect
{
public:
	ServerRect(int32 l, int32 t, int32 r, int32 b)
		{	left=l; top=t; right=r; bottom=b; }
	ServerRect(void)
		{	left=top=0; right=bottom=0; }
	ServerRect(const ServerRect &rect);
    ServerRect  Bounds(void) const
    	{ return( ServerRect( 0, 0, right - left, bottom - top ) );}
    	
    virtual ~ServerRect(void);
    bool Intersect(const ServerRect &rect ) const
    	{ return( !( rect.right < left || rect.left > right || rect.bottom < top || rect.top > bottom ) ); }
	
	
	int32 left,top,right,bottom;
};

ServerRect::ServerRect( const ServerRect& rect )
{
    left   = rect.left;
    top    = rect.top;
    right  = rect.right;
    bottom = rect.bottom;
}

ServerRect::~ServerRect(void)
{
}

#endif