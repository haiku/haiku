NAME= Cortex
TYPE= APP

#%{
# @src->@ 

SRCS= \
	RouteApp/ConnectionIO.cpp \
	RouteApp/DormantNodeIO.cpp \
	RouteApp/LiveNodeIO.cpp \
	RouteApp/NodeSetIOContext.cpp \
	RouteApp/RouteApp.cpp \
	RouteApp/RouteAppNodeManager.cpp \
	RouteApp/RouteWindow.cpp \
	RouteApp/route_app_io.cpp \
	RouteApp/StatusView.cpp \
	DiagramView/DiagramBox.cpp \
	DiagramView/DiagramEndPoint.cpp \
	DiagramView/DiagramItem.cpp \
	DiagramView/DiagramItemGroup.cpp \
	DiagramView/DiagramView.cpp \
	DiagramView/DiagramWire.cpp \
	DormantNodeView/DormantNodeListItem.cpp \
	DormantNodeView/DormantNodeView.cpp \
	DormantNodeView/DormantNodeWindow.cpp \
	expat/xmlparse/xmlparse.c \
	expat/xmlparse/hashtable.c \
	expat/xmltok/xmltok.c \
	expat/xmltok/xmlrole.c \
	InfoView/AppNodeInfoView.cpp \
	InfoView/ConnectionInfoView.cpp \
	InfoView/DormantNodeInfoView.cpp \
	InfoView/EndPointInfoView.cpp \
	InfoView/FileNodeInfoView.cpp \
	InfoView/InfoView.cpp \
	InfoView/InfoWindow.cpp \
	InfoView/InfoWindowManager.cpp \
	InfoView/LiveNodeInfoView.cpp \
	MediaRoutingView/MediaJack.cpp \
	MediaRoutingView/MediaNodePanel.cpp \
	MediaRoutingView/MediaRoutingView.cpp \
	MediaRoutingView/MediaWire.cpp \
	NodeManager/AddOnHost.cpp \
	NodeManager/Connection.cpp \
	NodeManager/NodeManager.cpp \
	NodeManager/NodeGroup.cpp \
	NodeManager/NodeRef.cpp \
	NodeManager/NodeSyncThread.cpp \
	ParameterView/ParameterWindowManager.cpp \
	ParameterView/ParameterWindow.cpp \
	ParameterView/ParameterContainerView.cpp \
	Persistence/ExportContext.cpp \
	Persistence/ImportContext.cpp \
	Persistence/Importer.cpp \
	Persistence/StringContent.cpp \
	Persistence/XML.cpp \
	Persistence/Wrappers/MediaFormatIO.cpp \
	Persistence/Wrappers/MessageIO.cpp \
	TipManager/TipManager.cpp \
	TipManager/TipManagerImpl.cpp \
	TipManager/TipWindow.cpp \
	TipManager/TipView.cpp \
	TransportView/TransportView.cpp \
	TransportView/TransportWindow.cpp \
	ValControl/ValControl.cpp \
	ValControl/ValControlSegment.cpp \
	ValControl/ValControlDigitSegment.cpp \
	ValControl/ValCtrlLayoutEntry.cpp \
	ValControl/NumericValControl.cpp \
	support/debug_tools.cpp \
	support/MediaIcon.cpp \
	support/MediaString.cpp \
	support/MouseTrackingHelpers.cpp \
	support/MultiInvoker.cpp \
	support/ObservableHandler.cpp \
	support/ObservableLooper.cpp \
	support/observe.cpp \
	support/SoundUtils.cpp \
	support/TextControlFloater.cpp
	
RSRCS= Resource.rsrc

# @<-src@ 
#%}

LIBS= be media tracker translation mail
X86_LIBS= stdc++.r4
PPC_LIBS= mslcpp_4_0

LOCAL_INCLUDE_PATHS = .

#	specify the level of optimization that you desire
#	NONE, SOME, FULL
OPTIMIZE= FULL

DEFINES= #DEBUG=1 NDEBUG=0
DEBUGGER = #TRUE

#	NONE = supress all warnings
#	ALL = enable all warnings
WARNINGS = ALL

#	specify whether image symbols will be created
#	so that stack crawls in the debugger are meaningful
#	if TRUE symbols will be created
SYMBOLS = 

# if TRUE, all symbols will be removed
STRIP_SYMBOLS = TRUE


COMPILER_FLAGS = -DCORTEX_NAMESPACE=cortex
LINKER_FLAGS =

## include modified makefile-engine
include makefile-engine

