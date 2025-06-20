//
// lxwGui.hpp
// main include file for LuaXmlWidgets
//
#ifndef lxwGui_h
#define lxwGui_h

#include "lxw.hpp"

class LxwDomElement;

class lxwGui {
    lxe::Engine*engine;
    wxDialog*toolWindow;
    LxwDomElement*initDomElement(LxwDomElement*domElement);
public:
    lxwGui();
    void load(wxString filePath);
    void load(wxString content, wxString filePath);
    wxDialog*getToolWindow() { return toolWindow; };
};
#endif /* lxwGui_h */
