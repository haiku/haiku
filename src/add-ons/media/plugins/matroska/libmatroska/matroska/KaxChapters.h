/****************************************************************************
** libmatroska : parse Matroska files, see http://www.matroska.org/
**
** <file/class description>
**
** Copyright (C) 2002-2005 Steve Lhomme.  All rights reserved.
**
** This file is part of libmatroska.
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
** See http://www.matroska.org/license/lgpl/ for LGPL licensing information.**
** Contact license@matroska.org if any conditions of this licensing are
** not clear to you.
**
**********************************************************************/

/*!
	\file
	\version \$Id: KaxChapters.h,v 1.9 2004/04/14 23:26:17 robux4 Exp $
	\author Steve Lhomme     <robux4 @ users.sf.net>
*/
#ifndef LIBMATROSKA_CHAPTERS_H
#define LIBMATROSKA_CHAPTERS_H

#include "matroska/KaxTypes.h"
#include "ebml/EbmlMaster.h"
#include "ebml/EbmlUInteger.h"
#include "ebml/EbmlUnicodeString.h"
#include "ebml/EbmlString.h"
#include "ebml/EbmlBinary.h"

using namespace LIBEBML_NAMESPACE;

START_LIBMATROSKA_NAMESPACE

class MATROSKA_DLL_API KaxChapters : public EbmlMaster {
	public:
		KaxChapters();
		KaxChapters(const KaxChapters & ElementToClone) :EbmlMaster(ElementToClone) {}
		static EbmlElement & Create() {return *(new KaxChapters);}
		const EbmlCallbacks & Generic() const {return ClassInfos;}
		static const EbmlCallbacks ClassInfos;
		operator const EbmlId &() const {return ClassInfos.GlobalId;}
		EbmlElement * Clone() const {return new KaxChapters(*this);}
};

class MATROSKA_DLL_API KaxEditionEntry : public EbmlMaster {
	public:
		KaxEditionEntry();
		KaxEditionEntry(const KaxEditionEntry & ElementToClone) :EbmlMaster(ElementToClone) {}
		static EbmlElement & Create() {return *(new KaxEditionEntry);}
		const EbmlCallbacks & Generic() const {return ClassInfos;}
		static const EbmlCallbacks ClassInfos;
		operator const EbmlId &() const {return ClassInfos.GlobalId;}
		EbmlElement * Clone() const {return new KaxEditionEntry(*this);}
};

class MATROSKA_DLL_API KaxEditionUID : public EbmlUInteger {
public:
    KaxEditionUID() {}
	KaxEditionUID(const KaxEditionUID & ElementToClone) :EbmlUInteger(ElementToClone) {}
    static EbmlElement & Create() {return *(new KaxEditionUID);}
    const EbmlCallbacks & Generic() const {return ClassInfos;}
    static const EbmlCallbacks ClassInfos;
    operator const EbmlId &() const {return ClassInfos.GlobalId;}
	EbmlElement * Clone() const {return new KaxEditionUID(*this);}
};

class MATROSKA_DLL_API KaxEditionFlagHidden : public EbmlUInteger {
public:
    KaxEditionFlagHidden(): EbmlUInteger(0) {}
	KaxEditionFlagHidden(const KaxEditionFlagHidden & ElementToClone) :EbmlUInteger(ElementToClone) {}
    static EbmlElement & Create() {return *(new KaxEditionFlagHidden);}
    const EbmlCallbacks & Generic() const {return ClassInfos;}
    static const EbmlCallbacks ClassInfos;
    operator const EbmlId &() const {return ClassInfos.GlobalId;}
	EbmlElement * Clone() const {return new KaxEditionFlagHidden(*this);}
};

class MATROSKA_DLL_API KaxEditionFlagDefault : public EbmlUInteger {
public:
    KaxEditionFlagDefault(): EbmlUInteger(0) {}
	KaxEditionFlagDefault(const KaxEditionFlagDefault & ElementToClone) :EbmlUInteger(ElementToClone) {}
    static EbmlElement & Create() {return *(new KaxEditionFlagDefault);}
    const EbmlCallbacks & Generic() const {return ClassInfos;}
    static const EbmlCallbacks ClassInfos;
    operator const EbmlId &() const {return ClassInfos.GlobalId;}
	EbmlElement * Clone() const {return new KaxEditionFlagDefault(*this);}
};

