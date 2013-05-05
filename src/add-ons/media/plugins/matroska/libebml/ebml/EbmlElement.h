/****************************************************************************
** libebml : parse EBML files, see http://embl.sourceforge.net/
**
** <file/class description>
**
** Copyright (C) 2002-2005 Steve Lhomme.  All rights reserved.
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
	\version \$Id: EbmlElement.h 1232 2005-10-15 15:56:52Z robux4 $
	\author Steve Lhomme     <robux4 @ users.sf.net>
*/
#ifndef LIBEBML_ELEMENT_H
#define LIBEBML_ELEMENT_H

#include <cassert>

#include "EbmlTypes.h"
#include "EbmlId.h"
#include "IOCallback.h"

START_LIBEBML_NAMESPACE

/*!
	\brief The size of the EBML-coded length
*/
int EBML_DLL_API CodedSizeLength(uint64 Length, unsigned int SizeLength, bool bSizeIsFinite = true);

/*!
	\brief The coded value of the EBML-coded length
	\note The size of OutBuffer must be 8 octets at least
*/
int EBML_DLL_API CodedValueLength(uint64 Length, int CodedSize, binary * OutBuffer);

/*!
	\brief Read an EBML-coded value from a buffer
	\return the value read
*/
uint64 EBML_DLL_API ReadCodedSizeValue(const binary * InBuffer, uint32 & BufferSize, uint64 & SizeUnknown);

/*!
	\brief The size of the EBML-coded signed length
*/
int EBML_DLL_API CodedSizeLengthSigned(int64 Length, unsigned int SizeLength);

/*!
	\brief The coded value of the EBML-coded signed length
	\note the size of OutBuffer must be 8 octets at least
*/
int EBML_DLL_API CodedValueLengthSigned(int64 Length, int CodedSize, binary * OutBuffer);

/*!
	\brief Read a signed EBML-coded value from a buffer
	\return the value read
*/
int64 EBML_DLL_API ReadCodedSizeSignedValue(const binary * InBuffer, uint32 & BufferSize, uint64 & SizeUnknown);

class EbmlStream;
class EbmlSemanticContext;
class EbmlElement;

// functions for generic handling of data (should be static to all classes)
/*!
	\todo Handle default value
*/
class EBML_DLL_API EbmlCallbacks {
	public:
		EbmlCallbacks(EbmlElement & (*Creator)(), const EbmlId & aGlobalId, const char * aDebugName, const EbmlSemanticContext & aContext)
			:Create(Creator)
			,GlobalId(aGlobalId)
			,DebugName(aDebugName)
			,Context(aContext)
		{}

		EbmlElement & (*Create)();
		const EbmlId & GlobalId;
		const char * DebugName;
		const EbmlSemanticContext & Context;
};

/*!
	\brief contains the semantic informations for a given level and all sublevels
	\todo move the ID in the element class
*/
class EBML_DLL_API EbmlSemantic {
	public:
		EbmlSemantic(bool aMandatory, bool aUnique, const EbmlCallbacks & aGetCallbacks)
			:Mandatory(aMandatory), Unique(aUnique), GetCallbacks(aGetCallbacks) {}

		bool Mandatory;
			///< whether the element is mandatory in the context or not
		bool Unique;
		const EbmlCallbacks & GetCallbacks;
};

typedef const class EbmlSemanticContext & (*_GetSemanticContext)();

/*!
	Context of the element
	\todo allow more than one parent ?
*/
class EBML_DLL_API EbmlSemanticContext {
	public:
		EbmlSemanticContext(unsigned int aSize,
			const EbmlSemantic *aMyTable,
			const EbmlSemanticContext *aUpTable,
			const _GetSemanticContext aGetGlobalContext,
			const EbmlCallbacks *aMasterElt)
			:Size(aSize), MyTable(aMyTable), UpTable(aUpTable),
			 GetGlobalContext(aGetGlobalContext), MasterElt(aMasterElt) {}

		bool operator!=(const EbmlSemanticContext & aElt) const {
			return ((Size != aElt.Size) || (MyTable != aElt.MyTable) ||
				(UpTable != aElt.UpTable) || (GetGlobalContext != aElt.GetGlobalContext) |
				(MasterElt != aElt.MasterElt));
		}


		unsigned int Size;          ///< number of elements in the table
		const EbmlSemantic *MyTable; ///< First element in the table
		const EbmlSemanticContext *UpTable; ///< Parent element
		/// \todo replace with the global context directly
		const _GetSemanticContext GetGlobalContext; ///< global elements supported at this level
		const EbmlCallbacks *MasterElt;
};

/*!
	\class EbmlElement
	\brief Hold basic informations about an EBML element (ID + length)
*/
class EBML_DLL_API EbmlElement {
	public:
		EbmlElement(const uint64 aDefaultSize, bool bValueSet = false);
		virtual ~EbmlElement() {assert(!bLocked);}
	
		/// Set the minimum length that will be used to write the element size (-1 = optimal)
		void SetSizeLength(const int NewSizeLength) {SizeLength = NewSizeLength;}
		int GetSizeLength() const {return SizeLength;}
		
