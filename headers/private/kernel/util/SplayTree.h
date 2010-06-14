/*
 * Copyright 2008-2009, Ingo Weinhold <ingo_weinhold@gmx.de>.
 * Distributed under the terms of the MIT License.
 *
 * Original Java implementation:
 * Available at http://www.link.cs.cmu.edu/splay/
 * Author: Danny Sleator <sleator@cs.cmu.edu>
 * This code is in the public domain.
 */
#ifndef KERNEL_UTIL_SPLAY_TREE_H
#define KERNEL_UTIL_SPLAY_TREE_H

/*!	Implements two classes:

	SplayTree: A top-down splay tree.

	IteratableSplayTree: Extends SplayTree by a singly-linked list to make it
	cheaply iteratable (requires another pointer per node).

	Both classes are templatized over a definition parameter with the following
	(or a compatible) interface:

	struct SplayTreeDefinition {
		typedef xxx KeyType;
		typedef	yyy NodeType;

		static const KeyType& GetKey(const NodeType* node);
		static SplayTreeLink<NodeType>* GetLink(NodeType* node);

		static int Compare(const KeyType& key, const NodeType* node);

		// for IteratableSplayTree only
		static NodeType** GetListLink(NodeType* node);
	};
*/


template<typename Node>
struct SplayTreeLink {
	Node*	left;
	Node*	right;
};


template<typename Definition>
class SplayTree {
protected:
	typedef typename Definition::KeyType	Key;
	typedef typename Definition::NodeType	Node;
	typedef SplayTreeLink<Node>				Link;

public:
	SplayTree()
		:
		fRoot(NULL)
	{
	}

	/*!
		Insert into the tree.
		\param node the item to insert.
	*/
	bool Insert(Node* node)
	{
		Link* nodeLink = Definition::GetLink(node);

		if (fRoot == NULL) {
			fRoot = node;
			nodeLink->left = NULL;
			nodeLink->right = NULL;
			return true;
		}

		Key key = Definition::GetKey(node);
		_Splay(key);

		int c = Definition::Compare(key, fRoot);
		if (c == 0)
			return false;

		Link* rootLink = Definition::GetLink(fRoot);

		if (c < 0) {
			nodeLink->left = rootLink->left;
			nodeLink->right = fRoot;
			rootLink->left = NULL;
		} else {
			nodeLink->right = rootLink->right;
			nodeLink->left = fRoot;
			rootLink->right = NULL;
		}

		fRoot = node;
		return true;
	}

	Node* Remove(const Key& key)
	{
		if (fRoot == NULL)
			return NULL;

		_Splay(key);

		if (Definition::Compare(key, fRoot) != 0)
			return NULL;

		// Now delete the root
		Node* node = fRoot;
		Link* rootLink = Definition::GetLink(fRoot);
		if (rootLink->left == NULL) {
			fRoot = rootLink->right;
		} else {
			Node* temp = rootLink->right;
			fRoot = rootLink->left;
			_Splay(key);
			Definition::GetLink(fRoot)->right = temp;
		}

		return node;
    }

	/*!
		Remove from the tree.
		\param node the item to remove.
	*/
	bool Remove(Node* node)
	{
		Key key = Definition::GetKey(node);
		_Splay(key);

		if (node != fRoot)
			return false;

		// Now delete the root
		Link* rootLink = Definition::GetLink(fRoot);
		if (rootLink->left == NULL) {
			fRoot = rootLink->right;
		} else {
			Node* temp = rootLink->right;
			fRoot = rootLink->left;
			_Splay(key);
			Definition::GetLink(fRoot)->right = temp;
		}

		return true;
    }

	/*!
		Find the smallest item in the tree.
	*/
	Node* FindMin()
	{
		if (fRoot == NULL)
			return NULL;

		Node* node = fRoot;

		while (Node* left = Definition::GetLink(node)->left)
			node = left;

		_Splay(Definition::GetKey(node));

		return node;
	}

    /*!
		Find the largest item in the tree.
     */
	Node* FindMax()
	{
		if (fRoot == NULL)
			return NULL;

		Node* node = fRoot;

		while (Node* right = Definition::GetLink(node)->right)
			node = right;

		_Splay(Definition::GetKey(node));

		return node;
    }

    /*!
		Find an item in the tree.
	*/
	Node* Lookup(const Key& key)
	{
		if (fRoot == NULL)
			return NULL;

		_Splay(key);

		return Definition::Compare(key, fRoot) == 0 ? fRoot : NULL;
    }

