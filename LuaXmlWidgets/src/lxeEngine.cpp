//
//  lxeEngine.cpp
//

#include "lxe.hpp"
#include "../../lua_std_lib/generated/lua_std_lib.hpp"

using namespace lxe;

String2BoolHashMap*DomElement::allowedAttributes=NULL;
String2BoolHashMap*DomElement::recreationRequiredAttributes=NULL;
String2BoolHashMap*Script::allowedAttributes=NULL;
String2BoolHashMap*Script::recreationRequiredAttributes=NULL;

DomElement::DomElement() {
    this->parent=NULL;
    if(allowedAttributes==NULL) {
        allowedAttributes=createAndFillStringsMap({"id", "properties", "innerLXML", "outerLXML"});
        recreationRequiredAttributes=createAndFillStringsMap({});
    }
    addAllowedAttributeNamesMap(allowedAttributes);
    
    attributes.setOnChangeEventHandler([this](const wxString&name, TagAttribute&oldValue, TagAttribute&newValue){
        handleChangedAttribute(name, oldValue, newValue);
    });
}

void DomElement::registerEvent(int event, const wxString&attributeName, std::function<void()>createListener, std::function<void()>deleteListener) {
    deleteListener();
    if(getComputedAttribute(attributeName).isNull())
        return;
    
    TagAttribute attributeValue=getComputedAttribute(attributeName);
    if (attributeValue.getType()==TA_STRING || attributeValue.getType()==TA_FUNCTION) {
        createListener();
    } else {
        throw RuntimeException(wxString::Format("Expected function name or function reference in attribute '%s' of tag %s", attributeName, tagName));
    }
}

DomElement*DomElement::topParent() {
    DomElement*parent=getParent();
    DomElement*lastNonNullParent=parent;
    while(true) {
        parent=parent->getParent();
        if (parent != NULL) {
            lastNonNullParent=parent;
            continue;
        } else {
            break;
        }
    }
    return lastNonNullParent;
}

void DomElement::initFromTag(Tag*tag) {
    this->tagName = tag->getTagName();
    AttributesMap&tagAttributes=tag->getAttributes();
    for(auto it = tagAttributes.begin(); it != tagAttributes.end(); ++it) {
        wxString key=it->first;
        TagAttribute&attrValue=it->second;
        attributes.setAttribute(key, attrValue, false);
    }
}

void DomElement::recreate() {
    setInitPhase(true);
    destroyElement();
    wxArrayString attributeNames = attributes.getAllSettedAtributeNames();
    initElement(getParent(),&attributeNames);
    applyAttributes(&attributeNames);
    setInitPhase(false);
    onFinishedInitialisation();
    DomElement*parent = getParent();
    if(parent!=NULL)parent->repaint();
}

void DomElement::applyAttributes(wxArrayString*attributeNames) {
    TagAttribute nullAttribute=TagAttribute().setNull();
    for(int i=0;i<attributeNames->size();i++){
        wxString&name=(*attributeNames)[i];
        if(isAttributeRequireRecreation(name))
            continue;
        TagAttribute attributeValue=attributes.getAttribute(name);
        attributes.getOnChangeEventHandler()(name, attributeValue, nullAttribute);
    }
}

void DomElement::addAllowedAttributeNamesMap(String2BoolHashMap*map) {
    attributes.addAllowedAttributeNamesMap(map);
}

void DomElement::addRecreationAttributeNamesMap(String2BoolHashMap*map) {
    attributes.addRecreationAttributeNamesMap(map);
}

bool DomElement::isAttributeRequireRecreation(const wxString&name) {
    return attributes.isAttributeRequireRecreation(name);
}

void DomElement::notifyParentAboutChange() {
    if(parent != NULL) parent->onChildChanged(this, wxString(""));
}

int DomElement::getChildIndex(DomElement*child) {
    unsigned long size = children.size();
    for (int i=0; i<size; i++) {
        if (children[i]->getLuaRefHandle() == child->getLuaRefHandle()) return i;
    }
    return -1;
}

void DomElement::addChild(DomElement*child) {
    children.push_back(child);
}

void DomElement::removeChildByIndex(int index) {
    children.erase(children.begin() + index);
}