class MATROSKA_DLL_API KaxEditionFlagOrdered : public EbmlUInteger {
public:
    KaxEditionFlagOrdered(): EbmlUInteger(0) {}
	KaxEditionFlagOrdered(const KaxEditionFlagOrdered & ElementToClone) :EbmlUInteger(ElementToClone) {}
    static EbmlElement & Create() {return *(new KaxEditionFlagOrdered);}
    const EbmlCallbacks & Generic() const {return ClassInfos;}
    static const EbmlCallbacks ClassInfos;
    operator const EbmlId &() const {return ClassInfos.GlobalId;}
	EbmlElement * Clone() const {return new KaxEditionFlagOrdered(*this);}
};

class MATROSKA_DLL_API KaxChapterAtom : public EbmlMaster {
public:
    KaxChapterAtom();
	KaxChapterAtom(const KaxChapterAtom & ElementToClone) :EbmlMaster(ElementToClone) {}
    static EbmlElement & Create() {return *(new KaxChapterAtom);}
    const EbmlCallbacks & Generic() const {return ClassInfos;}
    static const EbmlCallbacks ClassInfos;
    operator const EbmlId &() const {return ClassInfos.GlobalId;}
	EbmlElement * Clone() const {return new KaxChapterAtom(*this);}
};

class MATROSKA_DLL_API KaxChapterUID : public EbmlUInteger {
public:
    KaxChapterUID() {}
	KaxChapterUID(const KaxChapterUID & ElementToClone) :EbmlUInteger(ElementToClone) {}
    static EbmlElement & Create() {return *(new KaxChapterUID);}
    const EbmlCallbacks & Generic() const {return ClassInfos;}
    static const EbmlCallbacks ClassInfos;
    operator const EbmlId &() const {return ClassInfos.GlobalId;}
	EbmlElement * Clone() const {return new KaxChapterUID(*this);}
};

class MATROSKA_DLL_API KaxChapterTimeStart : public EbmlUInteger {
public:
    KaxChapterTimeStart() {}
	KaxChapterTimeStart(const KaxChapterTimeStart & ElementToClone) :EbmlUInteger(ElementToClone) {}
    static EbmlElement & Create() {return *(new KaxChapterTimeStart);}
    const EbmlCallbacks & Generic() const {return ClassInfos;}
    static const EbmlCallbacks ClassInfos;
    operator const EbmlId &() const {return ClassInfos.GlobalId;}
	EbmlElement * Clone() const {return new KaxChapterTimeStart(*this);}
};

class MATROSKA_DLL_API KaxChapterTimeEnd : public EbmlUInteger {
public:
    KaxChapterTimeEnd() {}
	KaxChapterTimeEnd(const KaxChapterTimeEnd & ElementToClone) :EbmlUInteger(ElementToClone) {}
    static EbmlElement & Create() {return *(new KaxChapterTimeEnd);}
    const EbmlCallbacks & Generic() const {return ClassInfos;}
    static const EbmlCallbacks ClassInfos;
    operator const EbmlId &() const {return ClassInfos.GlobalId;}
	EbmlElement * Clone() const {return new KaxChapterTimeEnd(*this);}
};

class MATROSKA_DLL_API KaxChapterFlagHidden : public EbmlUInteger {
public:
	KaxChapterFlagHidden(): EbmlUInteger(0) {}
	KaxChapterFlagHidden(const KaxChapterFlagHidden & ElementToClone) :EbmlUInteger(ElementToClone) {}
	static EbmlElement & Create() {return *(new KaxChapterFlagHidden);}
	const EbmlCallbacks & Generic() const {return ClassInfos;}
	static const EbmlCallbacks ClassInfos;
	operator const EbmlId &() const {return ClassInfos.GlobalId;}
	EbmlElement * Clone() const {return new KaxChapterFlagHidden(*this);}
};

class MATROSKA_DLL_API KaxChapterFlagEnabled : public EbmlUInteger {
public:
	KaxChapterFlagEnabled(): EbmlUInteger(1) {}
	KaxChapterFlagEnabled(const KaxChapterFlagEnabled & ElementToClone) :EbmlUInteger(ElementToClone) {}
	static EbmlElement & Create() {return *(new KaxChapterFlagEnabled);}
	const EbmlCallbacks & Generic() const {return ClassInfos;}
	static const EbmlCallbacks ClassInfos;
	operator const EbmlId &() const {return ClassInfos.GlobalId;}
	EbmlElement * Clone() const {return new KaxChapterFlagEnabled(*this);}
};

class MATROSKA_DLL_API KaxChapterSegmentUID : public EbmlBinary {
public:
    KaxChapterSegmentUID() {}
	KaxChapterSegmentUID(const KaxChapterSegmentUID & ElementToClone) :EbmlBinary(ElementToClone) {}
    static EbmlElement & Create() {return *(new KaxChapterSegmentUID);}
    const EbmlCallbacks & Generic() const {return ClassInfos;}
    static const EbmlCallbacks ClassInfos;
    operator const EbmlId &() const {return ClassInfos.GlobalId;}
	EbmlElement * Clone() const {return new KaxChapterSegmentUID(*this);}
	bool ValidateSize() const { return (Size == 16);}
};

