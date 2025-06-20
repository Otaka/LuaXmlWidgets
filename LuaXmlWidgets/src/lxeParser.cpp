//
//  lxeParser.cpp
//

#include "lxe.hpp"

using namespace lxe;

StringParser::StringParser(wxString&string){
    this->string=string;
    stringLength=(unsigned int)string.length();
    currentIndex=0;
    line=0;
}

bool StringParser::eof(){
    return currentIndex>=stringLength;
}

void StringParser::skip(int count){
    for (int i = 0; i < count; i++)
        getChar();
}

wchar_t StringParser::getChar() {
    if (eof()) {
        return (char) -1;
    }
    wchar_t c = string[currentIndex].GetValue();
    if (c == '\n')
        line++;
    currentIndex++;
    return c;
}
wchar_t StringParser::peek(int offset) {
    if (currentIndex + offset >= stringLength) {
        return (wchar_t) -1;
    }
    return string[currentIndex + offset].GetValue();
}

void StringParser::rewind() {
    currentIndex--;
}

void StringParser::skipBlank() {
    while (!eof()) {
        wchar_t c = getChar();
        if (!(c == ' ' || c == '\t' || c == '\n' || c == '\r')) {
            rewind();
            break;
        }
    }
}

int StringParser::getCurrentIndex() {
    return currentIndex;
}

int StringParser::getLine(){
    return line;
}

void StringParser::setCurrentIndex(int currentIndex) {
    this->currentIndex = currentIndex;
}

bool StringParser::match(wxString token) {
    int restOfLength = stringLength - currentIndex;
    if (token.length() > restOfLength) {
        return false;
    }

    for (int i = 0; i < token.length(); i++) {
        wchar_t tokenChar = token[i].GetValue();
        wchar_t sourceChar = string[currentIndex + i].GetValue();
        if (tokenChar != sourceChar) {
            return false;
        }
    }
    skip((unsigned int)token.length());
    return true;
}

Token Token::createEOF(int line){
    Token token;
    token.type=TokenType_EOF;
    token.line=line;
    return token;
}
Token Token::createTagToken(int line, wxString&tagName, AttributesMap&attributes, bool selfClosed){
    Token token;
    token.type=TokenType_TAG;
    token.line=line;
    token.selfClosed=selfClosed;
    token.attributes=attributes;
    token.tagName=tagName;
    return token;
}
Token Token::createClosingTagToken(int line, wxString&tagName){
    Token token;
    token.type=TokenType_TAG_CLOSING;
    token.line=line;
    token.tagName=tagName;
    return token;
}
Token Token::createCommentToken(int line, wxString&text){
    Token token;
    token.type=TokenType_COMMENT;
    token.line=line;
    token.text=text;
    return token;
}
Token Token::createRawTextToken(int line, wxString&text){
    Token token;
    token.type=TokenType_RAW_TEXT;
    token.line=line;
    token.text=text;
    return token;
}



TagsTokenizer::TagsTokenizer(wxString source) {
    stringParser=new StringParser(source);
}

TagsTokenizer::~TagsTokenizer(){
    delete stringParser;
}

void TagsTokenizer::addTagWithRawTextContent(wxString tag) {
    tagsWithRawTextContent.Add(tag);
}

void TagsTokenizer::ungetToken(Token ungetToken) {
    this->_ungetToken = ungetToken;
    hasUngetToken = true;
}

Token TagsTokenizer::nextToken() {
    if (hasUngetToken) {
        hasUngetToken=false;
        return _ungetToken;
    }

    stringParser->skipBlank();
    if (stringParser->eof()) {
        return Token::createEOF(stringParser->getLine());
    }
    
    if (stringParser->match("<!--")) {
        return readComment();
    }
    if(stringParser->match("<")){
        return readTag();
    }

    rawTextFinished = false;
    if (!rawTextFinished && !tagsStack.empty() && tagsWithRawTextContent.Index(lastTagInStack())!=wxNOT_FOUND) {
        return readRawText();
    }
    wchar_t c = stringParser->getChar();
    throw ParseException(wxString::Format("Expected start of tag, but found [%c]", c), stringParser->getLine());
}

Token TagsTokenizer::readRawText() {
    int line = stringParser->getLine();
    wxString text;
    while (true) {
        wchar_t c = stringParser->peek(0);
        if (c == '<') {
            int parsingPosition = stringParser->getCurrentIndex();
            stringParser->getChar();
            stringParser->skipBlank();
            if (stringParser->peek(0) == '/') {
                stringParser->getChar();
                if (isTokenStartChar(stringParser->peek(0))) {
                    wxString endTagName = readToken();
                    if (!tagsStack.empty() && lastTagInStack()==endTagName) {
                        stringParser->skipBlank();
                        if (stringParser->peek(0) == '>') {
                            //finish token is matched
                            stringParser->setCurrentIndex(parsingPosition);
                            rawTextFinished = true;
                            break;
                        }
                    }
                }
            }
            stringParser->setCurrentIndex(parsingPosition);
        }

        text.append(c);
        stringParser->getChar();
    }
    return Token::createRawTextToken(line, text);
}

