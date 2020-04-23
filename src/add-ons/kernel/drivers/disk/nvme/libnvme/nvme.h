/*-
 *   BSD LICENSE
 *
 *   Copyright (c) Intel Corporation. All rights reserved.
 *   Copyright (c) 2017, Western Digital Corporation or its affiliates.
 *
 *   Redistribution and use in source and binary forms, with or without
 *   modification, are permitted provided that the following conditions
 *   are met:
 *
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in
 *       the documentation and/or other materials provided with the
 *       distribution.
 *     * Neither the name of Intel Corporation nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 *   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *   "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *   LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 *   A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 *   OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 *   SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 *   LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 *   DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 *   THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 *   (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 *   OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/**
 * @file
 * NVMe driver public API
 *
 * @mainpage
 *
 * libnvme is a user space utility to provide control over NVMe,
 * the host controller interface for drives based on PCI Express.
 *
 * \addtogroup libnvme
 *  @{
 */

#ifndef __LIBNVME_H__
#define __LIBNVME_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <libnvme/nvme_spec.h>

#ifndef __HAIKU__
#include <pciaccess.h>
#endif
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

/**
 * Log levels.
 */
enum nvme_log_level {

	/**
	 * Disable all log messages.
	 */
	NVME_LOG_NONE = 0,

	/**
	 * System is unusable.
	 */
	NVME_LOG_EMERG,

	/**
	 * Action must be taken immediately.
	 */
	NVME_LOG_ALERT,

	/**
	 * Critical conditions.
	 */
	NVME_LOG_CRIT,

	/**
	 * Error conditions.
	 */
	NVME_LOG_ERR,

	/**
	 * Warning conditions.
	 */
	NVME_LOG_WARNING,

	/**
	 * Normal but significant condition.
	 */
	NVME_LOG_NOTICE,

	/**
	 * Informational messages.
	 */
	NVME_LOG_INFO,

	/**
	 * Debug-level messages.
	 */
	NVME_LOG_DEBUG,

};

/**
 * Log facilities.
 */
enum nvme_log_facility {

	/**
	 * Standard output log facility
	 */
	NVME_LOG_STDOUT = 0x00000001,

	/**
	 * Regular file output log facility
	 */
	NVME_LOG_FILE = 0x00000002,

	/**
	 * syslog service output log facility
	 */
	NVME_LOG_SYSLOG = 0x00000004,

};

/**
 * @brief Initialize libnvme
 *
 * @param level	Library log level
 * @param facility	Facility code
 * @param path		File name for the NVME_LOG_FILE facility
 *
 * This function must always be called first before any other
 * function provided by libnvme. The arguments allow setting the
 * initial log level and log facility so that any problem during
 * initialization can be caught.
 *
 * @return 0 on success and a negative error code on failure.
 */
extern int nvme_lib_init(enum nvme_log_level level,
			 enum nvme_log_facility facility, const char *path);

/**
 * @brief Set the library log level
 *
 * @param level	Library log level
 */
extern void nvme_set_log_level(enum nvme_log_level level);

/**
 * @brief Get the current log level
 *
 * @return The current library log level.
 */
extern enum nvme_log_level nvme_get_log_level(void);

/**
 * @brief Change the library log facility
 *
 * @param facility	Facility code
 * @param path		File name for the NVME_LOG_FILE facility
 *
 * Set th library log facility. On failure, the facility is
 * always automatically set to stdout.
 *
 * @return 0 on success and a negative error code on failure.
 */
extern int nvme_set_log_facility(enum nvme_log_facility facility,
				 const char *path);

/**
 * @brief Get the current library log facility.
 *
 * @return The current library log facility.
 */
extern enum nvme_log_facility nvme_get_log_facility(void);

/**
 * @brief Opaque handle to a controller returned by nvme_ctrlr_open().
 */
struct nvme_ctrlr;

/**
 * @brief Opaque handle to a namespace
 */
struct nvme_ns;

/**
 * @brief Opaque handle to an I/O queue pair
 */
struct nvme_qpair;

/**
 * @brief Capabilities register of a controller
 */
struct nvme_register_data {

	/**
	 * Maximum Queue Entries Supported indicates the maximum individual
	 * queue size that the controller supports. This is a 0’s based value,
	 * so 1 has to be added.
	 */
	unsigned int		mqes;

};

/**
 * Length of the string for the serial number
 */
#define NVME_SERIAL_NUMBER_LENGTH	NVME_SERIAL_NUMBER_CHARACTERS + 1

/**
 * Length of the string for the model number
 */