bool DomElement::handleChangedAttribute(const wxString&name, TagAttribute&oldValue, TagAttribute&newValue) {
    if (name=="id") {
        wxString id = getAttribute(name);
        engine->unregisterDomElementById(id);
        engine->registerDomElementById(id, this);
        return true;
    }
    if (name=="innerLXML") {
        wxString innerLXML=newValue.defaultIfNull(wxString(""));
        engine->replaceChildrenFromString(this, innerLXML);
        return true;
    }
    return false;
}

bool DomElement::getDynamicAttributeValue(const wxString&name, TagAttribute&tagAttribute) {
    if (name=="innerLXML") {
        wxString result;
        for(int i=0;i<getChildrenCount();i++) {
            TagAttribute value;
            getChild(i)->getDynamicAttributeValue("outerLXML", value);
            result+=value.defaultIfNull(wxString(""));
        }
        tagAttribute.setString(result);
        return true;
    }
    if (name=="outerLXML") {
        wxString result="<"+getTagName();
        bool hasAttributes=getAllSettedAtributeNames().size()!=0;
        wxArrayString settedAttributesNames=getAllSettedAtributeNames();
        bool hasChild=getChildrenCount()>0;
        if(hasAttributes) {
            result+=" ";
            for(int i=0;i<settedAttributesNames.size();i++) {
                if(i!=0) result+=" ";
                result+=settedAttributesNames[i]+"=";
                TagAttributeType attributeType=getAttributeType(settedAttributesNames[i]);
                bool needQuoting=attributeType==TA_STRING || attributeType==TA_FUNCTION;
                if(needQuoting) result+="\"";
                wxString attributeValue=getAttribute(settedAttributesNames[i]);
                if(attributeValue.Contains("\n")) {
                    attributeValue.Replace("\n", "\\n");
                }
                if(attributeValue.Contains("\"")) {
                    attributeValue.Replace("\"", "\\\"");
                }
                result+=attributeValue;
                if(needQuoting) result+="\"";
            }
        }

        if(!hasChild) {
            result+="/>";
            tagAttribute.setString(result);
            return true;
        }
        result+=">";

        for(int i=0;i<getChildrenCount();i++) {
            TagAttribute value;
            getChild(i)->getDynamicAttributeValue("outer", value);
            result += value.defaultIfNull(wxString(""));
        }

        result += "</"+getTagName()+">";
        tagAttribute.setString(result);
        return true;
    }
    return false;
}

bool DomElement::hasSettedAttribute(const wxString&attributeName) {
    return attributes.isDirectAttributeSet(attributeName);
}

wxArrayString DomElement::getAllSettedAtributeNames() {
    return attributes.getAllSettedAtributeNames();
}

void DomElement::setAttribute(const wxString&attributeName, TagAttribute&value) {
    if(!attributes.isAttributeNameAllowed(attributeName)) {
        throw RuntimeException(wxString::Format("Tag %s does not support attribute %s", getTagName(), attributeName));
    }
    bool fireEvent = true;
    bool requireRecreation = isAttributeRequireRecreation(attributeName);
    if(requireRecreation) {
        fireEvent = false;
    }
    attributes.setAttribute(attributeName, value, fireEvent);
    if(requireRecreation){
        recreate();
    }
}

TagAttributeType DomElement::getAttributeType(const wxString&attributeName){
    TagAttribute attribute = attributes.getAttribute(attributeName);
    return attribute.getType();
}

int DomElement::getAttribute(const wxString&attributeName, int defaultValue) {
    if(hasSettedAttribute(attributeName)) {
        TagAttribute attribute=attributes.getAttribute(attributeName);
        switch(attribute.getType()) {
            case TA_INT:return attribute.getValue().As<int>();
            case TA_DOUBLE:return (int)attribute.getValue().As<double>();
            case TA_BOOL:return (int)attribute.getValue().As<bool>();
            case TA_FUNCTION:return attribute.getValue().As<int>();
            case TA_STRING: {
                double value;
                if(attribute.getValue().As<wxString>().ToDouble(&value)) {
                    return value;
                } else {
                    return defaultValue;
                }
            };
            case TA_NULL:
                throw RuntimeException(wxString::Format("Cannot get value of attribute %s as number. It is null", attributeName));
            default:
                throw RuntimeException(wxString::Format("Cannot get value of attribute %s as number. Attribute type %d does not implemented", attributeName, attribute.getType()));
        }
    }
    return defaultValue;
}

