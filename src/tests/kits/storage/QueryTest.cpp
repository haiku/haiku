// QueryTest.cpp

#include <ctype.h>
#include <fs_info.h>
#include <stdio.h>
#include <string>
#include <unistd.h>

#include "QueryTest.h"

#include <Application.h>
#include <Message.h>
#include <MessageQueue.h>
#include <Messenger.h>
#include <NodeMonitor.h>
#include <OS.h>
#include <Path.h>
#include <Query.h>
#include <String.h>
#include <Volume.h>
#include <TestApp.h>
#include <TestUtils.h>

// Query

class Query : public BQuery {
public:
#if TEST_R5
	status_t PushValue(int32 value)			{ PushInt32(value); return B_OK; }
	status_t PushValue(uint32 value)		{ PushUInt32(value); return B_OK; }
	status_t PushValue(int64 value)			{ PushInt64(value); return B_OK; }
	status_t PushValue(uint64 value)		{ PushUInt64(value); return B_OK; }
	status_t PushValue(float value)			{ PushFloat(value); return B_OK; }
	status_t PushValue(double value)		{ PushDouble(value); return B_OK; }
	status_t PushValue(const BString value, bool caseInsensitive = false)
	{
		PushString(value.String(), caseInsensitive); return B_OK;
	}
	status_t PushAttr(const char *attribute)
	{
		BQuery::PushAttr(attribute);
		return B_OK;
	}
	status_t PushOp(query_op op)
	{
		BQuery::PushOp(op);
		return B_OK;
	}
#else
	status_t PushValue(int32 value)			{ return PushInt32(value); }
	status_t PushValue(uint32 value)		{ return PushUInt32(value); }
	status_t PushValue(int64 value)			{ return PushInt64(value); }
	status_t PushValue(uint64 value)		{ return PushUInt64(value); }
	status_t PushValue(float value)			{ return PushFloat(value); }
	status_t PushValue(double value)		{ return PushDouble(value); }
	status_t PushValue(const BString value, bool caseInsensitive = false)
	{
		return PushString(value.String(), caseInsensitive);
	}
#endif
};


// PredicateNode

class PredicateNode {
public:
	virtual ~PredicateNode() {}

	virtual status_t push(Query &query) const = 0;
	virtual BString toString() const = 0;
};


// ValueNode

template<typename ValueType>
class ValueNode : public PredicateNode {
public:
	ValueNode(ValueType v) : value(v) {}

	virtual ~ValueNode() {}

	virtual status_t push(Query &query) const
	{
		return query.PushValue(value);
	}
	
	virtual BString toString() const
	{
		return BString() << value;
	}

	ValueType value;
};

// float specialization
BString
ValueNode<float>::toString() const
{
	char buffer[32];
	sprintf(buffer, "0x%08lx", *(int32*)&value);
	return BString() << buffer;
}

// double specialization
BString
ValueNode<double>::toString() const
{
	char buffer[32];
	sprintf(buffer, "0x%016Lx", *(int64*)&value);
	return BString() << buffer;
}

// StringNode

class StringNode : public PredicateNode {
public:
	StringNode(BString v, bool caseInsensitive = false)
		: value(v), caseInsensitive(caseInsensitive)
	{
	}

	virtual ~StringNode() {}

	virtual status_t push(Query &query) const
	{
		return query.PushValue(value, caseInsensitive);
	}
	
	virtual BString toString() const
	{
		BString escaped;
		if (caseInsensitive) {
			const char *str = value.String();
			int32 len = value.Length();
			for (int32 i = 0; i < len; i++) {
				char c = str[i];
				if (isalpha(c)) {
					int lower = tolower(c);
					int upper = toupper(c);
					if (lower < 0 || upper < 0)
						escaped << c;
					else
						escaped << "[" << (char)lower << (char)upper << "]";
				} else
					escaped << c;
			}
		} else
			escaped = value;
		escaped.CharacterEscape("\"\\'", '\\');
		return BString("\"") << escaped << "\"";
	}

	BString value;
	bool	caseInsensitive;
};


// DateNode

class DateNode : public PredicateNode {
public:
	DateNode(BString v) : value(v) {}

	virtual ~DateNode() {}

	virtual status_t push(Query &query) const
	{
		return query.PushDate(value.String());
	}
	
	virtual BString toString() const
	{
		BString escaped(value);
		escaped.CharacterEscape("%\"\\'", '\\');
		return BString("%") << escaped << "%";
	}

	BString value;
};


// AttributeNode

class AttributeNode : public PredicateNode {
public:
	AttributeNode(BString v) : value(v) {}

	virtual ~AttributeNode() {}

	virtual status_t push(Query &query) const
	{
		return query.PushAttr(value.String());
	}
	
	virtual BString toString() const
	{
		return value;
	}

	BString value;
};


// short hands
typedef ValueNode<int32>	Int32Node;
typedef ValueNode<uint32>	UInt32Node;
typedef ValueNode<int64>	Int64Node;
typedef ValueNode<uint64>	UInt64Node;
typedef ValueNode<float>	FloatNode;
typedef ValueNode<double>	DoubleNode;


// ListNode

class ListNode : public PredicateNode {
public:
	ListNode(PredicateNode *child1 = NULL, PredicateNode *child2 = NULL,
			 PredicateNode *child3 = NULL, PredicateNode *child4 = NULL,
			 PredicateNode *child5 = NULL, PredicateNode *child6 = NULL)
		: children()
	{
		addChild(child1);
		addChild(child2);
		addChild(child3);
		addChild(child4);
		addChild(child5);
		addChild(child6);
	}

	virtual ~ListNode()
	{
		for (int32 i = 0; PredicateNode *child = childAt(i); i++)
			delete child;
	}

	virtual status_t push(Query &query) const
	{
		status_t error = B_OK;
		for (int32 i = 0; PredicateNode *child = childAt(i); i++) {
			error = child->push(query);
			if (error != B_OK)
				break;
		}
		return error;
	}
	
	virtual BString toString() const
	{
		return BString("INVALID");
	}

	ListNode &addChild(PredicateNode *child)
	{
		if (child)
			children.AddItem(child);
		return *this;
	}

	PredicateNode *childAt(int32 index) const
	{
		return (PredicateNode*)children.ItemAt(index);
	}

	BList children;
};

// OpNode

class OpNode : public ListNode {
public:
	OpNode(query_op op, PredicateNode *left, PredicateNode *right = NULL)
		: ListNode(left, right), op(op) {}

	virtual ~OpNode() { }

	virtual status_t push(Query &query) const
	{
		status_t error = ListNode::push(query);
		if (error == B_OK)
			error = query.PushOp(op);
		return error;
	}
	