#define NVME_MODEL_NUMBER_LENGTH	NVME_MODEL_NUMBER_CHARACTERS + 1

/**
 * @brief Controller information
 */
struct nvme_ctrlr_stat {

	/**
	 * PCI device vendor ID.
	 */
	unsigned short		vendor_id;

	/**
	 * PCI device ID.
	 */
	unsigned short		device_id;

	/**
	 * PCI device sub-vendor ID.
	 */
	unsigned short		subvendor_id;

	/**
	 * PCI sub-device ID.
	 */
	unsigned short		subdevice_id;

	/**
	 * PCI device class.
	 */
	unsigned int		device_class;

	/**
	 * PCI device revision.
	 */
	unsigned char		revision;

	/**
	 * PCI slot domain.
	 */
	unsigned int		domain;

	/**
	 * PCI slot bus.
	 */
	unsigned int		bus;

	/**
	 * PCI slot bus device number.
	 */
	unsigned int		dev;

	/**
	 * PCI slot device function.
	 */
	unsigned int		func;

	/**
	 * Serial number
	 */
	char			sn[NVME_SERIAL_NUMBER_LENGTH];

	/**
	 * Model number
	 */
	char			mn[NVME_MODEL_NUMBER_LENGTH];

	/**
	 * Maximum transfer size.
	 */
	size_t			max_xfer_size;

	/**
	 * All the log pages supported.
	 */
	bool			log_pages[256];

	/**
	 * Whether SGL is supported by the controller.
	 *
	 * Note that this does not mean all SGL requests will fail;
	 * many are convertible into standard (PRP) requests by libnvme.
	 */
	bool			sgl_supported;

	/**
	 * All the features supported.
	 */
	bool			features[256];

	/**
	 * Number of valid namespaces in the array of namespace IDs.
	 */
	unsigned int		nr_ns;

	/**
	 * Array of valid namespace IDs of the controller.
	 * Namspeace IDs are integers between 1 and NVME_MAX_NS
	 */
	unsigned int		ns_ids[NVME_MAX_NS];

	/**
	 * Maximum number of I/O queue pairs
	 */
	unsigned int		max_io_qpairs;

	/**
	 * Number of I/O queue pairs allocated
	 */
	unsigned int		io_qpairs;

	/**
	 * Number of I/O queue pairs enabled
	 */
	unsigned int		enabled_io_qpairs;

	/**
	 * IO qpairs maximum entries
	 */
	unsigned int		max_qd;
};

/**
 * @brief NVMe controller options
 *
 * Allow the user to request non-default options.
 */
struct nvme_ctrlr_opts {

	/**
	 * Number of I/O queues to initialize.
	 * (default: all possible I/O queues)
	 */
	unsigned int		io_queues;

	/**
	 * Enable submission queue in controller memory buffer
	 * (default: false)
	 */
	bool 			use_cmb_sqs;

	/**
	 * Type of arbitration mechanism.
	 * (default: round-robin == NVME_CC_AMS_RR)
	 */
	enum nvme_cc_ams	arb_mechanism;

};

/**
 * @brief Namespace command support flags
 */
enum nvme_ns_flags {

	/**
	 * The deallocate command is supported.
	 */
	NVME_NS_DEALLOCATE_SUPPORTED	= 0x1,

	/**
	 * The flush command is supported.
	 */
	NVME_NS_FLUSH_SUPPORTED		= 0x2,

	/**
	 * The reservation command is supported.
	 */
	NVME_NS_RESERVATION_SUPPORTED	= 0x4,

	/**
	 * The write zeroes command is supported.
	 */
	NVME_NS_WRITE_ZEROES_SUPPORTED	= 0x8,

	/**
	 * The end-to-end data protection is supported.
	 */
	NVME_NS_DPS_PI_SUPPORTED	= 0x10,

	/**
	 * The extended lba format is supported, metadata is transferred as
	 * a contiguous part of the logical block that it is associated with.
	 */
	NVME_NS_EXTENDED_LBA_SUPPORTED	= 0x20,

};

/**
 * @brief Namespace information
 */
struct nvme_ns_stat {

	/**
	 * Namespace ID.
	 */
	unsigned int			id;

	/**
	 * Namespace command support flags.
	 */
	enum nvme_ns_flags		flags;

	/**
	 * Namespace sector size in bytes.
	 */
	size_t				sector_size;

	/**
	 * Namespace number of sectors.
	 */
	uint64_t			sectors;

	/**
	 * Namespace metadata size in bytes.
	 */
	size_t				md_size;

