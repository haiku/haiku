/*
 * Copyright 2013-2014, Stephan Aßmus <superstippi@gmx.de>.
 * Copyright 2016-2023, Andrew Lindesay <apl@lindesay.co.nz>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */
 #ifndef PUBLISHER_INFO_H
 #define PUBLISHER_INFO_H


 #include <String.h>


 class PublisherInfo {
 public:
 								PublisherInfo();
 								PublisherInfo(const BString& name,
 									const BString& email,
 									const BString& website);
 								PublisherInfo(const PublisherInfo& other);

 			PublisherInfo&		operator=(const PublisherInfo& other);
 			bool				operator==(const PublisherInfo& other) const;
 			bool				operator!=(const PublisherInfo& other) const;

 			const BString&		Name() const
 									{ return fName; }
 			const BString&		Email() const
 									{ return fEmail; }
 			const BString&		Website() const
 									{ return fWebsite; }

 private:
 			BString				fName;
 			BString				fEmail;
 			BString				fWebsite;
 };

 #endif // PUBLISHER_INFO_H
