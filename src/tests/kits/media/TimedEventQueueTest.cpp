#include <TimedEventQueue.h>
#include <stdio.h>

#define DEBUG 1
#include <Debug.h>

BTimedEventQueue::queue_action DoForEachHook(media_timed_event *event, void *context);
void DumpEvent(const media_timed_event & e);
void DumpEvent(const media_timed_event * e);
void InsertRemoveTest();
void DoForEachTest();
void MatchTest();
void FlushTest();

void DumpEvent(const media_timed_event & e)
{
	DumpEvent(&e);
}

void DumpEvent(const media_timed_event * e)
{
	if (!e) {
		printf("NULL\n");
		return;
	}
	printf("time = 0x%x, type = ",int(e->event_time));
	switch (e->type) {
		case BTimedEventQueue::B_NO_EVENT: printf("B_NO_EVENT\n"); break;
		case BTimedEventQueue::B_ANY_EVENT: printf("B_ANY_EVENT\n"); break;
		case BTimedEventQueue::B_START: printf("B_START\n"); break;
		case BTimedEventQueue::B_STOP: printf("B_STOP\n"); break;
		case BTimedEventQueue::B_SEEK: printf("B_SEEK\n"); break;
		case BTimedEventQueue::B_WARP: printf("B_WARP\n"); break;
		case BTimedEventQueue::B_TIMER: printf("B_TIMER\n"); break;
		case BTimedEventQueue::B_HANDLE_BUFFER: printf("B_HANDLE_BUFFER\n"); break;
		case BTimedEventQueue::B_DATA_STATUS: printf("B_DATA_STATUS\n"); break;
		case BTimedEventQueue::B_HARDWARE: printf("B_HARDWARE\n"); break;
		case BTimedEventQueue::B_PARAMETER: printf("B_PARAMETER\n"); break;
		default: printf("0x%x\n",int(e->type));
	}
}

void InsertRemoveTest()
{
	BTimedEventQueue *q =new BTimedEventQueue;
	q->AddEvent(media_timed_event(0x1007,BTimedEventQueue::B_START));//
	q->AddEvent(media_timed_event(0x1005,BTimedEventQueue::B_START));//
	q->AddEvent(media_timed_event(0x9999,BTimedEventQueue::B_STOP));//
	q->AddEvent(media_timed_event(0x1006,BTimedEventQueue::B_START));//
	q->AddEvent(media_timed_event(0x1002,BTimedEventQueue::B_START));//
	q->AddEvent(media_timed_event(0x1011,BTimedEventQueue::B_START));//
	q->AddEvent(media_timed_event(0x1000,BTimedEventQueue::B_START));//
	q->AddEvent(media_timed_event(0x0777,BTimedEventQueue::B_START));//
	q->AddEvent(media_timed_event(0x1001,BTimedEventQueue::B_START));//
	q->AddEvent(media_timed_event(0x1000,BTimedEventQueue::B_STOP));//
	q->AddEvent(media_timed_event(0x1003,BTimedEventQueue::B_START));//
	q->AddEvent(media_timed_event(0x1000,BTimedEventQueue::B_SEEK));//
	ASSERT(q->EventCount() == 12);
	ASSERT(q->HasEvents() == true);
	
	media_timed_event e1(0x1003,BTimedEventQueue::B_START);
	q->RemoveEvent(&e1);
	ASSERT(q->EventCount() == 11);
	ASSERT(q->HasEvents() == true);

	media_timed_event e2(0x1007,BTimedEventQueue::B_START);
	q->RemoveEvent(&e2);
	ASSERT(q->EventCount() == 10);
	ASSERT(q->HasEvents() == true);

	media_timed_event e3(0x1000,BTimedEventQueue::B_STOP);
	q->RemoveEvent(&e3);
	ASSERT(q->EventCount() == 9);
	ASSERT(q->HasEvents() == true);

	media_timed_event e4(0x1000,BTimedEventQueue::B_SEEK);
	q->RemoveEvent(&e4);
	ASSERT(q->EventCount() == 8);
	ASSERT(q->HasEvents() == true);

	//remove non existing element (time)
	media_timed_event e5(0x1111,BTimedEventQueue::B_STOP);
	q->RemoveEvent(&e5);
	ASSERT(q->EventCount() == 8);
	ASSERT(q->HasEvents() == true);

	//remove non existing element (type)
	media_timed_event e6(0x1011,BTimedEventQueue::B_STOP);
	q->RemoveEvent(&e6);
	ASSERT(q->EventCount() == 8);
	ASSERT(q->HasEvents() == true);

	media_timed_event e7(0x1000,BTimedEventQueue::B_START);
	q->RemoveEvent(&e7);
	ASSERT(q->EventCount() == 7);
	ASSERT(q->HasEvents() == true);

	media_timed_event e8(0x1011,BTimedEventQueue::B_START);
	q->RemoveEvent(&e8);
	ASSERT(q->EventCount() == 6);
	ASSERT(q->HasEvents() == true);

	media_timed_event e9(0x1002,BTimedEventQueue::B_START);
	q->RemoveEvent(&e9);
	ASSERT(q->EventCount() == 5);
	ASSERT(q->HasEvents() == true);

	media_timed_event e10(0x0777,BTimedEventQueue::B_START);
	q->RemoveEvent(&e10);
	ASSERT(q->EventCount() == 4);
	ASSERT(q->HasEvents() == true);

	media_timed_event e11(0x9999,BTimedEventQueue::B_STOP);
	q->RemoveEvent(&e11);
	ASSERT(q->EventCount() == 3);
	ASSERT(q->HasEvents() == true);

	media_timed_event e12(0x1006,BTimedEventQueue::B_START);
	q->RemoveEvent(&e12);
	ASSERT(q->EventCount() == 2);
	ASSERT(q->HasEvents() == true);

	media_timed_event e13(0x1001,BTimedEventQueue::B_START);
	q->RemoveEvent(&e13);
	ASSERT(q->EventCount() == 1);
	ASSERT(q->HasEvents() == true);

	media_timed_event e14(0x1005,BTimedEventQueue::B_START);
	q->RemoveEvent(&e14);
	ASSERT(q->EventCount() == 0);
	ASSERT(q->HasEvents() == false);
	
	delete q;
}