	/**
	 * Namespace priority information type.
	 */
	enum nvme_pi_type		pi_type;

};

/**
 * @brief Queue pair information
 */
struct nvme_qpair_stat {

	/**
	 * Qpair ID
	 */
	unsigned int		id;

	/**
	 * Qpair number of entries
	 */
	unsigned int		qd;

	/**
	 * Qpair is enabled
	 */
	bool			enabled;

	/**
	 * Qpair priority
	 */
	unsigned int		qprio;
};

/**
 * @brief Command completion callback function signature
 *
 * @param cmd_cb_arg	Callback function input argument.
 * @param cpl_status	Contains the completion status.
 */
typedef void (*nvme_cmd_cb)(void *cmd_cb_arg,
			    const struct nvme_cpl *cpl_status);

/**
 * @brief Asynchronous error request completion callback
 *
 * @param aer_cb_arg	AER context set by nvme_register_aer_callback()
 * @param cpl_status	Completion status of the asynchronous event request
 */
typedef void (*nvme_aer_cb)(void *aer_cb_arg,
			    const struct nvme_cpl *cpl_status);

/**
 * @brief Restart SGL walk to the specified offset callback
 *
 * @param cb_arg	Value passed to nvme_readv/nvme_writev
 * @param offset	Offset in the SGL
 */
typedef void (*nvme_req_reset_sgl_cb)(void *cb_arg, uint32_t offset);

/**
 * @brief Get an SGL entry address and length and advance to the next entry
 *
 * @param cb_arg	Value passed to readv/writev
 * @param address	Physical address of this segment
 * @param length	Length of this physical segment
 *
 * Fill out address and length with the current SGL entry and advance
 * to the next entry for the next time the callback is invoked
 */
typedef int (*nvme_req_next_sge_cb)(void *cb_arg,
				    uint64_t *address, uint32_t *length);

/**
 * @brief Open an NVMe controller
 *
 * @param url	PCI device URL
 * @param opts	controller options
 *
 * Obtain a handle for an NVMe controller specified as a PCI device URL,
 * e.g. pci://[DDDD:]BB:DD.F. If called more than once for the same
 * controller, NULL is returned.
 * To stop using the the controller and release its associated resources,
 * call nvme_ctrlr_close() with the handle returned by this function.
 *
 * @return A handle to the controller on success and NULL on failure.
 */
struct pci_device {
	uint16_t  vendor_id;
	uint16_t  device_id;
	uint16_t  subvendor_id;
	uint16_t  subdevice_id;

	uint16_t domain;
	uint16_t bus;
	uint16_t dev;
	uint16_t func;

	void* pci_info;
};

extern struct nvme_ctrlr * nvme_ctrlr_open(struct pci_device *pdev,
					   struct nvme_ctrlr_opts *opts);

/**
 * @brief Close an open NVMe controller
 *
 * @param ctrlr	Controller handle
 *
 * This function should be called while no other threads
 * are actively using the controller.
 *
 * @return 0 on success and a negative error code on failure.
 */
extern int nvme_ctrlr_close(struct nvme_ctrlr *ctrlr);

/**
 * @brief Get controller capabilities and features
 *
 * @param ctrlr	Controller handle
 * @param cstat	Controller information
 *
 * @return 0 on success and a negative error code on failure.
 */
extern int nvme_ctrlr_stat(struct nvme_ctrlr *ctrlr,
			   struct nvme_ctrlr_stat *cstat);

/**
 * @brief Get controller data and some data from the capabilities register
 *
 * @param ctrlr	Controller handle
 * @param cdata	Controller data to fill
 * @param rdata	Capabilities register data to fill
 *
 * cdata and rdata are optional (NULL can be specified).
 *
 * @return 0 on success and a negative error code on failure.
 */
extern int nvme_ctrlr_data(struct nvme_ctrlr *ctrlr,
			   struct nvme_ctrlr_data *cdata,
			   struct nvme_register_data *rdata);

/**
 * @brief Get a specific feature of a controller
 *
 * @param ctrlr		Controller handle
 * @param sel		Feature selector
 * @param feature 	Feature identifier
 * @param cdw11 	Command word 11 (command dependent)
 * @param attributes	Features attributes
 *
 * This function is thread safe and can be called at any point while
 * the controller is attached.
 *
 * @return 0 on success and a negative error code on failure.
 *
 * See nvme_ctrlr_set_feature()
 */
