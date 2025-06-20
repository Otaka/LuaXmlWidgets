//
//  lxwControls.cpp
//  LuaXmlWidgets
//

#include "lxw.hpp"

#include <wx/minifram.h>
#include <wx/caret.h>
#include <wx/tglbtn.h>
#include <wx/commandlinkbutton.h>
#include <sstream>

using namespace lxe;

String2BoolHashMap*App::allowedAttributes=NULL;
String2BoolHashMap*App::recreationRequiredAttributes=NULL;
String2BoolHashMap*AbstractWindow::allowedAttributes=NULL;
String2BoolHashMap*AbstractWindow::recreationRequiredAttributes=NULL;
String2BoolHashMap*Control::allowedAttributes=NULL;
String2BoolHashMap*Control::recreationRequiredAttributes=NULL;
String2BoolHashMap*Window::allowedAttributes=NULL;
String2BoolHashMap*Window::recreationRequiredAttributes=NULL;
String2BoolHashMap*Button::allowedAttributes=NULL;
String2BoolHashMap*Button::recreationRequiredAttributes=NULL;
String2BoolHashMap*CheckBox::allowedAttributes=NULL;
String2BoolHashMap*CheckBox::recreationRequiredAttributes=NULL;
String2BoolHashMap*Label::allowedAttributes=NULL;
String2BoolHashMap*Label::recreationRequiredAttributes=NULL;
String2BoolHashMap*TextInput::allowedAttributes=NULL;
String2BoolHashMap*TextInput::recreationRequiredAttributes=NULL;
String2BoolHashMap*DropDown::allowedAttributes=NULL;
String2BoolHashMap*DropDown::recreationRequiredAttributes=NULL;
String2BoolHashMap*Option::allowedAttributes=NULL;
String2BoolHashMap*Option::recreationRequiredAttributes=NULL;
String2BoolHashMap*Progress::allowedAttributes=NULL;
String2BoolHashMap*Progress::recreationRequiredAttributes=NULL;
String2BoolHashMap*Hyperlink::allowedAttributes=NULL;
String2BoolHashMap*Hyperlink::recreationRequiredAttributes=NULL;
String2BoolHashMap*GlobalHotkey::allowedAttributes=NULL;
String2BoolHashMap*GlobalHotkey::recreationRequiredAttributes=NULL;
String2BoolHashMap*Tree::allowedAttributes=NULL;
String2BoolHashMap*Tree::recreationRequiredAttributes=NULL;
String2BoolHashMap*TreeNode::allowedAttributes=NULL;
String2BoolHashMap*TreeNode::recreationRequiredAttributes=NULL;
String2BoolHashMap*Panel::allowedAttributes=NULL;
String2BoolHashMap*Panel::recreationRequiredAttributes=NULL;

wxColour parseAttributeColor(wxString colorString, wxString attributeName) {
    wxColor color;
    color.Set(colorString);
    if(!color.IsOk()){
        throw RuntimeException(wxString::Format("Wrong color value in %s attribute '%s'", attributeName, colorString));
    }
    return color;
}

wxWindow*getParentWindow(DomElement*parent) {
    AbstractWindow*windowDomElement=dynamic_cast<AbstractWindow*>(parent);
    if(windowDomElement==NULL)
        throw RuntimeException(wxString::Format("Required to have parent element to be window DOM element, but it is %s", parent->getTagName()));
    return windowDomElement->getWindow();
}


//------------- App
App::App() {
    if(allowedAttributes==NULL){
        addAllowedAttributeNamesMap((allowedAttributes=createAndFillStringsMap({})));
        recreationRequiredAttributes=createAndFillStringsMap({});
    }
    setChildrenAllowed(true);
}

//------------- AbstractWindow
AbstractWindow::AbstractWindow() {
    window=NULL;
    
    // Initialize layout-related members
    layoutEntity = nullptr;
    layoutManager = nullptr;
    layoutConstraints = nullptr;
    isLayoutContainer = false;
    layoutDirty = false;
    
    if(allowedAttributes==NULL) {
        allowedAttributes=createAndFillStringsMap({"cursor","border","visible","tabtraversal","receiveTabEnter","repaintOnResize","vscroll","hscroll","focusable","width","height","x","y","freeze","label","tooltip","enable","bgcolor","fgcolor","allowDropFiles","helptext","caretx","carety","caretw","careth","caretvisible","font","layoutContainer","layout"});
        recreationRequiredAttributes=createAndFillStringsMap({"border", "tabtraversal", "receiveTabEnter","vscroll","hscroll"});
    }
    addAllowedAttributeNamesMap(allowedAttributes);
    addRecreationAttributeNamesMap(recreationRequiredAttributes);
    
    // Initialize layout entity - will be called later when window is created
    // initLayoutEntity(); // Moved to after window creation
}

int AbstractWindow::getComputedWindowStyle() {
    wxBorder border=getBorder();
    bool tabTraversal=getComputedAttribute("tabtraversal").defaultIfNull(true);
    bool receiveTabEnter=getComputedAttribute("receiveTabEnter").defaultIfNull(false);
    bool repaintOnResize=getComputedAttribute("repaintOnResize").defaultIfNull(false);
    bool vscroll=getComputedAttribute("vscroll").defaultIfNull(false);
    bool hscroll=getComputedAttribute("hscroll").defaultIfNull(false);
    return border
    |(tabTraversal?wxTAB_TRAVERSAL:0)
    |(receiveTabEnter?wxWANTS_CHARS:0)
    |(repaintOnResize?wxFULL_REPAINT_ON_RESIZE:0)
    |(vscroll?wxVSCROLL:0)
    |(hscroll?wxHSCROLL:0)
    ;
}

wxBorder AbstractWindow::getBorder(){
    wxString borderStr=getComputedAttribute("border").defaultIfNull(wxString("default"));
    int border=(wxBorder)selector(borderStr,
                                  {"default",       "simple",       "sunken",       "raised",       "static",       "theme",        "none"},
                                  {wxBORDER_DEFAULT, wxBORDER_SIMPLE,wxBORDER_SUNKEN,wxBORDER_RAISED,wxBORDER_STATIC,wxBORDER_THEME, wxBORDER_NONE});
    if (border==-1)
        throw RuntimeException(wxString::Format("%s 'border' attribute does not accept '%s' value", getTagName(), borderStr));
    return (wxBorder)border;
}

void AbstractWindow::onWillAddToParent(DomElement*parentElement){
    if(window && getParentWindow(parentElement)!=window) {
        window->Reparent(getParentWindow(parentElement));
    }
    
    // Bind resize event for layout containers
    if (isLayoutContainer && window) {
        window->Bind(wxEVT_SIZE, [this](wxSizeEvent& event) {
            this->invalidateLayout();
            event.Skip();
        });
    }
}

void AbstractWindow::onFinishedInitialisation() {
    // Call parent implementation
    LxwDomElement::onFinishedInitialisation();
    
    // If this is a layout container and layout is dirty, rebuild it now
    if (isLayoutContainer && layoutDirty) {
        rebuildLayoutFromChildren();
    }
}

void AbstractWindow::destroyElement() {
    // Cleanup layout resources
    destroyLayoutResources();
    
    if(window!=NULL) {
        delete window;
        window=NULL;
    }
}

