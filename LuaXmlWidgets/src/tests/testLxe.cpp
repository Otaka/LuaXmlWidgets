//
//  testLxe.cpp
//  LuaXmlWidgets
//

#include "lxw.hpp"

#ifdef LUA_XML_TEST

#define TEST_NO_MAIN
#include "accutestWrapper.hpp"
/*
 root
  -folderA
    -nestedFolder
      -nestedFile.txt
    -file In FolderA
  -folderB
    -file In FolderB.txt
  -file_in_root_1
 */
const char*SerializedFolderData="\x08\x00\x00\x00\x14\x00\x00\x00\x66ile In FolderB.txt\x00\x00\t\x00\x00\x00\x00\x00\x00\x00\x08\x00\x00\x00\x66olderB\x00\x01\x01\x00\x00\x00\x00\x00\x00\x00\x13\x00\x00\x00\x66ile_in_root_1.txt\x00\x00\x07\x00\x00\x00\t\x00\x00\x00\x14\x00\x00\x00\x66ile In FolderA.txt\x00\x00\t\x00\x00\x00\x10\x00\x00\x00\x0f\x00\x00\x00nestedFile.txt\x00\x00\x0e\x00\x00\x00\x19\x00\x00\x00\r\x00\x00\x00nestedFolder\x00\x01\x01\x00\x00\x00\x04\x00\x00\x00\x08\x00\x00\x00\x66olderA\x00\x01\x02\x00\x00\x00\x03\x00\x00\x00\x05\x00\x00\x00\x02\x00\x00\x00/\x00\x01\x03\x00\x00\x00\x01\x00\x00\x00\x02\x00\x00\x00\x06\x00\x00\x00\x62 content134\n567a contentnested content";

void testSerializedFolderReader() {
    SerializedFolderReader folderReader;
    folderReader.load(SerializedFolderData, [](){});
    auto rootFolder=folderReader.findChunk("/");
    TEST_EQUALS_BOOL(rootFolder->isFile, false);
    auto rootFolderChildren=folderReader.listChildren(rootFolder);
    TEST_EQUALS_INT((int)rootFolderChildren.size(), 3);
    TEST_EQUALS_WXSTR(rootFolderChildren[0]->name, "folderB");
    TEST_EQUALS_BOOL(rootFolderChildren[0]->isFile, false);
    TEST_EQUALS_WXSTR(rootFolderChildren[1]->name, "file_in_root_1.txt");
    TEST_EQUALS_BOOL(rootFolderChildren[1]->isFile, true);
    TEST_EQUALS_WXSTR(wxString::FromAscii(rootFolderChildren[1]->data, rootFolderChildren[1]->dataLength), "134\n567");
    TEST_EQUALS_WXSTR(rootFolderChildren[2]->name, "folderA");
    TEST_EQUALS_BOOL(rootFolderChildren[2]->isFile, false);
}

void testSerializedFolderReader_GetByPath() {
    SerializedFolderReader folderReader;
    folderReader.load(SerializedFolderData, [](){});
    auto file1=folderReader.findChunk("/folderA/file In FolderA.txt");
    TEST_EQUALS_BOOL(file1->isFile, true);
    TEST_EQUALS_WXSTR(file1->name, "file In FolderA.txt");
    TEST_EQUALS_WXSTR(wxString::FromAscii(file1->data, file1->dataLength), "a content");
    
    auto file2=folderReader.findChunk("/folderA/nestedFolder/nestedFile.txt");
    TEST_EQUALS_BOOL(file2->isFile, true);
    TEST_EQUALS_WXSTR(file2->name, "nestedFile.txt");
    TEST_EQUALS_WXSTR(wxString::FromAscii(file2->data, file2->dataLength), "nested content");
    
}

ACUTEST_MODULE_INITIALIZER(lxe_module) {
    ACUTEST_ADD_TEST_(testSerializedFolderReader);
    ACUTEST_ADD_TEST_(testSerializedFolderReader_GetByPath);
}

#endif