extern int nvme_ctrlr_get_feature(struct nvme_ctrlr *ctrlr,
				  enum nvme_feat_sel sel,
				  enum nvme_feat feature,
				  uint32_t cdw11, uint32_t *attributes);

/**
 * @brief Set a specific feature of a controller
 *
 * @param ctrlr		Controller handle
 * @param save		Save feature across power cycles
 * @param feature 	Feature identifier
 * @param cdw11 	Command word 11 (feature dependent)
 * @param cdw12 	Command word 12 (feature dependent)
 * @param attributes	Features attributes
 *
 * This function is thread safe and can be called at any point while
 * the controller is attached to the NVMe driver.
 *
 * @return 0 on success and a negative error code on failure.
 *
 * See nvme_ctrlr_get_feature()
 */
extern int nvme_ctrlr_set_feature(struct nvme_ctrlr *ctrlr,
				  bool save, enum nvme_feat feature,
				  uint32_t cdw11, uint32_t cdw12,
				  uint32_t *attributes);

/**
 * @brief Attach the specified namespace to controllers
 *
 * @param ctrlr Controller handle to use for command submission
 * @param nsid	Namespace ID of the namespaces to attach
 * @param clist List of controllers as defined in the NVMe specification
 *
 * @return 0 on success and a negative error code on failure.
 */
extern int nvme_ctrlr_attach_ns(struct nvme_ctrlr *ctrlr, unsigned int nsid,
				struct nvme_ctrlr_list *clist);

/**
 * @brief Detach the specified namespace from controllers
 *
 * @param ctrlr Controller handle to use for command submission
 * @param nsid	Namespace ID of the namespaces to detach
 * @param clist List of controllers as defined in the NVMe specification
 *
 * @return 0 on success and a negative error code on failure.
 */
extern int nvme_ctrlr_detach_ns(struct nvme_ctrlr *ctrlr, unsigned int nsid,
				struct nvme_ctrlr_list *clist);

/**
 * @brief Create a namespace
 *
 * @param ctrlr 	Controller handle
 * @param nsdata	namespace data
 *
 * @return Namespace ID (>= 1) on success and 0 on failure.
 */
extern unsigned int nvme_ctrlr_create_ns(struct nvme_ctrlr *ctrlr,
					 struct nvme_ns_data *nsdata);

/**
 * @brief Delete a namespace
 *
 * @param ctrlr	Controller handle
 * @param nsid	ID of the namespace to delete
 *
 * @return 0 on success and a negative error code on failure.
 */
extern int nvme_ctrlr_delete_ns(struct nvme_ctrlr *ctrlr, unsigned int nsid);

/**
 * @brief Format media
 *
 * @param ctrlr		Controller handle
 * @param nsid		ID of the namespace to format
 * @param format	Format information
 *
 * This function requests a low-level format of the media.
 * If nsid is NVME_GLOBAL_NS_TAG, all namspaces attached to the contoller
 * are formatted.
 *
 * @return 0 on success and a negative error code on failure.
 */
extern int nvme_ctrlr_format_ns(struct nvme_ctrlr *ctrlr,
				unsigned int nsid, struct nvme_format *format);

/**
 * @brief Download a new firmware image
 *
 * @param ctrlr	Controller handle
 * @param fw	Firmware data buffer
 * @param size	Firmware buffer size
 * @param slot 	Firmware image slot to use
 *
 * @return 0 on success and a negative error code on failure.
 */
extern int nvme_ctrlr_update_firmware(struct nvme_ctrlr *ctrlr,
				      void *fw, size_t size, int slot);

/**
 * @brief Get an I/O queue pair
 *
 * @param ctrlr	Controller handle
 * @param qprio I/O queue pair priority for weighted round robin arbitration
 * @param qd 	I/O queue pair maximum submission queue depth
 *
 * A queue depth of 0 will result in the maximum hardware defined queue
 * depth being used. The use of a queue pair is not thread safe. Applications
 * must ensure mutual exclusion access to the queue pair during I/O processing.
 *
 * @return An I/O queue pair handle on success and NULL in case of failure.
 */
extern struct nvme_qpair * nvme_ioqp_get(struct nvme_ctrlr *ctrlr,
					 enum nvme_qprio qprio,
					 unsigned int qd);

/**
 * @brief Release an I/O queue pair
 *
 * @param qpair	I/O queue pair handle
 *
 * @return 0 on success and a negative error code on failure.
 */
extern int nvme_ioqp_release(struct nvme_qpair *qpair);

/**
 * @brief Get information on an I/O queue pair
 *
 * @param qpair		I/O queue pair handle
 * @param qpstat	I/O queue pair information to fill
 *
 * @return 0 on success and a negative error code on failure.
 */