bool AbstractWindow::handleChangedAttribute(const wxString&attributeName, TagAttribute&oldValue, TagAttribute&newValue) {
    if(attributeName=="cursor") {
        wxString cursorString=getComputedAttribute(attributeName).defaultIfNull(wxString("default"));
        int cursorIndex=selector(cursorString,
                                             {"default",        "wait",       "arrow",       "rightarrow",        "cross",       "hand",       "question",              "sizenesw",        "sizens",       "sizenwse",       "sizewe",       "sizing",       "ibeam"},
                                             { wxCURSOR_DEFAULT, wxCURSOR_WAIT,wxCURSOR_ARROW,wxCURSOR_RIGHT_ARROW,wxCURSOR_CROSS,wxCURSOR_HAND,wxCURSOR_QUESTION_ARROW, wxCURSOR_SIZENESW, wxCURSOR_SIZENS,wxCURSOR_SIZENWSE,wxCURSOR_SIZEWE,wxCURSOR_SIZING,wxCURSOR_IBEAM});
        if(cursorIndex==-1) {
            throw RuntimeException(wxString::Format("Unknown cursor '%s'", cursorString));
        }
        if (window) {
            window->SetCursor(wxCursor((wxStockCursor)cursorIndex));
        }
        return true;
    }
    if(attributeName=="bgcolor") {
        wxColour c=parseAttributeColor(getComputedAttribute(attributeName).defaultIfNull(wxString("")), attributeName);
        if (window) {
            window->SetBackgroundColour(c);
        }
        return true;
    }
    if(attributeName=="fgcolor") {
        wxString colorString=getComputedAttribute(attributeName).getString();
        wxColor color;
        color.Set(colorString);
        if(!color.IsOk()){
            throw RuntimeException(wxString::Format("Wrong color value in fgcolor attribute '%s'", colorString));
        }
        if (window) {
            window->SetOwnForegroundColour(color);
        }
        return true;
    }
    if(attributeName=="visible") {
        bool visible=getComputedAttribute(attributeName).defaultIfNull(false);
        if(visible)
            window->Show();
        else
            window->Hide();
        return true;
    }
    if(attributeName=="focusable") {
        window->SetCanFocus(getComputedAttributeWithoutDynamic(attributeName).defaultIfNull(false));
        return true;
    }
    if(attributeName=="width") {
        wxSize size=window->GetSize();
        window->SetSize(getComputedAttributeWithoutDynamic(attributeName).defaultIfNull(10), size.GetHeight());
        return true;
    }
    if(attributeName=="height") {
        wxSize size=window->GetSize();
        window->SetSize(size.GetWidth(), getComputedAttributeWithoutDynamic(attributeName).defaultIfNull(10));
        return true;
    }
    if(attributeName=="x") {
        wxPoint pos=window->GetPosition();
        pos.x=getComputedAttributeWithoutDynamic(attributeName).defaultIfNull(0);
        window->SetPosition(pos);
        return true;
    }
    if(attributeName=="y") {
        wxPoint pos=window->GetPosition();
        pos.y=getComputedAttributeWithoutDynamic(attributeName).defaultIfNull(0);
        window->SetPosition(pos);
        return true;
    }
    if(attributeName=="freeze") {
        bool freeze = getComputedAttributeWithoutDynamic(attributeName).defaultIfNull(false);
        if (freeze) { window->Freeze(); } else { window->Thaw();}
        return true;
    }
    if(attributeName=="label") {
        window->SetLabel(getComputedAttributeWithoutDynamic(attributeName).getString());
        return true;
    }
    if(attributeName=="tooltip") {
        window->SetToolTip(getComputedAttributeWithoutDynamic(attributeName).getString());
        return true;
    }
    if(attributeName=="enable") {
        bool enabled=getComputedAttributeWithoutDynamic(attributeName).defaultIfNull(true);
        if (window) {
            if(enabled){
                window->Enable();
            }else{
                window->Disable();
            }
        }
        return true;
    }
    if(attributeName=="allowDropFiles") {
        bool enabled=getComputedAttributeWithoutDynamic(attributeName).defaultIfNull(true);
        if (window) {
            window->DragAcceptFiles(enabled);
        }
        return true;
    }
    if(attributeName=="helptext") {
        if (window) {
            window->SetHelpText(getComputedAttributeWithoutDynamic(attributeName).getString());
        }
        return true;
    }
    if(attributeName=="caretx") {
        ensureCaretPresent();
        if (window && window->GetCaret()) {
            wxPoint pos=window->GetCaret()->GetPosition();
            window->GetCaret()->Move(getComputedAttributeWithoutDynamic(attributeName).defaultIfNull(0), pos.y);
        }
        return true;
    }
    if(attributeName=="carety") {
        ensureCaretPresent();
        if (window && window->GetCaret()) {
            wxPoint pos=window->GetCaret()->GetPosition();
            window->GetCaret()->Move(pos.x,getComputedAttributeWithoutDynamic(attributeName).defaultIfNull(0));
        }
        return true;
    }
    if(attributeName=="caretw") {
        ensureCaretPresent();
        if (window && window->GetCaret()) {
            wxSize size=window->GetCaret()->GetSize();
            window->GetCaret()->SetSize(getComputedAttributeWithoutDynamic(attributeName).defaultIfNull(0), size.y);
        }
        return true;
    }
    if(attributeName=="careth") {
        ensureCaretPresent();
        if (window && window->GetCaret()) {
            wxSize size=window->GetCaret()->GetSize();
            window->GetCaret()->SetSize(size.x, getComputedAttributeWithoutDynamic(attributeName).defaultIfNull(0));
        }
        return true;
    }
    if(attributeName=="caretvisible") {
        ensureCaretPresent();
        bool enabled=getComputedAttributeWithoutDynamic(attributeName).defaultIfNull(true);
        if (window && window->GetCaret()) {
            if(enabled)
                window->GetCaret()->Show();
            else
                window->GetCaret()->Hide();
        }
        return true;
    }
    if(attributeName=="font") {
        wxFont font;
        wxString fontString = getComputedAttributeWithoutDynamic(attributeName).getString();
        if (!font.SetNativeFontInfoUserDesc(fontString)) {
            throw RuntimeException(wxString::Format("Cannot parse font string '%s'", fontString));
        }
        if (window) {
            window->SetFont(font);
        }
        return true;
    }
    
    // Handle layout-specific attributes
    if (attributeName == "layoutContainer") {
        wxString layoutContainerString = getComputedAttributeWithoutDynamic(attributeName).getString();
        setLayoutContainer(layoutContainerString);
        return true;
    }
    if (attributeName == "layout") {
        wxString layoutString = getComputedAttributeWithoutDynamic(attributeName).getString();
        setLayoutConstraints(layoutString);
        // Notify parent that layout needs rebuilding
        if (auto parent = dynamic_cast<AbstractWindow*>(getParent())) {
            if (parent->isLayoutContainer) {
                parent->invalidateLayout();
            }
        }
        return true;
    }
    
    return DomElement::handleChangedAttribute(attributeName, oldValue, newValue);
}

bool AbstractWindow::getDynamicAttributeValue(const wxString&attributeName, TagAttribute&tagAttribute) {
    if(attributeName=="font") {
        if (window) {
            wxString fontDescription=window->GetFont().GetNativeFontInfoUserDesc();
            tagAttribute=TagAttribute().setString(fontDescription);
        }
        return true;
    }
    if(attributeName=="border") {
        tagAttribute=TagAttribute().setString(getAttribute(attributeName, wxString("default")));
        return true;
    }
    if(attributeName=="freeze") {
        if (window) {
            tagAttribute=TagAttribute().setBool(window->IsFrozen());
        }
        return true;
    }
    if(attributeName=="enable") {
        if (window) {
            tagAttribute=TagAttribute().setBool(window->IsEnabled());
        }
        return true;
    }
    return DomElement::getDynamicAttributeValue(attributeName, tagAttribute);
}

void AbstractWindow::ensureCaretPresent() {
    if(window && window->GetCaret()==NULL) {
        window->SetCaret(new wxCaret(window, 1, 1));
    }
}

//------------- Layout Integration Methods for AbstractWindow

void AbstractWindow::initLayoutEntity() {
    if (!layoutEntity) {
        layoutEntity = new LayoutEngine::LayoutEntity();
        layoutEntity->setUpdateCallback([this](float x, float y, float width, float height) {
            this->onLayoutPositionChanged(x, y, width, height);
        });
        
        // Set initial preferred size based on wxWindow
        if (window) {
            wxSize preferredSize = window->GetBestSize();
            layoutEntity->setPreferredSize(preferredSize.x, preferredSize.y);
        } else {
            // Set default size if window is not available yet
            layoutEntity->setPreferredSize(100, 30);
        }
    } else if (window && layoutEntity) {
        // Update preferred size if window was created after layout entity
        wxSize preferredSize = window->GetBestSize();
        layoutEntity->setPreferredSize(preferredSize.x, preferredSize.y);
    }
}

LayoutEngine::LayoutEntity* AbstractWindow::getLayoutEntity() {
    if (!layoutEntity) {
        initLayoutEntity();
    }
    return layoutEntity;
}

void AbstractWindow::destroyLayoutResources() {
    if (layoutManager) {
        delete layoutManager;
        layoutManager = nullptr;
    }
    if (layoutEntity) {
        delete layoutEntity;
        layoutEntity = nullptr;
    }
    if (layoutConstraints) {
        delete layoutConstraints;
        layoutConstraints = nullptr;
    }
}

void AbstractWindow::setLayoutContainer(const wxString& layoutConfig) {
    isLayoutContainer = true;
    if (!layoutManager) {
        layoutManager = new LayoutEngine::FlexGridLayout();
    }
    
    // Parse container configuration using layoutEngineStringParser
    parseLayoutContainer(layoutConfig);
    
    // Only rebuild layout if we're not in init phase and have children
    if (!isInitPhase() && getChildrenCount() > 0) {
        rebuildLayoutFromChildren();
    } else {
        // Mark layout as dirty so it will be rebuilt later
        layoutDirty = true;
    }
}

