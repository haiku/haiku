/*
 * Copyright 2015, Dario Casalinuovo. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#include <MediaClient.h>
#include <MediaConnection.h>

#include "debug.h"


media_client_id
media_client::Id() const
{
	return node.node;
}


media_client_kinds
media_client::Kinds() const
{
	return kinds;
}


const media_client&
media_connection::Client() const
{
	return client;
}


media_connection_id
media_connection::Id() const
{
	return id;
}


media_connection_kinds
media_connection::Kinds() const
{
	return kinds;
}


bool
media_connection::IsInput() const
{
	return Kinds() == B_MEDIA_INPUT;
}


bool
media_connection::IsOutput() const
{
	return Kinds() == B_MEDIA_OUTPUT;
}


media_input
media_connection::_MediaInput() const
{
	media_input input;
	input.node = client.node;
	input.source = source;
	input.destination = destination;
	input.format = format;
	return input;
}


media_output
media_connection::_MediaOutput() const
{
	media_output output;
	output.node = client.node;
	output.source = source;
	output.destination = destination;
	output.format = format;
	return output;
}


const media_source&
media_connection::_Source() const
{
	return source;
}


const media_destination&
media_connection::_Destination() const
{
	return destination;
}


media_node
media_connection::_RemoteNode() const
{
	return remote_node;
}


media_node
media_connection::_Node() const
{
	return client.node;
}
