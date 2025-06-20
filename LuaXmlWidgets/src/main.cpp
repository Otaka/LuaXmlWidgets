#include "lxw.hpp"

#include <wx/filename.h>

#ifndef LUA_XML_TEST

class LuaXmlWidgetsApp : public wxApp {
public:
    virtual bool OnInit();
    void testCreateGui();
    virtual bool OnExceptionInMainLoop() {
        try {
            throw; // Rethrow the current exception.
        } catch (RuntimeException& e) {
            wxPrintf("Exception %s\n", e.getErrorMessage());
        }catch ( ... ) {
            wxPrintf("Unknown exception\n");
        }
        return false;
    }
    virtual void OnUnhandledException() {
        try {
            throw; // Rethrow the current exception.
        } catch (RuntimeException& e) {
            wxPrintf("Exception %s\n", e.getErrorMessage());
        }catch ( ... ) {
            wxPrintf("Unknown exception\n");
        }
    }

};
DECLARE_APP(LuaXmlWidgetsApp)
LuaXmlWidgetsApp*app;

IMPLEMENT_APP(LuaXmlWidgetsApp)

wxString normalizeFilePath(wxString string){
    wxFileName fileName(string);
    fileName.Normalize(wxPATH_NORM_ENV_VARS);
    return fileName.GetAbsolutePath();
}

class CommandLineArgs{
public:
    wxString*sourceDirectory=NULL;
    wxString*mainFilePath=NULL;
    bool printHelp=false;
};

CommandLineArgs parseCommandLineArgs(wxArrayString args){
    CommandLineArgs result;
    auto expectArg=[&args](int index, wxString errorMessage){
        if(args.size()<=index)
            throw wxString::Format("%s\n", errorMessage);
    };

    for(int i=1;i<args.size();i++) {
        if(args[i]=="-h" || args[i]=="--help") {
            result.printHelp=true;
            continue;
        }
        if(args[i]=="-d" || args[i]=="--directory") {
            expectArg(i+1, "-d/--directory expects existing path to source directory");
            i++;
            result.sourceDirectory= new wxString(normalizeFilePath(args[i]));
            continue;
        }
        if(args[i]=="-f" || args[i]=="--main-file") {
            expectArg(i+1, "-f/--main-file expects name of the main file inside source directory");
            i++;
            result.mainFilePath=new wxString(args[i]);
            continue;
        }
        throw wxString::Format("Unknown arg %s\n", args[i]);
    }
    return result;
}


bool LuaXmlWidgetsApp::OnInit() {
    //testCreateGui();
    CommandLineArgs args;
    try {
        args = parseCommandLineArgs(wxApp::argv.GetArguments());
    } catch(wxString&ex) {
        wxPrintf("Command line arguments error: %s\n", ex);
        return false;
    }
    
    if(args.printHelp){
        wxPrintf( R"(
LuaXmlWidgets - A console application for creating GUI widgets from LXML files.
Usage: lxmlwidgets [options]

Options:
    -h, --help          Print this help message.
    -d, --directory     Specify the source folder where the source LXML files are located. Default is "." - the current directory.
    -f, --main-file     Specify the main .lxml file relative path inside the working folder. Extension ".lxml" is optional. Default is "main".

Description:
    LuaXmlWidgets reads LXML files, which are similar to HTML but in XML format, and creates windows, buttons, text fields, and other GUI widgets. The application supports the <script> tag with embedded Lua scripts, allowing dynamic and interactive interfaces using native components based on the WxWidgets library.
)" );
        return false;
    }
    
    if(args.mainFilePath!=NULL ) {
        if(args.sourceDirectory==NULL){
            wxFileName fileName=*args.mainFilePath;
            wxString parent=fileName.GetPath(true);
            if(parent=="") {
                parent=normalizeFilePath(".");
            }
            args.sourceDirectory=new wxString(parent);
            delete args.mainFilePath;
            args.mainFilePath=new wxString(fileName.GetFullName());
        }
    }
    
    if(args.sourceDirectory==NULL) {
        args.sourceDirectory=new wxString(normalizeFilePath("."));
    }
    
    if(args.mainFilePath==NULL) {
        args.mainFilePath=new wxString("main");
    }
    
    wxFileName fileName = *args.mainFilePath;
    if(!fileName.HasExt()) {
        fileName.SetExt("lxml");
        *args.mainFilePath=fileName.GetFullName();
    }
    
    wxFileName sourceDir(*args.sourceDirectory);
    if(!sourceDir.Exists()) {
        wxPrintf("Source directory folder does not exists path: %s\n", *args.sourceDirectory);
        return false;
    }
    
    wxFileName mainFilePath(*args.sourceDirectory, *args.mainFilePath);
    if(!mainFilePath.Exists()) {
        wxPrintf("Main file '%s' does not exists in source directory '%s'\n", *args.mainFilePath, *args.sourceDirectory);
        return false;
    }
    wxFile sourceFile = wxFile(mainFilePath.GetAbsolutePath());
    wxString source;
    sourceFile.ReadAll(&source);
    try {
        lxwGui*gui = new lxwGui();
        gui->load(source, *args.mainFilePath);
    } catch(std::runtime_error&err) {
        wxPrintf("Error running the application: %s\n", err.what());
        return false;
    } catch(RuntimeException&err) {
        wxPrintf("Error running the application: %s\n", err.getErrorMessage());
        return false;
    }

    return true;
}




