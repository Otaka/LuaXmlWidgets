//
//  lxwControls.hpp
//

#ifndef lxwControls_hpp
#define lxwControls_hpp
#include "lxw.hpp"
#include "layoutEngine.hpp"

#include <wx/hyperlink.h>
#include <wx/treectrl.h>

class LxwDomElement: public virtual lxe::DomElement {
    lxwGui*gui;
public:
    void setGui(lxwGui*gui){
        this->gui=gui;
    }
    lxwGui*getGui() { return gui; }
};

class App: public virtual LxwDomElement {
    static String2BoolHashMap*allowedAttributes;
    static String2BoolHashMap*recreationRequiredAttributes;
public:
    App();
};

class AbstractWindow: public virtual LxwDomElement {
    static String2BoolHashMap*allowedAttributes;
    static String2BoolHashMap*recreationRequiredAttributes;
    wxWindow *window;
    bool tabTraversal;
    
    // Layout engine integration
    LayoutEngine::LayoutEntity* layoutEntity = nullptr;
    LayoutEngine::FlexGridLayout* layoutManager = nullptr;  // Only for containers
    LayoutEngine::EntityConstraints* layoutConstraints = nullptr;  // Child's own constraints
    bool isLayoutContainer = false;
    bool layoutDirty = false;  // Flag to track when layout needs recalculation
    
protected:
    wxBorder getBorder();
public:
    AbstractWindow();
    virtual int getComputedWindowStyle();
    virtual void repaint()override {this->window->Refresh();}
    virtual void onWillAddToParent(DomElement*parentElement) override;
    virtual void destroyElement() override;
    virtual void onFinishedInitialisation() override;
    void setWindow(wxWindow *window) { 
        this->window=window;
        // Initialize layout entity now that window is available
        initLayoutEntity();
    }
    wxWindow* getWindow()const{return window;}
    virtual bool handleChangedAttribute(const wxString&name, lxe::TagAttribute&oldValue, lxe::TagAttribute&newValue)override;
    virtual bool getDynamicAttributeValue(const wxString&attributeName, lxe::TagAttribute&tagAttribute)override;
    void ensureCaretPresent();
    
    // Layout container management
    void setLayoutContainer(const wxString& layoutConfig);
    void performLayout();
    void invalidateLayout();
    
    // Layout entity management
    LayoutEngine::LayoutEntity* getLayoutEntity();
    void setLayoutConstraints(const wxString& constraintString);
    
    // Layout callbacks
    void onLayoutPositionChanged(float x, float y, float width, float height);
    
    // Debug helper methods
    std::string getLayoutDebugInfo();
    void printLayoutDebugInfo() {
        wxLogDebug("%s", getLayoutDebugInfo().c_str());
    }
    
    // Phase 2 enhancements: Advanced layout features
    void validateLayoutConstraints() const;
    void performLayoutWithCascadePrevention();
    static std::pair<size_t, size_t> getLayoutCacheStats();
    static void clearLayoutCache();
    
    // Override existing DOM methods to integrate layout
    virtual void onChildAdded(lxe::DomElement*child) override;
    virtual void onChildRemoving(lxe::DomElement*child) override;
    
private:
    void initLayoutEntity();
    void destroyLayoutResources();
    bool parseLayoutContainer(const wxString& config);
    void rebuildLayoutFromChildren();  // Rebuilds layout from current children
};

class Control: public virtual AbstractWindow {
    static String2BoolHashMap*allowedAttributes;
    static String2BoolHashMap*recreationRequiredAttributes;
protected:
public:
    Control();
    virtual bool handleChangedAttribute(const wxString&name, lxe::TagAttribute&oldValue, lxe::TagAttribute&newValue)override;
    virtual bool getDynamicAttributeValue(const wxString&attributeName, lxe::TagAttribute&tagAttribute)override;
};


