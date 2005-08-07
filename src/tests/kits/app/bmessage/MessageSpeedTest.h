/*
 * Copyright 2005, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Michael Lotz <mmlr@mlotz.ch>
 *		Olivier Milla <methedras at online dot fr>
 */

#ifndef _MESSAGE_SPEED_TEST_H_
#define _MESSAGE_SPEED_TEST_H_

#include "../common.h"

class TMessageSpeedTest : public TestCase {

public:
					TMessageSpeedTest() {};
					TMessageSpeedTest(std::string name)
						: TestCase(name)
					{};

		void		MessageSpeedTestCreate5Int32();
		void		MessageSpeedTestCreate50Int32();
		void		MessageSpeedTestCreate500Int32();
		void		MessageSpeedTestCreate5000Int32();

		void		MessageSpeedTestCreate5String();
		void		MessageSpeedTestCreate50String();
		void		MessageSpeedTestCreate500String();
		void		MessageSpeedTestCreate5000String();

		void		MessageSpeedTestLookup5Int32();
		void		MessageSpeedTestLookup50Int32();
		void		MessageSpeedTestLookup500Int32();
		void		MessageSpeedTestLookup5000Int32();

		void		MessageSpeedTestRead5Int32();
		void		MessageSpeedTestRead50Int32();
		void		MessageSpeedTestRead500Int32();
		void		MessageSpeedTestRead5000Int32();

		void		MessageSpeedTestRead5String();
		void		MessageSpeedTestRead50String();
		void		MessageSpeedTestRead500String();
		void		MessageSpeedTestRead5000String();

		void		MessageSpeedTestFlatten5Int32();
		void		MessageSpeedTestFlatten50Int32();
		void		MessageSpeedTestFlatten500Int32();
		void		MessageSpeedTestFlatten5000Int32();

		void		MessageSpeedTestFlatten5String();
		void		MessageSpeedTestFlatten50String();
		void		MessageSpeedTestFlatten500String();
		void		MessageSpeedTestFlatten5000String();

		void		MessageSpeedTestFlattenIndividual5Int32();
		void		MessageSpeedTestFlattenIndividual50Int32();
		void		MessageSpeedTestFlattenIndividual500Int32();
		void		MessageSpeedTestFlattenIndividual5000Int32();

		void		MessageSpeedTestFlattenIndividual5String();
		void		MessageSpeedTestFlattenIndividual50String();
		void		MessageSpeedTestFlattenIndividual500String();
		void		MessageSpeedTestFlattenIndividual5000String();

		void		MessageSpeedTestUnflatten5Int32(); 
		void		MessageSpeedTestUnflatten50Int32(); 
		void		MessageSpeedTestUnflatten500Int32();
		void		MessageSpeedTestUnflatten5000Int32();

		void		MessageSpeedTestUnflatten5String(); 
		void		MessageSpeedTestUnflatten50String(); 
		void		MessageSpeedTestUnflatten500String();
		void		MessageSpeedTestUnflatten5000String();

		void		MessageSpeedTestUnflattenIndividual5Int32(); 
		void		MessageSpeedTestUnflattenIndividual50Int32(); 
		void		MessageSpeedTestUnflattenIndividual500Int32();
		void		MessageSpeedTestUnflattenIndividual5000Int32();

		void		MessageSpeedTestUnflattenIndividual5String(); 
		void		MessageSpeedTestUnflattenIndividual50String(); 
		void		MessageSpeedTestUnflattenIndividual500String();
		void		MessageSpeedTestUnflattenIndividual5000String();

static	TestSuite	*Suite();
};

#endif	// _MESSAGE_SPEED_TEST_H_