		static EbmlElement * FindNextElement(IOCallback & DataStream, const EbmlSemanticContext & Context, int & UpperLevel, uint64 MaxDataSize, bool AllowDummyElt, unsigned int MaxLowerLevel = 1);
		static EbmlElement * FindNextID(IOCallback & DataStream, const EbmlCallbacks & ClassInfos, const uint64 MaxDataSize);

		/*!
			\brief find the next element with the same ID
		*/
		EbmlElement * FindNext(IOCallback & DataStream, const uint64 MaxDataSize);

		EbmlElement * SkipData(EbmlStream & DataStream, const EbmlSemanticContext & Context, EbmlElement * TestReadElt = NULL, bool AllowDummyElt = false);

		/*!
			\brief Give a copy of the element, all data inside the element is copied
			\return NULL if there is not enough memory
		*/
		virtual EbmlElement * Clone() const = 0;

		virtual operator const EbmlId &() const = 0;

		// by default only allow to set element as finite (override when needed)
		virtual bool SetSizeInfinite(bool bIsInfinite = true) {return !bIsInfinite;}

		virtual bool ValidateSize() const = 0;

		uint64 GetElementPosition() const {
			return ElementPosition;
		}

		uint64 ElementSize(bool bKeepIntact = false) const; /// return the size of the header+data, before writing
		
		uint32 Render(IOCallback & output, bool bKeepIntact = false, bool bKeepPosition = false, bool bForceRender = false);

		virtual uint64 UpdateSize(bool bKeepIntact = false, bool bForceRender = false) = 0; /// update the Size of the Data stored
		virtual uint64 GetSize() const {return Size;} /// return the size of the data stored in the element, on reading

		virtual uint64 ReadData(IOCallback & input, ScopeMode ReadFully = SCOPE_ALL_DATA) = 0;
		virtual void Read(EbmlStream & inDataStream, const EbmlSemanticContext & Context, int & UpperEltFound, EbmlElement * & FoundElt, bool AllowDummyElt = false, ScopeMode ReadFully = SCOPE_ALL_DATA);
		
		/// return the generic callback to monitor a derived class
		virtual const EbmlCallbacks & Generic() const = 0;

		bool IsLocked() const {return bLocked;}
		void Lock(bool bLock = true) { bLocked = bLock;}

		/*!
			\brief default comparison for elements that can't be compared
		*/
		virtual bool operator<(const EbmlElement & EltB) const {
			return true;
		}

		static bool CompareElements(const EbmlElement *A, const EbmlElement *B);

		virtual bool IsDummy() const {return false;}
		virtual bool IsMaster() const {return false;}

		uint8 HeadSize() const {
			return EbmlId(*this).Length + CodedSizeLength(Size, SizeLength, bSizeIsFinite);
		} /// return the size of the head, on reading/writing
		
		/*!
			\brief Force the size of an element
			\warning only possible if the size is "undefined"
		*/
		bool ForceSize(uint64 NewSize);

		uint32 OverwriteHead(IOCallback & output, bool bKeepPosition = false);

		/*!
			\brief void the content of the element (replace by EbmlVoid)
		*/
		uint32 VoidMe(IOCallback & output, bool bKeepIntact = false);

		bool DefaultISset() const {return DefaultIsSet;}
		virtual bool IsDefaultValue() const = 0;
		bool IsFiniteSize() const {return bSizeIsFinite;}

		/*!
			\brief set the default size of an element
		*/
		virtual void SetDefaultSize(const uint64 aDefaultSize) {DefaultSize = aDefaultSize;}

		bool ValueIsSet() const {return bValueIsSet;}

		inline uint64 GetEndPosition() const {
			return SizePosition + CodedSizeLength(Size, SizeLength, bSizeIsFinite) + Size;
		}
		
	protected:
		uint64 Size;        ///< the size of the data to write
		uint64 DefaultSize; ///< Minimum data size to fill on rendering (0 = optimal)
		int SizeLength; /// the minimum size on which the size will be written (0 = optimal)
		bool bSizeIsFinite;
		uint64 ElementPosition;
		uint64 SizePosition;
		bool bValueIsSet;
		bool DefaultIsSet;
		bool bLocked;

		/*!
			\brief find any element in the stream
			\return a DummyRawElement if the element is unknown or NULL if the element dummy is not allowed
		*/
		static EbmlElement *CreateElementUsingContext(const EbmlId & aID, const EbmlSemanticContext & Context, int & LowLevel, bool IsGlobalContext, bool bAllowDummy = false, unsigned int MaxLowerLevel = 1);

		uint32 RenderHead(IOCallback & output, bool bForceRender, bool bKeepIntact = false, bool bKeepPosition = false);
		uint32 MakeRenderHead(IOCallback & output, bool bKeepPosition);
	
		/*!
			\brief prepare the data before writing them (in case it's not already done by default)
		*/
		virtual uint32 RenderData(IOCallback & output, bool bForceRender, bool bKeepIntact = false) = 0;

		/*!
			\brief special constructor for cloning
		*/
		EbmlElement(const EbmlElement & ElementToClone);
};

END_LIBEBML_NAMESPACE

#endif // LIBEBML_ELEMENT_H