	virtual BString toString() const
	{
		PredicateNode *left = childAt(0);
		PredicateNode *right = childAt(1);
		if (!left)
			return "INVALID ARGS";
		BString result;
		BString leftString = left->toString();
		BString rightString;
		if (right)
			rightString = right->toString();
		switch (op) {
			case B_INVALID_OP:
				result = "INVALID";
				break;
			case B_EQ:
				result << "(" << leftString << "==" << rightString << ")";
				break;
			case B_GT:
				result << "(" << leftString << ">" << rightString << ")";
				break;
			case B_GE:
				result << "(" << leftString << ">=" << rightString << ")";
				break;
			case B_LT:
				result << "(" << leftString << "<" << rightString << ")";
				break;
			case B_LE:
				result << "(" << leftString << "<=" << rightString << ")";
				break;
			case B_NE:
				result << "(" << leftString << "!=" << rightString << ")";
				break;
			case B_CONTAINS:
			{
				StringNode *strNode = dynamic_cast<StringNode*>(right);
				if (strNode) {
					rightString = StringNode(BString("*") << strNode->value
											 << "*").toString();
				}
				result << "(" << leftString << "==" << rightString << ")";
				break;
			}
			case B_BEGINS_WITH:
			{
				StringNode *strNode = dynamic_cast<StringNode*>(right);
				if (strNode) {
					rightString = StringNode(BString(strNode->value) << "*")
											 .toString();
				}
				result << "(" << leftString << "==" << rightString << ")";
				break;
			}
			case B_ENDS_WITH:
			{
				StringNode *strNode = dynamic_cast<StringNode*>(right);
				if (strNode) {
					rightString = StringNode(BString("*") << strNode->value)
											 .toString();
				}
				result << "(" << leftString << "==" << rightString << ")";
				break;
			}
			case B_AND:
				result << "(" << leftString << "&&" << rightString << ")";
				break;
			case B_OR:
				result << "(" << leftString << "||" << rightString << ")";
				break;
			case B_NOT:
				result << "(" << "!" << leftString << ")";
				break;
			case _B_RESERVED_OP_:
				result = "RESERVED";
				break;
		}
		return result;
	}

	query_op op;
};


// QueryTestEntry
class QueryTestEntry {
public:
	QueryTestEntry(string path, node_flavor kind,
				   const QueryTestEntry *linkTarget = NULL)
		: path(path),
		  cpath(NULL),
		  kind(kind),
		  linkToPath(),
		  clinkToPath(NULL),
		  directory(-1),
		  node(-1),
		  name()
	{
		cpath = this->path.c_str();
		if (linkTarget)
			linkToPath = linkTarget->path;
		clinkToPath = this->linkToPath.c_str();
	}

	string operator+(string leaf) const
	{
		return path + "/" + leaf;
	}
	
	string		path;
	const char	*cpath;
	node_flavor	kind;
	string		linkToPath;
	const char	*clinkToPath;
	ino_t		directory;
	ino_t		node;
	string		name;
};

static const char *testVolumeImage	= "/tmp/query-test-image";
static const char *testMountPoint	= "/non-existing-mount-point";

// the test entry hierarchy:
// mountPoint
// + dir1
//   + subdir11
//   + subdir12
//   + file11
//   + file12
//   + link11
// + dir2
//   + subdir21
//   + subdir22
//   + subdir23
//   + file21
//   + file22
//   + link21
// + dir3
//   + subdir31
//   + subdir32
//   + file31
//   + file32
//   + link31
// + file1
// + file2
// + file3
// + link1
// + link2
// + link3
static QueryTestEntry mountPoint(testMountPoint, B_DIRECTORY_NODE);
static QueryTestEntry dir1(mountPoint + "dir1", B_DIRECTORY_NODE);
static QueryTestEntry subdir11(dir1 + "subdir11", B_DIRECTORY_NODE);
static QueryTestEntry subdir12(dir1 + "subdir12", B_DIRECTORY_NODE);
static QueryTestEntry file11(dir1 + "file11", B_FILE_NODE);
static QueryTestEntry file12(dir1 + "file12", B_FILE_NODE);
static QueryTestEntry link11(dir1 + "link11", B_SYMLINK_NODE, &file11);
static QueryTestEntry dir2(mountPoint + "dir2", B_DIRECTORY_NODE);
static QueryTestEntry subdir21(dir2 + "subdir21", B_DIRECTORY_NODE);
static QueryTestEntry subdir22(dir2 + "subdir22", B_DIRECTORY_NODE);
static QueryTestEntry subdir23(dir2 + "subdir23", B_DIRECTORY_NODE);
static QueryTestEntry file21(dir2 + "file21", B_FILE_NODE);
static QueryTestEntry file22(dir2 + "file22", B_FILE_NODE);
static QueryTestEntry link21(dir2 + "link21", B_SYMLINK_NODE, &file12);
static QueryTestEntry dir3(mountPoint + "dir3", B_DIRECTORY_NODE);
static QueryTestEntry subdir31(dir3 + "subdir31", B_DIRECTORY_NODE);
static QueryTestEntry subdir32(dir3 + "subdir32", B_DIRECTORY_NODE);
static QueryTestEntry file31(dir3 + "file31", B_FILE_NODE);
static QueryTestEntry file32(dir3 + "file32", B_FILE_NODE);
static QueryTestEntry link31(dir3 + "link31", B_SYMLINK_NODE, &file22);
static QueryTestEntry file1(mountPoint + "file1", B_FILE_NODE);
static QueryTestEntry file2(mountPoint + "file2", B_FILE_NODE);
static QueryTestEntry file3(mountPoint + "file3", B_FILE_NODE);
static QueryTestEntry link1(mountPoint + "link1", B_SYMLINK_NODE, &file1);
static QueryTestEntry link2(mountPoint + "link2", B_SYMLINK_NODE, &file2);
static QueryTestEntry link3(mountPoint + "link3", B_SYMLINK_NODE, &file3);

static QueryTestEntry *allTestEntries[] = {
	&dir1, &subdir11, &subdir12, &file11, &file12, &link11,
	&dir2, &subdir21, &subdir22, &subdir23, &file21, &file22, &link21,
	&dir3, &subdir31, &subdir32, &file31, &file32, &link31,
	&file1, &file2, &file3, &link1, &link2, &link3
};
static const int32 allTestEntryCount
	= sizeof(allTestEntries) / sizeof(QueryTestEntry*);

// create_test_entries
void
create_test_entries(QueryTestEntry **testEntries, int32 count)
{
	// create the command line
	string cmdLine("true");
	for (int32 i = 0; i < count; i++) {
		const QueryTestEntry *entry = testEntries[i];
		switch (entry->kind) {
			case B_DIRECTORY_NODE:
				cmdLine += " ; mkdir " + entry->path;
				break;
			case B_FILE_NODE:
				cmdLine += " ; touch " + entry->path;
				break;
			case B_SYMLINK_NODE:
				cmdLine += " ; ln -s " + entry->linkToPath + " " + entry->path;
				break;
			case B_ANY_NODE:
			default:
				printf("WARNING: invalid node kind\n");
				break;
		}
	}
	BasicTest::execCommand(cmdLine);
}

