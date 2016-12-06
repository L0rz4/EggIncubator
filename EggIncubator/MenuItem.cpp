// 
// 
// 

#include "MenuItem.h"

MenuItem::MenuItem()
{
	this->children = nullptr;
	this->parent = nullptr;
	this->previous = nullptr;
	this->next = nullptr;
	this->callback = nullptr;
	this->refresh = false;
	this->isSelectable = false;
}

MenuItem::MenuItem(MENU_ACTION_CALLBACK_FUNC func) {
	this->children = nullptr;
	this->parent = nullptr;
	this->previous = nullptr;
	this->next = nullptr;
	this->callback = nullptr;
	this->refresh = false;
	this->_id = 0;
	this->IsSelectable(false);
	this->SetCallbackFunction(func);
}

MenuItem::MenuItem(int id) {
	this->children = nullptr;
	this->parent = nullptr;
	this->previous = nullptr;
	this->next = nullptr;
	this->callback = nullptr;
	this->refresh = false;
	this->_id = id;
	this->IsSelectable(false);
}

MenuItem::MenuItem(int id, MENU_ACTION_CALLBACK_FUNC func, bool isselectable) {
	this->children = nullptr;
	this->parent = nullptr;
	this->previous = nullptr;
	this->next = nullptr;
	this->callback = nullptr;
	this->refresh = false;
	this->_id = id;
	this->IsSelectable(isselectable);
	this->SetCallbackFunction(func);
}


void MenuItem::AddNewSibling(MenuItem *item) {
	if (this->next == nullptr) {
		this->next = item;
		item->previous = this;
		item->parent = this->parent;
	}
	else {
		this->next->AddNewSibling(item);
	}
	
}

MenuItem *MenuItem::GetPreviousSibling() {
	return this->previous;
}

MenuItem *MenuItem::GetNextSibling() {
	return this->next;
}

MenuItem *MenuItem::GetChild() {
	return this->children;
}

MenuItem *MenuItem::GetParent() {
	return this->parent;
}

void MenuItem::AddChild(MenuItem *item) {
	if (this->children == nullptr) {
		this->children = item;
		item->parent = this;
	}
	else {
		this->children->AddNewSibling(item);
		item->parent = this;
	}
}

void MenuItem::SetCallbackFunction(MENU_ACTION_CALLBACK_FUNC func) {
	this->callback = func;
}

void MenuItem::ExecuteCallback() {
	if (this->callback != nullptr) {
		this->callback();
	}
	
}

int MenuItem::GetId() {
	return _id;
}


bool MenuItem::IsSelectable() {
	return isSelectable;
}

void MenuItem::IsSelectable(bool selectable) {
	this->isSelectable = selectable;
}

bool MenuItem::Refresh() {
	if (refresh) {
		refresh = false;
		return true;
	}
	else {
		return refresh;
	}
}

void MenuItem::RequestRefresh() {
	this->refresh = true;

}