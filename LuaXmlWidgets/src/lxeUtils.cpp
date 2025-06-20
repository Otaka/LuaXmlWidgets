//
//  lxeUtils.cpp
//  LuaXmlWidgets
//

#include "lxe.hpp"

#include <wx/tokenzr.h>

double random_double_bound(double max) {
    return ((double)rand() / RAND_MAX)*max;
}

int random_int_bound(int max) {
    return ((double)rand() / RAND_MAX)*max;
}


double random_double() {
    return (double)rand() / (double)RAND_MAX ;
}

int selector(const wxString&valueToFind, std::vector<wxString>stringValues, std::vector<int>intValues) {
    if(stringValues.size()!=intValues.size())
        throw RuntimeException(wxString::Format("Function 'selector' should accept two lists with equal size, but first has %d and second %d", stringValues.size(), intValues.size()));
    for(int i=0;i<stringValues.size();i++) {
        if(valueToFind==stringValues[i]) {
            return intValues[i];
        }
    }
    return -1;
}

int selector(bool flag, int valTrue, int valFalse){
    return flag?valTrue: valFalse;
}

void removeFromStringArray(wxArrayString*stringArray, wxString str){
    int index=stringArray->Index(str);
    if(index!=-1) stringArray->RemoveAt(index);
}

wxString eraseFromLeft(wxString str, int count) {
    if(count>=str.length())return "";
    return str.Right(str.size()-count);
};

String2BoolHashMap*createAndFillStringsMap(std::vector<wxString>names) {
    String2BoolHashMap*m = new String2BoolHashMap();
    for (int i=0; i<names.size(); i++) {
        wxString&attrName = names[i];
        (*m)[attrName] = true;
    }
    return m;
}

wxString SerializedFolderReader::readString(const char*data, unsigned int&dataIndex){
    int length=readUint32(data, dataIndex);
    wxString str=wxString::FromUTF8(data+dataIndex);
    dataIndex+=length;
    return str;
}

uint32_t SerializedFolderReader::readUint32(const char*data, unsigned int&dataIndex){
    uint32_t value=*(uint32_t*)(data+dataIndex);
    dataIndex+=4;
    return value;
}

bool SerializedFolderReader::readBool(const char*data, unsigned int&dataIndex){
    char value=*(char*)(data+dataIndex);
    dataIndex++;
    return value;
}

SerializedFileChunk*SerializedFolderReader::findChunk(wxString path){
    path.Replace("\\", "/");
    //we accept only absolute file paths
    if(!path.StartsWith("/")) {
        return NULL;
    }
    std::vector<wxString>pathParts = splitPathParts(path);
    if(pathParts.size()==1 && pathParts[0]==""){
        return &chunks[chunks.size()-1];
    }
    return findChunk(pathParts, 1, &chunks[chunks.size()-1]);
}

SerializedFileChunk*SerializedFolderReader::findChunk(std::vector<wxString>&pathParts, int pathIndex, SerializedFileChunk*parentFileChunk) {
    wxString&pathPartName=pathParts[pathIndex];
    bool isLastPathPart=pathIndex>=pathParts.size()-1;
    for(int i=0;i<parentFileChunk->childrenIndexes.size();i++){
        int childIndex=parentFileChunk->childrenIndexes[i];
        SerializedFileChunk*chunk=&chunks[childIndex];
        if (chunk->name==pathPartName) {
            if (isLastPathPart) {
                return chunk;
            } else {
                return findChunk(pathParts, pathIndex+1, chunk);
            }
        }
    }
    return NULL;
}



std::vector<wxString> SerializedFolderReader::splitPathParts(wxString&path){
    wxStringTokenizer tokenizer(path, wxT("/"));
    std::vector<wxString> tokens;
    while (tokenizer.HasMoreTokens()) {
        tokens.push_back(tokenizer.GetNextToken());
    }
    return tokens;
}

SerializedFileChunk SerializedFolderReader::readChunk(const char*data, unsigned int&dataIndex) {
    SerializedFileChunk file;
    file.name=readString(data, dataIndex);
    file.isFile = !readBool(data, dataIndex);
    if (file.isFile) {
        uint32_t dataSize=readUint32(data, dataIndex);
        uint32_t dataOffset=readUint32(data, dataIndex);
        file.data=reinterpret_cast<char*>(dataOffset);
        file.dataLength=dataSize;
    } else {
        uint32_t childrenCount = readUint32(data, dataIndex);
        file.data=NULL;
        file.dataLength=0;
        for(int i=0;i<childrenCount;i++) {
            int fileIndex=readUint32(data, dataIndex);
            file.childrenIndexes.push_back(fileIndex);
        }
    }
    return file;
}

std::vector<SerializedFileChunk*> SerializedFolderReader::listChildren(SerializedFileChunk*chunk) {
    std::vector<SerializedFileChunk*>result;
    for (int i=0; i<chunk->childrenIndexes.size(); i++) {
        result.push_back(&chunks[chunk->childrenIndexes[i]]);
    }
    return result;
}

std::pair<char*, uint32_t>getFileContentWithSize(SerializedFileChunk*chunk) {
    std::pair<char*, uint32_t>dataWithSize;
    dataWithSize.first=chunk->data;
    dataWithSize.second=chunk->dataLength;
    return dataWithSize;
}

void SerializedFolderReader::load(const char*data, std::function<void()> onUnload) {
    this->data=data;
    this->onUnload=onUnload;
    unsigned int dataIndex=0;
    uint32_t chunksCount=readUint32(data, dataIndex);
    for(int i=0;i<chunksCount;i++) {
        chunks.push_back(readChunk(data, dataIndex));
    }
    const char*startOfDataSection=data+dataIndex;
    for(int i=0;i<chunksCount;i++) {
        SerializedFileChunk&chunk=chunks[i];
        if(chunk.isFile){
            chunk.data=(char*)startOfDataSection+ (unsigned int)(unsigned long long)chunk.data;
        }
    }
}