// delete_test_entries
void
delete_test_entries(QueryTestEntry **testEntries, int32 count)
{
	// create the command line
	string cmdLine("true");
	for (int32 i = 0; i < count; i++) {
		const QueryTestEntry *entry = testEntries[i];
		switch (entry->kind) {
			case B_DIRECTORY_NODE:
			case B_FILE_NODE:
			case B_SYMLINK_NODE:
				cmdLine += " ; rm -rf " + entry->path;
				break;
			case B_ANY_NODE:
			default:
				printf("WARNING: invalid node kind\n");
				break;
		}
	}
	BasicTest::execCommand(cmdLine);
}




// QueryTest

// Suite
CppUnit::Test*
QueryTest::Suite() {
	CppUnit::TestSuite *suite = new CppUnit::TestSuite();
	typedef CppUnit::TestCaller<QueryTest> TC;
		
	suite->addTest( new TC("BQuery::Predicate Test",
						   &QueryTest::PredicateTest) );
	suite->addTest( new TC("BQuery::Parameter Test",
						   &QueryTest::ParameterTest) );
	suite->addTest( new TC("BQuery::Fetch Test", &QueryTest::FetchTest) );
	suite->addTest( new TC("BQuery::Live Test", &QueryTest::LiveTest) );

	return suite;
}		

// setUp
void
QueryTest::setUp()
{
	BasicTest::setUp();
	fApplication = new BTestApp("application/x-vnd.obos.query-test");
	if (fApplication->Init() != B_OK) {
		fprintf(stderr, "Failed to initialize application.\n");
		delete fApplication;
		fApplication = NULL;
	}
	fVolumeCreated = false;
}
	
// tearDown
void
QueryTest::tearDown()
{
	BasicTest::tearDown();
	if (fApplication) {
		fApplication->Terminate();
		delete fApplication;
		fApplication = NULL;
	}
	if (fVolumeCreated) {
		deleteVolume(testVolumeImage, testMountPoint);
		fVolumeCreated = false;
	}
}

// TestPredicate
static
void
TestPredicate(const PredicateNode &predicateNode, status_t pushResult = B_OK,
			  status_t getResult = B_OK)
{
	BString predicateString = predicateNode.toString().String();
//printf("predicate: `%s'\n", predicateString.String());
	// GetPredicate(BString *)
	{
		Query query;
//		CPPUNIT_ASSERT( predicateNode.push(query) == pushResult );
status_t error = predicateNode.push(query);
if (error != pushResult) {
printf("predicate: `%s'\n", predicateString.String());
printf("error: %lx vs %lx\n", error, pushResult);
}
CPPUNIT_ASSERT( error == pushResult );
		if (pushResult == B_OK) {
			BString predicate;
//			CPPUNIT_ASSERT( query.GetPredicate(&predicate) == getResult );
error = query.GetPredicate(&predicate);
if (error != getResult) {
printf("predicate: `%s'\n", predicateString.String());
printf("error: %lx vs %lx\n", error, getResult);
}
CPPUNIT_ASSERT( error == getResult );
			if (getResult == B_OK) {
				CPPUNIT_ASSERT( (int32)query.PredicateLength()
								== predicateString.Length() + 1 );
				CPPUNIT_ASSERT( predicateString == predicate );
			}
		}
	}
	// GetPredicate(char *, size_t)
	{
		Query query;
		CPPUNIT_ASSERT( predicateNode.push(query) == pushResult );
		if (pushResult == B_OK) {
			char buffer[1024];
			CPPUNIT_ASSERT( query.GetPredicate(buffer, sizeof(buffer))
							== getResult );
			if (getResult == B_OK)
				CPPUNIT_ASSERT( predicateString == buffer );
		}
	}
	// PredicateLength()
	{
		Query query;
		CPPUNIT_ASSERT( predicateNode.push(query) == pushResult );
		if (pushResult == B_OK) {
			size_t expectedLength
				= (getResult == B_OK ? predicateString.Length() + 1 : 0);
			CPPUNIT_ASSERT( query.PredicateLength() == expectedLength );
		}
	}
	// SetPredicate()
	{
		Query query;
		CPPUNIT_ASSERT( query.SetPredicate(predicateString.String()) == B_OK );
		CPPUNIT_ASSERT( (int32)query.PredicateLength()
						== predicateString.Length() + 1 );
		BString predicate;
		CPPUNIT_ASSERT( query.GetPredicate(&predicate) == B_OK );
		CPPUNIT_ASSERT( predicateString == predicate );
	}
}

// TestOperator
static
void
TestOperator(query_op op)
{
	// well formed
	TestPredicate(OpNode(op,
		new AttributeNode("attribute"),
		new Int32Node(42)
	));
	TestPredicate(OpNode(op,
		new AttributeNode("attribute"),
		new StringNode("some string")
	));
	TestPredicate(OpNode(op,
		new AttributeNode("attribute"),
		new DateNode("22 May 2002")
	));
	// ill formed
	TestPredicate(OpNode(op, new AttributeNode("attribute"), NULL), B_OK,
				  B_NO_INIT);
// R5: crashs when pushing B_CONTAINS/B_BEGINS/ENDS_WITH on an empty stack
#if TEST_R5
if (op < B_CONTAINS || op > B_ENDS_WITH)
#endif
	TestPredicate(OpNode(op, NULL, NULL), B_OK, B_NO_INIT);
	TestPredicate(OpNode(op,
		new AttributeNode("attribute"),
		new DateNode("22 May 2002")
	).addChild(new Int32Node(42)), B_OK, B_NO_INIT);
}

