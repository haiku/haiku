// KPartitionVisitor.cpp

#include "KPartitionVisitor.h"
#include <util/kernel_cpp.h>

// constructor
KPartitionVisitor::KPartitionVisitor()
{
}

// destructor
KPartitionVisitor::~KPartitionVisitor()
{
}

// VisitPre
bool
KPartitionVisitor::VisitPre(KPartition *partition)
{
	return false;
}

// VisitPost
bool
KPartitionVisitor::VisitPost(KPartition *partition)
{
	return false;
}

