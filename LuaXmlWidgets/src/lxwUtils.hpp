//
//  lxwUtils.hpp
//

#ifndef lxwUtils_hpp
#define lxwUtils_hpp

#include "lxw.hpp"

class RegisteredEvent {
public:
    wxString functionName;
    int functionRef;
};

class ImageHolder{
private:
    std::function<void(wxBitmap*bitmap)>onImageLoaded;
    std::function<void()>onImageRemoved;
    std::function<void(wxString errorMessage)>onError;
    wxString imagePath;
    /// Image loading often asynchronous process, you just point the address, and after some time the system will execute callback with loaded bitmap
    /// The problem appears when you set path with huge image, it starts to load, and then you immidiately ask to load another image. Both threads will load the image and after some random time both will execute onImageLoaded callback. but in fact only latest should be loaded.
    /// loading index helps to identify this. !!! Right now it is not asynchronous, that is why it does nothing
    volatile int loadingIndex;
    wxBitmap*bitmap;
public:
    void init(std::function<void(wxBitmap*bitmap)>onImageLoaded,std::function<void()>onImageRemoved, std::function<void(wxString errorMessage)>onError);
    void load(wxString&path);
};

class Hotkey {
public:
    int key;
    int modifiers;
};
int stringToWxKey(wxString keyString);
Hotkey parseHotkeyString(wxString hotkeyStr);



#endif /* lxwUtils_hpp */
