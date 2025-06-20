//  LxeEngine.hpp
//  LuaXmlWidgets
//
#ifndef WxControls_hpp
#define WxControls_hpp
#include "lxe.hpp"

namespace lxe {
class Engine;
class DomElement {
private:
    static String2BoolHashMap*allowedAttributes;
    static String2BoolHashMap*recreationRequiredAttributes;
    TableRef luaRef;
    wxString id;
    wxString tagName;
    Engine*engine;
    bool initPhase;
    DomElement*parent;
    AttributesStorage attributes;
    std::vector<DomElement*>children;

    wxString textContent;
    std::unordered_map<int, bool> registeredEvents;
    bool childrenAllowed = false;
    bool initChildrenBeforeTag = false;
protected:
    void addAllowedAttributeNamesMap(String2BoolHashMap*map);
    void addRecreationAttributeNamesMap(String2BoolHashMap*map);
    bool isAttributeRequireRecreation(const wxString&name);
    void registerEvent(int event, const wxString&fieldName, std::function<void()>createListener, std::function<void()>deleteListener);
    void setChildrenAllowed(bool value){childrenAllowed=value;}
    void setInitChildrenBeforeTag(bool value){initChildrenBeforeTag=value;}
    
public:
    DomElement();
    virtual ~DomElement(){}
    virtual void repaint(){}
    void initFromTag(Tag*tag);
    void setLuaRef(TableRef luaRef){this->luaRef=luaRef;}
    TableRef&getLuaRef(){return this->luaRef;}
    void clearLuaRef(){luaRef.ref=0;}
    int getLuaRefHandle(){return this->luaRef.ref;}
    void setEngine(Engine*engine) {this->engine=engine;}
    Engine*getEngine() {return engine;}
    wxString& getTagName() {return tagName;}
    void setTextContent(wxString&text) {this->textContent=text;}
    wxString&getTextContent() {return textContent;};
    void setParent(DomElement*parent) {this->parent=parent;}
    DomElement* getParent() {return parent;}
    DomElement*topParent();
    bool isChildrenAllowed(){return childrenAllowed;}
    bool isInitChildrenBeforeTag(){return initChildrenBeforeTag;}
    void setInitPhase(bool value) {initPhase=value;}
    bool isInitPhase() {return initPhase;}
    virtual void initElement(DomElement*parent,wxArrayString*attributesNames) {}
    virtual void destroyElement() {}
    virtual void recreate();
    void applyAttributes(wxArrayString*attributesNames);
    DomElement* getChild(int index){return children[index];}
    int getChildrenCount()const{return (int)children.size();}
    virtual void onWillAddToParent(DomElement*parentElement) {}
    virtual void onAddedToParent() {}
    virtual void onRemovingFromParent() {}
    virtual void onChildAdded(DomElement*child) {}
    virtual void onChildRemoving(DomElement*child) {};
    ///Did not sent automatically, sent by child if necessary via notifyParentAboutChange()
    virtual void onChildChanged(DomElement*child, wxString changeType) {}
    virtual void onFinishedInitialisation() {}
    virtual void notifyParentAboutChange();
    int getChildIndex(DomElement*child);
    virtual void addChild(DomElement*child);
    virtual void removeChildByIndex(int index);
    virtual bool handleChangedAttribute(const wxString&name, TagAttribute&oldValue, TagAttribute&newValue);
    wxArrayString getAllSettedAtributeNames();
    bool hasSettedAttribute(const wxString&attributeName);
    
    void setAttribute(const wxString&attributeName, TagAttribute&value);
    virtual bool getDynamicAttributeValue(const wxString&attributeName, TagAttribute&tagAttribute);
    TagAttributeType getAttributeType(const wxString&attributeName);
    wxString getAttribute(const wxString&attributeName, const wxString&defaultValue);
    wxString getAttribute(const wxString&attributeName){return getAttribute(attributeName, wxString(""));}
    int getAttribute(const wxString&attributeName, int defaultValue);
    double getAttribute(const wxString&attributeName, double defaultValue);
    bool getAttribute(const wxString&attributeName, bool defaultValue);
    TagAttribute getComputedAttributeWithoutDynamic(const wxString&attributeName);
    TagAttribute getComputedAttribute(const wxString&attributeName);
};

class Engine {
private:
    Lua*lua;
    SerializedFolderReader serializedFolderReader;
    DomElement*rootElement;
    std::unordered_map<wxString, std::function<DomElement*()>> tagName2DomElementFactory;
    std::unordered_map<wxString, DomElement*>idToDomElementMap;
    std::vector<std::function<void(wxString, DomElement*)>>elementIdChangedEventHandlers;
    long long handleGenerator=0;
public:
    Engine() { init(); }
    virtual void init();
    void initLua();
    void registerNativeFunctions();
    Lua*getLua(){return lua;}
    void registerTagFactory(wxString tagName, std::function<DomElement*()>tagFactory);
    long long nextHandle() { return ++handleGenerator; }
    void run(wxString source, wxString fileName);
    DomElement*recursivelyInitElement(Tag*tag, DomElement*parent);
    void removeDomElement(DomElement*domElement);
    void unregisterDomElementById(wxString&id);
    void registerDomElementById(wxString&id, DomElement*domElement);
    void replaceChildrenFromString(DomElement*domElement, wxString&innerHtml);
    DomElement*getDomElementById(wxString&id);
    void addElementIdChangedEventHandler(std::function<void(wxString, DomElement*element)>handler);
    void removeElementIdChangedEventHandler(std::function<void(wxString, DomElement*element)>handler);
    void fireElementIdChangedEvent(wxString&id, DomElement*domElement);
    ExecBuilder execFunctionFromAttributeBuilder(const TagAttribute&tagAttribute);
};

class Script: public virtual DomElement {
    static String2BoolHashMap*allowedAttributes;
    static String2BoolHashMap*recreationRequiredAttributes;
public:
    Script();
    virtual void onFinishedInitialisation()override;
    virtual bool handleChangedAttribute(const wxString&name, TagAttribute&oldValue, TagAttribute&newValue) override;
    virtual bool getDynamicAttributeValue(const wxString&attributeName, TagAttribute&tagAttribute) override;
};
}
#endif /* WxControls_hpp */