// PredicateTest
void
QueryTest::PredicateTest()
{
	// tests:
	// * Push*()
	// * Set/GetPredicate(), PredicateLength()
	// empty predicate
	NextSubTest();
	char buffer[1024];
	{
		Query query;
		BString predicate;
		CPPUNIT_ASSERT( query.GetPredicate(&predicate) == B_NO_INIT );
	}
	{
		Query query;
		CPPUNIT_ASSERT( query.GetPredicate(buffer, sizeof(buffer))
						== B_NO_INIT );
	}
	// one element predicates
	NextSubTest();
	TestPredicate(Int32Node(42));
	TestPredicate(UInt32Node(42));
	TestPredicate(Int64Node(42));
// R5: buggy PushUInt64() implementation.
#if !TEST_R5
	TestPredicate(UInt64Node(42));
#endif
	TestPredicate(FloatNode(42));
	TestPredicate(DoubleNode(42));
	TestPredicate(StringNode("some \" chars ' to \\ be ( escaped ) or "
							 "% not!"));
	TestPredicate(StringNode("some \" chars ' to \\ be ( escaped ) or "
							 "% not!", true));
	TestPredicate(DateNode("+15 min"));
	TestPredicate(DateNode("22 May 2002"));
	TestPredicate(DateNode("tomorrow"));
	TestPredicate(DateNode("17:57"));
	TestPredicate(DateNode("invalid date"), B_BAD_VALUE);
	TestPredicate(AttributeNode("some attribute"));
	// operators
	NextSubTest();
	TestOperator(B_EQ);
	TestOperator(B_GT);
	TestOperator(B_GE);
	TestOperator(B_LT);
	TestOperator(B_LE);
	TestOperator(B_NE);
	TestOperator(B_CONTAINS);
	TestOperator(B_BEGINS_WITH);
	TestOperator(B_ENDS_WITH);
	TestOperator(B_AND);
	TestOperator(B_OR);
	{	
		// B_NOT
		TestPredicate(OpNode(B_NOT, new AttributeNode("attribute")));
		TestPredicate(OpNode(B_NOT, new Int32Node(42)));
		TestPredicate(OpNode(B_NOT, new StringNode("some string")));
		TestPredicate(OpNode(B_NOT, new StringNode("some string", true)));
		TestPredicate(OpNode(B_NOT, new DateNode("22 May 2002")));
		TestPredicate(OpNode(B_NOT, NULL), B_OK, B_NO_INIT);
	}
	// well formed, legal predicate
	NextSubTest();
	TestPredicate(OpNode(B_AND,
		new OpNode(B_CONTAINS,
			new AttributeNode("attribute"),
			new StringNode("hello")
		),
		new OpNode(B_OR,
			new OpNode(B_NOT,
				new OpNode(B_EQ,
					new AttributeNode("attribute2"),
					new UInt32Node(7)
				),
				NULL
			),
			new OpNode(B_GE,
				new AttributeNode("attribute3"),
				new DateNode("20 May 2002")
			)
		)
	));
	// well formed, illegal predicate
	NextSubTest();
	TestPredicate(OpNode(B_EQ,
		new StringNode("hello"),
		new OpNode(B_LE,
			new OpNode(B_NOT,
				new Int32Node(17),
				NULL
			),
			new DateNode("20 May 2002")
		)
	));
	// ill formed predicates
	// Some have already been tested in TestOperator, so we only test a few
	// special ones.
	NextSubTest();
	TestPredicate(ListNode(new Int32Node(42), new StringNode("hello!")),
				  B_OK, B_NO_INIT);
	TestPredicate(OpNode(B_EQ,
		new StringNode("hello"),
		new OpNode(B_NOT, NULL)
	), B_OK, B_NO_INIT);
	// precedence Push*() over SetPredicate()
	NextSubTest();
	{
		Query query;
		OpNode predicate1(B_CONTAINS,
			new AttributeNode("attribute"),
			new StringNode("hello")
		);
		StringNode predicate2("I'm the loser. :´-(");
		CPPUNIT_ASSERT( predicate1.push(query) == B_OK );
		CPPUNIT_ASSERT( query.SetPredicate(predicate2.toString().String())
						== B_OK );
		BString predicate;
		CPPUNIT_ASSERT( query.GetPredicate(&predicate) == B_OK );
		CPPUNIT_ASSERT( predicate == predicate1.toString() );
	}
	// GetPredicate() clears the stack
	NextSubTest();
	{
		Query query;
		OpNode predicate1(B_CONTAINS,
			new AttributeNode("attribute"),
			new StringNode("hello")
		);
		StringNode predicate2("I'm the winner. :-)");
		CPPUNIT_ASSERT( predicate1.push(query) == B_OK );
		BString predicate;
		CPPUNIT_ASSERT( query.GetPredicate(&predicate) == B_OK );
		CPPUNIT_ASSERT( predicate == predicate1.toString() );
		CPPUNIT_ASSERT( query.SetPredicate(predicate2.toString().String())
						== B_OK );
		CPPUNIT_ASSERT( query.GetPredicate(&predicate) == B_OK );
		CPPUNIT_ASSERT( predicate == predicate2.toString() );
	}
	// PredicateLength() clears the stack
	NextSubTest();
	{
		Query query;
		OpNode predicate1(B_CONTAINS,
			new AttributeNode("attribute"),
			new StringNode("hello")
		);
		StringNode predicate2("I'm the winner. :-)");
		CPPUNIT_ASSERT( predicate1.push(query) == B_OK );
		CPPUNIT_ASSERT( (int32)query.PredicateLength()
						== predicate1.toString().Length() + 1 );
		CPPUNIT_ASSERT( query.SetPredicate(predicate2.toString().String())
						== B_OK );
		BString predicate;
		CPPUNIT_ASSERT( query.GetPredicate(&predicate) == B_OK );
		CPPUNIT_ASSERT( predicate == predicate2.toString() );
	}
	// SetPredicate(), Push*() fail after Fetch()
	NextSubTest();
	{
		Query query;
		CPPUNIT_ASSERT( query.SetPredicate("name=\"ThisShouldNotExist\"")
						== B_OK );
		BVolume volume(dev_for_path("/boot"));
		CPPUNIT_ASSERT( volume.InitCheck() == B_OK );
		CPPUNIT_ASSERT( query.SetVolume(&volume) == B_OK );
		CPPUNIT_ASSERT( query.Fetch() == B_OK );
		CPPUNIT_ASSERT( query.SetPredicate("name=\"ThisShouldNotExistEither\"")
						== B_NOT_ALLOWED );
// R5: Push*()ing a new predicate does work, though it doesn't make any sense
#if TEST_R5
		CPPUNIT_ASSERT( query.PushDate("20 May 2002") == B_OK );
		CPPUNIT_ASSERT( query.PushValue((int32)42) == B_OK );
		CPPUNIT_ASSERT( query.PushValue((uint32)42) == B_OK );
		CPPUNIT_ASSERT( query.PushValue((int64)42) == B_OK );
		CPPUNIT_ASSERT( query.PushValue((uint64)42) == B_OK );
		CPPUNIT_ASSERT( query.PushValue((float)42) == B_OK );
		CPPUNIT_ASSERT( query.PushValue((double)42) == B_OK );
		CPPUNIT_ASSERT( query.PushValue("hello") == B_OK );
		CPPUNIT_ASSERT( query.PushAttr("attribute") == B_OK );
		CPPUNIT_ASSERT( query.PushOp(B_EQ) == B_OK );
#else
		CPPUNIT_ASSERT( query.PushDate("20 May 2002") == B_NOT_ALLOWED );
		CPPUNIT_ASSERT( query.PushValue((int32)42) == B_NOT_ALLOWED );
		CPPUNIT_ASSERT( query.PushValue((uint32)42) == B_NOT_ALLOWED );
		CPPUNIT_ASSERT( query.PushValue((int64)42) == B_NOT_ALLOWED );
		CPPUNIT_ASSERT( query.PushValue((uint64)42) == B_NOT_ALLOWED );
		CPPUNIT_ASSERT( query.PushValue((float)42) == B_NOT_ALLOWED );
		CPPUNIT_ASSERT( query.PushValue((double)42) == B_NOT_ALLOWED );
		CPPUNIT_ASSERT( query.PushValue("hello") == B_NOT_ALLOWED );
		CPPUNIT_ASSERT( query.PushAttr("attribute") == B_NOT_ALLOWED );
		CPPUNIT_ASSERT( query.PushOp(B_EQ) == B_NOT_ALLOWED );
#endif
	}
	// SetPredicate(): bad args
// R5: crashes when passing NULL to Set/GetPredicate()
#if !TEST_R5
	NextSubTest();
	{
		Query query;
		CPPUNIT_ASSERT( query.SetPredicate(NULL) == B_BAD_VALUE );
		CPPUNIT_ASSERT( query.SetPredicate("hello") == B_OK );
		CPPUNIT_ASSERT( query.GetPredicate(NULL) == B_BAD_VALUE );
		CPPUNIT_ASSERT( query.GetPredicate(NULL, 10) == B_BAD_VALUE );
	}
#endif
}