void AbstractWindow::setLayoutConstraints(const wxString& constraintString) {
    if (!layoutConstraints) {
        layoutConstraints = new LayoutEngine::EntityConstraints();
    }
    
    // Parse constraints using basic parser for Phase 1
    try {
        std::string constraintStr = constraintString.ToStdString();
        LayoutEngine::EntityConstraints* parsedConstraints = LayoutEngine::parseEntityConstraints(constraintStr);
        if (parsedConstraints) {
            *layoutConstraints = *parsedConstraints;
            delete parsedConstraints;
        }
    } catch (const std::exception& e) {
        wxLogError("Layout constraints parsing error: %s", e.what());
    }
}

void AbstractWindow::onLayoutPositionChanged(float x, float y, float width, float height) {
    if (window) {
        window->SetPosition(wxPoint(static_cast<int>(x), static_cast<int>(y)));
        window->SetSize(wxSize(static_cast<int>(width), static_cast<int>(height)));
    }
}

// Override DOM lifecycle methods to integrate layout
void AbstractWindow::onChildAdded(lxe::DomElement*child) {
    // Call parent implementation first
    LxwDomElement::onChildAdded(child);
    
    // If this is a layout container, mark layout as dirty
    if (isLayoutContainer) {
        wxLogDebug("onChildAdded: Adding child %s to layout container %s", 
                  child->getTagName(), getTagName());
        
        // If not in init phase, rebuild layout immediately to include new child
        if (!isInitPhase()) {
            rebuildLayoutFromChildren();
        } else {
            invalidateLayout();
        }
    }
}

void AbstractWindow::onChildRemoving(lxe::DomElement*child) {
    // Call parent implementation first
    LxwDomElement::onChildRemoving(child);
    
    // If this is a layout container, mark layout as dirty
    if (isLayoutContainer) {
        wxLogDebug("onChildRemoving: Removing child %s from layout container %s", 
                  child->getTagName(), getTagName());
        
        // Rebuild layout immediately to update after child removal
        if (!isInitPhase()) {
            rebuildLayoutFromChildren();
        } else {
            invalidateLayout();
        }
    }
}

void AbstractWindow::invalidateLayout() {
    if (!isLayoutContainer) return;
    
    layoutDirty = true;
    
    // Use deferred layout for better performance (Phase 2 enhancement)
    // Schedule layout recalculation on next idle event instead of immediate
    if (window) {
        window->CallAfter([this]() {
            if (layoutDirty) { // Check if still dirty when the idle event fires
                performLayout();
            }
        });
    }
}

void AbstractWindow::rebuildLayoutFromChildren() {
    if (!layoutManager) {
        wxLogDebug("rebuildLayoutFromChildren: No layout manager");
        return;
    }
    
    wxLogDebug("rebuildLayoutFromChildren: Starting rebuild for %s with %d children", 
              getTagName(), getChildrenCount());
    
    // Clear existing layout
    layoutManager->clearEntities();
    
    int addedChildren = 0;
    
    // Iterate through all children and add those that are AbstractWindows
    for (int i = 0; i < getChildrenCount(); i++) {
        auto child = getChild(i);
        if (auto childWindow = dynamic_cast<AbstractWindow*>(child)) {
            // Ensure child has a layout entity
            childWindow->initLayoutEntity();
            
            // Check if child has explicit positioning (x, y attributes) - if so, skip layout
            bool hasXAttr = childWindow->hasSettedAttribute("x");
            bool hasYAttr = childWindow->hasSettedAttribute("y");
            bool hasExplicitPositioning = hasXAttr || hasYAttr;
            
            if (!hasExplicitPositioning) {
                // Get child's layout constraints from its "layout" attribute
                bool hasLayoutAttr = childWindow->hasSettedAttribute("layout");
                if (hasLayoutAttr) {
                    wxString layoutAttr = childWindow->getAttribute("layout");
                    if (!layoutAttr.IsEmpty()) {
                        childWindow->setLayoutConstraints(layoutAttr);
                    } else {
                        // Set default constraints for children without explicit layout
                        if (!childWindow->layoutConstraints) {
                            childWindow->layoutConstraints = new LayoutEngine::EntityConstraints();
                        }
                    }
                } else {
                    // Set default constraints for children without explicit layout
                    if (!childWindow->layoutConstraints) {
                        childWindow->layoutConstraints = new LayoutEngine::EntityConstraints();
                    }
                }
                
                // Add to layout manager - ensure both entity and constraints exist
                if (childWindow->layoutEntity && childWindow->layoutConstraints) {
                    layoutManager->addEntity(childWindow->layoutEntity, childWindow->layoutConstraints);
                    addedChildren++;
                    wxLogDebug("rebuildLayoutFromChildren: Added child %s to layout", 
                              childWindow->getTagName());
                } else {
                    wxLogError("Failed to add child %s to layout: missing entity or constraints", 
                              childWindow->getTagName());
                }
            } else {
                wxLogDebug("rebuildLayoutFromChildren: Skipping child %s (explicit positioning)", 
                          childWindow->getTagName());
            }
        }
    }
    
    wxLogDebug("rebuildLayoutFromChildren: Added %d children to layout manager", addedChildren);
    
    layoutDirty = true;
    performLayout();
}

bool AbstractWindow::parseLayoutContainer(const wxString& config) {
    if (!layoutManager) return false;
    
    try {
        std::string configStr = config.ToStdString();
        
        // Use basic layoutEngineStringParser to configure the layout manager (Phase 1)
        LayoutEngine::parseContainerConfiguration(layoutManager, configStr);
        
        return true;
    } catch (const std::exception& e) {
        // Log error but don't crash
        wxLogError("Layout container configuration error: %s", e.what());
        return false;
    }
}

void AbstractWindow::performLayout() {
    if (!layoutManager || !window) {
        wxLogDebug("performLayout: No layout manager or window available");
        return;
    }
    
    // Only perform layout if dirty flag is set
    if (!layoutDirty) {
        wxLogDebug("performLayout: Layout not dirty, skipping");
        return;
    }
    
    wxLogDebug("performLayout: Starting layout for %s", getTagName());
    
    try {
        // Get available space from wxWindow
        wxSize clientSize = window->GetClientSize();
        LayoutEngine::LayoutConstraints constraints(clientSize.x, clientSize.y);
        
        wxLogDebug("performLayout: Container size: %dx%d", clientSize.x, clientSize.y);
        wxLogDebug("performLayout: Entity count before layout: %zu", 
                  layoutManager ? layoutManager->getEntities().size() : 0);
        
        // Perform layout calculation
        layoutManager->performLayout(constraints);
        
        layoutDirty = false;
        wxLogDebug("performLayout: Layout completed successfully");
        // Layout manager will call update callbacks which will position children
        
    } catch (const std::exception& e) {
        wxLogError("Layout calculation error: %s", e.what());
        layoutDirty = false; // Clear dirty flag even on error to prevent infinite loops
    }
}

// Enhanced layout debugging and monitoring methods (Phase 2)
std::string AbstractWindow::getLayoutDebugInfo() {
    if (!layoutManager) return "No layout manager";
    
    std::ostringstream info;
    info << "=== Layout Debug Info for " << getTagName().ToStdString() << " ===" << "\n";
    info << "Layout Container: " << (isLayoutContainer ? "Yes" : "No") << "\n";
    info << "Layout Dirty: " << (layoutDirty ? "Yes" : "No") << "\n";
    info << "Child Count: " << getChildrenCount() << "\n";
    info << "Layout Entity: " << (layoutEntity ? "Present" : "Missing") << "\n";
    info << "Layout Manager: " << (layoutManager ? "Present" : "Missing") << "\n";
    info << "Layout Constraints: " << (layoutConstraints ? "Present" : "Missing") << "\n";
    
    if (window) {
        wxSize clientSize = window->GetClientSize();
        info << "Container Size: " << clientSize.x << "x" << clientSize.y << "\n";
    } else {
        info << "Container Size: No window available\n";
    }
    
    // Count actual AbstractWindow children
    int abstractWindowChildren = 0;
    for (int i = 0; i < getChildrenCount(); i++) {
        auto child = getChild(i);
        if (dynamic_cast<AbstractWindow*>(child)) {
            abstractWindowChildren++;
        }
    }
    info << "AbstractWindow Children: " << abstractWindowChildren << "\n";
    
    if (layoutManager) {
        try {
            info << "Layout Manager Debug Info:\n" << layoutManager->getLayoutDebugInfo(true, true);
        } catch (...) {
            info << "Error getting layout debug info\n";
        }
    }
    
    return info.str();
}

void AbstractWindow::validateLayoutConstraints() const {
    if (!layoutManager) return;
    
    try {
        layoutManager->validateConstraints();
    } catch (const std::exception& e) {
        wxLogError("Layout validation error: %s", e.what());
    }
}

// Advanced layout recalculation with cascade prevention
void AbstractWindow::performLayoutWithCascadePrevention() {
    static thread_local bool layoutInProgress = false;
    
    // Prevent recursive layout calls
    if (layoutInProgress) {
        wxLogWarning("Recursive layout call prevented");
        return;
    }
    
    layoutInProgress = true;
    try {
        performLayout();
    } catch (...) {
        layoutInProgress = false;
        throw;
    }
    layoutInProgress = false;
}

