/****************************************************************************
** libebml : parse EBML files, see http://embl.sourceforge.net/
**
** <file/class description>
**
** Copyright (C) 2002-2005 Steve Lhomme.  All rights reserved.
**
** This file is part of libebml.
**
** This library is free software; you can redistribute it and/or
** modify it under the terms of the GNU Lesser General Public
** License as published by the Free Software Foundation; either
** version 2.1 of the License, or (at your option) any later version.
** 
** This library is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
** Lesser General Public License for more details.
** 
** You should have received a copy of the GNU Lesser General Public
** License along with this library; if not, write to the Free Software
** Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
**
** See http://www.matroska.org/license/lgpl/ for LGPL licensing information.
**
** Contact license@matroska.org if any conditions of this licensing are
** not clear to you.
**
**********************************************************************/

/*!
	\file
	\version \$Id: EbmlMaster.h 1232 2005-10-15 15:56:52Z robux4 $
	\author Steve Lhomme     <robux4 @ users.sf.net>
*/
#ifndef LIBEBML_MASTER_H
#define LIBEBML_MASTER_H

#include <string>
#include <vector>

#include "EbmlTypes.h"
#include "EbmlElement.h"
#include "EbmlCrc32.h"

START_LIBEBML_NAMESPACE

const bool bChecksumUsedByDefault = false;

/*!
    \class EbmlMaster
    \brief Handle all operations on an EBML element that contains other EBML elements
*/
class EBML_DLL_API EbmlMaster : public EbmlElement {
	public:
		EbmlMaster(const EbmlSemanticContext & aContext, bool bSizeIsKnown = true);
		EbmlMaster(const EbmlMaster & ElementToClone);
		bool ValidateSize() const {return true;}
		/*!
			\warning be carefull to clear the memory allocated in the ElementList elsewhere
		*/
		virtual ~EbmlMaster();
	
		uint32 RenderData(IOCallback & output, bool bForceRender, bool bKeepIntact = false);
		uint64 ReadData(IOCallback & input, ScopeMode ReadFully);
		uint64 UpdateSize(bool bKeepIntact = false, bool bForceRender = false);
		
		/*!
			\brief Set whether the size is finite (size is known in advance when writing, or infinite size is not known on writing)
		*/
		bool SetSizeInfinite(bool aIsInfinite = true) {bSizeIsFinite = !aIsInfinite; return true;}
	
		bool PushElement(EbmlElement & element);
		uint64 GetSize() const { 
			if (bSizeIsFinite)
				return Size;
			else
				return (0-1);
		}
		
		uint64 GetDataStart() const {
			return ElementPosition + EbmlId(*this).Length + CodedSizeLength(Size, SizeLength, bSizeIsFinite);
		}

		/*!
			\brief find the element corresponding to the ID of the element, NULL if not found
		*/
		EbmlElement *FindElt(const EbmlCallbacks & Callbacks) const;
		/*!
			\brief find the first element corresponding to the ID of the element
		*/
		EbmlElement *FindFirstElt(const EbmlCallbacks & Callbacks, const bool bCreateIfNull);
		EbmlElement *FindFirstElt(const EbmlCallbacks & Callbacks) const;

		/*!
			\brief find the element of the same type of PasElt following in the list of elements
		*/
		EbmlElement *FindNextElt(const EbmlElement & PastElt, const bool bCreateIfNull);
		EbmlElement *FindNextElt(const EbmlElement & PastElt) const;
		EbmlElement *AddNewElt(const EbmlCallbacks & Callbacks);

		/*!
			\brief add an element at a specified location
		*/
		bool InsertElement(EbmlElement & element, size_t position = 0);
		bool InsertElement(EbmlElement & element, const EbmlElement & before);

		/*!
			\brief Read the data and keep the known children
		*/
		void Read(EbmlStream & inDataStream, const EbmlSemanticContext & Context, int & UpperEltFound, EbmlElement * & FoundElt, bool AllowDummyElt, ScopeMode ReadFully = SCOPE_ALL_DATA);
		
		/*!
			\brief sort Data when they can
		*/
		void Sort();

		size_t ListSize() const {return ElementList.size();}

		EbmlElement * operator[](unsigned int position) {return ElementList[position];}
		const EbmlElement * operator[](unsigned int position) const {return ElementList[position];}

		bool IsDefaultValue() const {
			return (ElementList.size() == 0);
		}
		virtual bool IsMaster() const {return true;}

		/*!
			\brief verify that all mandatory elements are present
			\note usefull after reading or before writing
		*/
		bool CheckMandatory() const;

		/*!
			\brief Remove an element from the list of the master
		*/
		void Remove(size_t Index);

		/*!
			\brief remove all elements, even the mandatory ones
		*/
		void RemoveAll() {ElementList.clear();}

		/*!
			\brief facility for Master elements to write only the head and force the size later
			\warning
		*/
		uint32 WriteHead(IOCallback & output, int SizeLength, bool bKeepIntact = false);

		void EnableChecksum(bool bIsEnabled = true) { bChecksumUsed = bIsEnabled; }
		bool HasChecksum() const {return bChecksumUsed;}
		bool VerifyChecksum() const;
		uint32 GetCrc32() const {return Checksum.GetCrc32();}
		void ForceChecksum(uint32 NewChecksum) { 
			Checksum.ForceCrc32(NewChecksum);
			bChecksumUsed = true;
		}

		/*!
			\brief drill down all sub-elements, finding any missing elements
		*/
		std::vector<std::string> FindAllMissingElements();

	protected:
		std::vector<EbmlElement *> ElementList;
	
		const EbmlSemanticContext & Context;

		bool      bChecksumUsed;
		EbmlCrc32 Checksum;
			
	private:
		/*!
			\brief Add all the mandatory elements to the list
		*/
		bool ProcessMandatory();
};

///< \todo add a restriction to only elements legal in the context
template <typename Type>
Type & GetChild(EbmlMaster & Master)
{
	return *(static_cast<Type *>(Master.FindFirstElt(Type::ClassInfos, true)));
}
// call with
// MyDocType = GetChild<EDocType>(TestHead);

template <typename Type>
Type * FindChild(EbmlMaster & Master)
{
	return static_cast<Type *>(Master.FindFirstElt(Type::ClassInfos, false));
}

template <typename Type>
Type & GetNextChild(EbmlMaster & Master, const Type & PastElt)
{
	return *(static_cast<Type *>(Master.FindNextElt(PastElt, true)));
}

template <typename Type>
Type & AddNewChild(EbmlMaster & Master)
{
	return *(static_cast<Type *>(Master.AddNewElt(Type::ClassInfos)));
}

END_LIBEBML_NAMESPACE

#endif // LIBEBML_MASTER_H
