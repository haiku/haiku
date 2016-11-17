/*
 * Copyright 2016, Dario Casalinuovo. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#include <MediaClient.h>
#include <MediaConnection.h>
#include <SupportDefs.h>

#include <assert.h>


int main()
{
	BMediaClient* producer = new BMediaClient("MediaClientProducer");
	BMediaClient* consumer = new BMediaClient("MediaClientConsumer");

	BMediaConnection* output = producer->BeginConnection(B_MEDIA_OUTPUT);
	BMediaConnection* input = consumer->BeginConnection(B_MEDIA_INPUT);

	assert(consumer->Connect(input, output) == B_OK);

	assert(consumer->Disconnect() == B_OK);

	delete producer;
	delete consumer;

	return 0;
}
