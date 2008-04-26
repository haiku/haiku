// rpc.h

#ifndef _BETALK_H_
#include "beCompat.h"
#include "betalk.h"
#endif

bt_inPacket *btRPCSimpleCall(unsigned int serverIP, int port, bt_outPacket *outPacket);
int btRPCConnect(unsigned int serverIP, int port);
bool btRPCSend(int session, bt_outPacket *outPacket);
bool btRPCCheckSignature(int session);
void btDestroyInPacket(bt_inPacket *packet);
bt_outPacket *btRPCPutHeader(unsigned char command, unsigned char argc, int32 length);
void btRPCPutArg(bt_outPacket *packet, unsigned int type, void *data, int length);

void btRPCCreateAck(bt_outPacket *packet, unsigned int xid, int error, int length);
void btRPCSendAck(int client, bt_outPacket *packet);
unsigned char btRPCGetChar(bt_inPacket *packet);
unsigned int btRPCGetInt32(bt_inPacket *packet);
int64 btRPCGetInt64(bt_inPacket *packet);
char *btRPCGetNewString(bt_inPacket *packet);
int btRPCGetString(bt_inPacket *packet, char *buffer, int length);
void btRPCGrowPacket(bt_outPacket *packet, int bytes);
void btRPCPutChar(bt_outPacket *packet, char value);
void btRPCPutInt32(bt_outPacket *packet, int32 value);
void btRPCPutInt64(bt_outPacket *packet, int64 value);
void btRPCPutString(bt_outPacket *packet, char *buffer, int length);
void btRPCPutBinary(bt_outPacket *packet, void *buffer, int length);
void btRPCPutHandle(bt_outPacket *packet, btFileHandle *fhandle);
int btRPCGetStat(bt_inPacket *packet, struct stat *st);
void btRPCPutStat(bt_outPacket *packet, struct stat *st);

int btRecv(int sock, void *data, int dataLen, int flags);
int btSend(int sock, void *data, int dataLen, int flags);
