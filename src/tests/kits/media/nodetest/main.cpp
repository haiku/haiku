#include <Application.h>
#include <MediaDefs.h>
#include <MediaRoster.h>
#include <TimeSource.h>

#include <string.h>

#include "ConsumerNode.h"
#include "misc.h"
#include "ProducerNode.h"


BMediaRoster *roster;
ProducerNode *producer;
ConsumerNode *consumer;
status_t rv;

int main()
{
	out("Basic BBufferProducer, BBufferConsumer, BMediaRoster test\n");
	out("for Haiku by Marcus Overhagen <Marcus@Overhagen.de>\n\n");
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


	live_node_info live_nodes[100];
	int32 live_count;
	media_format liveformat;
	memset(&liveformat, 0, sizeof(liveformat));

	liveformat.type = B_MEDIA_RAW_AUDIO;

	out("Calling GetLiveNodes(), has_input = B_MEDIA_RAW_AUDIO\n");
	live_count = 100;
	rv = roster->GetLiveNodes(live_nodes, &live_count, &liveformat);
	val(rv);
	out("Found %ld\n",live_count);

	out("Calling GetLiveNodes(), has_output = B_MEDIA_RAW_AUDIO\n");
	live_count = 100;
	rv = roster->GetLiveNodes(live_nodes, &live_count, NULL, &liveformat);
	val(rv);
	out("Found %ld\n",live_count);

	out("Calling GetLiveNodes(), has_input = has_output = B_MEDIA_RAW_AUDIO\n");
	live_count = 100;
	rv = roster->GetLiveNodes(live_nodes, &live_count, &liveformat, &liveformat);
	val(rv);
	out("Found %ld\n",live_count);

	liveformat.type = B_MEDIA_RAW_VIDEO;

	out("Calling GetLiveNodes(), has_input = B_MEDIA_RAW_VIDEO\n");
	live_count = 100;
	rv = roster->GetLiveNodes(live_nodes, &live_count, &liveformat);
	val(rv);
	out("Found %ld\n",live_count);

	out("Calling GetLiveNodes(), has_output = B_MEDIA_RAW_VIDEO\n");
	live_count = 100;
	rv = roster->GetLiveNodes(live_nodes, &live_count, NULL, &liveformat);
	val(rv);
	out("Found %ld\n",live_count);

	out("Calling GetLiveNodes(), has_input = has_output = B_MEDIA_RAW_VIDEO\n");
	live_count = 100;
	rv = roster->GetLiveNodes(live_nodes, &live_count, &liveformat, &liveformat);
	val(rv);
	out("Found %ld\n",live_count);


	media_output output;
	media_input input;
	media_output outputs[2];
	media_input inputs[2];
	int32 count;

	out("Calling GetAllOutputsFor(source)\n");
	rv = roster->GetAllOutputsFor(sourceNode,outputs,2,&count);
	val(rv);
	out("Found %ld\n",count);
	rv = (count == 1) ? B_OK : B_ERROR;
	val(rv);

	out("Calling GetAllInputsFor(destination)\n");
	rv = roster->GetAllInputsFor(destinationNode,inputs,2,&count);
	val(rv);
	out("Found %ld\n",count);
	rv = (count == 1) ? B_OK : B_ERROR;
	val(rv);

	out("Calling GetAllInputsFor(source) (should fail)\n");
	rv = roster->GetAllInputsFor(sourceNode,inputs,2,&count);
	val(rv);
	out("Found %ld\n",count);

	out("Calling GetAllOutputsFor(destination) (should fail)\n");
	rv = roster->GetAllOutputsFor(destinationNode,outputs,2,&count);
	val(rv);
	out("Found %ld\n",count);

	out("Calling GetConnectedOutputsFor(source)\n");
	rv = roster->GetConnectedOutputsFor(sourceNode,outputs,2,&count);
	val(rv);
	out("Found %ld\n",count);
	rv = (count == 0) ? B_OK : B_ERROR;
	val(rv);

	out("Calling GetConnectedInputsFor(destination)\n");
	rv = roster->GetConnectedInputsFor(destinationNode,inputs,2,&count);
	val(rv);
	out("Found %ld\n",count);
	rv = (count == 0) ? B_OK : B_ERROR;
	val(rv);

	out("Calling GetConnectedInputsFor(source) (should fail)\n");
	rv = roster->GetConnectedInputsFor(sourceNode,inputs,2,&count);
	val(rv);
	out("Found %ld\n",count);

	out("Calling GetConnectedOutputsFor(destination) (should fail)\n");
	rv = roster->GetConnectedOutputsFor(destinationNode,outputs,2,&count);
	val(rv);
	out("Found %ld\n",count);

	out("Calling GetFreeOutputsFor(source)\n");
	rv = roster->GetFreeOutputsFor(sourceNode,&output,1,&count,B_MEDIA_RAW_AUDIO);
	val(rv);
	out("Found %ld\n",count);
	rv = (count == 1) ? B_OK : B_ERROR;
	val(rv);

	out("Calling GetFreeInputsFor(destination)\n");
	rv = roster->GetFreeInputsFor(destinationNode,&input,1,&count,B_MEDIA_RAW_AUDIO);
	val(rv);
	out("Found %ld\n",count);
	rv = (count == 1) ? B_OK : B_ERROR;
	val(rv);
		
	out("Calling GetFreeOutputsFor(destination) (should fail)\n");
	rv = roster->GetFreeOutputsFor(destinationNode,outputs,2,&count,B_MEDIA_RAW_AUDIO);
	val(rv);
	out("Found %ld\n",count);

	out("Calling GetFreeInputsFor(source) (should fail)\n");
	rv = roster->GetFreeInputsFor(sourceNode,inputs,2,&count,B_MEDIA_RAW_AUDIO);
	val(rv);
	out("Found %ld\n",count);

	out("Calling GetAttNodeAttributesFor(source)\n");
	media_node_attribute attr[10];
	ssize_t size = roster->GetNodeAttributesFor(sourceNode, attr, 10);
	val_size(size);
	out("Found %" B_PRIdSSIZE "\n", size);

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

	out("Testing SyncToNode performance time set is 5 sec\n");
	rv = roster->SyncToNode(sourceNode,
		producer->TimeSource()->PerformanceTimeFor(
			BTimeSource::RealTime() + 5000000), B_INFINITE_TIMEOUT);
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

