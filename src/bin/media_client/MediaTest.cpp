/*
 * Copyright 2017, Dario Casalinuovo. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#include "MediaTest.h"

#include <SimpleMediaClient.h>
#include <MediaConnection.h>
#include <SupportDefs.h>

#include <assert.h>
#include <stdio.h>

#include "MediaDebug.h"

#ifdef DEBUG
#define DELAYED_MODE 1
#define SNOOZE_FOR 10000000
#endif

#define MAX_MULTI_CLIENTS 3

static BSimpleMediaClient* sProducer = NULL;
static BSimpleMediaClient* sConsumer = NULL;
static BSimpleMediaClient* sFilter = NULL;

static BSimpleMediaClient* sProducers[MAX_MULTI_CLIENTS];
static BSimpleMediaClient* sConsumers[MAX_MULTI_CLIENTS];


void
_InitClients(bool hasFilter)
{
	sProducer = new BSimpleMediaClient("MediaClientProducer");
	sConsumer = new BSimpleMediaClient("MediaClientConsumer");

	if (hasFilter)
		sFilter = new BSimpleMediaClient("MediaClientFilter");
	else
		sFilter = NULL;
}


void
_InitClientsMulti(bool isMixer)
{
	if (!isMixer) {
		for (int i = 0; i < MAX_MULTI_CLIENTS; i++) {
			sConsumers[i] = new BSimpleMediaClient("Test Consumer");
		}
		sProducer = new BSimpleMediaClient("MediaClientProducer");
	} else {
		for (int i = 0; i < MAX_MULTI_CLIENTS; i++) {
			sProducers[i] = new BSimpleMediaClient("Test Producer");
		}
		sConsumer = new BSimpleMediaClient("MediaClientConsumer");
	}

	sFilter = new BSimpleMediaClient("MediaClientFilter");
}


void
_DeleteClients()
{
	delete sProducer;
	delete sConsumer;
	delete sFilter;
}


void
_DeleteClientsMulti(bool isMixer)
{
	if (!isMixer) {
		for (int i = 0; i < MAX_MULTI_CLIENTS; i++) {
			delete sConsumers[i];
		}
		delete sProducer;
	} else {
		for (int i = 0; i < MAX_MULTI_CLIENTS; i++) {
			delete sProducers[i];
		}
		delete sConsumer;
	}
	delete sFilter;
}


media_format
_BuildRawAudioFormat()
{
	media_format format;
	format.type = B_MEDIA_RAW_AUDIO;
	format.u.raw_audio = media_multi_audio_format::wildcard;

	return format;
}


void
_ConsumerProducerTest()
{
	_InitClients(false);

	BSimpleMediaOutput* output = sProducer->BeginOutput();
	BSimpleMediaInput* input = sConsumer->BeginInput();

	output->SetAcceptedFormat(_BuildRawAudioFormat());
	input->SetAcceptedFormat(_BuildRawAudioFormat());

	assert(sConsumer->Connect(input, output) == B_OK);

	#ifdef DELAYED_MODE
	snooze(SNOOZE_FOR);
	#endif

	assert(input->Disconnect() == B_OK);

	_DeleteClients();
}


void
_ProducerConsumerTest()
{
	_InitClients(false);

	BMediaOutput* output = sProducer->BeginOutput();
	BMediaInput* input = sConsumer->BeginInput();

	assert(sProducer->Connect(output, input) == B_OK);

	#ifdef DELAYED_MODE
	snooze(SNOOZE_FOR);
	#endif

	assert(sProducer->Disconnect() == B_OK);

	_DeleteClients();
}


void
_ProducerFilterConsumerTest()
{
	_InitClients(true);

	BMediaOutput* output = sProducer->BeginOutput();
	BMediaInput* input = sConsumer->BeginInput();

	BMediaInput* filterInput = sFilter->BeginInput();
	BMediaOutput* filterOutput = sFilter->BeginOutput();

	assert(sFilter->Bind(filterInput, filterOutput) == B_OK);

	assert(sProducer->Connect(output, filterInput) == B_OK);
	assert(sFilter->Connect(filterOutput, input) == B_OK);
	#ifdef DELAYED_MODE
	snooze(SNOOZE_FOR);
	#endif

	assert(sFilter->Disconnect() == B_OK);

	_DeleteClients();
}


void
_SplitterConfigurationTest()
{
	_InitClientsMulti(false);

	for (int i = 0; i < MAX_MULTI_CLIENTS; i++) {
		BMediaOutput* output = sFilter->BeginOutput();
		assert(sFilter->Connect(output, sConsumers[i]->BeginInput()) == B_OK);
	}

	assert(sProducer->Connect(sProducer->BeginOutput(),
		sFilter->BeginInput()) == B_OK);

	#ifdef DELAYED_MODE
	snooze(SNOOZE_FOR);
	#endif

	_DeleteClientsMulti(false);
}


void
_MixerConfigurationTest()
{
	_InitClientsMulti(true);

	for (int i = 0; i < MAX_MULTI_CLIENTS; i++) {
		BMediaInput* input = sFilter->BeginInput();
		assert(sFilter->Connect(input, sConsumers[i]->BeginInput()) == B_OK);
	}

	assert(sConsumer->Connect(sConsumer->BeginInput(),
		sFilter->BeginOutput()) == B_OK);

	#ifdef DELAYED_MODE
	snooze(SNOOZE_FOR);
	#endif

	_DeleteClientsMulti(true);
}


void
media_test()
{
	printf("Testing Simple (1:1) Producer-Consumer configuration: ");
	_ConsumerProducerTest();
	_ProducerConsumerTest();
	printf("OK\n");

	printf("Testing Simple (1:1:1) Producer-Filter-Consumer configuration: ");
	_ProducerFilterConsumerTest();
	printf("OK\n");

	printf("Testing Splitter Configuration (N:1:1): ");
	_SplitterConfigurationTest();
	printf("OK\n");

	printf("Testing Mixer Configuration (N:1:1): ");
	_SplitterConfigurationTest();
	printf("OK\n");
}
