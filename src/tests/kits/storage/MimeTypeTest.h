// MimeTypeTest.h

#ifndef __sk_mime_type_test_h__
#define __sk_mime_type_test_h__

#include <cppunit/TestCaller.h>
#include <cppunit/TestSuite.h>

#include "BasicTest.h"
#include <Mime.h>

class BTestApp;

// Function pointer types for test sharing between {Get,Set}{Short,Long}Description()
typedef status_t (BMimeType::*GetDescriptionFunc)(char* description) const;
typedef status_t (BMimeType::*SetDescriptionFunc)(const char* description);
typedef status_t (BMimeType::*DeleteDescriptionFunc)();

class IconHelper;
class IconForTypeHelper;
class NotificationMessage;

class MimeTypeTest : public BasicTest {
public:
	static CppUnit::Test* Suite();
	
	// This function called before *each* test added in Suite()
	void setUp();
	
	// This function called after *each* test added in Suite()
	void tearDown();

	//------------------------------------------------------------
	// Test functions
	//------------------------------------------------------------
	void InstallDeleteTest();
	void AppHintTest();
	void AttrInfoTest();
	void FileExtensionsTest();
	void LargeIconTest();
	void MiniIconTest();
	void LargeIconForTypeTest();
	void MiniIconForTypeTest();
	void InstalledTypesTest();
	void LongDescriptionTest();
	void ShortDescriptionTest();
	void PreferredAppTest();
	void SupportingAppsTest();
	void SupportedTypesTest();
	void WildcardAppsTest();
	
	void InitTest();
	void StringTest();
	void MonitoringTest();
	void UpdateMimeInfoTest();
	void CreateAppMetaMimeTest();
	void GetDeviceIconTest();
	void SnifferRuleTest();
	void SniffingTest();

	//------------------------------------------------------------
	// Helper functions
	//------------------------------------------------------------
	void DescriptionTest(GetDescriptionFunc getDescr, SetDescriptionFunc setDescr,
	                       DeleteDescriptionFunc deleteDescr);
	void IconTest(IconHelper &helper);
	void IconForTypeTest(IconForTypeHelper &helper);

	void CheckNotificationMessages(const NotificationMessage *messages,
								   int32 count);
	void VerifyInstalledTypes();

private:
	BTestApp	*fApplication;
};


#endif	// __sk_mime_type_test_h__
