#include <Application.h>
#include <MediaRoster.h>
#include <TimeSource.h>
#include "ConsumerNode.h"
#include "ProducerNode.h"
#include "misc.h"

BMediaRoster *roster;
ProducerNode *producer;
ConsumerNode *consumer;
status_t rv;

int main()
{
	out("Basic BBufferProducer, BBufferConsumer, BMediaRoster test\n");
	out("for OpenBeOS by Marcus Overhagen <Marcus@Overhagen.de>\n\n");
	out("Creating BApplication now\n");
	BApplication app("application/x-vnd.OpenBeOS-NodeTest");	

	out("Creating MediaRoster\n");
	roster = BMediaRoster::Roster();
	val(roster);

	out("Creating ProducerNode\n");
	producer = new ProducerNode();
	val(producer);

	out("Creating ConsumerNode\n");
	consumer = new ConsumerNode();
	val(consumer);
	
	out("Registering ProducerNode\n");
	rv = roster->RegisterNode(producer);
	val(rv);

	out("Registering ConsumerNode\n");
	rv = roster->RegisterNode(consumer);
	val(rv);

	media_node sourceNode;
	media_node destinationNode;

	out("Calling producer->Node()\n");
	sourceNode = producer->Node();

	out("Calling consumer->Node()\n");
	destinationNode = consumer->Node();

	media_output output;
	media_input input;
	int32 count;

#if 1
	out("Calling GetAllOutputsFor(source)\n");
	rv = roster->GetAllOutputsFor(sourceNode,&output,1,&count);
	val(rv);
	rv = (count == 1) ? B_OK : B_ERROR;
	val(rv);

	out("Calling GetAllInputsFor(destination)\n");
	rv = roster->GetAllInputsFor(destinationNode,&input,1,&count);
	val(rv);
	rv = (count == 1) ? B_OK : B_ERROR;
	val(rv);
#else
	out("Calling GetFreeOutputsFor(source)\n");
	rv = roster->GetFreeOutputsFor(sourceNode,&output,1,&count,B_MEDIA_RAW_AUDIO);
	val(rv);
	rv = (count == 1) ? B_OK : B_ERROR;
	val(rv);

	out("Calling GetFreeInputsFor(destination)\n");
	rv = roster->GetFreeInputsFor(destinationNode,&input,1,&count,B_MEDIA_RAW_AUDIO);
	val(rv);
	rv = (count == 1) ? B_OK : B_ERROR;
	val(rv);
#endif
		
	media_format format;
	format.type = B_MEDIA_RAW_AUDIO; 
	format.u.raw_audio = media_raw_audio_format::wildcard;
		
	out("Connecting nodes\n");
	rv = roster->Connect(output.source, input.destination, &format, &output, &input);
	val(rv);

	out("Prerolling Producer()\n");
	rv = roster->PrerollNode(sourceNode); 
	val(rv);

	out("Prerolling Consumer\n");
	rv = roster->PrerollNode(destinationNode);
	val(rv);

	bigtime_t time1;
	bigtime_t time2;
	bigtime_t start;

	out("Getting Producer startlatency\n");
	rv = roster->GetStartLatencyFor(destinationNode, &time1);
	val(rv);

	out("Getting Consumer startlatency\n");
	rv = roster->GetStartLatencyFor(sourceNode, &time2);
	val(rv);
	
	start = max_c(time1,time2) + producer->TimeSource()->PerformanceTimeFor(BTimeSource::RealTime() + 2000000); 

	out("Starting Consumer in 2 sec\n");
	rv = roster->StartNode(destinationNode, start); 
	val(rv);

	out("Starting Producer in 2 sec\n");
	rv = roster->StartNode(sourceNode, start); 
	val(rv);

	out("########################## PRESS ENTER TO QUIT ##########################\n");
	getchar();

	media_node_id sourceNodeID;
	media_node_id destinationNodeID;

	out("Calling producer->ID()\n");
	sourceNodeID = producer->ID();

	out("Calling consumer->ID()\n");
	destinationNodeID = consumer->ID();

	out("Stopping Producer()\n");
	rv = roster->StopNode(sourceNode, 0, true); 
	val(rv);

	out("Stopping Consumer\n");
	rv = roster->StopNode(destinationNode, 0, true);
	val(rv);

	out("Disconnecting nodes\n");
	rv = roster->Disconnect(sourceNodeID, output.source, destinationNodeID, input.destination);
	val(rv);

	out("Unregistering ProducerNode\n");
	rv = roster->UnregisterNode(producer);
	val(rv);

	out("Unregistering ConsumerNode\n");
	rv = roster->UnregisterNode(consumer);
	val(rv);

	out("Releasing ProducerNode\n");
	rv = (producer->Release() == NULL) ? B_OK : B_ERROR;
	val(rv);

	out("Releasing ConsumerNode\n");
	rv = (consumer->Release() == NULL) ? B_OK : B_ERROR;
	val(rv);

	return 0;
}

