#ifndef DESKTOPCLASSES_H
#define DESKTOPCLASSES_H

#include <SupportDefs.h>
#include <GraphicsCard.h>
#include <Window.h>	// for workspace defs
#include "RootLayer.h"

class DisplayDriver;
class ServerWindow;
class RGBColor;

class Workspace
{
public:
	Workspace(const graphics_card_info &gcinfo, const frame_buffer_info &fbinfo);
	~Workspace(void);
	void SetBGColor(const RGBColor &c);
	RootLayer *GetRoot(void);
	void SetData(const graphics_card_info &gcinfo, const frame_buffer_info &fbinfo);
	void GetData(graphics_card_info *gcinfo, frame_buffer_info *fbinfo);

protected:
	RootLayer *_rootlayer;
	graphics_card_info _gcinfo;
	frame_buffer_info _fbinfo;
};

class Screen
{
public:
	Screen(DisplayDriver *gfxmodule, uint8 workspaces);
	~Screen(void);
	void AddWorkspace(int32 index=-1);
	void DeleteWorkspace(int32 index);
	int32 CountWorkspaces(void);
	void SetWorkspaceCount(int32 count);
	int32 CurrentWorkspace(void);
	void SetWorkspace(int32 index);
	void Activate(bool active=true);
	DisplayDriver *GetGfxDriver(void);
	status_t SetSpace(int32 index, int32 res,bool stick=true);
	void AddWindow(ServerWindow *win, int32 workspace=B_CURRENT_WORKSPACE);
	void RemoveWindow(ServerWindow *win);
	ServerWindow *ActiveWindow(void);
	void SetActiveWindow(ServerWindow *win);
	Layer *GetRootLayer(int32 workspace=B_CURRENT_WORKSPACE);
	bool IsInitialized(void);
	Workspace *GetActiveWorkspace(void);

protected:
	int32 _resolution;
	ServerWindow *_activewin;
	int32 _currentworkspace;
	int32 _workspacecount;
	BList *_workspacelist;
	DisplayDriver *_driver;
	bool _init;
	Workspace *_activeworkspace;
};

#endif