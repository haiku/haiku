/*
 * Copyright 2026, Andrew Lindesay <apl@lindesay.co.nz>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef SIMPLE_ALERT_H
#define SIMPLE_ALERT_H


#include <Alert.h>
#include <Archivable.h>
#include <String.h>


/*!	A model for conveying a simple alert. This can be passed from background
	processes to the application for display in the UI.
*/

class SimpleAlert : public BArchivable {
public:
								SimpleAlert(const BMessage* from);
								SimpleAlert(const BString& title, const BString& text,
									alert_type type = B_INFO_ALERT);
								SimpleAlert();
	virtual						~SimpleAlert();

	const	BString				Title() const;
	const	BString				Text() const;
			alert_type			Type() const;

			bool				operator==(const SimpleAlert& other) const;

			status_t			Archive(BMessage* into, bool deep = true) const;

private:
			BString				fTitle;
			BString				fText;
			alert_type			fType;
};


#endif // SIMPLE_ALERT_H