extern int nvme_qpair_stat(struct nvme_qpair *qpair,
			   struct nvme_qpair_stat *qpstat);

/**
 * @brief Submit an NVMe command
 *
 * @param qpair		I/O qpair handle
 * @param cmd		Command to submit
 * @param buf		Payload buffer
 * @param len		Payload buffer length
 * @param cb_fn		Callback function
 * @param cb_arg	Argument for the call back function
 *
 * This is a low level interface for submitting I/O commands directly.
 * The validity of the command will not be checked.
 *
 * When constructing the nvme_command it is not necessary to fill out the PRP
 * list/SGL or the CID. The driver will handle both of those for you.
 *
 * @return 0 on success and a negative error code on failure.
 */
extern int nvme_ioqp_submit_cmd(struct nvme_qpair *qpair,
				struct nvme_cmd *cmd,
				void *buf, size_t len,
				nvme_cmd_cb cb_fn, void *cb_arg);

/**
 * @brief Process I/O command completions
 *
 * @param qpair			I/O queue pair handle
 * @param max_completions	Maximum number of completions to check
 *
 * This call is non-blocking, i.e. it only processes completions that are
 * ready at the time of this function call. It does not wait for
 * outstanding commands to complete.
 * For each completed command, the request callback function will
 * be called if specified as non-NULL when the request was submitted.
 * This function may be called at any point after the command submission
 * while the controller is open
 *
 * @return The number of completions processed (may be 0).
 *
 * @sa nvme_cmd_cb
 */
extern unsigned int nvme_qpair_poll(struct nvme_qpair *qpair,
				   unsigned int max_completions);

/**
 * @brief Open a name space
 *
 * @param ctrlr	Controller handle
 * @param ns_id	ID of the name space to open
 *
 * @return A namspace handle on success or NULL in case of failure.
 */
extern struct nvme_ns *nvme_ns_open(struct nvme_ctrlr *ctrlr,
				    unsigned int ns_id);

/**
 * @brief Close an open name space
 *
 * @param ns	Namspace handle
 *
 * See nvme_ns_open()
 */
extern int nvme_ns_close(struct nvme_ns *ns);

/**
 * @brief Get information on a namespace
 *
 * @param ns		Namespace handle
 * @param ns_stat	Namespace information
 *
 * @return 0 on success and a negative error code in case of failure.
 */
extern int nvme_ns_stat(struct nvme_ns *ns,
			struct nvme_ns_stat *ns_stat);

/**
 * @brief Get namespace data
 *
 * @param ns		Namespace handle
 * @param nsdata	Namespace data
 *
 * @return 0 on success and a negative error code in case of failure.
 */
extern int nvme_ns_data(struct nvme_ns *ns,
			struct nvme_ns_data *nsdata);

/**
 * @brief Submit a write I/O
 *
 * @param ns		Namespace handle
 * @param qpair		I/O queue pair handle
 * @param buffer	Physically contiguous data buffer
 * @param lba		Starting LBA to read from
 * @param lba_count	Number of LBAs to read
 * @param cb_fn		Completion callback
 * @param cb_arg	Argument to pass to the completion callback
 * @param io_flags	I/O flags (NVME_IO_FLAGS_*)
 *
 * @return 0 on success and a negative error code in case of failure.
 */
extern int nvme_ns_write(struct nvme_ns *ns, struct nvme_qpair *qpair,
			 void *buffer,
			 uint64_t lba, uint32_t lba_count,
			 nvme_cmd_cb cb_fn, void *cb_arg,
			 unsigned int io_flags);

/**
 * @brief Submit a scattered write I/O
 *
 * @param ns		Namespace handle
 * @param qpair		I/O queue pair handle
 * @param lba		Starting LBA to write to
 * @param lba_count	Number of LBAs to write
 * @param cb_fn		Completion callback
 * @param cb_arg	Argument to pass to the completion callback
 * @param io_flags	I/O flags (NVME_IO_FLAGS_*)
 * @param reset_sgl_fn	Reset scattered payload callback
 * @param next_sge_fn	Scattered payload iteration callback
 *
 * @return 0 on success and a negative error code in case of failure.
 */
extern int nvme_ns_writev(struct nvme_ns *ns, struct nvme_qpair *qpair,
			  uint64_t lba, uint32_t lba_count,
			  nvme_cmd_cb cb_fn, void *cb_arg,
			  unsigned int io_flags,
			  nvme_req_reset_sgl_cb reset_sgl_fn,
			  nvme_req_next_sge_cb next_sge_fn);

