//
//  lxeAttributes.cpp
//
#include "lxe.hpp"

using namespace lxe;

void lxe::attributeHashMapNamesVisitor(AttributesMap*map, std::function<void(wxString&attrName)>handler) {
    for(auto it = map->begin(); it != map->end(); ++it) {
        wxString key=it->first;
        handler(key);
    }
}

TagAttribute::TagAttribute():value(0) {
    this->type=TA_NULL;
}
wxAny&TagAttribute::getValue() {
    return value;
}
TagAttributeType TagAttribute::getType()const {
    return type;
}
TagAttribute&TagAttribute::setNull() {
    type=TA_NULL;
    this->value.MakeNull();
    return *this;
}
TagAttribute&TagAttribute::setString(const wxString&str) {
    type=TA_STRING;
    this->value=str;
    return *this;
}
TagAttribute&TagAttribute::setInt(int value) {
    type=TA_INT;
    this->value=value;
    return *this;
}
TagAttribute&TagAttribute::setDouble(double value) {
    type=TA_DOUBLE;
    this->value=value;
    return *this;
}
TagAttribute&TagAttribute::setBool(bool value) {
    type=TA_BOOL;
    this->value=value;
    return *this;
}
TagAttribute&TagAttribute::setFunction(FunctionRef ref) {
    type=TA_FUNCTION;
    this->value=ref;
    return *this;
}
bool TagAttribute::isNull()const{
    return type==TA_NULL;
}

bool TagAttribute::equals(TagAttribute&attr)const {
    if(type!=attr.getType())return false;
    switch(type) {
        case TA_NULL:return true;
        case TA_INT:return value.As<int>()==attr.value.As<int>();
        case TA_DOUBLE:return value.As<double>()==attr.value.As<double>();
        case TA_BOOL:return value.As<bool>()==attr.value.As<bool>();
        case TA_FUNCTION:return value.As<FunctionRef>().ref==attr.getInt();
        case TA_STRING: {
            wxString left=value.As<wxString>();
            wxString right=attr.value.As<wxString>();
            return left==right;
        }
    }
    return true;
}

wxString TagAttribute::getString()const {
    switch(type) {
        case TA_INT:return wxString::Format("%d",value.As<int>());
        case TA_DOUBLE:return wxString::Format("%lf",value.As<double>());
        case TA_BOOL:return wxString(value.As<bool>()?"true":"false");
        case TA_FUNCTION:return wxString::Format("Function_ref#%d",value.As<FunctionRef>().ref);
        case TA_STRING:return value.As<wxString>();
        case TA_NULL:
            return "null";
        default:
            throw RuntimeException(wxString::Format("Cannot get value of attribute with type %d as wxString", type));
    }
}

int TagAttribute::getInt()const {
    switch(type) {
        case TA_INT:return value.As<int>();
        case TA_DOUBLE:return (int)value.As<double>();
        case TA_BOOL:return (int)value.As<bool>();
        case TA_FUNCTION:return value.As<FunctionRef>().ref;
        case TA_STRING: {
            double dvalue;
            if(value.As<wxString>().ToDouble(&dvalue)) {
                return dvalue;
            } else {
                throw RuntimeException("Cannot convert string value to integer");
            }
        };
        case TA_NULL:
            throw RuntimeException("Cannot get value of attribute as integer. It is null");
        default:
            throw RuntimeException(wxString::Format("Cannot get value of attribute as integer. Attribute type %d does not implemented", type));
    }
}

double TagAttribute::getDouble()const {
    switch(type) {
        case TA_INT:return value.As<int>();
        case TA_DOUBLE:return value.As<double>();
        case TA_BOOL:return value.As<bool>()?1.0:0.0;
        case TA_STRING: {
            double dvalue;
            if(value.As<wxString>().ToDouble(&dvalue)) {
                return dvalue;
            } else {
                throw RuntimeException("Cannot convert string value to double");
            }
        };
        case TA_NULL:
            return 0;
        default:
            throw RuntimeException(wxString::Format("Cannot get value of attribute as double. Attribute type %d does not implemented", type));
    }
}

bool TagAttribute::getBool()const {
    switch(type) {
        case TA_INT:return value.As<int>()!=0;
        case TA_DOUBLE:return ((int)value.As<double>())!=0;
        case TA_BOOL:return value.As<bool>();
        case TA_FUNCTION:return true;
        case TA_STRING: {
            const wxString&strValue=value.As<wxString>();
            return strValue=="true"||strValue=="1";
        };
        case TA_NULL:
            return false;
        default:
            throw RuntimeException(wxString::Format("Cannot get value of attribute. Attribute type %d does not implemented", type));
    }
}
FunctionRef TagAttribute::getFunctionRef()const{
    switch(type) {
        case TA_FUNCTION:return value.As<FunctionRef>();
        case TA_NULL:
            FunctionRef ref;
            ref.ref=-1;
            return ref;
        default:
            throw RuntimeException(wxString::Format("Cannot get value of attribute as function. Attribute type %d does not implemented", type));
    }
}


void AttributesStorage::setOnChangeEventHandler(std::function<void(const wxString&name, TagAttribute&oldValue, TagAttribute&newValue)>onChange) {
    this->onChange=onChange;
}

std::function<void(const wxString&name, TagAttribute&oldValue, TagAttribute&newValue)> AttributesStorage::getOnChangeEventHandler() {
    return onChange;
}

void AttributesStorage::addAllowedAttributeNamesMap(String2BoolHashMap*map) {
    allowedAttributeNamesMaps.push_back(map);
}
void AttributesStorage::addRecreationAttributeNamesMap(String2BoolHashMap*map) {
    recreationAttributeNamesMaps.push_back(map);
}

