//
//  lxwUtils.cpp
//  LuaXmlWidgets
//

#include "lxw.hpp"

void ImageHolder::init(std::function<void(wxBitmap*bitmap)>onImageLoaded, std::function<void()>onImageRemoved, std::function<void(wxString errorMessage)>onError) {
    this->onImageLoaded = onImageLoaded;
    this->onImageRemoved = onImageRemoved;
    this->onError = onError;
}

void ImageHolder::load(wxString&path){
    path.Trim();
    if(path==imagePath)
        return;
    imagePath=path;
    if(path=="") {
        onImageRemoved();
        return;
    }
    wxBitmap bitmap;
    bool result=bitmap.LoadFile(path,wxBITMAP_TYPE_ANY);
    if(result) {
        onImageLoaded(&bitmap);
    } else {
        onError(wxString::Format("Cannot load image '%s'", path));
    }
}

std::map<std::string, int> StringToWxKeyMap = {
    {"esc", WXK_ESCAPE},
    {"f1", WXK_F1},
    {"f2", WXK_F2},
    {"f3", WXK_F3},
    {"f4", WXK_F4},
    {"f5", WXK_F5},
    {"f6", WXK_F6},
    {"f7", WXK_F7},
    {"f8", WXK_F8},
    {"f9", WXK_F9},
    {"f10", WXK_F10},
    {"f11", WXK_F11},
    {"f12", WXK_F12},
    {"f13", WXK_F13},
    {"f14", WXK_F14},
    {"f15", WXK_F15},
    {"f16", WXK_F16},
    {"f17", WXK_F17},
    {"f18", WXK_F18},
    {"f19", WXK_F19},
    {"f20", WXK_F20},
    {"f21", WXK_F21},
    {"f22", WXK_F22},
    {"f23", WXK_F23},
    {"f24", WXK_F24},
    {"space", WXK_SPACE},
    {"backspace", WXK_BACK},
    {"delete", WXK_DELETE},
    {"tab", WXK_TAB},
    {"enter", WXK_RETURN},
    {"insert", WXK_INSERT},
    {"home", WXK_HOME},
    {"end", WXK_END},
    {"pageup", WXK_PAGEUP},
    {"pagedown", WXK_PAGEDOWN},
    {"escape", WXK_ESCAPE},
    {"right", WXK_RIGHT},
    {"left", WXK_LEFT},
    {"up", WXK_UP},
    {"down", WXK_DOWN},
    {"0", '0'},{"1", '1'},{"2", '2'},
    {"3", '3'},{"4", '4'},{"5", '5'},
    {"6", '6'},{"7", '7'},{"8", '8'},{"9", '9'},
    {"a", 'A'},{"b", 'B'},{"c", 'C'},{"d", 'D'},
    {"e", 'E'},{"f", 'F'},{"g", 'G'},{"h", 'H'},
    {"i", 'I'},{"j", 'J'},{"k", 'K'},{"l", 'L'},
    {"m", 'M'},{"n", 'N'},{"o", 'O'},{"p", 'P'},
    {"q", 'Q'},{"r", 'R'},{"s", 'S'},{"t", 'T'},
    {"u", 'U'},{"v", 'V'},{"w", 'W'},{"x", 'X'},
    {"y", 'Y'},{"z", 'Z'}
};

int stringToWxKey(wxString keyString){
    keyString = keyString.Lower();
    auto element = StringToWxKeyMap.find(keyString.ToStdString());
    if (element == StringToWxKeyMap.end())
        return -1;
    return element->second;
}

Hotkey parseHotkeyString(wxString hotkeyStr) {
    wxString originalHotkey=hotkeyStr;
    // Split the hotkey string by '+'
    std::vector<wxString> tokens;
    int pos = 0;
    
    while ((pos = hotkeyStr.find('+')) != std::string::npos) {
        tokens.push_back(wxString(hotkeyStr.substr(0, pos)).MakeLower());
        hotkeyStr=eraseFromLeft(hotkeyStr, pos + 1);
    }
    tokens.push_back(hotkeyStr); // Add the last token
    bool hasCtrl=false;
    bool hasShift=false;
    bool hasAlt=false;
    bool hasOption=false;
    bool hasCommand=false;
    int key=-1;
    // Parse each token
    for (const auto& token : tokens) {
        if (token=="ctrl") {
            if(hasCtrl) throw RuntimeException(wxString::Format("Hotkey '%s' should not have two Ctrl modifiers", originalHotkey));
            hasCtrl=true;
            continue;
        }
        if (token=="shift") {
            if(hasShift) throw RuntimeException(wxString::Format("Hotkey '%s' should not have two Shift modifiers", originalHotkey));
            hasShift=true;
            continue;
        }
        if (token=="alt") {
            if(hasAlt) throw RuntimeException(wxString::Format("Hotkey '%s' should not have two Alt modifiers", originalHotkey));
            hasAlt=true;
            continue;
        }
#ifdef __WXMAC__
        if (token=="option") {
            if(hasOption) throw RuntimeException(wxString::Format("Hotkey '%s' should not have two Option modifiers", originalHotkey));
            hasOption=true;
            continue;
        }
        if (token=="command") {
            if(hasCommand) throw RuntimeException(wxString::Format("Hotkey '%s' should not have two Command modifiers", originalHotkey));
            hasCommand=true;
            continue;
        }
#endif
        
        int _key = stringToWxKey(token);
        if (_key == -1) throw RuntimeException(wxString::Format("Hotkey '%s' contains unknown key '%s'", originalHotkey, token));
        if (key!=-1) throw RuntimeException(wxString::Format("Hotkey '%s' several keys in hotkey are not supported(not counting modifier keys)", originalHotkey));
        key=_key;
    }
    
    if (key==-1) throw RuntimeException(wxString::Format("Hokey '%s' should have one key(not counting modifier keys)", originalHotkey));
    
    Hotkey hotkey;
    hotkey.key = key;
    hotkey.modifiers = 0;
#ifdef __WXMAC__
    if(hasCtrl)hotkey.modifiers   |= wxMOD_RAW_CONTROL;
    if(hasCommand)hotkey.modifiers|= wxMOD_CONTROL;
    if(hasOption)hotkey.modifiers |= wxMOD_ALT;
#else
    if(hasCtrl)hotkey.modifiers  |= wxMOD_CONTROL;
#endif
    if(hasShift)hotkey.modifiers |= wxMOD_SHIFT;
    if(hasAlt)hotkey.modifiers   |= wxMOD_ALT;

    return hotkey;
}
