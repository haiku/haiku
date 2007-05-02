#ifndef _FBSD_COMPAT_NET_BPF_H_
#define _FBSD_COMPAT_NET_BPF_H_

#define bpf_mtap(bpf_if, mbuf) do { } while (0)
#define BPF_MTAP(ifp, m) do { } while (0)

#endif
