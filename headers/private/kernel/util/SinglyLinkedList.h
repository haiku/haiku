#ifndef _SINGLY_LINKED_LIST_H_
#define _SINGLY_LINKED_LIST_H_

#include "MallocFreeAllocator.h"
#include "SupportDefs.h"

namespace Strategy {
	namespace SinglyLinkedList {
		//! Automatic node strategy (works like STL containers do)
		namespace Private {
			template <class Value>
			class ListNode
			{
			public:
				typedef Value ValueType;
				
				ListNode(const ValueType &data)
					: data(data)
					, next(NULL)
				{
				}
				
				ValueType data;
				ListNode<Value> *next;
			};
		}	// namespace Private
	
		template <typename Value, template <class> class Allocator = MallocFreeAllocator>
		class Auto
		{
		public:	
			typedef Private::ListNode<Value> NodeType;
			typedef Value ValueType;
		
			inline NodeType *Allocate(const ValueType &data)
			{
				NodeType* result = fAllocator.Allocate();
				fAllocator.Construct(result, data);
				return result;
			}
		
			inline void Free(NodeType *node)
			{
				fAllocator.Destruct(node);
				fAllocator.Deallocate(node);
			}
			
			inline ValueType& GetValue(NodeType *node) const {
				return node->data;
			}
			
			inline NodeType* GetNext(NodeType *node) const {
				return node->next;
			}
		
			inline NodeType* SetNext(NodeType *node, NodeType* next) const {
				return node->next = next;
			}
		
			inline NodeType* GetPrevious(NodeType *node) const {
				return node->previous;
			}
		
			inline NodeType* SetPrevious(NodeType *node, NodeType* previous) const {
				return node->previous = previous;
			}
		
		protected:
			Allocator<NodeType> fAllocator;
		};
		
	
		//! User managed node strategy (user is responsible for node allocation/deallocation)
		template <class Node, Node* Node::* NextMember = &Node::next>
		class User;		
	}
}

template <class Value, class Reference, class Pointer, class Parent>
class SinglyLinkedListIterator;	

template <class Value, class NodeStrategy = Strategy::SinglyLinkedList::Auto<Value> >
class SinglyLinkedList
{	
public:
	typedef SinglyLinkedList<Value, NodeStrategy> Type;

	typedef typename NodeStrategy::NodeType NodeType;
	typedef typename NodeStrategy::ValueType ValueType;

	typedef NodeType* NodeType::* NodePointerMember;
	
	typedef SinglyLinkedListIterator<ValueType, ValueType&, ValueType*, Type> Iterator;
	typedef SinglyLinkedListIterator<ValueType, const ValueType&, const ValueType*, const Type> ConstIterator;
	
	SinglyLinkedList()
		: fFirst(NULL)
		, fLast(NULL)
		, fCount(0)
	{
	}
	~SinglyLinkedList();
	
	ssize_t Count() const;
	bool IsEmpty() const;
	void MakeEmpty();

	status_t PushFront(const ValueType &data);
	status_t PushBack(const ValueType &data);
	
	void PopFront();
	
	Iterator Begin();
	ConstIterator Begin() const;
	Iterator End();
	ConstIterator End() const;
	Iterator Null();
	ConstIterator Null() const;
	
	ssize_t Remove(const ValueType &value);
	Iterator Erase(Iterator &pos);

protected:
	friend class Iterator;
	friend class ConstIterator;

	inline NodeType *Allocate(const ValueType &data) { return fStrategy.Allocate(data); }
	inline void Free(NodeType *node) { fStrategy.Free(node); }
	inline ValueType& GetValue(NodeType *node) const { return fStrategy.GetValue(node); }
	inline NodeType* GetNext(NodeType *node) const { return fStrategy.GetNext(node); }
	inline NodeType* SetNext(NodeType *node, NodeType* next) const { return fStrategy.SetNext(node, next); }

	NodeStrategy fStrategy;

	NodeType *fFirst;
	NodeType *fLast;
	
	ssize_t fCount;
};

//--------------------------------------------------------------------------
// SinglyLinkedListIterator
//--------------------------------------------------------------------------
	
template <class Value, class Reference, class Pointer, class Parent>
class SinglyLinkedListIterator {
public:
	typedef SinglyLinkedListIterator<Value, Reference, Pointer, Parent> IteratorType;
	typedef typename Parent::NodeType NodeType;

	SinglyLinkedListIterator();
	SinglyLinkedListIterator(Parent *parent, NodeType *node, NodeType *previous);
	SinglyLinkedListIterator(const IteratorType &ref);
	
	bool operator==(const IteratorType &ref) const;
	bool operator!=(const IteratorType &ref) const;
	IteratorType &operator=(const IteratorType &ref);
	inline Reference operator*() const;
	inline Pointer operator->() const;
	IteratorType &operator++();
	IteratorType operator++(int);
					
private:
	Parent *fParent;
	NodeType *fNode;
	NodeType *fPrevious;
};
	