class Window: public virtual AbstractWindow {
    static String2BoolHashMap*allowedAttributes;
    static String2BoolHashMap*recreationRequiredAttributes;
public:
    Window();
    virtual void initElement(DomElement*parent,wxArrayString*attributesNames)override;
    virtual bool handleChangedAttribute(const wxString&name, lxe::TagAttribute&oldValue, lxe::TagAttribute&newValue)override;
    virtual bool getDynamicAttributeValue(const wxString&attributeName, lxe::TagAttribute&tagAttribute)override;
    virtual void onWillAddToParent(lxe::DomElement*parentElement) override;
};


class Button: public virtual Control {
    static String2BoolHashMap*allowedAttributes;
    static String2BoolHashMap*recreationRequiredAttributes;
    ImageHolder imageHolder;
    ImageHolder disabledImageHolder;
    ImageHolder pressedImageHolder;
    ImageHolder hoverImageHolder;
public:
    Button();
    virtual void initElement(DomElement*parent,wxArrayString*attributesNames)override;
    virtual bool handleChangedAttribute(const wxString&name, lxe::TagAttribute&oldValue, lxe::TagAttribute&newValue)override;
    virtual bool getDynamicAttributeValue(const wxString&attributeName, lxe::TagAttribute&tagAttribute)override;
    void onClickEventHandler(wxCommandEvent&e);
};

class CheckBox: public virtual Control {
    static String2BoolHashMap*allowedAttributes;
    static String2BoolHashMap*recreationRequiredAttributes;
public:
    CheckBox();
    virtual void initElement(DomElement*parent,wxArrayString*attributesNames)override;
    virtual bool handleChangedAttribute(const wxString&name, lxe::TagAttribute&oldValue, lxe::TagAttribute&newValue)override;
    virtual bool getDynamicAttributeValue(const wxString&attributeName, lxe::TagAttribute&tagAttribute)override;
    void onChangeEventHandler(wxCommandEvent&e);
};


class Label: public virtual Control {
    static String2BoolHashMap*allowedAttributes;
    static String2BoolHashMap*recreationRequiredAttributes;
    bool htmlMarkup=false;
public:
    Label();
    virtual void initElement(DomElement*parent,wxArrayString*attributesNames)override;
    virtual bool handleChangedAttribute(const wxString&name, lxe::TagAttribute&oldValue, lxe::TagAttribute&newValue)override;
    virtual bool getDynamicAttributeValue(const wxString&attributeName, lxe::TagAttribute&tagAttribute)override;
};


class TextInput: public virtual Control {
    static String2BoolHashMap*allowedAttributes;
    static String2BoolHashMap*recreationRequiredAttributes;
public:
    TextInput();
    virtual void initElement(DomElement*parent,wxArrayString*attributesNames)override;
    virtual bool handleChangedAttribute(const wxString&name, lxe::TagAttribute&oldValue, lxe::TagAttribute&newValue)override;
    virtual bool getDynamicAttributeValue(const wxString&attributeName, lxe::TagAttribute&tagAttribute)override;
};

class DropDown: public virtual Control {
    static String2BoolHashMap*allowedAttributes;
    static String2BoolHashMap*recreationRequiredAttributes;
public:
    DropDown();
    virtual void initElement(lxe::DomElement*parent,wxArrayString*attributesNames)override;
    virtual bool handleChangedAttribute(const wxString&name, lxe::TagAttribute&oldValue, lxe::TagAttribute&newValue)override;
    virtual bool getDynamicAttributeValue(const wxString&attributeName, lxe::TagAttribute&tagAttribute)override;
    virtual void onChildAdded(lxe::DomElement*child) override;
    virtual void onChildRemoving(lxe::DomElement*child) override;
    virtual void onChildChanged(lxe::DomElement*child, wxString changeType) override;
    void onChangeEventHandler(wxCommandEvent&e);
};

class Option: public virtual LxwDomElement {
    static String2BoolHashMap*allowedAttributes;
    static String2BoolHashMap*recreationRequiredAttributes;
public:
    Option();
    virtual void initElement(lxe::DomElement*parent, wxArrayString*attributesNames)override;
    virtual bool handleChangedAttribute(const wxString&name, lxe::TagAttribute&oldValue, lxe::TagAttribute&newValue)override;
    virtual bool getDynamicAttributeValue(const wxString&attributeName, lxe::TagAttribute&tagAttribute)override;
};