/**
 * @brief Submits a write I/O with metadata
 *
 * @param ns		Namespace handle
 * @param qpair		I/O queue pair handle
 * @param payload	Data buffer
 * @param metadata	Metadata payload
 * @param lba		Starting LBA to write to
 * @param lba_count	Number of LBAs to write
 * @param cb_fn		Completion callback
 * @param cb_arg	Argument to pass to the completion callback
 * @param io_flags	I/O flags (NVME_IO_FLAGS_*)
 * @param apptag_mask	Application tag mask
 * @param apptag	Application tag to use end-to-end protection information
 *
 * @return 0 on success and a negative error code in case of failure.
 */
extern int nvme_ns_write_with_md(struct nvme_ns *ns, struct nvme_qpair *qpair,
				 void *payload, void *metadata,
				 uint64_t lba, uint32_t lba_count,
				 nvme_cmd_cb cb_fn, void *cb_arg,
				 unsigned int io_flags,
				 uint16_t apptag_mask, uint16_t apptag);

/**
 * @brief Submit a write zeroes I/O
 *
 * @param ns		Namespace handle
 * @param qpair		I/O queue pair handle
 * @param lba		Starting LBA to write to
 * @param lba_count	Number of LBAs to write
 * @param cb_fn		Completion callback
 * @param cb_arg	Argument to pass to the completion callback
 * @param io_flags	I/O flags (NVME_IO_FLAGS_*)
 *
 * @return 0 on success and a negative error code in case of failure.
 */
extern int nvme_ns_write_zeroes(struct nvme_ns *ns, struct nvme_qpair *qpair,
				uint64_t lba, uint32_t lba_count,
				nvme_cmd_cb cb_fn, void *cb_arg,
				unsigned int io_flags);

/**
 * @brief Submit a read I/O
 *
 * @param ns		Namespace handle
 * @param qpair		I/O queue pair handle
 * @param buffer	Physically contiguous data buffer
 * @param lba		Starting LBA to read from
 * @param lba_count	Number of LBAs to read
 * @param cb_fn		Completion callback
 * @param cb_arg	Argument to pass to the completion callback
 * @param io_flags	I/O flags (NVME_IO_FLAGS_*)
 *
 * @return 0 on success and a negative error code in case of failure.
 */
extern int nvme_ns_read(struct nvme_ns *ns, struct nvme_qpair *qpair,
			void *buffer,
			uint64_t lba, uint32_t lba_count,
			nvme_cmd_cb cb_fn, void *cb_arg,
			unsigned int io_flags);

/**
 * @brief Submit a scattered read I/O
 *
 * @param ns		Namespace handle
 * @param qpair		I/O queue pair handle
 * @param lba		Starting LBA to read from
 * @param lba_count	Number of LBAs to read
 * @param cb_fn		Completion callback
 * @param cb_arg	Argument to pass to the completion callback
 * @param io_flags	I/O flags (NVME_IO_FLAGS_*)
 * @param reset_sgl_fn	Reset scattered payload callback
 * @param next_sge_fn	Scattered payload iteration callback
 *
 * @return 0 on success and a negative error code in case of failure.
 */
extern int nvme_ns_readv(struct nvme_ns *ns, struct nvme_qpair *qpair,
			 uint64_t lba, uint32_t lba_count,
			 nvme_cmd_cb cb_fn, void *cb_arg,
			 unsigned int io_flags,
			 nvme_req_reset_sgl_cb reset_sgl_fn,
			 nvme_req_next_sge_cb next_sge_fn);

/**
 * @brief Submit a read I/O with metadata
 *
 * @param ns		Namespace handle
 * @param qpair		I/O queue pair handle
 * @param buffer	Data buffer
 * @param metadata	Metadata payload
 * @param lba		Starting LBA to read from
 * @param lba_count	Number of LBAs to read
 * @param cb_fn		Completion callback
 * @param cb_arg	Argument to pass to the completion callback
 * @param io_flags	I/O flags (NVME_IO_FLAGS_*)
 * @param apptag_mask	Application tag mask
 * @param apptag	Application tag to use end-to-end protection information
 *
 * @return 0 on success and a negative error code in case of failure.
 */
extern int nvme_ns_read_with_md(struct nvme_ns *ns, struct nvme_qpair *qpair,
				void *buffer, void *metadata,
				uint64_t lba, uint32_t lba_count,
				nvme_cmd_cb cb_fn, void *cb_arg,
				unsigned int io_flags,
				uint16_t apptag_mask, uint16_t apptag);