// Cache statistics and management (TODO: implement in Phase 2)
std::pair<size_t, size_t> AbstractWindow::getLayoutCacheStats() {
    // return std::make_pair(LayoutEngine::getConstraintCacheSize(), 0);
    return std::make_pair(0, 0); // Placeholder for Phase 1
}

void AbstractWindow::clearLayoutCache() {
    // LayoutEngine::clearConstraintCache();
    // TODO: implement in Phase 2
}

//--------- Control

Control::Control() {
    if(allowedAttributes==NULL) {
        allowedAttributes=createAndFillStringsMap({});
    }
    addAllowedAttributeNamesMap(allowedAttributes);
}

bool Control::handleChangedAttribute(const wxString&attributeName, TagAttribute&oldValue, TagAttribute&newValue) {
    return AbstractWindow::handleChangedAttribute(attributeName, oldValue, newValue);
}

bool Control::getDynamicAttributeValue(const wxString&attributeName, TagAttribute&tagAttribute){
    return AbstractWindow::getDynamicAttributeValue(attributeName, tagAttribute);
}

//---------------- Window

Window::Window() {
    if(allowedAttributes==NULL) {
        allowedAttributes=createAndFillStringsMap({"text", "transparent", "decoration","enableMinimizeButton","enableMaximizeButton","enableCloseButton","enableSystemMenu","resizeable","stayOnTop","stayOnTopOfParent","smallFrame","showInTaskBar"});
        recreationRequiredAttributes=createAndFillStringsMap({"decoration","enableMinimizeButton","enableMaximizeButton","enableCloseButton","enableSystemMenu","resizeable","stayOnTop","stayOnTopOfParent","smallFrame","showInTaskBar"});
    }
    addAllowedAttributeNamesMap(allowedAttributes);
    addRecreationAttributeNamesMap(recreationRequiredAttributes);
    setChildrenAllowed(true);
}

void Window::initElement(DomElement*parent,wxArrayString*attributesNames) {
    bool decoration=getComputedAttributeWithoutDynamic("decoration").defaultIfNull(true);
    bool enableMinimizeButton=getComputedAttributeWithoutDynamic("enableMinimizeButton").defaultIfNull(true);
    bool enableMaximizeButton=getComputedAttributeWithoutDynamic("enableMaximizeButton").defaultIfNull(true);
    bool enableCloseButton=getComputedAttributeWithoutDynamic("enableCloseButton").defaultIfNull(true);
    bool enableSystemMenu=getComputedAttributeWithoutDynamic("enableSystemMenu").defaultIfNull(true);
    bool resizeable=getComputedAttributeWithoutDynamic("resizeable").defaultIfNull(true);
    bool stayOnTop=getComputedAttributeWithoutDynamic("stayOnTop").defaultIfNull(false);
    bool stayOnTopOfParent=getComputedAttributeWithoutDynamic("stayOnTopOfParent").defaultIfNull(false);
    bool smallFrame=getComputedAttributeWithoutDynamic("smallFrame").defaultIfNull(false);
    bool showInTaskbar=getComputedAttributeWithoutDynamic("showInTaskBar").defaultIfNull(true);
    if(decoration==false){
        enableMinimizeButton=false;
        enableMaximizeButton=false;
        enableSystemMenu=false;
        enableCloseButton=false;
    }
    int computedStyle=getComputedWindowStyle()
        |selector(decoration, wxCAPTION, 0)
        |selector(enableMinimizeButton, wxMINIMIZE_BOX, 0)
        |selector(enableMaximizeButton, wxMAXIMIZE_BOX, 0)
        |selector(enableCloseButton, wxCLOSE_BOX, 0)
        |selector(enableSystemMenu, wxSYSTEM_MENU, 0)
        |selector(resizeable, wxRESIZE_BORDER, 0)
        |selector(stayOnTop, wxSTAY_ON_TOP, 0)
        |selector(stayOnTopOfParent, wxFRAME_FLOAT_ON_PARENT, 0)
        |selector(!showInTaskbar, wxFRAME_NO_TASKBAR, 0)
    ;
    wxFrame*frame;
    if(smallFrame) {
        frame=new wxMiniFrame((wxFrame*) NULL, -1, getComputedAttributeWithoutDynamic("text").defaultIfNull(wxString("")),wxDefaultPosition, wxDefaultSize, computedStyle);
    } else {
        frame=new wxFrame((wxFrame*) NULL, -1, getComputedAttributeWithoutDynamic("text").defaultIfNull(wxString("")),wxDefaultPosition, wxDefaultSize, computedStyle);
    }
    setWindow(frame);
}

bool Window::handleChangedAttribute(const wxString&attributeName, TagAttribute&oldValue, TagAttribute&newValue) {
    if(attributeName=="transparent") {
        double transparency=getAttribute(attributeName, 1.0);
        if(transparency>1 ||transparency<0)
            throw RuntimeException(wxString::Format("Transparency should be in interval [0..1], but found %lf", transparency));
        getWindow()->SetTransparent((int)(transparency*255));
        return true;
    }
    return AbstractWindow::handleChangedAttribute(attributeName, oldValue, newValue);
}

bool Window::getDynamicAttributeValue(const wxString&attributeName, TagAttribute&tagAttribute){
    return AbstractWindow::getDynamicAttributeValue(attributeName, tagAttribute);
}

void Window::onWillAddToParent(DomElement*parentElement) {
    if (parentElement->getTagName() == "Window") {
        getWindow()->Reparent(getParentWindow(parentElement));
    } else if (parentElement->getTagName() == "App") {
        //Do nothing
    } else {
        throw RuntimeException(wxString::Format("You can add Window tag only to App or to other Window tag"));
    }
}


//------------ Button
Button::Button() {
    if(allowedAttributes==NULL) {
        allowedAttributes=createAndFillStringsMap({"onClick", "text", "icon", "iconHover", "iconPressed","iconDisabled","type", "note", "toggled"});
        recreationRequiredAttributes=createAndFillStringsMap({"type"});
    }
    addAllowedAttributeNamesMap(allowedAttributes);
    addRecreationAttributeNamesMap(recreationRequiredAttributes);

    imageHolder.init([this](wxBitmap*loaded){
        ((wxButton*)getWindow())->SetBitmap(wxBitmapBundle::FromBitmap(*loaded));
    }, [this](){
        wxBitmapBundle emptyBundle;
        ((wxButton*)getWindow())->SetBitmap(emptyBundle);
    }, [this](wxString message){
        wxPrintf(wxString::Format("Error while load image in button '%s'", message));
    });
    disabledImageHolder.init([this](wxBitmap*loaded){
        ((wxButton*)getWindow())->SetBitmapDisabled(wxBitmapBundle::FromBitmap(*loaded));
    }, [this](){
        wxBitmapBundle emptyBundle;
        ((wxButton*)getWindow())->SetBitmapDisabled(emptyBundle);
    }, [this](wxString message){
        wxPrintf(wxString::Format("Error while load disabled image in button '%s'", message));
    });
    pressedImageHolder.init([this](wxBitmap*loaded){
        ((wxButton*)getWindow())->SetBitmapPressed(wxBitmapBundle::FromBitmap(*loaded));
    }, [this](){
        wxBitmapBundle emptyBundle;
        ((wxButton*)getWindow())->SetBitmapPressed(emptyBundle);
    }, [this](wxString message){
        wxPrintf(wxString::Format("Error while load pressed image in button '%s'", message));
    });
    hoverImageHolder.init([this](wxBitmap*loaded){
        ((wxButton*)getWindow())->SetBitmapHover(*loaded);
    }, [this](){
        wxBitmap bitmap;
        ((wxButton*)getWindow())->SetBitmapHover(bitmap);
    }, [this](wxString message){
        wxPrintf(wxString::Format("Error while load hower image in button '%s'", message));
    });
}

void Button::initElement(DomElement*parent,wxArrayString*attributesNames) {
    wxWindow*parentWindow = getParentWindow(parent);
    wxString typeStr = getComputedAttribute("type").defaultIfNull(wxString("default"));
    wxString text = getComputedAttribute("text").defaultIfNull(wxString("Button"));
    if(typeStr=="default" || typeStr=="simple") {
        wxButton*button=new wxButton(parentWindow, -1, text, wxDefaultPosition, wxDefaultSize, getComputedWindowStyle());
        setWindow(button);
    } else if(typeStr=="toggle") {
        wxToggleButton*button=new wxToggleButton(parentWindow, -1, text, wxDefaultPosition, wxDefaultSize, getComputedWindowStyle());
        setWindow(button);
    } else if(typeStr=="commandlink") {
        wxString note=getComputedAttribute("note").defaultIfNull(wxString(""));
        wxCommandLinkButton*button = new wxCommandLinkButton(parentWindow, -1, text, note, wxDefaultPosition, wxDefaultSize, getComputedWindowStyle());
        setWindow(button);
    } else {
        throw RuntimeException(wxString::Format("Unknown button type '%s'", typeStr));
    }
}