bool DomElement::getAttribute(const wxString&attributeName, bool defaultValue) {
    if(hasSettedAttribute(attributeName)) {
        TagAttribute attribute = attributes.getAttribute(attributeName);
        switch(attribute.getType()) {
            case TA_INT:return attribute.getValue().As<int>()!=0;
            case TA_DOUBLE:return attribute.getValue().As<double>()!=0;
            case TA_BOOL:return attribute.getValue().As<bool>();
            case TA_FUNCTION:return attribute.getValue().As<int>()!=0;
            case TA_STRING:{
                const wxString&value=attribute.getValue().As<wxString>();
                return value=="true"||value=="1";
            };
            case TA_NULL:
                return false;
            default:
                throw RuntimeException(wxString::Format("Cannot get value of attribute %s. Attribute type %d does not implemented", attributeName, attribute.getType()));
        }
    }
    return defaultValue;
}

wxString DomElement::getAttribute(const wxString&attributeName, const wxString&defaultValue) {
    if(hasSettedAttribute(attributeName)) {
        TagAttribute attribute = attributes.getAttribute(attributeName);
        switch(attribute.getType()){
            case TA_INT:return wxString::Format("%d",attribute.getValue().As<int>());
            case TA_DOUBLE:return wxString::Format("%lf",attribute.getValue().As<double>());
            case TA_BOOL:return wxString(attribute.getValue().As<bool>()?"true":"false");
            case TA_STRING:return attribute.getValue().As<wxString>();
            case TA_FUNCTION:return wxString("Function#%d", attribute.getValue().As<int>());
            case TA_NULL:
                return "null";
            default:
                throw RuntimeException(wxString::Format("Cannot get value of attribute %s. Attribute type %d does not implemented", attributeName, attribute.getType()));
        }
    }
    return defaultValue;
}

double DomElement::getAttribute(const wxString&attributeName, double defaultValue) {
    if(hasSettedAttribute(attributeName)) {
        TagAttribute attribute=attributes.getAttribute(attributeName);
        switch(attribute.getType()) {
            case TA_INT:return attribute.getValue().As<int>();
            case TA_DOUBLE:return attribute.getValue().As<double>();
            case TA_BOOL:return attribute.getValue().As<bool>()?1.0:0.0;
            case TA_FUNCTION:return attribute.getValue().As<int>();
            case TA_STRING: {
                double value;
                if(attribute.getValue().As<wxString>().ToDouble(&value)) {
                    return value;
                } else {
                    return defaultValue;
                }
            };
            case TA_NULL:
                return 0;
            default:
                throw RuntimeException(wxString::Format("Cannot get value of attribute %s. Attribute type %d does not implemented", attributeName, attribute.getType()));
        }
    }
    return defaultValue;
}

TagAttribute DomElement::getComputedAttributeWithoutDynamic(const wxString&attributeName) {
    if (hasSettedAttribute(attributeName)) {
        return attributes.getAttribute(attributeName);
    }
    
    TagAttribute value;
    value.setNull();
    return value;
}

TagAttribute DomElement::getComputedAttribute(const wxString&attributeName) {
    TagAttribute value;
    if(getDynamicAttributeValue(attributeName, value)) {
        return value;
    }
    
    if (hasSettedAttribute(attributeName)) {
        return attributes.getAttribute(attributeName);
    }

    value.setNull();
    return value;
}

void Engine::init() {
    initLua();
    registerNativeFunctions();
    lua->evalExpression("require \"resource://lxe/lxe.lua\"", [](bool state, wxString&result) {
        if(!state)
            throw RuntimeException("Error while load lxe.lua module. " + result);
    });
    registerTagFactory("Script", [](){return new Script();});
}

void Engine::initLua() {
    lua = new Lua(true);

    serializedFolderReader.load(LUA_STD_LIB, [](){});
    lua->registerLuaModuleReader([this](char*filePath) {
        wxString path = filePath;
        if(!path.StartsWith("resource://")) {
            return (char*)NULL;
        }
        path.Remove(0, wxString("resource://").size());
        path = wxString("/") + path;

        SerializedFileChunk*chunk = serializedFolderReader.findChunk(path.ToUTF8().data());
        if(chunk == NULL)
            return (char*)NULL;
        char*content = new char[chunk->dataLength + 1];
        memchr(content, 0, chunk->dataLength + 1);
        memcpy(content, chunk->data, chunk->dataLength);
        return (char*)content;
    });
}

