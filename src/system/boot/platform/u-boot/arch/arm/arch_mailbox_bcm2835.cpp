/*
 * Copyright 2012 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Michael Lotz, mmlr@mlotz.ch
 *		François Revol, revol@free.fr
 *		Alexander von Gluck IV, kallisti5@unixzen.com
 */


#include "arch_mailbox.h"

#include <atomic>
#include <arch/arm/bcm283X.h>

#include "arch_cpu.h"


class ArchMailboxArmBCM2835 final : public ArchMailbox {
public:
							ArchMailboxArmBCM2835(addr_t base)
								:
								ArchMailbox(base) {}
							~ArchMailboxArmBCM2835() {}

virtual status_t			Write(uint8 channel, uint32 value) override;
virtual status_t			Read(uint8 channel, uint32& value) override;

private:
		auto&				GetRegister(unsigned reg);
		void				RegisterWrite(addr_t reg, uint32 value);
		uint32				RegisterRead(addr_t reg);
};


extern "C" ArchMailbox*
arch_get_mailbox_arm_bcm2835(addr_t base)
{
	return new ArchMailboxArmBCM2835(base);
}


status_t
ArchMailboxArmBCM2835::Write(uint8 channel, uint32 value)
{
	// We have to wait for the mailbox to drain if it is marked full.
	while ((RegisterRead(ARM_MAILBOX_STATUS) & ARM_MAILBOX_FULL) != 0)
		;

	value &= ARM_MAILBOX_DATA_MASK;
	RegisterWrite(ARM_MAILBOX_WRITE, value | channel);
	return B_OK;
}


status_t
ArchMailboxArmBCM2835::Read(uint8 channel, uint32& value)
{
	while (true) {
		// Wait for something to arrive in the mailbox.
		if ((RegisterRead(ARM_MAILBOX_STATUS) & ARM_MAILBOX_EMPTY) != 0)
			continue;

		value = RegisterRead(ARM_MAILBOX_READ);
		if ((value & ARM_MAILBOX_CHANNEL_MASK) != channel) {
			// Not for us, retry.
			continue;
		}

		break;
	}

	value &= ARM_MAILBOX_DATA_MASK;
	return B_OK;
}


inline auto&
ArchMailboxArmBCM2835::GetRegister(unsigned reg)
{
	auto addr = fBase + reg;
	return *reinterpret_cast<std::atomic<uint32_t>*>(addr);
}


inline uint32
ArchMailboxArmBCM2835::RegisterRead(addr_t reg)
{
	return GetRegister(reg).load(std::memory_order_acquire);
}


inline void
ArchMailboxArmBCM2835::RegisterWrite(addr_t reg, uint32 value)
{
	GetRegister(reg).store(value, std::memory_order_release);
}
