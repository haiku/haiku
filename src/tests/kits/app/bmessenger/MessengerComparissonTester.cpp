//------------------------------------------------------------------------------
//	MessengerComparissonTester.cpp
//
//------------------------------------------------------------------------------

// Standard Includes -----------------------------------------------------------
#include <stdio.h>

// System Includes -------------------------------------------------------------
#include <be/app/Message.h>
#include <be/kernel/OS.h>

#include <Handler.h>
#include <Looper.h>
#include <Messenger.h>

// Project Includes ------------------------------------------------------------
#include <TestUtils.h>
#include <ThreadedTestCaller.h>
#include <cppunit/TestSuite.h>

// Local Includes --------------------------------------------------------------
#include "Helpers.h"
#include "MessengerComparissonTester.h"

// Local Defines ---------------------------------------------------------------

// Globals ---------------------------------------------------------------------

//------------------------------------------------------------------------------

struct FakeMessenger {
	port_id	fPort;
	int32	fHandlerToken;
	team_id	fTeam;
	int32	extra0;
	int32	extra1;
	bool	fPreferredTarget;
	bool	extra2;
	bool	extra3;
	bool	extra4;
};

static
bool
operator<(const FakeMessenger& a, const FakeMessenger& b)
{
	// significance:
	// * fPort
	// * fHandlerToken
	// * fPreferredTarget
	// fTeam is insignificant
	return (a.fPort < b.fPort
			|| a.fPort == b.fPort
				&& (a.fHandlerToken < b.fHandlerToken
					|| a.fHandlerToken == b.fHandlerToken
						&& !a.fPreferredTarget && b.fPreferredTarget));
}

static
bool
operator!=(const FakeMessenger& a, const FakeMessenger& b)
{
	return (a < b || b < a);
}

static
bool
operator==(const FakeMessenger& a, const FakeMessenger& b)
{
	return !(a != b);
}

// constructor
MessengerComparissonTester::MessengerComparissonTester()
	: BThreadedTestCase(),
	  fHandler(NULL),
	  fLooper(NULL)
{
}

// constructor
MessengerComparissonTester::MessengerComparissonTester(std::string name)
	: BThreadedTestCase(name),
	  fHandler(NULL),
	  fLooper(NULL)
{
}

// destructor
MessengerComparissonTester::~MessengerComparissonTester()
{
	if (fLooper) {
		fLooper->Lock();
		if (fHandler) {
			fLooper->RemoveHandler(fHandler);
			delete fHandler;
		}
		fLooper->Quit();
	}
}

/*
	BMessenger &operator=(const BMessenger &from)
	@case 1			from is uninitialized
	@results		IsValid() and IsTargetLocal() should return false
					Target() should return NULL and NULL for looper.
					Team() should return -1.
 */
void MessengerComparissonTester::ComparissonTest1()
{
	BMessenger a;
	BMessenger b;
	CHK(a == b);
	CHK(b == a);
	CHK(!(a != b));
	CHK(!(b != a));
}

/*
	BMessenger &operator=(const BMessenger &from)
	@case 2			from is properly initialized to a local target
					(preferred handler)
	@results		IsValid() and IsTargetLocal() should return true
					Target() should return the same values as for from.
					Team() should return this team.
 */
void MessengerComparissonTester::ComparissonTest2()
{
	// create looper
	BLooper *looper = new BLooper;
	looper->Run();
	LooperQuitter quitter(looper);
	// create messenger
	BMessenger a(NULL, looper);
	BMessenger b;
	CHK(a != b);
	CHK(b != a);
	CHK(!(a == b));
	CHK(!(b == a));
}

/*
	BMessenger &operator=(const BMessenger &from)
	@case 3			from is properly initialized to a local target
					(specific handler)
	@results		IsValid() and IsTargetLocal() should return true
					Target() should return the same values as for from.
					Team() should return this team.
 */
