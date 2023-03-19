/*
 * Copyright 2022, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 */


#ifndef _X86PCICONTROLLER_H_
#define _X86PCICONTROLLER_H_

#include <bus/PCI.h>

#include <AutoDeleterOS.h>
#include <lock.h>


#define CHECK_RET(err) {status_t _err = (err); if (_err < B_OK) return _err;}

#define PCI_X86_DRIVER_MODULE_NAME "busses/pci/x86/driver_v1"


class X86PCIController {
public:
	virtual ~X86PCIController() = default;

	static float SupportsDevice(device_node* parent);
	static status_t RegisterDevice(device_node* parent);
	static status_t InitDriver(device_node* node, X86PCIController*& outDriver);
	void UninitDriver();

	virtual status_t ReadConfig(
		uint8 bus, uint8 device, uint8 function,
		uint16 offset, uint8 size, uint32 &value) = 0;

	virtual status_t WriteConfig(
		uint8 bus, uint8 device, uint8 function,
		uint16 offset, uint8 size, uint32 value) = 0;

	virtual status_t GetMaxBusDevices(int32& count) = 0;

	status_t ReadIrq(
		uint8 bus, uint8 device, uint8 function,
		uint8 pin, uint8& irq);

	status_t WriteIrq(
		uint8 bus, uint8 device, uint8 function,
		uint8 pin, uint8 irq);

	status_t GetRange(uint32 index, pci_resource_range* range);

protected:
	static status_t CreateDriver(device_node* node, X86PCIController* driver,
		X86PCIController*& driverOut);
	virtual status_t InitDriverInt(device_node* node);

protected:
	spinlock fLock = B_SPINLOCK_INITIALIZER;

	device_node* fNode{};

	addr_t fPCIeBase{};
	uint8 fStartBusNumber{};
	uint8 fEndBusNumber{};
};


class X86PCIControllerMeth1: public X86PCIController {
public:
	virtual ~X86PCIControllerMeth1() = default;

	status_t InitDriverInt(device_node* node) override;

	status_t ReadConfig(
		uint8 bus, uint8 device, uint8 function,
		uint16 offset, uint8 size, uint32 &value) override;

	status_t WriteConfig(
		uint8 bus, uint8 device, uint8 function,
		uint16 offset, uint8 size, uint32 value) override;

	status_t GetMaxBusDevices(int32& count) override;
};


class X86PCIControllerMeth2: public X86PCIController {
public:
	virtual ~X86PCIControllerMeth2() = default;

	status_t InitDriverInt(device_node* node) final;

	status_t ReadConfig(
		uint8 bus, uint8 device, uint8 function,
		uint16 offset, uint8 size, uint32 &value) final;

	status_t WriteConfig(
		uint8 bus, uint8 device, uint8 function,
		uint16 offset, uint8 size, uint32 value) final;

	status_t GetMaxBusDevices(int32& count) final;
};


class X86PCIControllerMethPcie: public X86PCIControllerMeth1 {
public:
	virtual ~X86PCIControllerMethPcie() = default;

	status_t InitDriverInt(device_node* node) final;

	status_t ReadConfig(
		uint8 bus, uint8 device, uint8 function,
		uint16 offset, uint8 size, uint32 &value) final;

	status_t WriteConfig(
		uint8 bus, uint8 device, uint8 function,
		uint16 offset, uint8 size, uint32 value) final;

	status_t GetMaxBusDevices(int32& count) final;
};


extern device_manager_info* gDeviceManager;

#endif	// _X86PCICONTROLLER_H_