media_timed_event DoForEachEvent;
int DoForEachCount;

BTimedEventQueue::queue_action 
DoForEachHook(media_timed_event *event, void *context)
{
	DoForEachEvent = *event;
	DoForEachCount++;
	printf("Callback, event_time = %x\n",int(event->event_time));
	return BTimedEventQueue::B_NO_ACTION;
}

void DoForEachTest()
{
	BTimedEventQueue *q =new BTimedEventQueue;
	ASSERT(q->EventCount() == 0);
	ASSERT(q->HasEvents() == false);

	q->AddEvent(media_timed_event(0x1000,BTimedEventQueue::B_SEEK));
	q->AddEvent(media_timed_event(0x1001,BTimedEventQueue::B_START));
	q->AddEvent(media_timed_event(0x1002,BTimedEventQueue::B_START));
	q->AddEvent(media_timed_event(0x1003,BTimedEventQueue::B_START));
	q->AddEvent(media_timed_event(0x1010,BTimedEventQueue::B_START));
	q->AddEvent(media_timed_event(0x1011,BTimedEventQueue::B_START));
	q->AddEvent(media_timed_event(0x1012,BTimedEventQueue::B_START));
	q->AddEvent(media_timed_event(0x1004,BTimedEventQueue::B_START));
	q->AddEvent(media_timed_event(0x1005,BTimedEventQueue::B_START));
	q->AddEvent(media_timed_event(0x1006,BTimedEventQueue::B_STOP));
	q->AddEvent(media_timed_event(0x1007,BTimedEventQueue::B_START));
	q->AddEvent(media_timed_event(0x1008,BTimedEventQueue::B_START));
	q->AddEvent(media_timed_event(0x1009,BTimedEventQueue::B_START));
	q->AddEvent(media_timed_event(0x1013,BTimedEventQueue::B_START));
	q->AddEvent(media_timed_event(0x1013,BTimedEventQueue::B_START));
	q->AddEvent(media_timed_event(0x1013,BTimedEventQueue::B_SEEK));
	ASSERT(q->EventCount() == 16);
	ASSERT(q->HasEvents() == true);
	

	printf("\n expected: 0x1000\n");
	DoForEachCount = 0;	
	q->DoForEach(DoForEachHook,(void*)1234,0x1000,BTimedEventQueue::B_AT_TIME);
	ASSERT(DoForEachEvent == media_timed_event(0x1000,BTimedEventQueue::B_SEEK));
	ASSERT(DoForEachCount == 1);
	

	printf("\n expected: 0x1006\n");
	DoForEachCount = 0;	
	q->DoForEach(DoForEachHook,(void*)1234,0x1006,BTimedEventQueue::B_AT_TIME);
	ASSERT(DoForEachEvent == media_timed_event(0x1006,BTimedEventQueue::B_STOP));
	ASSERT(DoForEachCount == 1);

	printf("\n expected: 0x1013, 0x1013, 0x1013\n");
	DoForEachCount = 0;	
	q->DoForEach(DoForEachHook,(void*)1234,0x1013,BTimedEventQueue::B_AT_TIME);
	ASSERT(DoForEachCount == 3);

	printf("\n expected: 0x1000, 0x1001, 0x1002\n");
	DoForEachCount = 0;	
	q->DoForEach(DoForEachHook,(void*)1234,0x1003,BTimedEventQueue::B_BEFORE_TIME,false);
	ASSERT(DoForEachCount == 3);

	printf("\n expected: 0x1000, 0x1001, 0x1002, 0x1003\n");
	DoForEachCount = 0;	
	q->DoForEach(DoForEachHook,(void*)1234,0x1003,BTimedEventQueue::B_BEFORE_TIME,true);
	ASSERT(DoForEachCount == 4);

	printf("\n expected: 0x1013, 0x1013, 0x1013\n");
	DoForEachCount = 0;	
	q->DoForEach(DoForEachHook,(void*)1234,0x1012,BTimedEventQueue::B_AFTER_TIME,false);
	ASSERT(DoForEachCount == 3);

	printf("\n expected: 0x1012, 0x1013, 0x1013, 0x1013\n");
	DoForEachCount = 0;	
	q->DoForEach(DoForEachHook,(void*)1234,0x1012,BTimedEventQueue::B_AFTER_TIME,true);
	ASSERT(DoForEachCount == 4);

	printf("\n expected: none\n");
	DoForEachCount = 0;	
	q->DoForEach(DoForEachHook,(void*)1234,0x1013,BTimedEventQueue::B_AFTER_TIME,false);
	ASSERT(DoForEachCount == 0);

	printf("\n expected: 0x1013, 0x1013, 0x1013\n");
	DoForEachCount = 0;	
	q->DoForEach(DoForEachHook,(void*)1234,0x1013,BTimedEventQueue::B_AFTER_TIME,true);
	ASSERT(DoForEachCount == 3);

	printf("\n expected: all 16\n");
	DoForEachCount = 0;	
	q->DoForEach(DoForEachHook,(void*)1234,0x0,BTimedEventQueue::B_ALWAYS);
	ASSERT(DoForEachCount == 16);

	printf("\n expected: none\n");
	DoForEachCount = 0;	
	q->DoForEach(DoForEachHook,(void*)1234,0x0,BTimedEventQueue::B_ALWAYS,false,BTimedEventQueue::B_WARP);
	ASSERT(DoForEachCount == 0);

	printf("\n expected: 0x1000, 0x1013\n");
	DoForEachCount = 0;	
	q->DoForEach(DoForEachHook,(void*)1234,0x0,BTimedEventQueue::B_ALWAYS,false,BTimedEventQueue::B_SEEK);
	ASSERT(DoForEachCount == 2);

	printf("\n expected: 0x1000, 0x1013\n");
	DoForEachCount = 0;	
	q->DoForEach(DoForEachHook,(void*)1234,0x0999,BTimedEventQueue::B_AFTER_TIME,false,BTimedEventQueue::B_SEEK);
	ASSERT(DoForEachCount == 2);

	printf("\n expected: 0x1000, 0x1013\n");
	DoForEachCount = 0;	
	q->DoForEach(DoForEachHook,(void*)1234,0x1014,BTimedEventQueue::B_BEFORE_TIME,false,BTimedEventQueue::B_SEEK);
	ASSERT(DoForEachCount == 2);

	printf("\n expected: none\n");
	DoForEachCount = 0;	
	q->DoForEach(DoForEachHook,(void*)1234,0x0004,BTimedEventQueue::B_BEFORE_TIME,true);
	ASSERT(DoForEachCount == 0);

	delete q;
}	

