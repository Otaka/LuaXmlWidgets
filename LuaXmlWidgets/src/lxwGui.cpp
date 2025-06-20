//
//  lxwGui.cpp
//

#include "lxw.hpp"
#include "wx/filename.h"

lxwGui::lxwGui() {
    engine = new lxe::Engine();
    engine->getLua()->evalExpression("require \"resource://lxw/lxw.lua\"", [](bool state, wxString&result){
        if(!state)throw RuntimeException("Error while load lxw.lua module "+result);
    });
    toolWindow = new wxDialog(NULL, -1, "", wxPoint(1,1), wxSize(1,1), 0);
    engine->registerTagFactory("App", [this](){return initDomElement(new App());});
    engine->registerTagFactory("Window", [this](){return initDomElement(new Window());});
    engine->registerTagFactory("Button", [this](){return initDomElement(new Button());});
    engine->registerTagFactory("Label",  [this]{return initDomElement(new Label());});
    engine->registerTagFactory("TextInput", [this](){return initDomElement(new TextInput());});
    engine->registerTagFactory("CheckBox",  [this](){return initDomElement(new CheckBox());});
    engine->registerTagFactory("DropDown",  [this](){return initDomElement(new DropDown());});
    engine->registerTagFactory("Option", [this](){return initDomElement(new Option());});
    engine->registerTagFactory("Progress",  [this](){return initDomElement(new Progress());});
    engine->registerTagFactory("Hyperlink", [this](){return initDomElement(new Hyperlink());});
    engine->registerTagFactory("GlobalHotkey", [this](){return initDomElement(new GlobalHotkey());});
    engine->registerTagFactory("Tree", [this](){return initDomElement(new Tree());});
    engine->registerTagFactory("TreeNode", [this](){return initDomElement(new TreeNode());});
}

LxwDomElement*lxwGui::initDomElement(LxwDomElement*domElement) {
    domElement->setGui(this);
    return domElement;
}

void lxwGui::load(wxString filePath) {
    wxFileName fileName(filePath);
    if (!fileName.Exists()) {
        throw std::runtime_error(wxString::Format("File not found '%s'", filePath).ToUTF8().data());
    }

    wxFile sourceFile=wxFile(fileName.GetAbsolutePath());
    
    wxString source;
    sourceFile.ReadAll(&source);
    load(source, fileName.GetAbsolutePath());
}

void lxwGui::load(wxString content, wxString filePath) {
    engine->run(content, filePath);
}
