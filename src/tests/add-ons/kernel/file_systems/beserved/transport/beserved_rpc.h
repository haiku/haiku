#include "betalk.h"

int btConnect(bt_rpcinfo *info, unsigned int serverIP, int port);
void btDisconnect(bt_rpcinfo *info);
void btRPCInit(bt_rpcinfo *info);
void btRPCClose(bt_rpcinfo *info);

bt_rpccall *btRPCInvoke(bt_rpcinfo *info, bt_outPacket *packet, bool lastPkt);
void btRPCRemoveCall(bt_rpccall *call);