void MatchTest()
{
	BTimedEventQueue *q = new BTimedEventQueue;
	ASSERT(q->EventCount() == 0);
	ASSERT(q->HasEvents() == false);

	q->AddEvent(media_timed_event(0x1000,BTimedEventQueue::B_SEEK));
	q->AddEvent(media_timed_event(0x1001,BTimedEventQueue::B_START));
	q->AddEvent(media_timed_event(0x1002,BTimedEventQueue::B_START));
	q->AddEvent(media_timed_event(0x1003,BTimedEventQueue::B_START));
	q->AddEvent(media_timed_event(0x1010,BTimedEventQueue::B_START));
	q->AddEvent(media_timed_event(0x1011,BTimedEventQueue::B_START));
	q->AddEvent(media_timed_event(0x1012,BTimedEventQueue::B_START));
	q->AddEvent(media_timed_event(0x1004,BTimedEventQueue::B_START));
	q->AddEvent(media_timed_event(0x1005,BTimedEventQueue::B_START));
	q->AddEvent(media_timed_event(0x1006,BTimedEventQueue::B_STOP));
	q->AddEvent(media_timed_event(0x1007,BTimedEventQueue::B_START));
	q->AddEvent(media_timed_event(0x1008,BTimedEventQueue::B_START));
	q->AddEvent(media_timed_event(0x1009,BTimedEventQueue::B_START));
	q->AddEvent(media_timed_event(0x1013,BTimedEventQueue::B_START));
	q->AddEvent(media_timed_event(0x1013,BTimedEventQueue::B_START));
	q->AddEvent(media_timed_event(0x1013,BTimedEventQueue::B_SEEK));
	ASSERT(q->EventCount() == 16);
	ASSERT(q->HasEvents() == true);
	
	printf("\nexpected: "); DumpEvent(media_timed_event(0x1006,BTimedEventQueue::B_STOP));
	printf("found:    "); DumpEvent(q->FindFirstMatch(0x1001,BTimedEventQueue::B_AFTER_TIME,true,BTimedEventQueue::B_STOP));

	printf("\nexpected: "); DumpEvent(media_timed_event(0x1006,BTimedEventQueue::B_STOP));
	printf("found:    "); DumpEvent(q->FindFirstMatch(0x1006,BTimedEventQueue::B_AT_TIME,true));

	printf("\nexpected: "); DumpEvent(media_timed_event(0x1006,BTimedEventQueue::B_STOP));
	printf("found:    "); DumpEvent(q->FindFirstMatch(0x1007,BTimedEventQueue::B_BEFORE_TIME,true,BTimedEventQueue::B_STOP));

	printf("\nexpected: "); DumpEvent(media_timed_event(0x1000,BTimedEventQueue::B_SEEK));
	printf("found:    "); DumpEvent(q->FindFirstMatch(0x1006,BTimedEventQueue::B_BEFORE_TIME,true));

	printf("\nexpected: "); DumpEvent(media_timed_event(0x1006,BTimedEventQueue::B_STOP));
	printf("found:    "); DumpEvent(q->FindFirstMatch(0x1006,BTimedEventQueue::B_AFTER_TIME,true));

	printf("\nexpected: "); DumpEvent(0);
	printf("found:    "); DumpEvent(q->FindFirstMatch(0x1006,BTimedEventQueue::B_BEFORE_TIME,false,BTimedEventQueue::B_STOP));

	printf("\nexpected: "); DumpEvent(0);
	printf("found:    "); DumpEvent(q->FindFirstMatch(0x1006,BTimedEventQueue::B_AFTER_TIME,false,BTimedEventQueue::B_STOP));

	printf("\nexpected: "); DumpEvent(0);
	printf("found:    "); DumpEvent(q->FindFirstMatch(0x1006,BTimedEventQueue::B_AT_TIME,false,BTimedEventQueue::B_SEEK));

	printf("\nexpected: "); DumpEvent(media_timed_event(0x1000,BTimedEventQueue::B_SEEK));
	printf("found:    "); DumpEvent(q->FindFirstMatch(0x1000,BTimedEventQueue::B_AFTER_TIME,true));

	printf("\nexpected: "); DumpEvent(media_timed_event(0x1000,BTimedEventQueue::B_SEEK));
	printf("found:    "); DumpEvent(q->FindFirstMatch(0x1010,BTimedEventQueue::B_BEFORE_TIME,true));

	printf("\nexpected: "); DumpEvent(media_timed_event(0x1000,BTimedEventQueue::B_SEEK));
	printf("found:    "); DumpEvent(q->FindFirstMatch(0x1007,BTimedEventQueue::B_BEFORE_TIME,false));

	printf("\nexpected: "); DumpEvent(media_timed_event(0x1013,BTimedEventQueue::B_SEEK));
	printf("found:    "); DumpEvent(q->FindFirstMatch(0x1001,BTimedEventQueue::B_AFTER_TIME,false,BTimedEventQueue::B_SEEK));

	printf("\nexpected: "); DumpEvent(media_timed_event(0x1010,BTimedEventQueue::B_START));
	printf("found:    "); DumpEvent(q->FindFirstMatch(0x1009,BTimedEventQueue::B_AFTER_TIME,false));

	printf("\nexpected: "); DumpEvent(media_timed_event(0x1010,BTimedEventQueue::B_START));
	printf("found:    "); DumpEvent(q->FindFirstMatch(0x1010,BTimedEventQueue::B_AFTER_TIME,true));

	printf("\nexpected: "); DumpEvent(media_timed_event(0x1010,BTimedEventQueue::B_START));
	printf("found:    "); DumpEvent(q->FindFirstMatch(0x1010,BTimedEventQueue::B_AT_TIME,true));
	
	delete q;
}	