void MessengerComparissonTester::ComparissonTest3()
{
	// messenger1: looper and handler
	BLooper *looper1 = new BLooper;
	looper1->Run();
	LooperQuitter quitter1(looper1);
	BHandler *handler1 = new BHandler;
	HandlerDeleter deleter1(handler1);
	CHK(looper1->Lock());
	looper1->AddHandler(handler1);
	looper1->Unlock();
	BMessenger messenger1(handler1, looper1);
	BMessenger messenger1a(handler1, looper1);
	// messenger2: looper (1)
	BMessenger messenger2(NULL, looper1);
	BMessenger messenger2a(NULL, looper1);
	// messenger3: looper and handler
	BLooper *looper2 = new BLooper;
	looper2->Run();
	LooperQuitter quitter2(looper2);
	BHandler *handler2 = new BHandler;
	HandlerDeleter deleter2(handler2);
	CHK(looper2->Lock());
	looper2->AddHandler(handler2);
	looper2->Unlock();
	BMessenger messenger3(handler2, looper2);
	BMessenger messenger3a(handler2, looper2);
	// messenger4: looper (2)
	BMessenger messenger4(NULL, looper2);
	BMessenger messenger4a(NULL, looper2);
	// TODO: remote targets
	// ...

	// targets -- test data
	struct target {
		BMessenger	&messenger;
		int32		id;			// identifies the target
	} targets[] = {
		{ messenger1,	1 },
		{ messenger1a,	1 },
		{ messenger2,	2 },
		{ messenger2,	2 },
		{ messenger3,	3 },
		{ messenger3a,	3 },
		{ messenger4,	4 },
		{ messenger4a,	4 },
	};
	int32 targetCount = sizeof(targets) / sizeof(target);

	// test
	for (int32 i = 0; i < targetCount; i++) {
		NextSubTest();
		const target &target1 = targets[i];
		const BMessenger &a = target1.messenger;
		for (int32 k = 0; k < targetCount; k++) {
			const target &target2 = targets[k];
			const BMessenger &b = target2.messenger;
			bool areEqual = (target1.id == target2.id);
			CHK((a == b) == areEqual);
			CHK((b == a) == areEqual);
			CHK((a != b) == !areEqual);
			CHK((b != a) == !areEqual);
		}
	}
}

// Compare
//
// Helper function for LessTest1().
static inline
void
Compare(const FakeMessenger &fake1, const FakeMessenger &fake2,
		const BMessenger &messenger1, const BMessenger &messenger2)
{
	CHK((messenger1 == messenger2) == (fake1 == fake2));
	CHK((messenger1 != messenger2) == (fake1 != fake2));
	CHK((messenger1 < messenger2) == (fake1 < fake2));
}

/*
	bool operator<(const BMessenger &a, const BMessenger &b)
	@case 1			set fields of a and b manually
	@results		should return whatever the reference implementation
					returns.
 */
void MessengerComparissonTester::LessTest1()
{
	port_id ports[] = { -1, 0, 1 } ;
	int32 tokens[] = { -1, 0, 1 } ;
	team_id teams[] = { -1, 0, 1 } ;
	bool preferreds[] = { false, true } ;
	int32 portCount = sizeof(ports) / sizeof(port_id);
	int32 tokenCount = sizeof(tokens) / sizeof(int32);
	int32 teamCount = sizeof(teams) / sizeof(team_id);
	int32 preferredCount = 2;
	for (int32 p1 = 0; p1 < portCount; p1++) {
		port_id port1 = ports[p1];
		for (int32 to1 = 0; to1 < tokenCount; to1++) {
			int32 token1 = tokens[to1];
			for (int32 te1 = 0; te1 < teamCount; te1++) {
				team_id team1 = teams[te1];
				for (int32 pr1 = 0; pr1 < preferredCount; pr1++) {
					bool preferred1 = preferreds[pr1];
					FakeMessenger fake1;
					fake1.fPort = port1;
					fake1.fHandlerToken = token1;
					fake1.fTeam = team1;
					fake1.fPreferredTarget = preferred1;
					BMessenger &messenger1 = *(BMessenger*)&fake1;
					for (int32 p2 = 0; p2 < portCount; p2++) {
						port_id port2 = ports[p2];
						for (int32 to2 = 0; to2 < tokenCount; to2++) {
							int32 token2 = tokens[to2];
							for (int32 te2 = 0; te2 < teamCount; te2++) {
								team_id team2 = teams[te2];
								for (int32 pr2 = 0; pr2 < preferredCount;
									 pr2++) {
									bool preferred2 = preferreds[pr2];
									FakeMessenger fake2;
									fake2.fPort = port2;
									fake2.fHandlerToken = token2;
									fake2.fTeam = team2;
									fake2.fPreferredTarget = preferred2;
									BMessenger &messenger2
										= *(BMessenger*)&fake2;
									Compare(fake1, fake2, messenger1,
											messenger2);
								}
							}
						}
					}
				}
			}
		}
	}
}


Test* MessengerComparissonTester::Suite()
{
	typedef BThreadedTestCaller<MessengerComparissonTester> TC;

	TestSuite* testSuite = new TestSuite;

	ADD_TEST(testSuite, MessengerComparissonTester, ComparissonTest1);
	ADD_TEST(testSuite, MessengerComparissonTester, ComparissonTest2);
	ADD_TEST(testSuite, MessengerComparissonTester, ComparissonTest3);
	ADD_TEST(testSuite, MessengerComparissonTester, LessTest1);

	return testSuite;
}