bool Button::handleChangedAttribute(const wxString&attributeName, TagAttribute&oldValue, TagAttribute&newValue) {
    wxString typeStr = getComputedAttributeWithoutDynamic("type").defaultIfNull(wxString("default"));
    if(attributeName=="onClick") {
        if(typeStr!="toggle"){
            getWindow()->Unbind(wxEVT_TOGGLEBUTTON, &Button::onClickEventHandler, this);
            registerEvent(wxEVT_BUTTON, attributeName, [this]() {
                getWindow()->Bind(wxEVT_BUTTON, &Button::onClickEventHandler, this);
            }, [this](){
                getWindow()->Unbind(wxEVT_BUTTON, &Button::onClickEventHandler, this);
            });
        }else{
            getWindow()->Unbind(wxEVT_BUTTON, &Button::onClickEventHandler, this);
            registerEvent(wxEVT_TOGGLEBUTTON, attributeName, [this]() {
                getWindow()->Bind(wxEVT_TOGGLEBUTTON, &Button::onClickEventHandler, this);
            }, [this](){
                getWindow()->Unbind(wxEVT_TOGGLEBUTTON, &Button::onClickEventHandler, this);
            });
        }
        return true;
    }
    if(attributeName=="text") {
        ((wxButton*)getWindow())->SetLabel(getComputedAttributeWithoutDynamic(attributeName).defaultIfNull(wxString("")));
        return true;
    }
    if(attributeName=="note") {
        if(typeStr=="commandlink") {
            ((wxCommandLinkButton*)getWindow())->SetNote(getComputedAttributeWithoutDynamic(attributeName).defaultIfNull(wxString("")));
        }
        return true;
    }
    if(attributeName=="icon") {
        wxString iconPath = getComputedAttributeWithoutDynamic(attributeName).defaultIfNull(wxString(""));
        imageHolder.load(iconPath);
        return true;
    }
    if(attributeName=="iconHover") {
        wxString iconPath = getComputedAttributeWithoutDynamic(attributeName).defaultIfNull(wxString(""));
        hoverImageHolder.load(iconPath);
        return true;
    }
    if(attributeName=="iconDisabled") {
        wxString iconPath = getComputedAttributeWithoutDynamic(attributeName).defaultIfNull(wxString(""));
        disabledImageHolder.load(iconPath);
        return true;
    }
    if(attributeName=="iconPressed") {
        wxString iconPath = getComputedAttributeWithoutDynamic(attributeName).defaultIfNull(wxString(""));
        pressedImageHolder.load(iconPath);
        return true;
    }
    if(attributeName=="toggled") {
        if(typeStr=="toggle") {
            ((wxToggleButton*)getWindow())->SetValue(getComputedAttributeWithoutDynamic(attributeName).defaultIfNull(false));
        }
        return true;
    }
    return Control::AbstractWindow::handleChangedAttribute(attributeName, oldValue, newValue);
}

bool Button::getDynamicAttributeValue(const wxString&attributeName, TagAttribute&tagAttribute) {
    wxString typeStr = getComputedAttributeWithoutDynamic("type").defaultIfNull(wxString("default"));
    if(attributeName=="toggled") {
        if(typeStr=="toggle") {
            tagAttribute.setBool(((wxToggleButton*)getWindow())->GetValue());
        } else {
            tagAttribute.setBool(false);
        }
        return true;
    }
    return Control::getDynamicAttributeValue(attributeName, tagAttribute);
}

void Button::onClickEventHandler(wxCommandEvent&e) {
    getEngine()->execFunctionFromAttributeBuilder(getComputedAttributeWithoutDynamic("onClick")).exec(0, [](bool status, ValuesListReader *results, wxString &errorMessage){});
}

//------------ CheckBox
CheckBox::CheckBox() {
    if(allowedAttributes == NULL) {
        allowedAttributes = createAndFillStringsMap({"type", "rightAlign", "onChange", "checked", "value", "text"});
        recreationRequiredAttributes = createAndFillStringsMap({"type", "rightAlign"});
    }

    addAllowedAttributeNamesMap(allowedAttributes);
    addRecreationAttributeNamesMap(recreationRequiredAttributes);
}

void CheckBox::initElement(DomElement*parent,wxArrayString*attributesNames) {
    wxWindow*parentWindow = getParentWindow(parent);
    wxString typeStr = getComputedAttributeWithoutDynamic("type").defaultIfNull(wxString("2state"));
    int componentStyle = selector(typeStr, {"2state", "2_5state", "3state"}, {wxCHK_2STATE, wxCHK_3STATE, wxCHK_ALLOW_3RD_STATE_FOR_USER|wxCHK_3STATE});
    if(componentStyle==-1) {
        throw RuntimeException(wxString::Format("Unknown type for CheckBox '%s' supported only [2state|2_5state|3state]", typeStr));
    }
    componentStyle |= getComputedAttributeWithoutDynamic("rightAlign").defaultIfNull(false)?wxALIGN_RIGHT : 0;
    wxString label = getComputedAttributeWithoutDynamic("label").defaultIfNull(wxString("CheckBox"));
    wxCheckBox*chkBox = new wxCheckBox(parentWindow, -1, label, wxDefaultPosition, wxDefaultSize, componentStyle|getComputedWindowStyle());
    setWindow(chkBox);
}

bool CheckBox::handleChangedAttribute(const wxString&attributeName, TagAttribute&oldValue, TagAttribute&newValue) {
    if(attributeName=="onChange") {
        registerEvent(wxEVT_CHECKBOX, attributeName, [this]() {
            getWindow()->Bind(wxEVT_CHECKBOX, &CheckBox::onChangeEventHandler, this);
        }, [this](){
            getWindow()->Unbind(wxEVT_CHECKBOX, &CheckBox::onChangeEventHandler, this);
        });
        return true;
    }
    if(attributeName=="text") {
        ((wxCheckBox*)getWindow())->SetLabel(getComputedAttributeWithoutDynamic(attributeName).defaultIfNull(wxString("")));
        return true;
    }
    if(attributeName=="checked") {
        ((wxCheckBox*)getWindow())->SetValue(getComputedAttributeWithoutDynamic(attributeName).defaultIfNull(true));
        return true;
    }
    if(attributeName=="value") {
        int value=getComputedAttributeWithoutDynamic(attributeName).defaultIfNull(0);
        wxCheckBoxState state;
        switch(value){
            case 0: state=wxCheckBoxState::wxCHK_UNCHECKED;break;
            case 1: state=wxCheckBoxState::wxCHK_CHECKED;break;
            case 2:
                if(getComputedAttributeWithoutDynamic("type").getString()=="2state") {
                    throw RuntimeException("Cannot set value 2 to field 'value' in checkbox with type '2state'");
                }
                state=wxCheckBoxState::wxCHK_UNDETERMINED;
                break;
            default:throw RuntimeException("Cannot set %d value to 'value' field of checkbox. Allowed only [0,1,2]");
        }
        ((wxCheckBox*)getWindow())->Set3StateValue(state);
        return true;
    }
    return Control::AbstractWindow::handleChangedAttribute(attributeName, oldValue, newValue);
}

bool CheckBox::getDynamicAttributeValue(const wxString&attributeName, TagAttribute&tagAttribute) {
    if(attributeName=="checked") {
        tagAttribute.setBool(((wxCheckBox*)getWindow())->IsChecked());
        return true;
    }
    if(attributeName=="value") {
        wxCheckBoxState state=((wxCheckBox*)getWindow())->Get3StateValue();
        switch (state) {
            case wxCheckBoxState::wxCHK_UNCHECKED:
                tagAttribute.setInt(0);
                break;
            case wxCheckBoxState::wxCHK_CHECKED:
                tagAttribute.setInt(1);
                break;
            case wxCheckBoxState::wxCHK_UNDETERMINED:
                tagAttribute.setInt(2);
                break;
        }
        return true;
    }
    return Control::getDynamicAttributeValue(attributeName, tagAttribute);
}

void CheckBox::onChangeEventHandler(wxCommandEvent&e) {
    getEngine()->execFunctionFromAttributeBuilder(getComputedAttributeWithoutDynamic("onChange")).exec(0, [](bool status, ValuesListReader *results, wxString &errorMessage){});
}





//------------ Label
Label::Label() {
    if(allowedAttributes==NULL) {
        allowedAttributes=createAndFillStringsMap({"text","htmlmarkup", "textalign","ellipsis", "autoresize"});
        recreationRequiredAttributes=createAndFillStringsMap({"textalign","ellipsis", "autoresize"});
    }
    addAllowedAttributeNamesMap(allowedAttributes);;
    addRecreationAttributeNamesMap(recreationRequiredAttributes);
}