void FlushTest()
{
	BTimedEventQueue *q = new BTimedEventQueue;
	ASSERT(q->EventCount() == 0);
	ASSERT(q->HasEvents() == false);

	q->AddEvent(media_timed_event(0x1000,BTimedEventQueue::B_SEEK));
	q->AddEvent(media_timed_event(0x1001,BTimedEventQueue::B_START));
	q->AddEvent(media_timed_event(0x1002,BTimedEventQueue::B_START));
	q->AddEvent(media_timed_event(0x1003,BTimedEventQueue::B_START));
	q->AddEvent(media_timed_event(0x1004,BTimedEventQueue::B_START));
	q->AddEvent(media_timed_event(0x1005,BTimedEventQueue::B_START));
	q->AddEvent(media_timed_event(0x1006,BTimedEventQueue::B_STOP));
	q->AddEvent(media_timed_event(0x1007,BTimedEventQueue::B_START));
	q->AddEvent(media_timed_event(0x1008,BTimedEventQueue::B_START));
	q->AddEvent(media_timed_event(0x1009,BTimedEventQueue::B_START));
	q->AddEvent(media_timed_event(0x1010,BTimedEventQueue::B_START));
	q->AddEvent(media_timed_event(0x1011,BTimedEventQueue::B_START));
	q->AddEvent(media_timed_event(0x1012,BTimedEventQueue::B_START));
	q->AddEvent(media_timed_event(0x1013,BTimedEventQueue::B_START));
	q->AddEvent(media_timed_event(0x1013,BTimedEventQueue::B_START));
	q->AddEvent(media_timed_event(0x1013,BTimedEventQueue::B_SEEK));
	ASSERT(q->EventCount() == 16);
	ASSERT(q->HasEvents() == true);

	printf("### removing 0x1007\n");	
	q->FlushEvents(0x1007, BTimedEventQueue::B_AT_TIME);
	ASSERT(q->EventCount() == 15);

	printf("### removing 0x1013\n");	
	q->FlushEvents(0x1012, BTimedEventQueue::B_AFTER_TIME,false);
	ASSERT(q->EventCount() == 12);

	printf("### removing none\n");	
	q->FlushEvents(0x1fff, BTimedEventQueue::B_AFTER_TIME,false);
	ASSERT(q->EventCount() == 12);

	printf("### removing none\n");	
	q->FlushEvents(0x1fff, BTimedEventQueue::B_AFTER_TIME,true);
	ASSERT(q->EventCount() == 12);

	printf("### removing 0x1010, 0x1011, 0x1012\n");	
	q->FlushEvents(0x1010, BTimedEventQueue::B_AFTER_TIME,true);
	ASSERT(q->EventCount() == 9);

	printf("### removing 0x1000 to 0x1005\n");	
	q->FlushEvents(0x1006, BTimedEventQueue::B_BEFORE_TIME,false);
	ASSERT(q->EventCount() == 3);

	printf("### removing 0x1006\n");	
	q->FlushEvents(0x1006, BTimedEventQueue::B_AT_TIME);
	ASSERT(q->EventCount() == 2);

	printf("### removing 0x1008 0x1009\n");	
	q->FlushEvents(0xffffff, BTimedEventQueue::B_BEFORE_TIME);
	ASSERT(q->EventCount() == 0);

	delete q;
}	


int main()
{
	InsertRemoveTest();
	DoForEachTest();
	MatchTest();
	FlushTest();
	return 0;
}