void Engine::registerTagFactory(wxString tagName, std::function<DomElement*()>tagFactory) {
    tagName2DomElementFactory[tagName] =  tagFactory;
}

TagsParser createParser(wxString&source, wxString fileName) {
    TagsParser parser(source, fileName);
    parser.addTagWithRawTextContent("Script");
    return parser;
}

void Engine::run(wxString source, wxString fileName) {
    try {
        TagsParser parser=createParser(source, fileName);
        std::vector<Tag*>tags = parser.parseTags();
        if(tags.size()==0) throw ParseException(wxString::Format("Source file %s does not have any tag", fileName), 0);
        if(tags.size()>1)  throw ParseException(wxString::Format("Expected only one root tag in file %s", fileName), 0);
        Tag*rootTag=tags[0];
        if (rootTag->getType() != TagType_TAG) throw ParseException(wxString::Format("Root xml entity should be tag, but found ", rootTag->getType()), 0);
        if (!rootTag->getTagName() == wxString("Application")) throw RuntimeException(wxString::Format("Root tag must be 'Application'. File '%s'", fileName));
        rootElement = recursivelyInitElement(rootTag, NULL);
        delete rootTag;
    } catch(ParseException&ex) {
        wxPrintf(wxString::Format("Parsing error line:%d message: %s\n", ex.getLine(), ex.getErrorMessage()));
    } catch(RuntimeException&ex) {
        wxPrintf(wxString::Format("Runtime error message: %s\n", ex.getErrorMessage()));
    }
}

TableRef createLuaDomElementObject(Engine*engine, DomElement*domElement){
    TableRef newObject=engine->getLua()->createNewLuaTable();
    engine->getLua()->editGlobalTable([newObject, &engine](TableReaderWriter*tbl){
        tbl->getTable("lxe", [newObject, &engine](TableReaderWriter*nestedTable){
            TableRef prototypeObject=nestedTable->getTableRef("DomElementPrototype");
            engine->getLua()->inheritTable(newObject, prototypeObject);
        });
    });
    
    engine->getLua()->editTable(newObject, [domElement](TableReaderWriter*tbl){
        tbl->put("_nativeHandler", domElement);
    });
    return newObject;
}

DomElement* Engine::recursivelyInitElement(Tag*tag, DomElement*parent) {
    if(tagName2DomElementFactory.find(tag->getTagName()) == tagName2DomElementFactory.end())
        throw RuntimeException(wxString::Format("Unknown tag name:%s", tag->getTagName()));
    
    std::function<DomElement*()> domElementSupplier = tagName2DomElementFactory[tag->getTagName()];
    DomElement*element = domElementSupplier();
    element->setEngine(this);
    
    TableRef luaObject=createLuaDomElementObject(this, element);
    element->setLuaRef(luaObject);
    
    element->setInitPhase(true);
    element->initFromTag(tag);
    
    int childrenCount = (int)tag->getChildren().size();
    if(childrenCount>0 && !element->isChildrenAllowed()) {
        throw RuntimeException(wxString::Format("Tag '%s' does not accept child tags", element->getTagName()));
    }
    if(element->isInitChildrenBeforeTag()) {
        for (int i = 0; i < childrenCount; i++) {
            Tag* childTag = tag->getChildren()[i];
            if(childTag->getType() == TagType_RAW_TEXT){
                wxString textContent=element->getTextContent()+childTag->getText();
                element->setTextContent(textContent);
            } else if(childTag->getType() == TagType_TAG) {
                recursivelyInitElement(childTag, element);
            }
        }
    }
    wxArrayString attributeNames = element->getAllSettedAtributeNames();
    element->initElement(parent, &attributeNames);
    element->applyAttributes(&attributeNames);
    
    element->onWillAddToParent(parent);
    element->setParent(parent);
    if (parent != NULL) {
        parent->addChild(element);
        parent->onChildAdded(element);
    }
    element->onAddedToParent();
    
    if(!element->isInitChildrenBeforeTag()) {
        for (int i = 0; i < childrenCount; i++) {
            Tag* childTag = tag->getChildren()[i];
            if(childTag->getType() == TagType_RAW_TEXT) {
                wxString textContent=element->getTextContent()+childTag->getText();
                element->setTextContent(textContent);
            } else if(childTag->getType() == TagType_TAG) {
                recursivelyInitElement(childTag, element);
            }
        }
    }
    
    element->setInitPhase(false);
    element->onFinishedInitialisation();
    return element;
}