void Label::initElement(DomElement*parent,wxArrayString*attributesNames) {
    wxString labelText = getComputedAttribute("text").getString();
    wxString textAlignString=getComputedAttribute("textalign").defaultIfNull(wxString("left"));
    wxString ellipsisString=getComputedAttribute("ellipsis").defaultIfNull(wxString("none"));
    bool autoresize=getComputedAttribute("autoresize").defaultIfNull(true);
    int textAlign=selector(textAlignString, {"left","right", "center"}, {wxALIGN_LEFT,wxALIGN_RIGHT, wxALIGN_CENTER_HORIZONTAL});
    int ellipsis=selector(ellipsisString, {"left","right", "center", "none"}, {wxST_ELLIPSIZE_START, wxST_ELLIPSIZE_END, wxST_ELLIPSIZE_MIDDLE, wxELLIPSIZE_FLAGS_NONE});
    if(textAlign==-1)
        throw RuntimeException(wxString::Format("Label 'textalign' attribute does not accept '%s' value", textAlignString));
    if(ellipsis==-1)
        throw RuntimeException(wxString::Format("Label 'ellipsis' attribute does not accept '%s' value", ellipsisString));
    int combinedStyle=getComputedWindowStyle()|textAlign|ellipsis|(autoresize?0:wxST_NO_AUTORESIZE);
    wxWindow*parentWindow = getParentWindow(parent);
    wxStaticText*staticText = new wxStaticText(parentWindow, -1, labelText, wxDefaultPosition, wxDefaultSize, combinedStyle);
    setWindow(staticText);
}

bool Label::handleChangedAttribute(const wxString&attributeName, TagAttribute&oldValue, TagAttribute&newValue){
    if(attributeName=="text") {
        wxString text=getComputedAttribute(attributeName).defaultIfNull(wxString(""));
        if(htmlMarkup) {
            ((wxStaticText*)getWindow())->SetLabelMarkup(text);
        } else {
            ((wxStaticText*)getWindow())->SetLabel(text);
        }
        
        wxBorder border=getWindow()->GetBorder();
        if(border!=wxBORDER_NONE&&border!=wxBORDER_DEFAULT) {
            //for some reason if text changed and the border is set, the border may not be updated(at least on Mac), updating component directly not helps, that is
            //why let's update whole parent
            if(getParent()!=NULL)getParent()->repaint();
        }
        
        return true;
    }
    if(attributeName=="htmlmarkup") {
        htmlMarkup=getComputedAttribute(attributeName).defaultIfNull(false);
        if(hasSettedAttribute("text")){
            handleChangedAttribute("text", TagAttribute().setNull(), TagAttribute().setNull());
        }
        return true;
    }
    return Control::handleChangedAttribute(attributeName,oldValue, newValue);
}

bool Label::getDynamicAttributeValue(const wxString&attributeName, TagAttribute&tagAttribute){
    return Control::getDynamicAttributeValue(attributeName, tagAttribute);
}


//----------------- TextInput
TextInput::TextInput() {
    if(allowedAttributes==NULL) {
        allowedAttributes=createAndFillStringsMap({"text"});
    }
    addAllowedAttributeNamesMap(allowedAttributes);
}

void TextInput::initElement(DomElement*parent,wxArrayString*attributesNames) {
    wxWindow*parentWindow=getParentWindow(parent);
    wxTextCtrl*text=new wxTextCtrl(parentWindow, -1, _T(""), wxDefaultPosition, wxDefaultSize, getComputedWindowStyle());
    setWindow(text);
}

bool TextInput::handleChangedAttribute(const wxString&name, TagAttribute&oldValue, TagAttribute&newValue) {
    if(name=="text") {
        wxString value=getAttribute(name);
        ((wxTextCtrl*)getWindow())->Clear();
        ((wxTextCtrl*)getWindow())->AppendText(value);
        return true;
    }
    return Control::AbstractWindow::handleChangedAttribute(name, oldValue, newValue);
}

bool TextInput::getDynamicAttributeValue(const wxString&attributeName, TagAttribute&tagAttribute) {
    return Control::getDynamicAttributeValue(attributeName, tagAttribute);
}


//----------------- DropDown
DropDown::DropDown() {
    if(allowedAttributes==NULL) {
        allowedAttributes=createAndFillStringsMap({"onChange", "selectedIndex"});
        recreationRequiredAttributes = createAndFillStringsMap({});
    }
    addAllowedAttributeNamesMap(allowedAttributes);
    addRecreationAttributeNamesMap(recreationRequiredAttributes);
    setChildrenAllowed(true);
}

void DropDown::initElement(DomElement*parent,wxArrayString*attributesNames) {
    auto windowStyle=getComputedWindowStyle();
    wxString choices[]={};
    wxChoice*choice=new wxChoice(getParentWindow(parent), -1, wxDefaultPosition, wxDefaultSize, 0, choices, windowStyle);
    setWindow(choice);
}

bool DropDown::handleChangedAttribute(const wxString&attributeName, TagAttribute&oldValue, TagAttribute&newValue) {
    if (attributeName=="onChange") {
        registerEvent(wxEVT_CHOICE, attributeName, [this]() {
            getWindow()->Bind(wxEVT_CHOICE, &DropDown::onChangeEventHandler, this);
        }, [this](){
            getWindow()->Unbind(wxEVT_CHOICE, &DropDown::onChangeEventHandler, this);
        });
        return true;
    }
    
    if (attributeName == "selectedIndex") {
        int newIndex = getComputedAttributeWithoutDynamic(attributeName).defaultIfNull(-1);
        ((wxChoice*)getWindow())->SetSelection(newIndex);
        return true;
    }

    if (attributeName == "selectedValue") {
        return true;
    }
    
    return Control::AbstractWindow::handleChangedAttribute(attributeName, oldValue, newValue);
}

bool DropDown::getDynamicAttributeValue(const wxString&attributeName, TagAttribute&tagAttribute) {
    if (attributeName == "selectedIndex") {
        tagAttribute.setInt(((wxChoice*)getWindow())->GetSelection());
        return true;
    }
    if (attributeName == "selectedValue") {
        int index=((wxChoice*)getWindow())->GetSelection();
        if(index==-1)return true;;
        tagAttribute=getChild(index)->getComputedAttribute("value");
        return true;
    }
    return Control::getDynamicAttributeValue(attributeName, tagAttribute);
}

void DropDown::onChildAdded(DomElement*child) {
    Control::onChildAdded(child);
    if(child->getTagName() != "Option")
        throw RuntimeException(wxString::Format("DropDown tag supports only Option child tags, but found '%s'", child->getTagName()));
    wxString newItem=child->getComputedAttribute("text").defaultIfNull(wxString("No value"));
    ((wxChoice*)getWindow())->Append(newItem);
}

void DropDown::onChildRemoving(DomElement*child) {
    Control::onChildRemoving(child);
    int index=getChildIndex(child);
    if(index==-1) return;
    int selectedIndex=((wxChoice*)getWindow())->GetSelection();
    ((wxChoice*)getWindow())->Delete(index);
    if(index==selectedIndex && ((wxChoice*)getWindow())->GetCount() > 0) {
        ((wxChoice*)getWindow())->SetSelection(0);
    }
}

void DropDown::onChildChanged(DomElement*child, wxString changeType) {
    Control::onChildChanged(child, changeType);
    int index=getChildIndex(child);
    if(index==-1) return;
    wxString newItemText=child->getComputedAttribute("text").defaultIfNull(wxString("No value"));
    ((wxChoice*)getWindow())->SetString(index, newItemText);
}

void DropDown::onChangeEventHandler(wxCommandEvent&e) {
    getEngine()->execFunctionFromAttributeBuilder(getComputedAttributeWithoutDynamic("onChange")).exec(0, [](bool status, ValuesListReader *results, wxString &errorMessage){});
}

//----------------- Option
Option::Option() {
    if (allowedAttributes == NULL) {
        allowedAttributes=createAndFillStringsMap({"text", "value"});
        recreationRequiredAttributes = createAndFillStringsMap({});
    }
    addAllowedAttributeNamesMap(allowedAttributes);
    addRecreationAttributeNamesMap(recreationRequiredAttributes);
}

void Option::initElement(DomElement*parent,wxArrayString*attributesNames) {
}

bool Option::handleChangedAttribute(const wxString&name, TagAttribute&oldValue, TagAttribute&newValue) {
    if(name=="text" || name=="value") {
        if(!isInitPhase()) {
            notifyParentAboutChange();
        }
        return true;
    }
    return DomElement::handleChangedAttribute(name, oldValue, newValue);
}