// ParameterTest
void
QueryTest::ParameterTest()
{
	// tests:
	// * SetVolume, TargetDevice()
	// * SetTarget(), IsLive()

	// SetVolume(), TargetDevice()
	// uninitialized BQuery
	NextSubTest();
	{
		BQuery query;
		CPPUNIT_ASSERT( query.TargetDevice() == B_ERROR );
	}
	// NULL volume
// R5: crashs when passing a NULL BVolume
#if !TEST_R5
	NextSubTest();
	{
		BQuery query;
		CPPUNIT_ASSERT( query.SetVolume(NULL) == B_BAD_VALUE );
		CPPUNIT_ASSERT( query.TargetDevice() == B_ERROR );
	}
#endif
	// invalid volume
	NextSubTest();
	{
		BQuery query;
		BVolume volume(-2);
		CPPUNIT_ASSERT( volume.InitCheck() == B_BAD_VALUE );
		CPPUNIT_ASSERT( query.SetVolume(&volume) == B_OK );
		CPPUNIT_ASSERT( query.TargetDevice() == B_ERROR );
	}
	// valid volume
	NextSubTest();
	{
		BQuery query;
		dev_t device = dev_for_path("/boot");
		BVolume volume(device);
		CPPUNIT_ASSERT( volume.InitCheck() == B_OK );
		CPPUNIT_ASSERT( query.SetVolume(&volume) == B_OK );
		CPPUNIT_ASSERT( query.TargetDevice() == device );
	}

	// SetTarget(), IsLive()
	// uninitialized BQuery
	NextSubTest();
	{
		BQuery query;
		CPPUNIT_ASSERT( query.IsLive() == false );
	}
	// uninitialized BMessenger
	NextSubTest();
	{
		BQuery query;
		BMessenger messenger;
		CPPUNIT_ASSERT( messenger.IsValid() == false );
		CPPUNIT_ASSERT( query.SetTarget(messenger) == B_BAD_VALUE );
		CPPUNIT_ASSERT( query.IsLive() == false );
	}
	// valid BMessenger
	NextSubTest();
	{
		BQuery query;
		BMessenger messenger(&fApplication->Handler());
		CPPUNIT_ASSERT( messenger.IsValid() == true );
		CPPUNIT_ASSERT( query.SetTarget(messenger) == B_OK );
		CPPUNIT_ASSERT( query.IsLive() == true );
	}

	// SetVolume/Target() fail after Fetch()
	NextSubTest();
	{
		Query query;
		CPPUNIT_ASSERT( query.SetPredicate("name=\"ThisShouldNotExist\"")
						== B_OK );
		BVolume volume(dev_for_path("/boot"));
		CPPUNIT_ASSERT( volume.InitCheck() == B_OK );
		CPPUNIT_ASSERT( query.SetVolume(&volume) == B_OK );
		CPPUNIT_ASSERT( query.Fetch() == B_OK );
		CPPUNIT_ASSERT( query.SetVolume(&volume) == B_NOT_ALLOWED );
		BMessenger messenger(&fApplication->Handler());
		CPPUNIT_ASSERT( messenger.IsValid() == true );
		CPPUNIT_ASSERT( query.SetTarget(messenger) == B_NOT_ALLOWED );
	}

	// Fetch() fails without a valid volume set
	NextSubTest();
	{
		Query query;
		CPPUNIT_ASSERT( query.SetPredicate("name=\"ThisShouldNotExist\"")
						== B_OK );
		CPPUNIT_ASSERT( query.Fetch() == B_NO_INIT );
	}
}

// TestFetchPredicateInit
static
void
TestFetchPredicateInit(Query &query, TestSet &testSet, const char *mountPoint,
					   const char *predicate, QueryTestEntry **entries,
					   int32 entryCount)
{
	// init the query
	CPPUNIT_ASSERT( query.SetPredicate(predicate) == B_OK );
	BVolume volume(dev_for_path(mountPoint));
	CPPUNIT_ASSERT( volume.InitCheck() == B_OK );
	CPPUNIT_ASSERT( query.SetVolume(&volume) == B_OK );
	CPPUNIT_ASSERT( query.Fetch() == B_OK );
	// init the test set
	testSet.clear();
	for (int32 i = 0; i < entryCount; i++)
		testSet.add(entries[i]->path);
}


