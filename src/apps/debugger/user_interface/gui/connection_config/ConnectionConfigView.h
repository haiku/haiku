/*
 * Copyright 2016, Rene Gollent, rene@gollent.com.
 * Distributed under the terms of the MIT License.
 */
#ifndef CONNECTION_CONFIG_VIEW_H
#define CONNECTION_CONFIG_VIEW_H

#include <View.h>


class Settings;
class TargetHostInterfaceInfo;


class ConnectionConfigView : public BView {
public:
	class Listener;
								ConnectionConfigView(const char* name);
	virtual						~ConnectionConfigView();

			status_t			Init(TargetHostInterfaceInfo* info,
									Listener* listener);

protected:
			TargetHostInterfaceInfo* InterfaceInfo() const
									{ return fInfo; }
			void				NotifyConfigurationChanged(Settings* settings);

	virtual	status_t			InitSpecific() = 0;

private:
			TargetHostInterfaceInfo* fInfo;
			Listener* fListener;
};


class ConnectionConfigView::Listener {
public:
	virtual						~Listener();

	virtual	void				ConfigurationChanged(Settings* settings) = 0;
};


#endif	// CONNECTION_CONFIG_VIEW_H
