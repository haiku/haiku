#ifndef __Heap__
#define __Heap__

/*****************************************************************************/
/* AUTHOR:      Michael J. Radwin                                            */
/* DATE:        2/14/94                                                      */
/* DESCRIPTION: This file contains the code for a generic Heap class.        */
/*              You will need to fill in all of the methods of the CHeap     */
/*              methods for any other classes you need.                      */
/* MODIFIED:    12/14/94                                                     */
/*****************************************************************************/


template <class T>
class CHeapNode {

public:
	CHeapNode();
	virtual ~CHeapNode();

	virtual void SetParent(CHeapNode<T> * Lparent) = 0;
	virtual void SetChild(CHeapNode<T> * LoldChild, CHeapNode<T> * LnewChild) = 0;
	virtual void SetKey(T key) = 0;
	virtual CHeapNode<T> * Insert(T key) = 0;
	virtual CHeapNode<T> * Remove() = 0;
	virtual T GetKey() = 0;
	virtual T UpHeap(T key) = 0;
	virtual T DownHeap() = 0;
	virtual int Empty() = 0;
	virtual CHeapNode<T> * NextNode(CHeapNode<T> * node) = 0;
	virtual CHeapNode<T> * PrevNode(CHeapNode<T> * node) = 0;
} ;


template <class T>
class CHeapSuperNode : public CHeapNode<T> {

	friend class CHeapInternalNode;
	friend class CHeapLeafNode;
	friend class CHeap;
public:
	CHeapSuperNode(CHeapNode<T> * LnewNode);
	virtual ~CHeapSuperNode();

	virtual void SetParent(CHeapNode<T> * Lparent) { exit(-1); } ;
	virtual void SetChild(CHeapNode<T> * LoldChild, CHeapNode<T> * LnewChild) { Lroot_ = LnewChild; };
	virtual void SetKey(T key) { exit(-1); };
	virtual CHeapNode<T> * Insert(T key) { return(Lroot_->Insert(key)); };
	virtual CHeapNode<T> * Remove();
	virtual T GetKey() { return (T) NULL; };
	virtual T UpHeap(T key) { return key; };
	virtual T DownHeap();
	virtual int Empty() { return Lroot_->Empty(); };
	virtual CHeapNode<T> * NextNode(CHeapNode<T> * node) { return Lroot_->NextNode(this); };
	virtual CHeapNode<T> * PrevNode(CHeapNode<T> * node) { return Lroot_->PrevNode(this); };
protected:
	CHeapNode<T> * Lroot_;          /* pointer to root of tree */
} ;



template <class T>
class CHeapInternalNode : public CHeapNode<T> {
public:
	CHeapInternalNode(T key,
		 CHeapNode<T> * LleftChild,
		 CHeapNode<T> * LrightChild
		 );
	virtual ~CHeapInternalNode();

	virtual void SetParent(CHeapNode<T> * Lparent) { Lparent_ = Lparent; } ;
	virtual void SetChild(CHeapNode<T> * LoldChild, CHeapNode<T> * LnewChild);
	virtual void SetKey(T key) { key_ = key; };
	virtual CHeapNode<T> * Insert(T key);
	virtual CHeapNode<T> * Remove();
	virtual T GetKey() { return key_; };
	virtual T UpHeap(T key);
	virtual T DownHeap();
	virtual int Empty() { return 0; };
	virtual CHeapNode<T> * NextNode(CHeapNode<T> * node);
	virtual CHeapNode<T> * PrevNode(CHeapNode<T> * node);
protected:
	T key_;            /* this is my key value */
	CHeapNode<T> * Lparent_;       /* my parent */
	CHeapNode<T> * LleftChild_;    /* my left child */
	CHeapNode<T> * LrightChild_;   /* my right child */
	T DownHeapLeft();
	T DownHeapRight();
} ;



template <class T>
class CHeapLeafNode : public CHeapNode<T> {
public:
	CHeapLeafNode();
	virtual ~CHeapLeafNode();

	virtual void SetParent(CHeapNode<T> * Lparent) { Lparent_ = Lparent; } ;
	virtual void SetChild(CHeapNode<T> * LoldChild, CHeapNode<T> * LnewChild) { exit(1); };
	virtual void SetKey(T key) { exit(-1); };
	virtual CHeapNode<T> * Insert(T key);
	virtual CHeapNode<T> * Remove() { return NULL; };
	virtual T GetKey() { return (T) NULL; }
	virtual T UpHeap(T key) { return (T) NULL; };
	virtual T DownHeap() { return (T) NULL; };
	virtual int Empty() { return 1; };
	virtual CHeapNode<T> * NextNode(CHeapNode<T> * node) { return Lparent_; };
	virtual CHeapNode<T> * PrevNode(CHeapNode<T> * node) { return Lparent_; };
protected:
	CHeapNode<T> * Lparent_;       /* my parent */
} ;


/*************************************************************************/
/* Class : CHeap                                                         */
/* Descr : The CHeap class is a generic Heap class which can be used     */
/*         with the c++ template mechanism.                              */
/*************************************************************************/

template <class T>
class CHeap {
public:
	CHeap();
	~CHeap();

	void Insert(T key);
	void Delete(T &key);
	int Empty() { return Lheap_->Empty(); };
	int Search(T) { return 0; };

protected:
	CHeapSuperNode<T> * Lheap_;  /* pointer to heap supernode */
	CHeapNode<T> * LlastNode_;
} ;

#endif