// TestFetchPredicate
static
void
TestFetchPredicate(const char *mountPoint, const char *predicate,
				   QueryTestEntry **entries, int32 entryCount)
{
	// GetNextEntry()
	{
		Query query;
		TestSet testSet;
		TestFetchPredicateInit(query, testSet, mountPoint, predicate, entries,
							   entryCount);
		BEntry entry;
		while (query.GetNextEntry(&entry) == B_OK) {
// Haiku supports rewinding queries, R5 does not.
#ifdef TEST_R5
			CPPUNIT_ASSERT( query.Rewind() == B_ERROR );
#endif
			CPPUNIT_ASSERT( query.CountEntries() == B_ERROR );
			BPath path;
			CPPUNIT_ASSERT( entry.InitCheck() == B_OK );
			CPPUNIT_ASSERT( entry.GetPath(&path) == B_OK );
			CPPUNIT_ASSERT( testSet.test(path.Path()) == true );
		}
		CPPUNIT_ASSERT( testSet.testDone() == true );
		CPPUNIT_ASSERT( query.GetNextEntry(&entry) == B_ENTRY_NOT_FOUND );
	}
	// GetNextRef()
	{
		Query query;
		TestSet testSet;
		TestFetchPredicateInit(query, testSet, mountPoint, predicate, entries,
							   entryCount);
		entry_ref ref;
		while (query.GetNextRef(&ref) == B_OK) {
// Haiku supports rewinding queries, R5 does not.
#ifdef TEST_R5
			CPPUNIT_ASSERT( query.Rewind() == B_ERROR );
#endif
			CPPUNIT_ASSERT( query.CountEntries() == B_ERROR );
			BPath path(&ref);
			CPPUNIT_ASSERT( path.InitCheck() == B_OK );
			CPPUNIT_ASSERT( testSet.test(path.Path()) == true );
		}
		CPPUNIT_ASSERT( testSet.testDone() == true );
		CPPUNIT_ASSERT( query.GetNextRef(&ref) == B_ENTRY_NOT_FOUND );
	}
	// GetNextDirents()
	{
		Query query;
		TestSet testSet;
		TestFetchPredicateInit(query, testSet, mountPoint, predicate, entries,
							   entryCount);
		size_t bufSize = (sizeof(dirent) + B_FILE_NAME_LENGTH) * 10;
		char buffer[bufSize];
		dirent *ents = (dirent *)buffer;
		while (query.GetNextDirents(ents, bufSize, 1) == 1) {
// Haiku supports rewinding queries, R5 does not.
#ifdef TEST_R5
			CPPUNIT_ASSERT( query.Rewind() == B_ERROR );
#endif
			CPPUNIT_ASSERT( query.CountEntries() == B_ERROR );
			entry_ref ref(ents->d_pdev, ents->d_pino, ents->d_name);
			BPath path(&ref);
			CPPUNIT_ASSERT( path.InitCheck() == B_OK );
			CPPUNIT_ASSERT( testSet.test(path.Path()) == true );
		}
		CPPUNIT_ASSERT( testSet.testDone() == true );
		CPPUNIT_ASSERT( query.GetNextDirents(ents, bufSize, 1) == 0 );
	}
	// interleaving use of the different methods
	{
		Query query;
		TestSet testSet;
		TestFetchPredicateInit(query, testSet, mountPoint, predicate, entries,
							   entryCount);
		size_t bufSize = (sizeof(dirent) + B_FILE_NAME_LENGTH) * 10;
		char buffer[bufSize];
		dirent *ents = (dirent *)buffer;
		entry_ref ref;
		BEntry entry;
		while (query.GetNextDirents(ents, bufSize, 1) == 1) {
// Haiku supports rewinding queries, R5 does not.
#ifdef TEST_R5
			CPPUNIT_ASSERT( query.Rewind() == B_ERROR );
#endif
			CPPUNIT_ASSERT( query.CountEntries() == B_ERROR );
			entry_ref entref(ents->d_pdev, ents->d_pino, ents->d_name);
			BPath entpath(&entref);
			CPPUNIT_ASSERT( entpath.InitCheck() == B_OK );
			CPPUNIT_ASSERT( testSet.test(entpath.Path()) == true );
			if (query.GetNextRef(&ref) == B_OK) {
				BPath refpath(&ref);
				CPPUNIT_ASSERT( refpath.InitCheck() == B_OK );
				CPPUNIT_ASSERT( testSet.test(refpath.Path()) == true );
			}
			if (query.GetNextEntry(&entry) == B_OK) {
				BPath path;
				CPPUNIT_ASSERT( entry.InitCheck() == B_OK );
				CPPUNIT_ASSERT( entry.GetPath(&path) == B_OK );
				CPPUNIT_ASSERT( testSet.test(path.Path()) == true );
			}
		}
		CPPUNIT_ASSERT( query.GetNextEntry(&entry) == B_ENTRY_NOT_FOUND );
		CPPUNIT_ASSERT( query.GetNextRef(&ref) == B_ENTRY_NOT_FOUND );
		CPPUNIT_ASSERT( query.GetNextDirents(ents, bufSize, 1) == 0 );
	}
}

