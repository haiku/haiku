/*
 * Copyright 2006-2015, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Axel Dörfler, axeld@pinc-software.de
 */
#ifndef SETTINGS_H
#define SETTINGS_H


#include <vector>

#include <Message.h>
#include <Messenger.h>
#include <NetworkAddress.h>
#include <Path.h>
#include <StringList.h>


namespace BNetworkKit {


class BNetworkInterfaceSettings;
class BNetworkServiceSettings;


class BNetworkSettings {
public:
	static	const uint32		kMsgInterfaceSettingsUpdated = 'SUif';
	static	const uint32		kMsgNetworkSettingsUpdated = 'SUnw';
	static	const uint32		kMsgServiceSettingsUpdated = 'SUsv';

public:
								BNetworkSettings();
								~BNetworkSettings();

			status_t			GetNextInterface(uint32& cookie,
									BMessage& interface);
			status_t			GetInterface(const char* name,
									BMessage& interface) const;
			status_t			AddInterface(const BMessage& interface);
			status_t			RemoveInterface(const char* name);
			BNetworkInterfaceSettings
								Interface(const char* name);
			const BNetworkInterfaceSettings
								Interface(const char* name) const;

			int32				CountNetworks() const;
			status_t			GetNextNetwork(uint32& cookie,
									BMessage& network) const;
			status_t			GetNetwork(const char* name,
									BMessage& network) const;
			status_t			AddNetwork(const BMessage& network);
			status_t			RemoveNetwork(const char* name);

			const BMessage&		Services() const;
			status_t			GetNextService(uint32& cookie,
									BMessage& service);
			status_t			GetService(const char* name,
									BMessage& service) const;
			status_t			AddService(const BMessage& service);
			status_t			RemoveService(const char* name);
			BNetworkServiceSettings
								Service(const char* name);
			const BNetworkServiceSettings
								Service(const char* name) const;

			status_t			StartMonitoring(const BMessenger& target);
			status_t			StopMonitoring(const BMessenger& target);

			status_t			Update(BMessage* message);

private:
			status_t			_Load(const char* name = NULL,
									uint32* _type = NULL);
			status_t			_Save(const char* name = NULL);
			BPath				_Path(BPath& parent, const char* name);
			status_t			_GetPath(const char* name, BPath& path);

			status_t			_StartWatching(const char* name,
									const BMessenger& target);

			bool				_IsWatching(const BMessenger& target) const
									{ return fListener == target; }
			bool				_IsWatching() const
									{ return fListener.IsValid(); }

			status_t			_ConvertNetworkToSettings(BMessage& message);
			status_t			_ConvertNetworkFromSettings(BMessage& message);

			status_t			_GetItem(const BMessage& container,
									const char* itemField,
									const char* nameField, const char* name,
									int32& _index, BMessage& item) const;
			status_t			_RemoveItem(BMessage& container,
									const char* itemField,
									const char* nameField, const char* name,
									const char* store = NULL);

private:
			BMessenger			fListener;
			BMessage			fInterfaces;
			BMessage			fNetworks;
			BMessage			fServices;
};


class BNetworkInterfaceAddressSettings {
public:
								BNetworkInterfaceAddressSettings();
								BNetworkInterfaceAddressSettings(
									const BMessage& data);
								BNetworkInterfaceAddressSettings(
									const BNetworkInterfaceAddressSettings&
										other);
								~BNetworkInterfaceAddressSettings();

			int					Family() const;
			void				SetFamily(int family);
			bool				IsAutoConfigure() const;
			void				SetAutoConfigure(bool configure);

			const BNetworkAddress&
								Address() const;
			BNetworkAddress&	Address();
			const BNetworkAddress&
								Mask() const;
			BNetworkAddress&	Mask();
			const BNetworkAddress&
								Peer() const;
			BNetworkAddress&	Peer();
			const BNetworkAddress&
								Broadcast() const;
			BNetworkAddress&	Broadcast();
			const BNetworkAddress&
								Gateway() const;
			BNetworkAddress&	Gateway();

