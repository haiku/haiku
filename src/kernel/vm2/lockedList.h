#ifndef _LOCKED_LIST_H
#define _LOCKED_LIST_H
#include <OS.h>
#include <list.h>
class lockedList: public list
{
	public:
		lockedList(void) {myLock=create_sem(1,"lockedListSem");}
		void add(node *newNode) {lock();list::add(newNode);unlock();}
		node *next(void) {lock();node *retVal=list::next();unlock();return retVal;}
		void remove(node *newNode) {lock();list::remove(newNode);unlock();}
		void dump() {lock();list::dump();unlock();}
		bool ensureSane(void) {lock();bool retVal=list::ensureSane();unlock();return retVal;}
		void lock() { acquire_sem(myLock);}
		void unlock() { release_sem(myLock);}
	private:
		sem_id myLock;
};
#endif