// FetchTest
void
QueryTest::FetchTest()
{
	// tests:
	// * Clear()/Fetch()
	// * BEntryList interface

	// Fetch()
	// uninitialized BQuery
	NextSubTest();
	{
		Query query;
		CPPUNIT_ASSERT( query.Fetch() == B_NO_INIT );
	}
	// incompletely initialized BQuery (no predicate)
	NextSubTest();
	{
		Query query;
		BVolume volume(dev_for_path("/boot"));
		CPPUNIT_ASSERT( volume.InitCheck() == B_OK );
		CPPUNIT_ASSERT( query.SetVolume(&volume) == B_OK );
		CPPUNIT_ASSERT( query.Fetch() == B_NO_INIT );
	}
	// incompletely initialized BQuery (no volume)
	NextSubTest();
	{
		Query query;
		CPPUNIT_ASSERT( query.SetPredicate("name=\"ThisShouldNotExist\"")
						== B_OK );
		CPPUNIT_ASSERT( query.Fetch() == B_NO_INIT );
	}
	// incompletely initialized BQuery (invalid predicate)
	NextSubTest();
	{
		Query query;
		CPPUNIT_ASSERT( query.SetPredicate("name=\"ThisShouldNotExist\"&&")
						== B_OK );
		BVolume volume(dev_for_path("/boot"));
		CPPUNIT_ASSERT( volume.InitCheck() == B_OK );
		CPPUNIT_ASSERT( query.SetVolume(&volume) == B_OK );
		CPPUNIT_ASSERT( query.Fetch() == B_BAD_VALUE );
	}
	// initialized BQuery, Fetch() twice
	NextSubTest();
	{
		Query query;
		CPPUNIT_ASSERT( query.SetPredicate("name=\"ThisShouldNotExist\"")
						== B_OK );
		BVolume volume(dev_for_path("/boot"));
		CPPUNIT_ASSERT( volume.InitCheck() == B_OK );
		CPPUNIT_ASSERT( query.SetVolume(&volume) == B_OK );
		CPPUNIT_ASSERT( query.Fetch() == B_OK );
		CPPUNIT_ASSERT( query.Fetch() == B_NOT_ALLOWED );
	}
	// initialized BQuery, successful Fetch(), different predicates
	createVolume(testVolumeImage, testMountPoint, 2);
	fVolumeCreated = true;
	create_test_entries(allTestEntries, allTestEntryCount);
	// ... all files
	NextSubTest();
	{
		QueryTestEntry *entries[] = {
			&file11, &file12, &file21, &file22, &file31, &file32, &file1,
			&file2, &file3
		};
		const int32 entryCount = sizeof(entries) / sizeof(QueryTestEntry*);
		TestFetchPredicate(testMountPoint, "name=\"file*\"", entries,
						   entryCount);
	}
	// ... all entries containing a "l"
	NextSubTest();
	{
		QueryTestEntry *entries[] = {
			&file11, &file12, &link11, &file21, &file22, &link21, &file31,
			&file32, &link31, &file1, &file2, &file3, &link1, &link2, &link3
		};
		const int32 entryCount = sizeof(entries) / sizeof(QueryTestEntry*);
		TestFetchPredicate(testMountPoint, "name=\"*l*\"", entries,
						   entryCount);
	}
	// ... all entries ending on "2"
	NextSubTest();
	{
		QueryTestEntry *entries[] = {
			&subdir12, &file12, &dir2, &subdir22, &file22, &subdir32, &file32,
			&file2, &link2
		};
		const int32 entryCount = sizeof(entries) / sizeof(QueryTestEntry*);
		TestFetchPredicate(testMountPoint, "name=\"*2\"", entries,
						   entryCount);
	}

	// Clear()
	// uninitialized BQuery
	NextSubTest();
	{
		Query query;
		CPPUNIT_ASSERT( query.Clear() == B_OK );
	}
	// initialized BQuery, Fetch(), Clear(), Fetch()
	NextSubTest();
	{
		Query query;
		CPPUNIT_ASSERT( query.SetPredicate("name=\"ThisShouldNotExist\"")
						== B_OK );
		BVolume volume(dev_for_path("/boot"));
		CPPUNIT_ASSERT( volume.InitCheck() == B_OK );
		CPPUNIT_ASSERT( query.SetVolume(&volume) == B_OK );
		CPPUNIT_ASSERT( query.Fetch() == B_OK );
		CPPUNIT_ASSERT( query.Clear() == B_OK );
		CPPUNIT_ASSERT( query.Fetch() == B_NO_INIT );
	}
	// initialized BQuery, Fetch(), Clear(), re-init, Fetch()
	NextSubTest();
	{
		Query query;
		CPPUNIT_ASSERT( query.SetPredicate("name=\"ThisShouldNotExist\"")
						== B_OK );
		BVolume volume(dev_for_path("/boot"));
		CPPUNIT_ASSERT( volume.InitCheck() == B_OK );
		CPPUNIT_ASSERT( query.SetVolume(&volume) == B_OK );
		CPPUNIT_ASSERT( query.Fetch() == B_OK );
		CPPUNIT_ASSERT( query.Clear() == B_OK );
		CPPUNIT_ASSERT( query.SetPredicate("name=\"ThisShouldNotExist\"")
						== B_OK );
		CPPUNIT_ASSERT( volume.SetTo(dev_for_path("/boot")) == B_OK );
		CPPUNIT_ASSERT( query.SetVolume(&volume) == B_OK );
		CPPUNIT_ASSERT( query.Fetch() == B_OK );
	}

	// BEntryList interface:
	// empty queries
	NextSubTest();
	{
		Query query;
		CPPUNIT_ASSERT( query.SetPredicate("name=\"ThisShouldNotExist\"")
						== B_OK );
		BVolume volume(dev_for_path("/boot"));
		CPPUNIT_ASSERT( volume.InitCheck() == B_OK );
		CPPUNIT_ASSERT( query.SetVolume(&volume) == B_OK );
		CPPUNIT_ASSERT( query.Fetch() == B_OK );
		BEntry entry;
		entry_ref ref;
		size_t bufSize = (sizeof(dirent) + B_FILE_NAME_LENGTH) * 10;
		char buffer[bufSize];
		dirent *ents = (dirent *)buffer;
		CPPUNIT_ASSERT( query.GetNextEntry(&entry) == B_ENTRY_NOT_FOUND );
		CPPUNIT_ASSERT( query.GetNextRef(&ref) == B_ENTRY_NOT_FOUND );
		CPPUNIT_ASSERT( query.GetNextDirents(ents, bufSize, 1) == 0 );
	}
	// uninitialized queries
	NextSubTest();
	{
		Query query;
		BEntry entry;
		entry_ref ref;
		size_t bufSize = (sizeof(dirent) + B_FILE_NAME_LENGTH) * 10;
		char buffer[bufSize];
		dirent *ents = (dirent *)buffer;
		CPPUNIT_ASSERT( query.GetNextEntry(&entry) == B_FILE_ERROR );
		CPPUNIT_ASSERT( query.GetNextRef(&ref) == B_FILE_ERROR );
		CPPUNIT_ASSERT( query.GetNextDirents(ents, bufSize, 1)
						== B_FILE_ERROR );
	}
	// bad args
	NextSubTest();
	{
		Query query;
		CPPUNIT_ASSERT( query.SetPredicate("name=\"ThisShouldNotExist\"")
						== B_OK );
		BVolume volume(dev_for_path("/boot"));
		CPPUNIT_ASSERT( volume.InitCheck() == B_OK );
		CPPUNIT_ASSERT( query.SetVolume(&volume) == B_OK );
		CPPUNIT_ASSERT( query.Fetch() == B_OK );
		size_t bufSize = (sizeof(dirent) + B_FILE_NAME_LENGTH) * 10;
// R5: crashs when passing a NULL BEntry or entry_ref
#if !TEST_R5
		CPPUNIT_ASSERT( query.GetNextEntry(NULL) == B_BAD_VALUE );
		CPPUNIT_ASSERT( query.GetNextRef(NULL) == B_BAD_VALUE );
#endif
		CPPUNIT_ASSERT( equals(query.GetNextDirents(NULL, bufSize, 1),
							   B_BAD_ADDRESS, B_BAD_VALUE) );
	}
}

// AddLiveEntries
void
QueryTest::AddLiveEntries(QueryTestEntry **entries, int32 entryCount,
						  QueryTestEntry **queryEntries, int32 queryEntryCount)
{
	create_test_entries(entries, entryCount);
	for (int32 i = 0; i < entryCount; i++) {
		QueryTestEntry *entry = entries[i];
		BNode node(entry->cpath);
		CPPUNIT_ASSERT( node.InitCheck() == B_OK );
		node_ref nref;
		CPPUNIT_ASSERT( node.GetNodeRef(&nref) == B_OK );
		entry->node = nref.node;
		entry_ref ref;
		CPPUNIT_ASSERT( get_ref_for_path(entry->cpath, &ref) == B_OK );
		entry->directory = ref.directory;
		entry->name = ref.name;
	}
	CheckUpdateMessages(B_ENTRY_CREATED, queryEntries, queryEntryCount);
}

// RemoveLiveEntries
void
QueryTest::RemoveLiveEntries(QueryTestEntry **entries, int32 entryCount,
							 QueryTestEntry **queryEntries,
							 int32 queryEntryCount)
{
	delete_test_entries(entries, entryCount);
	CheckUpdateMessages(B_ENTRY_REMOVED, queryEntries, queryEntryCount);
	for (int32 i = 0; i < entryCount; i++) {
		QueryTestEntry *entry = entries[i];
		entry->directory = -1;
		entry->node = -1;
		entry->name = "";
	}
}

