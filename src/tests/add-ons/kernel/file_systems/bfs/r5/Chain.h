#ifndef CHAIN_H
#define CHAIN_H
/* Chain - a chain implementation; it's used for the callback management
**		throughout the code (currently TreeIterator, and AttributeIterator).
**
** Initial version by Axel Dörfler, axeld@pinc-software.de
** This file may be used under the terms of the MIT License.
*/


/** The Link class you want to use with the Chain class needs to have
 *	a "fNext" member which is accessable from within the Chain class.
 */

template<class Link> class Chain {
	public:
		Chain()
			:
			fFirst(NULL)
		{
		}

		void Add(Link *link)
		{
			link->fNext = fFirst;
			fFirst = link;
		}

		void Remove(Link *link)
		{
			// search list for the correct callback to remove
			Link *last = NULL,*entry;
			for (entry = fFirst;link != entry;entry = entry->fNext)
				last = entry;
			if (link == entry) {
				if (last)
					last->fNext = link->fNext;
				else
					fFirst = link->fNext;
			}
		}

		Link *Next(Link *last)
		{
			if (last == NULL)
				return fFirst;

			return last->fNext;
		}

	private:
		Link	*fFirst;
};

#endif	/* CHAIN_H */