Token TagsTokenizer::readTag() {
    stringParser->skipBlank();
    int line = stringParser->getLine();
    wchar_t c = stringParser->peek(0);
    if (c == '/') {
        stringParser->getChar();
        return readClosingTag();
    } else if (c == '>') {
        throw ParseException("Empty tags <> are not allowed", stringParser->getLine());
    } else if (isTokenStartChar(c)) {
        wxString tagName = readToken();
        bool selfClosing = false;
        AttributesMap attributes;
        while (true) {
            stringParser->skipBlank();
            c = stringParser->peek(0);
            if (c == (wchar_t) -1) {
                throw ParseException(wxString::Format("Unexpected end while parsing tag [%s]", tagName), stringParser->getLine());
            }
            if (c == '/') {
                selfClosing = true;
                stringParser->getChar();
                if (stringParser->peek(0) != '>') {
                    throw ParseException(wxString::Format("Expected [>] after self close mark [/], but found [%c]", stringParser->peek(0)), stringParser->getLine());
                }
                stringParser->getChar();
                break;
            } else if (c == '>') {
                stringParser->getChar();
                break;
            }
            if (isTokenStartChar(c)) {
                auto[name, tagAttribute] = readAttribute();
                attributes[name] = tagAttribute;
            }
        }

        if (!selfClosing) {
            tagsStack.push_back(tagName);
        }
        
        return Token::createTagToken(line, tagName, attributes, selfClosing);
    } else {
        throw ParseException(wxString::Format("Unexpected symbol [%c] while parsing tag", c), stringParser->getLine());
    }
}

Token TagsTokenizer::readComment() {
    int line = stringParser->getLine();
    wxString text;
    while (true) {
        if (stringParser->eof()) {
            break;
        }
        if (stringParser->match("-->")) {
            break;
        }
        wchar_t c = stringParser->peek(0);
        text.append(c);
        stringParser->getChar();
    }
    return Token::createCommentToken(line, text);
}

std::tuple<wxString,TagAttribute> TagsTokenizer::readAttribute() {
    wxString attributeName = readToken();
    stringParser->skipBlank();
    wchar_t equalSign = stringParser->getChar();
    if (equalSign != '=') {
        throw ParseException("Expected [=] after attribute name", stringParser->getLine());
    }
    stringParser->skipBlank();
    wchar_t c = stringParser->peek(0);
    if (c == '"' || c == '\'') {
        wxString str=readQuotedString();
        return {attributeName, TagAttribute().setString(str)};
    } else if (isNumberChar(c)) {
        wxString value = readNumber();
        if(value.Contains('.')) {
            double resultDouble;
            value.ToDouble(&resultDouble);
            return {attributeName, TagAttribute().setDouble(resultDouble)};
        } else {
            int resultInt;
            value.ToInt(&resultInt);
            return {attributeName, TagAttribute().setInt(resultInt)};
        }
    } else {
        if (stringParser->match("true")) {
            return {attributeName, TagAttribute().setBool(true)};
        } else if (stringParser->match("false")) {
            return {attributeName, TagAttribute().setBool(false)};
        } else {
            throw ParseException(wxString::Format("Cannot parse attribute value of attribute [%s]", attributeName), stringParser->getLine());
        }
    }
}

Token TagsTokenizer::readClosingTag() {
    int line = stringParser->getLine();
    wxString tagName = readToken();
    if (tagName.empty()) {
        throw ParseException("Expected tag name", line);
    }
    wxString openingTagName = lastTagInStack();
    tagsStack.pop_back();
    if (openingTagName!=tagName) {
        throw ParseException(wxString::Format("Closing tag [%s] is not related to opening tag [%s]",tagName,openingTagName), line);
    }
    stringParser->skipBlank();
    wchar_t closeMark = stringParser->getChar();
    if (closeMark != '>') {
        throw ParseException(wxString::Format("Closing tag should end with [>] found [%c]",closeMark), stringParser->getLine());
    }
    return Token::createClosingTagToken(line, tagName);
}

wxString TagsTokenizer::readToken() {
    stringParser->skipBlank();
    wxString sb;
    while (true) {
        wchar_t peekedChar = stringParser->peek(0);
        if (isTokenChar(peekedChar)) {
            sb.append(peekedChar);
            stringParser->getChar();
        } else {
            break;
        }
    }
    return sb;
}

