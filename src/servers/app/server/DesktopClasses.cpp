#include "DesktopClasses.h"

Workspace::Workspace(const graphics_card_info &gcinfo, const frame_buffer_info &fbinfo)
{
	_gcinfo=gcinfo;
	_fbinfo=fbinfo;
	
	//TODO: create the root layer here based on gcinfo and fbinfo.
	_rootlayer=NULL;
}

Workspace::~Workspace(void)
{
	if(_rootlayer)
	{
		_rootlayer->PruneTree();
		delete _rootlayer;
	}
}

void Workspace::SetBGColor(const RGBColor &c)
{
	_rootlayer->SetColor(c);
}

RootLayer *Workspace::GetRoot(void)
{
	return _rootlayer;
}

void Workspace::SetData(const graphics_card_info &gcinfo, const frame_buffer_info &fbinfo)
{
	_gcinfo=gcinfo;
	_fbinfo=fbinfo;
}

void Workspace::GetData(graphics_card_info *gcinfo, frame_buffer_info *fbinfo)
{
	*gcinfo=_gcinfo;
	*fbinfo=_fbinfo;
}

Screen::Screen(DisplayDriver *gfxmodule, uint8 workspaces)
{
	// TODO: implement
	_workspacelist=NULL;
	_resolution=0;
	_activewin=NULL;
	_currentworkspace=-1;
	_workspacecount=0;
	_driver=NULL;
	_init=false;
}

Screen::~Screen(void)
{
}

void Screen::AddWorkspace(int32 index=-1)
{
}

void Screen::DeleteWorkspace(int32 index)
{
}

int32 Screen::CountWorkspaces(void)
{
	return _workspacecount;
}

void Screen::SetWorkspaceCount(int32 count)
{
}

int32 Screen::CurrentWorkspace(void)
{
	return _currentworkspace;
}

void Screen::SetWorkspace(int32 index)
{
}

void Screen::Activate(bool active=true)
{
}

DisplayDriver *Screen::GetGfxDriver(void)
{
	return _driver;
}

status_t Screen::SetSpace(int32 index, int32 res,bool stick=true)
{
	return B_OK;
}

void Screen::AddWindow(ServerWindow *win, int32 workspace=B_CURRENT_WORKSPACE)
{
}

void Screen::RemoveWindow(ServerWindow *win)
{
}

ServerWindow *Screen::ActiveWindow(void)
{
	return _activewin;
}

void Screen::SetActiveWindow(ServerWindow *win)
{
}

Layer *Screen::GetRootLayer(int32 workspace=B_CURRENT_WORKSPACE)
{
	return (Layer*)_activeworkspace->GetRoot();
}

bool Screen::IsInitialized(void)
{
	return _init;
}

Workspace *Screen::GetActiveWorkspace(void)
{
	return _activeworkspace;
}