class Progress: public virtual Control {
    static String2BoolHashMap*allowedAttributes;
    static String2BoolHashMap*recreationRequiredAttributes;
    int value;
    bool indeterminate;
public:
    Progress();
    virtual void initElement(DomElement*parent,wxArrayString*attributesNames)override;
    virtual bool handleChangedAttribute(const wxString&name, lxe::TagAttribute&oldValue, lxe::TagAttribute&newValue)override;
    virtual bool getDynamicAttributeValue(const wxString&attributeName, lxe::TagAttribute&tagAttribute)override;
};

class Hyperlink: public virtual Control {
    static String2BoolHashMap*allowedAttributes;
    static String2BoolHashMap*recreationRequiredAttributes;
    
public:
    Hyperlink();
    virtual void initElement(lxe::DomElement*parent, wxArrayString*attributesNames)override;
    virtual bool handleChangedAttribute(const wxString&name, lxe::TagAttribute&oldValue, lxe::TagAttribute&newValue)override;
    virtual bool getDynamicAttributeValue(const wxString&attributeName, lxe::TagAttribute&tagAttribute)override;
    void onHyperLinkEventHandler(wxHyperlinkEvent&e);
};

class GlobalHotkey: public virtual LxwDomElement {
    static String2BoolHashMap*allowedAttributes;
    static String2BoolHashMap*recreationRequiredAttributes;
    int hotkeyId;
public:
    GlobalHotkey();
    virtual void initElement(lxe::DomElement*parent, wxArrayString*attributesNames) override;
    virtual void destroyElement() override;
    virtual bool handleChangedAttribute(const wxString&name, lxe::TagAttribute&oldValue, lxe::TagAttribute&newValue) override;
    void onHotkey(wxKeyEvent&e);
};

class TreeNode;
class Tree: public virtual Control {
    static String2BoolHashMap*allowedAttributes;
    static String2BoolHashMap*recreationRequiredAttributes;
    wxTreeItemId rootItemId;
public:
    Tree();
    virtual void initElement(lxe::DomElement*parent, wxArrayString*attributesNames)override;
    virtual bool handleChangedAttribute(const wxString&name, lxe::TagAttribute&oldValue, lxe::TagAttribute&newValue)override;
    virtual void onChildAdded(lxe::DomElement*child) override;
    virtual void onChildRemoving(lxe::DomElement*child) override;
    virtual void onChildChanged(lxe::DomElement*child, wxString changeType) override;
    void onNodeAdded(wxTreeItemId parentNodeId, TreeNode*node);
    void onNodeRemoved(TreeNode*node);
};

class TreeNode: public virtual LxwDomElement {
    static String2BoolHashMap*allowedAttributes;
    static String2BoolHashMap*recreationRequiredAttributes;
    Tree*owner=NULL;
    wxTreeItemId itemId;
public:
    TreeNode();
    virtual void initElement(lxe::DomElement*parent, wxArrayString*attributesNames)override;
    virtual bool handleChangedAttribute(const wxString&name, lxe::TagAttribute&oldValue, lxe::TagAttribute&newValue)override;
    void setOwner(Tree*owner){this->owner=owner;}
    Tree*getOwner(){return owner;}
    void notifyOwnerAboutChange(wxString changeType);
    void setItemId(wxTreeItemId itemId){this->itemId=itemId;}
    wxTreeItemId getItemId(){return itemId;}
    virtual void onChildAdded(lxe::DomElement*child) override;
    virtual void onChildRemoving(lxe::DomElement*child) override;
};

class Panel: public virtual AbstractWindow {
    static String2BoolHashMap*allowedAttributes;
    static String2BoolHashMap*recreationRequiredAttributes;
public:
    Panel();
    virtual void initElement(lxe::DomElement*parent,wxArrayString*attributesNames)override;
    virtual bool handleChangedAttribute(const wxString&name, lxe::TagAttribute&oldValue, lxe::TagAttribute&newValue)override;
    virtual bool getDynamicAttributeValue(const wxString&attributeName, lxe::TagAttribute&tagAttribute)override;
};


#endif /* lxwControls_hpp */
