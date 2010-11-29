#ifndef INSTANTIATE_OBJECT_TESTER_H
#define INSTANTIATE_OBJECT_TESTER_H


#include "LocalCommon.h"


class TInstantiateObjectTester : public BTestCase {
public:
								TInstantiateObjectTester(
									std::string name = "");

			void				Case1();
			void				Case2();
			void				Case3();
			void				Case4();
			void				Case5();
			void				Case6();
			void				Case7();
			void				Case8();
			void				Case9();
			void				Case10();
			void				Case11();
			void				Case12();
			void				Case13();
			void				Case14();
		
			void				RunTests();

	static	CppUnit::Test*		Suite();

private:
			void				LoadAddon();
			void				UnloadAddon();
			std::string			GetLocalSignature();

private:
			image_id			fAddonId;
};


#endif	// INSTANTIATE_OBJECT_TESTER_H