/**
 * @brief Submit a deallocate command
 *
 * @param ns		Namespace handle
 * @param qpair		I/O queue pair handle
 * @param payload	List of LBA ranges to deallocate
 * @param num_ranges	Number of ranges in the list
 * @param cb_fn		Completion callback
 * @param cb_arg	Argument to pass to the completion callback
 *
 * The number of LBA ranges must be at least 1 and at most
 * NVME_DATASET_MANAGEMENT_MAX_RANGES.
 *
 * @return 0 on success and a negative error code in case of failure.
 */
extern int nvme_ns_deallocate(struct nvme_ns *ns, struct nvme_qpair *qpair,
			      void *payload, uint16_t num_ranges,
			      nvme_cmd_cb cb_fn, void *cb_arg);

/**
 * @brief Submit a flush command
 *
 * @param ns		Namespace handle
 * @param qpair		I/O queue pair handle
 * @param cb_fn		Completion callback
 * @param cb_arg	Argument to pass to the completion callback
 *
 * @return 0 on success and a negative error code in case of failure.
 */
extern int nvme_ns_flush(struct nvme_ns *ns, struct nvme_qpair *qpair,
			 nvme_cmd_cb cb_fn, void *cb_arg);

/**
 * @brief Submit a reservation register command
 *
 * @param ns		Namespace handle
 * @param qpair		I/O queue pair handle
 * @param payload	Reservation register data buffer
 * @param ignore_key	Enable or not the current reservation key check
 * @param action	Registration action
 * @param cptpl		Persist Through Power Loss state
 * @param cb_fn		Completion callback
 * @param cb_arg	Argument to pass to the completion callback
 *
 * @return 0 on success and a negative error code in case of failure.
 */
extern int nvme_ns_reservation_register(struct nvme_ns *ns,
				struct nvme_qpair *qpair,
				struct nvme_reservation_register_data *payload,
				bool ignore_key,
				enum nvme_reservation_register_action action,
				enum nvme_reservation_register_cptpl cptpl,
				nvme_cmd_cb cb_fn, void *cb_arg);

/**
 * @brief Submit a reservation release command
 *
 * @param ns		Namespace handle
 * @param qpair		I/O queue pair handle
 * @param payload	Current reservation key buffer
 * @param ignore_key	Enable or not the current reservation key check
 * @param action	Reservation release action
 * @param type		Reservation type
 * @param cb_fn		Completion callback
 * @param cb_arg	Argument to pass to the completion callback
 *
 * @return 0 on success and a negative error code in case of failure.
 */
extern int nvme_ns_reservation_release(struct nvme_ns *ns,
			       struct nvme_qpair *qpair,
			       struct nvme_reservation_key_data *payload,
			       bool ignore_key,
			       enum nvme_reservation_release_action action,
			       enum nvme_reservation_type type,
			       nvme_cmd_cb cb_fn, void *cb_arg);

/**
 * @brief Submit a reservation acquire command
 *
 * @param ns		Namespace handle
 * @param qpair		I/O queue pair handle
 * @param payload	Reservation acquire data buffer
 * @param ignore_key	Enable or not the current reservation key check
 * @param action	Reservation acquire action
 * @param type		Reservation type
 * @param cb_fn		Completion callback
 * @param cb_arg	Argument to pass to the completion callback
 *
 * @return 0 on success and a negative error code in case of failure.
 */
extern int nvme_ns_reservation_acquire(struct nvme_ns *ns,
				struct nvme_qpair *qpair,
				struct nvme_reservation_acquire_data *payload,
				bool ignore_key,
				enum nvme_reservation_acquire_action action,
				enum nvme_reservation_type type,
				nvme_cmd_cb cb_fn, void *cb_arg);

/**
 * @brief Submits a reservation report to a namespace
 *
 * @param ns		Namespace handle
 * @param qpair		I/O queue pair handle
 * @param payload	Reservation status data buffer
 * @param len		Length in bytes of the reservation status data
 * @param cb_fn		Completion callback
 * @param cb_arg	Argument to pass to the completion callback
 *
 * The command is submitted to a qpair allocated by nvme_ctrlr_alloc_io_qpair().
 * The user must ensure that only one thread submits I/O on
 * a given qpair at any given time.
 *
 * @return 0 on success and a negative error code in case of failure.
 */
extern int nvme_ns_reservation_report(struct nvme_ns *ns,
				      struct nvme_qpair *qpair,
				      void *payload, size_t len,
				      nvme_cmd_cb cb_fn, void *cb_arg);

