#ifndef _PPP_STATS_H_
#define _PPP_STATS_H_

/*
 * Statistics.
 */
struct pppstat	{
    uint	ppp_ibytes;	       /* bytes received */
    uint	ppp_ipackets;	   /* packets received */
    uint	ppp_ierrors;	   /* receive errors */
    uint	ppp_obytes;	       /* bytes sent */
    uint	ppp_opackets;	   /* packets sent */
    uint	ppp_oerrors;	   /* transmit errors */
};

struct vjstat {
    uint	vjs_packets;	/* outbound packets */
    uint	vjs_compressed;	/* outbound compressed packets */
    uint	vjs_searches;	/* searches for connection state */
    uint	vjs_misses;	/* times couldn't find conn. state */
    uint	vjs_uncompressedin; /* inbound uncompressed packets */
    uint	vjs_compressedin;   /* inbound compressed packets */
    uint	vjs_errorin;	/* inbound unknown type packets */
    uint	vjs_tossed;	/* inbound packets tossed because of error */
};

struct ppp_stats {
    struct pppstat	p;	/* basic PPP statistics */
    struct vjstat	vj;	/* VJ header compression statistics */
};

struct compstat {
    uint	unc_bytes;	/* total uncompressed bytes */
    uint	unc_packets;	/* total uncompressed packets */
    uint	comp_bytes;	/* compressed bytes */
    uint	comp_packets;	/* compressed packets */
    uint	inc_bytes;	/* incompressible bytes */
    uint	inc_packets;	/* incompressible packets */
    uint	ratio;		/* recent compression ratio << 8 */
};

struct ppp_comp_stats {
    struct compstat	c;	/* packet compression statistics */
    struct compstat	d;	/* packet decompression statistics */
};

#endif /* _PPP_STATS_H_ */