bool AttributesStorage::isAttributeNameAllowed(const wxString&attributeName) {
    for(int i=0;i<allowedAttributeNamesMaps.size();i++){
        if(allowedAttributeNamesMaps[i]->find(attributeName)!=allowedAttributeNamesMaps[i]->end()) {
            return true;
        }
    }
    return false;
}

bool AttributesStorage::isAttributeRequireRecreation(const wxString&attributeName) {
    for(int i=0;i<recreationAttributeNamesMaps.size();i++){
        if(recreationAttributeNamesMaps[i]->find(attributeName)!=recreationAttributeNamesMaps[i]->end()){
            return true;
        }
    }
    return false;
}

bool AttributesStorage::isDirectAttributeSet(const wxString&attributeName) {
    return directAttributes.find(attributeName)!=directAttributes.end();
}

wxArrayString AttributesStorage::getSettedDirectAttributeNames() {
    wxArrayString result;
    attributeHashMapNamesVisitor(&directAttributes, [&result](wxString&name) {
        result.push_back(name);
    });
    return result;
}

wxArrayString AttributesStorage::getAllSettedAtributeNames() {
    wxArrayString result;
    String2BoolHashMap map;
    attributeHashMapNamesVisitor(&directAttributes, [&map](wxString&name) {
        map[name]=true;
    });
    for(int i=0;i<attributesFromProperties.size();i++) {
        attributeHashMapNamesVisitor(&(attributesFromProperties[i].attributes), [&map](wxString&name) {
            map[name]=true;
        });
    }
    for(auto it = map.begin(); it != map.end(); ++it) {
        result.push_back(it->first);
    }
    return result;
}

TagAttribute AttributesStorage::getAttribute(const wxString&attributeName) {
    auto found=directAttributes.find(attributeName);
    if(found!=directAttributes.end()) {
        return found->second;
    }
    for(int i=0;i<attributesFromProperties.size();i++) {
        PropertiesAttributes&propertiesAttributes=attributesFromProperties[i];
        auto found=propertiesAttributes.attributes.find(attributeName);
        if(found!=propertiesAttributes.attributes.end()) {
            return found->second;
        }
    }
    return TagAttribute().setNull();
}

void AttributesStorage::removeAttribute(const wxString&name, bool fireEvent) {
    if(isDirectAttributeSet(name)) {
        TagAttribute oldValue = getAttribute(name);
        directAttributes.erase(name);
        if(fireEvent && !oldValue.isNull()) {
            TagAttribute newValue;
            newValue.setNull();
            onChange(name, oldValue, newValue);
        }
    }
}

void AttributesStorage::setAttribute(const wxString&attributeName, TagAttribute&value, bool fireEvent) {
    if(!supportedAttributeName(attributeName)) {
        throw RuntimeException(wxString::Format("Tag does not support attribute '%s'", attributeName));
    }
    TagAttribute oldAttribute = getAttribute(attributeName);
    directAttributes[attributeName] = value;
    if(fireEvent && !oldAttribute.equals(value)) {
        onChange(attributeName, oldAttribute, value);
    }
}

bool AttributesStorage::supportedAttributeName(const wxString&attributeName){
    for(int i=0;i<allowedAttributeNamesMaps.size();i++){
        if(allowedAttributeNamesMaps[i]->find(attributeName)!=allowedAttributeNamesMaps[i]->end()){
            return true;
        }
    }
    return false;
}

void AttributesStorage::addProperties(PropertiesAttributes&propertiesAttributes, bool fireEvent) {
    modifyProperties(propertiesAttributes, fireEvent, [this, &propertiesAttributes](){
        attributesFromProperties.push_back(propertiesAttributes);
        sortPropertiesByOrder();
    });
}

void AttributesStorage::removeProperties(int id, bool fireEvent){
    for(int i=0;i<attributesFromProperties.size();i++){
        PropertiesAttributes&propertiesAttributes=attributesFromProperties[i];
        if(propertiesAttributes.propertiesId==id){
            modifyProperties(propertiesAttributes, fireEvent, [this, &propertiesAttributes, i](){
                attributesFromProperties.erase(attributesFromProperties.begin()+i);
            });
            break;
        }
    }
}

void AttributesStorage::modifyProperties(PropertiesAttributes&propertiesAttributes, bool fireEvent, std::function<void()>actualModification) {
    //remember old values
    std::vector<TagAttribute>oldAttributeValues;
    AttributesMap&propAttrMap=propertiesAttributes.attributes;
    wxArrayString propertiesKeys;
    for(auto it = propAttrMap.begin(); it != propAttrMap.end(); ++it) {
        propertiesKeys.push_back(it->first);
    }
    for(int i=0;i<propertiesKeys.size();i++){
        oldAttributeValues.push_back(getAttribute(propertiesKeys[i]));
    }
    // Modify the properties list
    actualModification();
    
    //check if old attribute not equal to new attribute and then send onChange event
    if(fireEvent) {
        for(int i=0;i<propertiesKeys.size();i++) {
            wxString&propKey=propertiesKeys[i];
            TagAttribute newAttr=getAttribute(propKey);
            TagAttribute oldAttr=oldAttributeValues[i];
            if(!newAttr.equals(oldAttr)) {
                onChange(propertiesKeys[i], oldAttr, newAttr);
            }
        }
    }
}

void AttributesStorage::sortPropertiesByOrder(){
    std::sort(attributesFromProperties.begin(), attributesFromProperties.end(), [](const PropertiesAttributes &a, const PropertiesAttributes &b)
    {
        return a.order > b.order;
    });
}
