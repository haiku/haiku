/****************************************************************************
** libmatroska : parse Matroska files, see http://www.matroska.org/
**
** <file/class description>
**
** Copyright (C) 2002-2004 Steve Lhomme.  All rights reserved.
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
	\version \$Id: KaxContentEncoding.h,v 1.7 2004/04/14 23:26:17 robux4 Exp $
	\author Moritz Bunkus <moritz @ bunkus.org>
*/
#ifndef LIBMATROSKA_CONTENT_ENCODING_H
#define LIBMATROSKA_CONTENT_ENCODING_H

#include "matroska/KaxTypes.h"
#include "ebml/EbmlMaster.h"
#include "ebml/EbmlUInteger.h"
#include "ebml/EbmlBinary.h"

using namespace LIBEBML_NAMESPACE;

START_LIBMATROSKA_NAMESPACE

class MATROSKA_DLL_API KaxContentEncodings: public EbmlMaster {
public:
  static const EbmlCallbacks ClassInfos;

  KaxContentEncodings();
  KaxContentEncodings(const KaxContentEncodings &ElementToClone):
    EbmlMaster(ElementToClone) {}
  static EbmlElement &Create() { return *(new KaxContentEncodings); }
  const EbmlCallbacks &Generic() const { return ClassInfos; }
  operator const EbmlId &() const { return ClassInfos.GlobalId; }
  EbmlElement *Clone() const { return new KaxContentEncodings(*this); }
};

class MATROSKA_DLL_API KaxContentEncoding: public EbmlMaster {
public:
  static const EbmlCallbacks ClassInfos;

  KaxContentEncoding();
  KaxContentEncoding(const KaxContentEncoding &ElementToClone):
    EbmlMaster(ElementToClone) {}
  static EbmlElement &Create() { return *(new KaxContentEncoding); }
  const EbmlCallbacks &Generic() const { return ClassInfos; }
  operator const EbmlId &() const { return ClassInfos.GlobalId; }
  EbmlElement *Clone() const { return new KaxContentEncoding(*this); }
};

class MATROSKA_DLL_API KaxContentEncodingOrder: public EbmlUInteger {
public:
  static const EbmlCallbacks ClassInfos;

  KaxContentEncodingOrder(): EbmlUInteger(0) {}
  KaxContentEncodingOrder(const KaxContentEncodingOrder &ElementToClone):
    EbmlUInteger(ElementToClone) {}
  static EbmlElement &Create() { return *(new KaxContentEncodingOrder); }
  const EbmlCallbacks &Generic() const { return ClassInfos; }
  operator const EbmlId &() const { return ClassInfos.GlobalId; }
  EbmlElement *Clone() const { return new KaxContentEncodingOrder(*this); }
};

class MATROSKA_DLL_API KaxContentEncodingScope: public EbmlUInteger {
public:
  static const EbmlCallbacks ClassInfos;

  KaxContentEncodingScope(): EbmlUInteger(1) {}
  KaxContentEncodingScope(const KaxContentEncodingScope &ElementToClone):
    EbmlUInteger(ElementToClone) {}
  static EbmlElement &Create() { return *(new KaxContentEncodingScope); }
  const EbmlCallbacks &Generic() const { return ClassInfos; }
  operator const EbmlId &() const { return ClassInfos.GlobalId; }
  EbmlElement *Clone() const { return new KaxContentEncodingScope(*this); }
};

class MATROSKA_DLL_API KaxContentEncodingType: public EbmlUInteger {
public:
  static const EbmlCallbacks ClassInfos;

  KaxContentEncodingType(): EbmlUInteger(0) {}
  KaxContentEncodingType(const KaxContentEncodingType &ElementToClone):
    EbmlUInteger(ElementToClone) {}
  static EbmlElement &Create() { return *(new KaxContentEncodingType); }
  const EbmlCallbacks &Generic() const { return ClassInfos; }
  operator const EbmlId &() const { return ClassInfos.GlobalId; }
  EbmlElement *Clone() const { return new KaxContentEncodingType(*this); }
};

class MATROSKA_DLL_API KaxContentCompression: public EbmlMaster {
public:
  static const EbmlCallbacks ClassInfos;

  KaxContentCompression();
  KaxContentCompression(const KaxContentCompression &ElementToClone):
    EbmlMaster(ElementToClone) {}
  static EbmlElement &Create() { return *(new KaxContentCompression); }
  const EbmlCallbacks &Generic() const { return ClassInfos; }
  operator const EbmlId &() const { return ClassInfos.GlobalId; }
  EbmlElement *Clone() const { return new KaxContentCompression(*this); }
};

class MATROSKA_DLL_API KaxContentCompAlgo: public EbmlUInteger {
public:
  static const EbmlCallbacks ClassInfos;

  KaxContentCompAlgo(): EbmlUInteger(0) {}
  KaxContentCompAlgo(const KaxContentCompAlgo &ElementToClone):
    EbmlUInteger(ElementToClone) {}
  static EbmlElement &Create() { return *(new KaxContentCompAlgo); }
  const EbmlCallbacks &Generic() const { return ClassInfos; }
  operator const EbmlId &() const { return ClassInfos.GlobalId; }
  EbmlElement *Clone() const { return new KaxContentCompAlgo(*this); }
};