/**
 * Any NUMA node.
 */
#define NVME_NODE_ID_ANY 	(~0U)

/**
 * @brief Allocate physically contiguous memory
 *
 * @param size 		Size (in bytes) to be allocated
 * @param align 	Memory alignment constraint
 * @param node_id	The NUMA node to get memory from or NVME_NODE_ID_ANY
 *
 * This function allocates memory from the hugepage area of memory. The
 * memory is not cleared. In NUMA systems, the memory allocated resides
 * on the requested NUMA node if node_id is not NVME_NODE_ID_ANY.
 * Otherwise, allocation will take preferrably on the node of the
 * function call context, or any other node if that fails.
 *
 * @return The address of the allocated memory on success and NULL on failure.
 */
extern void *nvme_malloc_node(size_t size, size_t align,
			      unsigned int node_id);

/**
 * @brief Allocate zero'ed memory
 *
 * @param size 		Size (in bytes) to be allocated
 * @param align 	Memory alignment constraint
 * @param node_id	The NUMA node to get memory from or NVME_NODE_ID_ANY
 *
 * See @nvme_malloc_node.
 */
static inline void *nvme_zmalloc_node(size_t size, size_t align,
				      unsigned int node_id)
{
	void *buf;

	buf = nvme_malloc_node(size, align, node_id);
	if (buf)
		memset(buf, 0, size);

	return buf;
}

/**
 * @brief Allocate zero'ed array memory
 *
 * @param num 		Size of the array
 * @param size 		Size (in bytes) of the array elements
 * @param align 	Memory alignment constraint
 * @param node_id	The NUMA node to get memory from or NVME_NODE_ID_ANY
 *
 * See @nvme_malloc_node.
 */
static inline void *nvme_calloc_node(size_t num, size_t size,
				     size_t align, unsigned int node_id)
{
	return nvme_zmalloc_node(size * num, align, node_id);
}

/**
 * @brief Allocate physically contiguous memory
 *
 * @param size 		Size (in bytes) to be allocated
 * @param align 	Memory alignment constraint
 *
 * @return The address of the allocated memory on success and NULL on error
 *
 * See @nvme_malloc_node.
 */
static inline void *nvme_malloc(size_t size, size_t align)
{
	return nvme_malloc_node(size, align, NVME_NODE_ID_ANY);
}

/**
 * @brief Allocate zero'ed memory
 *
 * @param size 		Size (in bytes) to be allocated
 * @param align 	Memory alignment constraint
 *
 * @return The address of the allocated memory on success and NULL on error
 *
 * See @nvme_zmalloc_node.
 */
static inline void *nvme_zmalloc(size_t size, size_t align)
{
	return nvme_zmalloc_node(size, align, NVME_NODE_ID_ANY);
}

/**
 * @brief Allocate zero'ed array memory
 *
 * @param num 		Size of the array
 * @param size 		Size (in bytes) of the array elements
 * @param align 	Memory alignment constraint
 *
 * See @nvme_calloc_node.
 */
static inline void *nvme_calloc(size_t num, size_t size, size_t align)
{
	return nvme_calloc_node(num, size, align, NVME_NODE_ID_ANY);
}

/**
 * @brief Free allocated memory
 *
 * @param addr	Address of the memory to free
 *
 * Free the memory at the specified address.
 * The address must be one that was returned by one of the
 * allocation function nvme_malloc_node(), nvme_zmalloc_node()
 * or nvme_calloc_node().
 *
 * If the pointer is NULL, the function does nothing.
 */
extern void nvme_free(void *addr);

/**
 * Structure to hold memory statistics.
 */
struct nvme_mem_stats {

	/**
	 * Number of huge pages allocated.
	 */
	size_t		nr_hugepages;

	/**
	 * Total bytes in memory pools.
	 */
	size_t		total_bytes;

	/**
	 * Total free bytes in memory pools.
	 */
	size_t		free_bytes;

};

/**
 * @brief Get memory usage information
 *
 * @param stats		Memory usage inforamtion structure to fill
 * @param node_id	NUMA node ID or NVME_NVME_NODE_ID_ANY
 *
 * Return memory usage statistics for the specified
 * NUMA node (CPU socket) or global memory usage if node_id
 * is NVME_NODE_ID_ANY.
 *
 * @return 0 on success and a negative error code on failure.
 */
extern int nvme_memstat(struct nvme_mem_stats *stats,
			unsigned int node_id);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* __LIBNVME_H__ */
