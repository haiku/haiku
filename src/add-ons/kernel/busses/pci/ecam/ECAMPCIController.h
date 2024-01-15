/*
 * Copyright 2022, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 */


#ifndef _ECAM_PCI_CONTROLLER_H_
#define _ECAM_PCI_CONTROLLER_H_

#include <bus/PCI.h>
#include <bus/FDT.h>
#include <ACPI.h>

#include <AutoDeleterOS.h>
#include <lock.h>
#include <util/Vector.h>


#define CHECK_RET(err) {status_t _err = (err); if (_err < B_OK) return _err;}

#define ECAM_PCI_DRIVER_MODULE_NAME "busses/pci/ecam/driver_v1"


enum {
	fdtPciRangeConfig      = 0x00000000,
	fdtPciRangeIoPort      = 0x01000000,
	fdtPciRangeMmio32Bit   = 0x02000000,
	fdtPciRangeMmio64Bit   = 0x03000000,
	fdtPciRangeTypeMask    = 0x03000000,
	fdtPciRangeAliased     = 0x20000000,
	fdtPciRangePrefechable = 0x40000000,
	fdtPciRangeRelocatable = 0x80000000,
};


enum PciBarKind {
	kRegIo,
	kRegMmio32,
	kRegMmio64,
	kRegMmio1MB,
	kRegUnknown,
};


union PciAddress {
	struct {
		uint32 offset: 8;
		uint32 function: 3;
		uint32 device: 5;
		uint32 bus: 8;
		uint32 unused: 8;
	};
	uint32 val;
};

union PciAddressEcam {
	struct {
		uint32 offset: 12;
		uint32 function: 3;
		uint32 device: 5;
		uint32 bus: 8;
		uint32 unused: 4;
	};
	uint32 val;
};

struct RegisterRange {
	phys_addr_t parentBase;
	phys_addr_t childBase;
	uint64 size;
};

struct InterruptMapMask {
	uint32_t childAdr;
	uint32_t childIrq;
};

struct InterruptMap {
	uint32_t childAdr;
	uint32_t childIrq;
	uint32_t parentIrqCtrl;
	uint32_t parentIrq;
};


class ECAMPCIController {
public:
	virtual ~ECAMPCIController() = default;

	static float SupportsDevice(device_node* parent);
	static status_t RegisterDevice(device_node* parent);
	static status_t InitDriver(device_node* node, ECAMPCIController*& outDriver);
	void UninitDriver();

	status_t ReadConfig(
				uint8 bus, uint8 device, uint8 function,
				uint16 offset, uint8 size, uint32 &value);

	status_t WriteConfig(
				uint8 bus, uint8 device, uint8 function,
				uint16 offset, uint8 size, uint32 value);

	status_t GetMaxBusDevices(int32& count);

	status_t ReadIrq(
				uint8 bus, uint8 device, uint8 function,
				uint8 pin, uint8& irq);

	status_t WriteIrq(
				uint8 bus, uint8 device, uint8 function,
				uint8 pin, uint8 irq);

	status_t GetRange(uint32 index, pci_resource_range* range);

	virtual status_t Finalize() = 0;

private:
	inline addr_t ConfigAddress(uint8 bus, uint8 device, uint8 function, uint16 offset);

protected:
	virtual status_t ReadResourceInfo() = 0;

protected:
	struct mutex fLock = MUTEX_INITIALIZER("ECAM PCI");

	device_node* fNode{};

	AreaDeleter fRegsArea;
	uint8 volatile* fRegs{};
	uint64 fRegsLen{};

	Vector<pci_resource_range> fResourceRanges;
};


class ECAMPCIControllerACPI: public ECAMPCIController {
public:
	~ECAMPCIControllerACPI() = default;

	status_t Finalize() final;

protected:
	status_t ReadResourceInfo() final;
	status_t ReadResourceInfo(device_node* parent);

	uint8 fStartBusNumber{};
	uint8 fEndBusNumber{};

private:
	friend class X86PCIControllerMethPcie;

	static acpi_status AcpiCrsScanCallback(acpi_resource *res, void *context);
	inline acpi_status AcpiCrsScanCallbackInt(acpi_resource *res);
};


class ECAMPCIControllerFDT: public ECAMPCIController {
public:
	~ECAMPCIControllerFDT() = default;

	status_t Finalize() final;

protected:
	status_t ReadResourceInfo() final;

private:
	static void FinalizeInterrupts(fdt_device_module_info* fdtModule,
		struct fdt_interrupt_map* interruptMap, int bus, int device, int function);
};


extern device_manager_info* gDeviceManager;
extern pci_module_info* gPCI;

#endif	// _ECAM_PCI_CONTROLLER_H_
