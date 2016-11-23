/*
 * Copyright 2016, Dario Casalinuovo. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#include <MediaClient.h>
#include <MediaConnection.h>
#include <SupportDefs.h>

#include <assert.h>
#include <stdio.h>


static BMediaClient* sProducer = NULL;
static BMediaClient* sConsumer = NULL;
static BMediaClient* sFilter = NULL;


void _InitClients(bool hasFilter)
{
	sProducer = new BMediaClient("MediaClientProducer");
	sConsumer = new BMediaClient("MediaClientConsumer");

	if (hasFilter)
		sFilter = new BMediaClient("MediaClientFilter");
	else
		sFilter = NULL;
}


void _DeleteClients()
{
	delete sProducer;
	delete sConsumer;
	delete sFilter;
}


media_format _BuildRawAudioFormat()
{
	media_format format;
	format.type = B_MEDIA_RAW_AUDIO;
	format.u.raw_audio = media_multi_audio_format::wildcard;

	return format;
}


void _ConsumerProducerTest()
{
	_InitClients(false);

	BMediaConnection* output = sProducer->BeginConnection(B_MEDIA_OUTPUT);
	BMediaConnection* input = sConsumer->BeginConnection(B_MEDIA_INPUT);

	output->SetAcceptedFormat(_BuildRawAudioFormat());
	input->SetAcceptedFormat(_BuildRawAudioFormat());

	assert(sConsumer->Connect(input, output) == B_OK);
	assert(sConsumer->Disconnect() == B_OK);

	_DeleteClients();
}


void _ProducerConsumerTest()
{
	_InitClients(false);

	BMediaConnection* output = sProducer->BeginConnection(B_MEDIA_OUTPUT);
	BMediaConnection* input = sConsumer->BeginConnection(B_MEDIA_INPUT);

	assert(sProducer->Connect(output, input) == B_OK);
	assert(sProducer->Disconnect() == B_OK);

	_DeleteClients();
}


void _ProducerFilterConsumerTest()
{
	_InitClients(true);

	BMediaConnection* output = sProducer->BeginConnection(B_MEDIA_OUTPUT);
	BMediaConnection* input = sConsumer->BeginConnection(B_MEDIA_INPUT);

	BMediaConnection* filterInput = sFilter->BeginConnection(B_MEDIA_INPUT);
	BMediaConnection* filterOutput = sFilter->BeginConnection(B_MEDIA_OUTPUT);

	assert(sFilter->Bind(filterInput, filterOutput) == B_OK);

	assert(sProducer->Connect(output, filterInput) == B_OK);
	assert(sFilter->Connect(filterOutput, input) == B_OK);

	assert(sFilter->Disconnect() == B_OK);

	_DeleteClients();
}


int main()
{
	printf("Testing Simple (1:1) Producer-Consumer configuration: ");
	_ConsumerProducerTest();
	_ProducerConsumerTest();
	printf("OK\n");

	printf("Testing Simple (1:1:1) Producer-Filter-Consumer configuration: ");
	_ProducerFilterConsumerTest();
	printf("OK\n");

	return 0;
}
