// MenuItem.h

#ifndef _MENUITEM_h
#define _MENUITEM_h

#if defined(ARDUINO) && ARDUINO >= 100
	#include "arduino.h"
#else
	#include "WProgram.h"
#endif

typedef void(*MENU_ACTION_CALLBACK_FUNC)();

class MenuItem {

private:
	MenuItem *previous;
	MenuItem *parent;
	MenuItem *children;
	MenuItem *next;
	MENU_ACTION_CALLBACK_FUNC callback;
	bool refresh;
	int _id;
	bool isSelectable;

public:
	MenuItem();
	MenuItem(MENU_ACTION_CALLBACK_FUNC func);
	MenuItem(int id);
	MenuItem(int id, MENU_ACTION_CALLBACK_FUNC func, bool isselectable);

	void AddNewSibling(MenuItem *item);
	void AddChild(MenuItem *item);

	MenuItem *GetPreviousSibling();
	MenuItem *GetNextSibling();
	MenuItem *GetChild();
	MenuItem *GetParent();

	void ExecuteCallback();
	void SetCallbackFunction(MENU_ACTION_CALLBACK_FUNC func);

	int GetId();
	bool IsSelectable();
	void IsSelectable(bool selectable);

	bool Refresh();
	void RequestRefresh();
};

#endif