wxString TagsTokenizer::readQuotedString() {
    int line=stringParser->getLine();
    wchar_t quote = stringParser->getChar();
    wxString sb;
    while (true) {
        wchar_t c = stringParser->getChar();
        if (c == (wchar_t) -1) {
            throw ParseException("Found EOF while parsing string", line);
        }
        if (c == quote) {
            break;
        }
        if (c == '\\') {
            wchar_t nc = stringParser->getChar();
            switch (nc) {
                case 'n':
                    sb.append("\n");break;
                case 'r':
                    sb.append("\r");break;
                case 't':
                    sb.append("\t");break;
                case '\\':
                    sb.append("\\");break;
                case '\'':
                    sb.append("'");break;
                case '"':
                    sb.append("\"");break;
                default:
                    sb.append(nc);break;
            }
            continue;
        }
        sb.append(c);
    }
    return sb;
}

wxString TagsTokenizer::readNumber() {
    wxString sb;
    if (stringParser->peek(0) == '-') {
        sb.Append("-");
        stringParser->getChar();
        stringParser->skipBlank();
    }
    while (true) {
        wchar_t c = stringParser->peek(0);
        if (isNumberChar(c)) {
            sb.append(c);
            stringParser->getChar();
        } else if (c == '.') {
            if (sb.Contains(".")) {
                throw ParseException(wxString::Format("Cannot parse number because it has 2 dots [.]: %s", sb), stringParser->getLine());
            }
            sb.append(".");
            stringParser->getChar();
        } else {
            break;
        }
    }
    return sb;
}

bool TagsTokenizer::isNumberChar(wchar_t c) {
    return c == '-' || (c>='0' && c<='9');
}

bool TagsTokenizer::isTokenStartChar(wchar_t c) {
    return (c>='a'&&c<='z')||(c>='A'&&c<='Z')||c=='_';
}

bool TagsTokenizer::isTokenChar(wchar_t c) {
    return isTokenStartChar(c) || isNumberChar(c) || c == ':' || c == '-';
}

wxString TagsTokenizer::lastTagInStack(){
    return tagsStack[tagsStack.size()-1];
}


TagsParser::TagsParser(wxString source, wxString fileName) {
   this->fileName = fileName;
   tokenizer = new TagsTokenizer(source);
}

TagsParser::~TagsParser() {
    delete tokenizer;
}

void TagsParser::addTagWithRawTextContent(wxString tag) {
    tagWithRawTextContent.push_back(tag);
    tokenizer->addTagWithRawTextContent(tag);
}


Tag*TagsParser::internalParseTag() {
    Token token = tokenizer->nextToken();
    if (token.getType() == TokenType_EOF) {
        return NULL;
    }
    
    if (token.getType() == TokenType_COMMENT) {
        Tag*tag=new Tag(token.getLine());
        tag->initAsComment(token.getText());
        return tag;
    }
    if (token.getType() == TokenType_RAW_TEXT) {
        Tag*tag=new Tag(token.getLine());
        tag->initAsRawText(token.getText());
        return tag;
    }
    if (token.getType() == TokenType_TAG_CLOSING) {
        throw ParseException(wxString("").Format("Found close tag [%s] that does not related to any opened tag",token.getTagName()), token.getLine());
    }
    if (token.getType() == TokenType_TAG) {
        Tag*tag=new Tag(token.getLine());
        tag->initAsTag(token.getTagName(),token.getAttributes());
        if (token.isSelfClosed()) {
            return tag;
        }
        
        while (true) {
            Token nextToken = tokenizer->nextToken();
            if (nextToken.getType() == TokenType_EOF) {
                throw ParseException(wxString("").Format("Found end of input while search for </%s>", token.getTagName()), token.getLine());
            }
            if (nextToken.getType() == TokenType_TAG_CLOSING) {
                if (nextToken.getTagName()!=token.getTagName()) {
                    throw ParseException(wxString("").Format("Opened tag [%s] but found closing tag [%s]", token.getTagName(), nextToken.getTagName()), nextToken.getLine());
                }
                return tag;
            }
            tokenizer->ungetToken(nextToken);
            Tag*childTag = internalParseTag();
            if(childTag!=NULL) {
                tag->addChild(childTag);
            }
        }
    }
    throw ParseException(wxString::Format("Unprocessed token type %d",token.getType()), token.getLine());
}

std::vector<Tag*>TagsParser::parseTags() {
    std::vector<Tag*>result;
    while (true) {
        Tag*entity = internalParseTag();
        if (entity == NULL) {
            return result;
        }
        if (entity->getType() == TagType_COMMENT) {
            continue;
        }
        result.push_back(entity);
    }
}