bool Option::getDynamicAttributeValue(const wxString&attributeName, TagAttribute&tagAttribute) {
    return DomElement::getDynamicAttributeValue(attributeName, tagAttribute);
}

//----------------- Progress

Progress::Progress() {
    if(allowedAttributes==NULL) {
        allowedAttributes=createAndFillStringsMap({"value", "max", "smooth", "vertical", "indeterminate"});
        recreationRequiredAttributes = createAndFillStringsMap({"smooth", "vertical"});
    }
    addAllowedAttributeNamesMap(allowedAttributes);
    addRecreationAttributeNamesMap(recreationRequiredAttributes);
    value=0;
    indeterminate=false;
}

void Progress::initElement(DomElement*parent,wxArrayString*attributesNames) {
    auto style = getComputedWindowStyle();
    style|=selector(getComputedAttribute("vertical").defaultIfNull(false), wxGA_VERTICAL, wxGA_HORIZONTAL);
    style|=selector(getComputedAttribute("smooth").defaultIfNull(false), wxGA_SMOOTH, 0);
    int max=getComputedAttributeWithoutDynamic("max").defaultIfNull(100);
    wxGauge*gauge=new wxGauge(getParentWindow(parent), -1, max, wxDefaultPosition, wxDefaultSize, style);
    removeFromStringArray(attributesNames, "vertical");
    removeFromStringArray(attributesNames, "smooth");
    removeFromStringArray(attributesNames, "max");
    removeFromStringArray(attributesNames, "indeterminate");
    removeFromStringArray(attributesNames, "value");
    indeterminate=getComputedAttribute("indeterminate").defaultIfNull(false);
    //if both attributes are present, just remember the value, but enable indeterminate
    if(indeterminate){
        value = getComputedAttributeWithoutDynamic("value").defaultIfNull(0);
        gauge->Pulse();
    }
    setWindow(gauge);
};

bool Progress::handleChangedAttribute(const wxString&name, TagAttribute&oldValue, TagAttribute&newValue){
    if(name=="value") {
        setAttribute("indeterminate", TagAttribute().setBool(false));
        value = getComputedAttributeWithoutDynamic(name).defaultIfNull(0);
        ((wxGauge*)getWindow())->SetValue(value);
        return true;
    }
    if(name=="max") {
        ((wxGauge*)getWindow())->SetRange(getComputedAttributeWithoutDynamic(name).defaultIfNull(100));
        return true;
    }
    if(name=="indeterminate") {
        indeterminate=getComputedAttributeWithoutDynamic(name).defaultIfNull(false);
        if (indeterminate) {
            ((wxGauge*)getWindow())->Pulse();
        } else {
            ((wxGauge*)getWindow())->SetValue(value);
        }
        return true;
    }
    return Control::handleChangedAttribute(name, oldValue, newValue);
}

bool Progress::getDynamicAttributeValue(const wxString&attributeName, TagAttribute&tagAttribute){
    if(attributeName=="value"){
        tagAttribute.setInt(((wxGauge*)getWindow())->GetValue());
        return true;
    }
    if(attributeName=="max"){
        tagAttribute.setInt(((wxGauge*)getWindow())->GetRange());
        return true;
    }
    return false;
}


//----------------- Hyperlink

Hyperlink::Hyperlink() {
    if(allowedAttributes==NULL) {
        allowedAttributes=createAndFillStringsMap({"href", "visited", "normalColor", "visitedColor", "hoverColor", "onLink", "align", "contextMenu"});
        recreationRequiredAttributes = createAndFillStringsMap({"align", "contextMenu"});
    }
    addAllowedAttributeNamesMap(allowedAttributes);
    addRecreationAttributeNamesMap(recreationRequiredAttributes);
}

void Hyperlink::initElement(DomElement*parent,wxArrayString*attributesNames) {
    auto style = getComputedWindowStyle();
    wxString alignString =getComputedAttribute("align").defaultIfNull(wxString("left"));
    int align=selector(alignString, {"left", "right", "center"}, {wxHL_ALIGN_LEFT, wxHL_ALIGN_RIGHT, wxHL_ALIGN_CENTRE});
    if(align==-1)
        throw RuntimeException(wxString::Format("Unknown align in hyperlink. Supported only [left,right,center] but found '%s'", alignString));
    style|=align;
    style|=(getComputedAttribute("contextMenu").defaultIfNull(false)==true)?wxHL_CONTEXTMENU:0;
    wxString text = getComputedAttribute("label").defaultIfNull(wxString("link"));
    wxString href = getComputedAttribute("href").defaultIfNull(wxString("http://example.com"));
    removeFromStringArray(attributesNames, "label");
    removeFromStringArray(attributesNames, "href");
    wxHyperlinkCtrl*link = new wxHyperlinkCtrl(getParentWindow(parent), -1, text, href, wxDefaultPosition, wxDefaultSize, style);
    setWindow(link);
};

bool Hyperlink::handleChangedAttribute(const wxString&attributeName, TagAttribute&oldValue, TagAttribute&newValue){
    if(attributeName=="href") {
        ((wxHyperlinkCtrl*)getWindow())->SetURL(getComputedAttribute(attributeName).defaultIfNull(wxString("")));
        return true;
    }
    if(attributeName=="visited") {
        bool visited=getComputedAttribute("visited").defaultIfNull(false);
        ((wxHyperlinkCtrl*)getWindow())->SetVisited(visited); // TODO: not updated until user will hover the mouse
        //visited color actually changed only after user will hover mouse over the component. Simulate mouse moving over the component
        wxMouseEvent event(wxEVT_MOTION);
        ((wxHyperlinkCtrl*)getWindow())->GetEventHandler()->ProcessEvent(event);
        wxMouseEvent event2(wxEVT_LEAVE_WINDOW);
        ((wxHyperlinkCtrl*)getWindow())->GetEventHandler()->ProcessEvent(event2);
        return true;
    }
    if(attributeName=="visitedColor") {
        ((wxHyperlinkCtrl*)getWindow())->SetVisitedColour(parseAttributeColor(getComputedAttribute("visitedColor").defaultIfNull(wxString("green")), attributeName));
        return true;
    }
    if(attributeName=="normalColor") {
        ((wxHyperlinkCtrl*)getWindow())->SetNormalColour(parseAttributeColor(getComputedAttribute("normalColor").defaultIfNull(wxString("blue")), attributeName));
        return true;
    }
    if(attributeName=="hoverColor") {
        ((wxHyperlinkCtrl*)getWindow())->SetHoverColour(parseAttributeColor(getComputedAttribute("hoverColor").defaultIfNull(wxString("red")), attributeName));
        return true;
    }
    if (attributeName=="onLink") {
        registerEvent(wxEVT_HYPERLINK, attributeName, [this]() {
            getWindow()->Bind(wxEVT_HYPERLINK, &Hyperlink::onHyperLinkEventHandler, this);
        }, [this](){
            getWindow()->Unbind(wxEVT_HYPERLINK, &Hyperlink::onHyperLinkEventHandler, this);
        });
        return true;
    }
    return Control::handleChangedAttribute(attributeName, oldValue, newValue);
}

bool Hyperlink::getDynamicAttributeValue(const wxString&attributeName, TagAttribute&tagAttribute) {
    return false;
}

void Hyperlink::onHyperLinkEventHandler(wxHyperlinkEvent&e){
    getEngine()->execFunctionFromAttributeBuilder(getComputedAttributeWithoutDynamic("onLink")).exec(0, [](bool status, ValuesListReader *results, wxString &errorMessage){});
}

//------------ GlobalHotkey
GlobalHotkey::GlobalHotkey() {
    if (allowedAttributes == NULL) {
        allowedAttributes=createAndFillStringsMap({"hotkey", "onHotkey", "state"});
        recreationRequiredAttributes = createAndFillStringsMap({"hotkey"});
    }
    addAllowedAttributeNamesMap(allowedAttributes);
    addRecreationAttributeNamesMap(recreationRequiredAttributes);
}

void GlobalHotkey::initElement(DomElement*parent, wxArrayString*attributesNames) {
    hotkeyId=(int)getEngine()->nextHandle();
    setAttribute("state", TagAttribute().setBool(false));
    wxString hotkeyString=getComputedAttribute("hotkey").defaultIfNull(wxString(""));
    if(hotkeyString == "")
        return;
    Hotkey hotkey=parseHotkeyString(hotkeyString);
    bool result = getGui()->getToolWindow()->RegisterHotKey(hotkeyId, hotkey.modifiers, hotkey.key);
    setAttribute("state", TagAttribute().setBool(result));
}

void GlobalHotkey::destroyElement() {
    getGui()->getToolWindow()->UnregisterHotKey(hotkeyId);
}

