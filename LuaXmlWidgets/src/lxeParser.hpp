//
//  lxeParser.hpp
//
#ifndef XmlParser_hpp
#define XmlParser_hpp

#include "lxe.hpp"
namespace lxe {

enum TokenType {
    TokenType_TAG=0, TokenType_TAG_CLOSING, TokenType_COMMENT, TokenType_RAW_TEXT, TokenType_EOF
};

enum TagType {
    TagType_TAG=200, TagType_COMMENT, TagType_RAW_TEXT
};

class ParseException {
private:
    wxString errorMessage;
    int line;
public:
    ParseException(wxString errorMessage, int line) {
        this->errorMessage=errorMessage;
        this->line=line;
    }
    wxString&getErrorMessage() {return errorMessage;}
    int getLine(){return line;}
};

class StringParser{
private:
    wxString string;
    int stringLength;
    int currentIndex;
    int line;
public:
    StringParser(wxString&string);
    bool eof();
    void skip(int count);

    wchar_t getChar();
    wchar_t peek(int offset);
    void rewind();
    void skipBlank();
    int getCurrentIndex();
    int getLine();
    void setCurrentIndex(int currentIndex);
    bool match(wxString token);
};

class Token {
protected:
    TokenType type;
    int line;
    //for tags
    wxString tagName;
    bool selfClosed;
    AttributesMap attributes;
    //for comments and for rawtext
    wxString text;
public:
    TokenType getType() { return type; }
    int getLine(){return line;}
    wxString&getTagName(){return tagName;}
    bool isSelfClosed() {return selfClosed;}
    AttributesMap& getAttributes(){return attributes;}
    wxString& getText(){return text;}
    
    static Token createEOF(int line);
    static Token createTagToken(int line, wxString&tagName, AttributesMap&attributes, bool selfClosed);
    static Token createClosingTagToken(int line, wxString&tagName);
    static Token createCommentToken(int line, wxString&text);
    static Token createRawTextToken(int line, wxString&text);
};

class Tag {
private:
    TagType type;
    std::vector<Tag*> children;
    wxString tagName;
    AttributesMap attributes;
    int line;
    wxString text;
public:
    Tag(int line) {
        this->line = line;
    }
    ~Tag(){
        for(int i=0;i<children.size();i++){
            delete children[i];
        }
    }
    wxArrayString getAttributeNames(){
        wxArrayString list;
        AttributesMap::iterator it;
        for( it = attributes.begin(); it != attributes.end(); ++it )
        {
            wxString key = it->first;
            list.Add(key);
        }
        return list;
    }
    Tag& initAsTag(wxString tagName, AttributesMap&attributes){
        this->type=TagType_TAG;
        this->tagName=tagName;
        this->attributes=attributes;
        return *this;
    }
    Tag& initAsRawText(wxString&text){
        this->type=TagType_RAW_TEXT;
        this->text=text;
        return *this;
    }
    Tag& initAsComment(wxString&text){
        this->type=TagType_COMMENT;
        this->text=text;
        return *this;
    }
    TagType getType(){return type;}
    std::vector<Tag*>&getChildren(){return children;}
    void addChild(Tag*child){children.push_back(child);}
    wxString&getTagName(){return tagName;}
    AttributesMap&getAttributes(){return attributes;}
    int getLine(){return line;}
    wxString&getText(){return text;}
};

class TagsTokenizer {
private:
    StringParser*stringParser;
    std::vector<wxString>tagsStack;
    wxArrayString tagsWithRawTextContent;
    bool rawTextFinished;
    Token _ungetToken;
    bool hasUngetToken=false;
public:
    TagsTokenizer(wxString source);
    ~TagsTokenizer();

    void addTagWithRawTextContent(wxString tag);
    void ungetToken(Token ungetToken);
    Token nextToken();
    Token readRawText();
    Token readTag();
    Token readComment();
    std::tuple<wxString,TagAttribute> readAttribute();
    Token readClosingTag();
    wxString readToken();
    wxString readQuotedString();
    wxString readNumber();
    bool isNumberChar(wchar_t c);
    bool isTokenStartChar(wchar_t c);
    bool isTokenChar(wchar_t c);
    wxString lastTagInStack();
};

class TagsParser {
private:
    TagsTokenizer*tokenizer;
    wxString fileName;
    wxArrayString tagWithRawTextContent;
    Tag*internalParseTag();
public:
    TagsParser(wxString source, wxString fileName);
    ~TagsParser();
    void addTagWithRawTextContent(wxString tag);
    std::vector<Tag*>parseTags();
};

}

#endif /* XmlParser_hpp */