#define _ITERATOR_TEMPLATE_LIST template <class Value, class Reference, class Pointer, class Parent>
#define _ITERATOR SinglyLinkedListIterator<Value, Reference, Pointer, Parent>

_ITERATOR_TEMPLATE_LIST
_ITERATOR::SinglyLinkedListIterator()
	: fParent(NULL)
	, fNode(NULL)
	, fPrevious(NULL)
{
}

_ITERATOR_TEMPLATE_LIST
_ITERATOR::SinglyLinkedListIterator(Parent *parent, NodeType *node, NodeType *previous)
	: fParent(parent)
	, fNode(node)
	, fPrevious(previous)
{
}

_ITERATOR_TEMPLATE_LIST
_ITERATOR::SinglyLinkedListIterator(const IteratorType &ref)
	: fParent(ref.fParent)
	, fNode(ref.fNode)
	, fPrevious(ref.fPrevious)
{
}

_ITERATOR_TEMPLATE_LIST
bool
_ITERATOR::operator==(const _ITERATOR &ref) const
{
	return fParent == ref.fParent && fNode == ref.fNode && fPrevious == ref.fPrevious;
}

_ITERATOR_TEMPLATE_LIST
bool
_ITERATOR::operator!=(const _ITERATOR &ref) const
{
	return !operator==(ref);
}

_ITERATOR_TEMPLATE_LIST
_ITERATOR&
_ITERATOR::operator=(const _ITERATOR &ref)
{
	fParent = ref.fParent;
	fNode = ref.fNode;
	fPrevious = ref.fPrevious;
	return *this;
}

_ITERATOR_TEMPLATE_LIST
inline
Reference
_ITERATOR::operator*() const
{
	return fParent->GetValue(fNode);			
}
		
_ITERATOR_TEMPLATE_LIST
Pointer
_ITERATOR::operator->() const
{
	return &(operator*());			
}
		
_ITERATOR_TEMPLATE_LIST
_ITERATOR&
_ITERATOR::operator++() {
	if (fNode) {
		fPrevious = fNode;
		fNode = fParent->GetNext(fNode);
	}
	return *this;
}
		
_ITERATOR_TEMPLATE_LIST
_ITERATOR
_ITERATOR::operator++(int) {
	IteratorType old = *this;
	++*this;
	return old;
}

//--------------------------------------------------------------------------
// SinglyLinkedList implementation
//--------------------------------------------------------------------------
	
#define _SINGLY_LINKED_LIST_TEMPLATE_LIST template <class Value, class NodeStrategy>
#define _SINGLY_LINKED_LIST SinglyLinkedList<Value, NodeStrategy>

_SINGLY_LINKED_LIST_TEMPLATE_LIST
_SINGLY_LINKED_LIST::~SinglyLinkedList()
{
	MakeEmpty();
}

_SINGLY_LINKED_LIST_TEMPLATE_LIST
ssize_t
_SINGLY_LINKED_LIST::Count() const
{
	return fCount;
}

_SINGLY_LINKED_LIST_TEMPLATE_LIST
bool
_SINGLY_LINKED_LIST::IsEmpty() const
{
	return Count() == 0;
}

_SINGLY_LINKED_LIST_TEMPLATE_LIST
void
_SINGLY_LINKED_LIST::MakeEmpty()
{
	for (NodeType *node = fFirst; node; ) {
		NodeType* next = GetNext(node);
		Free(node);
		node = next;
	}
	fFirst = fLast = NULL;
	fCount = 0;
}

_SINGLY_LINKED_LIST_TEMPLATE_LIST
status_t
_SINGLY_LINKED_LIST::PushFront(const ValueType &data)
{
	status_t err = B_OK;
	
	if (!fFirst) {
		fFirst = fLast = Allocate(data);
		if (fFirst)
			SetNext(fFirst, NULL);
		else
			err = B_NO_MEMORY;
	} else {
		NodeType* node = Allocate(data);
		if (node) {
			SetNext(node, fFirst);
			fFirst = node;			
		} else
			err = B_NO_MEMORY;
	}
	
	if (!err)
		++fCount;
	
	return err;
}

_SINGLY_LINKED_LIST_TEMPLATE_LIST
status_t
_SINGLY_LINKED_LIST::PushBack(const ValueType &data)
{
	status_t err = B_OK;
	
	if (!fLast) {
		fFirst = fLast = Allocate(data);
		if (fFirst)
			SetNext(fFirst, NULL);
		else
			err = B_NO_MEMORY;
	} else {
		NodeType* node = Allocate(data);
		if (node) {
			SetNext(fLast, node);
			SetNext(node, NULL);
			fLast = node;	
		} else
			err = B_NO_MEMORY;
	}
	
	if (!err)
		++fCount;
	
	return err;
}