class MATROSKA_DLL_API KaxChapterSegmentEditionUID : public EbmlBinary {
public:
    KaxChapterSegmentEditionUID() {}
	KaxChapterSegmentEditionUID(const KaxChapterSegmentEditionUID & ElementToClone) :EbmlBinary(ElementToClone) {}
    static EbmlElement & Create() {return *(new KaxChapterSegmentEditionUID);}
    const EbmlCallbacks & Generic() const {return ClassInfos;}
    static const EbmlCallbacks ClassInfos;
    operator const EbmlId &() const {return ClassInfos.GlobalId;}
	EbmlElement * Clone() const {return new KaxChapterSegmentEditionUID(*this);}
	bool ValidateSize() const { return (Size == 16);}
};

class MATROSKA_DLL_API KaxChapterPhysicalEquiv : public EbmlUInteger {
public:
	KaxChapterPhysicalEquiv(): EbmlUInteger() {}
	KaxChapterPhysicalEquiv(const KaxChapterPhysicalEquiv & ElementToClone) :EbmlUInteger(ElementToClone) {}
	static EbmlElement & Create() {return *(new KaxChapterPhysicalEquiv);}
	const EbmlCallbacks & Generic() const {return ClassInfos;}
	static const EbmlCallbacks ClassInfos;
	operator const EbmlId &() const {return ClassInfos.GlobalId;}
	EbmlElement * Clone() const {return new KaxChapterPhysicalEquiv(*this);}
};

class MATROSKA_DLL_API KaxChapterTrack : public EbmlMaster {
public:
    KaxChapterTrack();
	KaxChapterTrack(const KaxChapterTrack & ElementToClone) :EbmlMaster(ElementToClone) {}
    static EbmlElement & Create() {return *(new KaxChapterTrack);}
    const EbmlCallbacks & Generic() const {return ClassInfos;}
    static const EbmlCallbacks ClassInfos;
    operator const EbmlId &() const {return ClassInfos.GlobalId;}
	EbmlElement * Clone() const {return new KaxChapterTrack(*this);}
};

class MATROSKA_DLL_API KaxChapterTrackNumber : public EbmlUInteger {
public:
    KaxChapterTrackNumber() {}
	KaxChapterTrackNumber(const KaxChapterTrackNumber & ElementToClone) :EbmlUInteger(ElementToClone) {}
    static EbmlElement & Create() {return *(new KaxChapterTrackNumber);}
    const EbmlCallbacks & Generic() const {return ClassInfos;}
    static const EbmlCallbacks ClassInfos;
    operator const EbmlId &() const {return ClassInfos.GlobalId;}
	EbmlElement * Clone() const {return new KaxChapterTrackNumber(*this);}
};

class MATROSKA_DLL_API KaxChapterDisplay : public EbmlMaster {
public:
    KaxChapterDisplay();
	KaxChapterDisplay(const KaxChapterDisplay & ElementToClone) :EbmlMaster(ElementToClone) {}
    static EbmlElement & Create() {return *(new KaxChapterDisplay);}
    const EbmlCallbacks & Generic() const {return ClassInfos;}
    static const EbmlCallbacks ClassInfos;
    operator const EbmlId &() const {return ClassInfos.GlobalId;}
	EbmlElement * Clone() const {return new KaxChapterDisplay(*this);}
};

class MATROSKA_DLL_API KaxChapterString : public EbmlUnicodeString {
public:
    KaxChapterString() {}
	KaxChapterString(const KaxChapterString & ElementToClone) :EbmlUnicodeString(ElementToClone) {}
    static EbmlElement & Create() {return *(new KaxChapterString);}
    const EbmlCallbacks & Generic() const {return ClassInfos;}
    static const EbmlCallbacks ClassInfos;
    operator const EbmlId &() const {return ClassInfos.GlobalId;}
	EbmlElement * Clone() const {return new KaxChapterString(*this);}
};

class MATROSKA_DLL_API KaxChapterLanguage : public EbmlString {
public:
    KaxChapterLanguage() :EbmlString("eng") {}
	KaxChapterLanguage(const KaxChapterLanguage & ElementToClone) :EbmlString(ElementToClone) {}
    static EbmlElement & Create() {return *(new KaxChapterLanguage);}
    const EbmlCallbacks & Generic() const {return ClassInfos;}
    static const EbmlCallbacks ClassInfos;
    operator const EbmlId &() const {return ClassInfos.GlobalId;}
	EbmlElement * Clone() const {return new KaxChapterLanguage(*this);}
};