void Engine::removeDomElement(DomElement*domElement) {
    DomElement*parent=domElement->getParent();
    if(parent!=NULL){
        int index=parent->getChildIndex(domElement);
        if(index!=-1) {
            domElement->onRemovingFromParent();
            parent->onChildRemoving(domElement);
            parent->removeChildByIndex(index);
        }
    }
    
    if (domElement->hasSettedAttribute("id")) {
        wxString id=domElement->getAttribute("id");
        unregisterDomElementById(id);
    }
    getLua()->tableRefRemove(domElement->getLuaRef());
    domElement->clearLuaRef();
    domElement->destroyElement();
    //TODO: check if I should do delete domElement
}

void Engine::addElementIdChangedEventHandler(std::function<void(wxString, DomElement*element)>handler) {
    elementIdChangedEventHandlers.push_back(handler);
}

void Engine::removeElementIdChangedEventHandler(std::function<void(wxString, DomElement*element)>handler) {
    for(int i=0;i<elementIdChangedEventHandlers.size();i++){
        auto&_handler=elementIdChangedEventHandlers[i];
        if(getFunctionAddress(_handler)==getFunctionAddress(handler)){
            elementIdChangedEventHandlers.erase(elementIdChangedEventHandlers.begin()+i);
            return;
        }
    }
}

void Engine::fireElementIdChangedEvent(wxString&id, DomElement*domElement) {
    for(int i=0; i<elementIdChangedEventHandlers.size(); i++) {
        elementIdChangedEventHandlers[i](id, domElement);
    }
}

void Engine::unregisterDomElementById(wxString&id) {
    if(idToDomElementMap.find(id)!=idToDomElementMap.end()) {
        idToDomElementMap.erase(id);
        fireElementIdChangedEvent(id, NULL);
    }
}

void Engine::registerDomElementById(wxString&id, DomElement*domElement) {
    idToDomElementMap[id]=domElement;
    fireElementIdChangedEvent(id, domElement);
}

void Engine::replaceChildrenFromString(DomElement*currentDomElement, wxString&innerLXML) {
    int childrenCount=currentDomElement->getChildrenCount();
    for (int i=0; i<childrenCount; i++) {
        removeDomElement(currentDomElement->getChild(0));
    }
    currentDomElement->repaint();
    
    innerLXML.Trim();
    if(innerLXML.IsEmpty()) return;
    
    TagsParser parser = createParser(innerLXML, "innerLXML");
    std::vector<Tag*>tags = parser.parseTags();
    for(int i=0;i<tags.size();i++){
        DomElement*newChildElement = recursivelyInitElement(tags[i], currentDomElement);
        delete tags[i];
    }
}

DomElement*Engine::getDomElementById(wxString&id) {
    return idToDomElementMap[id];
}

ExecBuilder Engine::execFunctionFromAttributeBuilder(const TagAttribute&tagAttribute) {
    if (tagAttribute.getType() == TA_FUNCTION) {
        return lua->functionRefExec(tagAttribute.getFunctionRef());
    } else if(tagAttribute.getType() == TA_STRING) {
        return lua->globalFunctionExec(tagAttribute.getString());
    }else{
        throw RuntimeException(wxString::Format("Cannot execute attribute as function. Attribute type is %d", tagAttribute.getType()));
    }
}

DomElement*getSelfDomElement(Engine*engine, ValuesListReader*args) {
    DomElement* domElement;
    args->getTable(0, [&domElement](TableReader*tableReader){
        domElement=(DomElement*)tableReader->getUserData("_nativeHandler");
    });
    if(domElement==NULL) {
        throw NativeError(wxString::Format("Cannot extract _nativehandle from dom element"));
    }
    return domElement;
}

void pushAttributeToFunctionResult(TagAttribute*attribute, ValuesListWriter*retValues) {
    TagAttributeType type=attribute->getType();
    switch(type) {
        case TA_NULL:{
            return;
        }
        case TA_INT:{
            int val=attribute->getInt();
            retValues->pushInt(val);
            return;
        }
        case TA_DOUBLE:{
            double val=attribute->getDouble();
            retValues->pushDouble(val);
            return;
        }
        case TA_STRING:{
            wxString val=attribute->getString();
            retValues->pushString(val);
            return;
        }
        case TA_BOOL:{
            bool val=attribute->getBool();
            retValues->pushBool(val);
            return;
        }
        case TA_FUNCTION:{
            FunctionRef ref=attribute->getFunctionRef();
            retValues->pushFunction(ref, false);
            return;
        }
    }
}