	Node* Root() const
	{
		return fRoot;
	}

    /*!
		Test if the tree is logically empty.
		\return true if empty, false otherwise.
	*/
	bool IsEmpty() const
	{
		return fRoot == NULL;
	}

	Node* PreviousDontSplay(const Key& key) const
	{
		Node* closestNode = NULL;
		Node* node = fRoot;
		while (node != NULL) {
			if (Definition::Compare(key, node) > 0) {
				closestNode = node;
				node = Definition::GetLink(node)->right;
			} else
				node = Definition::GetLink(node)->left;
		}

		return closestNode;
	}

	Node* FindClosest(const Key& key, bool greater, bool orEqual)
	{
		if (fRoot == NULL)
			return NULL;

		_Splay(key);

		Node* closestNode = NULL;
		Node* node = fRoot;
		while (node != NULL) {
			int compare = Definition::Compare(key, node);
			if (compare == 0 && orEqual)
				return node;

			if (greater) {
				if (compare < 0) {
					closestNode = node;
					node = Definition::GetLink(node)->left;
				} else
					node = Definition::GetLink(node)->right;
			} else {
				if (compare > 0) {
					closestNode = node;
					node = Definition::GetLink(node)->right;
				} else
					node = Definition::GetLink(node)->left;
			}
		}

		return closestNode;
	}

	SplayTree& operator=(const SplayTree& other)
	{
		fRoot = other.fRoot;
		return *this;
	}

private:
	/*!
		Internal method to perform a top-down splay.

		_Splay(key) does the splay operation on the given key.
		If key is in the tree, then the node containing
		that key becomes the root.  If key is not in the tree,
		then after the splay, key.root is either the greatest key
		< key in the tree, or the least key > key in the tree.

		This means, among other things, that if you splay with
		a key that's larger than any in the tree, the rightmost
		node of the tree becomes the root.  This property is used
		in the Remove() method.
	*/
    void _Splay(const Key& key) {
		Link headerLink;
		headerLink.left = headerLink.right = NULL;

		Link* lLink = &headerLink;
		Link* rLink = &headerLink;

		Node* l = NULL;
		Node* r = NULL;
		Node* t = fRoot;

		for (;;) {
			int c = Definition::Compare(key, t);
			if (c < 0) {
				Node*& left = Definition::GetLink(t)->left;
				if (left == NULL)
					break;

				if (Definition::Compare(key, left) < 0) {
					// rotate right
					Node* y = left;
					Link* yLink = Definition::GetLink(y);
					left = yLink->right;
					yLink->right = t;
					t = y;
					if (yLink->left == NULL)
						break;
				}

				// link right
				rLink->left = t;
				r = t;
				rLink = Definition::GetLink(r);
				t = rLink->left;
			} else if (c > 0) {
				Node*& right = Definition::GetLink(t)->right;
				if (right == NULL)
					break;

				if (Definition::Compare(key, right) > 0) {
					// rotate left
					Node* y = right;
					Link* yLink = Definition::GetLink(y);
					right = yLink->left;
					yLink->left = t;
					t = y;
					if (yLink->right == NULL)
						break;
				}

				// link left
				lLink->right = t;
				l = t;
				lLink = Definition::GetLink(l);
				t = lLink->right;
			} else
				break;
		}

		// assemble
		Link* tLink = Definition::GetLink(t);
		lLink->right = tLink->left;
		rLink->left = tLink->right;
		tLink->left = headerLink.right;
		tLink->right = headerLink.left;
		fRoot = t;
	}

protected:
	Node*	fRoot;
};


template<typename Definition>
class IteratableSplayTree {
protected:
	typedef typename Definition::KeyType	Key;
	typedef typename Definition::NodeType	Node;
	typedef SplayTreeLink<Node>				Link;
	typedef IteratableSplayTree<Definition>	Tree;

public:
	class Iterator {
	public:
		Iterator()
		{
		}

		Iterator(const Iterator& other)
		{
			*this = other;
		}

		Iterator(Tree* tree)
			:
			fTree(tree)
		{
			Rewind();
		}

		Iterator(Tree* tree, Node* next)
			:
			fTree(tree),
			fCurrent(NULL),
			fNext(next)
		{
		}

		bool HasNext() const
		{
			return fNext != NULL;
		}