_SINGLY_LINKED_LIST_TEMPLATE_LIST
void
_SINGLY_LINKED_LIST::PopFront()
{
	if (fFirst) {
		if (fFirst == fLast)
			fLast = NULL;
		NodeType* temp = fFirst;
		fFirst = GetNext(fFirst);
		Free(temp);
		--fCount;
	}		
}

_SINGLY_LINKED_LIST_TEMPLATE_LIST
_SINGLY_LINKED_LIST::Iterator
_SINGLY_LINKED_LIST::Begin()
{
	return Iterator(this, fFirst, NULL);
}

_SINGLY_LINKED_LIST_TEMPLATE_LIST
_SINGLY_LINKED_LIST::ConstIterator
_SINGLY_LINKED_LIST::Begin() const
{
	return ConstIterator(this, fFirst, NULL);
}

_SINGLY_LINKED_LIST_TEMPLATE_LIST
_SINGLY_LINKED_LIST::Iterator
_SINGLY_LINKED_LIST::End() 
{
	return Iterator(this, NULL, fLast);
}

_SINGLY_LINKED_LIST_TEMPLATE_LIST
_SINGLY_LINKED_LIST::ConstIterator
_SINGLY_LINKED_LIST::End() const
{
	return ConstIterator(this, NULL, fLast);
}

_SINGLY_LINKED_LIST_TEMPLATE_LIST
_SINGLY_LINKED_LIST::Iterator
_SINGLY_LINKED_LIST::Null() 
{
	return Iterator(this, NULL, NULL);
}

_SINGLY_LINKED_LIST_TEMPLATE_LIST
_SINGLY_LINKED_LIST::ConstIterator
_SINGLY_LINKED_LIST::Null() const
{
	return ConstIterator(this, NULL, NULL);
}

/*! \brief Removes all elements in the list whose value
	matches \c value.
	
	\return The number of elements removed.
*/
_SINGLY_LINKED_LIST_TEMPLATE_LIST
ssize_t
_SINGLY_LINKED_LIST::Remove(const ValueType &value)
{
	ssize_t count = 0;
	for (Iterator i = Begin(); i != End(); ) {
		if (*i == value) {
			i = Erase(i);
			++count;
		} else {
			++i;
		}
	}
	return count;
}

/*! \brief Removes the specified item from the list, returning
	the item that previously followed \c pos.
	
	Note that this operation invalidates any previously valid
	iterators for the given container.
	
	\return An iterator pointing to the item following \c pos
	before the removal, or End() if pos is an invalid argument.
*/
_SINGLY_LINKED_LIST_TEMPLATE_LIST
_SINGLY_LINKED_LIST::Iterator
_SINGLY_LINKED_LIST::Erase(Iterator &pos)
{
	if (pos.fNode) {
		if (pos.fNode == fStart) {
			if (pos.fNode != fLast) {
				// Strictly first node in the list
				fFirst = GetNext(pos.fNode);
				Free(pos.fNode);
				--fCount;
				return Begin();
			} else {
				// Only node in the list
				fFist = fLast = NULL;
				Free(pos.fNode);
				--fCount;
				return End();
			} 		
		} else if (pos.fNode == fLast) {
			// Strictly last node in the list
			fLast = pos.fPrevious;
			SetNext(fLast, NULL);
			Free(pos.fNode);
			--fCount;
			return End();
		} else if (pos.fPrevious) {		
			// Neither first nor last, but at least valid
			NodeType *next = GetNext(pos.fNode);
			SetNext(pos.fPrevious, next);	// fPrev->next = fNode->next
			Free(pos.fNode);
			--fCount;
			return Iterator(this, next, pos.fPrevious);
		}	
	}
	
	// Invalid position for erasing
	return End();	// Better to return Null()?
}

//------------------------------------------------------------------------------
// Strategies
//------------------------------------------------------------------------------

namespace Strategy {
	namespace SinglyLinkedList {

		template <class Node, Node* Node::* NextMember>
		class User
		{
		public:
			typedef Node NodeType;
			typedef Node ValueType;
		
			inline NodeType *Allocate(const ValueType &data)
			{
				return (NodeType*)&data;
			}
		
			inline void Free(NodeType *node)
			{
			}
		
			inline ValueType& GetValue(NodeType *node) const {
				return *node;
			}
			
			inline NodeType* GetNext(NodeType *node) const {
				return node->*NextMember;
			}
		
			inline NodeType* SetNext(NodeType *node, NodeType* next) const {
				return node->*NextMember = next;
			}
		};

	}	// namespace SinglyLinkedList
}	// namespace Strategy

#endif	// #ifndef _SINGLY_LINKED_LIST_H_