TagAttribute extractTagAttributeValueFromArgs(ValuesListReader*args, int index) {
    switch (args->getType(index)) {
        case lxe::LTYPE_DOUBLE: return TagAttribute().setDouble(args->getDouble(index));
        case lxe::LTYPE_STRING:  return TagAttribute().setString(args->getString(index));
        case lxe::LTYPE_BOOL: return TagAttribute().setBool(args->getBool(index));
        case lxe::LTYPE_FUNCTION:return TagAttribute().setFunction(args->getFunctionRef(index));
        default: return TagAttribute().setNull();
    }
}

void ffi_DomElementPrototype_setAttribute(Engine*engine, ValuesListReader*args, ValuesListWriter*retValues) {
    DomElement*domElement = getSelfDomElement(engine, args);
    wxString attributeName = args->getString(1);
    try {
        TagAttribute attribute=extractTagAttributeValueFromArgs(args, 2);
        domElement->setAttribute(attributeName, attribute);
    } catch(RuntimeException&ex) {
        throw NativeError(wxString::Format("Cannot set attribute '%s'. Error message: %s", attributeName, ex.getErrorMessage()));
    }
}

void ffi_DomElementPrototype_getAttribute(Engine*engine, ValuesListReader*args, ValuesListWriter*retValues) {
    DomElement*domElement=getSelfDomElement(engine, args);
    wxString attributeName=args->getString(1);
    TagAttribute attribute;
    if(domElement->getDynamicAttributeValue(attributeName, attribute)) {
        pushAttributeToFunctionResult(&attribute, retValues);
        return;
    }
    if(!domElement->hasSettedAttribute(attributeName)) {
        return;//nil
    }
    attribute=domElement->getComputedAttribute(attributeName);
    pushAttributeToFunctionResult(&attribute, retValues);
}

int ffi_DomElementPrototype_hasAttribute(Engine*engine, ValuesListReader*args) {
    DomElement*domElement = getSelfDomElement(engine, args);
    wxString attributeName = args->getString(1);
    return domElement->hasSettedAttribute(attributeName);
}

void ffi_Document_getElementById(Engine*engine, ValuesListReader*args, ValuesListWriter*retValues) {
    wxString id=args->getString(1);
    
    DomElement*domElement=engine->getDomElementById(id);
    retValues->pushTableRef(domElement->getLuaRef(), false);
}

void Engine::registerNativeFunctions(){
    lua->registerNativeFunction("DomElementPrototype_hasAttribute", [this](ValuesListReader*args, ValuesListWriter*retValues) {
        retValues->pushBool(ffi_DomElementPrototype_hasAttribute(this, args));
    });
    lua->registerNativeFunction("DomElementPrototype_getAttribute", [this](ValuesListReader*args, ValuesListWriter*retValues) {
        ffi_DomElementPrototype_getAttribute(this, args, retValues);
    });
    lua->registerNativeFunction("DomElementPrototype_setAttribute", [this](ValuesListReader*args, ValuesListWriter*retValues) {
        ffi_DomElementPrototype_setAttribute(this, args, retValues);
    });
    lua->registerNativeFunction("Document_getElementById", [this](ValuesListReader*args, ValuesListWriter*retValues) {
        ffi_Document_getElementById(this, args, retValues);
    });
}

//----------------- Script
Script::Script() {
    if(allowedAttributes==NULL) {
        allowedAttributes=createAndFillStringsMap({});
    }
    addAllowedAttributeNamesMap(allowedAttributes);
    setChildrenAllowed(true);
}

void Script::onFinishedInitialisation() {
    getEngine()->getLua()->evalFile(getTextContent(), "scriptTag");
}

bool Script::handleChangedAttribute(const wxString&name, TagAttribute&oldValue, TagAttribute&newValue) {
    return DomElement::handleChangedAttribute(name, oldValue, newValue);
}

bool Script::getDynamicAttributeValue(const wxString&attributeName, TagAttribute&tagAttribute) {
    return DomElement::getDynamicAttributeValue(attributeName, tagAttribute);
}