		Node* Next()
		{
			fCurrent = fNext;
			if (fNext != NULL)
				fNext = *Definition::GetListLink(fNext);
			return fCurrent;
		}

		Node* Current()
		{
			return fCurrent;
		}

		Node* Remove()
		{
			Node* element = fCurrent;
			if (fCurrent) {
				fTree->Remove(fCurrent);
				fCurrent = NULL;
			}
			return element;
		}

		Iterator &operator=(const Iterator &other)
		{
			fTree = other.fTree;
			fCurrent = other.fCurrent;
			fNext = other.fNext;
			return *this;
		}

		void Rewind()
		{
			fCurrent = NULL;
			fNext = fTree->fFirst;
		}

	private:
		Tree*	fTree;
		Node*	fCurrent;
		Node*	fNext;
	};

	class ConstIterator {
	public:
		ConstIterator()
		{
		}

		ConstIterator(const ConstIterator& other)
		{
			*this = other;
		}

		ConstIterator(const Tree* tree)
			:
			fTree(tree)
		{
			Rewind();
		}

		ConstIterator(const Tree* tree, Node* next)
			:
			fTree(tree),
			fNext(next)
		{
		}

		bool HasNext() const
		{
			return fNext != NULL;
		}

		Node* Next()
		{
			Node* node = fNext;
			if (fNext != NULL)
				fNext = *Definition::GetListLink(fNext);
			return node;
		}

		ConstIterator &operator=(const ConstIterator &other)
		{
			fTree = other.fTree;
			fNext = other.fNext;
			return *this;
		}

		void Rewind()
		{
			fNext = fTree->fFirst;
		}

	private:
		const Tree*	fTree;
		Node*		fNext;
	};

	IteratableSplayTree()
		:
		fTree(),
		fFirst(NULL)
	{
	}

	bool Insert(Node* node)
	{
		if (!fTree.Insert(node))
			return false;

		Node** previousNext;
		if (Node* previous = fTree.PreviousDontSplay(Definition::GetKey(node)))
			previousNext = Definition::GetListLink(previous);
		else
			previousNext = &fFirst;

		*Definition::GetListLink(node) = *previousNext;
		*previousNext = node;

		return true;
	}

	Node* Remove(const Key& key)
	{
		Node* node = fTree.Remove(key);
		if (node == NULL)
			return NULL;

		Node** previousNext;
		if (Node* previous = fTree.PreviousDontSplay(key))
			previousNext = Definition::GetListLink(previous);
		else
			previousNext = &fFirst;

		*previousNext = *Definition::GetListLink(node);

		return node;
	}

	bool Remove(Node* node)
	{
		if (!fTree.Remove(node))
			return false;

		Node** previousNext;
		if (Node* previous = fTree.PreviousDontSplay(Definition::GetKey(node)))
			previousNext = Definition::GetListLink(previous);
		else
			previousNext = &fFirst;

		*previousNext = *Definition::GetListLink(node);

		return true;
	}

	Node* Lookup(const Key& key)
	{
		return fTree.Lookup(key);
	}

	Node* Root() const
	{
		return fTree.Root();
	}

    /*!
		Test if the tree is logically empty.
		\return true if empty, false otherwise.
	*/
	bool IsEmpty() const
	{
		return fTree.IsEmpty();
	}

	Node* FindMin()
	{
		return fTree.FindMin();
	}

	Node* FindMax()
	{
		return fTree.FindMax();
	}

	Iterator GetIterator()
	{
		return Iterator(this);
	}

	ConstIterator GetIterator() const
	{
		return ConstIterator(this);
	}

	Iterator GetIterator(const Key& key, bool greater, bool orEqual)
	{
		return Iterator(this, fTree.FindClosest(key, greater, orEqual));
	}

	ConstIterator GetIterator(const Key& key, bool greater, bool orEqual) const
	{
		return ConstIterator(this, FindClosest(key, greater, orEqual));
	}

	IteratableSplayTree& operator=(const IteratableSplayTree& other)
	{
		fTree = other.fTree;
		fFirst = other.fFirst;
		return *this;
	}

protected:
	friend class Iterator;
	friend class ConstIterator;
		// needed for gcc 2.95.3 only

	SplayTree<Definition>	fTree;
	Node*					fFirst;
};


#endif	// KERNEL_UTIL_SPLAY_TREE_H