class MATROSKA_DLL_API KaxChapterCountry : public EbmlString {
public:
    KaxChapterCountry() :EbmlString() {}
	KaxChapterCountry(const KaxChapterCountry & ElementToClone) :EbmlString(ElementToClone) {}
    static EbmlElement & Create() {return *(new KaxChapterCountry);}
    const EbmlCallbacks & Generic() const {return ClassInfos;}
    static const EbmlCallbacks ClassInfos;
    operator const EbmlId &() const {return ClassInfos.GlobalId;}
	EbmlElement * Clone() const {return new KaxChapterCountry(*this);}
};

class MATROSKA_DLL_API KaxChapterProcess : public EbmlMaster {
public:
    KaxChapterProcess();
	KaxChapterProcess(const KaxChapterProcess & ElementToClone) :EbmlMaster(ElementToClone) {}
    static EbmlElement & Create() {return *(new KaxChapterProcess);}
    const EbmlCallbacks & Generic() const {return ClassInfos;}
    static const EbmlCallbacks ClassInfos;
    operator const EbmlId &() const {return ClassInfos.GlobalId;}
	EbmlElement * Clone() const {return new KaxChapterProcess(*this);}
};

class MATROSKA_DLL_API KaxChapterProcessCodecID : public EbmlUInteger {
public:
    KaxChapterProcessCodecID() :EbmlUInteger(0) {}
	KaxChapterProcessCodecID(const KaxChapterProcessCodecID & ElementToClone) :EbmlUInteger(ElementToClone) {}
    static EbmlElement & Create() {return *(new KaxChapterProcessCodecID);}
    const EbmlCallbacks & Generic() const {return ClassInfos;}
    static const EbmlCallbacks ClassInfos;
    operator const EbmlId &() const {return ClassInfos.GlobalId;}
	EbmlElement * Clone() const {return new KaxChapterProcessCodecID(*this);}
};

class MATROSKA_DLL_API KaxChapterProcessPrivate : public EbmlBinary {
public:
    KaxChapterProcessPrivate() {}
	KaxChapterProcessPrivate(const KaxChapterProcessPrivate & ElementToClone) :EbmlBinary(ElementToClone) {}
    static EbmlElement & Create() {return *(new KaxChapterProcessPrivate);}
    const EbmlCallbacks & Generic() const {return ClassInfos;}
    static const EbmlCallbacks ClassInfos;
    operator const EbmlId &() const {return ClassInfos.GlobalId;}
	EbmlElement * Clone() const {return new KaxChapterProcessPrivate(*this);}
	bool ValidateSize() const {return true;}
};

class MATROSKA_DLL_API KaxChapterProcessCommand : public EbmlMaster {
public:
    KaxChapterProcessCommand();
	KaxChapterProcessCommand(const KaxChapterProcessCommand & ElementToClone) :EbmlMaster(ElementToClone) {}
    static EbmlElement & Create() {return *(new KaxChapterProcessCommand);}
    const EbmlCallbacks & Generic() const {return ClassInfos;}
    static const EbmlCallbacks ClassInfos;
    operator const EbmlId &() const {return ClassInfos.GlobalId;}
	EbmlElement * Clone() const {return new KaxChapterProcessCommand(*this);}
};

class MATROSKA_DLL_API KaxChapterProcessTime : public EbmlUInteger {
public:
    KaxChapterProcessTime() {}
	KaxChapterProcessTime(const KaxChapterProcessTime & ElementToClone) :EbmlUInteger(ElementToClone) {}
    static EbmlElement & Create() {return *(new KaxChapterProcessTime);}
    const EbmlCallbacks & Generic() const {return ClassInfos;}
    static const EbmlCallbacks ClassInfos;
    operator const EbmlId &() const {return ClassInfos.GlobalId;}
	EbmlElement * Clone() const {return new KaxChapterProcessTime(*this);}
};

class MATROSKA_DLL_API KaxChapterProcessData : public EbmlBinary {
public:
    KaxChapterProcessData() {}
	KaxChapterProcessData(const KaxChapterProcessData & ElementToClone) :EbmlBinary(ElementToClone) {}
    static EbmlElement & Create() {return *(new KaxChapterProcessData);}
    const EbmlCallbacks & Generic() const {return ClassInfos;}
    static const EbmlCallbacks ClassInfos;
    operator const EbmlId &() const {return ClassInfos.GlobalId;}
	EbmlElement * Clone() const {return new KaxChapterProcessData(*this);}
	bool ValidateSize() const {return true;}
};

END_LIBMATROSKA_NAMESPACE

#endif // LIBMATROSKA_CHAPTERS_H