			status_t			GetMessage(BMessage& data) const;

			BNetworkInterfaceAddressSettings&
								operator=(
									const BNetworkInterfaceAddressSettings&
										other);

private:
			int32				fFamily;
			bool				fAutoConfigure;
			BNetworkAddress		fAddress;
			BNetworkAddress		fMask;
			BNetworkAddress		fPeer;
			BNetworkAddress		fBroadcast;
			BNetworkAddress		fGateway;
};


class BNetworkInterfaceSettings {
public:
								BNetworkInterfaceSettings();
								BNetworkInterfaceSettings(
									const BMessage& message);
								~BNetworkInterfaceSettings();

			const char*			Name() const;
			void				SetName(const char* name);

			int32				Flags() const;
			void				SetFlags(int32 flags);
			int32				MTU() const;
			void				SetMTU(int32 mtu);
			int32				Metric() const;
			void				SetMetric(int32 metric);

			int32				CountAddresses() const;
			const BNetworkInterfaceAddressSettings&
								AddressAt(int32 index) const;
			BNetworkInterfaceAddressSettings&
								AddressAt(int32 index);
			int32				FindFirstAddress(int family) const;
			void				AddAddress(const
									BNetworkInterfaceAddressSettings& address);
			void				RemoveAddress(int32 index);

			bool				IsAutoConfigure(int family) const;

			status_t			GetMessage(BMessage& data) const;

private:
			BString				fName;
			int32				fFlags;
			int32				fMTU;
			int32				fMetric;
			std::vector<BNetworkInterfaceAddressSettings>
								fAddresses;
};


class BNetworkServiceAddressSettings {
public:
								BNetworkServiceAddressSettings();
								BNetworkServiceAddressSettings(
									const BMessage& data, int family = -1,
									int type = -1, int protocol = -1,
									int port = -1);
								~BNetworkServiceAddressSettings();

			int					Family() const;
			void				SetFamily(int family);
			int					Protocol() const;
			void				SetProtocol(int protocol);
			int					Type() const;
			void				SetType(int type);

			const BNetworkAddress&
								Address() const;
			BNetworkAddress&	Address();

			status_t			GetMessage(BMessage& data) const;

			bool				operator==(
									const BNetworkServiceAddressSettings& other)
										const;

private:
			int32				fFamily;
			int					fProtocol;
			int					fType;
			BNetworkAddress		fAddress;
};


class BNetworkServiceSettings {
public:
								BNetworkServiceSettings();
								BNetworkServiceSettings(
									const BMessage& message);
								~BNetworkServiceSettings();

			status_t			InitCheck() const;

			const char*			Name() const;
			void				SetName(const char* name);
			bool				IsStandAlone() const;
			void				SetStandAlone(bool alone);
			bool				IsEnabled() const;
			void				SetEnabled(bool enable);

			int					Family() const;
			void				SetFamily(int family);
			int					Protocol() const;
			void				SetProtocol(int protocol);
			int					Type() const;
			void				SetType(int type);
			int					Port() const;
			void				SetPort(int port);

			int32				CountArguments() const;
			const char*			ArgumentAt(int32 index) const;
			void				AddArgument(const char* argument);
			void				RemoveArgument(int32 index);

			int32				CountAddresses() const;
			const BNetworkServiceAddressSettings&
								AddressAt(int32 index) const;
			void				AddAddress(const
									BNetworkServiceAddressSettings& address);
			void				RemoveAddress(int32 index);

			bool				IsRunning() const;

			status_t			GetMessage(BMessage& data) const;

private:
			BString				fName;
			int32				fFamily;
			int32				fType;
			int32				fProtocol;
			int32				fPort;
			bool				fEnabled;
			bool				fStandAlone;
			BStringList			fArguments;
			std::vector<BNetworkServiceAddressSettings>
								fAddresses;
};


}	// namespace BNetworkKit


#endif	// SETTINGS_H