//void OnHotkey1(wxKeyEvent& e) {
//    wxPrintf("Hotkey1 pressed id=%d\n", e.GetId());
//}

//void LuaXmlWidgetsApp::testCreateGui() {
//    try {
//        wxFrame *frame = new wxFrame((wxFrame*) NULL, -1, _T("Hello wxWidgets World"), wxDefaultPosition, wxDefaultSize, wxDEFAULT_FRAME_STYLE );
////        Hotkey hotkey=parseHotkeyString("Ctrl+6");
////        int hotkeyId1=1;
////        bool hotkeyRegistered=frame->RegisterHotKey(hotkeyId1, hotkey.modifiers, hotkey.key);
////        wxPrintf("Hotkey1 registered %d\n", hotkeyRegistered);
////        frame->Bind(wxEVT_HOTKEY, OnHotkey1, hotkeyId1);
//        
//        frame->Move(600, 600);
//        frame->Layout();
//        wxInitAllImageHandlers();
//        wxStaticText*label=new wxStaticText(frame, -1, _T("Static text adkjh asdkjhas dsad aslkh asdlkjh asdlkh ad jk hlkjh lkj hlas klasd lkasdl akjsdl kajdlkjsdklj dhsaljkh dsafkjh sdfkljhsdfakljhsda flkjsdhaf s"), wxDefaultPosition,wxSize(200,50), wxBORDER_SUNKEN|wxST_NO_AUTORESIZE| wxST_ELLIPSIZE_END);
//        wxTextCtrl*text=new wxTextCtrl(frame, -1, _T(""), wxPoint(10, 50),wxSize(200, 50));
//        text->Bind(wxEVT_KEY_DOWN, [](wxKeyEvent&event){
//            wxPrintf("Pressed %c\n", event.GetKeyCode());
//        });
//        wxButton*button=new wxButton(frame, -1, _T("Button"), wxDefaultPosition,wxSize(50, 50), wxBORDER_DEFAULT);
//        button->SetLabelMarkup("m<b>b</b>");
//
//        // Create a wxDataViewCtrl
//        wxTreeCtrl* tree = new wxTreeCtrl(frame, wxID_ANY, wxDefaultPosition, wxSize(200, 300), wxTR_DEFAULT_STYLE|wxTR_HIDE_ROOT|wxTR_HAS_BUTTONS|wxTR_TWIST_BUTTONS);
//        auto _root=tree->GetRootItem();
//        auto root=tree->AppendItem(_root,"MyRoot");
//        auto folder1=tree->AppendItem(root,"Folder1");
//        auto folder2=tree->AppendItem(root,"Folder2");
//
//        auto k1=tree->AppendItem(folder2,"Kjsdsflk sdflkj ");
//        auto folder3=tree->AppendItem(root,"Folder3");
//        wxColour c(255,0,0);
//        tree->SetFocusedItem(folder3);
//        frame->Show(true);
//        SetTopWindow(frame);
//    } catch(RuntimeException&ex) {
//        wxPrintf("Unhandled exception: %s\n", ex.getErrorMessage());
//    }
//}

/**
 wxFrame *frame = new wxFrame((wxFrame*) NULL, -1, _T("Hello wxWidgets World"));
 frame->CreateStatusBar();
 frame->SetStatusText(_T("Hello World"));
 wxStyledTextCtrl*text=new wxStyledTextCtrl(frame);
 frame->Show(true);
 SetTopWindow(frame);
 */


#endif
