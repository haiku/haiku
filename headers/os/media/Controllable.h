/*******************************************************************************
/
/	File:			Controllable.h
/
/   Description:  A BControllable is a BMediaNode with "tweakable" parameters/settings.
/
/	Copyright 1997-98, Be Incorporated, All Rights Reserved
/
*******************************************************************************/

#if !defined(_CONTROLLABLE_H)
#define _CONTROLLABLE_H

#include <MediaDefs.h>
#include <MediaNode.h>

class BParameterWeb;


class BControllable :
	public virtual BMediaNode
{
protected:
				/* Need this to force vtable */
virtual	~BControllable();

public:

		/* BControllable might seem somewhat spartan. */

		/* That is because control change requests and notifications */
		/* typically come in/go out in B_MEDIA_PARAMETERS type buffers */
		/* (and a BControllable thus also needs to be a BBufferConsumer */
		/* and/or a BBufferProducer) */
		/* The format of these buffers is: */
		/* media_node(node), int32(count) */
		/* repeat(count) { int64(when), int32(control_id), int32(value_size), <value> } */

		BParameterWeb * Web();
		bool LockParameterWeb();
		void UnlockParameterWeb();

protected:

		BControllable();	/* call SetParameterWeb() from your constructor */
		status_t SetParameterWeb(
				BParameterWeb * web);

virtual	status_t HandleMessage(
				int32 message,
				const void * data,
				size_t size);

		/* Call when the actual control changes, NOT when the value changes. */
		/* A typical case would be a CD with a Selector for Track when a new CD is inserted */
		status_t BroadcastChangedParameter(
				int32 id);

		/* Call this function when a value change takes effect, and */
		/* you want people who are interested to stay in sync with you. */
		/* Don't call this too densely, though, or you will flood the system */
		/* with messages. */
		status_t BroadcastNewParameterValue(
				bigtime_t when, 				//	performance time
				int32 id,						//	parameter ID
				void * newValue,
				size_t valueSize);

		/* These are alternate methods of accomplishing the same thing as */
		/* connecting to control information source/destinations would. */
virtual	status_t GetParameterValue(
				int32 id,
				bigtime_t * last_change,
				void * value,
				size_t * ioSize) = 0;
virtual	void SetParameterValue(
				int32 id,
				bigtime_t when,
				const void * value,
				size_t size) = 0;

		/* The default implementation of StartControlPanel launches the add-on */
		/* as an application (if the Node lives in an add-on). Thus, you can write your */
		/* control panel as a "main()" in your add-on, and it'll automagically work! */
		/* Your add-on needs to have multi-launch app flags for this to work right. */
		/* The first argv argument to main() will be a string of the format "node=%d" */
		/* with the node ID in question as "%d". */
virtual	status_t StartControlPanel(
				BMessenger * out_messenger);

		/* Call this from your BufferReceived() for control information buffers */
		/* if you implement BBufferConsumer for that format (recommended!) */
		status_t ApplyParameterData(
				const void * value,
				size_t size);
		/* If you want to generate control information for a set of controls, you */
		/* can use this utility function. */
		status_t MakeParameterData(
				const int32 * controls,
				int32 count,
				void * buf,
				size_t * ioSize);

private:

	friend class BMediaNode;

		BControllable(		/* private unimplemented */
				const BControllable & clone);
		BControllable & operator=(
				const BControllable & clone);

		/* Mmmh, stuffing! */
virtual		status_t _Reserved_Controllable_0(void *);
virtual		status_t _Reserved_Controllable_1(void *);
virtual		status_t _Reserved_Controllable_2(void *);
virtual		status_t _Reserved_Controllable_3(void *);
virtual		status_t _Reserved_Controllable_4(void *);
virtual		status_t _Reserved_Controllable_5(void *);
virtual		status_t _Reserved_Controllable_6(void *);
virtual		status_t _Reserved_Controllable_7(void *);
virtual		status_t _Reserved_Controllable_8(void *);
virtual		status_t _Reserved_Controllable_9(void *);
virtual		status_t _Reserved_Controllable_10(void *);
virtual		status_t _Reserved_Controllable_11(void *);
virtual		status_t _Reserved_Controllable_12(void *);
virtual		status_t _Reserved_Controllable_13(void *);
virtual		status_t _Reserved_Controllable_14(void *);
virtual		status_t _Reserved_Controllable_15(void *);

		BParameterWeb * _mWeb;
		sem_id _m_webSem;
		int32 _m_webBen;
		uint32 _reserved_controllable_[14];
};


#endif /* _CONTROLLABLE_H */

