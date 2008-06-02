#include <net_datalink.h>
#include <net_protocol.h>
#include <net_stack.h>
#include <NetBufferUtilities.h>

#include <KernelExport.h>
#include <util/list.h>

#include <netinet/in.h>
#include <new>
#include <stdlib.h>
#include <string.h>

#include <bluetooth/HCI/btHCI_acl.h>
#include <btDebug.h>


typedef NetBufferField<uint16, offsetof(hci_acl_header, alen)> ICMPChecksumField;

#define ICMP_TYPE_ECHO_REPLY	0
#define ICMP_TYPE_UNREACH		3
#define ICMP_TYPE_REDIRECT		5
#define ICMP_TYPE_ECHO_REQUEST	8

// type unreach codes
#define ICMP_CODE_UNREACH_NEED_FRAGMENT	4	// this is used for path MTU discovery

struct l2cap_protocol : net_protocol {
};


net_buffer_module_info *gBufferModule;
static net_stack_module_info *sStackModule;


net_protocol *
l2cap_init_protocol(net_socket *socket)
{
	l2cap_protocol *protocol = new (std::nothrow) l2cap_protocol;
	if (protocol == NULL)
		return NULL;

	return protocol;
}


status_t
l2cap_uninit_protocol(net_protocol *protocol)
{
	delete protocol;
	return B_OK;
}


status_t
l2cap_open(net_protocol *protocol)
{
	return B_OK;
}


status_t
l2cap_close(net_protocol *protocol)
{
	return B_OK;
}


status_t
l2cap_free(net_protocol *protocol)
{
	return B_OK;
}


status_t
l2cap_connect(net_protocol *protocol, const struct sockaddr *address)
{
	return B_ERROR;
}


status_t
l2cap_accept(net_protocol *protocol, struct net_socket **_acceptedSocket)
{
	return EOPNOTSUPP;
}


status_t
l2cap_control(net_protocol *protocol, int level, int option, void *value,
	size_t *_length)
{
	return protocol->next->module->control(protocol->next, level, option,
		value, _length);
}


status_t
l2cap_getsockopt(net_protocol *protocol, int level, int option,
	void *value, int *length)
{
	return protocol->next->module->getsockopt(protocol->next, level, option,
		value, length);
}


status_t
l2cap_setsockopt(net_protocol *protocol, int level, int option,
	const void *value, int length)
{
	return protocol->next->module->setsockopt(protocol->next, level, option,
		value, length);
}


status_t
l2cap_bind(net_protocol *protocol, const struct sockaddr *address)
{
	return B_ERROR;
}


status_t
l2cap_unbind(net_protocol *protocol, struct sockaddr *address)
{
	return B_ERROR;
}


status_t
l2cap_listen(net_protocol *protocol, int count)
{
	return EOPNOTSUPP;
}


status_t
l2cap_shutdown(net_protocol *protocol, int direction)
{
	return EOPNOTSUPP;
}


status_t
l2cap_send_data(net_protocol *protocol, net_buffer *buffer)
{
	return protocol->next->module->send_data(protocol->next, buffer);
}


status_t
l2cap_send_routed_data(net_protocol *protocol, struct net_route *route,
	net_buffer *buffer)
{
	return protocol->next->module->send_routed_data(protocol->next, route, buffer);
}


ssize_t
l2cap_send_avail(net_protocol *protocol)
{
	return B_ERROR;
}


status_t
l2cap_read_data(net_protocol *protocol, size_t numBytes, uint32 flags,
	net_buffer **_buffer)
{
	return B_ERROR;
}


ssize_t
l2cap_read_avail(net_protocol *protocol)
{
	return B_ERROR;
}


struct net_domain *
l2cap_get_domain(net_protocol *protocol)
{
	return protocol->next->module->get_domain(protocol->next);
}


size_t
l2cap_get_mtu(net_protocol *protocol, const struct sockaddr *address)
{
	return protocol->next->module->get_mtu(protocol->next, address);
}


status_t
l2cap_receive_data(net_buffer *buffer)
{
	debugf("ICMP received some data, buffer length %lu\n", buffer->size);

	NetBufferHeaderReader<hci_acl_header> bufferHeader(buffer);
	if (bufferHeader.Status() < B_OK)
		return bufferHeader.Status();

	hci_acl_header &header = bufferHeader.Data();

	debugf("  got type %u, code %u, checksum %u\n", header.type, header.code, ntohs(header.checksum));
	debugf("  computed checksum: %ld\n", gBufferModule->checksum(buffer, 0, buffer->size, true));

	if (gBufferModule->checksum(buffer, 0, buffer->size, true) != 0)
		return B_BAD_DATA;

	gBufferModule->free(buffer);
	return B_OK;
}


status_t
l2cap_error(uint32 code, net_buffer *data)
{
	return B_ERROR;
}


status_t
l2cap_error_reply(net_protocol *protocol, net_buffer *causedError, uint32 code,
	void *errorData)
{
	return B_ERROR;
}


//	#pragma mark -


static status_t
l2cap_std_ops(int32 op, ...)
{
	switch (op) {
		case B_MODULE_INIT:
		{
			sStackModule->register_domain_protocols(AF_INET, SOCK_DGRAM, IPPROTO_ICMP,
				"network/protocols/l2cap/v1",
				NULL);

			sStackModule->register_domain_receiving_protocol(AF_INET, IPPROTO_ICMP,
				"network/protocols/l2cap/v1");
			return B_OK;
		}

		case B_MODULE_UNINIT:
			return B_OK;

		default:
			return B_ERROR;
	}
}


net_protocol_module_info sL2CAPModule = {
	{
		"network/protocols/l2cap/v1",
		0,
		l2cap_std_ops
	},
	NET_PROTOCOL_ATOMIC_MESSAGES,

	l2cap_init_protocol,
	l2cap_uninit_protocol,
	l2cap_open,
	l2cap_close,
	l2cap_free,
	l2cap_connect,
	l2cap_accept,
	l2cap_control,
	l2cap_getsockopt,
	l2cap_setsockopt,
	l2cap_bind,
	l2cap_unbind,
	l2cap_listen,
	l2cap_shutdown,
	l2cap_send_data,
	l2cap_send_routed_data,
	l2cap_send_avail,
	l2cap_read_data,
	l2cap_read_avail,
	l2cap_get_domain,
	l2cap_get_mtu,
	l2cap_receive_data,
	NULL,
	l2cap_error,
	l2cap_error_reply,
};

module_dependency module_dependencies[] = {
	{NET_STACK_MODULE_NAME, (module_info **)&sStackModule},
	{NET_BUFFER_MODULE_NAME, (module_info **)&gBufferModule},
	{}
};

module_info *modules[] = {
	(module_info *)&sL2CAPModule,
	NULL
};