// CheckUpdateMessages
void
QueryTest::CheckUpdateMessages(uint32 opcode, QueryTestEntry **entries,
							   int32 entryCount)
{

	// wait for the messages
	snooze(100000);
	if (fApplication) {
		BMessageQueue &queue = fApplication->Handler().Queue();
		CPPUNIT_ASSERT( queue.Lock() );
		try {
			int32 entryNum = 0;
			while (BMessage *_message = queue.NextMessage()) {
				BMessage message(*_message);
				delete _message;
				CPPUNIT_ASSERT( entryNum < entryCount );
				QueryTestEntry *entry = entries[entryNum];
				CPPUNIT_ASSERT( message.what == B_QUERY_UPDATE );
				uint32 msgOpcode;
				CPPUNIT_ASSERT( message.FindInt32("opcode", (int32*)&msgOpcode)
								== B_OK );
				CPPUNIT_ASSERT( msgOpcode == opcode );
				dev_t device;
				CPPUNIT_ASSERT( message.FindInt32("device", &device)
								== B_OK );
				CPPUNIT_ASSERT( device == dev_for_path(testMountPoint) );
				ino_t directory;
				CPPUNIT_ASSERT( message.FindInt64("directory", &directory)
								== B_OK );
				CPPUNIT_ASSERT( directory == entry->directory );
				ino_t node;
				CPPUNIT_ASSERT( message.FindInt64("node", &node)
								== B_OK );
				CPPUNIT_ASSERT( node == entry->node );
				if (opcode == B_ENTRY_CREATED) {
					const char *name;
					CPPUNIT_ASSERT( message.FindString("name", &name)
									== B_OK );
					CPPUNIT_ASSERT( entry->name == name );
				}
				entryNum++;
			}
			CPPUNIT_ASSERT( entryNum == entryCount );
		} catch (CppUnit::Exception exception) {
			queue.Unlock();
			throw exception;
		}
		queue.Unlock();
	}
}

// LiveTest
void
QueryTest::LiveTest()
{
	// tests:
	// * live queries
	CPPUNIT_ASSERT( fApplication != NULL );
	createVolume(testVolumeImage, testMountPoint, 2);
	fVolumeCreated = true;
	create_test_entries(allTestEntries, allTestEntryCount);
	BMessenger target(&fApplication->Handler());

	// empty query, add some files, remove some files
	NextSubTest();
	{
		Query query;
		CPPUNIT_ASSERT( query.SetPredicate("name=\"*Argh\"")
						== B_OK );
		BVolume volume(dev_for_path(testMountPoint));
		CPPUNIT_ASSERT( volume.InitCheck() == B_OK );
		CPPUNIT_ASSERT( query.SetVolume(&volume) == B_OK );
		CPPUNIT_ASSERT( query.SetTarget(target) == B_OK );
		CPPUNIT_ASSERT( query.Fetch() == B_OK );
		BEntry entry;
		CPPUNIT_ASSERT( query.GetNextEntry(&entry) == B_ENTRY_NOT_FOUND );
		// the test entries
		QueryTestEntry testDir1(dir1 + "testDirArgh", B_DIRECTORY_NODE);
		QueryTestEntry testDir2(dir1 + "testDir2", B_DIRECTORY_NODE);
		QueryTestEntry testFile1(subdir21 + "testFileArgh", B_FILE_NODE);
		QueryTestEntry testFile2(subdir21 + "testFile2", B_FILE_NODE);
		QueryTestEntry testLink1(subdir32 + "testLinkArgh", B_SYMLINK_NODE,
								 &file11);
		QueryTestEntry testLink2(subdir32 + "testLink2", B_SYMLINK_NODE,
								 &file11);
		QueryTestEntry *entries[] = {
			&testDir1, &testDir2, &testFile1, &testFile2,
			&testLink1, &testLink2
		};
		int32 entryCount = sizeof(entries) / sizeof(QueryTestEntry*);
		QueryTestEntry *queryEntries[] = {
			&testDir1, &testFile1, &testLink1
		};
		int32 queryEntryCount = sizeof(queryEntries) / sizeof(QueryTestEntry*);
		AddLiveEntries(entries, entryCount, queryEntries, queryEntryCount);
		RemoveLiveEntries(entries, entryCount, queryEntries, queryEntryCount);
	}
	// non-empty query, add some files, remove some files
	NextSubTest();
	{
		Query query;
		TestSet testSet;
		CPPUNIT_ASSERT( query.SetTarget(target) == B_OK );
		QueryTestEntry *initialEntries[] = {
			&file11, &file12, &file21, &file22, &file31, &file32, &file1,
			&file2, &file3
		};
		int32 initialEntryCount
			= sizeof(initialEntries) / sizeof(QueryTestEntry*);
		TestFetchPredicateInit(query, testSet, testMountPoint,
							   "name=\"*ile*\"", initialEntries,
							   initialEntryCount);
		BEntry entry;
		while (query.GetNextEntry(&entry) == B_OK) {
			BPath path;
			CPPUNIT_ASSERT( entry.InitCheck() == B_OK );
			CPPUNIT_ASSERT( entry.GetPath(&path) == B_OK );
			CPPUNIT_ASSERT( testSet.test(path.Path()) == true );
		}
		CPPUNIT_ASSERT( testSet.testDone() == true );
		CPPUNIT_ASSERT( query.GetNextEntry(&entry) == B_ENTRY_NOT_FOUND );
		// the test entries
		QueryTestEntry testDir1(dir1 + "testDir1", B_DIRECTORY_NODE);
		QueryTestEntry testDir2(dir1 + "testDir2", B_DIRECTORY_NODE);
		QueryTestEntry testFile1(subdir21 + "testFile1", B_FILE_NODE);
		QueryTestEntry testFile2(subdir21 + "testFile2", B_FILE_NODE);
		QueryTestEntry testLink1(subdir32 + "testLink1", B_SYMLINK_NODE,
								 &file11);
		QueryTestEntry testLink2(subdir32 + "testLink2", B_SYMLINK_NODE,
								 &file11);
		QueryTestEntry testFile3(subdir32 + "testFile3", B_FILE_NODE);
		QueryTestEntry *entries[] = {
			&testDir1, &testDir2, &testFile1, &testFile2,
			&testLink1, &testLink2, &testFile3
		};
		int32 entryCount = sizeof(entries) / sizeof(QueryTestEntry*);
		QueryTestEntry *queryEntries[] = {
			&testFile1, &testFile2, &testFile3
		};
		int32 queryEntryCount = sizeof(queryEntries) / sizeof(QueryTestEntry*);
		AddLiveEntries(entries, entryCount, queryEntries, queryEntryCount);
		RemoveLiveEntries(entries, entryCount, queryEntries, queryEntryCount);
	}
}





