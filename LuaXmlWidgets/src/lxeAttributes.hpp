//
//  lxeAttributes.hpp
//

#ifndef attributes_h
#define attributes_h

#include "lxe.hpp"

namespace lxe {

enum TagAttributeType {
    TA_INT=100, TA_DOUBLE, TA_STRING, TA_BOOL,TA_FUNCTION, TA_NULL
};

class TagAttribute {
private:
    wxAny value;
    TagAttributeType type;
public:
    TagAttribute();
    wxAny&getValue();
    TagAttributeType getType()const;
    TagAttribute&setNull();
    TagAttribute&setString(const wxString&str);
    TagAttribute&setInt(int value);
    TagAttribute&setDouble(double value);
    TagAttribute&setBool(bool value);
    TagAttribute&setFunction(FunctionRef ref);
    
    bool isNull()const;
    bool equals(TagAttribute&attr)const;
    
    wxString getString()const;
    int getInt()const;
    double getDouble()const;
    bool getBool()const;
    FunctionRef getFunctionRef()const;
    
    wxString defaultIfNull(const wxString defaultValue)const{
        return isNull()?
    defaultValue:
        getString();
        
    }
    int defaultIfNull(int defaultValue)const{return isNull()?defaultValue:getInt();}
    double defaultIfNull(double defaultValue)const{return isNull()?defaultValue:getDouble();}
    bool defaultIfNull(bool defaultValue)const{return isNull()?defaultValue:getBool();}
};

typedef std::unordered_map<wxString,  TagAttribute> AttributesMap;

void attributeHashMapNamesVisitor(AttributesMap*map, std::function<void(wxString&attrName)>handler);

class PropertiesAttributes {
public:
    int propertiesId;
    AttributesMap attributes;
    int order;
};

class AttributesStorage {
    std::vector<String2BoolHashMap*>recreationAttributeNamesMaps;
    std::vector<String2BoolHashMap*>allowedAttributeNamesMaps;
    AttributesMap directAttributes;
    std::vector<PropertiesAttributes> attributesFromProperties;
    std::function<void(const wxString&name, TagAttribute&oldValue, TagAttribute&newValue)>onChange;
public:
    void setOnChangeEventHandler(std::function<void(const wxString&name, TagAttribute&oldValue, TagAttribute&newValue)>onChange);
    std::function<void(const wxString&name, TagAttribute&oldValue, TagAttribute&newValue)> getOnChangeEventHandler();
    void addAllowedAttributeNamesMap(String2BoolHashMap*map);
    void addRecreationAttributeNamesMap(String2BoolHashMap*map);
    bool isAttributeNameAllowed(const wxString&attributeName);
    bool isAttributeRequireRecreation(const wxString&attributeName);
    bool isDirectAttributeSet(const wxString&attributeName);
    wxArrayString getSettedDirectAttributeNames();
    wxArrayString getAllSettedAtributeNames();
    TagAttribute getAttribute(const wxString&attributeName);
    void removeAttribute(const wxString&name, bool fireEvent);
    void setAttribute(const wxString&attributeName, TagAttribute&value, bool fireEvent);
    bool supportedAttributeName(const wxString&attributeName);
    void addProperties(PropertiesAttributes&propertiesAttributes, bool fireEvent);
    void removeProperties(int id, bool fireEvent);
    void modifyProperties(PropertiesAttributes&propertiesAttributes, bool fireEvent, std::function<void()>actualModification) ;
    void sortPropertiesByOrder();
};

}
#endif /* attributes_h */