class MATROSKA_DLL_API KaxContentCompSettings: public EbmlBinary {
public:
  static const EbmlCallbacks ClassInfos;

  KaxContentCompSettings() {}
  KaxContentCompSettings(const KaxContentCompSettings &ElementToClone):
    EbmlBinary(ElementToClone) {}
  static EbmlElement &Create() { return *(new KaxContentCompSettings); }
  const EbmlCallbacks &Generic() const { return ClassInfos; }
  operator const EbmlId &() const { return ClassInfos.GlobalId; }
  EbmlElement *Clone() const {
    return new KaxContentCompSettings(*this);
  }
  bool ValidateSize(void) const { return true; }
};

class MATROSKA_DLL_API KaxContentEncryption: public EbmlMaster {
public:
  static const EbmlCallbacks ClassInfos;

  KaxContentEncryption();
  KaxContentEncryption(const KaxContentEncryption &ElementToClone):
    EbmlMaster(ElementToClone) {}
  static EbmlElement &Create() { return *(new KaxContentEncryption); }
  const EbmlCallbacks &Generic() const { return ClassInfos; }
  operator const EbmlId &() const { return ClassInfos.GlobalId; }
  EbmlElement *Clone() const { return new KaxContentEncryption(*this); }
};

class MATROSKA_DLL_API KaxContentEncAlgo: public EbmlUInteger {
public:
  static const EbmlCallbacks ClassInfos;

  KaxContentEncAlgo(): EbmlUInteger(0) {}
  KaxContentEncAlgo(const KaxContentEncAlgo &ElementToClone):
    EbmlUInteger(ElementToClone) {}
  static EbmlElement &Create() { return *(new KaxContentEncAlgo); }
  const EbmlCallbacks &Generic() const { return ClassInfos; }
  operator const EbmlId &() const { return ClassInfos.GlobalId; }
  EbmlElement *Clone() const { return new KaxContentEncAlgo(*this); }
};

class MATROSKA_DLL_API KaxContentEncKeyID: public EbmlBinary {
public:
  static const EbmlCallbacks ClassInfos;

  KaxContentEncKeyID() {}
  KaxContentEncKeyID(const KaxContentEncKeyID &ElementToClone):
    EbmlBinary(ElementToClone) {}
  static EbmlElement &Create() { return *(new KaxContentEncKeyID); }
  const EbmlCallbacks &Generic() const { return ClassInfos; }
  operator const EbmlId &() const { return ClassInfos.GlobalId; }
  EbmlElement *Clone() const { return new KaxContentEncKeyID(*this); }
  bool ValidateSize(void) const { return true; }
};

class MATROSKA_DLL_API KaxContentSignature: public EbmlBinary {
public:
  static const EbmlCallbacks ClassInfos;

  KaxContentSignature() {}
  KaxContentSignature(const KaxContentSignature &ElementToClone):
    EbmlBinary(ElementToClone) {}
  static EbmlElement &Create() { return *(new KaxContentSignature); }
  const EbmlCallbacks &Generic() const { return ClassInfos; }
  operator const EbmlId &() const { return ClassInfos.GlobalId; }
  EbmlElement *Clone() const { return new KaxContentSignature(*this); }
  bool ValidateSize(void) const { return true; }
};

class MATROSKA_DLL_API KaxContentSigKeyID: public EbmlBinary {
public:
  static const EbmlCallbacks ClassInfos;

  KaxContentSigKeyID() {}
  KaxContentSigKeyID(const KaxContentSigKeyID &ElementToClone):
    EbmlBinary(ElementToClone) {}
  static EbmlElement &Create() { return *(new KaxContentSigKeyID); }
  const EbmlCallbacks &Generic() const { return ClassInfos; }
  operator const EbmlId &() const { return ClassInfos.GlobalId; }
  EbmlElement *Clone() const { return new KaxContentSigKeyID(*this); }
  bool ValidateSize(void) const { return true; }
};

class MATROSKA_DLL_API KaxContentSigAlgo: public EbmlUInteger {
public:
  static const EbmlCallbacks ClassInfos;

  KaxContentSigAlgo() {}
  KaxContentSigAlgo(const KaxContentSigAlgo &ElementToClone):
    EbmlUInteger(ElementToClone) {}
  static EbmlElement &Create() { return *(new KaxContentSigAlgo); }
  const EbmlCallbacks &Generic() const { return ClassInfos; }
  operator const EbmlId &() const { return ClassInfos.GlobalId; }
  EbmlElement *Clone() const { return new KaxContentSigAlgo(*this); }
};

class MATROSKA_DLL_API KaxContentSigHashAlgo: public EbmlUInteger {
public:
  static const EbmlCallbacks ClassInfos;

  KaxContentSigHashAlgo() {}
  KaxContentSigHashAlgo(const KaxContentSigHashAlgo &ElementToClone):
    EbmlUInteger(ElementToClone) {}
  static EbmlElement &Create() { return *(new KaxContentSigHashAlgo); }
  const EbmlCallbacks &Generic() const { return ClassInfos; }
  operator const EbmlId &() const { return ClassInfos.GlobalId; }
  EbmlElement *Clone() const { return new KaxContentSigHashAlgo(*this); }
};

END_LIBMATROSKA_NAMESPACE

#endif // LIBMATROSKA_CONTENT_ENCODING_H
