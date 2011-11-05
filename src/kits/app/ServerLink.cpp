/*
 * Copyright 2001-2009, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Pahtz <pahtz@yahoo.com.au>
 *		Axel Dörfler, axeld@pinc-software.de
 */


/*!	Class for low-overhead port-based messaging */


#include <ServerLink.h>

#include <stdlib.h>
#include <string.h>
#include <new>

#include <Gradient.h>
#include <GradientLinear.h>
#include <GradientRadial.h>
#include <GradientRadialFocus.h>
#include <GradientDiamond.h>
#include <GradientConic.h>
#include <Region.h>
#include <Shape.h>

#include <ServerProtocol.h>


//#define TRACE_SERVER_LINK_GRADIENTS
#ifdef TRACE_SERVER_LINK_GRADIENTS
#	include <OS.h>
#	define GTRACE(x) debug_printf x
#else
#	define GTRACE(x) ;
#endif


namespace BPrivate {


ServerLink::ServerLink()
{
}


ServerLink::~ServerLink()
{
}


void
ServerLink::SetTo(port_id sender, port_id receiver)
{
	fSender->SetPort(sender);
	fReceiver->SetPort(receiver);
}


status_t
ServerLink::ReadRegion(BRegion* region)
{
	fReceiver->Read(&region->fCount, sizeof(long));
	if (region->fCount > 0) {
		fReceiver->Read(&region->fBounds, sizeof(clipping_rect));
		if (!region->_SetSize(region->fCount))
			return B_NO_MEMORY;
		return fReceiver->Read(region->fData,
			region->fCount * sizeof(clipping_rect));
	}

	return fReceiver->Read(&region->fBounds, sizeof(clipping_rect));
}


status_t
ServerLink::AttachRegion(const BRegion& region)
{
	fSender->Attach(&region.fCount, sizeof(long));
	if (region.fCount > 0) {
		fSender->Attach(&region.fBounds, sizeof(clipping_rect));
		return fSender->Attach(region.fData,
			region.fCount * sizeof(clipping_rect));
	}

	return fSender->Attach(&region.fBounds, sizeof(clipping_rect));
}


status_t
ServerLink::ReadShape(BShape* shape)
{
	int32 opCount, ptCount;
	fReceiver->Read(&opCount, sizeof(int32));
	fReceiver->Read(&ptCount, sizeof(int32));

	uint32 opList[opCount];
	if (opCount > 0)
		fReceiver->Read(opList, opCount * sizeof(uint32));

	BPoint ptList[ptCount];
	if (ptCount > 0)
		fReceiver->Read(ptList, ptCount * sizeof(BPoint));

	shape->SetData(opCount, ptCount, opList, ptList);
	return B_OK;
}


status_t
ServerLink::AttachShape(BShape& shape)
{
	int32 opCount, ptCount;
	uint32* opList;
	BPoint* ptList;

	shape.GetData(&opCount, &ptCount, &opList, &ptList);

	fSender->Attach(&opCount, sizeof(int32));
	fSender->Attach(&ptCount, sizeof(int32));
	if (opCount > 0)
		fSender->Attach(opList, opCount * sizeof(uint32));
	if (ptCount > 0)
		fSender->Attach(ptList, ptCount * sizeof(BPoint));
	return B_OK;
}


status_t
ServerLink::ReadGradient(BGradient** _gradient)
{
	GTRACE(("ServerLink::ReadGradient\n"));
	return fReceiver->ReadGradient(_gradient);
}


status_t
ServerLink::AttachGradient(const BGradient& gradient)
{
	GTRACE(("ServerLink::AttachGradient\n"));
	BGradient::Type gradientType = gradient.GetType();
	int32 stopCount = gradient.CountColorStops();
	GTRACE(("ServerLink::AttachGradient> color stop count == %d\n",
		(int)stopCount));
	fSender->Attach(&gradientType, sizeof(BGradient::Type));
	fSender->Attach(&stopCount, sizeof(int32));
	if (stopCount > 0) {
		for (int i = 0; i < stopCount; i++) {
			fSender->Attach(gradient.ColorStopAtFast(i),
				sizeof(BGradient::ColorStop));
		}
	}

	switch (gradientType) {
		case BGradient::TYPE_LINEAR:
		{
			GTRACE(("ServerLink::AttachGradient> type == TYPE_LINEAR\n"));
			const BGradientLinear* linear = (BGradientLinear*)&gradient;
			fSender->Attach(linear->Start());
			fSender->Attach(linear->End());
			break;
		}
		case BGradient::TYPE_RADIAL:
		{
			GTRACE(("ServerLink::AttachGradient> type == TYPE_RADIAL\n"));
			const BGradientRadial* radial = (BGradientRadial*)&gradient;
			BPoint center = radial->Center();
			float radius = radial->Radius();
			fSender->Attach(&center, sizeof(BPoint));
			fSender->Attach(&radius, sizeof(float));
			break;
		}
		case BGradient::TYPE_RADIAL_FOCUS:
		{
			GTRACE(("ServerLink::AttachGradient> type == TYPE_RADIAL_FOCUS\n"));
			const BGradientRadialFocus* radialFocus
				= (BGradientRadialFocus*)&gradient;
			BPoint center = radialFocus->Center();
			BPoint focal = radialFocus->Focal();
			float radius = radialFocus->Radius();
			fSender->Attach(&center, sizeof(BPoint));
			fSender->Attach(&focal, sizeof(BPoint));
			fSender->Attach(&radius, sizeof(float));
			break;
		}
		case BGradient::TYPE_DIAMOND:
		{
			GTRACE(("ServerLink::AttachGradient> type == TYPE_DIAMOND\n"));
			const BGradientDiamond* diamond = (BGradientDiamond*)&gradient;
			BPoint center = diamond->Center();
			fSender->Attach(&center, sizeof(BPoint));
			break;
		}
		case BGradient::TYPE_CONIC:
		{
			GTRACE(("ServerLink::AttachGradient> type == TYPE_CONIC\n"));
			const BGradientConic* conic = (BGradientConic*)&gradient;
			BPoint center = conic->Center();
			float angle = conic->Angle();
			fSender->Attach(&center, sizeof(BPoint));
			fSender->Attach(&angle, sizeof(float));
			break;
		}
		case BGradient::TYPE_NONE:
		{
			GTRACE(("ServerLink::AttachGradient> type == TYPE_NONE\n"));
			break;
		}
	}
	return B_OK;
}


status_t
ServerLink::FlushWithReply(int32& code)
{
	status_t status = Flush(B_INFINITE_TIMEOUT, true);
	if (status < B_OK)
		return status;

	return GetNextMessage(code);
}


}	// namespace BPrivate