bool GlobalHotkey::handleChangedAttribute(const wxString&attributeName, TagAttribute&oldValue, TagAttribute&newValue){
    if (attributeName=="onHotkey") {
        registerEvent(wxEVT_HOTKEY, attributeName, [this]() {
            getGui()->getToolWindow()->Bind(wxEVT_HOTKEY, &GlobalHotkey::onHotkey, this, hotkeyId);
        }, [this](){
            getGui()->getToolWindow()->Unbind(wxEVT_HOTKEY, &GlobalHotkey::onHotkey, this, hotkeyId);
        });
        return true;
    }
    return DomElement::handleChangedAttribute(attributeName, oldValue, newValue);
}

void GlobalHotkey::onHotkey(wxKeyEvent&e){
    getEngine()->execFunctionFromAttributeBuilder(getComputedAttributeWithoutDynamic("onHotkey")).exec(0, [](bool status, ValuesListReader *results, wxString &errorMessage){});
}

//------------ Tree
Tree::Tree() {
    if(allowedAttributes==NULL) {
        allowedAttributes=createAndFillStringsMap({"rowLines", "multipleSelection"});
        recreationRequiredAttributes = createAndFillStringsMap({"rowLines", "multipleSelection"});
    }
    addAllowedAttributeNamesMap(allowedAttributes);
    addRecreationAttributeNamesMap(recreationRequiredAttributes);
    setChildrenAllowed(true);
    setInitChildrenBeforeTag(true);
}

void Tree::initElement(DomElement*parent,wxArrayString*attributesNames) {
    auto style = wxTR_DEFAULT_STYLE |wxTR_HAS_BUTTONS| wxTR_HIDE_ROOT;
    bool rowLines = getComputedAttribute("rowLines").defaultIfNull(false);
    if(rowLines) {
        style|=wxTR_ROW_LINES;
    }
    bool multipleSelection = getComputedAttribute("multipleSelection").defaultIfNull(false);
    if(multipleSelection) {
        style|=wxTR_MULTIPLE;
    }
    wxTreeCtrl*tree = new wxTreeCtrl(getParentWindow(parent), -1, wxDefaultPosition, wxDefaultSize, style);
    setWindow(tree);
    
    rootItemId = tree->AppendItem(tree->GetRootItem(), "root");
    int childrenCount = getChildrenCount();
    for (int i=0; i<childrenCount; i++) {
        DomElement*childDomElement=getChild(i);
        if (childDomElement->getTagName() != "TreeNode") {
            throw RuntimeException(wxString::Format("Tree can contain only TreeNode items, but found %s", childDomElement->getTagName()));
        }
        TreeNode*treeNodeDomElement=dynamic_cast<TreeNode*>(childDomElement);
        onNodeAdded(rootItemId, treeNodeDomElement);
    }
}

bool Tree::handleChangedAttribute(const wxString&attributeName, TagAttribute&oldValue, TagAttribute&newValue) {
    return Control::handleChangedAttribute(attributeName, oldValue, newValue);
}

void Tree::onChildAdded(DomElement*child) {
    if (isInitPhase()) return; //ignore in init stage, because items added in initElement method
    if (child->getTagName() != "TreeNode") {
        throw RuntimeException(wxString::Format("Tree can contain only TreeNode items, but found %s", child->getTagName()));
    }
    TreeNode*treeNodeDomElement=dynamic_cast<TreeNode*>(child);
    onNodeAdded(rootItemId, treeNodeDomElement);
}

void Tree::onChildRemoving(DomElement*child) {
    if(isInitPhase()) return;
    TreeNode*treeNodeDomElement = dynamic_cast<TreeNode*>(child);
    onNodeRemoved(treeNodeDomElement);
}

void Tree::onChildChanged(DomElement*child, wxString changeType) {
    if (isInitPhase()) return; //ignore in init stage, because items added in initElement method
    TreeNode*treeNodeDomElement = dynamic_cast<TreeNode*>(child);
    wxTreeCtrl*tree=((wxTreeCtrl*)getWindow());
    if (changeType=="text") {
        tree->SetItemText(treeNodeDomElement->getItemId(), treeNodeDomElement->getComputedAttribute("text").defaultIfNull(wxString("")));
    } else if (changeType=="bold") {
        tree->SetItemBold(treeNodeDomElement->getItemId(),treeNodeDomElement->getComputedAttribute("bold").defaultIfNull(false));
    }
}

void Tree::onNodeAdded(wxTreeItemId parentNodeId, TreeNode*node) {
    node->setOwner(this);
    wxTreeCtrl*tree=(wxTreeCtrl*)getWindow();
    wxString text = node->getComputedAttribute("text").defaultIfNull(wxString(""));
    bool bold=node->getComputedAttribute("bold").defaultIfNull(false);
    node->setItemId(tree->AppendItem(parentNodeId, text));
    if(bold) {
        tree->SetItemBold(node->getItemId(), true);
    }
    if(node->hasSettedAttribute("fgcolor")) {
        wxColour color=parseAttributeColor(node->getComputedAttribute("fgcolor").defaultIfNull(wxString("black")), "fgcolor");
        tree->SetItemTextColour(node->getItemId(), color);
    }
    if(node->hasSettedAttribute("bgcolor")) {
        wxColour color=parseAttributeColor(node->getComputedAttribute("bgcolor").defaultIfNull(wxString("white")), "bgcolor");
        tree->SetItemBackgroundColour(node->getItemId(), color);
    }
    
    
    int childrenCount = node->getChildrenCount();
    for (int i=0; i<childrenCount; i++) {
        DomElement*childDomElement=node->getChild(i);
        if (childDomElement->getTagName() != "TreeNode") {
            throw RuntimeException(wxString::Format("Tree can contain only TreeNode items, but found %s", childDomElement->getTagName()));
        }
        TreeNode*treeNodeDomElement=dynamic_cast<TreeNode*>(childDomElement);
        onNodeAdded(node->getItemId(), treeNodeDomElement);
    }
}

void Tree::onNodeRemoved(TreeNode*node) {
    if (!node) return;
    
    wxTreeCtrl*tree = (wxTreeCtrl*)getWindow();
    if (tree && node->getItemId().IsOk()) {
        tree->Delete(node->getItemId());
    }
    node->setOwner(nullptr);
}

//==============================================================================
// TreeNode Implementation
//==============================================================================

TreeNode::TreeNode() {
    if (allowedAttributes == NULL) {
        allowedAttributes=createAndFillStringsMap({"text", "bold", "fgcolor", "bgcolor"});
        recreationRequiredAttributes = createAndFillStringsMap({});
    }
    addAllowedAttributeNamesMap(allowedAttributes);
    addRecreationAttributeNamesMap(recreationRequiredAttributes);
    setChildrenAllowed(true);
}

void TreeNode::initElement(DomElement*parent, wxArrayString*attributesNames) {
    LxwDomElement::initElement(parent, attributesNames);
    
    Tree* treeOwner = dynamic_cast<Tree*>(parent);
    if (treeOwner) {
        setOwner(treeOwner);
    }
}

bool TreeNode::handleChangedAttribute(const wxString&name, TagAttribute&oldValue, TagAttribute&newValue) {
    if (owner) {
        wxTreeCtrl* tree = (wxTreeCtrl*)owner->getWindow();
        if (tree && itemId.IsOk()) {
            if (name == "text") {
                tree->SetItemText(itemId, newValue.defaultIfNull(wxString("")));
                return true;
            } else if (name == "bold") {
                tree->SetItemBold(itemId, newValue.defaultIfNull(false));
                return true;
            } else if (name == "fgcolor") {
                wxColour color = parseAttributeColor(newValue.defaultIfNull(wxString("black")), "fgcolor");
                tree->SetItemTextColour(itemId, color);
                return true;
            } else if (name == "bgcolor") {
                wxColour color = parseAttributeColor(newValue.defaultIfNull(wxString("white")), "bgcolor");
                tree->SetItemBackgroundColour(itemId, color);
                return true;
            }
        }
    }
    return LxwDomElement::handleChangedAttribute(name, oldValue, newValue);
}

void TreeNode::notifyOwnerAboutChange(wxString changeType) {
    if (owner) {
        owner->onChildChanged(this, changeType);
    }
}

void TreeNode::onChildAdded(DomElement*child) {
    LxwDomElement::onChildAdded(child);
    
    if (child->getTagName() == "TreeNode") {
        TreeNode* childNode = dynamic_cast<TreeNode*>(child);
        if (childNode && owner && itemId.IsOk()) {
            owner->onNodeAdded(itemId, childNode);
        }
    }
}

void TreeNode::onChildRemoving(DomElement*child) {
    if (child->getTagName() == "TreeNode") {
        TreeNode* childNode = dynamic_cast<TreeNode*>(child);
        if (childNode && owner) {
            owner->onNodeRemoved(childNode);
        }
    }
    
    LxwDomElement::onChildRemoving(child);
}
