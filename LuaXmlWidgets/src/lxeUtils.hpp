//
//  lxeUtils.hpp
//  LuaXmlWidgets
//

#ifndef lxml_utils_hpp
#define lxml_utils_hpp

#include "lxe.hpp"

typedef std::unordered_map<wxString, bool> String2BoolHashMap;
typedef std::unordered_map<wxString, int> String2IntHashMap;

class RuntimeException {
private:
    wxString errorMessage;
public:
    RuntimeException(wxString errorMessage) {
        this->errorMessage=errorMessage;
    }
    wxString&getErrorMessage() {return errorMessage;}
};

class DomEntityRequireRecreationException {
    
};

double random_double_bound(double max);
int random_int_bound(int max);
double random_double();
int selector(const wxString&valueToFind, std::vector<wxString>stringValues, std::vector<int>intValues);
int selector(bool flag, int valTrue, int valFalse);
void removeFromStringArray(wxArrayString*stringArray, wxString str);
wxString eraseFromLeft(wxString str, int count);



template<typename T, typename... U>
size_t getFunctionAddress(std::function<T(U...)> f) {
    typedef T(fnType)(U...);
    fnType ** fnPointer = f.template target<fnType*>();
    return (size_t) *fnPointer;
}

class SerializedFileChunk {
public:
    wxString name;
    bool isFile;
    char*data;
    int dataLength;
    std::vector<int>childrenIndexes;
    
};

class SerializedFolderReader {
    std::vector<SerializedFileChunk> chunks;//last chunk is always root directory
    const char* data;
    std::function<void()> onUnload;
    
    SerializedFileChunk readChunk(const char*data, unsigned int&dataIndex);
    uint32_t readUint32(const char*data, unsigned int&dataIndex);
    wxString readString(const char*data, unsigned int&dataIndex);
    bool readBool(const char*data, unsigned int&dataIndex);
    SerializedFileChunk*findChunk(std::vector<wxString>&pathParts, int pathIndex, SerializedFileChunk*parentFileChunk);
    std::vector<wxString>splitPathParts(wxString&path);
    
public:
    void load(const char*data, std::function<void()> onUnload);
    void unload() { onUnload(); }
    SerializedFileChunk*findChunk(wxString path);
    bool isFile(SerializedFileChunk*chunk){ return chunk->isFile; }
    bool isFolder(SerializedFileChunk*chunk){ return !chunk->isFile; }
    std::vector<SerializedFileChunk*> listChildren(SerializedFileChunk*chunk);
};

String2BoolHashMap*createAndFillStringsMap(std::vector<wxString>names);
#endif
