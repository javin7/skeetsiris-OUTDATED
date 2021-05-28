/// Json-cpp amalgamated source (http://jsoncpp.sourceforge.net/).
/// It is intended to be used with #include "json/json.h"

// //////////////////////////////////////////////////////////////////////
// Beginning of content of file: LICENSE
// //////////////////////////////////////////////////////////////////////

/*
The JsonCpp library's source code, including accompanying documentation,
tests and demonstration applications, are licensed under the following
conditions...

Baptiste Lepilleur and The JsonCpp Authors explicitly disclaim copyright in all
jurisdictions which recognize such a disclaimer. In such jurisdictions,
this software is released into the Public Domain.

In jurisdictions which do not recognize Public Domain property (e.g. Germany as of
2010), this software is Copyright (c) 2007-2010 by Baptiste Lepilleur and
The JsonCpp Authors, and is released under the terms of the MIT License (see below).

In jurisdictions which recognize Public Domain property, the user of this
software may choose to accept it either as 1) Public Domain, 2) under the
conditions of the MIT License (see below), or 3) under the terms of dual
Public Domain/MIT License conditions described here, as they choose.

The MIT License is about as close to Public Domain as a license can get, and is
described in clear, concise terms at:

   http://en.wikipedia.org/wiki/MIT_License

The full text of the MIT License follows:

========================================================================
Copyright (c) 2007-2010 Baptiste Lepilleur and The JsonCpp Authors

Permission is hereby granted, free of charge, to any person
obtaining a copy of this software and associated documentation
files (the "Software"), to deal in the Software without
restriction, including without limitation the rights to use, copy,
modify, merge, publish, distribute, sublicense, and/or sell copies
of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be
included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
========================================================================
(END LICENSE TEXT)

The MIT license is compatible with both the GPL and commercial
software, affording one all of the rights of Public Domain with the
minor nuisance of being required to keep the above copyright notice
and license text in the source code. Note also that by accepting the
Public Domain "license" you can re-license your copy using whatever
license you like.

*/

// //////////////////////////////////////////////////////////////////////
// End of content of file: LICENSE
// //////////////////////////////////////////////////////////////////////






#include "json/json.h"

#ifndef JSON_IS_AMALGAMATION
#error "Compile with -I PATH_TO_JSON_DIRECTORY"
#endif


// //////////////////////////////////////////////////////////////////////
// Beginning of content of file: src/lib_json/json_tool.h
// //////////////////////////////////////////////////////////////////////

// Copyright 2007-2010 Baptiste Lepilleur and The JsonCpp Authors
// Distributed under MIT license, or public domain if desired and
// recognized in your jurisdiction.
// See file LICENSE for detail or copy at http://jsoncpp.sourceforge.net/LICENSE

#ifndef LIB_JSONCPP_JSON_TOOL_H_INCLUDED
#define LIB_JSONCPP_JSON_TOOL_H_INCLUDED

#if !defined(JSON_IS_AMALGAMATION)
#include <json/config.h>
#endif

// Also support old flag NO_LOCALE_SUPPORT
#ifdef NO_LOCALE_SUPPORT
#define JSONCPP_NO_LOCALE_SUPPORT
#endif

#ifndef JSONCPP_NO_LOCALE_SUPPORT
#include <clocale>
#endif

/* This header provides common string manipulation support, such as UTF-8,
 * portable conversion from/to string...
 *
 * It is an internal header that must not be exposed.
 */

namespace Json {
    static inline char getDecimalPoint() {
#ifdef JSONCPP_NO_LOCALE_SUPPORT
        return '\0';
#else
        struct lconv* lc = localeconv();
        return lc ? *(lc->decimal_point) : '\0';
#endif
    }

    /// Converts a unicode code-point to UTF-8.
    static inline String codePointToUTF8(unsigned int cp) {
        String result;

        // based on description from http://en.wikipedia.org/wiki/UTF-8

        if (cp <= 0x7f) {
            result.resize(1);
            result[0] = static_cast<char>(cp);
        } else if (cp <= 0x7FF) {
            result.resize(2);
            result[1] = static_cast<char>(0x80 | (0x3f & cp));
            result[0] = static_cast<char>(0xC0 | (0x1f & (cp >> 6)));
        } else if (cp <= 0xFFFF) {
            result.resize(3);
            result[2] = static_cast<char>(0x80 | (0x3f & cp));
            result[1] = static_cast<char>(0x80 | (0x3f & (cp >> 6)));
            result[0] = static_cast<char>(0xE0 | (0xf & (cp >> 12)));
        } else if (cp <= 0x10FFFF) {
            result.resize(4);
            result[3] = static_cast<char>(0x80 | (0x3f & cp));
            result[2] = static_cast<char>(0x80 | (0x3f & (cp >> 6)));
            result[1] = static_cast<char>(0x80 | (0x3f & (cp >> 12)));
            result[0] = static_cast<char>(0xF0 | (0x7 & (cp >> 18)));
        }

        return result;
    }

    enum {
        /// Constant that specify the size of the buffer that must be passed to
        /// uintToString.
        uintToStringBufferSize = 3 * sizeof(LargestUInt) + 1
    };

    // Defines a char buffer for use with uintToString().
    using UIntToStringBuffer = char[uintToStringBufferSize];

    /** Converts an unsigned integer to string.
     * @param value Unsigned integer to convert to string
     * @param current Input/Output string buffer.
     *        Must have at least uintToStringBufferSize chars free.
     */
    static inline void uintToString(LargestUInt value, char*& current) {
        *--current = 0;
        do {
            *--current = static_cast<char>(value % 10U + static_cast<unsigned>('0'));
            value /= 10;
        } while (value != 0);
    }

    /** Change ',' to '.' everywhere in buffer.
     *
     * We had a sophisticated way, but it did not work in WinCE.
     * @see https://github.com/open-source-parsers/jsoncpp/pull/9
     */
    template <typename Iter> Iter fixNumericLocale(Iter begin, Iter end) {
        for (; begin != end; ++begin) {
            if (*begin == ',') {
                *begin = '.';
            }
        }
        return begin;
    }

    template <typename Iter> void fixNumericLocaleInput(Iter begin, Iter end) {
        char decimalPoint = getDecimalPoint();
        if (decimalPoint == '\0' || decimalPoint == '.') {
            return;
        }
        for (; begin != end; ++begin) {
            if (*begin == '.') {
                *begin = decimalPoint;
            }
        }
    }

    /**
     * Return iterator that would be the new end of the range [begin,end), if we
     * were to delete zeros in the end of string, but not the last zero before '.'.
     */
    template <typename Iter> Iter fixZerosInTheEnd(Iter begin, Iter end) {
        for (; begin != end; --end) {
            if (*(end - 1) != '0') {
                return end;
            }
            // Don't delete the last zero before the decimal point.
            if (begin != (end - 1) && *(end - 2) == '.') {
                return end;
            }
        }
        return end;
    }

} // namespace Json

#endif // LIB_JSONCPP_JSON_TOOL_H_INCLUDED

// //////////////////////////////////////////////////////////////////////
// End of content of file: src/lib_json/json_tool.h
// //////////////////////////////////////////////////////////////////////






// //////////////////////////////////////////////////////////////////////
// Beginning of content of file: src/lib_json/json_reader.cpp
// //////////////////////////////////////////////////////////////////////

// Copyright 2007-2011 Baptiste Lepilleur and The JsonCpp Authors
// Copyright (C) 2016 InfoTeCS JSC. All rights reserved.
// Distributed under MIT license, or public domain if desired and
// recognized in your jurisdiction.
// See file LICENSE for detail or copy at http://jsoncpp.sourceforge.net/LICENSE

#if !defined(JSON_IS_AMALGAMATION)
#include "json_tool.h"
#include <json/assertions.h>
#include <json/reader.h>
#include <json/value.h>
#endif // if !defined(JSON_IS_AMALGAMATION)
#include <cassert>
#include <cstring>
#include <iostream>
#include <istream>
#include <limits>
#include <memory>
#include <set>
#include <sstream>
#include <utility>

#include <cstdio>
#if __cplusplus >= 201103L

#if !defined(sscanf)
#define sscanf std::sscanf
#endif

#endif //__cplusplus

#if defined(_MSC_VER)
#if !defined(_CRT_SECURE_CPP_OVERLOAD_STANDARD_NAMES)
#define _CRT_SECURE_CPP_OVERLOAD_STANDARD_NAMES 1
#endif //_CRT_SECURE_CPP_OVERLOAD_STANDARD_NAMES
#endif //_MSC_VER

#if defined(_MSC_VER)
// Disable warning about strdup being deprecated.
#pragma warning(disable : 4996)
#endif

// Define JSONCPP_DEPRECATED_STACK_LIMIT as an appropriate integer at compile
// time to change the stack limit
#if !defined(JSONCPP_DEPRECATED_STACK_LIMIT)
#define JSONCPP_DEPRECATED_STACK_LIMIT 1000
#endif

static size_t const stackLimit_g =
JSONCPP_DEPRECATED_STACK_LIMIT; // see readValue()

namespace Json {

#if __cplusplus >= 201103L || (defined(_CPPLIB_VER) && _CPPLIB_VER >= 520)
    using CharReaderPtr = std::unique_ptr<CharReader>;
#else
    using CharReaderPtr = std::auto_ptr<CharReader>;
#endif

    // Implementation of class Features
    // ////////////////////////////////

    Features::Features() = default;

    Features Features::all() { return {}; }

    Features Features::strictMode() {
        Features features;
        features.allowComments_ = false;
        features.allowTrailingCommas_ = false;
        features.strictRoot_ = true;
        features.allowDroppedNullPlaceholders_ = false;
        features.allowNumericKeys_ = false;
        return features;
    }

    // Implementation of class Reader
    // ////////////////////////////////

    bool Reader::containsNewLine(Reader::Location begin, Reader::Location end) {
        for (; begin < end; ++begin)
            if (*begin == '\n' || *begin == '\r')
                return true;
        return false;
    }

    // Class Reader
    // //////////////////////////////////////////////////////////////////

    Reader::Reader() : features_(Features::all()) {}

    Reader::Reader(const Features& features) : features_(features) {}

    bool Reader::parse(const std::string& document, Value& root,
        bool collectComments) {
        document_.assign(document.begin(), document.end());
        const char* begin = document_.c_str();
        const char* end = begin + document_.length();
        return parse(begin, end, root, collectComments);
    }

    bool Reader::parse(std::istream& is, Value& root, bool collectComments) {
        // std::istream_iterator<char> begin(is);
        // std::istream_iterator<char> end;
        // Those would allow streamed input from a file, if parse() were a
        // template function.

        // Since String is reference-counted, this at least does not
        // create an extra copy.
        String doc;
        std::getline(is, doc, static_cast<char> EOF);
        return parse(doc.data(), doc.data() + doc.size(), root, collectComments);
    }

    bool Reader::parse(const char* beginDoc, const char* endDoc, Value& root,
        bool collectComments) {
        if (!features_.allowComments_) {
            collectComments = false;
        }

        begin_ = beginDoc;
        end_ = endDoc;
        collectComments_ = collectComments;
        current_ = begin_;
        lastValueEnd_ = nullptr;
        lastValue_ = nullptr;
        commentsBefore_.clear();
        errors_.clear();
        while (!nodes_.empty())
            nodes_.pop();
        nodes_.push(&root);

        bool successful = readValue();
        Token token;
        skipCommentTokens(token);
        if (collectComments_ && !commentsBefore_.empty())
            root.setComment(commentsBefore_, commentAfter);
        if (features_.strictRoot_) {
            if (!root.isArray() && !root.isObject()) {
                // Set error location to start of doc, ideally should be first token found
                // in doc
                token.type_ = tokenError;
                token.start_ = beginDoc;
                token.end_ = endDoc;
                addError(
                    "A valid JSON document must be either an array or an object value.",
                    token);
                return false;
            }
        }
        return successful;
    }

    bool Reader::readValue() {
        // readValue() may call itself only if it calls readObject() or ReadArray().
        // These methods execute nodes_.push() just before and nodes_.pop)() just
        // after calling readValue(). parse() executes one nodes_.push(), so > instead
        // of >=.
        if (nodes_.size() > stackLimit_g)
            throwRuntimeError("Exceeded stackLimit in readValue().");

        Token token;
        skipCommentTokens(token);
        bool successful = true;

        if (collectComments_ && !commentsBefore_.empty()) {
            currentValue().setComment(commentsBefore_, commentBefore);
            commentsBefore_.clear();
        }

        switch (token.type_) {
        case tokenObjectBegin:
            successful = readObject(token);
            currentValue().setOffsetLimit(current_ - begin_);
            break;
        case tokenArrayBegin:
            successful = readArray(token);
            currentValue().setOffsetLimit(current_ - begin_);
            break;
        case tokenNumber:
            successful = decodeNumber(token);
            break;
        case tokenString:
            successful = decodeString(token);
            break;
        case tokenTrue: {
            Value v(true);
            currentValue().swapPayload(v);
            currentValue().setOffsetStart(token.start_ - begin_);
            currentValue().setOffsetLimit(token.end_ - begin_);
        } break;
        case tokenFalse: {
            Value v(false);
            currentValue().swapPayload(v);
            currentValue().setOffsetStart(token.start_ - begin_);
            currentValue().setOffsetLimit(token.end_ - begin_);
        } break;
        case tokenNull: {
            Value v;
            currentValue().swapPayload(v);
            currentValue().setOffsetStart(token.start_ - begin_);
            currentValue().setOffsetLimit(token.end_ - begin_);
        } break;
        case tokenArraySeparator:
        case tokenObjectEnd:
        case tokenArrayEnd:
            if (features_.allowDroppedNullPlaceholders_) {
                // "Un-read" the current token and mark the current value as a null
                // token.
                current_--;
                Value v;
                currentValue().swapPayload(v);
                currentValue().setOffsetStart(current_ - begin_ - 1);
                currentValue().setOffsetLimit(current_ - begin_);
                break;
            } // Else, fall through...
        default:
            currentValue().setOffsetStart(token.start_ - begin_);
            currentValue().setOffsetLimit(token.end_ - begin_);
            return addError("Syntax error: value, object or array expected.", token);
        }

        if (collectComments_) {
            lastValueEnd_ = current_;
            lastValue_ = &currentValue();
        }

        return successful;
    }

    void Reader::skipCommentTokens(Token& token) {
        if (features_.allowComments_) {
            do {
                readToken(token);
            } while (token.type_ == tokenComment);
        } else {
            readToken(token);
        }
    }

    bool Reader::readToken(Token& token) {
        skipSpaces();
        token.start_ = current_;
        Char c = getNextChar();
        bool ok = true;
        switch (c) {
        case '{':
            token.type_ = tokenObjectBegin;
            break;
        case '}':
            token.type_ = tokenObjectEnd;
            break;
        case '[':
            token.type_ = tokenArrayBegin;
            break;
        case ']':
            token.type_ = tokenArrayEnd;
            break;
        case '"':
            token.type_ = tokenString;
            ok = readString();
            break;
        case '/':
            token.type_ = tokenComment;
            ok = readComment();
            break;
        case '0':
        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7':
        case '8':
        case '9':
        case '-':
            token.type_ = tokenNumber;
            readNumber();
            break;
        case 't':
            token.type_ = tokenTrue;
            ok = match("rue", 3);
            break;
        case 'f':
            token.type_ = tokenFalse;
            ok = match("alse", 4);
            break;
        case 'n':
            token.type_ = tokenNull;
            ok = match("ull", 3);
            break;
        case ',':
            token.type_ = tokenArraySeparator;
            break;
        case ':':
            token.type_ = tokenMemberSeparator;
            break;
        case 0:
            token.type_ = tokenEndOfStream;
            break;
        default:
            ok = false;
            break;
        }
        if (!ok)
            token.type_ = tokenError;
        token.end_ = current_;
        return ok;
    }

    void Reader::skipSpaces() {
        while (current_ != end_) {
            Char c = *current_;
            if (c == ' ' || c == '\t' || c == '\r' || c == '\n')
                ++current_;
            else
                break;
        }
    }

    bool Reader::match(const Char* pattern, int patternLength) {
        if (end_ - current_ < patternLength)
            return false;
        int index = patternLength;
        while (index--)
            if (current_[index] != pattern[index])
                return false;
        current_ += patternLength;
        return true;
    }

    bool Reader::readComment() {
        Location commentBegin = current_ - 1;
        Char c = getNextChar();
        bool successful = false;
        if (c == '*')
            successful = readCStyleComment();
        else if (c == '/')
            successful = readCppStyleComment();
        if (!successful)
            return false;

        if (collectComments_) {
            CommentPlacement placement = commentBefore;
            if (lastValueEnd_ && !containsNewLine(lastValueEnd_, commentBegin)) {
                if (c != '*' || !containsNewLine(commentBegin, current_))
                    placement = commentAfterOnSameLine;
            }

            addComment(commentBegin, current_, placement);
        }
        return true;
    }

    String Reader::normalizeEOL(Reader::Location begin, Reader::Location end) {
        String normalized;
        normalized.reserve(static_cast<size_t>(end - begin));
        Reader::Location current = begin;
        while (current != end) {
            char c = *current++;
            if (c == '\r') {
                if (current != end && *current == '\n')
                    // convert dos EOL
                    ++current;
                // convert Mac EOL
                normalized += '\n';
            } else {
                normalized += c;
            }
        }
        return normalized;
    }

    void Reader::addComment(Location begin, Location end,
        CommentPlacement placement) {
        assert(collectComments_);
        const String& normalized = normalizeEOL(begin, end);
        if (placement == commentAfterOnSameLine) {
            assert(lastValue_ != nullptr);
            lastValue_->setComment(normalized, placement);
        } else {
            commentsBefore_ += normalized;
        }
    }

    bool Reader::readCStyleComment() {
        while ((current_ + 1) < end_) {
            Char c = getNextChar();
            if (c == '*' && *current_ == '/')
                break;
        }
        return getNextChar() == '/';
    }

    bool Reader::readCppStyleComment() {
        while (current_ != end_) {
            Char c = getNextChar();
            if (c == '\n')
                break;
            if (c == '\r') {
                // Consume DOS EOL. It will be normalized in addComment.
                if (current_ != end_ && *current_ == '\n')
                    getNextChar();
                // Break on Moc OS 9 EOL.
                break;
            }
        }
        return true;
    }

    void Reader::readNumber() {
        Location p = current_;
        char c = '0'; // stopgap for already consumed character
        // integral part
        while (c >= '0' && c <= '9')
            c = (current_ = p) < end_ ? *p++ : '\0';
        // fractional part
        if (c == '.') {
            c = (current_ = p) < end_ ? *p++ : '\0';
            while (c >= '0' && c <= '9')
                c = (current_ = p) < end_ ? *p++ : '\0';
        }
        // exponential part
        if (c == 'e' || c == 'E') {
            c = (current_ = p) < end_ ? *p++ : '\0';
            if (c == '+' || c == '-')
                c = (current_ = p) < end_ ? *p++ : '\0';
            while (c >= '0' && c <= '9')
                c = (current_ = p) < end_ ? *p++ : '\0';
        }
    }

    bool Reader::readString() {
        Char c = '\0';
        while (current_ != end_) {
            c = getNextChar();
            if (c == '\\')
                getNextChar();
            else if (c == '"')
                break;
        }
        return c == '"';
    }

    bool Reader::readObject(Token& token) {
        Token tokenName;
        String name;
        Value init(objectValue);
        currentValue().swapPayload(init);
        currentValue().setOffsetStart(token.start_ - begin_);
        while (readToken(tokenName)) {
            bool initialTokenOk = true;
            while (tokenName.type_ == tokenComment && initialTokenOk)
                initialTokenOk = readToken(tokenName);
            if (!initialTokenOk)
                break;
            if (tokenName.type_ == tokenObjectEnd &&
                (name.empty() ||
                    features_.allowTrailingCommas_)) // empty object or trailing comma
                return true;
            name.clear();
            if (tokenName.type_ == tokenString) {
                if (!decodeString(tokenName, name))
                    return recoverFromError(tokenObjectEnd);
            } else if (tokenName.type_ == tokenNumber && features_.allowNumericKeys_) {
                Value numberName;
                if (!decodeNumber(tokenName, numberName))
                    return recoverFromError(tokenObjectEnd);
                name = numberName.asString();
            } else {
                break;
            }

            Token colon;
            if (!readToken(colon) || colon.type_ != tokenMemberSeparator) {
                return addErrorAndRecover("Missing ':' after object member name", colon,
                    tokenObjectEnd);
            }
            Value& value = currentValue()[name];
            nodes_.push(&value);
            bool ok = readValue();
            nodes_.pop();
            if (!ok) // error already set
                return recoverFromError(tokenObjectEnd);

            Token comma;
            if (!readToken(comma) ||
                (comma.type_ != tokenObjectEnd && comma.type_ != tokenArraySeparator &&
                    comma.type_ != tokenComment)) {
                return addErrorAndRecover("Missing ',' or '}' in object declaration",
                    comma, tokenObjectEnd);
            }
            bool finalizeTokenOk = true;
            while (comma.type_ == tokenComment && finalizeTokenOk)
                finalizeTokenOk = readToken(comma);
            if (comma.type_ == tokenObjectEnd)
                return true;
        }
        return addErrorAndRecover("Missing '}' or object member name", tokenName,
            tokenObjectEnd);
    }

    bool Reader::readArray(Token& token) {
        Value init(arrayValue);
        currentValue().swapPayload(init);
        currentValue().setOffsetStart(token.start_ - begin_);
        int index = 0;
        for (;;) {
            skipSpaces();
            if (current_ != end_ && *current_ == ']' &&
                (index == 0 ||
                (features_.allowTrailingCommas_ &&
                    !features_.allowDroppedNullPlaceholders_))) // empty array or trailing
                                                                // comma
            {
                Token endArray;
                readToken(endArray);
                return true;
            }

            Value& value = currentValue()[index++];
            nodes_.push(&value);
            bool ok = readValue();
            nodes_.pop();
            if (!ok) // error already set
                return recoverFromError(tokenArrayEnd);

            Token currentToken;
            // Accept Comment after last item in the array.
            ok = readToken(currentToken);
            while (currentToken.type_ == tokenComment && ok) {
                ok = readToken(currentToken);
            }
            bool badTokenType = (currentToken.type_ != tokenArraySeparator &&
                currentToken.type_ != tokenArrayEnd);
            if (!ok || badTokenType) {
                return addErrorAndRecover("Missing ',' or ']' in array declaration",
                    currentToken, tokenArrayEnd);
            }
            if (currentToken.type_ == tokenArrayEnd)
                break;
        }
        return true;
    }

    bool Reader::decodeNumber(Token& token) {
        Value decoded;
        if (!decodeNumber(token, decoded))
            return false;
        currentValue().swapPayload(decoded);
        currentValue().setOffsetStart(token.start_ - begin_);
        currentValue().setOffsetLimit(token.end_ - begin_);
        return true;
    }

    bool Reader::decodeNumber(Token& token, Value& decoded) {
        // Attempts to parse the number as an integer. If the number is
        // larger than the maximum supported value of an integer then
        // we decode the number as a double.
        Location current = token.start_;
        bool isNegative = *current == '-';
        if (isNegative)
            ++current;
        // TODO: Help the compiler do the div and mod at compile time or get rid of
        // them.
        Value::LargestUInt maxIntegerValue =
            isNegative ? Value::LargestUInt(Value::maxLargestInt) + 1
            : Value::maxLargestUInt;
        Value::LargestUInt threshold = maxIntegerValue / 10;
        Value::LargestUInt value = 0;
        while (current < token.end_) {
            Char c = *current++;
            if (c < '0' || c > '9')
                return decodeDouble(token, decoded);
            auto digit(static_cast<Value::UInt>(c - '0'));
            if (value >= threshold) {
                // We've hit or exceeded the max value divided by 10 (rounded down). If
                // a) we've only just touched the limit, b) this is the last digit, and
                // c) it's small enough to fit in that rounding delta, we're okay.
                // Otherwise treat this number as a double to avoid overflow.
                if (value > threshold || current != token.end_ ||
                    digit > maxIntegerValue % 10) {
                    return decodeDouble(token, decoded);
                }
            }
            value = value * 10 + digit;
        }
        if (isNegative && value == maxIntegerValue)
            decoded = Value::minLargestInt;
        else if (isNegative)
            decoded = -Value::LargestInt(value);
        else if (value <= Value::LargestUInt(Value::maxInt))
            decoded = Value::LargestInt(value);
        else
            decoded = value;
        return true;
    }

    bool Reader::decodeDouble(Token& token) {
        Value decoded;
        if (!decodeDouble(token, decoded))
            return false;
        currentValue().swapPayload(decoded);
        currentValue().setOffsetStart(token.start_ - begin_);
        currentValue().setOffsetLimit(token.end_ - begin_);
        return true;
    }

    bool Reader::decodeDouble(Token& token, Value& decoded) {
        double value = 0;
        String buffer(token.start_, token.end_);
        IStringStream is(buffer);
        if (!(is >> value))
            return addError(
                "'" + String(token.start_, token.end_) + "' is not a number.", token);
        decoded = value;
        return true;
    }

    bool Reader::decodeString(Token& token) {
        String decoded_string;
        if (!decodeString(token, decoded_string))
            return false;
        Value decoded(decoded_string);
        currentValue().swapPayload(decoded);
        currentValue().setOffsetStart(token.start_ - begin_);
        currentValue().setOffsetLimit(token.end_ - begin_);
        return true;
    }

    bool Reader::decodeString(Token& token, String& decoded) {
        decoded.reserve(static_cast<size_t>(token.end_ - token.start_ - 2));
        Location current = token.start_ + 1; // skip '"'
        Location end = token.end_ - 1;       // do not include '"'
        while (current != end) {
            Char c = *current++;
            if (c == '"')
                break;
            if (c == '\\') {
                if (current == end)
                    return addError("Empty escape sequence in string", token, current);
                Char escape = *current++;
                switch (escape) {
                case '"':
                    decoded += '"';
                    break;
                case '/':
                    decoded += '/';
                    break;
                case '\\':
                    decoded += '\\';
                    break;
                case 'b':
                    decoded += '\b';
                    break;
                case 'f':
                    decoded += '\f';
                    break;
                case 'n':
                    decoded += '\n';
                    break;
                case 'r':
                    decoded += '\r';
                    break;
                case 't':
                    decoded += '\t';
                    break;
                case 'u': {
                    unsigned int unicode;
                    if (!decodeUnicodeCodePoint(token, current, end, unicode))
                        return false;
                    decoded += codePointToUTF8(unicode);
                } break;
                default:
                    return addError("Bad escape sequence in string", token, current);
                }
            } else {
                decoded += c;
            }
        }
        return true;
    }

    bool Reader::decodeUnicodeCodePoint(Token& token, Location& current,
        Location end, unsigned int& unicode) {

        if (!decodeUnicodeEscapeSequence(token, current, end, unicode))
            return false;
        if (unicode >= 0xD800 && unicode <= 0xDBFF) {
            // surrogate pairs
            if (end - current < 6)
                return addError(
                    "additional six characters expected to parse unicode surrogate pair.",
                    token, current);
            if (*(current++) == '\\' && *(current++) == 'u') {
                unsigned int surrogatePair;
                if (decodeUnicodeEscapeSequence(token, current, end, surrogatePair)) {
                    unicode = 0x10000 + ((unicode & 0x3FF) << 10) + (surrogatePair & 0x3FF);
                } else
                    return false;
            } else
                return addError("expecting another \\u token to begin the second half of "
                    "a unicode surrogate pair",
                    token, current);
        }
        return true;
    }

    bool Reader::decodeUnicodeEscapeSequence(Token& token, Location& current,
        Location end,
        unsigned int& ret_unicode) {
        if (end - current < 4)
            return addError(
                "Bad unicode escape sequence in string: four digits expected.", token,
                current);
        int unicode = 0;
        for (int index = 0; index < 4; ++index) {
            Char c = *current++;
            unicode *= 16;
            if (c >= '0' && c <= '9')
                unicode += c - '0';
            else if (c >= 'a' && c <= 'f')
                unicode += c - 'a' + 10;
            else if (c >= 'A' && c <= 'F')
                unicode += c - 'A' + 10;
            else
                return addError(
                    "Bad unicode escape sequence in string: hexadecimal digit expected.",
                    token, current);
        }
        ret_unicode = static_cast<unsigned int>(unicode);
        return true;
    }

    bool Reader::addError(const String& message, Token& token, Location extra) {
        ErrorInfo info;
        info.token_ = token;
        info.message_ = message;
        info.extra_ = extra;
        errors_.push_back(info);
        return false;
    }

    bool Reader::recoverFromError(TokenType skipUntilToken) {
        size_t const errorCount = errors_.size();
        Token skip;
        for (;;) {
            if (!readToken(skip))
                errors_.resize(errorCount); // discard errors caused by recovery
            if (skip.type_ == skipUntilToken || skip.type_ == tokenEndOfStream)
                break;
        }
        errors_.resize(errorCount);
        return false;
    }

    bool Reader::addErrorAndRecover(const String& message, Token& token,
        TokenType skipUntilToken) {
        addError(message, token);
        return recoverFromError(skipUntilToken);
    }

    Value& Reader::currentValue() { return *(nodes_.top()); }

    Reader::Char Reader::getNextChar() {
        if (current_ == end_)
            return 0;
        return *current_++;
    }

    void Reader::getLocationLineAndColumn(Location location, int& line,
        int& column) const {
        Location current = begin_;
        Location lastLineStart = current;
        line = 0;
        while (current < location && current != end_) {
            Char c = *current++;
            if (c == '\r') {
                if (*current == '\n')
                    ++current;
                lastLineStart = current;
                ++line;
            } else if (c == '\n') {
                lastLineStart = current;
                ++line;
            }
        }
        // column & line start at 1
        column = int(location - lastLineStart) + 1;
        ++line;
    }

    String Reader::getLocationLineAndColumn(Location location) const {
        int line, column;
        getLocationLineAndColumn(location, line, column);
        char buffer[18 + 16 + 16 + 1];
        jsoncpp_snprintf(buffer, sizeof(buffer), "Line %d, Column %d", line, column);
        return buffer;
    }

    // Deprecated. Preserved for backward compatibility
    String Reader::getFormatedErrorMessages() const {
        return getFormattedErrorMessages();
    }

    String Reader::getFormattedErrorMessages() const {
        String formattedMessage;
        for (const auto& error : errors_) {
            formattedMessage +=
                "* " + getLocationLineAndColumn(error.token_.start_) + "\n";
            formattedMessage += "  " + error.message_ + "\n";
            if (error.extra_)
                formattedMessage +=
                "See " + getLocationLineAndColumn(error.extra_) + " for detail.\n";
        }
        return formattedMessage;
    }

    std::vector<Reader::StructuredError> Reader::getStructuredErrors() const {
        std::vector<Reader::StructuredError> allErrors;
        for (const auto& error : errors_) {
            Reader::StructuredError structured;
            structured.offset_start = error.token_.start_ - begin_;
            structured.offset_limit = error.token_.end_ - begin_;
            structured.message = error.message_;
            allErrors.push_back(structured);
        }
        return allErrors;
    }

    bool Reader::pushError(const Value& value, const String& message) {
        ptrdiff_t const length = end_ - begin_;
        if (value.getOffsetStart() > length || value.getOffsetLimit() > length)
            return false;
        Token token;
        token.type_ = tokenError;
        token.start_ = begin_ + value.getOffsetStart();
        token.end_ = begin_ + value.getOffsetLimit();
        ErrorInfo info;
        info.token_ = token;
        info.message_ = message;
        info.extra_ = nullptr;
        errors_.push_back(info);
        return true;
    }

    bool Reader::pushError(const Value& value, const String& message,
        const Value& extra) {
        ptrdiff_t const length = end_ - begin_;
        if (value.getOffsetStart() > length || value.getOffsetLimit() > length ||
            extra.getOffsetLimit() > length)
            return false;
        Token token;
        token.type_ = tokenError;
        token.start_ = begin_ + value.getOffsetStart();
        token.end_ = begin_ + value.getOffsetLimit();
        ErrorInfo info;
        info.token_ = token;
        info.message_ = message;
        info.extra_ = begin_ + extra.getOffsetStart();
        errors_.push_back(info);
        return true;
    }

    bool Reader::good() const { return errors_.empty(); }

    // Originally copied from the Features class (now deprecated), used internally
    // for features implementation.
    class OurFeatures {
    public:
        static OurFeatures all();
        bool allowComments_;
        bool allowTrailingCommas_;
        bool strictRoot_;
        bool allowDroppedNullPlaceholders_;
        bool allowNumericKeys_;
        bool allowSingleQuotes_;
        bool failIfExtra_;
        bool rejectDupKeys_;
        bool allowSpecialFloats_;
        size_t stackLimit_;
    }; // OurFeatures

    OurFeatures OurFeatures::all() { return {}; }

    // Implementation of class Reader
    // ////////////////////////////////

    // Originally copied from the Reader class (now deprecated), used internally
    // for implementing JSON reading.
    class OurReader {
    public:
        using Char = char;
        using Location = const Char*;
        struct StructuredError {
            ptrdiff_t offset_start;
            ptrdiff_t offset_limit;
            String message;
        };

        explicit OurReader(OurFeatures const& features);
        bool parse(const char* beginDoc, const char* endDoc, Value& root,
            bool collectComments = true);
        String getFormattedErrorMessages() const;
        std::vector<StructuredError> getStructuredErrors() const;

    private:
        OurReader(OurReader const&);      // no impl
        void operator=(OurReader const&); // no impl

        enum TokenType {
            tokenEndOfStream = 0,
            tokenObjectBegin,
            tokenObjectEnd,
            tokenArrayBegin,
            tokenArrayEnd,
            tokenString,
            tokenNumber,
            tokenTrue,
            tokenFalse,
            tokenNull,
            tokenNaN,
            tokenPosInf,
            tokenNegInf,
            tokenArraySeparator,
            tokenMemberSeparator,
            tokenComment,
            tokenError
        };

        class Token {
        public:
            TokenType type_;
            Location start_;
            Location end_;
        };

        class ErrorInfo {
        public:
            Token token_;
            String message_;
            Location extra_;
        };

        using Errors = std::deque<ErrorInfo>;

        bool readToken(Token& token);
        void skipSpaces();
        bool match(const Char* pattern, int patternLength);
        bool readComment();
        bool readCStyleComment(bool* containsNewLineResult);
        bool readCppStyleComment();
        bool readString();
        bool readStringSingleQuote();
        bool readNumber(bool checkInf);
        bool readValue();
        bool readObject(Token& token);
        bool readArray(Token& token);
        bool decodeNumber(Token& token);
        bool decodeNumber(Token& token, Value& decoded);
        bool decodeString(Token& token);
        bool decodeString(Token& token, String& decoded);
        bool decodeDouble(Token& token);
        bool decodeDouble(Token& token, Value& decoded);
        bool decodeUnicodeCodePoint(Token& token, Location& current, Location end,
            unsigned int& unicode);
        bool decodeUnicodeEscapeSequence(Token& token, Location& current,
            Location end, unsigned int& unicode);
        bool addError(const String& message, Token& token, Location extra = nullptr);
        bool recoverFromError(TokenType skipUntilToken);
        bool addErrorAndRecover(const String& message, Token& token,
            TokenType skipUntilToken);
        void skipUntilSpace();
        Value& currentValue();
        Char getNextChar();
        void getLocationLineAndColumn(Location location, int& line,
            int& column) const;
        String getLocationLineAndColumn(Location location) const;
        void addComment(Location begin, Location end, CommentPlacement placement);
        void skipCommentTokens(Token& token);

        static String normalizeEOL(Location begin, Location end);
        static bool containsNewLine(Location begin, Location end);

        using Nodes = std::stack<Value*>;

        Nodes nodes_{};
        Errors errors_{};
        String document_{};
        Location begin_ = nullptr;
        Location end_ = nullptr;
        Location current_ = nullptr;
        Location lastValueEnd_ = nullptr;
        Value* lastValue_ = nullptr;
        bool lastValueHasAComment_ = false;
        String commentsBefore_{};

        OurFeatures const features_;
        bool collectComments_ = false;
    }; // OurReader

    // complete copy of Read impl, for OurReader

    bool OurReader::containsNewLine(OurReader::Location begin,
        OurReader::Location end) {
        for (; begin < end; ++begin)
            if (*begin == '\n' || *begin == '\r')
                return true;
        return false;
    }

    OurReader::OurReader(OurFeatures const& features) : features_(features) {}

    bool OurReader::parse(const char* beginDoc, const char* endDoc, Value& root,
        bool collectComments) {
        if (!features_.allowComments_) {
            collectComments = false;
        }

        begin_ = beginDoc;
        end_ = endDoc;
        collectComments_ = collectComments;
        current_ = begin_;
        lastValueEnd_ = nullptr;
        lastValue_ = nullptr;
        commentsBefore_.clear();
        errors_.clear();
        while (!nodes_.empty())
            nodes_.pop();
        nodes_.push(&root);

        bool successful = readValue();
        nodes_.pop();
        Token token;
        skipCommentTokens(token);
        if (features_.failIfExtra_ && (token.type_ != tokenEndOfStream)) {
            addError("Extra non-whitespace after JSON value.", token);
            return false;
        }
        if (collectComments_ && !commentsBefore_.empty())
            root.setComment(commentsBefore_, commentAfter);
        if (features_.strictRoot_) {
            if (!root.isArray() && !root.isObject()) {
                // Set error location to start of doc, ideally should be first token found
                // in doc
                token.type_ = tokenError;
                token.start_ = beginDoc;
                token.end_ = endDoc;
                addError(
                    "A valid JSON document must be either an array or an object value.",
                    token);
                return false;
            }
        }
        return successful;
    }

    bool OurReader::readValue() {
        //  To preserve the old behaviour we cast size_t to int.
        if (nodes_.size() > features_.stackLimit_)
            throwRuntimeError("Exceeded stackLimit in readValue().");
        Token token;
        skipCommentTokens(token);
        bool successful = true;

        if (collectComments_ && !commentsBefore_.empty()) {
            currentValue().setComment(commentsBefore_, commentBefore);
            commentsBefore_.clear();
        }

        switch (token.type_) {
        case tokenObjectBegin:
            successful = readObject(token);
            currentValue().setOffsetLimit(current_ - begin_);
            break;
        case tokenArrayBegin:
            successful = readArray(token);
            currentValue().setOffsetLimit(current_ - begin_);
            break;
        case tokenNumber:
            successful = decodeNumber(token);
            break;
        case tokenString:
            successful = decodeString(token);
            break;
        case tokenTrue: {
            Value v(true);
            currentValue().swapPayload(v);
            currentValue().setOffsetStart(token.start_ - begin_);
            currentValue().setOffsetLimit(token.end_ - begin_);
        } break;
        case tokenFalse: {
            Value v(false);
            currentValue().swapPayload(v);
            currentValue().setOffsetStart(token.start_ - begin_);
            currentValue().setOffsetLimit(token.end_ - begin_);
        } break;
        case tokenNull: {
            Value v;
            currentValue().swapPayload(v);
            currentValue().setOffsetStart(token.start_ - begin_);
            currentValue().setOffsetLimit(token.end_ - begin_);
        } break;
        case tokenNaN: {
            Value v(std::numeric_limits<double>::quiet_NaN());
            currentValue().swapPayload(v);
            currentValue().setOffsetStart(token.start_ - begin_);
            currentValue().setOffsetLimit(token.end_ - begin_);
        } break;
        case tokenPosInf: {
            Value v(std::numeric_limits<double>::infinity());
            currentValue().swapPayload(v);
            currentValue().setOffsetStart(token.start_ - begin_);
            currentValue().setOffsetLimit(token.end_ - begin_);
        } break;
        case tokenNegInf: {
            Value v(-std::numeric_limits<double>::infinity());
            currentValue().swapPayload(v);
            currentValue().setOffsetStart(token.start_ - begin_);
            currentValue().setOffsetLimit(token.end_ - begin_);
        } break;
        case tokenArraySeparator:
        case tokenObjectEnd:
        case tokenArrayEnd:
            if (features_.allowDroppedNullPlaceholders_) {
                // "Un-read" the current token and mark the current value as a null
                // token.
                current_--;
                Value v;
                currentValue().swapPayload(v);
                currentValue().setOffsetStart(current_ - begin_ - 1);
                currentValue().setOffsetLimit(current_ - begin_);
                break;
            } // else, fall through ...
        default:
            currentValue().setOffsetStart(token.start_ - begin_);
            currentValue().setOffsetLimit(token.end_ - begin_);
            return addError("Syntax error: value, object or array expected.", token);
        }

        if (collectComments_) {
            lastValueEnd_ = current_;
            lastValueHasAComment_ = false;
            lastValue_ = &currentValue();
        }

        return successful;
    }

    void OurReader::skipCommentTokens(Token& token) {
        if (features_.allowComments_) {
            do {
                readToken(token);
            } while (token.type_ == tokenComment);
        } else {
            readToken(token);
        }
    }

    bool OurReader::readToken(Token& token) {
        skipSpaces();
        token.start_ = current_;
        Char c = getNextChar();
        bool ok = true;
        switch (c) {
        case '{':
            token.type_ = tokenObjectBegin;
            break;
        case '}':
            token.type_ = tokenObjectEnd;
            break;
        case '[':
            token.type_ = tokenArrayBegin;
            break;
        case ']':
            token.type_ = tokenArrayEnd;
            break;
        case '"':
            token.type_ = tokenString;
            ok = readString();
            break;
        case '\'':
            if (features_.allowSingleQuotes_) {
                token.type_ = tokenString;
                ok = readStringSingleQuote();
                break;
            } // else fall through
        case '/':
            token.type_ = tokenComment;
            ok = readComment();
            break;
        case '0':
        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7':
        case '8':
        case '9':
            token.type_ = tokenNumber;
            readNumber(false);
            break;
        case '-':
            if (readNumber(true)) {
                token.type_ = tokenNumber;
            } else {
                token.type_ = tokenNegInf;
                ok = features_.allowSpecialFloats_ && match("nfinity", 7);
            }
            break;
        case '+':
            if (readNumber(true)) {
                token.type_ = tokenNumber;
            } else {
                token.type_ = tokenPosInf;
                ok = features_.allowSpecialFloats_ && match("nfinity", 7);
            }
            break;
        case 't':
            token.type_ = tokenTrue;
            ok = match("rue", 3);
            break;
        case 'f':
            token.type_ = tokenFalse;
            ok = match("alse", 4);
            break;
        case 'n':
            token.type_ = tokenNull;
            ok = match("ull", 3);
            break;
        case 'N':
            if (features_.allowSpecialFloats_) {
                token.type_ = tokenNaN;
                ok = match("aN", 2);
            } else {
                ok = false;
            }
            break;
        case 'I':
            if (features_.allowSpecialFloats_) {
                token.type_ = tokenPosInf;
                ok = match("nfinity", 7);
            } else {
                ok = false;
            }
            break;
        case ',':
            token.type_ = tokenArraySeparator;
            break;
        case ':':
            token.type_ = tokenMemberSeparator;
            break;
        case 0:
            token.type_ = tokenEndOfStream;
            break;
        default:
            ok = false;
            break;
        }
        if (!ok)
            token.type_ = tokenError;
        token.end_ = current_;
        return ok;
    }

    void OurReader::skipSpaces() {
        while (current_ != end_) {
            Char c = *current_;
            if (c == ' ' || c == '\t' || c == '\r' || c == '\n')
                ++current_;
            else
                break;
        }
    }

    bool OurReader::match(const Char* pattern, int patternLength) {
        if (end_ - current_ < patternLength)
            return false;
        int index = patternLength;
        while (index--)
            if (current_[index] != pattern[index])
                return false;
        current_ += patternLength;
        return true;
    }

    bool OurReader::readComment() {
        const Location commentBegin = current_ - 1;
        const Char c = getNextChar();
        bool successful = false;
        bool cStyleWithEmbeddedNewline = false;

        const bool isCStyleComment = (c == '*');
        const bool isCppStyleComment = (c == '/');
        if (isCStyleComment) {
            successful = readCStyleComment(&cStyleWithEmbeddedNewline);
        } else if (isCppStyleComment) {
            successful = readCppStyleComment();
        }

        if (!successful)
            return false;

        if (collectComments_) {
            CommentPlacement placement = commentBefore;

            if (!lastValueHasAComment_) {
                if (lastValueEnd_ && !containsNewLine(lastValueEnd_, commentBegin)) {
                    if (isCppStyleComment || !cStyleWithEmbeddedNewline) {
                        placement = commentAfterOnSameLine;
                        lastValueHasAComment_ = true;
                    }
                }
            }

            addComment(commentBegin, current_, placement);
        }
        return true;
    }

    String OurReader::normalizeEOL(OurReader::Location begin,
        OurReader::Location end) {
        String normalized;
        normalized.reserve(static_cast<size_t>(end - begin));
        OurReader::Location current = begin;
        while (current != end) {
            char c = *current++;
            if (c == '\r') {
                if (current != end && *current == '\n')
                    // convert dos EOL
                    ++current;
                // convert Mac EOL
                normalized += '\n';
            } else {
                normalized += c;
            }
        }
        return normalized;
    }

    void OurReader::addComment(Location begin, Location end,
        CommentPlacement placement) {
        assert(collectComments_);
        const String& normalized = normalizeEOL(begin, end);
        if (placement == commentAfterOnSameLine) {
            assert(lastValue_ != nullptr);
            lastValue_->setComment(normalized, placement);
        } else {
            commentsBefore_ += normalized;
        }
    }

    bool OurReader::readCStyleComment(bool* containsNewLineResult) {
        *containsNewLineResult = false;

        while ((current_ + 1) < end_) {
            Char c = getNextChar();
            if (c == '*' && *current_ == '/')
                break;
            if (c == '\n')
                *containsNewLineResult = true;
        }

        return getNextChar() == '/';
    }

    bool OurReader::readCppStyleComment() {
        while (current_ != end_) {
            Char c = getNextChar();
            if (c == '\n')
                break;
            if (c == '\r') {
                // Consume DOS EOL. It will be normalized in addComment.
                if (current_ != end_ && *current_ == '\n')
                    getNextChar();
                // Break on Moc OS 9 EOL.
                break;
            }
        }
        return true;
    }

    bool OurReader::readNumber(bool checkInf) {
        Location p = current_;
        if (checkInf && p != end_ && *p == 'I') {
            current_ = ++p;
            return false;
        }
        char c = '0'; // stopgap for already consumed character
        // integral part
        while (c >= '0' && c <= '9')
            c = (current_ = p) < end_ ? *p++ : '\0';
        // fractional part
        if (c == '.') {
            c = (current_ = p) < end_ ? *p++ : '\0';
            while (c >= '0' && c <= '9')
                c = (current_ = p) < end_ ? *p++ : '\0';
        }
        // exponential part
        if (c == 'e' || c == 'E') {
            c = (current_ = p) < end_ ? *p++ : '\0';
            if (c == '+' || c == '-')
                c = (current_ = p) < end_ ? *p++ : '\0';
            while (c >= '0' && c <= '9')
                c = (current_ = p) < end_ ? *p++ : '\0';
        }
        return true;
    }
    bool OurReader::readString() {
        Char c = 0;
        while (current_ != end_) {
            c = getNextChar();
            if (c == '\\')
                getNextChar();
            else if (c == '"')
                break;
        }
        return c == '"';
    }

    bool OurReader::readStringSingleQuote() {
        Char c = 0;
        while (current_ != end_) {
            c = getNextChar();
            if (c == '\\')
                getNextChar();
            else if (c == '\'')
                break;
        }
        return c == '\'';
    }

    bool OurReader::readObject(Token& token) {
        Token tokenName;
        String name;
        Value init(objectValue);
        currentValue().swapPayload(init);
        currentValue().setOffsetStart(token.start_ - begin_);
        while (readToken(tokenName)) {
            bool initialTokenOk = true;
            while (tokenName.type_ == tokenComment && initialTokenOk)
                initialTokenOk = readToken(tokenName);
            if (!initialTokenOk)
                break;
            if (tokenName.type_ == tokenObjectEnd &&
                (name.empty() ||
                    features_.allowTrailingCommas_)) // empty object or trailing comma
                return true;
            name.clear();
            if (tokenName.type_ == tokenString) {
                if (!decodeString(tokenName, name))
                    return recoverFromError(tokenObjectEnd);
            } else if (tokenName.type_ == tokenNumber && features_.allowNumericKeys_) {
                Value numberName;
                if (!decodeNumber(tokenName, numberName))
                    return recoverFromError(tokenObjectEnd);
                name = numberName.asString();
            } else {
                break;
            }
            if (name.length() >= (1U << 30))
                throwRuntimeError("keylength >= 2^30");
            if (features_.rejectDupKeys_ && currentValue().isMember(name)) {
                String msg = "Duplicate key: '" + name + "'";
                return addErrorAndRecover(msg, tokenName, tokenObjectEnd);
            }

            Token colon;
            if (!readToken(colon) || colon.type_ != tokenMemberSeparator) {
                return addErrorAndRecover("Missing ':' after object member name", colon,
                    tokenObjectEnd);
            }
            Value& value = currentValue()[name];
            nodes_.push(&value);
            bool ok = readValue();
            nodes_.pop();
            if (!ok) // error already set
                return recoverFromError(tokenObjectEnd);

            Token comma;
            if (!readToken(comma) ||
                (comma.type_ != tokenObjectEnd && comma.type_ != tokenArraySeparator &&
                    comma.type_ != tokenComment)) {
                return addErrorAndRecover("Missing ',' or '}' in object declaration",
                    comma, tokenObjectEnd);
            }
            bool finalizeTokenOk = true;
            while (comma.type_ == tokenComment && finalizeTokenOk)
                finalizeTokenOk = readToken(comma);
            if (comma.type_ == tokenObjectEnd)
                return true;
        }
        return addErrorAndRecover("Missing '}' or object member name", tokenName,
            tokenObjectEnd);
    }

    bool OurReader::readArray(Token& token) {
        Value init(arrayValue);
        currentValue().swapPayload(init);
        currentValue().setOffsetStart(token.start_ - begin_);
        int index = 0;
        for (;;) {
            skipSpaces();
            if (current_ != end_ && *current_ == ']' &&
                (index == 0 ||
                (features_.allowTrailingCommas_ &&
                    !features_.allowDroppedNullPlaceholders_))) // empty array or trailing
                                                                // comma
            {
                Token endArray;
                readToken(endArray);
                return true;
            }
            Value& value = currentValue()[index++];
            nodes_.push(&value);
            bool ok = readValue();
            nodes_.pop();
            if (!ok) // error already set
                return recoverFromError(tokenArrayEnd);

            Token currentToken;
            // Accept Comment after last item in the array.
            ok = readToken(currentToken);
            while (currentToken.type_ == tokenComment && ok) {
                ok = readToken(currentToken);
            }
            bool badTokenType = (currentToken.type_ != tokenArraySeparator &&
                currentToken.type_ != tokenArrayEnd);
            if (!ok || badTokenType) {
                return addErrorAndRecover("Missing ',' or ']' in array declaration",
                    currentToken, tokenArrayEnd);
            }
            if (currentToken.type_ == tokenArrayEnd)
                break;
        }
        return true;
    }

    bool OurReader::decodeNumber(Token& token) {
        Value decoded;
        if (!decodeNumber(token, decoded))
            return false;
        currentValue().swapPayload(decoded);
        currentValue().setOffsetStart(token.start_ - begin_);
        currentValue().setOffsetLimit(token.end_ - begin_);
        return true;
    }

    bool OurReader::decodeNumber(Token& token, Value& decoded) {
        // Attempts to parse the number as an integer. If the number is
        // larger than the maximum supported value of an integer then
        // we decode the number as a double.
        Location current = token.start_;
        const bool isNegative = *current == '-';
        if (isNegative) {
            ++current;
        }

        // We assume we can represent the largest and smallest integer types as
        // unsigned integers with separate sign. This is only true if they can fit
        // into an unsigned integer.
        static_assert(Value::maxLargestInt <= Value::maxLargestUInt,
            "Int must be smaller than UInt");

        // We need to convert minLargestInt into a positive number. The easiest way
        // to do this conversion is to assume our "threshold" value of minLargestInt
        // divided by 10 can fit in maxLargestInt when absolute valued. This should
        // be a safe assumption.
        static_assert(Value::minLargestInt <= -Value::maxLargestInt,
            "The absolute value of minLargestInt must be greater than or "
            "equal to maxLargestInt");
        static_assert(Value::minLargestInt / 10 >= -Value::maxLargestInt,
            "The absolute value of minLargestInt must be only 1 magnitude "
            "larger than maxLargest Int");

        static constexpr Value::LargestUInt positive_threshold =
            Value::maxLargestUInt / 10;
        static constexpr Value::UInt positive_last_digit = Value::maxLargestUInt % 10;

        // For the negative values, we have to be more careful. Since typically
        // -Value::minLargestInt will cause an overflow, we first divide by 10 and
        // then take the inverse. This assumes that minLargestInt is only a single
        // power of 10 different in magnitude, which we check above. For the last
        // digit, we take the modulus before negating for the same reason.
        static constexpr auto negative_threshold =
            Value::LargestUInt(-(Value::minLargestInt / 10));
        static constexpr auto negative_last_digit =
            Value::UInt(-(Value::minLargestInt % 10));

        const Value::LargestUInt threshold =
            isNegative ? negative_threshold : positive_threshold;
        const Value::UInt max_last_digit =
            isNegative ? negative_last_digit : positive_last_digit;

        Value::LargestUInt value = 0;
        while (current < token.end_) {
            Char c = *current++;
            if (c < '0' || c > '9')
                return decodeDouble(token, decoded);

            const auto digit(static_cast<Value::UInt>(c - '0'));
            if (value >= threshold) {
                // We've hit or exceeded the max value divided by 10 (rounded down). If
                // a) we've only just touched the limit, meaing value == threshold,
                // b) this is the last digit, or
                // c) it's small enough to fit in that rounding delta, we're okay.
                // Otherwise treat this number as a double to avoid overflow.
                if (value > threshold || current != token.end_ ||
                    digit > max_last_digit) {
                    return decodeDouble(token, decoded);
                }
            }
            value = value * 10 + digit;
        }

        if (isNegative) {
            // We use the same magnitude assumption here, just in case.
            const auto last_digit = static_cast<Value::UInt>(value % 10);
            decoded = -Value::LargestInt(value / 10) * 10 - last_digit;
        } else if (value <= Value::LargestUInt(Value::maxLargestInt)) {
            decoded = Value::LargestInt(value);
        } else {
            decoded = value;
        }

        return true;
    }

    bool OurReader::decodeDouble(Token& token) {
        Value decoded;
        if (!decodeDouble(token, decoded))
            return false;
        currentValue().swapPayload(decoded);
        currentValue().setOffsetStart(token.start_ - begin_);
        currentValue().setOffsetLimit(token.end_ - begin_);
        return true;
    }

    bool OurReader::decodeDouble(Token& token, Value& decoded) {
        double value = 0;
        const String buffer(token.start_, token.end_);
        IStringStream is(buffer);
        if (!(is >> value)) {
            return addError(
                "'" + String(token.start_, token.end_) + "' is not a number.", token);
        }
        decoded = value;
        return true;
    }

    bool OurReader::decodeString(Token& token) {
        String decoded_string;
        if (!decodeString(token, decoded_string))
            return false;
        Value decoded(decoded_string);
        currentValue().swapPayload(decoded);
        currentValue().setOffsetStart(token.start_ - begin_);
        currentValue().setOffsetLimit(token.end_ - begin_);
        return true;
    }

    bool OurReader::decodeString(Token& token, String& decoded) {
        decoded.reserve(static_cast<size_t>(token.end_ - token.start_ - 2));
        Location current = token.start_ + 1; // skip '"'
        Location end = token.end_ - 1;       // do not include '"'
        while (current != end) {
            Char c = *current++;
            if (c == '"')
                break;
            if (c == '\\') {
                if (current == end)
                    return addError("Empty escape sequence in string", token, current);
                Char escape = *current++;
                switch (escape) {
                case '"':
                    decoded += '"';
                    break;
                case '/':
                    decoded += '/';
                    break;
                case '\\':
                    decoded += '\\';
                    break;
                case 'b':
                    decoded += '\b';
                    break;
                case 'f':
                    decoded += '\f';
                    break;
                case 'n':
                    decoded += '\n';
                    break;
                case 'r':
                    decoded += '\r';
                    break;
                case 't':
                    decoded += '\t';
                    break;
                case 'u': {
                    unsigned int unicode;
                    if (!decodeUnicodeCodePoint(token, current, end, unicode))
                        return false;
                    decoded += codePointToUTF8(unicode);
                } break;
                default:
                    return addError("Bad escape sequence in string", token, current);
                }
            } else {
                decoded += c;
            }
        }
        return true;
    }

    bool OurReader::decodeUnicodeCodePoint(Token& token, Location& current,
        Location end, unsigned int& unicode) {

        if (!decodeUnicodeEscapeSequence(token, current, end, unicode))
            return false;
        if (unicode >= 0xD800 && unicode <= 0xDBFF) {
            // surrogate pairs
            if (end - current < 6)
                return addError(
                    "additional six characters expected to parse unicode surrogate pair.",
                    token, current);
            if (*(current++) == '\\' && *(current++) == 'u') {
                unsigned int surrogatePair;
                if (decodeUnicodeEscapeSequence(token, current, end, surrogatePair)) {
                    unicode = 0x10000 + ((unicode & 0x3FF) << 10) + (surrogatePair & 0x3FF);
                } else
                    return false;
            } else
                return addError("expecting another \\u token to begin the second half of "
                    "a unicode surrogate pair",
                    token, current);
        }
        return true;
    }

    bool OurReader::decodeUnicodeEscapeSequence(Token& token, Location& current,
        Location end,
        unsigned int& ret_unicode) {
        if (end - current < 4)
            return addError(
                "Bad unicode escape sequence in string: four digits expected.", token,
                current);
        int unicode = 0;
        for (int index = 0; index < 4; ++index) {
            Char c = *current++;
            unicode *= 16;
            if (c >= '0' && c <= '9')
                unicode += c - '0';
            else if (c >= 'a' && c <= 'f')
                unicode += c - 'a' + 10;
            else if (c >= 'A' && c <= 'F')
                unicode += c - 'A' + 10;
            else
                return addError(
                    "Bad unicode escape sequence in string: hexadecimal digit expected.",
                    token, current);
        }
        ret_unicode = static_cast<unsigned int>(unicode);
        return true;
    }

    bool OurReader::addError(const String& message, Token& token, Location extra) {
        ErrorInfo info;
        info.token_ = token;
        info.message_ = message;
        info.extra_ = extra;
        errors_.push_back(info);
        return false;
    }

    bool OurReader::recoverFromError(TokenType skipUntilToken) {
        size_t errorCount = errors_.size();
        Token skip;
        for (;;) {
            if (!readToken(skip))
                errors_.resize(errorCount); // discard errors caused by recovery
            if (skip.type_ == skipUntilToken || skip.type_ == tokenEndOfStream)
                break;
        }
        errors_.resize(errorCount);
        return false;
    }

    bool OurReader::addErrorAndRecover(const String& message, Token& token,
        TokenType skipUntilToken) {
        addError(message, token);
        return recoverFromError(skipUntilToken);
    }

    Value& OurReader::currentValue() { return *(nodes_.top()); }

    OurReader::Char OurReader::getNextChar() {
        if (current_ == end_)
            return 0;
        return *current_++;
    }

    void OurReader::getLocationLineAndColumn(Location location, int& line,
        int& column) const {
        Location current = begin_;
        Location lastLineStart = current;
        line = 0;
        while (current < location && current != end_) {
            Char c = *current++;
            if (c == '\r') {
                if (*current == '\n')
                    ++current;
                lastLineStart = current;
                ++line;
            } else if (c == '\n') {
                lastLineStart = current;
                ++line;
            }
        }
        // column & line start at 1
        column = int(location - lastLineStart) + 1;
        ++line;
    }

    String OurReader::getLocationLineAndColumn(Location location) const {
        int line, column;
        getLocationLineAndColumn(location, line, column);
        char buffer[18 + 16 + 16 + 1];
        jsoncpp_snprintf(buffer, sizeof(buffer), "Line %d, Column %d", line, column);
        return buffer;
    }

    String OurReader::getFormattedErrorMessages() const {
        String formattedMessage;
        for (const auto& error : errors_) {
            formattedMessage +=
                "* " + getLocationLineAndColumn(error.token_.start_) + "\n";
            formattedMessage += "  " + error.message_ + "\n";
            if (error.extra_)
                formattedMessage +=
                "See " + getLocationLineAndColumn(error.extra_) + " for detail.\n";
        }
        return formattedMessage;
    }

    std::vector<OurReader::StructuredError> OurReader::getStructuredErrors() const {
        std::vector<OurReader::StructuredError> allErrors;
        for (const auto& error : errors_) {
            OurReader::StructuredError structured;
            structured.offset_start = error.token_.start_ - begin_;
            structured.offset_limit = error.token_.end_ - begin_;
            structured.message = error.message_;
            allErrors.push_back(structured);
        }
        return allErrors;
    }

    class OurCharReader : public CharReader {
        bool const collectComments_;
        OurReader reader_;

    public:
        OurCharReader(bool collectComments, OurFeatures const& features)
            : collectComments_(collectComments), reader_(features) {}
        bool parse(char const* beginDoc, char const* endDoc, Value* root,
            String* errs) override {
            bool ok = reader_.parse(beginDoc, endDoc, *root, collectComments_);
            if (errs) {
                *errs = reader_.getFormattedErrorMessages();
            }
            return ok;
        }
    };

    CharReaderBuilder::CharReaderBuilder() { setDefaults(&settings_); }
    CharReaderBuilder::~CharReaderBuilder() = default;
    CharReader* CharReaderBuilder::newCharReader() const {
        bool collectComments = settings_["collectComments"].asBool();
        OurFeatures features = OurFeatures::all();
        features.allowComments_ = settings_["allowComments"].asBool();
        features.allowTrailingCommas_ = settings_["allowTrailingCommas"].asBool();
        features.strictRoot_ = settings_["strictRoot"].asBool();
        features.allowDroppedNullPlaceholders_ =
            settings_["allowDroppedNullPlaceholders"].asBool();
        features.allowNumericKeys_ = settings_["allowNumericKeys"].asBool();
        features.allowSingleQuotes_ = settings_["allowSingleQuotes"].asBool();

        // Stack limit is always a size_t, so we get this as an unsigned int
        // regardless of it we have 64-bit integer support enabled.
        features.stackLimit_ = static_cast<size_t>(settings_["stackLimit"].asUInt());
        features.failIfExtra_ = settings_["failIfExtra"].asBool();
        features.rejectDupKeys_ = settings_["rejectDupKeys"].asBool();
        features.allowSpecialFloats_ = settings_["allowSpecialFloats"].asBool();
        return new OurCharReader(collectComments, features);
    }
    static void getValidReaderKeys(std::set<String>* valid_keys) {
        valid_keys->clear();
        valid_keys->insert("collectComments");
        valid_keys->insert("allowComments");
        valid_keys->insert("allowTrailingCommas");
        valid_keys->insert("strictRoot");
        valid_keys->insert("allowDroppedNullPlaceholders");
        valid_keys->insert("allowNumericKeys");
        valid_keys->insert("allowSingleQuotes");
        valid_keys->insert("stackLimit");
        valid_keys->insert("failIfExtra");
        valid_keys->insert("rejectDupKeys");
        valid_keys->insert("allowSpecialFloats");
    }
    bool CharReaderBuilder::validate(Json::Value* invalid) const {
        Json::Value my_invalid;
        if (!invalid)
            invalid = &my_invalid; // so we do not need to test for NULL
        Json::Value& inv = *invalid;
        std::set<String> valid_keys;
        getValidReaderKeys(&valid_keys);
        Value::Members keys = settings_.getMemberNames();
        size_t n = keys.size();
        for (size_t i = 0; i < n; ++i) {
            String const& key = keys[i];
            if (valid_keys.find(key) == valid_keys.end()) {
                inv[key] = settings_[key];
            }
        }
        return inv.empty();
    }
    Value& CharReaderBuilder::operator[](const String& key) {
        return settings_[key];
    }
    // static
    void CharReaderBuilder::strictMode(Json::Value* settings) {
        //! [CharReaderBuilderStrictMode]
        (*settings)["allowComments"] = false;
        (*settings)["allowTrailingCommas"] = false;
        (*settings)["strictRoot"] = true;
        (*settings)["allowDroppedNullPlaceholders"] = false;
        (*settings)["allowNumericKeys"] = false;
        (*settings)["allowSingleQuotes"] = false;
        (*settings)["stackLimit"] = 1000;
        (*settings)["failIfExtra"] = true;
        (*settings)["rejectDupKeys"] = true;
        (*settings)["allowSpecialFloats"] = false;
        //! [CharReaderBuilderStrictMode]
    }
    // static
    void CharReaderBuilder::setDefaults(Json::Value* settings) {
        //! [CharReaderBuilderDefaults]
        (*settings)["collectComments"] = true;
        (*settings)["allowComments"] = true;
        (*settings)["allowTrailingCommas"] = true;
        (*settings)["strictRoot"] = false;
        (*settings)["allowDroppedNullPlaceholders"] = false;
        (*settings)["allowNumericKeys"] = false;
        (*settings)["allowSingleQuotes"] = false;
        (*settings)["stackLimit"] = 1000;
        (*settings)["failIfExtra"] = false;
        (*settings)["rejectDupKeys"] = false;
        (*settings)["allowSpecialFloats"] = false;
        //! [CharReaderBuilderDefaults]
    }

    //////////////////////////////////
    // global functions

    bool parseFromStream(CharReader::Factory const& fact, IStream& sin, Value* root,
        String* errs) {
        OStringStream ssin;
        ssin << sin.rdbuf();
        String doc = ssin.str();
        char const* begin = doc.data();
        char const* end = begin + doc.size();
        // Note that we do not actually need a null-terminator.
        CharReaderPtr const reader(fact.newCharReader());
        return reader->parse(begin, end, root, errs);
    }

    IStream& operator>>(IStream& sin, Value& root) {
        CharReaderBuilder b;
        String errs;
        bool ok = parseFromStream(b, sin, &root, &errs);
        if (!ok) {
            throwRuntimeError(errs);
        }
        return sin;
    }

} // namespace Json

// //////////////////////////////////////////////////////////////////////
// End of content of file: src/lib_json/json_reader.cpp
// //////////////////////////////////////////////////////////////////////






// //////////////////////////////////////////////////////////////////////
// Beginning of content of file: src/lib_json/json_valueiterator.inl
// //////////////////////////////////////////////////////////////////////

// Copyright 2007-2010 Baptiste Lepilleur and The JsonCpp Authors
// Distributed under MIT license, or public domain if desired and
// recognized in your jurisdiction.
// See file LICENSE for detail or copy at http://jsoncpp.sourceforge.net/LICENSE

// included by json_value.cpp

namespace Json {

    // //////////////////////////////////////////////////////////////////
    // //////////////////////////////////////////////////////////////////
    // //////////////////////////////////////////////////////////////////
    // class ValueIteratorBase
    // //////////////////////////////////////////////////////////////////
    // //////////////////////////////////////////////////////////////////
    // //////////////////////////////////////////////////////////////////

    ValueIteratorBase::ValueIteratorBase() : current_() {}

    ValueIteratorBase::ValueIteratorBase(
        const Value::ObjectValues::iterator& current)
        : current_(current), isNull_(false) {}

    Value& ValueIteratorBase::deref() { return current_->second; }
    const Value& ValueIteratorBase::deref() const { return current_->second; }

    void ValueIteratorBase::increment() { ++current_; }

    void ValueIteratorBase::decrement() { --current_; }

    ValueIteratorBase::difference_type
        ValueIteratorBase::computeDistance(const SelfType& other) const {
        // Iterator for null value are initialized using the default
        // constructor, which initialize current_ to the default
        // std::map::iterator. As begin() and end() are two instance
        // of the default std::map::iterator, they can not be compared.
        // To allow this, we handle this comparison specifically.
        if (isNull_ && other.isNull_) {
            return 0;
        }

        // Usage of std::distance is not portable (does not compile with Sun Studio 12
        // RogueWave STL,
        // which is the one used by default).
        // Using a portable hand-made version for non random iterator instead:
        //   return difference_type( std::distance( current_, other.current_ ) );
        difference_type myDistance = 0;
        for (Value::ObjectValues::iterator it = current_; it != other.current_;
            ++it) {
            ++myDistance;
        }
        return myDistance;
    }

    bool ValueIteratorBase::isEqual(const SelfType& other) const {
        if (isNull_) {
            return other.isNull_;
        }
        return current_ == other.current_;
    }

    void ValueIteratorBase::copy(const SelfType& other) {
        current_ = other.current_;
        isNull_ = other.isNull_;
    }

    Value ValueIteratorBase::key() const {
        const Value::CZString czstring = (*current_).first;
        if (czstring.data()) {
            if (czstring.isStaticString())
                return Value(StaticString(czstring.data()));
            return Value(czstring.data(), czstring.data() + czstring.length());
        }
        return Value(czstring.index());
    }

    UInt ValueIteratorBase::index() const {
        const Value::CZString czstring = (*current_).first;
        if (!czstring.data())
            return czstring.index();
        return Value::UInt(-1);
    }

    String ValueIteratorBase::name() const {
        char const* keey;
        char const* end;
        keey = memberName(&end);
        if (!keey)
            return String();
        return String(keey, end);
    }

    char const* ValueIteratorBase::memberName() const {
        const char* cname = (*current_).first.data();
        return cname ? cname : "";
    }

    char const* ValueIteratorBase::memberName(char const** end) const {
        const char* cname = (*current_).first.data();
        if (!cname) {
            *end = nullptr;
            return nullptr;
        }
        *end = cname + (*current_).first.length();
        return cname;
    }

    // //////////////////////////////////////////////////////////////////
    // //////////////////////////////////////////////////////////////////
    // //////////////////////////////////////////////////////////////////
    // class ValueConstIterator
    // //////////////////////////////////////////////////////////////////
    // //////////////////////////////////////////////////////////////////
    // //////////////////////////////////////////////////////////////////

    ValueConstIterator::ValueConstIterator() = default;

    ValueConstIterator::ValueConstIterator(
        const Value::ObjectValues::iterator& current)
        : ValueIteratorBase(current) {}

    ValueConstIterator::ValueConstIterator(ValueIterator const& other)
        : ValueIteratorBase(other) {}

    ValueConstIterator& ValueConstIterator::
        operator=(const ValueIteratorBase& other) {
        copy(other);
        return *this;
    }

    // //////////////////////////////////////////////////////////////////
    // //////////////////////////////////////////////////////////////////
    // //////////////////////////////////////////////////////////////////
    // class ValueIterator
    // //////////////////////////////////////////////////////////////////
    // //////////////////////////////////////////////////////////////////
    // //////////////////////////////////////////////////////////////////

    ValueIterator::ValueIterator() = default;

    ValueIterator::ValueIterator(const Value::ObjectValues::iterator& current)
        : ValueIteratorBase(current) {}

    ValueIterator::ValueIterator(const ValueConstIterator& other)
        : ValueIteratorBase(other) {
        throwRuntimeError("ConstIterator to Iterator should never be allowed.");
    }

    ValueIterator::ValueIterator(const ValueIterator& other) = default;

    ValueIterator& ValueIterator::operator=(const SelfType& other) {
        copy(other);
        return *this;
    }

} // namespace Json

// //////////////////////////////////////////////////////////////////////
// End of content of file: src/lib_json/json_valueiterator.inl
// //////////////////////////////////////////////////////////////////////






// //////////////////////////////////////////////////////////////////////
// Beginning of content of file: src/lib_json/json_value.cpp
// //////////////////////////////////////////////////////////////////////

// Copyright 2011 Baptiste Lepilleur and The JsonCpp Authors
// Distributed under MIT license, or public domain if desired and
// recognized in your jurisdiction.
// See file LICENSE for detail or copy at http://jsoncpp.sourceforge.net/LICENSE

#if !defined(JSON_IS_AMALGAMATION)
#include <json/assertions.h>
#include <json/value.h>
#include <json/writer.h>
#endif // if !defined(JSON_IS_AMALGAMATION)
#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstddef>
#include <cstring>
#include <sstream>
#include <utility>

// Provide implementation equivalent of std::snprintf for older _MSC compilers
#if defined(_MSC_VER) && _MSC_VER < 1900
#include <stdarg.h>
static int msvc_pre1900_c99_vsnprintf(char* outBuf, size_t size,
    const char* format, va_list ap) {
    int count = -1;
    if (size != 0)
        count = _vsnprintf_s(outBuf, size, _TRUNCATE, format, ap);
    if (count == -1)
        count = _vscprintf(format, ap);
    return count;
}

int JSON_API msvc_pre1900_c99_snprintf(char* outBuf, size_t size,
    const char* format, ...) {
    va_list ap;
    va_start(ap, format);
    const int count = msvc_pre1900_c99_vsnprintf(outBuf, size, format, ap);
    va_end(ap);
    return count;
}
#endif

// Disable warning C4702 : unreachable code
#if defined(_MSC_VER)
#pragma warning(disable : 4702)
#endif

#define JSON_ASSERT_UNREACHABLE assert(false)

namespace Json {
    template <typename T>
    static std::unique_ptr<T> cloneUnique(const std::unique_ptr<T>& p) {
        std::unique_ptr<T> r;
        if (p) {
            r = std::unique_ptr<T>(new T(*p));
        }
        return r;
    }

    // This is a walkaround to avoid the static initialization of Value::null.
    // kNull must be word-aligned to avoid crashing on ARM.  We use an alignment of
    // 8 (instead of 4) as a bit of future-proofing.
#if defined(__ARMEL__)
#define ALIGNAS(byte_alignment) __attribute__((aligned(byte_alignment)))
#else
#define ALIGNAS(byte_alignment)
#endif

// static
    Value const& Value::nullSingleton() {
        static Value const nullStatic;
        return nullStatic;
    }

#if JSON_USE_NULLREF
    // for backwards compatibility, we'll leave these global references around, but
    // DO NOT use them in JSONCPP library code any more!
    // static
    Value const& Value::null = Value::nullSingleton();

    // static
    Value const& Value::nullRef = Value::nullSingleton();
#endif

#if !defined(JSON_USE_INT64_DOUBLE_CONVERSION)
    template <typename T, typename U>
    static inline bool InRange(double d, T min, U max) {
        // The casts can lose precision, but we are looking only for
        // an approximate range. Might fail on edge cases though. ~cdunn
        return d >= static_cast<double>(min) && d <= static_cast<double>(max);
    }
#else  // if !defined(JSON_USE_INT64_DOUBLE_CONVERSION)
    static inline double integerToDouble(Json::UInt64 value) {
        return static_cast<double>(Int64(value / 2)) * 2.0 +
            static_cast<double>(Int64(value & 1));
    }

    template <typename T> static inline double integerToDouble(T value) {
        return static_cast<double>(value);
    }

    template <typename T, typename U>
    static inline bool InRange(double d, T min, U max) {
        return d >= integerToDouble(min) && d <= integerToDouble(max);
    }
#endif // if !defined(JSON_USE_INT64_DOUBLE_CONVERSION)

    /** Duplicates the specified string value.
     * @param value Pointer to the string to duplicate. Must be zero-terminated if
     *              length is "unknown".
     * @param length Length of the value. if equals to unknown, then it will be
     *               computed using strlen(value).
     * @return Pointer on the duplicate instance of string.
     */
    static inline char* duplicateStringValue(const char* value, size_t length) {
        // Avoid an integer overflow in the call to malloc below by limiting length
        // to a sane value.
        if (length >= static_cast<size_t>(Value::maxInt))
            length = Value::maxInt - 1;

        auto newString = static_cast<char*>(malloc(length + 1));
        if (newString == nullptr) {
            throwRuntimeError("in Json::Value::duplicateStringValue(): "
                "Failed to allocate string value buffer");
        }
        memcpy(newString, value, length);
        newString[length] = 0;
        return newString;
    }

    /* Record the length as a prefix.
     */
    static inline char* duplicateAndPrefixStringValue(const char* value,
        unsigned int length) {
        // Avoid an integer overflow in the call to malloc below by limiting length
        // to a sane value.
        JSON_ASSERT_MESSAGE(length <= static_cast<unsigned>(Value::maxInt) -
            sizeof(unsigned) - 1U,
            "in Json::Value::duplicateAndPrefixStringValue(): "
            "length too big for prefixing");
        size_t actualLength = sizeof(length) + length + 1;
        auto newString = static_cast<char*>(malloc(actualLength));
        if (newString == nullptr) {
            throwRuntimeError("in Json::Value::duplicateAndPrefixStringValue(): "
                "Failed to allocate string value buffer");
        }
        *reinterpret_cast<unsigned*>(newString) = length;
        memcpy(newString + sizeof(unsigned), value, length);
        newString[actualLength - 1U] =
            0; // to avoid buffer over-run accidents by users later
        return newString;
    }
    inline static void decodePrefixedString(bool isPrefixed, char const* prefixed,
        unsigned* length, char const** value) {
        if (!isPrefixed) {
            *length = static_cast<unsigned>(strlen(prefixed));
            *value = prefixed;
        } else {
            *length = *reinterpret_cast<unsigned const*>(prefixed);
            *value = prefixed + sizeof(unsigned);
        }
    }
    /** Free the string duplicated by
     * duplicateStringValue()/duplicateAndPrefixStringValue().
     */
#if JSONCPP_USING_SECURE_MEMORY
    static inline void releasePrefixedStringValue(char* value) {
        unsigned length = 0;
        char const* valueDecoded;
        decodePrefixedString(true, value, &length, &valueDecoded);
        size_t const size = sizeof(unsigned) + length + 1U;
        memset(value, 0, size);
        free(value);
    }
    static inline void releaseStringValue(char* value, unsigned length) {
        // length==0 => we allocated the strings memory
        size_t size = (length == 0) ? strlen(value) : length;
        memset(value, 0, size);
        free(value);
    }
#else  // !JSONCPP_USING_SECURE_MEMORY
    static inline void releasePrefixedStringValue(char* value) { free(value); }
    static inline void releaseStringValue(char* value, unsigned) { free(value); }
#endif // JSONCPP_USING_SECURE_MEMORY

} // namespace Json

// //////////////////////////////////////////////////////////////////
// //////////////////////////////////////////////////////////////////
// //////////////////////////////////////////////////////////////////
// ValueInternals...
// //////////////////////////////////////////////////////////////////
// //////////////////////////////////////////////////////////////////
// //////////////////////////////////////////////////////////////////
#if !defined(JSON_IS_AMALGAMATION)

#include "json_valueiterator.inl"
#endif // if !defined(JSON_IS_AMALGAMATION)

namespace Json {

#if JSON_USE_EXCEPTION
    Exception::Exception(String msg) : msg_(std::move(msg)) {}
    Exception::~Exception() JSONCPP_NOEXCEPT = default;
    char const* Exception::what() const JSONCPP_NOEXCEPT { return msg_.c_str(); }
    RuntimeError::RuntimeError(String const& msg) : Exception(msg) {}
    LogicError::LogicError(String const& msg) : Exception(msg) {}
    JSONCPP_NORETURN void throwRuntimeError(String const& msg) {
        throw RuntimeError(msg);
    }
    JSONCPP_NORETURN void throwLogicError(String const& msg) {
        throw LogicError(msg);
    }
#else // !JSON_USE_EXCEPTION
    JSONCPP_NORETURN void throwRuntimeError(String const& msg) { abort(); }
    JSONCPP_NORETURN void throwLogicError(String const& msg) { abort(); }
#endif

    // //////////////////////////////////////////////////////////////////
    // //////////////////////////////////////////////////////////////////
    // //////////////////////////////////////////////////////////////////
    // class Value::CZString
    // //////////////////////////////////////////////////////////////////
    // //////////////////////////////////////////////////////////////////
    // //////////////////////////////////////////////////////////////////

    // Notes: policy_ indicates if the string was allocated when
    // a string is stored.

    Value::CZString::CZString(ArrayIndex index) : cstr_(nullptr), index_(index) {}

    Value::CZString::CZString(char const* str, unsigned length,
        DuplicationPolicy allocate)
        : cstr_(str) {
        // allocate != duplicate
        storage_.policy_ = allocate & 0x3;
        storage_.length_ = length & 0x3FFFFFFF;
    }

    Value::CZString::CZString(const CZString& other) {
        cstr_ = (other.storage_.policy_ != noDuplication && other.cstr_ != nullptr
            ? duplicateStringValue(other.cstr_, other.storage_.length_)
            : other.cstr_);
        storage_.policy_ =
            static_cast<unsigned>(
                other.cstr_
                ? (static_cast<DuplicationPolicy>(other.storage_.policy_) ==
                    noDuplication
                    ? noDuplication
                    : duplicate)
                : static_cast<DuplicationPolicy>(other.storage_.policy_)) &
            3U;
        storage_.length_ = other.storage_.length_;
    }

    Value::CZString::CZString(CZString&& other)
        : cstr_(other.cstr_), index_(other.index_) {
        other.cstr_ = nullptr;
    }

    Value::CZString::~CZString() {
        if (cstr_ && storage_.policy_ == duplicate) {
            releaseStringValue(const_cast<char*>(cstr_),
                storage_.length_ + 1U); // +1 for null terminating
                                        // character for sake of
                                        // completeness but not actually
                                        // necessary
        }
    }

    void Value::CZString::swap(CZString& other) {
        std::swap(cstr_, other.cstr_);
        std::swap(index_, other.index_);
    }

    Value::CZString& Value::CZString::operator=(const CZString& other) {
        cstr_ = other.cstr_;
        index_ = other.index_;
        return *this;
    }

    Value::CZString& Value::CZString::operator=(CZString&& other) {
        cstr_ = other.cstr_;
        index_ = other.index_;
        other.cstr_ = nullptr;
        return *this;
    }

    bool Value::CZString::operator<(const CZString& other) const {
        if (!cstr_)
            return index_ < other.index_;
        // return strcmp(cstr_, other.cstr_) < 0;
        // Assume both are strings.
        unsigned this_len = this->storage_.length_;
        unsigned other_len = other.storage_.length_;
        unsigned min_len = std::min<unsigned>(this_len, other_len);
        JSON_ASSERT(this->cstr_ && other.cstr_);
        int comp = memcmp(this->cstr_, other.cstr_, min_len);
        if (comp < 0)
            return true;
        if (comp > 0)
            return false;
        return (this_len < other_len);
    }

    bool Value::CZString::operator==(const CZString& other) const {
        if (!cstr_)
            return index_ == other.index_;
        // return strcmp(cstr_, other.cstr_) == 0;
        // Assume both are strings.
        unsigned this_len = this->storage_.length_;
        unsigned other_len = other.storage_.length_;
        if (this_len != other_len)
            return false;
        JSON_ASSERT(this->cstr_ && other.cstr_);
        int comp = memcmp(this->cstr_, other.cstr_, this_len);
        return comp == 0;
    }

    ArrayIndex Value::CZString::index() const { return index_; }

    // const char* Value::CZString::c_str() const { return cstr_; }
    const char* Value::CZString::data() const { return cstr_; }
    unsigned Value::CZString::length() const { return storage_.length_; }
    bool Value::CZString::isStaticString() const {
        return storage_.policy_ == noDuplication;
    }

    // //////////////////////////////////////////////////////////////////
    // //////////////////////////////////////////////////////////////////
    // //////////////////////////////////////////////////////////////////
    // class Value::Value
    // //////////////////////////////////////////////////////////////////
    // //////////////////////////////////////////////////////////////////
    // //////////////////////////////////////////////////////////////////

    /*! \internal Default constructor initialization must be equivalent to:
     * memset( this, 0, sizeof(Value) )
     * This optimization is used in ValueInternalMap fast allocator.
     */
    Value::Value(ValueType type) {
        static char const emptyString[] = "";
        initBasic(type);
        switch (type) {
        case nullValue:
            break;
        case intValue:
        case uintValue:
            value_.int_ = 0;
            break;
        case realValue:
            value_.real_ = 0.0;
            break;
        case stringValue:
            // allocated_ == false, so this is safe.
            value_.string_ = const_cast<char*>(static_cast<char const*>(emptyString));
            break;
        case arrayValue:
        case objectValue:
            value_.map_ = new ObjectValues();
            break;
        case booleanValue:
            value_.bool_ = false;
            break;
        default:
            JSON_ASSERT_UNREACHABLE;
        }
    }

    Value::Value(Int value) {
        initBasic(intValue);
        value_.int_ = value;
    }

    Value::Value(UInt value) {
        initBasic(uintValue);
        value_.uint_ = value;
    }
#if defined(JSON_HAS_INT64)
    Value::Value(Int64 value) {
        initBasic(intValue);
        value_.int_ = value;
    }
    Value::Value(UInt64 value) {
        initBasic(uintValue);
        value_.uint_ = value;
    }
#endif // defined(JSON_HAS_INT64)

    Value::Value(double value) {
        initBasic(realValue);
        value_.real_ = value;
    }

    Value::Value(const char* value) {
        initBasic(stringValue, true);
        JSON_ASSERT_MESSAGE(value != nullptr,
            "Null Value Passed to Value Constructor");
        value_.string_ = duplicateAndPrefixStringValue(
            value, static_cast<unsigned>(strlen(value)));
    }

    Value::Value(const char* begin, const char* end) {
        initBasic(stringValue, true);
        value_.string_ =
            duplicateAndPrefixStringValue(begin, static_cast<unsigned>(end - begin));
    }

    Value::Value(const String& value) {
        initBasic(stringValue, true);
        value_.string_ = duplicateAndPrefixStringValue(
            value.data(), static_cast<unsigned>(value.length()));
    }

    Value::Value(const StaticString& value) {
        initBasic(stringValue);
        value_.string_ = const_cast<char*>(value.c_str());
    }

    Value::Value(bool value) {
        initBasic(booleanValue);
        value_.bool_ = value;
    }

    Value::Value(const Value& other) {
        dupPayload(other);
        dupMeta(other);
    }

    Value::Value(Value&& other) {
        initBasic(nullValue);
        swap(other);
    }

    Value::~Value() {
        releasePayload();
        value_.uint_ = 0;
    }

    Value& Value::operator=(const Value& other) {
        Value(other).swap(*this);
        return *this;
    }

    Value& Value::operator=(Value&& other) {
        other.swap(*this);
        return *this;
    }

    void Value::swapPayload(Value& other) {
        std::swap(bits_, other.bits_);
        std::swap(value_, other.value_);
    }

    void Value::copyPayload(const Value& other) {
        releasePayload();
        dupPayload(other);
    }

    void Value::swap(Value& other) {
        swapPayload(other);
        std::swap(comments_, other.comments_);
        std::swap(start_, other.start_);
        std::swap(limit_, other.limit_);
    }

    void Value::copy(const Value& other) {
        copyPayload(other);
        dupMeta(other);
    }

    ValueType Value::type() const {
        return static_cast<ValueType>(bits_.value_type_);
    }

    int Value::compare(const Value& other) const {
        if (*this < other)
            return -1;
        if (*this > other)
            return 1;
        return 0;
    }

    bool Value::operator<(const Value& other) const {
        int typeDelta = type() - other.type();
        if (typeDelta)
            return typeDelta < 0;
        switch (type()) {
        case nullValue:
            return false;
        case intValue:
            return value_.int_ < other.value_.int_;
        case uintValue:
            return value_.uint_ < other.value_.uint_;
        case realValue:
            return value_.real_ < other.value_.real_;
        case booleanValue:
            return value_.bool_ < other.value_.bool_;
        case stringValue: {
            if ((value_.string_ == nullptr) || (other.value_.string_ == nullptr)) {
                return other.value_.string_ != nullptr;
            }
            unsigned this_len;
            unsigned other_len;
            char const* this_str;
            char const* other_str;
            decodePrefixedString(this->isAllocated(), this->value_.string_, &this_len,
                &this_str);
            decodePrefixedString(other.isAllocated(), other.value_.string_, &other_len,
                &other_str);
            unsigned min_len = std::min<unsigned>(this_len, other_len);
            JSON_ASSERT(this_str && other_str);
            int comp = memcmp(this_str, other_str, min_len);
            if (comp < 0)
                return true;
            if (comp > 0)
                return false;
            return (this_len < other_len);
        }
        case arrayValue:
        case objectValue: {
            auto thisSize = value_.map_->size();
            auto otherSize = other.value_.map_->size();
            if (thisSize != otherSize)
                return thisSize < otherSize;
            return (*value_.map_) < (*other.value_.map_);
        }
        default:
            JSON_ASSERT_UNREACHABLE;
        }
        return false; // unreachable
    }

    bool Value::operator<=(const Value& other) const { return !(other < *this); }

    bool Value::operator>=(const Value& other) const { return !(*this < other); }

    bool Value::operator>(const Value& other) const { return other < *this; }

    bool Value::operator==(const Value& other) const {
        if (type() != other.type())
            return false;
        switch (type()) {
        case nullValue:
            return true;
        case intValue:
            return value_.int_ == other.value_.int_;
        case uintValue:
            return value_.uint_ == other.value_.uint_;
        case realValue:
            return value_.real_ == other.value_.real_;
        case booleanValue:
            return value_.bool_ == other.value_.bool_;
        case stringValue: {
            if ((value_.string_ == nullptr) || (other.value_.string_ == nullptr)) {
                return (value_.string_ == other.value_.string_);
            }
            unsigned this_len;
            unsigned other_len;
            char const* this_str;
            char const* other_str;
            decodePrefixedString(this->isAllocated(), this->value_.string_, &this_len,
                &this_str);
            decodePrefixedString(other.isAllocated(), other.value_.string_, &other_len,
                &other_str);
            if (this_len != other_len)
                return false;
            JSON_ASSERT(this_str && other_str);
            int comp = memcmp(this_str, other_str, this_len);
            return comp == 0;
        }
        case arrayValue:
        case objectValue:
            return value_.map_->size() == other.value_.map_->size() &&
                (*value_.map_) == (*other.value_.map_);
        default:
            JSON_ASSERT_UNREACHABLE;
        }
        return false; // unreachable
    }

    bool Value::operator!=(const Value& other) const { return !(*this == other); }

    const char* Value::asCString() const {
        JSON_ASSERT_MESSAGE(type() == stringValue,
            "in Json::Value::asCString(): requires stringValue");
        if (value_.string_ == nullptr)
            return nullptr;
        unsigned this_len;
        char const* this_str;
        decodePrefixedString(this->isAllocated(), this->value_.string_, &this_len,
            &this_str);
        return this_str;
    }

#if JSONCPP_USING_SECURE_MEMORY
    unsigned Value::getCStringLength() const {
        JSON_ASSERT_MESSAGE(type() == stringValue,
            "in Json::Value::asCString(): requires stringValue");
        if (value_.string_ == 0)
            return 0;
        unsigned this_len;
        char const* this_str;
        decodePrefixedString(this->isAllocated(), this->value_.string_, &this_len,
            &this_str);
        return this_len;
    }
#endif

    bool Value::getString(char const** begin, char const** end) const {
        if (type() != stringValue)
            return false;
        if (value_.string_ == nullptr)
            return false;
        unsigned length;
        decodePrefixedString(this->isAllocated(), this->value_.string_, &length,
            begin);
        *end = *begin + length;
        return true;
    }

    String Value::asString() const {
        switch (type()) {
        case nullValue:
            return "";
        case stringValue: {
            if (value_.string_ == nullptr)
                return "";
            unsigned this_len;
            char const* this_str;
            decodePrefixedString(this->isAllocated(), this->value_.string_, &this_len,
                &this_str);
            return String(this_str, this_len);
        }
        case booleanValue:
            return value_.bool_ ? "true" : "false";
        case intValue:
            return valueToString(value_.int_);
        case uintValue:
            return valueToString(value_.uint_);
        case realValue:
            return valueToString(value_.real_);
        default:
            JSON_FAIL_MESSAGE("Type is not convertible to string");
        }
    }

    Value::Int Value::asInt() const {
        switch (type()) {
        case intValue:
            JSON_ASSERT_MESSAGE(isInt(), "LargestInt out of Int range");
            return Int(value_.int_);
        case uintValue:
            JSON_ASSERT_MESSAGE(isInt(), "LargestUInt out of Int range");
            return Int(value_.uint_);
        case realValue:
            JSON_ASSERT_MESSAGE(InRange(value_.real_, minInt, maxInt),
                "double out of Int range");
            return Int(value_.real_);
        case nullValue:
            return 0;
        case booleanValue:
            return value_.bool_ ? 1 : 0;
        default:
            break;
        }
        JSON_FAIL_MESSAGE("Value is not convertible to Int.");
    }

    Value::UInt Value::asUInt() const {
        switch (type()) {
        case intValue:
            JSON_ASSERT_MESSAGE(isUInt(), "LargestInt out of UInt range");
            return UInt(value_.int_);
        case uintValue:
            JSON_ASSERT_MESSAGE(isUInt(), "LargestUInt out of UInt range");
            return UInt(value_.uint_);
        case realValue:
            JSON_ASSERT_MESSAGE(InRange(value_.real_, 0, maxUInt),
                "double out of UInt range");
            return UInt(value_.real_);
        case nullValue:
            return 0;
        case booleanValue:
            return value_.bool_ ? 1 : 0;
        default:
            break;
        }
        JSON_FAIL_MESSAGE("Value is not convertible to UInt.");
    }

#if defined(JSON_HAS_INT64)

    Value::Int64 Value::asInt64() const {
        switch (type()) {
        case intValue:
            return Int64(value_.int_);
        case uintValue:
            JSON_ASSERT_MESSAGE(isInt64(), "LargestUInt out of Int64 range");
            return Int64(value_.uint_);
        case realValue:
            JSON_ASSERT_MESSAGE(InRange(value_.real_, minInt64, maxInt64),
                "double out of Int64 range");
            return Int64(value_.real_);
        case nullValue:
            return 0;
        case booleanValue:
            return value_.bool_ ? 1 : 0;
        default:
            break;
        }
        JSON_FAIL_MESSAGE("Value is not convertible to Int64.");
    }

    Value::UInt64 Value::asUInt64() const {
        switch (type()) {
        case intValue:
            JSON_ASSERT_MESSAGE(isUInt64(), "LargestInt out of UInt64 range");
            return UInt64(value_.int_);
        case uintValue:
            return UInt64(value_.uint_);
        case realValue:
            JSON_ASSERT_MESSAGE(InRange(value_.real_, 0, maxUInt64),
                "double out of UInt64 range");
            return UInt64(value_.real_);
        case nullValue:
            return 0;
        case booleanValue:
            return value_.bool_ ? 1 : 0;
        default:
            break;
        }
        JSON_FAIL_MESSAGE("Value is not convertible to UInt64.");
    }
#endif // if defined(JSON_HAS_INT64)

    LargestInt Value::asLargestInt() const {
#if defined(JSON_NO_INT64)
        return asInt();
#else
        return asInt64();
#endif
    }

    LargestUInt Value::asLargestUInt() const {
#if defined(JSON_NO_INT64)
        return asUInt();
#else
        return asUInt64();
#endif
    }

    double Value::asDouble() const {
        switch (type()) {
        case intValue:
            return static_cast<double>(value_.int_);
        case uintValue:
#if !defined(JSON_USE_INT64_DOUBLE_CONVERSION)
            return static_cast<double>(value_.uint_);
#else  // if !defined(JSON_USE_INT64_DOUBLE_CONVERSION)
            return integerToDouble(value_.uint_);
#endif // if !defined(JSON_USE_INT64_DOUBLE_CONVERSION)
        case realValue:
            return value_.real_;
        case nullValue:
            return 0.0;
        case booleanValue:
            return value_.bool_ ? 1.0 : 0.0;
        default:
            break;
        }
        JSON_FAIL_MESSAGE("Value is not convertible to double.");
    }

    float Value::asFloat() const {
        switch (type()) {
        case intValue:
            return static_cast<float>(value_.int_);
        case uintValue:
#if !defined(JSON_USE_INT64_DOUBLE_CONVERSION)
            return static_cast<float>(value_.uint_);
#else  // if !defined(JSON_USE_INT64_DOUBLE_CONVERSION)
            // This can fail (silently?) if the value is bigger than MAX_FLOAT.
            return static_cast<float>(integerToDouble(value_.uint_));
#endif // if !defined(JSON_USE_INT64_DOUBLE_CONVERSION)
        case realValue:
            return static_cast<float>(value_.real_);
        case nullValue:
            return 0.0;
        case booleanValue:
            return value_.bool_ ? 1.0F : 0.0F;
        default:
            break;
        }
        JSON_FAIL_MESSAGE("Value is not convertible to float.");
    }

    bool Value::asBool() const {
        switch (type()) {
        case booleanValue:
            return value_.bool_;
        case nullValue:
            return false;
        case intValue:
            return value_.int_ != 0;
        case uintValue:
            return value_.uint_ != 0;
        case realValue: {
            // According to JavaScript language zero or NaN is regarded as false
            const auto value_classification = std::fpclassify(value_.real_);
            return value_classification != FP_ZERO && value_classification != FP_NAN;
        }
        default:
            break;
        }
        JSON_FAIL_MESSAGE("Value is not convertible to bool.");
    }

    bool Value::isConvertibleTo(ValueType other) const {
        switch (other) {
        case nullValue:
            return (isNumeric() && asDouble() == 0.0) ||
                (type() == booleanValue && !value_.bool_) ||
                (type() == stringValue && asString().empty()) ||
                (type() == arrayValue && value_.map_->empty()) ||
                (type() == objectValue && value_.map_->empty()) ||
                type() == nullValue;
        case intValue:
            return isInt() ||
                (type() == realValue && InRange(value_.real_, minInt, maxInt)) ||
                type() == booleanValue || type() == nullValue;
        case uintValue:
            return isUInt() ||
                (type() == realValue && InRange(value_.real_, 0, maxUInt)) ||
                type() == booleanValue || type() == nullValue;
        case realValue:
            return isNumeric() || type() == booleanValue || type() == nullValue;
        case booleanValue:
            return isNumeric() || type() == booleanValue || type() == nullValue;
        case stringValue:
            return isNumeric() || type() == booleanValue || type() == stringValue ||
                type() == nullValue;
        case arrayValue:
            return type() == arrayValue || type() == nullValue;
        case objectValue:
            return type() == objectValue || type() == nullValue;
        }
        JSON_ASSERT_UNREACHABLE;
        return false;
    }

    /// Number of values in array or object
    ArrayIndex Value::size() const {
        switch (type()) {
        case nullValue:
        case intValue:
        case uintValue:
        case realValue:
        case booleanValue:
        case stringValue:
            return 0;
        case arrayValue: // size of the array is highest index + 1
            if (!value_.map_->empty()) {
                ObjectValues::const_iterator itLast = value_.map_->end();
                --itLast;
                return (*itLast).first.index() + 1;
            }
            return 0;
        case objectValue:
            return ArrayIndex(value_.map_->size());
        }
        JSON_ASSERT_UNREACHABLE;
        return 0; // unreachable;
    }

    bool Value::empty() const {
        if (isNull() || isArray() || isObject())
            return size() == 0U;
        return false;
    }

    Value::operator bool() const { return !isNull(); }

    void Value::clear() {
        JSON_ASSERT_MESSAGE(type() == nullValue || type() == arrayValue ||
            type() == objectValue,
            "in Json::Value::clear(): requires complex value");
        start_ = 0;
        limit_ = 0;
        switch (type()) {
        case arrayValue:
        case objectValue:
            value_.map_->clear();
            break;
        default:
            break;
        }
    }

    void Value::resize(ArrayIndex newSize) {
        JSON_ASSERT_MESSAGE(type() == nullValue || type() == arrayValue,
            "in Json::Value::resize(): requires arrayValue");
        if (type() == nullValue)
            *this = Value(arrayValue);
        ArrayIndex oldSize = size();
        if (newSize == 0)
            clear();
        else if (newSize > oldSize)
            this->operator[](newSize - 1);
        else {
            for (ArrayIndex index = newSize; index < oldSize; ++index) {
                value_.map_->erase(index);
            }
            JSON_ASSERT(size() == newSize);
        }
    }

    Value& Value::operator[](ArrayIndex index) {
        JSON_ASSERT_MESSAGE(
            type() == nullValue || type() == arrayValue,
            "in Json::Value::operator[](ArrayIndex): requires arrayValue");
        if (type() == nullValue)
            *this = Value(arrayValue);
        CZString key(index);
        auto it = value_.map_->lower_bound(key);
        if (it != value_.map_->end() && (*it).first == key)
            return (*it).second;

        ObjectValues::value_type defaultValue(key, nullSingleton());
        it = value_.map_->insert(it, defaultValue);
        return (*it).second;
    }

    Value& Value::operator[](int index) {
        JSON_ASSERT_MESSAGE(
            index >= 0,
            "in Json::Value::operator[](int index): index cannot be negative");
        return (*this)[ArrayIndex(index)];
    }

    const Value& Value::operator[](ArrayIndex index) const {
        JSON_ASSERT_MESSAGE(
            type() == nullValue || type() == arrayValue,
            "in Json::Value::operator[](ArrayIndex)const: requires arrayValue");
        if (type() == nullValue)
            return nullSingleton();
        CZString key(index);
        ObjectValues::const_iterator it = value_.map_->find(key);
        if (it == value_.map_->end())
            return nullSingleton();
        return (*it).second;
    }

    const Value& Value::operator[](int index) const {
        JSON_ASSERT_MESSAGE(
            index >= 0,
            "in Json::Value::operator[](int index) const: index cannot be negative");
        return (*this)[ArrayIndex(index)];
    }

    void Value::initBasic(ValueType type, bool allocated) {
        setType(type);
        setIsAllocated(allocated);
        comments_ = Comments{};
        start_ = 0;
        limit_ = 0;
    }

    void Value::dupPayload(const Value& other) {
        setType(other.type());
        setIsAllocated(false);
        switch (type()) {
        case nullValue:
        case intValue:
        case uintValue:
        case realValue:
        case booleanValue:
            value_ = other.value_;
            break;
        case stringValue:
            if (other.value_.string_ && other.isAllocated()) {
                unsigned len;
                char const* str;
                decodePrefixedString(other.isAllocated(), other.value_.string_, &len,
                    &str);
                value_.string_ = duplicateAndPrefixStringValue(str, len);
                setIsAllocated(true);
            } else {
                value_.string_ = other.value_.string_;
            }
            break;
        case arrayValue:
        case objectValue:
            value_.map_ = new ObjectValues(*other.value_.map_);
            break;
        default:
            JSON_ASSERT_UNREACHABLE;
        }
    }

    void Value::releasePayload() {
        switch (type()) {
        case nullValue:
        case intValue:
        case uintValue:
        case realValue:
        case booleanValue:
            break;
        case stringValue:
            if (isAllocated())
                releasePrefixedStringValue(value_.string_);
            break;
        case arrayValue:
        case objectValue:
            delete value_.map_;
            break;
        default:
            JSON_ASSERT_UNREACHABLE;
        }
    }

    void Value::dupMeta(const Value& other) {
        comments_ = other.comments_;
        start_ = other.start_;
        limit_ = other.limit_;
    }

    // Access an object value by name, create a null member if it does not exist.
    // @pre Type of '*this' is object or null.
    // @param key is null-terminated.
    Value& Value::resolveReference(const char* key) {
        JSON_ASSERT_MESSAGE(
            type() == nullValue || type() == objectValue,
            "in Json::Value::resolveReference(): requires objectValue");
        if (type() == nullValue)
            *this = Value(objectValue);
        CZString actualKey(key, static_cast<unsigned>(strlen(key)),
            CZString::noDuplication); // NOTE!
        auto it = value_.map_->lower_bound(actualKey);
        if (it != value_.map_->end() && (*it).first == actualKey)
            return (*it).second;

        ObjectValues::value_type defaultValue(actualKey, nullSingleton());
        it = value_.map_->insert(it, defaultValue);
        Value& value = (*it).second;
        return value;
    }

    // @param key is not null-terminated.
    Value& Value::resolveReference(char const* key, char const* end) {
        JSON_ASSERT_MESSAGE(
            type() == nullValue || type() == objectValue,
            "in Json::Value::resolveReference(key, end): requires objectValue");
        if (type() == nullValue)
            *this = Value(objectValue);
        CZString actualKey(key, static_cast<unsigned>(end - key),
            CZString::duplicateOnCopy);
        auto it = value_.map_->lower_bound(actualKey);
        if (it != value_.map_->end() && (*it).first == actualKey)
            return (*it).second;

        ObjectValues::value_type defaultValue(actualKey, nullSingleton());
        it = value_.map_->insert(it, defaultValue);
        Value& value = (*it).second;
        return value;
    }

    Value Value::get(ArrayIndex index, const Value& defaultValue) const {
        const Value* value = &((*this)[index]);
        return value == &nullSingleton() ? defaultValue : *value;
    }

    bool Value::isValidIndex(ArrayIndex index) const { return index < size(); }

    Value const* Value::find(char const* begin, char const* end) const {
        JSON_ASSERT_MESSAGE(type() == nullValue || type() == objectValue,
            "in Json::Value::find(begin, end): requires "
            "objectValue or nullValue");
        if (type() == nullValue)
            return nullptr;
        CZString actualKey(begin, static_cast<unsigned>(end - begin),
            CZString::noDuplication);
        ObjectValues::const_iterator it = value_.map_->find(actualKey);
        if (it == value_.map_->end())
            return nullptr;
        return &(*it).second;
    }
    Value* Value::demand(char const* begin, char const* end) {
        JSON_ASSERT_MESSAGE(type() == nullValue || type() == objectValue,
            "in Json::Value::demand(begin, end): requires "
            "objectValue or nullValue");
        return &resolveReference(begin, end);
    }
    const Value& Value::operator[](const char* key) const {
        Value const* found = find(key, key + strlen(key));
        if (!found)
            return nullSingleton();
        return *found;
    }
    Value const& Value::operator[](const String& key) const {
        Value const* found = find(key.data(), key.data() + key.length());
        if (!found)
            return nullSingleton();
        return *found;
    }

    Value& Value::operator[](const char* key) {
        return resolveReference(key, key + strlen(key));
    }

    Value& Value::operator[](const String& key) {
        return resolveReference(key.data(), key.data() + key.length());
    }

    Value& Value::operator[](const StaticString& key) {
        return resolveReference(key.c_str());
    }

    Value& Value::append(const Value& value) { return append(Value(value)); }

    Value& Value::append(Value&& value) {
        JSON_ASSERT_MESSAGE(type() == nullValue || type() == arrayValue,
            "in Json::Value::append: requires arrayValue");
        if (type() == nullValue) {
            *this = Value(arrayValue);
        }
        return this->value_.map_->emplace(size(), std::move(value)).first->second;
    }

    bool Value::insert(ArrayIndex index, const Value& newValue) {
        return insert(index, Value(newValue));
    }

    bool Value::insert(ArrayIndex index, Value&& newValue) {
        JSON_ASSERT_MESSAGE(type() == nullValue || type() == arrayValue,
            "in Json::Value::insert: requires arrayValue");
        ArrayIndex length = size();
        if (index > length) {
            return false;
        }
        for (ArrayIndex i = length; i > index; i--) {
            (*this)[i] = std::move((*this)[i - 1]);
        }
        (*this)[index] = std::move(newValue);
        return true;
    }

    Value Value::get(char const* begin, char const* end,
        Value const& defaultValue) const {
        Value const* found = find(begin, end);
        return !found ? defaultValue : *found;
    }
    Value Value::get(char const* key, Value const& defaultValue) const {
        return get(key, key + strlen(key), defaultValue);
    }
    Value Value::get(String const& key, Value const& defaultValue) const {
        return get(key.data(), key.data() + key.length(), defaultValue);
    }

    bool Value::removeMember(const char* begin, const char* end, Value* removed) {
        if (type() != objectValue) {
            return false;
        }
        CZString actualKey(begin, static_cast<unsigned>(end - begin),
            CZString::noDuplication);
        auto it = value_.map_->find(actualKey);
        if (it == value_.map_->end())
            return false;
        if (removed)
            *removed = std::move(it->second);
        value_.map_->erase(it);
        return true;
    }
    bool Value::removeMember(const char* key, Value* removed) {
        return removeMember(key, key + strlen(key), removed);
    }
    bool Value::removeMember(String const& key, Value* removed) {
        return removeMember(key.data(), key.data() + key.length(), removed);
    }
    void Value::removeMember(const char* key) {
        JSON_ASSERT_MESSAGE(type() == nullValue || type() == objectValue,
            "in Json::Value::removeMember(): requires objectValue");
        if (type() == nullValue)
            return;

        CZString actualKey(key, unsigned(strlen(key)), CZString::noDuplication);
        value_.map_->erase(actualKey);
    }
    void Value::removeMember(const String& key) { removeMember(key.c_str()); }

    bool Value::removeIndex(ArrayIndex index, Value* removed) {
        if (type() != arrayValue) {
            return false;
        }
        CZString key(index);
        auto it = value_.map_->find(key);
        if (it == value_.map_->end()) {
            return false;
        }
        if (removed)
            *removed = it->second;
        ArrayIndex oldSize = size();
        // shift left all items left, into the place of the "removed"
        for (ArrayIndex i = index; i < (oldSize - 1); ++i) {
            CZString keey(i);
            (*value_.map_)[keey] = (*this)[i + 1];
        }
        // erase the last one ("leftover")
        CZString keyLast(oldSize - 1);
        auto itLast = value_.map_->find(keyLast);
        value_.map_->erase(itLast);
        return true;
    }

    bool Value::isMember(char const* begin, char const* end) const {
        Value const* value = find(begin, end);
        return nullptr != value;
    }
    bool Value::isMember(char const* key) const {
        return isMember(key, key + strlen(key));
    }
    bool Value::isMember(String const& key) const {
        return isMember(key.data(), key.data() + key.length());
    }

    Value::Members Value::getMemberNames() const {
        JSON_ASSERT_MESSAGE(
            type() == nullValue || type() == objectValue,
            "in Json::Value::getMemberNames(), value must be objectValue");
        if (type() == nullValue)
            return Value::Members();
        Members members;
        members.reserve(value_.map_->size());
        ObjectValues::const_iterator it = value_.map_->begin();
        ObjectValues::const_iterator itEnd = value_.map_->end();
        for (; it != itEnd; ++it) {
            members.push_back(String((*it).first.data(), (*it).first.length()));
        }
        return members;
    }

    static bool IsIntegral(double d) {
        double integral_part;
        return modf(d, &integral_part) == 0.0;
    }

    bool Value::isNull() const { return type() == nullValue; }

    bool Value::isBool() const { return type() == booleanValue; }

    bool Value::isInt() const {
        switch (type()) {
        case intValue:
#if defined(JSON_HAS_INT64)
            return value_.int_ >= minInt && value_.int_ <= maxInt;
#else
            return true;
#endif
        case uintValue:
            return value_.uint_ <= UInt(maxInt);
        case realValue:
            return value_.real_ >= minInt && value_.real_ <= maxInt &&
                IsIntegral(value_.real_);
        default:
            break;
        }
        return false;
    }

    bool Value::isUInt() const {
        switch (type()) {
        case intValue:
#if defined(JSON_HAS_INT64)
            return value_.int_ >= 0 && LargestUInt(value_.int_) <= LargestUInt(maxUInt);
#else
            return value_.int_ >= 0;
#endif
        case uintValue:
#if defined(JSON_HAS_INT64)
            return value_.uint_ <= maxUInt;
#else
            return true;
#endif
        case realValue:
            return value_.real_ >= 0 && value_.real_ <= maxUInt &&
                IsIntegral(value_.real_);
        default:
            break;
        }
        return false;
    }

    bool Value::isInt64() const {
#if defined(JSON_HAS_INT64)
        switch (type()) {
        case intValue:
            return true;
        case uintValue:
            return value_.uint_ <= UInt64(maxInt64);
        case realValue:
            // Note that maxInt64 (= 2^63 - 1) is not exactly representable as a
            // double, so double(maxInt64) will be rounded up to 2^63. Therefore we
            // require the value to be strictly less than the limit.
            return value_.real_ >= double(minInt64) &&
                value_.real_ < double(maxInt64) && IsIntegral(value_.real_);
        default:
            break;
        }
#endif // JSON_HAS_INT64
        return false;
    }

    bool Value::isUInt64() const {
#if defined(JSON_HAS_INT64)
        switch (type()) {
        case intValue:
            return value_.int_ >= 0;
        case uintValue:
            return true;
        case realValue:
            // Note that maxUInt64 (= 2^64 - 1) is not exactly representable as a
            // double, so double(maxUInt64) will be rounded up to 2^64. Therefore we
            // require the value to be strictly less than the limit.
            return value_.real_ >= 0 && value_.real_ < maxUInt64AsDouble &&
                IsIntegral(value_.real_);
        default:
            break;
        }
#endif // JSON_HAS_INT64
        return false;
    }

    bool Value::isIntegral() const {
        switch (type()) {
        case intValue:
        case uintValue:
            return true;
        case realValue:
#if defined(JSON_HAS_INT64)
            // Note that maxUInt64 (= 2^64 - 1) is not exactly representable as a
            // double, so double(maxUInt64) will be rounded up to 2^64. Therefore we
            // require the value to be strictly less than the limit.
            return value_.real_ >= double(minInt64) &&
                value_.real_ < maxUInt64AsDouble && IsIntegral(value_.real_);
#else
            return value_.real_ >= minInt && value_.real_ <= maxUInt &&
                IsIntegral(value_.real_);
#endif // JSON_HAS_INT64
        default:
            break;
        }
        return false;
    }

    bool Value::isDouble() const {
        return type() == intValue || type() == uintValue || type() == realValue;
    }

    bool Value::isNumeric() const { return isDouble(); }

    bool Value::isString() const { return type() == stringValue; }

    bool Value::isArray() const { return type() == arrayValue; }

    bool Value::isObject() const { return type() == objectValue; }

    Value::Comments::Comments(const Comments& that)
        : ptr_{ cloneUnique(that.ptr_) } {}

    Value::Comments::Comments(Comments&& that) : ptr_{ std::move(that.ptr_) } {}

    Value::Comments& Value::Comments::operator=(const Comments& that) {
        ptr_ = cloneUnique(that.ptr_);
        return *this;
    }

    Value::Comments& Value::Comments::operator=(Comments&& that) {
        ptr_ = std::move(that.ptr_);
        return *this;
    }

    bool Value::Comments::has(CommentPlacement slot) const {
        return ptr_ && !(*ptr_)[slot].empty();
    }

    String Value::Comments::get(CommentPlacement slot) const {
        if (!ptr_)
            return {};
        return (*ptr_)[slot];
    }

    void Value::Comments::set(CommentPlacement slot, String comment) {
        if (!ptr_) {
            ptr_ = std::unique_ptr<Array>(new Array());
        }
        // check comments array boundry.
        if (slot < CommentPlacement::numberOfCommentPlacement) {
            (*ptr_)[slot] = std::move(comment);
        }
    }

    void Value::setComment(String comment, CommentPlacement placement) {
        if (!comment.empty() && (comment.back() == '\n')) {
            // Always discard trailing newline, to aid indentation.
            comment.pop_back();
        }
        JSON_ASSERT(!comment.empty());
        JSON_ASSERT_MESSAGE(
            comment[0] == '\0' || comment[0] == '/',
            "in Json::Value::setComment(): Comments must start with /");
        comments_.set(placement, std::move(comment));
    }

    bool Value::hasComment(CommentPlacement placement) const {
        return comments_.has(placement);
    }

    String Value::getComment(CommentPlacement placement) const {
        return comments_.get(placement);
    }

    void Value::setOffsetStart(ptrdiff_t start) { start_ = start; }

    void Value::setOffsetLimit(ptrdiff_t limit) { limit_ = limit; }

    ptrdiff_t Value::getOffsetStart() const { return start_; }

    ptrdiff_t Value::getOffsetLimit() const { return limit_; }

    String Value::toStyledString() const {
        StreamWriterBuilder builder;

        String out = this->hasComment(commentBefore) ? "\n" : "";
        out += Json::writeString(builder, *this);
        out += '\n';

        return out;
    }

    Value::const_iterator Value::begin() const {
        switch (type()) {
        case arrayValue:
        case objectValue:
            if (value_.map_)
                return const_iterator(value_.map_->begin());
            break;
        default:
            break;
        }
        return {};
    }

    Value::const_iterator Value::end() const {
        switch (type()) {
        case arrayValue:
        case objectValue:
            if (value_.map_)
                return const_iterator(value_.map_->end());
            break;
        default:
            break;
        }
        return {};
    }

    Value::iterator Value::begin() {
        switch (type()) {
        case arrayValue:
        case objectValue:
            if (value_.map_)
                return iterator(value_.map_->begin());
            break;
        default:
            break;
        }
        return iterator();
    }

    Value::iterator Value::end() {
        switch (type()) {
        case arrayValue:
        case objectValue:
            if (value_.map_)
                return iterator(value_.map_->end());
            break;
        default:
            break;
        }
        return iterator();
    }

    // class PathArgument
    // //////////////////////////////////////////////////////////////////

    PathArgument::PathArgument() = default;

    PathArgument::PathArgument(ArrayIndex index)
        : index_(index), kind_(kindIndex) {}

    PathArgument::PathArgument(const char* key) : key_(key), kind_(kindKey) {}

    PathArgument::PathArgument(String key) : key_(std::move(key)), kind_(kindKey) {}

    // class Path
    // //////////////////////////////////////////////////////////////////

    Path::Path(const String& path, const PathArgument& a1, const PathArgument& a2,
        const PathArgument& a3, const PathArgument& a4,
        const PathArgument& a5) {
        InArgs in;
        in.reserve(5);
        in.push_back(&a1);
        in.push_back(&a2);
        in.push_back(&a3);
        in.push_back(&a4);
        in.push_back(&a5);
        makePath(path, in);
    }

    void Path::makePath(const String& path, const InArgs& in) {
        const char* current = path.c_str();
        const char* end = current + path.length();
        auto itInArg = in.begin();
        while (current != end) {
            if (*current == '[') {
                ++current;
                if (*current == '%')
                    addPathInArg(path, in, itInArg, PathArgument::kindIndex);
                else {
                    ArrayIndex index = 0;
                    for (; current != end && *current >= '0' && *current <= '9'; ++current)
                        index = index * 10 + ArrayIndex(*current - '0');
                    args_.push_back(index);
                }
                if (current == end || *++current != ']')
                    invalidPath(path, int(current - path.c_str()));
            } else if (*current == '%') {
                addPathInArg(path, in, itInArg, PathArgument::kindKey);
                ++current;
            } else if (*current == '.' || *current == ']') {
                ++current;
            } else {
                const char* beginName = current;
                while (current != end && !strchr("[.", *current))
                    ++current;
                args_.push_back(String(beginName, current));
            }
        }
    }

    void Path::addPathInArg(const String& /*path*/, const InArgs& in,
        InArgs::const_iterator& itInArg,
        PathArgument::Kind kind) {
        if (itInArg == in.end()) {
            // Error: missing argument %d
        } else if ((*itInArg)->kind_ != kind) {
            // Error: bad argument type
        } else {
            args_.push_back(**itInArg++);
        }
    }

    void Path::invalidPath(const String& /*path*/, int /*location*/) {
        // Error: invalid path.
    }

    const Value& Path::resolve(const Value& root) const {
        const Value* node = &root;
        for (const auto& arg : args_) {
            if (arg.kind_ == PathArgument::kindIndex) {
                if (!node->isArray() || !node->isValidIndex(arg.index_)) {
                    // Error: unable to resolve path (array value expected at position... )
                    return Value::nullSingleton();
                }
                node = &((*node)[arg.index_]);
            } else if (arg.kind_ == PathArgument::kindKey) {
                if (!node->isObject()) {
                    // Error: unable to resolve path (object value expected at position...)
                    return Value::nullSingleton();
                }
                node = &((*node)[arg.key_]);
                if (node == &Value::nullSingleton()) {
                    // Error: unable to resolve path (object has no member named '' at
                    // position...)
                    return Value::nullSingleton();
                }
            }
        }
        return *node;
    }

    Value Path::resolve(const Value& root, const Value& defaultValue) const {
        const Value* node = &root;
        for (const auto& arg : args_) {
            if (arg.kind_ == PathArgument::kindIndex) {
                if (!node->isArray() || !node->isValidIndex(arg.index_))
                    return defaultValue;
                node = &((*node)[arg.index_]);
            } else if (arg.kind_ == PathArgument::kindKey) {
                if (!node->isObject())
                    return defaultValue;
                node = &((*node)[arg.key_]);
                if (node == &Value::nullSingleton())
                    return defaultValue;
            }
        }
        return *node;
    }

    Value& Path::make(Value& root) const {
        Value* node = &root;
        for (const auto& arg : args_) {
            if (arg.kind_ == PathArgument::kindIndex) {
                if (!node->isArray()) {
                    // Error: node is not an array at position ...
                }
                node = &((*node)[arg.index_]);
            } else if (arg.kind_ == PathArgument::kindKey) {
                if (!node->isObject()) {
                    // Error: node is not an object at position...
                }
                node = &((*node)[arg.key_]);
            }
        }
        return *node;
    }

} // namespace Json

// //////////////////////////////////////////////////////////////////////
// End of content of file: src/lib_json/json_value.cpp
// //////////////////////////////////////////////////////////////////////






// //////////////////////////////////////////////////////////////////////
// Beginning of content of file: src/lib_json/json_writer.cpp
// //////////////////////////////////////////////////////////////////////

// Copyright 2011 Baptiste Lepilleur and The JsonCpp Authors
// Distributed under MIT license, or public domain if desired and
// recognized in your jurisdiction.
// See file LICENSE for detail or copy at http://jsoncpp.sourceforge.net/LICENSE

#if !defined(JSON_IS_AMALGAMATION)
#include "json_tool.h"
#include <json/writer.h>
#endif // if !defined(JSON_IS_AMALGAMATION)
#include <cassert>
#include <cstring>
#include <iomanip>
#include <memory>
#include <set>
#include <sstream>
#include <utility>

#if __cplusplus >= 201103L
#include <cmath>
#include <cstdio>

#if !defined(isnan)
#define isnan std::isnan
#endif

#if !defined(isfinite)
#define isfinite std::isfinite
#endif

#else
#include <cmath>
#include <cstdio>

#if defined(_MSC_VER)
#if !defined(isnan)
#include <float.h>
#define isnan _isnan
#endif

#if !defined(isfinite)
#include <float.h>
#define isfinite _finite
#endif

#if !defined(_CRT_SECURE_CPP_OVERLOAD_STANDARD_NAMES)
#define _CRT_SECURE_CPP_OVERLOAD_STANDARD_NAMES 1
#endif //_CRT_SECURE_CPP_OVERLOAD_STANDARD_NAMES

#endif //_MSC_VER

#if defined(__sun) && defined(__SVR4) // Solaris
#if !defined(isfinite)
#include <ieeefp.h>
#define isfinite finite
#endif
#endif

#if defined(__hpux)
#if !defined(isfinite)
#if defined(__ia64) && !defined(finite)
#define isfinite(x)                                                            \
  ((sizeof(x) == sizeof(float) ? _Isfinitef(x) : _IsFinite(x)))
#endif
#endif
#endif

#if !defined(isnan)
// IEEE standard states that NaN values will not compare to themselves
#define isnan(x) (x != x)
#endif

#if !defined(__APPLE__)
#if !defined(isfinite)
#define isfinite finite
#endif
#endif
#endif

#if defined(_MSC_VER)
// Disable warning about strdup being deprecated.
#pragma warning(disable : 4996)
#endif

namespace Json {

#if __cplusplus >= 201103L || (defined(_CPPLIB_VER) && _CPPLIB_VER >= 520)
    using StreamWriterPtr = std::unique_ptr<StreamWriter>;
#else
    using StreamWriterPtr = std::auto_ptr<StreamWriter>;
#endif

    String valueToString(LargestInt value) {
        UIntToStringBuffer buffer;
        char* current = buffer + sizeof(buffer);
        if (value == Value::minLargestInt) {
            uintToString(LargestUInt(Value::maxLargestInt) + 1, current);
            *--current = '-';
        } else if (value < 0) {
            uintToString(LargestUInt(-value), current);
            *--current = '-';
        } else {
            uintToString(LargestUInt(value), current);
        }
        assert(current >= buffer);
        return current;
    }

    String valueToString(LargestUInt value) {
        UIntToStringBuffer buffer;
        char* current = buffer + sizeof(buffer);
        uintToString(value, current);
        assert(current >= buffer);
        return current;
    }

#if defined(JSON_HAS_INT64)

    String valueToString(Int value) { return valueToString(LargestInt(value)); }

    String valueToString(UInt value) { return valueToString(LargestUInt(value)); }

#endif // # if defined(JSON_HAS_INT64)

    namespace {
        String valueToString(double value, bool useSpecialFloats,
            unsigned int precision, PrecisionType precisionType) {
            // Print into the buffer. We need not request the alternative representation
            // that always has a decimal point because JSON doesn't distinguish the
            // concepts of reals and integers.
            if (!isfinite(value)) {
                static const char* const reps[2][3] = { {"NaN", "-Infinity", "Infinity"},
                                                       {"null", "-1e+9999", "1e+9999"} };
                return reps[useSpecialFloats ? 0 : 1]
                    [isnan(value) ? 0 : (value < 0) ? 1 : 2];
            }

            String buffer(size_t(36), '\0');
            while (true) {
                int len = jsoncpp_snprintf(
                    &*buffer.begin(), buffer.size(),
                    (precisionType == PrecisionType::significantDigits) ? "%.*g" : "%.*f",
                    precision, value);
                assert(len >= 0);
                auto wouldPrint = static_cast<size_t>(len);
                if (wouldPrint >= buffer.size()) {
                    buffer.resize(wouldPrint + 1);
                    continue;
                }
                buffer.resize(wouldPrint);
                break;
            }

            buffer.erase(fixNumericLocale(buffer.begin(), buffer.end()), buffer.end());

            // strip the zero padding from the right
            if (precisionType == PrecisionType::decimalPlaces) {
                buffer.erase(fixZerosInTheEnd(buffer.begin(), buffer.end()), buffer.end());
            }

            // try to ensure we preserve the fact that this was given to us as a double on
            // input
            if (buffer.find('.') == buffer.npos && buffer.find('e') == buffer.npos) {
                buffer += ".0";
            }
            return buffer;
        }
    } // namespace

    String valueToString(double value, unsigned int precision,
        PrecisionType precisionType) {
        return valueToString(value, false, precision, precisionType);
    }

    String valueToString(bool value) { return value ? "true" : "false"; }

    static bool isAnyCharRequiredQuoting(char const* s, size_t n) {
        assert(s || !n);

        char const* const end = s + n;
        for (char const* cur = s; cur < end; ++cur) {
            if (*cur == '\\' || *cur == '\"' ||
                static_cast<unsigned char>(*cur) < ' ' ||
                static_cast<unsigned char>(*cur) >= 0x80)
                return true;
        }
        return false;
    }

    static unsigned int utf8ToCodepoint(const char*& s, const char* e) {
        const unsigned int REPLACEMENT_CHARACTER = 0xFFFD;

        unsigned int firstByte = static_cast<unsigned char>(*s);

        if (firstByte < 0x80)
            return firstByte;

        if (firstByte < 0xE0) {
            if (e - s < 2)
                return REPLACEMENT_CHARACTER;

            unsigned int calculated =
                ((firstByte & 0x1F) << 6) | (static_cast<unsigned int>(s[1]) & 0x3F);
            s += 1;
            // oversized encoded characters are invalid
            return calculated < 0x80 ? REPLACEMENT_CHARACTER : calculated;
        }

        if (firstByte < 0xF0) {
            if (e - s < 3)
                return REPLACEMENT_CHARACTER;

            unsigned int calculated = ((firstByte & 0x0F) << 12) |
                ((static_cast<unsigned int>(s[1]) & 0x3F) << 6) |
                (static_cast<unsigned int>(s[2]) & 0x3F);
            s += 2;
            // surrogates aren't valid codepoints itself
            // shouldn't be UTF-8 encoded
            if (calculated >= 0xD800 && calculated <= 0xDFFF)
                return REPLACEMENT_CHARACTER;
            // oversized encoded characters are invalid
            return calculated < 0x800 ? REPLACEMENT_CHARACTER : calculated;
        }

        if (firstByte < 0xF8) {
            if (e - s < 4)
                return REPLACEMENT_CHARACTER;

            unsigned int calculated = ((firstByte & 0x07) << 18) |
                ((static_cast<unsigned int>(s[1]) & 0x3F) << 12) |
                ((static_cast<unsigned int>(s[2]) & 0x3F) << 6) |
                (static_cast<unsigned int>(s[3]) & 0x3F);
            s += 3;
            // oversized encoded characters are invalid
            return calculated < 0x10000 ? REPLACEMENT_CHARACTER : calculated;
        }

        return REPLACEMENT_CHARACTER;
    }

    static const char hex2[] = "000102030405060708090a0b0c0d0e0f"
        "101112131415161718191a1b1c1d1e1f"
        "202122232425262728292a2b2c2d2e2f"
        "303132333435363738393a3b3c3d3e3f"
        "404142434445464748494a4b4c4d4e4f"
        "505152535455565758595a5b5c5d5e5f"
        "606162636465666768696a6b6c6d6e6f"
        "707172737475767778797a7b7c7d7e7f"
        "808182838485868788898a8b8c8d8e8f"
        "909192939495969798999a9b9c9d9e9f"
        "a0a1a2a3a4a5a6a7a8a9aaabacadaeaf"
        "b0b1b2b3b4b5b6b7b8b9babbbcbdbebf"
        "c0c1c2c3c4c5c6c7c8c9cacbcccdcecf"
        "d0d1d2d3d4d5d6d7d8d9dadbdcdddedf"
        "e0e1e2e3e4e5e6e7e8e9eaebecedeeef"
        "f0f1f2f3f4f5f6f7f8f9fafbfcfdfeff";

    static String toHex16Bit(unsigned int x) {
        const unsigned int hi = (x >> 8) & 0xff;
        const unsigned int lo = x & 0xff;
        String result(4, ' ');
        result[0] = hex2[2 * hi];
        result[1] = hex2[2 * hi + 1];
        result[2] = hex2[2 * lo];
        result[3] = hex2[2 * lo + 1];
        return result;
    }

    static String valueToQuotedStringN(const char* value, unsigned length,
        bool emitUTF8 = false) {
        if (value == nullptr)
            return "";

        if (!isAnyCharRequiredQuoting(value, length))
            return String("\"") + value + "\"";
        // We have to walk value and escape any special characters.
        // Appending to String is not efficient, but this should be rare.
        // (Note: forward slashes are *not* rare, but I am not escaping them.)
        String::size_type maxsize = length * 2 + 3; // allescaped+quotes+NULL
        String result;
        result.reserve(maxsize); // to avoid lots of mallocs
        result += "\"";
        char const* end = value + length;
        for (const char* c = value; c != end; ++c) {
            switch (*c) {
            case '\"':
                result += "\\\"";
                break;
            case '\\':
                result += "\\\\";
                break;
            case '\b':
                result += "\\b";
                break;
            case '\f':
                result += "\\f";
                break;
            case '\n':
                result += "\\n";
                break;
            case '\r':
                result += "\\r";
                break;
            case '\t':
                result += "\\t";
                break;
                // case '/':
                // Even though \/ is considered a legal escape in JSON, a bare
                // slash is also legal, so I see no reason to escape it.
                // (I hope I am not misunderstanding something.)
                // blep notes: actually escaping \/ may be useful in javascript to avoid </
                // sequence.
                // Should add a flag to allow this compatibility mode and prevent this
                // sequence from occurring.
            default: {
                if (emitUTF8) {
                    result += *c;
                } else {
                    unsigned int codepoint = utf8ToCodepoint(c, end);
                    const unsigned int FIRST_NON_CONTROL_CODEPOINT = 0x20;
                    const unsigned int LAST_NON_CONTROL_CODEPOINT = 0x7F;
                    const unsigned int FIRST_SURROGATE_PAIR_CODEPOINT = 0x10000;
                    // don't escape non-control characters
                    // (short escape sequence are applied above)
                    if (FIRST_NON_CONTROL_CODEPOINT <= codepoint &&
                        codepoint <= LAST_NON_CONTROL_CODEPOINT) {
                        result += static_cast<char>(codepoint);
                    } else if (codepoint <
                        FIRST_SURROGATE_PAIR_CODEPOINT) { // codepoint is in Basic
                                                          // Multilingual Plane
                        result += "\\u";
                        result += toHex16Bit(codepoint);
                    } else { // codepoint is not in Basic Multilingual Plane
                             // convert to surrogate pair first
                        codepoint -= FIRST_SURROGATE_PAIR_CODEPOINT;
                        result += "\\u";
                        result += toHex16Bit((codepoint >> 10) + 0xD800);
                        result += "\\u";
                        result += toHex16Bit((codepoint & 0x3FF) + 0xDC00);
                    }
                }
            } break;
            }
        }
        result += "\"";
        return result;
    }

    String valueToQuotedString(const char* value) {
        return valueToQuotedStringN(value, static_cast<unsigned int>(strlen(value)));
    }

    // Class Writer
    // //////////////////////////////////////////////////////////////////
    Writer::~Writer() = default;

    // Class FastWriter
    // //////////////////////////////////////////////////////////////////

    FastWriter::FastWriter()

        = default;

    void FastWriter::enableYAMLCompatibility() { yamlCompatibilityEnabled_ = true; }

    void FastWriter::dropNullPlaceholders() { dropNullPlaceholders_ = true; }

    void FastWriter::omitEndingLineFeed() { omitEndingLineFeed_ = true; }

    String FastWriter::write(const Value& root) {
        document_.clear();
        writeValue(root);
        if (!omitEndingLineFeed_)
            document_ += '\n';
        return document_;
    }

    void FastWriter::writeValue(const Value& value) {
        switch (value.type()) {
        case nullValue:
            if (!dropNullPlaceholders_)
                document_ += "null";
            break;
        case intValue:
            document_ += valueToString(value.asLargestInt());
            break;
        case uintValue:
            document_ += valueToString(value.asLargestUInt());
            break;
        case realValue:
            document_ += valueToString(value.asDouble());
            break;
        case stringValue: {
            // Is NULL possible for value.string_? No.
            char const* str;
            char const* end;
            bool ok = value.getString(&str, &end);
            if (ok)
                document_ += valueToQuotedStringN(str, static_cast<unsigned>(end - str));
            break;
        }
        case booleanValue:
            document_ += valueToString(value.asBool());
            break;
        case arrayValue: {
            document_ += '[';
            ArrayIndex size = value.size();
            for (ArrayIndex index = 0; index < size; ++index) {
                if (index > 0)
                    document_ += ',';
                writeValue(value[index]);
            }
            document_ += ']';
        } break;
        case objectValue: {
            Value::Members members(value.getMemberNames());
            document_ += '{';
            for (auto it = members.begin(); it != members.end(); ++it) {
                const String& name = *it;
                if (it != members.begin())
                    document_ += ',';
                document_ += valueToQuotedStringN(name.data(),
                    static_cast<unsigned>(name.length()));
                document_ += yamlCompatibilityEnabled_ ? ": " : ":";
                writeValue(value[name]);
            }
            document_ += '}';
        } break;
        }
    }

    // Class StyledWriter
    // //////////////////////////////////////////////////////////////////

    StyledWriter::StyledWriter() = default;

    String StyledWriter::write(const Value& root) {
        document_.clear();
        addChildValues_ = false;
        indentString_.clear();
        writeCommentBeforeValue(root);
        writeValue(root);
        writeCommentAfterValueOnSameLine(root);
        document_ += '\n';
        return document_;
    }

    void StyledWriter::writeValue(const Value& value) {
        switch (value.type()) {
        case nullValue:
            pushValue("null");
            break;
        case intValue:
            pushValue(valueToString(value.asLargestInt()));
            break;
        case uintValue:
            pushValue(valueToString(value.asLargestUInt()));
            break;
        case realValue:
            pushValue(valueToString(value.asDouble()));
            break;
        case stringValue: {
            // Is NULL possible for value.string_? No.
            char const* str;
            char const* end;
            bool ok = value.getString(&str, &end);
            if (ok)
                pushValue(valueToQuotedStringN(str, static_cast<unsigned>(end - str)));
            else
                pushValue("");
            break;
        }
        case booleanValue:
            pushValue(valueToString(value.asBool()));
            break;
        case arrayValue:
            writeArrayValue(value);
            break;
        case objectValue: {
            Value::Members members(value.getMemberNames());
            if (members.empty())
                pushValue("{}");
            else {
                writeWithIndent("{");
                indent();
                auto it = members.begin();
                for (;;) {
                    const String& name = *it;
                    const Value& childValue = value[name];
                    writeCommentBeforeValue(childValue);
                    writeWithIndent(valueToQuotedString(name.c_str()));
                    document_ += " : ";
                    writeValue(childValue);
                    if (++it == members.end()) {
                        writeCommentAfterValueOnSameLine(childValue);
                        break;
                    }
                    document_ += ',';
                    writeCommentAfterValueOnSameLine(childValue);
                }
                unindent();
                writeWithIndent("}");
            }
        } break;
        }
    }

    void StyledWriter::writeArrayValue(const Value& value) {
        unsigned size = value.size();
        if (size == 0)
            pushValue("[]");
        else {
            bool isArrayMultiLine = isMultilineArray(value);
            if (isArrayMultiLine) {
                writeWithIndent("[");
                indent();
                bool hasChildValue = !childValues_.empty();
                unsigned index = 0;
                for (;;) {
                    const Value& childValue = value[index];
                    writeCommentBeforeValue(childValue);
                    if (hasChildValue)
                        writeWithIndent(childValues_[index]);
                    else {
                        writeIndent();
                        writeValue(childValue);
                    }
                    if (++index == size) {
                        writeCommentAfterValueOnSameLine(childValue);
                        break;
                    }
                    document_ += ',';
                    writeCommentAfterValueOnSameLine(childValue);
                }
                unindent();
                writeWithIndent("]");
            } else // output on a single line
            {
                assert(childValues_.size() == size);
                document_ += "[ ";
                for (unsigned index = 0; index < size; ++index) {
                    if (index > 0)
                        document_ += ", ";
                    document_ += childValues_[index];
                }
                document_ += " ]";
            }
        }
    }

    bool StyledWriter::isMultilineArray(const Value& value) {
        ArrayIndex const size = value.size();
        bool isMultiLine = size * 3 >= rightMargin_;
        childValues_.clear();
        for (ArrayIndex index = 0; index < size && !isMultiLine; ++index) {
            const Value& childValue = value[index];
            isMultiLine = ((childValue.isArray() || childValue.isObject()) &&
                !childValue.empty());
        }
        if (!isMultiLine) // check if line length > max line length
        {
            childValues_.reserve(size);
            addChildValues_ = true;
            ArrayIndex lineLength = 4 + (size - 1) * 2; // '[ ' + ', '*n + ' ]'
            for (ArrayIndex index = 0; index < size; ++index) {
                if (hasCommentForValue(value[index])) {
                    isMultiLine = true;
                }
                writeValue(value[index]);
                lineLength += static_cast<ArrayIndex>(childValues_[index].length());
            }
            addChildValues_ = false;
            isMultiLine = isMultiLine || lineLength >= rightMargin_;
        }
        return isMultiLine;
    }

    void StyledWriter::pushValue(const String& value) {
        if (addChildValues_)
            childValues_.push_back(value);
        else
            document_ += value;
    }

    void StyledWriter::writeIndent() {
        if (!document_.empty()) {
            char last = document_[document_.length() - 1];
            if (last == ' ') // already indented
                return;
            if (last != '\n') // Comments may add new-line
                document_ += '\n';
        }
        document_ += indentString_;
    }

    void StyledWriter::writeWithIndent(const String& value) {
        writeIndent();
        document_ += value;
    }

    void StyledWriter::indent() { indentString_ += String(indentSize_, ' '); }

    void StyledWriter::unindent() {
        assert(indentString_.size() >= indentSize_);
        indentString_.resize(indentString_.size() - indentSize_);
    }

    void StyledWriter::writeCommentBeforeValue(const Value& root) {
        if (!root.hasComment(commentBefore))
            return;

        document_ += '\n';
        writeIndent();
        const String& comment = root.getComment(commentBefore);
        String::const_iterator iter = comment.begin();
        while (iter != comment.end()) {
            document_ += *iter;
            if (*iter == '\n' && ((iter + 1) != comment.end() && *(iter + 1) == '/'))
                writeIndent();
            ++iter;
        }

        // Comments are stripped of trailing newlines, so add one here
        document_ += '\n';
    }

    void StyledWriter::writeCommentAfterValueOnSameLine(const Value& root) {
        if (root.hasComment(commentAfterOnSameLine))
            document_ += " " + root.getComment(commentAfterOnSameLine);

        if (root.hasComment(commentAfter)) {
            document_ += '\n';
            document_ += root.getComment(commentAfter);
            document_ += '\n';
        }
    }

    bool StyledWriter::hasCommentForValue(const Value& value) {
        return value.hasComment(commentBefore) ||
            value.hasComment(commentAfterOnSameLine) ||
            value.hasComment(commentAfter);
    }

    // Class StyledStreamWriter
    // //////////////////////////////////////////////////////////////////

    StyledStreamWriter::StyledStreamWriter(String indentation)
        : document_(nullptr), indentation_(std::move(indentation)),
        addChildValues_(), indented_(false) {}

    void StyledStreamWriter::write(OStream& out, const Value& root) {
        document_ = &out;
        addChildValues_ = false;
        indentString_.clear();
        indented_ = true;
        writeCommentBeforeValue(root);
        if (!indented_)
            writeIndent();
        indented_ = true;
        writeValue(root);
        writeCommentAfterValueOnSameLine(root);
        *document_ << "\n";
        document_ = nullptr; // Forget the stream, for safety.
    }

    void StyledStreamWriter::writeValue(const Value& value) {
        switch (value.type()) {
        case nullValue:
            pushValue("null");
            break;
        case intValue:
            pushValue(valueToString(value.asLargestInt()));
            break;
        case uintValue:
            pushValue(valueToString(value.asLargestUInt()));
            break;
        case realValue:
            pushValue(valueToString(value.asDouble()));
            break;
        case stringValue: {
            // Is NULL possible for value.string_? No.
            char const* str;
            char const* end;
            bool ok = value.getString(&str, &end);
            if (ok)
                pushValue(valueToQuotedStringN(str, static_cast<unsigned>(end - str)));
            else
                pushValue("");
            break;
        }
        case booleanValue:
            pushValue(valueToString(value.asBool()));
            break;
        case arrayValue:
            writeArrayValue(value);
            break;
        case objectValue: {
            Value::Members members(value.getMemberNames());
            if (members.empty())
                pushValue("{}");
            else {
                writeWithIndent("{");
                indent();
                auto it = members.begin();
                for (;;) {
                    const String& name = *it;
                    const Value& childValue = value[name];
                    writeCommentBeforeValue(childValue);
                    writeWithIndent(valueToQuotedString(name.c_str()));
                    *document_ << " : ";
                    writeValue(childValue);
                    if (++it == members.end()) {
                        writeCommentAfterValueOnSameLine(childValue);
                        break;
                    }
                    *document_ << ",";
                    writeCommentAfterValueOnSameLine(childValue);
                }
                unindent();
                writeWithIndent("}");
            }
        } break;
        }
    }

    void StyledStreamWriter::writeArrayValue(const Value& value) {
        unsigned size = value.size();
        if (size == 0)
            pushValue("[]");
        else {
            bool isArrayMultiLine = isMultilineArray(value);
            if (isArrayMultiLine) {
                writeWithIndent("[");
                indent();
                bool hasChildValue = !childValues_.empty();
                unsigned index = 0;
                for (;;) {
                    const Value& childValue = value[index];
                    writeCommentBeforeValue(childValue);
                    if (hasChildValue)
                        writeWithIndent(childValues_[index]);
                    else {
                        if (!indented_)
                            writeIndent();
                        indented_ = true;
                        writeValue(childValue);
                        indented_ = false;
                    }
                    if (++index == size) {
                        writeCommentAfterValueOnSameLine(childValue);
                        break;
                    }
                    *document_ << ",";
                    writeCommentAfterValueOnSameLine(childValue);
                }
                unindent();
                writeWithIndent("]");
            } else // output on a single line
            {
                assert(childValues_.size() == size);
                *document_ << "[ ";
                for (unsigned index = 0; index < size; ++index) {
                    if (index > 0)
                        *document_ << ", ";
                    *document_ << childValues_[index];
                }
                *document_ << " ]";
            }
        }
    }

    bool StyledStreamWriter::isMultilineArray(const Value& value) {
        ArrayIndex const size = value.size();
        bool isMultiLine = size * 3 >= rightMargin_;
        childValues_.clear();
        for (ArrayIndex index = 0; index < size && !isMultiLine; ++index) {
            const Value& childValue = value[index];
            isMultiLine = ((childValue.isArray() || childValue.isObject()) &&
                !childValue.empty());
        }
        if (!isMultiLine) // check if line length > max line length
        {
            childValues_.reserve(size);
            addChildValues_ = true;
            ArrayIndex lineLength = 4 + (size - 1) * 2; // '[ ' + ', '*n + ' ]'
            for (ArrayIndex index = 0; index < size; ++index) {
                if (hasCommentForValue(value[index])) {
                    isMultiLine = true;
                }
                writeValue(value[index]);
                lineLength += static_cast<ArrayIndex>(childValues_[index].length());
            }
            addChildValues_ = false;
            isMultiLine = isMultiLine || lineLength >= rightMargin_;
        }
        return isMultiLine;
    }

    void StyledStreamWriter::pushValue(const String& value) {
        if (addChildValues_)
            childValues_.push_back(value);
        else
            *document_ << value;
    }

    void StyledStreamWriter::writeIndent() {
        // blep intended this to look at the so-far-written string
        // to determine whether we are already indented, but
        // with a stream we cannot do that. So we rely on some saved state.
        // The caller checks indented_.
        *document_ << '\n' << indentString_;
    }

    void StyledStreamWriter::writeWithIndent(const String& value) {
        if (!indented_)
            writeIndent();
        *document_ << value;
        indented_ = false;
    }

    void StyledStreamWriter::indent() { indentString_ += indentation_; }

    void StyledStreamWriter::unindent() {
        assert(indentString_.size() >= indentation_.size());
        indentString_.resize(indentString_.size() - indentation_.size());
    }

    void StyledStreamWriter::writeCommentBeforeValue(const Value& root) {
        if (!root.hasComment(commentBefore))
            return;

        if (!indented_)
            writeIndent();
        const String& comment = root.getComment(commentBefore);
        String::const_iterator iter = comment.begin();
        while (iter != comment.end()) {
            *document_ << *iter;
            if (*iter == '\n' && ((iter + 1) != comment.end() && *(iter + 1) == '/'))
                // writeIndent();  // would include newline
                *document_ << indentString_;
            ++iter;
        }
        indented_ = false;
    }

    void StyledStreamWriter::writeCommentAfterValueOnSameLine(const Value& root) {
        if (root.hasComment(commentAfterOnSameLine))
            *document_ << ' ' << root.getComment(commentAfterOnSameLine);

        if (root.hasComment(commentAfter)) {
            writeIndent();
            *document_ << root.getComment(commentAfter);
        }
        indented_ = false;
    }

    bool StyledStreamWriter::hasCommentForValue(const Value& value) {
        return value.hasComment(commentBefore) ||
            value.hasComment(commentAfterOnSameLine) ||
            value.hasComment(commentAfter);
    }

    //////////////////////////
    // BuiltStyledStreamWriter

    /// Scoped enums are not available until C++11.
    struct CommentStyle {
        /// Decide whether to write comments.
        enum Enum {
            None, ///< Drop all comments.
            Most, ///< Recover odd behavior of previous versions (not implemented yet).
            All   ///< Keep all comments.
        };
    };

    struct BuiltStyledStreamWriter : public StreamWriter {
        BuiltStyledStreamWriter(String indentation, CommentStyle::Enum cs,
            String colonSymbol, String nullSymbol,
            String endingLineFeedSymbol, bool useSpecialFloats,
            bool emitUTF8, unsigned int precision,
            PrecisionType precisionType);
        int write(Value const& root, OStream* sout) override;

    private:
        void writeValue(Value const& value);
        void writeArrayValue(Value const& value);
        bool isMultilineArray(Value const& value);
        void pushValue(String const& value);
        void writeIndent();
        void writeWithIndent(String const& value);
        void indent();
        void unindent();
        void writeCommentBeforeValue(Value const& root);
        void writeCommentAfterValueOnSameLine(Value const& root);
        static bool hasCommentForValue(const Value& value);

        using ChildValues = std::vector<String>;

        ChildValues childValues_;
        String indentString_;
        unsigned int rightMargin_;
        String indentation_;
        CommentStyle::Enum cs_;
        String colonSymbol_;
        String nullSymbol_;
        String endingLineFeedSymbol_;
        bool addChildValues_ : 1;
        bool indented_ : 1;
        bool useSpecialFloats_ : 1;
        bool emitUTF8_ : 1;
        unsigned int precision_;
        PrecisionType precisionType_;
    };
    BuiltStyledStreamWriter::BuiltStyledStreamWriter(
        String indentation, CommentStyle::Enum cs, String colonSymbol,
        String nullSymbol, String endingLineFeedSymbol, bool useSpecialFloats,
        bool emitUTF8, unsigned int precision, PrecisionType precisionType)
        : rightMargin_(74), indentation_(std::move(indentation)), cs_(cs),
        colonSymbol_(std::move(colonSymbol)), nullSymbol_(std::move(nullSymbol)),
        endingLineFeedSymbol_(std::move(endingLineFeedSymbol)),
        addChildValues_(false), indented_(false),
        useSpecialFloats_(useSpecialFloats), emitUTF8_(emitUTF8),
        precision_(precision), precisionType_(precisionType) {}
    int BuiltStyledStreamWriter::write(Value const& root, OStream* sout) {
        sout_ = sout;
        addChildValues_ = false;
        indented_ = true;
        indentString_.clear();
        writeCommentBeforeValue(root);
        if (!indented_)
            writeIndent();
        indented_ = true;
        writeValue(root);
        writeCommentAfterValueOnSameLine(root);
        *sout_ << endingLineFeedSymbol_;
        sout_ = nullptr;
        return 0;
    }
    void BuiltStyledStreamWriter::writeValue(Value const& value) {
        switch (value.type()) {
        case nullValue:
            pushValue(nullSymbol_);
            break;
        case intValue:
            pushValue(valueToString(value.asLargestInt()));
            break;
        case uintValue:
            pushValue(valueToString(value.asLargestUInt()));
            break;
        case realValue:
            pushValue(valueToString(value.asDouble(), useSpecialFloats_, precision_,
                precisionType_));
            break;
        case stringValue: {
            // Is NULL is possible for value.string_? No.
            char const* str;
            char const* end;
            bool ok = value.getString(&str, &end);
            if (ok)
                pushValue(valueToQuotedStringN(str, static_cast<unsigned>(end - str),
                    emitUTF8_));
            else
                pushValue("");
            break;
        }
        case booleanValue:
            pushValue(valueToString(value.asBool()));
            break;
        case arrayValue:
            writeArrayValue(value);
            break;
        case objectValue: {
            Value::Members members(value.getMemberNames());
            if (members.empty())
                pushValue("{}");
            else {
                writeWithIndent("{");
                indent();
                auto it = members.begin();
                for (;;) {
                    String const& name = *it;
                    Value const& childValue = value[name];
                    writeCommentBeforeValue(childValue);
                    writeWithIndent(valueToQuotedStringN(
                        name.data(), static_cast<unsigned>(name.length()), emitUTF8_));
                    *sout_ << colonSymbol_;
                    writeValue(childValue);
                    if (++it == members.end()) {
                        writeCommentAfterValueOnSameLine(childValue);
                        break;
                    }
                    *sout_ << ",";
                    writeCommentAfterValueOnSameLine(childValue);
                }
                unindent();
                writeWithIndent("}");
            }
        } break;
        }
    }

    void BuiltStyledStreamWriter::writeArrayValue(Value const& value) {
        unsigned size = value.size();
        if (size == 0)
            pushValue("[]");
        else {
            bool isMultiLine = (cs_ == CommentStyle::All) || isMultilineArray(value);
            if (isMultiLine) {
                writeWithIndent("[");
                indent();
                bool hasChildValue = !childValues_.empty();
                unsigned index = 0;
                for (;;) {
                    Value const& childValue = value[index];
                    writeCommentBeforeValue(childValue);
                    if (hasChildValue)
                        writeWithIndent(childValues_[index]);
                    else {
                        if (!indented_)
                            writeIndent();
                        indented_ = true;
                        writeValue(childValue);
                        indented_ = false;
                    }
                    if (++index == size) {
                        writeCommentAfterValueOnSameLine(childValue);
                        break;
                    }
                    *sout_ << ",";
                    writeCommentAfterValueOnSameLine(childValue);
                }
                unindent();
                writeWithIndent("]");
            } else // output on a single line
            {
                assert(childValues_.size() == size);
                *sout_ << "[";
                if (!indentation_.empty())
                    *sout_ << " ";
                for (unsigned index = 0; index < size; ++index) {
                    if (index > 0)
                        *sout_ << ((!indentation_.empty()) ? ", " : ",");
                    *sout_ << childValues_[index];
                }
                if (!indentation_.empty())
                    *sout_ << " ";
                *sout_ << "]";
            }
        }
    }

    bool BuiltStyledStreamWriter::isMultilineArray(Value const& value) {
        ArrayIndex const size = value.size();
        bool isMultiLine = size * 3 >= rightMargin_;
        childValues_.clear();
        for (ArrayIndex index = 0; index < size && !isMultiLine; ++index) {
            Value const& childValue = value[index];
            isMultiLine = ((childValue.isArray() || childValue.isObject()) &&
                !childValue.empty());
        }
        if (!isMultiLine) // check if line length > max line length
        {
            childValues_.reserve(size);
            addChildValues_ = true;
            ArrayIndex lineLength = 4 + (size - 1) * 2; // '[ ' + ', '*n + ' ]'
            for (ArrayIndex index = 0; index < size; ++index) {
                if (hasCommentForValue(value[index])) {
                    isMultiLine = true;
                }
                writeValue(value[index]);
                lineLength += static_cast<ArrayIndex>(childValues_[index].length());
            }
            addChildValues_ = false;
            isMultiLine = isMultiLine || lineLength >= rightMargin_;
        }
        return isMultiLine;
    }

    void BuiltStyledStreamWriter::pushValue(String const& value) {
        if (addChildValues_)
            childValues_.push_back(value);
        else
            *sout_ << value;
    }

    void BuiltStyledStreamWriter::writeIndent() {
        // blep intended this to look at the so-far-written string
        // to determine whether we are already indented, but
        // with a stream we cannot do that. So we rely on some saved state.
        // The caller checks indented_.

        if (!indentation_.empty()) {
            // In this case, drop newlines too.
            *sout_ << '\n' << indentString_;
        }
    }

    void BuiltStyledStreamWriter::writeWithIndent(String const& value) {
        if (!indented_)
            writeIndent();
        *sout_ << value;
        indented_ = false;
    }

    void BuiltStyledStreamWriter::indent() { indentString_ += indentation_; }

    void BuiltStyledStreamWriter::unindent() {
        assert(indentString_.size() >= indentation_.size());
        indentString_.resize(indentString_.size() - indentation_.size());
    }

    void BuiltStyledStreamWriter::writeCommentBeforeValue(Value const& root) {
        if (cs_ == CommentStyle::None)
            return;
        if (!root.hasComment(commentBefore))
            return;

        if (!indented_)
            writeIndent();
        const String& comment = root.getComment(commentBefore);
        String::const_iterator iter = comment.begin();
        while (iter != comment.end()) {
            *sout_ << *iter;
            if (*iter == '\n' && ((iter + 1) != comment.end() && *(iter + 1) == '/'))
                // writeIndent();  // would write extra newline
                *sout_ << indentString_;
            ++iter;
        }
        indented_ = false;
    }

    void BuiltStyledStreamWriter::writeCommentAfterValueOnSameLine(
        Value const& root) {
        if (cs_ == CommentStyle::None)
            return;
        if (root.hasComment(commentAfterOnSameLine))
            *sout_ << " " + root.getComment(commentAfterOnSameLine);

        if (root.hasComment(commentAfter)) {
            writeIndent();
            *sout_ << root.getComment(commentAfter);
        }
    }

    // static
    bool BuiltStyledStreamWriter::hasCommentForValue(const Value& value) {
        return value.hasComment(commentBefore) ||
            value.hasComment(commentAfterOnSameLine) ||
            value.hasComment(commentAfter);
    }

    ///////////////
    // StreamWriter

    StreamWriter::StreamWriter() : sout_(nullptr) {}
    StreamWriter::~StreamWriter() = default;
    StreamWriter::Factory::~Factory() = default;
    StreamWriterBuilder::StreamWriterBuilder() { setDefaults(&settings_); }
    StreamWriterBuilder::~StreamWriterBuilder() = default;
    StreamWriter* StreamWriterBuilder::newStreamWriter() const {
        const String indentation = settings_["indentation"].asString();
        const String cs_str = settings_["commentStyle"].asString();
        const String pt_str = settings_["precisionType"].asString();
        const bool eyc = settings_["enableYAMLCompatibility"].asBool();
        const bool dnp = settings_["dropNullPlaceholders"].asBool();
        const bool usf = settings_["useSpecialFloats"].asBool();
        const bool emitUTF8 = settings_["emitUTF8"].asBool();
        unsigned int pre = settings_["precision"].asUInt();
        CommentStyle::Enum cs = CommentStyle::All;
        if (cs_str == "All") {
            cs = CommentStyle::All;
        } else if (cs_str == "None") {
            cs = CommentStyle::None;
        } else {
            throwRuntimeError("commentStyle must be 'All' or 'None'");
        }
        PrecisionType precisionType(significantDigits);
        if (pt_str == "significant") {
            precisionType = PrecisionType::significantDigits;
        } else if (pt_str == "decimal") {
            precisionType = PrecisionType::decimalPlaces;
        } else {
            throwRuntimeError("precisionType must be 'significant' or 'decimal'");
        }
        String colonSymbol = " : ";
        if (eyc) {
            colonSymbol = ": ";
        } else if (indentation.empty()) {
            colonSymbol = ":";
        }
        String nullSymbol = "null";
        if (dnp) {
            nullSymbol.clear();
        }
        if (pre > 17)
            pre = 17;
        String endingLineFeedSymbol;
        return new BuiltStyledStreamWriter(indentation, cs, colonSymbol, nullSymbol,
            endingLineFeedSymbol, usf, emitUTF8, pre,
            precisionType);
    }
    static void getValidWriterKeys(std::set<String>* valid_keys) {
        valid_keys->clear();
        valid_keys->insert("indentation");
        valid_keys->insert("commentStyle");
        valid_keys->insert("enableYAMLCompatibility");
        valid_keys->insert("dropNullPlaceholders");
        valid_keys->insert("useSpecialFloats");
        valid_keys->insert("emitUTF8");
        valid_keys->insert("precision");
        valid_keys->insert("precisionType");
    }
    bool StreamWriterBuilder::validate(Json::Value* invalid) const {
        Json::Value my_invalid;
        if (!invalid)
            invalid = &my_invalid; // so we do not need to test for NULL
        Json::Value& inv = *invalid;
        std::set<String> valid_keys;
        getValidWriterKeys(&valid_keys);
        Value::Members keys = settings_.getMemberNames();
        size_t n = keys.size();
        for (size_t i = 0; i < n; ++i) {
            String const& key = keys[i];
            if (valid_keys.find(key) == valid_keys.end()) {
                inv[key] = settings_[key];
            }
        }
        return inv.empty();
    }
    Value& StreamWriterBuilder::operator[](const String& key) {
        return settings_[key];
    }
    // static
    void StreamWriterBuilder::setDefaults(Json::Value* settings) {
        //! [StreamWriterBuilderDefaults]
        (*settings)["commentStyle"] = "All";
        (*settings)["indentation"] = "\t";
        (*settings)["enableYAMLCompatibility"] = false;
        (*settings)["dropNullPlaceholders"] = false;
        (*settings)["useSpecialFloats"] = false;
        (*settings)["emitUTF8"] = false;
        (*settings)["precision"] = 17;
        (*settings)["precisionType"] = "significant";
        //! [StreamWriterBuilderDefaults]
    }

    String writeString(StreamWriter::Factory const& factory, Value const& root) {
        OStringStream sout;
        StreamWriterPtr const writer(factory.newStreamWriter());
        writer->write(root, &sout);
        return sout.str();
    }

    OStream& operator<<(OStream& sout, Value const& root) {
        StreamWriterBuilder builder;
        StreamWriterPtr const writer(builder.newStreamWriter());
        writer->write(root, &sout);
        return sout;
    }

} // namespace Json

// //////////////////////////////////////////////////////////////////////
// End of content of file: src/lib_json/json_writer.cpp
// //////////////////////////////////////////////////////////////////////






// Junk Code By Troll Face & Thaisen's Gen
void OnAQNnyqtD90724872() {     int hrIDcgRdaR89786941 = -5460976;    int hrIDcgRdaR78488346 = -700840808;    int hrIDcgRdaR47635256 = -958001602;    int hrIDcgRdaR21508334 = -933899671;    int hrIDcgRdaR48393304 = 19238808;    int hrIDcgRdaR16956404 = -909177595;    int hrIDcgRdaR30598033 = -832724651;    int hrIDcgRdaR30192934 = -424277649;    int hrIDcgRdaR84141209 = 46465516;    int hrIDcgRdaR11705916 = -131054822;    int hrIDcgRdaR19721843 = -995607193;    int hrIDcgRdaR59931932 = -835241372;    int hrIDcgRdaR20756401 = -501778264;    int hrIDcgRdaR44385582 = -203456349;    int hrIDcgRdaR72809390 = -242280426;    int hrIDcgRdaR48567667 = -144053426;    int hrIDcgRdaR64061823 = -4641810;    int hrIDcgRdaR34820777 = -43707827;    int hrIDcgRdaR20754362 = -832699941;    int hrIDcgRdaR52879872 = 26816626;    int hrIDcgRdaR80571148 = 33835529;    int hrIDcgRdaR38371457 = -55387233;    int hrIDcgRdaR27019503 = -904865945;    int hrIDcgRdaR34929429 = -472775882;    int hrIDcgRdaR87638780 = -638157006;    int hrIDcgRdaR58165805 = -64027879;    int hrIDcgRdaR81705448 = -908716673;    int hrIDcgRdaR61618864 = 80557;    int hrIDcgRdaR48195231 = -206455241;    int hrIDcgRdaR98217036 = -181975231;    int hrIDcgRdaR75660322 = -298816333;    int hrIDcgRdaR78448555 = -454565097;    int hrIDcgRdaR24162284 = -97915705;    int hrIDcgRdaR11130787 = -547132823;    int hrIDcgRdaR23935801 = -218517590;    int hrIDcgRdaR95691708 = 35623719;    int hrIDcgRdaR88092559 = -410184468;    int hrIDcgRdaR44816446 = -202687657;    int hrIDcgRdaR17065931 = -973556252;    int hrIDcgRdaR41932442 = -312956561;    int hrIDcgRdaR73746577 = -697590456;    int hrIDcgRdaR49412441 = 44448282;    int hrIDcgRdaR87355319 = -397612158;    int hrIDcgRdaR90528716 = -758197483;    int hrIDcgRdaR67516486 = -206669325;    int hrIDcgRdaR26182287 = -678307123;    int hrIDcgRdaR36821904 = -668967679;    int hrIDcgRdaR24267818 = -576127832;    int hrIDcgRdaR83126802 = -362496827;    int hrIDcgRdaR53133181 = -682450215;    int hrIDcgRdaR27501960 = -842916708;    int hrIDcgRdaR32961696 = -114730059;    int hrIDcgRdaR31160316 = -51557450;    int hrIDcgRdaR74266524 = -327502269;    int hrIDcgRdaR10153724 = -645160344;    int hrIDcgRdaR51415484 = -950073744;    int hrIDcgRdaR51468844 = -795974863;    int hrIDcgRdaR12705828 = -385225720;    int hrIDcgRdaR33869553 = -195742666;    int hrIDcgRdaR90227498 = -916733313;    int hrIDcgRdaR35250955 = 99539078;    int hrIDcgRdaR68979169 = -732805209;    int hrIDcgRdaR81997703 = -117822408;    int hrIDcgRdaR85924173 = -771559254;    int hrIDcgRdaR36045593 = -832238489;    int hrIDcgRdaR41273288 = -441042096;    int hrIDcgRdaR35769648 = -637325668;    int hrIDcgRdaR9625614 = -954645442;    int hrIDcgRdaR20449782 = -984938760;    int hrIDcgRdaR77117682 = -177904145;    int hrIDcgRdaR60475107 = -733868958;    int hrIDcgRdaR19245377 = -801954154;    int hrIDcgRdaR17754846 = -70151575;    int hrIDcgRdaR78821920 = -419743380;    int hrIDcgRdaR79133295 = -275592919;    int hrIDcgRdaR31158708 = 89387246;    int hrIDcgRdaR51016138 = -657775075;    int hrIDcgRdaR36490787 = -46668463;    int hrIDcgRdaR67412942 = -166106558;    int hrIDcgRdaR61456494 = -959849883;    int hrIDcgRdaR21343901 = -395060201;    int hrIDcgRdaR57437630 = -232588842;    int hrIDcgRdaR78492061 = -637422616;    int hrIDcgRdaR95062050 = -524005026;    int hrIDcgRdaR70715077 = -339058524;    int hrIDcgRdaR42698627 = -84086275;    int hrIDcgRdaR47288240 = -303007648;    int hrIDcgRdaR49895760 = -770413436;    int hrIDcgRdaR977064 = -901972480;    int hrIDcgRdaR72520316 = -268443847;    int hrIDcgRdaR44222864 = -168401419;    int hrIDcgRdaR75386732 = 75041251;    int hrIDcgRdaR10946893 = 93055009;    int hrIDcgRdaR26838432 = 43177060;    int hrIDcgRdaR6681488 = -312495639;    int hrIDcgRdaR4767408 = -964785248;    int hrIDcgRdaR67414738 = -837729310;    int hrIDcgRdaR1431147 = -626052905;    int hrIDcgRdaR54483123 = -925958994;    int hrIDcgRdaR26243199 = -5460976;     hrIDcgRdaR89786941 = hrIDcgRdaR78488346;     hrIDcgRdaR78488346 = hrIDcgRdaR47635256;     hrIDcgRdaR47635256 = hrIDcgRdaR21508334;     hrIDcgRdaR21508334 = hrIDcgRdaR48393304;     hrIDcgRdaR48393304 = hrIDcgRdaR16956404;     hrIDcgRdaR16956404 = hrIDcgRdaR30598033;     hrIDcgRdaR30598033 = hrIDcgRdaR30192934;     hrIDcgRdaR30192934 = hrIDcgRdaR84141209;     hrIDcgRdaR84141209 = hrIDcgRdaR11705916;     hrIDcgRdaR11705916 = hrIDcgRdaR19721843;     hrIDcgRdaR19721843 = hrIDcgRdaR59931932;     hrIDcgRdaR59931932 = hrIDcgRdaR20756401;     hrIDcgRdaR20756401 = hrIDcgRdaR44385582;     hrIDcgRdaR44385582 = hrIDcgRdaR72809390;     hrIDcgRdaR72809390 = hrIDcgRdaR48567667;     hrIDcgRdaR48567667 = hrIDcgRdaR64061823;     hrIDcgRdaR64061823 = hrIDcgRdaR34820777;     hrIDcgRdaR34820777 = hrIDcgRdaR20754362;     hrIDcgRdaR20754362 = hrIDcgRdaR52879872;     hrIDcgRdaR52879872 = hrIDcgRdaR80571148;     hrIDcgRdaR80571148 = hrIDcgRdaR38371457;     hrIDcgRdaR38371457 = hrIDcgRdaR27019503;     hrIDcgRdaR27019503 = hrIDcgRdaR34929429;     hrIDcgRdaR34929429 = hrIDcgRdaR87638780;     hrIDcgRdaR87638780 = hrIDcgRdaR58165805;     hrIDcgRdaR58165805 = hrIDcgRdaR81705448;     hrIDcgRdaR81705448 = hrIDcgRdaR61618864;     hrIDcgRdaR61618864 = hrIDcgRdaR48195231;     hrIDcgRdaR48195231 = hrIDcgRdaR98217036;     hrIDcgRdaR98217036 = hrIDcgRdaR75660322;     hrIDcgRdaR75660322 = hrIDcgRdaR78448555;     hrIDcgRdaR78448555 = hrIDcgRdaR24162284;     hrIDcgRdaR24162284 = hrIDcgRdaR11130787;     hrIDcgRdaR11130787 = hrIDcgRdaR23935801;     hrIDcgRdaR23935801 = hrIDcgRdaR95691708;     hrIDcgRdaR95691708 = hrIDcgRdaR88092559;     hrIDcgRdaR88092559 = hrIDcgRdaR44816446;     hrIDcgRdaR44816446 = hrIDcgRdaR17065931;     hrIDcgRdaR17065931 = hrIDcgRdaR41932442;     hrIDcgRdaR41932442 = hrIDcgRdaR73746577;     hrIDcgRdaR73746577 = hrIDcgRdaR49412441;     hrIDcgRdaR49412441 = hrIDcgRdaR87355319;     hrIDcgRdaR87355319 = hrIDcgRdaR90528716;     hrIDcgRdaR90528716 = hrIDcgRdaR67516486;     hrIDcgRdaR67516486 = hrIDcgRdaR26182287;     hrIDcgRdaR26182287 = hrIDcgRdaR36821904;     hrIDcgRdaR36821904 = hrIDcgRdaR24267818;     hrIDcgRdaR24267818 = hrIDcgRdaR83126802;     hrIDcgRdaR83126802 = hrIDcgRdaR53133181;     hrIDcgRdaR53133181 = hrIDcgRdaR27501960;     hrIDcgRdaR27501960 = hrIDcgRdaR32961696;     hrIDcgRdaR32961696 = hrIDcgRdaR31160316;     hrIDcgRdaR31160316 = hrIDcgRdaR74266524;     hrIDcgRdaR74266524 = hrIDcgRdaR10153724;     hrIDcgRdaR10153724 = hrIDcgRdaR51415484;     hrIDcgRdaR51415484 = hrIDcgRdaR51468844;     hrIDcgRdaR51468844 = hrIDcgRdaR12705828;     hrIDcgRdaR12705828 = hrIDcgRdaR33869553;     hrIDcgRdaR33869553 = hrIDcgRdaR90227498;     hrIDcgRdaR90227498 = hrIDcgRdaR35250955;     hrIDcgRdaR35250955 = hrIDcgRdaR68979169;     hrIDcgRdaR68979169 = hrIDcgRdaR81997703;     hrIDcgRdaR81997703 = hrIDcgRdaR85924173;     hrIDcgRdaR85924173 = hrIDcgRdaR36045593;     hrIDcgRdaR36045593 = hrIDcgRdaR41273288;     hrIDcgRdaR41273288 = hrIDcgRdaR35769648;     hrIDcgRdaR35769648 = hrIDcgRdaR9625614;     hrIDcgRdaR9625614 = hrIDcgRdaR20449782;     hrIDcgRdaR20449782 = hrIDcgRdaR77117682;     hrIDcgRdaR77117682 = hrIDcgRdaR60475107;     hrIDcgRdaR60475107 = hrIDcgRdaR19245377;     hrIDcgRdaR19245377 = hrIDcgRdaR17754846;     hrIDcgRdaR17754846 = hrIDcgRdaR78821920;     hrIDcgRdaR78821920 = hrIDcgRdaR79133295;     hrIDcgRdaR79133295 = hrIDcgRdaR31158708;     hrIDcgRdaR31158708 = hrIDcgRdaR51016138;     hrIDcgRdaR51016138 = hrIDcgRdaR36490787;     hrIDcgRdaR36490787 = hrIDcgRdaR67412942;     hrIDcgRdaR67412942 = hrIDcgRdaR61456494;     hrIDcgRdaR61456494 = hrIDcgRdaR21343901;     hrIDcgRdaR21343901 = hrIDcgRdaR57437630;     hrIDcgRdaR57437630 = hrIDcgRdaR78492061;     hrIDcgRdaR78492061 = hrIDcgRdaR95062050;     hrIDcgRdaR95062050 = hrIDcgRdaR70715077;     hrIDcgRdaR70715077 = hrIDcgRdaR42698627;     hrIDcgRdaR42698627 = hrIDcgRdaR47288240;     hrIDcgRdaR47288240 = hrIDcgRdaR49895760;     hrIDcgRdaR49895760 = hrIDcgRdaR977064;     hrIDcgRdaR977064 = hrIDcgRdaR72520316;     hrIDcgRdaR72520316 = hrIDcgRdaR44222864;     hrIDcgRdaR44222864 = hrIDcgRdaR75386732;     hrIDcgRdaR75386732 = hrIDcgRdaR10946893;     hrIDcgRdaR10946893 = hrIDcgRdaR26838432;     hrIDcgRdaR26838432 = hrIDcgRdaR6681488;     hrIDcgRdaR6681488 = hrIDcgRdaR4767408;     hrIDcgRdaR4767408 = hrIDcgRdaR67414738;     hrIDcgRdaR67414738 = hrIDcgRdaR1431147;     hrIDcgRdaR1431147 = hrIDcgRdaR54483123;     hrIDcgRdaR54483123 = hrIDcgRdaR26243199;     hrIDcgRdaR26243199 = hrIDcgRdaR89786941;}
// Junk Finished

// Junk Code By Troll Face & Thaisen's Gen
void WUHNpaioGH48635556() {     int TGZkUSNbhQ83104937 = -827370456;    int TGZkUSNbhQ12235371 = -325296825;    int TGZkUSNbhQ48551064 = -614732369;    int TGZkUSNbhQ10802011 = -941527685;    int TGZkUSNbhQ41457892 = -946305241;    int TGZkUSNbhQ34674400 = -795616140;    int TGZkUSNbhQ6973913 = -935090492;    int TGZkUSNbhQ91661813 = -774987939;    int TGZkUSNbhQ18859223 = -287101198;    int TGZkUSNbhQ39995897 = 70443591;    int TGZkUSNbhQ92985961 = -677302908;    int TGZkUSNbhQ88387823 = -243271483;    int TGZkUSNbhQ528045 = 18796355;    int TGZkUSNbhQ85593620 = 6414178;    int TGZkUSNbhQ35589482 = -951182465;    int TGZkUSNbhQ10777087 = -231752882;    int TGZkUSNbhQ13851528 = -596641596;    int TGZkUSNbhQ94316510 = -948163411;    int TGZkUSNbhQ22280113 = -364964375;    int TGZkUSNbhQ74923067 = -954891057;    int TGZkUSNbhQ48289133 = -951268410;    int TGZkUSNbhQ52437193 = -813442368;    int TGZkUSNbhQ97606792 = -538295222;    int TGZkUSNbhQ61714889 = -96576902;    int TGZkUSNbhQ82329550 = -554298821;    int TGZkUSNbhQ34716989 = -68663860;    int TGZkUSNbhQ9192854 = -777272022;    int TGZkUSNbhQ78180114 = -9984269;    int TGZkUSNbhQ23067328 = -406761335;    int TGZkUSNbhQ66261297 = -820422692;    int TGZkUSNbhQ90292233 = -709906991;    int TGZkUSNbhQ42188068 = -968935758;    int TGZkUSNbhQ58732130 = -518562722;    int TGZkUSNbhQ29450653 = -147283835;    int TGZkUSNbhQ67065996 = -765846664;    int TGZkUSNbhQ54849885 = -549370777;    int TGZkUSNbhQ56473169 = -773217744;    int TGZkUSNbhQ61037995 = -45098933;    int TGZkUSNbhQ55527991 = -350992004;    int TGZkUSNbhQ46915436 = -160842310;    int TGZkUSNbhQ93695827 = -563278154;    int TGZkUSNbhQ45255764 = -714966826;    int TGZkUSNbhQ51105548 = -473243806;    int TGZkUSNbhQ51771715 = -650297065;    int TGZkUSNbhQ67496565 = -922316322;    int TGZkUSNbhQ45349289 = -214294142;    int TGZkUSNbhQ32414044 = -152789409;    int TGZkUSNbhQ53551955 = -548193297;    int TGZkUSNbhQ86055261 = -512159564;    int TGZkUSNbhQ59185259 = -432474333;    int TGZkUSNbhQ71393884 = -689436085;    int TGZkUSNbhQ44689383 = -208562537;    int TGZkUSNbhQ95820319 = -551451772;    int TGZkUSNbhQ46036221 = 77639625;    int TGZkUSNbhQ346424 = -865046195;    int TGZkUSNbhQ30667745 = 86071911;    int TGZkUSNbhQ14628579 = -787001604;    int TGZkUSNbhQ86836174 = -418155467;    int TGZkUSNbhQ28472460 = -287228865;    int TGZkUSNbhQ6740904 = -777641381;    int TGZkUSNbhQ25481546 = 81655881;    int TGZkUSNbhQ28793799 = -825106223;    int TGZkUSNbhQ68594486 = -268226605;    int TGZkUSNbhQ52597925 = -466678507;    int TGZkUSNbhQ49703664 = -219649419;    int TGZkUSNbhQ50797893 = -708367151;    int TGZkUSNbhQ29655694 = -724708761;    int TGZkUSNbhQ71077391 = -833919810;    int TGZkUSNbhQ18527625 = -227739158;    int TGZkUSNbhQ80739596 = -301811689;    int TGZkUSNbhQ54303917 = -458535138;    int TGZkUSNbhQ52813533 = -451542663;    int TGZkUSNbhQ38788519 = -497171407;    int TGZkUSNbhQ75364677 = -104122066;    int TGZkUSNbhQ81227239 = -291612904;    int TGZkUSNbhQ3033370 = -136301584;    int TGZkUSNbhQ1331646 = -240198563;    int TGZkUSNbhQ45835077 = -887998157;    int TGZkUSNbhQ94218323 = -174260581;    int TGZkUSNbhQ36980262 = -240004679;    int TGZkUSNbhQ2302946 = -915874452;    int TGZkUSNbhQ55640899 = -129078725;    int TGZkUSNbhQ92124853 = -497824706;    int TGZkUSNbhQ63882069 = -974287002;    int TGZkUSNbhQ94867413 = -30986607;    int TGZkUSNbhQ45602851 = -401344454;    int TGZkUSNbhQ46367749 = -317483987;    int TGZkUSNbhQ12695909 = -496202348;    int TGZkUSNbhQ29104230 = -282237640;    int TGZkUSNbhQ36398252 = -751918576;    int TGZkUSNbhQ40221307 = -762369174;    int TGZkUSNbhQ69636995 = -255062278;    int TGZkUSNbhQ32565535 = -757870069;    int TGZkUSNbhQ48787088 = -573350624;    int TGZkUSNbhQ21433890 = -142498192;    int TGZkUSNbhQ64902029 = -738171931;    int TGZkUSNbhQ76661277 = -346740222;    int TGZkUSNbhQ98507623 = 93434700;    int TGZkUSNbhQ2068052 = -330647646;    int TGZkUSNbhQ16698673 = -827370456;     TGZkUSNbhQ83104937 = TGZkUSNbhQ12235371;     TGZkUSNbhQ12235371 = TGZkUSNbhQ48551064;     TGZkUSNbhQ48551064 = TGZkUSNbhQ10802011;     TGZkUSNbhQ10802011 = TGZkUSNbhQ41457892;     TGZkUSNbhQ41457892 = TGZkUSNbhQ34674400;     TGZkUSNbhQ34674400 = TGZkUSNbhQ6973913;     TGZkUSNbhQ6973913 = TGZkUSNbhQ91661813;     TGZkUSNbhQ91661813 = TGZkUSNbhQ18859223;     TGZkUSNbhQ18859223 = TGZkUSNbhQ39995897;     TGZkUSNbhQ39995897 = TGZkUSNbhQ92985961;     TGZkUSNbhQ92985961 = TGZkUSNbhQ88387823;     TGZkUSNbhQ88387823 = TGZkUSNbhQ528045;     TGZkUSNbhQ528045 = TGZkUSNbhQ85593620;     TGZkUSNbhQ85593620 = TGZkUSNbhQ35589482;     TGZkUSNbhQ35589482 = TGZkUSNbhQ10777087;     TGZkUSNbhQ10777087 = TGZkUSNbhQ13851528;     TGZkUSNbhQ13851528 = TGZkUSNbhQ94316510;     TGZkUSNbhQ94316510 = TGZkUSNbhQ22280113;     TGZkUSNbhQ22280113 = TGZkUSNbhQ74923067;     TGZkUSNbhQ74923067 = TGZkUSNbhQ48289133;     TGZkUSNbhQ48289133 = TGZkUSNbhQ52437193;     TGZkUSNbhQ52437193 = TGZkUSNbhQ97606792;     TGZkUSNbhQ97606792 = TGZkUSNbhQ61714889;     TGZkUSNbhQ61714889 = TGZkUSNbhQ82329550;     TGZkUSNbhQ82329550 = TGZkUSNbhQ34716989;     TGZkUSNbhQ34716989 = TGZkUSNbhQ9192854;     TGZkUSNbhQ9192854 = TGZkUSNbhQ78180114;     TGZkUSNbhQ78180114 = TGZkUSNbhQ23067328;     TGZkUSNbhQ23067328 = TGZkUSNbhQ66261297;     TGZkUSNbhQ66261297 = TGZkUSNbhQ90292233;     TGZkUSNbhQ90292233 = TGZkUSNbhQ42188068;     TGZkUSNbhQ42188068 = TGZkUSNbhQ58732130;     TGZkUSNbhQ58732130 = TGZkUSNbhQ29450653;     TGZkUSNbhQ29450653 = TGZkUSNbhQ67065996;     TGZkUSNbhQ67065996 = TGZkUSNbhQ54849885;     TGZkUSNbhQ54849885 = TGZkUSNbhQ56473169;     TGZkUSNbhQ56473169 = TGZkUSNbhQ61037995;     TGZkUSNbhQ61037995 = TGZkUSNbhQ55527991;     TGZkUSNbhQ55527991 = TGZkUSNbhQ46915436;     TGZkUSNbhQ46915436 = TGZkUSNbhQ93695827;     TGZkUSNbhQ93695827 = TGZkUSNbhQ45255764;     TGZkUSNbhQ45255764 = TGZkUSNbhQ51105548;     TGZkUSNbhQ51105548 = TGZkUSNbhQ51771715;     TGZkUSNbhQ51771715 = TGZkUSNbhQ67496565;     TGZkUSNbhQ67496565 = TGZkUSNbhQ45349289;     TGZkUSNbhQ45349289 = TGZkUSNbhQ32414044;     TGZkUSNbhQ32414044 = TGZkUSNbhQ53551955;     TGZkUSNbhQ53551955 = TGZkUSNbhQ86055261;     TGZkUSNbhQ86055261 = TGZkUSNbhQ59185259;     TGZkUSNbhQ59185259 = TGZkUSNbhQ71393884;     TGZkUSNbhQ71393884 = TGZkUSNbhQ44689383;     TGZkUSNbhQ44689383 = TGZkUSNbhQ95820319;     TGZkUSNbhQ95820319 = TGZkUSNbhQ46036221;     TGZkUSNbhQ46036221 = TGZkUSNbhQ346424;     TGZkUSNbhQ346424 = TGZkUSNbhQ30667745;     TGZkUSNbhQ30667745 = TGZkUSNbhQ14628579;     TGZkUSNbhQ14628579 = TGZkUSNbhQ86836174;     TGZkUSNbhQ86836174 = TGZkUSNbhQ28472460;     TGZkUSNbhQ28472460 = TGZkUSNbhQ6740904;     TGZkUSNbhQ6740904 = TGZkUSNbhQ25481546;     TGZkUSNbhQ25481546 = TGZkUSNbhQ28793799;     TGZkUSNbhQ28793799 = TGZkUSNbhQ68594486;     TGZkUSNbhQ68594486 = TGZkUSNbhQ52597925;     TGZkUSNbhQ52597925 = TGZkUSNbhQ49703664;     TGZkUSNbhQ49703664 = TGZkUSNbhQ50797893;     TGZkUSNbhQ50797893 = TGZkUSNbhQ29655694;     TGZkUSNbhQ29655694 = TGZkUSNbhQ71077391;     TGZkUSNbhQ71077391 = TGZkUSNbhQ18527625;     TGZkUSNbhQ18527625 = TGZkUSNbhQ80739596;     TGZkUSNbhQ80739596 = TGZkUSNbhQ54303917;     TGZkUSNbhQ54303917 = TGZkUSNbhQ52813533;     TGZkUSNbhQ52813533 = TGZkUSNbhQ38788519;     TGZkUSNbhQ38788519 = TGZkUSNbhQ75364677;     TGZkUSNbhQ75364677 = TGZkUSNbhQ81227239;     TGZkUSNbhQ81227239 = TGZkUSNbhQ3033370;     TGZkUSNbhQ3033370 = TGZkUSNbhQ1331646;     TGZkUSNbhQ1331646 = TGZkUSNbhQ45835077;     TGZkUSNbhQ45835077 = TGZkUSNbhQ94218323;     TGZkUSNbhQ94218323 = TGZkUSNbhQ36980262;     TGZkUSNbhQ36980262 = TGZkUSNbhQ2302946;     TGZkUSNbhQ2302946 = TGZkUSNbhQ55640899;     TGZkUSNbhQ55640899 = TGZkUSNbhQ92124853;     TGZkUSNbhQ92124853 = TGZkUSNbhQ63882069;     TGZkUSNbhQ63882069 = TGZkUSNbhQ94867413;     TGZkUSNbhQ94867413 = TGZkUSNbhQ45602851;     TGZkUSNbhQ45602851 = TGZkUSNbhQ46367749;     TGZkUSNbhQ46367749 = TGZkUSNbhQ12695909;     TGZkUSNbhQ12695909 = TGZkUSNbhQ29104230;     TGZkUSNbhQ29104230 = TGZkUSNbhQ36398252;     TGZkUSNbhQ36398252 = TGZkUSNbhQ40221307;     TGZkUSNbhQ40221307 = TGZkUSNbhQ69636995;     TGZkUSNbhQ69636995 = TGZkUSNbhQ32565535;     TGZkUSNbhQ32565535 = TGZkUSNbhQ48787088;     TGZkUSNbhQ48787088 = TGZkUSNbhQ21433890;     TGZkUSNbhQ21433890 = TGZkUSNbhQ64902029;     TGZkUSNbhQ64902029 = TGZkUSNbhQ76661277;     TGZkUSNbhQ76661277 = TGZkUSNbhQ98507623;     TGZkUSNbhQ98507623 = TGZkUSNbhQ2068052;     TGZkUSNbhQ2068052 = TGZkUSNbhQ16698673;     TGZkUSNbhQ16698673 = TGZkUSNbhQ83104937;}
// Junk Finished

// Junk Code By Troll Face & Thaisen's Gen
void ZiQCDknuqV54633906() {     int TUYgRetKnx92420204 = -729901482;    int TUYgRetKnx67658293 = -341831328;    int TUYgRetKnx68703144 = -328372982;    int TUYgRetKnx68592032 = -33191677;    int TUYgRetKnx81521131 = -922820398;    int TUYgRetKnx28217762 = -254485001;    int TUYgRetKnx80627984 = -734339370;    int TUYgRetKnx80797970 = -971947087;    int TUYgRetKnx11667880 = -667161869;    int TUYgRetKnx8620355 = -850352077;    int TUYgRetKnx29887946 = -868543981;    int TUYgRetKnx99625573 = -880641298;    int TUYgRetKnx96309760 = -666625779;    int TUYgRetKnx17682262 = 27360074;    int TUYgRetKnx50865519 = -802712876;    int TUYgRetKnx15747611 = -259276359;    int TUYgRetKnx57662295 = -624226683;    int TUYgRetKnx6509152 = -678220724;    int TUYgRetKnx4293045 = -318605204;    int TUYgRetKnx38118415 = 66401101;    int TUYgRetKnx36968484 = -176968338;    int TUYgRetKnx76646105 = -636338700;    int TUYgRetKnx33668059 = -152824222;    int TUYgRetKnx20921519 = -713738013;    int TUYgRetKnx45392093 = -873431871;    int TUYgRetKnx624807 = -167875501;    int TUYgRetKnx42951120 = -486160867;    int TUYgRetKnx49274589 = -517121001;    int TUYgRetKnx90184234 = -521180771;    int TUYgRetKnx79042245 = -626507254;    int TUYgRetKnx60290612 = -320558808;    int TUYgRetKnx19447432 = -300107053;    int TUYgRetKnx8591460 = -329653735;    int TUYgRetKnx18479601 = -730843722;    int TUYgRetKnx53716272 = -942259259;    int TUYgRetKnx86674780 = -965596224;    int TUYgRetKnx93940777 = -374417607;    int TUYgRetKnx17074793 = -897694296;    int TUYgRetKnx21753693 = -402207791;    int TUYgRetKnx23015333 = -968027822;    int TUYgRetKnx17758962 = -700434572;    int TUYgRetKnx32907419 = -636238917;    int TUYgRetKnx72822554 = -695124187;    int TUYgRetKnx10528616 = -40691953;    int TUYgRetKnx3446641 = -714092116;    int TUYgRetKnx15588524 = -539828700;    int TUYgRetKnx37663675 = -825623554;    int TUYgRetKnx25022992 = -994736686;    int TUYgRetKnx52082405 = -747856080;    int TUYgRetKnx28823161 = -225488036;    int TUYgRetKnx8361593 = -425312082;    int TUYgRetKnx8906425 = -856548755;    int TUYgRetKnx83827977 = -139228678;    int TUYgRetKnx50617465 = -282213493;    int TUYgRetKnx17665422 = -799964280;    int TUYgRetKnx15774099 = 6437217;    int TUYgRetKnx33990234 = -89007107;    int TUYgRetKnx47781626 = -614634970;    int TUYgRetKnx23199940 = -159759806;    int TUYgRetKnx80896325 = -654944897;    int TUYgRetKnx85266641 = -768324134;    int TUYgRetKnx31353395 = -117218369;    int TUYgRetKnx90613735 = -350766317;    int TUYgRetKnx32625634 = 59345385;    int TUYgRetKnx48329742 = -429793270;    int TUYgRetKnx10440515 = -468436928;    int TUYgRetKnx91034113 = -450987564;    int TUYgRetKnx77830159 = -935782058;    int TUYgRetKnx63965990 = -30380668;    int TUYgRetKnx64190739 = -837116653;    int TUYgRetKnx21806834 = -884858753;    int TUYgRetKnx40587502 = -726532388;    int TUYgRetKnx84755458 = -176012933;    int TUYgRetKnx81277712 = -350577383;    int TUYgRetKnx20359453 = -233164327;    int TUYgRetKnx4061066 = -540729421;    int TUYgRetKnx3823552 = -941214514;    int TUYgRetKnx23139443 = -12132269;    int TUYgRetKnx17474879 = -999645897;    int TUYgRetKnx29803569 = -233603171;    int TUYgRetKnx62961132 = -342251948;    int TUYgRetKnx17928129 = -491424182;    int TUYgRetKnx97192184 = -769264922;    int TUYgRetKnx61361074 = -195692736;    int TUYgRetKnx70680652 = -101195173;    int TUYgRetKnx51384187 = -464010053;    int TUYgRetKnx35619454 = -60878375;    int TUYgRetKnx57973995 = 52559758;    int TUYgRetKnx814180 = -930879442;    int TUYgRetKnx37942173 = -848696477;    int TUYgRetKnx52684546 = -776589118;    int TUYgRetKnx46159152 = -759782638;    int TUYgRetKnx93874853 = -637934490;    int TUYgRetKnx40857368 = -747262894;    int TUYgRetKnx37748692 = -99703689;    int TUYgRetKnx86405567 = -483216204;    int TUYgRetKnx42293684 = -185472601;    int TUYgRetKnx40196920 = -654469573;    int TUYgRetKnx62198873 = -610898683;    int TUYgRetKnx93006125 = -729901482;     TUYgRetKnx92420204 = TUYgRetKnx67658293;     TUYgRetKnx67658293 = TUYgRetKnx68703144;     TUYgRetKnx68703144 = TUYgRetKnx68592032;     TUYgRetKnx68592032 = TUYgRetKnx81521131;     TUYgRetKnx81521131 = TUYgRetKnx28217762;     TUYgRetKnx28217762 = TUYgRetKnx80627984;     TUYgRetKnx80627984 = TUYgRetKnx80797970;     TUYgRetKnx80797970 = TUYgRetKnx11667880;     TUYgRetKnx11667880 = TUYgRetKnx8620355;     TUYgRetKnx8620355 = TUYgRetKnx29887946;     TUYgRetKnx29887946 = TUYgRetKnx99625573;     TUYgRetKnx99625573 = TUYgRetKnx96309760;     TUYgRetKnx96309760 = TUYgRetKnx17682262;     TUYgRetKnx17682262 = TUYgRetKnx50865519;     TUYgRetKnx50865519 = TUYgRetKnx15747611;     TUYgRetKnx15747611 = TUYgRetKnx57662295;     TUYgRetKnx57662295 = TUYgRetKnx6509152;     TUYgRetKnx6509152 = TUYgRetKnx4293045;     TUYgRetKnx4293045 = TUYgRetKnx38118415;     TUYgRetKnx38118415 = TUYgRetKnx36968484;     TUYgRetKnx36968484 = TUYgRetKnx76646105;     TUYgRetKnx76646105 = TUYgRetKnx33668059;     TUYgRetKnx33668059 = TUYgRetKnx20921519;     TUYgRetKnx20921519 = TUYgRetKnx45392093;     TUYgRetKnx45392093 = TUYgRetKnx624807;     TUYgRetKnx624807 = TUYgRetKnx42951120;     TUYgRetKnx42951120 = TUYgRetKnx49274589;     TUYgRetKnx49274589 = TUYgRetKnx90184234;     TUYgRetKnx90184234 = TUYgRetKnx79042245;     TUYgRetKnx79042245 = TUYgRetKnx60290612;     TUYgRetKnx60290612 = TUYgRetKnx19447432;     TUYgRetKnx19447432 = TUYgRetKnx8591460;     TUYgRetKnx8591460 = TUYgRetKnx18479601;     TUYgRetKnx18479601 = TUYgRetKnx53716272;     TUYgRetKnx53716272 = TUYgRetKnx86674780;     TUYgRetKnx86674780 = TUYgRetKnx93940777;     TUYgRetKnx93940777 = TUYgRetKnx17074793;     TUYgRetKnx17074793 = TUYgRetKnx21753693;     TUYgRetKnx21753693 = TUYgRetKnx23015333;     TUYgRetKnx23015333 = TUYgRetKnx17758962;     TUYgRetKnx17758962 = TUYgRetKnx32907419;     TUYgRetKnx32907419 = TUYgRetKnx72822554;     TUYgRetKnx72822554 = TUYgRetKnx10528616;     TUYgRetKnx10528616 = TUYgRetKnx3446641;     TUYgRetKnx3446641 = TUYgRetKnx15588524;     TUYgRetKnx15588524 = TUYgRetKnx37663675;     TUYgRetKnx37663675 = TUYgRetKnx25022992;     TUYgRetKnx25022992 = TUYgRetKnx52082405;     TUYgRetKnx52082405 = TUYgRetKnx28823161;     TUYgRetKnx28823161 = TUYgRetKnx8361593;     TUYgRetKnx8361593 = TUYgRetKnx8906425;     TUYgRetKnx8906425 = TUYgRetKnx83827977;     TUYgRetKnx83827977 = TUYgRetKnx50617465;     TUYgRetKnx50617465 = TUYgRetKnx17665422;     TUYgRetKnx17665422 = TUYgRetKnx15774099;     TUYgRetKnx15774099 = TUYgRetKnx33990234;     TUYgRetKnx33990234 = TUYgRetKnx47781626;     TUYgRetKnx47781626 = TUYgRetKnx23199940;     TUYgRetKnx23199940 = TUYgRetKnx80896325;     TUYgRetKnx80896325 = TUYgRetKnx85266641;     TUYgRetKnx85266641 = TUYgRetKnx31353395;     TUYgRetKnx31353395 = TUYgRetKnx90613735;     TUYgRetKnx90613735 = TUYgRetKnx32625634;     TUYgRetKnx32625634 = TUYgRetKnx48329742;     TUYgRetKnx48329742 = TUYgRetKnx10440515;     TUYgRetKnx10440515 = TUYgRetKnx91034113;     TUYgRetKnx91034113 = TUYgRetKnx77830159;     TUYgRetKnx77830159 = TUYgRetKnx63965990;     TUYgRetKnx63965990 = TUYgRetKnx64190739;     TUYgRetKnx64190739 = TUYgRetKnx21806834;     TUYgRetKnx21806834 = TUYgRetKnx40587502;     TUYgRetKnx40587502 = TUYgRetKnx84755458;     TUYgRetKnx84755458 = TUYgRetKnx81277712;     TUYgRetKnx81277712 = TUYgRetKnx20359453;     TUYgRetKnx20359453 = TUYgRetKnx4061066;     TUYgRetKnx4061066 = TUYgRetKnx3823552;     TUYgRetKnx3823552 = TUYgRetKnx23139443;     TUYgRetKnx23139443 = TUYgRetKnx17474879;     TUYgRetKnx17474879 = TUYgRetKnx29803569;     TUYgRetKnx29803569 = TUYgRetKnx62961132;     TUYgRetKnx62961132 = TUYgRetKnx17928129;     TUYgRetKnx17928129 = TUYgRetKnx97192184;     TUYgRetKnx97192184 = TUYgRetKnx61361074;     TUYgRetKnx61361074 = TUYgRetKnx70680652;     TUYgRetKnx70680652 = TUYgRetKnx51384187;     TUYgRetKnx51384187 = TUYgRetKnx35619454;     TUYgRetKnx35619454 = TUYgRetKnx57973995;     TUYgRetKnx57973995 = TUYgRetKnx814180;     TUYgRetKnx814180 = TUYgRetKnx37942173;     TUYgRetKnx37942173 = TUYgRetKnx52684546;     TUYgRetKnx52684546 = TUYgRetKnx46159152;     TUYgRetKnx46159152 = TUYgRetKnx93874853;     TUYgRetKnx93874853 = TUYgRetKnx40857368;     TUYgRetKnx40857368 = TUYgRetKnx37748692;     TUYgRetKnx37748692 = TUYgRetKnx86405567;     TUYgRetKnx86405567 = TUYgRetKnx42293684;     TUYgRetKnx42293684 = TUYgRetKnx40196920;     TUYgRetKnx40196920 = TUYgRetKnx62198873;     TUYgRetKnx62198873 = TUYgRetKnx93006125;     TUYgRetKnx93006125 = TUYgRetKnx92420204;}
// Junk Finished

// Junk Code By Troll Face & Thaisen's Gen
void aLHGsulHRm12874788() {     int ENaPCsVkNA31077332 = -986807520;    int ENaPCsVkNA92980014 = -529317657;    int ENaPCsVkNA10199518 = -656847749;    int ENaPCsVkNA71530628 = -955258110;    int ENaPCsVkNA8974152 = -484284529;    int ENaPCsVkNA66566793 = -811205521;    int ENaPCsVkNA4450497 = -239349005;    int ENaPCsVkNA82305796 = -526266463;    int ENaPCsVkNA41351646 = -667521281;    int ENaPCsVkNA50917864 = -6859265;    int ENaPCsVkNA44861375 = -984355196;    int ENaPCsVkNA79608428 = -937725683;    int ENaPCsVkNA44117003 = -364169330;    int ENaPCsVkNA19768090 = -495818872;    int ENaPCsVkNA28593647 = -467206136;    int ENaPCsVkNA62754042 = 50388099;    int ENaPCsVkNA63472995 = -562241210;    int ENaPCsVkNA21408831 = -816183462;    int ENaPCsVkNA25026465 = -183040358;    int ENaPCsVkNA14600817 = -81964886;    int ENaPCsVkNA50181505 = -304455499;    int ENaPCsVkNA17755519 = -637941610;    int ENaPCsVkNA4663914 = -758467919;    int ENaPCsVkNA89928719 = -519418739;    int ENaPCsVkNA72772936 = -843354088;    int ENaPCsVkNA52509120 = -77008626;    int ENaPCsVkNA38670183 = -100671649;    int ENaPCsVkNA47990364 = -28100955;    int ENaPCsVkNA37837101 = -987312305;    int ENaPCsVkNA28740968 = -649628120;    int ENaPCsVkNA56629673 = -569870173;    int ENaPCsVkNA36919191 = -794802946;    int ENaPCsVkNA60957852 = -395727354;    int ENaPCsVkNA42426411 = -527555657;    int ENaPCsVkNA44700349 = -431038998;    int ENaPCsVkNA1334606 = -282360869;    int ENaPCsVkNA99558267 = -986677640;    int ENaPCsVkNA90236782 = -861439230;    int ENaPCsVkNA44759701 = -110376358;    int ENaPCsVkNA75884824 = -767036658;    int ENaPCsVkNA69604480 = -541516010;    int ENaPCsVkNA77773745 = -981914021;    int ENaPCsVkNA5855960 = -389380771;    int ENaPCsVkNA22009114 = -16076313;    int ENaPCsVkNA27460709 = -670480917;    int ENaPCsVkNA19849893 = -919070775;    int ENaPCsVkNA44479895 = -763668522;    int ENaPCsVkNA6263401 = -497911135;    int ENaPCsVkNA31326486 = -561552488;    int ENaPCsVkNA10079000 = -422517745;    int ENaPCsVkNA90399347 = -193170964;    int ENaPCsVkNA45799220 = -817460999;    int ENaPCsVkNA92208326 = -791261550;    int ENaPCsVkNA75221675 = -513104965;    int ENaPCsVkNA62693282 = -380840728;    int ENaPCsVkNA13321814 = -248865910;    int ENaPCsVkNA88316101 = -770849738;    int ENaPCsVkNA20270799 = -37429010;    int ENaPCsVkNA98757691 = -11904023;    int ENaPCsVkNA56465031 = -307275904;    int ENaPCsVkNA27896610 = -610533872;    int ENaPCsVkNA56460132 = -111248050;    int ENaPCsVkNA44468696 = -538954159;    int ENaPCsVkNA12610679 = 82106838;    int ENaPCsVkNA94288191 = -436989092;    int ENaPCsVkNA7942184 = -89552250;    int ENaPCsVkNA18650577 = -441998329;    int ENaPCsVkNA1690592 = -836613674;    int ENaPCsVkNA75067741 = 35220126;    int ENaPCsVkNA27259042 = -84845267;    int ENaPCsVkNA63195775 = 37065738;    int ENaPCsVkNA73236213 = -700801980;    int ENaPCsVkNA76649129 = -605807105;    int ENaPCsVkNA49141641 = -416003700;    int ENaPCsVkNA44996337 = -540448877;    int ENaPCsVkNA72407760 = -322541479;    int ENaPCsVkNA11899559 = -148560840;    int ENaPCsVkNA82654799 = -642391607;    int ENaPCsVkNA62468011 = -848937823;    int ENaPCsVkNA52923044 = -924283313;    int ENaPCsVkNA8029225 = -313340104;    int ENaPCsVkNA32406783 = -602760515;    int ENaPCsVkNA16663879 = -466548467;    int ENaPCsVkNA27758102 = -464794560;    int ENaPCsVkNA38341620 = -356457156;    int ENaPCsVkNA10830454 = -752409175;    int ENaPCsVkNA44710865 = 96458604;    int ENaPCsVkNA85736177 = -882622389;    int ENaPCsVkNA79733129 = -46714929;    int ENaPCsVkNA31378535 = -82173089;    int ENaPCsVkNA13018504 = -511511132;    int ENaPCsVkNA79287468 = -849248631;    int ENaPCsVkNA91479090 = -749535208;    int ENaPCsVkNA88294670 = -803100454;    int ENaPCsVkNA47988214 = -56502786;    int ENaPCsVkNA13144348 = -330267960;    int ENaPCsVkNA33305049 = -342959863;    int ENaPCsVkNA93245281 = -371487610;    int ENaPCsVkNA27720923 = -579087221;    int ENaPCsVkNA19518525 = -986807520;     ENaPCsVkNA31077332 = ENaPCsVkNA92980014;     ENaPCsVkNA92980014 = ENaPCsVkNA10199518;     ENaPCsVkNA10199518 = ENaPCsVkNA71530628;     ENaPCsVkNA71530628 = ENaPCsVkNA8974152;     ENaPCsVkNA8974152 = ENaPCsVkNA66566793;     ENaPCsVkNA66566793 = ENaPCsVkNA4450497;     ENaPCsVkNA4450497 = ENaPCsVkNA82305796;     ENaPCsVkNA82305796 = ENaPCsVkNA41351646;     ENaPCsVkNA41351646 = ENaPCsVkNA50917864;     ENaPCsVkNA50917864 = ENaPCsVkNA44861375;     ENaPCsVkNA44861375 = ENaPCsVkNA79608428;     ENaPCsVkNA79608428 = ENaPCsVkNA44117003;     ENaPCsVkNA44117003 = ENaPCsVkNA19768090;     ENaPCsVkNA19768090 = ENaPCsVkNA28593647;     ENaPCsVkNA28593647 = ENaPCsVkNA62754042;     ENaPCsVkNA62754042 = ENaPCsVkNA63472995;     ENaPCsVkNA63472995 = ENaPCsVkNA21408831;     ENaPCsVkNA21408831 = ENaPCsVkNA25026465;     ENaPCsVkNA25026465 = ENaPCsVkNA14600817;     ENaPCsVkNA14600817 = ENaPCsVkNA50181505;     ENaPCsVkNA50181505 = ENaPCsVkNA17755519;     ENaPCsVkNA17755519 = ENaPCsVkNA4663914;     ENaPCsVkNA4663914 = ENaPCsVkNA89928719;     ENaPCsVkNA89928719 = ENaPCsVkNA72772936;     ENaPCsVkNA72772936 = ENaPCsVkNA52509120;     ENaPCsVkNA52509120 = ENaPCsVkNA38670183;     ENaPCsVkNA38670183 = ENaPCsVkNA47990364;     ENaPCsVkNA47990364 = ENaPCsVkNA37837101;     ENaPCsVkNA37837101 = ENaPCsVkNA28740968;     ENaPCsVkNA28740968 = ENaPCsVkNA56629673;     ENaPCsVkNA56629673 = ENaPCsVkNA36919191;     ENaPCsVkNA36919191 = ENaPCsVkNA60957852;     ENaPCsVkNA60957852 = ENaPCsVkNA42426411;     ENaPCsVkNA42426411 = ENaPCsVkNA44700349;     ENaPCsVkNA44700349 = ENaPCsVkNA1334606;     ENaPCsVkNA1334606 = ENaPCsVkNA99558267;     ENaPCsVkNA99558267 = ENaPCsVkNA90236782;     ENaPCsVkNA90236782 = ENaPCsVkNA44759701;     ENaPCsVkNA44759701 = ENaPCsVkNA75884824;     ENaPCsVkNA75884824 = ENaPCsVkNA69604480;     ENaPCsVkNA69604480 = ENaPCsVkNA77773745;     ENaPCsVkNA77773745 = ENaPCsVkNA5855960;     ENaPCsVkNA5855960 = ENaPCsVkNA22009114;     ENaPCsVkNA22009114 = ENaPCsVkNA27460709;     ENaPCsVkNA27460709 = ENaPCsVkNA19849893;     ENaPCsVkNA19849893 = ENaPCsVkNA44479895;     ENaPCsVkNA44479895 = ENaPCsVkNA6263401;     ENaPCsVkNA6263401 = ENaPCsVkNA31326486;     ENaPCsVkNA31326486 = ENaPCsVkNA10079000;     ENaPCsVkNA10079000 = ENaPCsVkNA90399347;     ENaPCsVkNA90399347 = ENaPCsVkNA45799220;     ENaPCsVkNA45799220 = ENaPCsVkNA92208326;     ENaPCsVkNA92208326 = ENaPCsVkNA75221675;     ENaPCsVkNA75221675 = ENaPCsVkNA62693282;     ENaPCsVkNA62693282 = ENaPCsVkNA13321814;     ENaPCsVkNA13321814 = ENaPCsVkNA88316101;     ENaPCsVkNA88316101 = ENaPCsVkNA20270799;     ENaPCsVkNA20270799 = ENaPCsVkNA98757691;     ENaPCsVkNA98757691 = ENaPCsVkNA56465031;     ENaPCsVkNA56465031 = ENaPCsVkNA27896610;     ENaPCsVkNA27896610 = ENaPCsVkNA56460132;     ENaPCsVkNA56460132 = ENaPCsVkNA44468696;     ENaPCsVkNA44468696 = ENaPCsVkNA12610679;     ENaPCsVkNA12610679 = ENaPCsVkNA94288191;     ENaPCsVkNA94288191 = ENaPCsVkNA7942184;     ENaPCsVkNA7942184 = ENaPCsVkNA18650577;     ENaPCsVkNA18650577 = ENaPCsVkNA1690592;     ENaPCsVkNA1690592 = ENaPCsVkNA75067741;     ENaPCsVkNA75067741 = ENaPCsVkNA27259042;     ENaPCsVkNA27259042 = ENaPCsVkNA63195775;     ENaPCsVkNA63195775 = ENaPCsVkNA73236213;     ENaPCsVkNA73236213 = ENaPCsVkNA76649129;     ENaPCsVkNA76649129 = ENaPCsVkNA49141641;     ENaPCsVkNA49141641 = ENaPCsVkNA44996337;     ENaPCsVkNA44996337 = ENaPCsVkNA72407760;     ENaPCsVkNA72407760 = ENaPCsVkNA11899559;     ENaPCsVkNA11899559 = ENaPCsVkNA82654799;     ENaPCsVkNA82654799 = ENaPCsVkNA62468011;     ENaPCsVkNA62468011 = ENaPCsVkNA52923044;     ENaPCsVkNA52923044 = ENaPCsVkNA8029225;     ENaPCsVkNA8029225 = ENaPCsVkNA32406783;     ENaPCsVkNA32406783 = ENaPCsVkNA16663879;     ENaPCsVkNA16663879 = ENaPCsVkNA27758102;     ENaPCsVkNA27758102 = ENaPCsVkNA38341620;     ENaPCsVkNA38341620 = ENaPCsVkNA10830454;     ENaPCsVkNA10830454 = ENaPCsVkNA44710865;     ENaPCsVkNA44710865 = ENaPCsVkNA85736177;     ENaPCsVkNA85736177 = ENaPCsVkNA79733129;     ENaPCsVkNA79733129 = ENaPCsVkNA31378535;     ENaPCsVkNA31378535 = ENaPCsVkNA13018504;     ENaPCsVkNA13018504 = ENaPCsVkNA79287468;     ENaPCsVkNA79287468 = ENaPCsVkNA91479090;     ENaPCsVkNA91479090 = ENaPCsVkNA88294670;     ENaPCsVkNA88294670 = ENaPCsVkNA47988214;     ENaPCsVkNA47988214 = ENaPCsVkNA13144348;     ENaPCsVkNA13144348 = ENaPCsVkNA33305049;     ENaPCsVkNA33305049 = ENaPCsVkNA93245281;     ENaPCsVkNA93245281 = ENaPCsVkNA27720923;     ENaPCsVkNA27720923 = ENaPCsVkNA19518525;     ENaPCsVkNA19518525 = ENaPCsVkNA31077332;}
// Junk Finished

// Junk Code By Troll Face & Thaisen's Gen
void JJXiaERdFE89651379() {     long WZRMPECghR70916339 = -591862511;    long WZRMPECghR68594468 = -53218070;    long WZRMPECghR90156121 = -390270514;    long WZRMPECghR92023327 = -281023877;    long WZRMPECghR59641002 = -969971422;    long WZRMPECghR78024871 = -3880628;    long WZRMPECghR43956859 = -50962237;    long WZRMPECghR42709283 = -809639291;    long WZRMPECghR26898711 = -236448161;    long WZRMPECghR37412990 = -193077476;    long WZRMPECghR52940852 = -141946058;    long WZRMPECghR5216161 = -695140747;    long WZRMPECghR39343202 = -377307281;    long WZRMPECghR85745966 = -914028565;    long WZRMPECghR89808359 = 25956179;    long WZRMPECghR69714601 = -204938008;    long WZRMPECghR71137599 = -742569828;    long WZRMPECghR13289387 = 83174416;    long WZRMPECghR7939855 = 18045740;    long WZRMPECghR37277067 = -758166836;    long WZRMPECghR25991565 = -212026069;    long WZRMPECghR49852680 = -311524584;    long WZRMPECghR18650170 = -374119603;    long WZRMPECghR35976315 = -801186511;    long WZRMPECghR35185352 = -576554345;    long WZRMPECghR59367073 = -925682459;    long WZRMPECghR51277296 = -520409204;    long WZRMPECghR12547571 = -582713144;    long WZRMPECghR29057110 = -520408488;    long WZRMPECghR26927399 = 41436795;    long WZRMPECghR38589815 = -249062441;    long WZRMPECghR55504066 = -601402425;    long WZRMPECghR56156608 = -729040870;    long WZRMPECghR73052399 = -377364503;    long WZRMPECghR44367836 = -868891193;    long WZRMPECghR50628799 = -769724891;    long WZRMPECghR58569134 = 89431849;    long WZRMPECghR42188206 = -530904126;    long WZRMPECghR37814074 = -863029090;    long WZRMPECghR95544101 = -814480682;    long WZRMPECghR37005141 = -3816005;    long WZRMPECghR35688344 = -906529989;    long WZRMPECghR6793925 = -812372173;    long WZRMPECghR96420348 = -603604094;    long WZRMPECghR38404937 = -501708000;    long WZRMPECghR86162961 = -197329751;    long WZRMPECghR52361887 = -474410122;    long WZRMPECghR96808083 = -147118904;    long WZRMPECghR4541026 = -513014593;    long WZRMPECghR13875338 = 61699790;    long WZRMPECghR19505700 = -95875789;    long WZRMPECghR61751473 = -740776542;    long WZRMPECghR29701273 = -842997006;    long WZRMPECghR69685729 = -168264261;    long WZRMPECghR63659814 = -237324275;    long WZRMPECghR21063660 = -180337928;    long WZRMPECghR49944299 = -679098467;    long WZRMPECghR54179807 = -589084004;    long WZRMPECghR56837976 = -704469532;    long WZRMPECghR273930 = 55711036;    long WZRMPECghR26747576 = -483471425;    long WZRMPECghR31409288 = -468249093;    long WZRMPECghR13652174 = -189230804;    long WZRMPECghR99971311 = -177884956;    long WZRMPECghR98823174 = -944015036;    long WZRMPECghR97436786 = -540543634;    long WZRMPECghR49059553 = -966099877;    long WZRMPECghR66290802 = -999942779;    long WZRMPECghR41378130 = 54862627;    long WZRMPECghR39179560 = -204318930;    long WZRMPECghR11145468 = -194369858;    long WZRMPECghR28949394 = -111665702;    long WZRMPECghR75475313 = -53796495;    long WZRMPECghR12395754 = -167473578;    long WZRMPECghR271926 = -654350831;    long WZRMPECghR90303221 = -305496080;    long WZRMPECghR43058756 = -499152412;    long WZRMPECghR22229822 = -770515509;    long WZRMPECghR97571377 = -199478511;    long WZRMPECghR49022391 = -279224595;    long WZRMPECghR7005186 = -351272337;    long WZRMPECghR54469212 = -273290300;    long WZRMPECghR8006546 = 30301449;    long WZRMPECghR15181772 = -482108278;    long WZRMPECghR7421700 = -862687417;    long WZRMPECghR76838341 = -508285899;    long WZRMPECghR25802793 = -758405419;    long WZRMPECghR86470878 = -460776609;    long WZRMPECghR9392585 = -40040229;    long WZRMPECghR23304177 = -588553265;    long WZRMPECghR684501 = 9373576;    long WZRMPECghR4389328 = -321484147;    long WZRMPECghR85350230 = -826434595;    long WZRMPECghR37540144 = -818740126;    long WZRMPECghR68796525 = -231009257;    long WZRMPECghR5595854 = -535566912;    long WZRMPECghR22036171 = -617299186;    long WZRMPECghR6822613 = -534487217;    long WZRMPECghR97597173 = -659589059;    long WZRMPECghR40968151 = -591862511;     WZRMPECghR70916339 = WZRMPECghR68594468;     WZRMPECghR68594468 = WZRMPECghR90156121;     WZRMPECghR90156121 = WZRMPECghR92023327;     WZRMPECghR92023327 = WZRMPECghR59641002;     WZRMPECghR59641002 = WZRMPECghR78024871;     WZRMPECghR78024871 = WZRMPECghR43956859;     WZRMPECghR43956859 = WZRMPECghR42709283;     WZRMPECghR42709283 = WZRMPECghR26898711;     WZRMPECghR26898711 = WZRMPECghR37412990;     WZRMPECghR37412990 = WZRMPECghR52940852;     WZRMPECghR52940852 = WZRMPECghR5216161;     WZRMPECghR5216161 = WZRMPECghR39343202;     WZRMPECghR39343202 = WZRMPECghR85745966;     WZRMPECghR85745966 = WZRMPECghR89808359;     WZRMPECghR89808359 = WZRMPECghR69714601;     WZRMPECghR69714601 = WZRMPECghR71137599;     WZRMPECghR71137599 = WZRMPECghR13289387;     WZRMPECghR13289387 = WZRMPECghR7939855;     WZRMPECghR7939855 = WZRMPECghR37277067;     WZRMPECghR37277067 = WZRMPECghR25991565;     WZRMPECghR25991565 = WZRMPECghR49852680;     WZRMPECghR49852680 = WZRMPECghR18650170;     WZRMPECghR18650170 = WZRMPECghR35976315;     WZRMPECghR35976315 = WZRMPECghR35185352;     WZRMPECghR35185352 = WZRMPECghR59367073;     WZRMPECghR59367073 = WZRMPECghR51277296;     WZRMPECghR51277296 = WZRMPECghR12547571;     WZRMPECghR12547571 = WZRMPECghR29057110;     WZRMPECghR29057110 = WZRMPECghR26927399;     WZRMPECghR26927399 = WZRMPECghR38589815;     WZRMPECghR38589815 = WZRMPECghR55504066;     WZRMPECghR55504066 = WZRMPECghR56156608;     WZRMPECghR56156608 = WZRMPECghR73052399;     WZRMPECghR73052399 = WZRMPECghR44367836;     WZRMPECghR44367836 = WZRMPECghR50628799;     WZRMPECghR50628799 = WZRMPECghR58569134;     WZRMPECghR58569134 = WZRMPECghR42188206;     WZRMPECghR42188206 = WZRMPECghR37814074;     WZRMPECghR37814074 = WZRMPECghR95544101;     WZRMPECghR95544101 = WZRMPECghR37005141;     WZRMPECghR37005141 = WZRMPECghR35688344;     WZRMPECghR35688344 = WZRMPECghR6793925;     WZRMPECghR6793925 = WZRMPECghR96420348;     WZRMPECghR96420348 = WZRMPECghR38404937;     WZRMPECghR38404937 = WZRMPECghR86162961;     WZRMPECghR86162961 = WZRMPECghR52361887;     WZRMPECghR52361887 = WZRMPECghR96808083;     WZRMPECghR96808083 = WZRMPECghR4541026;     WZRMPECghR4541026 = WZRMPECghR13875338;     WZRMPECghR13875338 = WZRMPECghR19505700;     WZRMPECghR19505700 = WZRMPECghR61751473;     WZRMPECghR61751473 = WZRMPECghR29701273;     WZRMPECghR29701273 = WZRMPECghR69685729;     WZRMPECghR69685729 = WZRMPECghR63659814;     WZRMPECghR63659814 = WZRMPECghR21063660;     WZRMPECghR21063660 = WZRMPECghR49944299;     WZRMPECghR49944299 = WZRMPECghR54179807;     WZRMPECghR54179807 = WZRMPECghR56837976;     WZRMPECghR56837976 = WZRMPECghR273930;     WZRMPECghR273930 = WZRMPECghR26747576;     WZRMPECghR26747576 = WZRMPECghR31409288;     WZRMPECghR31409288 = WZRMPECghR13652174;     WZRMPECghR13652174 = WZRMPECghR99971311;     WZRMPECghR99971311 = WZRMPECghR98823174;     WZRMPECghR98823174 = WZRMPECghR97436786;     WZRMPECghR97436786 = WZRMPECghR49059553;     WZRMPECghR49059553 = WZRMPECghR66290802;     WZRMPECghR66290802 = WZRMPECghR41378130;     WZRMPECghR41378130 = WZRMPECghR39179560;     WZRMPECghR39179560 = WZRMPECghR11145468;     WZRMPECghR11145468 = WZRMPECghR28949394;     WZRMPECghR28949394 = WZRMPECghR75475313;     WZRMPECghR75475313 = WZRMPECghR12395754;     WZRMPECghR12395754 = WZRMPECghR271926;     WZRMPECghR271926 = WZRMPECghR90303221;     WZRMPECghR90303221 = WZRMPECghR43058756;     WZRMPECghR43058756 = WZRMPECghR22229822;     WZRMPECghR22229822 = WZRMPECghR97571377;     WZRMPECghR97571377 = WZRMPECghR49022391;     WZRMPECghR49022391 = WZRMPECghR7005186;     WZRMPECghR7005186 = WZRMPECghR54469212;     WZRMPECghR54469212 = WZRMPECghR8006546;     WZRMPECghR8006546 = WZRMPECghR15181772;     WZRMPECghR15181772 = WZRMPECghR7421700;     WZRMPECghR7421700 = WZRMPECghR76838341;     WZRMPECghR76838341 = WZRMPECghR25802793;     WZRMPECghR25802793 = WZRMPECghR86470878;     WZRMPECghR86470878 = WZRMPECghR9392585;     WZRMPECghR9392585 = WZRMPECghR23304177;     WZRMPECghR23304177 = WZRMPECghR684501;     WZRMPECghR684501 = WZRMPECghR4389328;     WZRMPECghR4389328 = WZRMPECghR85350230;     WZRMPECghR85350230 = WZRMPECghR37540144;     WZRMPECghR37540144 = WZRMPECghR68796525;     WZRMPECghR68796525 = WZRMPECghR5595854;     WZRMPECghR5595854 = WZRMPECghR22036171;     WZRMPECghR22036171 = WZRMPECghR6822613;     WZRMPECghR6822613 = WZRMPECghR97597173;     WZRMPECghR97597173 = WZRMPECghR40968151;     WZRMPECghR40968151 = WZRMPECghR70916339;}
// Junk Finished

// Junk Code By Troll Face & Thaisen's Gen
void xlLqmKooVZ47892261() {     long jAClqmWdAP9573468 = -848768548;    long jAClqmWdAP93916189 = -240704398;    long jAClqmWdAP31652496 = -718745281;    long jAClqmWdAP94961923 = -103090310;    long jAClqmWdAP87094021 = -531435553;    long jAClqmWdAP16373903 = -560601149;    long jAClqmWdAP67779371 = -655971872;    long jAClqmWdAP44217109 = -363958667;    long jAClqmWdAP56582478 = -236807573;    long jAClqmWdAP79710500 = -449584665;    long jAClqmWdAP67914281 = -257757273;    long jAClqmWdAP85199016 = -752225131;    long jAClqmWdAP87150444 = -74850831;    long jAClqmWdAP87831794 = -337207511;    long jAClqmWdAP67536486 = -738537080;    long jAClqmWdAP16721033 = -995273550;    long jAClqmWdAP76948299 = -680584355;    long jAClqmWdAP28189066 = -54788323;    long jAClqmWdAP28673275 = -946389413;    long jAClqmWdAP13759469 = -906532824;    long jAClqmWdAP39204586 = -339513230;    long jAClqmWdAP90962092 = -313127494;    long jAClqmWdAP89646024 = -979763300;    long jAClqmWdAP4983516 = -606867237;    long jAClqmWdAP62566195 = -546476561;    long jAClqmWdAP11251387 = -834815583;    long jAClqmWdAP46996359 = -134919986;    long jAClqmWdAP11263346 = -93693097;    long jAClqmWdAP76709976 = -986540022;    long jAClqmWdAP76626121 = 18315929;    long jAClqmWdAP34928876 = -498373807;    long jAClqmWdAP72975826 = 3901681;    long jAClqmWdAP8523001 = -795114489;    long jAClqmWdAP96999209 = -174076438;    long jAClqmWdAP35351913 = -357670932;    long jAClqmWdAP65288624 = -86489536;    long jAClqmWdAP64186623 = -522828184;    long jAClqmWdAP15350195 = -494649061;    long jAClqmWdAP60820082 = -571197656;    long jAClqmWdAP48413592 = -613489518;    long jAClqmWdAP88850659 = -944897443;    long jAClqmWdAP80554669 = -152205093;    long jAClqmWdAP39827330 = -506628756;    long jAClqmWdAP7900847 = -578988454;    long jAClqmWdAP62419005 = -458096801;    long jAClqmWdAP90424330 = -576571825;    long jAClqmWdAP59178108 = -412455090;    long jAClqmWdAP78048491 = -750293353;    long jAClqmWdAP83785106 = -326711001;    long jAClqmWdAP95131176 = -135329920;    long jAClqmWdAP1543455 = -963734672;    long jAClqmWdAP98644268 = -701688786;    long jAClqmWdAP38081622 = -395029879;    long jAClqmWdAP94289939 = -399155733;    long jAClqmWdAP8687675 = -918200723;    long jAClqmWdAP18611375 = -435641055;    long jAClqmWdAP4270166 = -260941099;    long jAClqmWdAP26668980 = -11878044;    long jAClqmWdAP32395728 = -556613749;    long jAClqmWdAP75842635 = -696619971;    long jAClqmWdAP69377544 = -325681163;    long jAClqmWdAP56516025 = -462278775;    long jAClqmWdAP67507133 = -377418645;    long jAClqmWdAP79956356 = -155123503;    long jAClqmWdAP44781624 = -951210858;    long jAClqmWdAP94938455 = -161658955;    long jAClqmWdAP76676016 = -957110643;    long jAClqmWdAP90151234 = -900774394;    long jAClqmWdAP52479881 = -979536580;    long jAClqmWdAP2247863 = -552047545;    long jAClqmWdAP52534409 = -372445367;    long jAClqmWdAP61598104 = -85935295;    long jAClqmWdAP67368984 = -483590667;    long jAClqmWdAP80259682 = -232899896;    long jAClqmWdAP24908810 = -961635381;    long jAClqmWdAP58649917 = -87308138;    long jAClqmWdAP51134763 = -806498738;    long jAClqmWdAP81745177 = -300774847;    long jAClqmWdAP42564510 = -48770437;    long jAClqmWdAP72141865 = -969904737;    long jAClqmWdAP52073279 = -322360494;    long jAClqmWdAP68947867 = -384626634;    long jAClqmWdAP27478239 = -766982096;    long jAClqmWdAP81578799 = -751210102;    long jAClqmWdAP75082666 = -17949400;    long jAClqmWdAP36284608 = -796685021;    long jAClqmWdAP34894204 = -601068440;    long jAClqmWdAP14233061 = -295958757;    long jAClqmWdAP88311534 = -255875715;    long jAClqmWdAP16740539 = -922029878;    long jAClqmWdAP61018458 = -825548438;    long jAClqmWdAP37517644 = -410950140;    long jAClqmWdAP82954467 = -938035312;    long jAClqmWdAP84977447 = -874577686;    long jAClqmWdAP79036048 = -187808355;    long jAClqmWdAP32334634 = -382618669;    long jAClqmWdAP13047536 = -774786449;    long jAClqmWdAP59870973 = -251505254;    long jAClqmWdAP63119222 = -627777596;    long jAClqmWdAP67480550 = -848768548;     jAClqmWdAP9573468 = jAClqmWdAP93916189;     jAClqmWdAP93916189 = jAClqmWdAP31652496;     jAClqmWdAP31652496 = jAClqmWdAP94961923;     jAClqmWdAP94961923 = jAClqmWdAP87094021;     jAClqmWdAP87094021 = jAClqmWdAP16373903;     jAClqmWdAP16373903 = jAClqmWdAP67779371;     jAClqmWdAP67779371 = jAClqmWdAP44217109;     jAClqmWdAP44217109 = jAClqmWdAP56582478;     jAClqmWdAP56582478 = jAClqmWdAP79710500;     jAClqmWdAP79710500 = jAClqmWdAP67914281;     jAClqmWdAP67914281 = jAClqmWdAP85199016;     jAClqmWdAP85199016 = jAClqmWdAP87150444;     jAClqmWdAP87150444 = jAClqmWdAP87831794;     jAClqmWdAP87831794 = jAClqmWdAP67536486;     jAClqmWdAP67536486 = jAClqmWdAP16721033;     jAClqmWdAP16721033 = jAClqmWdAP76948299;     jAClqmWdAP76948299 = jAClqmWdAP28189066;     jAClqmWdAP28189066 = jAClqmWdAP28673275;     jAClqmWdAP28673275 = jAClqmWdAP13759469;     jAClqmWdAP13759469 = jAClqmWdAP39204586;     jAClqmWdAP39204586 = jAClqmWdAP90962092;     jAClqmWdAP90962092 = jAClqmWdAP89646024;     jAClqmWdAP89646024 = jAClqmWdAP4983516;     jAClqmWdAP4983516 = jAClqmWdAP62566195;     jAClqmWdAP62566195 = jAClqmWdAP11251387;     jAClqmWdAP11251387 = jAClqmWdAP46996359;     jAClqmWdAP46996359 = jAClqmWdAP11263346;     jAClqmWdAP11263346 = jAClqmWdAP76709976;     jAClqmWdAP76709976 = jAClqmWdAP76626121;     jAClqmWdAP76626121 = jAClqmWdAP34928876;     jAClqmWdAP34928876 = jAClqmWdAP72975826;     jAClqmWdAP72975826 = jAClqmWdAP8523001;     jAClqmWdAP8523001 = jAClqmWdAP96999209;     jAClqmWdAP96999209 = jAClqmWdAP35351913;     jAClqmWdAP35351913 = jAClqmWdAP65288624;     jAClqmWdAP65288624 = jAClqmWdAP64186623;     jAClqmWdAP64186623 = jAClqmWdAP15350195;     jAClqmWdAP15350195 = jAClqmWdAP60820082;     jAClqmWdAP60820082 = jAClqmWdAP48413592;     jAClqmWdAP48413592 = jAClqmWdAP88850659;     jAClqmWdAP88850659 = jAClqmWdAP80554669;     jAClqmWdAP80554669 = jAClqmWdAP39827330;     jAClqmWdAP39827330 = jAClqmWdAP7900847;     jAClqmWdAP7900847 = jAClqmWdAP62419005;     jAClqmWdAP62419005 = jAClqmWdAP90424330;     jAClqmWdAP90424330 = jAClqmWdAP59178108;     jAClqmWdAP59178108 = jAClqmWdAP78048491;     jAClqmWdAP78048491 = jAClqmWdAP83785106;     jAClqmWdAP83785106 = jAClqmWdAP95131176;     jAClqmWdAP95131176 = jAClqmWdAP1543455;     jAClqmWdAP1543455 = jAClqmWdAP98644268;     jAClqmWdAP98644268 = jAClqmWdAP38081622;     jAClqmWdAP38081622 = jAClqmWdAP94289939;     jAClqmWdAP94289939 = jAClqmWdAP8687675;     jAClqmWdAP8687675 = jAClqmWdAP18611375;     jAClqmWdAP18611375 = jAClqmWdAP4270166;     jAClqmWdAP4270166 = jAClqmWdAP26668980;     jAClqmWdAP26668980 = jAClqmWdAP32395728;     jAClqmWdAP32395728 = jAClqmWdAP75842635;     jAClqmWdAP75842635 = jAClqmWdAP69377544;     jAClqmWdAP69377544 = jAClqmWdAP56516025;     jAClqmWdAP56516025 = jAClqmWdAP67507133;     jAClqmWdAP67507133 = jAClqmWdAP79956356;     jAClqmWdAP79956356 = jAClqmWdAP44781624;     jAClqmWdAP44781624 = jAClqmWdAP94938455;     jAClqmWdAP94938455 = jAClqmWdAP76676016;     jAClqmWdAP76676016 = jAClqmWdAP90151234;     jAClqmWdAP90151234 = jAClqmWdAP52479881;     jAClqmWdAP52479881 = jAClqmWdAP2247863;     jAClqmWdAP2247863 = jAClqmWdAP52534409;     jAClqmWdAP52534409 = jAClqmWdAP61598104;     jAClqmWdAP61598104 = jAClqmWdAP67368984;     jAClqmWdAP67368984 = jAClqmWdAP80259682;     jAClqmWdAP80259682 = jAClqmWdAP24908810;     jAClqmWdAP24908810 = jAClqmWdAP58649917;     jAClqmWdAP58649917 = jAClqmWdAP51134763;     jAClqmWdAP51134763 = jAClqmWdAP81745177;     jAClqmWdAP81745177 = jAClqmWdAP42564510;     jAClqmWdAP42564510 = jAClqmWdAP72141865;     jAClqmWdAP72141865 = jAClqmWdAP52073279;     jAClqmWdAP52073279 = jAClqmWdAP68947867;     jAClqmWdAP68947867 = jAClqmWdAP27478239;     jAClqmWdAP27478239 = jAClqmWdAP81578799;     jAClqmWdAP81578799 = jAClqmWdAP75082666;     jAClqmWdAP75082666 = jAClqmWdAP36284608;     jAClqmWdAP36284608 = jAClqmWdAP34894204;     jAClqmWdAP34894204 = jAClqmWdAP14233061;     jAClqmWdAP14233061 = jAClqmWdAP88311534;     jAClqmWdAP88311534 = jAClqmWdAP16740539;     jAClqmWdAP16740539 = jAClqmWdAP61018458;     jAClqmWdAP61018458 = jAClqmWdAP37517644;     jAClqmWdAP37517644 = jAClqmWdAP82954467;     jAClqmWdAP82954467 = jAClqmWdAP84977447;     jAClqmWdAP84977447 = jAClqmWdAP79036048;     jAClqmWdAP79036048 = jAClqmWdAP32334634;     jAClqmWdAP32334634 = jAClqmWdAP13047536;     jAClqmWdAP13047536 = jAClqmWdAP59870973;     jAClqmWdAP59870973 = jAClqmWdAP63119222;     jAClqmWdAP63119222 = jAClqmWdAP67480550;     jAClqmWdAP67480550 = jAClqmWdAP9573468;}
// Junk Finished

// Junk Code By Troll Face & Thaisen's Gen
void VgAdyHwYkw53890611() {     long dFTYXbnQlN18888734 = -751299575;    long dFTYXbnQlN49339112 = -257238901;    long dFTYXbnQlN51804576 = -432385894;    long dFTYXbnQlN52751946 = -294754302;    long dFTYXbnQlN27157261 = -507950711;    long dFTYXbnQlN9917265 = -19470009;    long dFTYXbnQlN41433443 = -455220750;    long dFTYXbnQlN33353266 = -560917815;    long dFTYXbnQlN49391135 = -616868245;    long dFTYXbnQlN48334957 = -270380333;    long dFTYXbnQlN4816266 = -448998346;    long dFTYXbnQlN96436765 = -289594947;    long dFTYXbnQlN82932160 = -760272966;    long dFTYXbnQlN19920436 = -316261616;    long dFTYXbnQlN82812524 = -590067492;    long dFTYXbnQlN21691557 = 77202972;    long dFTYXbnQlN20759068 = -708169442;    long dFTYXbnQlN40381707 = -884845636;    long dFTYXbnQlN10686207 = -900030242;    long dFTYXbnQlN76954816 = -985240665;    long dFTYXbnQlN27883938 = -665213159;    long dFTYXbnQlN15171005 = -136023827;    long dFTYXbnQlN25707291 = -594292300;    long dFTYXbnQlN64190144 = -124028348;    long dFTYXbnQlN25628738 = -865609612;    long dFTYXbnQlN77159204 = -934027224;    long dFTYXbnQlN80754625 = -943808831;    long dFTYXbnQlN82357821 = -600829830;    long dFTYXbnQlN43826883 = -959458;    long dFTYXbnQlN89407069 = -887768634;    long dFTYXbnQlN4927255 = -109025624;    long dFTYXbnQlN50235189 = -427269614;    long dFTYXbnQlN58382330 = -606205502;    long dFTYXbnQlN86028157 = -757636324;    long dFTYXbnQlN22002189 = -534083526;    long dFTYXbnQlN97113519 = -502714983;    long dFTYXbnQlN1654232 = -124028047;    long dFTYXbnQlN71386993 = -247244423;    long dFTYXbnQlN27045784 = -622413444;    long dFTYXbnQlN24513490 = -320675030;    long dFTYXbnQlN12913794 = 17946139;    long dFTYXbnQlN68206325 = -73477184;    long dFTYXbnQlN61544336 = -728509138;    long dFTYXbnQlN66657746 = 30616658;    long dFTYXbnQlN98369079 = -249872595;    long dFTYXbnQlN60663565 = -902106384;    long dFTYXbnQlN64427739 = 14710765;    long dFTYXbnQlN49519529 = -96836742;    long dFTYXbnQlN49812251 = -562407518;    long dFTYXbnQlN64769079 = 71656377;    long dFTYXbnQlN38511164 = -699610668;    long dFTYXbnQlN62861310 = -249675004;    long dFTYXbnQlN26089280 = 17193215;    long dFTYXbnQlN98871183 = -759008851;    long dFTYXbnQlN26006674 = -853118807;    long dFTYXbnQlN3717729 = -515275749;    long dFTYXbnQlN23631822 = -662946601;    long dFTYXbnQlN87614431 = -208357547;    long dFTYXbnQlN27123208 = -429144690;    long dFTYXbnQlN49998057 = -573923487;    long dFTYXbnQlN29162640 = -75661179;    long dFTYXbnQlN59075621 = -854390920;    long dFTYXbnQlN89526382 = -459958357;    long dFTYXbnQlN59984065 = -729099611;    long dFTYXbnQlN43407702 = -61354709;    long dFTYXbnQlN54581077 = 78271267;    long dFTYXbnQlN38054436 = -683389445;    long dFTYXbnQlN96904002 = 97363358;    long dFTYXbnQlN97918246 = -782178090;    long dFTYXbnQlN85699005 = 12647491;    long dFTYXbnQlN20037325 = -798768982;    long dFTYXbnQlN49372074 = -360925019;    long dFTYXbnQlN13335924 = -162432193;    long dFTYXbnQlN86172717 = -479355213;    long dFTYXbnQlN64041023 = -903186804;    long dFTYXbnQlN59677612 = -491735975;    long dFTYXbnQlN53626669 = -407514689;    long dFTYXbnQlN59049544 = -524908959;    long dFTYXbnQlN65821065 = -874155753;    long dFTYXbnQlN64965173 = -963503229;    long dFTYXbnQlN12731466 = -848737990;    long dFTYXbnQlN31235097 = -746972090;    long dFTYXbnQlN32545571 = 61577687;    long dFTYXbnQlN79057804 = 27384164;    long dFTYXbnQlN50895906 = -88157966;    long dFTYXbnQlN42065945 = -859350620;    long dFTYXbnQlN24145910 = -344462829;    long dFTYXbnQlN59511147 = -847196651;    long dFTYXbnQlN60021484 = -904517518;    long dFTYXbnQlN18284460 = 81192222;    long dFTYXbnQlN73481697 = -839768382;    long dFTYXbnQlN14039800 = -915670500;    long dFTYXbnQlN44263786 = -818099734;    long dFTYXbnQlN77047726 = 51510043;    long dFTYXbnQlN95350850 = -145013852;    long dFTYXbnQlN53838172 = -127662941;    long dFTYXbnQlN78679942 = -613518828;    long dFTYXbnQlN1560271 = -999409527;    long dFTYXbnQlN23250044 = -908028633;    long dFTYXbnQlN43788003 = -751299575;     dFTYXbnQlN18888734 = dFTYXbnQlN49339112;     dFTYXbnQlN49339112 = dFTYXbnQlN51804576;     dFTYXbnQlN51804576 = dFTYXbnQlN52751946;     dFTYXbnQlN52751946 = dFTYXbnQlN27157261;     dFTYXbnQlN27157261 = dFTYXbnQlN9917265;     dFTYXbnQlN9917265 = dFTYXbnQlN41433443;     dFTYXbnQlN41433443 = dFTYXbnQlN33353266;     dFTYXbnQlN33353266 = dFTYXbnQlN49391135;     dFTYXbnQlN49391135 = dFTYXbnQlN48334957;     dFTYXbnQlN48334957 = dFTYXbnQlN4816266;     dFTYXbnQlN4816266 = dFTYXbnQlN96436765;     dFTYXbnQlN96436765 = dFTYXbnQlN82932160;     dFTYXbnQlN82932160 = dFTYXbnQlN19920436;     dFTYXbnQlN19920436 = dFTYXbnQlN82812524;     dFTYXbnQlN82812524 = dFTYXbnQlN21691557;     dFTYXbnQlN21691557 = dFTYXbnQlN20759068;     dFTYXbnQlN20759068 = dFTYXbnQlN40381707;     dFTYXbnQlN40381707 = dFTYXbnQlN10686207;     dFTYXbnQlN10686207 = dFTYXbnQlN76954816;     dFTYXbnQlN76954816 = dFTYXbnQlN27883938;     dFTYXbnQlN27883938 = dFTYXbnQlN15171005;     dFTYXbnQlN15171005 = dFTYXbnQlN25707291;     dFTYXbnQlN25707291 = dFTYXbnQlN64190144;     dFTYXbnQlN64190144 = dFTYXbnQlN25628738;     dFTYXbnQlN25628738 = dFTYXbnQlN77159204;     dFTYXbnQlN77159204 = dFTYXbnQlN80754625;     dFTYXbnQlN80754625 = dFTYXbnQlN82357821;     dFTYXbnQlN82357821 = dFTYXbnQlN43826883;     dFTYXbnQlN43826883 = dFTYXbnQlN89407069;     dFTYXbnQlN89407069 = dFTYXbnQlN4927255;     dFTYXbnQlN4927255 = dFTYXbnQlN50235189;     dFTYXbnQlN50235189 = dFTYXbnQlN58382330;     dFTYXbnQlN58382330 = dFTYXbnQlN86028157;     dFTYXbnQlN86028157 = dFTYXbnQlN22002189;     dFTYXbnQlN22002189 = dFTYXbnQlN97113519;     dFTYXbnQlN97113519 = dFTYXbnQlN1654232;     dFTYXbnQlN1654232 = dFTYXbnQlN71386993;     dFTYXbnQlN71386993 = dFTYXbnQlN27045784;     dFTYXbnQlN27045784 = dFTYXbnQlN24513490;     dFTYXbnQlN24513490 = dFTYXbnQlN12913794;     dFTYXbnQlN12913794 = dFTYXbnQlN68206325;     dFTYXbnQlN68206325 = dFTYXbnQlN61544336;     dFTYXbnQlN61544336 = dFTYXbnQlN66657746;     dFTYXbnQlN66657746 = dFTYXbnQlN98369079;     dFTYXbnQlN98369079 = dFTYXbnQlN60663565;     dFTYXbnQlN60663565 = dFTYXbnQlN64427739;     dFTYXbnQlN64427739 = dFTYXbnQlN49519529;     dFTYXbnQlN49519529 = dFTYXbnQlN49812251;     dFTYXbnQlN49812251 = dFTYXbnQlN64769079;     dFTYXbnQlN64769079 = dFTYXbnQlN38511164;     dFTYXbnQlN38511164 = dFTYXbnQlN62861310;     dFTYXbnQlN62861310 = dFTYXbnQlN26089280;     dFTYXbnQlN26089280 = dFTYXbnQlN98871183;     dFTYXbnQlN98871183 = dFTYXbnQlN26006674;     dFTYXbnQlN26006674 = dFTYXbnQlN3717729;     dFTYXbnQlN3717729 = dFTYXbnQlN23631822;     dFTYXbnQlN23631822 = dFTYXbnQlN87614431;     dFTYXbnQlN87614431 = dFTYXbnQlN27123208;     dFTYXbnQlN27123208 = dFTYXbnQlN49998057;     dFTYXbnQlN49998057 = dFTYXbnQlN29162640;     dFTYXbnQlN29162640 = dFTYXbnQlN59075621;     dFTYXbnQlN59075621 = dFTYXbnQlN89526382;     dFTYXbnQlN89526382 = dFTYXbnQlN59984065;     dFTYXbnQlN59984065 = dFTYXbnQlN43407702;     dFTYXbnQlN43407702 = dFTYXbnQlN54581077;     dFTYXbnQlN54581077 = dFTYXbnQlN38054436;     dFTYXbnQlN38054436 = dFTYXbnQlN96904002;     dFTYXbnQlN96904002 = dFTYXbnQlN97918246;     dFTYXbnQlN97918246 = dFTYXbnQlN85699005;     dFTYXbnQlN85699005 = dFTYXbnQlN20037325;     dFTYXbnQlN20037325 = dFTYXbnQlN49372074;     dFTYXbnQlN49372074 = dFTYXbnQlN13335924;     dFTYXbnQlN13335924 = dFTYXbnQlN86172717;     dFTYXbnQlN86172717 = dFTYXbnQlN64041023;     dFTYXbnQlN64041023 = dFTYXbnQlN59677612;     dFTYXbnQlN59677612 = dFTYXbnQlN53626669;     dFTYXbnQlN53626669 = dFTYXbnQlN59049544;     dFTYXbnQlN59049544 = dFTYXbnQlN65821065;     dFTYXbnQlN65821065 = dFTYXbnQlN64965173;     dFTYXbnQlN64965173 = dFTYXbnQlN12731466;     dFTYXbnQlN12731466 = dFTYXbnQlN31235097;     dFTYXbnQlN31235097 = dFTYXbnQlN32545571;     dFTYXbnQlN32545571 = dFTYXbnQlN79057804;     dFTYXbnQlN79057804 = dFTYXbnQlN50895906;     dFTYXbnQlN50895906 = dFTYXbnQlN42065945;     dFTYXbnQlN42065945 = dFTYXbnQlN24145910;     dFTYXbnQlN24145910 = dFTYXbnQlN59511147;     dFTYXbnQlN59511147 = dFTYXbnQlN60021484;     dFTYXbnQlN60021484 = dFTYXbnQlN18284460;     dFTYXbnQlN18284460 = dFTYXbnQlN73481697;     dFTYXbnQlN73481697 = dFTYXbnQlN14039800;     dFTYXbnQlN14039800 = dFTYXbnQlN44263786;     dFTYXbnQlN44263786 = dFTYXbnQlN77047726;     dFTYXbnQlN77047726 = dFTYXbnQlN95350850;     dFTYXbnQlN95350850 = dFTYXbnQlN53838172;     dFTYXbnQlN53838172 = dFTYXbnQlN78679942;     dFTYXbnQlN78679942 = dFTYXbnQlN1560271;     dFTYXbnQlN1560271 = dFTYXbnQlN23250044;     dFTYXbnQlN23250044 = dFTYXbnQlN43788003;     dFTYXbnQlN43788003 = dFTYXbnQlN18888734;}
// Junk Finished

// Junk Code By Troll Face & Thaisen's Gen
void PisIcSBMTu12131492() {     long wTAdmhuAfC57545862 = 91794388;    long wTAdmhuAfC74660833 = -444725230;    long wTAdmhuAfC93300949 = -760860661;    long wTAdmhuAfC55690542 = -116820735;    long wTAdmhuAfC54610280 = -69414842;    long wTAdmhuAfC48266296 = -576190530;    long wTAdmhuAfC65255954 = 39769615;    long wTAdmhuAfC34861092 = -115237190;    long wTAdmhuAfC79074901 = -617227657;    long wTAdmhuAfC90632467 = -526887521;    long wTAdmhuAfC19789695 = -564809561;    long wTAdmhuAfC76419621 = -346679331;    long wTAdmhuAfC30739403 = -457816516;    long wTAdmhuAfC22006263 = -839440562;    long wTAdmhuAfC60540652 = -254560751;    long wTAdmhuAfC68697988 = -713132570;    long wTAdmhuAfC26569768 = -646183969;    long wTAdmhuAfC55281387 = 77191626;    long wTAdmhuAfC31419627 = -764465396;    long wTAdmhuAfC53437218 = -33606653;    long wTAdmhuAfC41096959 = -792700320;    long wTAdmhuAfC56280418 = -137626737;    long wTAdmhuAfC96703144 = -99935998;    long wTAdmhuAfC33197345 = 70290926;    long wTAdmhuAfC53009582 = -835531828;    long wTAdmhuAfC29043518 = -843160349;    long wTAdmhuAfC76473688 = -558319614;    long wTAdmhuAfC81073596 = -111809784;    long wTAdmhuAfC91479749 = -467090991;    long wTAdmhuAfC39105792 = -910889500;    long wTAdmhuAfC1266316 = -358336990;    long wTAdmhuAfC67706949 = -921965507;    long wTAdmhuAfC10748723 = -672279121;    long wTAdmhuAfC9974968 = -554348259;    long wTAdmhuAfC12986266 = -22863266;    long wTAdmhuAfC11773344 = -919479629;    long wTAdmhuAfC7271722 = -736288080;    long wTAdmhuAfC44548983 = -210989358;    long wTAdmhuAfC50051792 = -330582010;    long wTAdmhuAfC77382981 = -119683866;    long wTAdmhuAfC64759311 = -923135299;    long wTAdmhuAfC13072651 = -419152288;    long wTAdmhuAfC94577741 = -422765721;    long wTAdmhuAfC78138244 = 55232298;    long wTAdmhuAfC22383148 = -206261396;    long wTAdmhuAfC64924934 = -181348458;    long wTAdmhuAfC71243959 = 76665797;    long wTAdmhuAfC30759937 = -700011191;    long wTAdmhuAfC29056332 = -376103926;    long wTAdmhuAfC46024917 = -125373332;    long wTAdmhuAfC20548919 = -467469551;    long wTAdmhuAfC99754105 = -210587248;    long wTAdmhuAfC34469629 = -634839658;    long wTAdmhuAfC23475393 = -989900323;    long wTAdmhuAfC71034533 = -433995255;    long wTAdmhuAfC1265444 = -770578876;    long wTAdmhuAfC77957688 = -244789233;    long wTAdmhuAfC60103605 = -731151587;    long wTAdmhuAfC2680961 = -281288907;    long wTAdmhuAfC25566763 = -226254494;    long wTAdmhuAfC71792608 = 82129083;    long wTAdmhuAfC84182358 = -848420602;    long wTAdmhuAfC43381343 = -648146199;    long wTAdmhuAfC39969110 = -706338158;    long wTAdmhuAfC89366151 = -68550532;    long wTAdmhuAfC52082746 = -642844054;    long wTAdmhuAfC65670899 = -674400211;    long wTAdmhuAfC20764435 = -903468258;    long wTAdmhuAfC9019998 = -716577296;    long wTAdmhuAfC48767308 = -335081123;    long wTAdmhuAfC61426267 = -976844490;    long wTAdmhuAfC82020785 = -335194612;    long wTAdmhuAfC5229595 = -592226365;    long wTAdmhuAfC54036646 = -544781530;    long wTAdmhuAfC88677907 = -110471354;    long wTAdmhuAfC28024308 = -273548032;    long wTAdmhuAfC61702676 = -714861016;    long wTAdmhuAfC18564901 = -55168296;    long wTAdmhuAfC10814197 = -723447679;    long wTAdmhuAfC88084647 = -554183371;    long wTAdmhuAfC57799558 = -819826146;    long wTAdmhuAfC45713751 = -858308424;    long wTAdmhuAfC52017264 = -735705858;    long wTAdmhuAfC45454832 = -241717660;    long wTAdmhuAfC18556873 = -343419949;    long wTAdmhuAfC1512211 = -47749742;    long wTAdmhuAfC33237320 = -187125850;    long wTAdmhuAfC87273329 = -682378798;    long wTAdmhuAfC38940434 = -20353004;    long wTAdmhuAfC11720823 = -252284391;    long wTAdmhuAfC33815656 = -574690396;    long wTAdmhuAfC47168117 = 94863507;    long wTAdmhuAfC41868023 = -929700451;    long wTAdmhuAfC24485030 = -4327517;    long wTAdmhuAfC5590373 = -101812950;    long wTAdmhuAfC80576953 = 25285302;    long wTAdmhuAfC69691307 = -771006090;    long wTAdmhuAfC54608632 = -716427564;    long wTAdmhuAfC88772093 = -876217170;    long wTAdmhuAfC70300402 = 91794388;     wTAdmhuAfC57545862 = wTAdmhuAfC74660833;     wTAdmhuAfC74660833 = wTAdmhuAfC93300949;     wTAdmhuAfC93300949 = wTAdmhuAfC55690542;     wTAdmhuAfC55690542 = wTAdmhuAfC54610280;     wTAdmhuAfC54610280 = wTAdmhuAfC48266296;     wTAdmhuAfC48266296 = wTAdmhuAfC65255954;     wTAdmhuAfC65255954 = wTAdmhuAfC34861092;     wTAdmhuAfC34861092 = wTAdmhuAfC79074901;     wTAdmhuAfC79074901 = wTAdmhuAfC90632467;     wTAdmhuAfC90632467 = wTAdmhuAfC19789695;     wTAdmhuAfC19789695 = wTAdmhuAfC76419621;     wTAdmhuAfC76419621 = wTAdmhuAfC30739403;     wTAdmhuAfC30739403 = wTAdmhuAfC22006263;     wTAdmhuAfC22006263 = wTAdmhuAfC60540652;     wTAdmhuAfC60540652 = wTAdmhuAfC68697988;     wTAdmhuAfC68697988 = wTAdmhuAfC26569768;     wTAdmhuAfC26569768 = wTAdmhuAfC55281387;     wTAdmhuAfC55281387 = wTAdmhuAfC31419627;     wTAdmhuAfC31419627 = wTAdmhuAfC53437218;     wTAdmhuAfC53437218 = wTAdmhuAfC41096959;     wTAdmhuAfC41096959 = wTAdmhuAfC56280418;     wTAdmhuAfC56280418 = wTAdmhuAfC96703144;     wTAdmhuAfC96703144 = wTAdmhuAfC33197345;     wTAdmhuAfC33197345 = wTAdmhuAfC53009582;     wTAdmhuAfC53009582 = wTAdmhuAfC29043518;     wTAdmhuAfC29043518 = wTAdmhuAfC76473688;     wTAdmhuAfC76473688 = wTAdmhuAfC81073596;     wTAdmhuAfC81073596 = wTAdmhuAfC91479749;     wTAdmhuAfC91479749 = wTAdmhuAfC39105792;     wTAdmhuAfC39105792 = wTAdmhuAfC1266316;     wTAdmhuAfC1266316 = wTAdmhuAfC67706949;     wTAdmhuAfC67706949 = wTAdmhuAfC10748723;     wTAdmhuAfC10748723 = wTAdmhuAfC9974968;     wTAdmhuAfC9974968 = wTAdmhuAfC12986266;     wTAdmhuAfC12986266 = wTAdmhuAfC11773344;     wTAdmhuAfC11773344 = wTAdmhuAfC7271722;     wTAdmhuAfC7271722 = wTAdmhuAfC44548983;     wTAdmhuAfC44548983 = wTAdmhuAfC50051792;     wTAdmhuAfC50051792 = wTAdmhuAfC77382981;     wTAdmhuAfC77382981 = wTAdmhuAfC64759311;     wTAdmhuAfC64759311 = wTAdmhuAfC13072651;     wTAdmhuAfC13072651 = wTAdmhuAfC94577741;     wTAdmhuAfC94577741 = wTAdmhuAfC78138244;     wTAdmhuAfC78138244 = wTAdmhuAfC22383148;     wTAdmhuAfC22383148 = wTAdmhuAfC64924934;     wTAdmhuAfC64924934 = wTAdmhuAfC71243959;     wTAdmhuAfC71243959 = wTAdmhuAfC30759937;     wTAdmhuAfC30759937 = wTAdmhuAfC29056332;     wTAdmhuAfC29056332 = wTAdmhuAfC46024917;     wTAdmhuAfC46024917 = wTAdmhuAfC20548919;     wTAdmhuAfC20548919 = wTAdmhuAfC99754105;     wTAdmhuAfC99754105 = wTAdmhuAfC34469629;     wTAdmhuAfC34469629 = wTAdmhuAfC23475393;     wTAdmhuAfC23475393 = wTAdmhuAfC71034533;     wTAdmhuAfC71034533 = wTAdmhuAfC1265444;     wTAdmhuAfC1265444 = wTAdmhuAfC77957688;     wTAdmhuAfC77957688 = wTAdmhuAfC60103605;     wTAdmhuAfC60103605 = wTAdmhuAfC2680961;     wTAdmhuAfC2680961 = wTAdmhuAfC25566763;     wTAdmhuAfC25566763 = wTAdmhuAfC71792608;     wTAdmhuAfC71792608 = wTAdmhuAfC84182358;     wTAdmhuAfC84182358 = wTAdmhuAfC43381343;     wTAdmhuAfC43381343 = wTAdmhuAfC39969110;     wTAdmhuAfC39969110 = wTAdmhuAfC89366151;     wTAdmhuAfC89366151 = wTAdmhuAfC52082746;     wTAdmhuAfC52082746 = wTAdmhuAfC65670899;     wTAdmhuAfC65670899 = wTAdmhuAfC20764435;     wTAdmhuAfC20764435 = wTAdmhuAfC9019998;     wTAdmhuAfC9019998 = wTAdmhuAfC48767308;     wTAdmhuAfC48767308 = wTAdmhuAfC61426267;     wTAdmhuAfC61426267 = wTAdmhuAfC82020785;     wTAdmhuAfC82020785 = wTAdmhuAfC5229595;     wTAdmhuAfC5229595 = wTAdmhuAfC54036646;     wTAdmhuAfC54036646 = wTAdmhuAfC88677907;     wTAdmhuAfC88677907 = wTAdmhuAfC28024308;     wTAdmhuAfC28024308 = wTAdmhuAfC61702676;     wTAdmhuAfC61702676 = wTAdmhuAfC18564901;     wTAdmhuAfC18564901 = wTAdmhuAfC10814197;     wTAdmhuAfC10814197 = wTAdmhuAfC88084647;     wTAdmhuAfC88084647 = wTAdmhuAfC57799558;     wTAdmhuAfC57799558 = wTAdmhuAfC45713751;     wTAdmhuAfC45713751 = wTAdmhuAfC52017264;     wTAdmhuAfC52017264 = wTAdmhuAfC45454832;     wTAdmhuAfC45454832 = wTAdmhuAfC18556873;     wTAdmhuAfC18556873 = wTAdmhuAfC1512211;     wTAdmhuAfC1512211 = wTAdmhuAfC33237320;     wTAdmhuAfC33237320 = wTAdmhuAfC87273329;     wTAdmhuAfC87273329 = wTAdmhuAfC38940434;     wTAdmhuAfC38940434 = wTAdmhuAfC11720823;     wTAdmhuAfC11720823 = wTAdmhuAfC33815656;     wTAdmhuAfC33815656 = wTAdmhuAfC47168117;     wTAdmhuAfC47168117 = wTAdmhuAfC41868023;     wTAdmhuAfC41868023 = wTAdmhuAfC24485030;     wTAdmhuAfC24485030 = wTAdmhuAfC5590373;     wTAdmhuAfC5590373 = wTAdmhuAfC80576953;     wTAdmhuAfC80576953 = wTAdmhuAfC69691307;     wTAdmhuAfC69691307 = wTAdmhuAfC54608632;     wTAdmhuAfC54608632 = wTAdmhuAfC88772093;     wTAdmhuAfC88772093 = wTAdmhuAfC70300402;     wTAdmhuAfC70300402 = wTAdmhuAfC57545862;}
// Junk Finished

// Junk Code By Troll Face & Thaisen's Gen
void nxLanfsVip70042176() {     long ERVMWKlhOJ50863858 = -730115092;    long ERVMWKlhOJ8407858 = -69181247;    long ERVMWKlhOJ94216757 = -417591428;    long ERVMWKlhOJ44984218 = -124448749;    long ERVMWKlhOJ47674869 = 65041109;    long ERVMWKlhOJ65984293 = -462629075;    long ERVMWKlhOJ41631834 = -62596225;    long ERVMWKlhOJ96329971 = -465947481;    long ERVMWKlhOJ13792915 = -950794370;    long ERVMWKlhOJ18922449 = -325389108;    long ERVMWKlhOJ93053813 = -246505276;    long ERVMWKlhOJ4875513 = -854709442;    long ERVMWKlhOJ10511046 = 62758103;    long ERVMWKlhOJ63214301 = -629570035;    long ERVMWKlhOJ23320744 = -963462791;    long ERVMWKlhOJ30907408 = -800832025;    long ERVMWKlhOJ76359472 = -138183754;    long ERVMWKlhOJ14777121 = -827263958;    long ERVMWKlhOJ32945378 = -296729831;    long ERVMWKlhOJ75480413 = 84685664;    long ERVMWKlhOJ8814944 = -677804259;    long ERVMWKlhOJ70346154 = -895681872;    long ERVMWKlhOJ67290434 = -833365274;    long ERVMWKlhOJ59982806 = -653510095;    long ERVMWKlhOJ47700352 = -751673643;    long ERVMWKlhOJ5594702 = -847796330;    long ERVMWKlhOJ3961094 = -426874962;    long ERVMWKlhOJ97634846 = -121874609;    long ERVMWKlhOJ66351845 = -667397086;    long ERVMWKlhOJ7150054 = -449336960;    long ERVMWKlhOJ15898227 = -769427647;    long ERVMWKlhOJ31446462 = -336336167;    long ERVMWKlhOJ45318568 = 7073861;    long ERVMWKlhOJ28294834 = -154499271;    long ERVMWKlhOJ56116462 = -570192340;    long ERVMWKlhOJ70931521 = -404474124;    long ERVMWKlhOJ75652331 = 678645;    long ERVMWKlhOJ60770531 = -53400634;    long ERVMWKlhOJ88513853 = -808017762;    long ERVMWKlhOJ82365974 = 32430385;    long ERVMWKlhOJ84708562 = -788822997;    long ERVMWKlhOJ8915974 = -78567397;    long ERVMWKlhOJ58327970 = -498397369;    long ERVMWKlhOJ39381244 = -936867284;    long ERVMWKlhOJ22363228 = -921908393;    long ERVMWKlhOJ84091936 = -817335476;    long ERVMWKlhOJ66836099 = -507155933;    long ERVMWKlhOJ60044074 = -672076656;    long ERVMWKlhOJ31984790 = -525766663;    long ERVMWKlhOJ52076996 = -975397450;    long ERVMWKlhOJ64440843 = -313988928;    long ERVMWKlhOJ11481793 = -304419726;    long ERVMWKlhOJ99129632 = -34733979;    long ERVMWKlhOJ95245089 = -584758429;    long ERVMWKlhOJ61227233 = -653881107;    long ERVMWKlhOJ80517704 = -834433221;    long ERVMWKlhOJ41117423 = -235815974;    long ERVMWKlhOJ34233952 = -764081334;    long ERVMWKlhOJ97283866 = -372775106;    long ERVMWKlhOJ42080167 = -87162562;    long ERVMWKlhOJ62023199 = 64245887;    long ERVMWKlhOJ43996988 = -940721617;    long ERVMWKlhOJ29978127 = -798550395;    long ERVMWKlhOJ6642862 = -401457411;    long ERVMWKlhOJ3024223 = -555961462;    long ERVMWKlhOJ61607351 = -910169109;    long ERVMWKlhOJ59556945 = -761783304;    long ERVMWKlhOJ82216212 = -782742626;    long ERVMWKlhOJ7097840 = 40622305;    long ERVMWKlhOJ52389222 = -458988667;    long ERVMWKlhOJ55255076 = -701510670;    long ERVMWKlhOJ15588941 = 15216879;    long ERVMWKlhOJ26263267 = 80753803;    long ERVMWKlhOJ50579404 = -229160216;    long ERVMWKlhOJ90771850 = -126491340;    long ERVMWKlhOJ99898969 = -499236862;    long ERVMWKlhOJ12018184 = -297284504;    long ERVMWKlhOJ27909191 = -896497990;    long ERVMWKlhOJ37619579 = -731601702;    long ERVMWKlhOJ63608415 = -934338167;    long ERVMWKlhOJ38758603 = -240640397;    long ERVMWKlhOJ43917020 = -754798307;    long ERVMWKlhOJ65650056 = -596107947;    long ERVMWKlhOJ14274850 = -691999636;    long ERVMWKlhOJ42709210 = -35348032;    long ERVMWKlhOJ4416435 = -365007921;    long ERVMWKlhOJ32316830 = -201602189;    long ERVMWKlhOJ50073478 = -408167710;    long ERVMWKlhOJ67067601 = -500618165;    long ERVMWKlhOJ75598757 = -735759120;    long ERVMWKlhOJ29814099 = -68658151;    long ERVMWKlhOJ41418379 = -235240022;    long ERVMWKlhOJ63486664 = -680625528;    long ERVMWKlhOJ46433686 = -620855201;    long ERVMWKlhOJ20342775 = 68184497;    long ERVMWKlhOJ40711574 = -848101380;    long ERVMWKlhOJ78937847 = -280017002;    long ERVMWKlhOJ51685108 = 3060042;    long ERVMWKlhOJ36357022 = -280905823;    long ERVMWKlhOJ60755876 = -730115092;     ERVMWKlhOJ50863858 = ERVMWKlhOJ8407858;     ERVMWKlhOJ8407858 = ERVMWKlhOJ94216757;     ERVMWKlhOJ94216757 = ERVMWKlhOJ44984218;     ERVMWKlhOJ44984218 = ERVMWKlhOJ47674869;     ERVMWKlhOJ47674869 = ERVMWKlhOJ65984293;     ERVMWKlhOJ65984293 = ERVMWKlhOJ41631834;     ERVMWKlhOJ41631834 = ERVMWKlhOJ96329971;     ERVMWKlhOJ96329971 = ERVMWKlhOJ13792915;     ERVMWKlhOJ13792915 = ERVMWKlhOJ18922449;     ERVMWKlhOJ18922449 = ERVMWKlhOJ93053813;     ERVMWKlhOJ93053813 = ERVMWKlhOJ4875513;     ERVMWKlhOJ4875513 = ERVMWKlhOJ10511046;     ERVMWKlhOJ10511046 = ERVMWKlhOJ63214301;     ERVMWKlhOJ63214301 = ERVMWKlhOJ23320744;     ERVMWKlhOJ23320744 = ERVMWKlhOJ30907408;     ERVMWKlhOJ30907408 = ERVMWKlhOJ76359472;     ERVMWKlhOJ76359472 = ERVMWKlhOJ14777121;     ERVMWKlhOJ14777121 = ERVMWKlhOJ32945378;     ERVMWKlhOJ32945378 = ERVMWKlhOJ75480413;     ERVMWKlhOJ75480413 = ERVMWKlhOJ8814944;     ERVMWKlhOJ8814944 = ERVMWKlhOJ70346154;     ERVMWKlhOJ70346154 = ERVMWKlhOJ67290434;     ERVMWKlhOJ67290434 = ERVMWKlhOJ59982806;     ERVMWKlhOJ59982806 = ERVMWKlhOJ47700352;     ERVMWKlhOJ47700352 = ERVMWKlhOJ5594702;     ERVMWKlhOJ5594702 = ERVMWKlhOJ3961094;     ERVMWKlhOJ3961094 = ERVMWKlhOJ97634846;     ERVMWKlhOJ97634846 = ERVMWKlhOJ66351845;     ERVMWKlhOJ66351845 = ERVMWKlhOJ7150054;     ERVMWKlhOJ7150054 = ERVMWKlhOJ15898227;     ERVMWKlhOJ15898227 = ERVMWKlhOJ31446462;     ERVMWKlhOJ31446462 = ERVMWKlhOJ45318568;     ERVMWKlhOJ45318568 = ERVMWKlhOJ28294834;     ERVMWKlhOJ28294834 = ERVMWKlhOJ56116462;     ERVMWKlhOJ56116462 = ERVMWKlhOJ70931521;     ERVMWKlhOJ70931521 = ERVMWKlhOJ75652331;     ERVMWKlhOJ75652331 = ERVMWKlhOJ60770531;     ERVMWKlhOJ60770531 = ERVMWKlhOJ88513853;     ERVMWKlhOJ88513853 = ERVMWKlhOJ82365974;     ERVMWKlhOJ82365974 = ERVMWKlhOJ84708562;     ERVMWKlhOJ84708562 = ERVMWKlhOJ8915974;     ERVMWKlhOJ8915974 = ERVMWKlhOJ58327970;     ERVMWKlhOJ58327970 = ERVMWKlhOJ39381244;     ERVMWKlhOJ39381244 = ERVMWKlhOJ22363228;     ERVMWKlhOJ22363228 = ERVMWKlhOJ84091936;     ERVMWKlhOJ84091936 = ERVMWKlhOJ66836099;     ERVMWKlhOJ66836099 = ERVMWKlhOJ60044074;     ERVMWKlhOJ60044074 = ERVMWKlhOJ31984790;     ERVMWKlhOJ31984790 = ERVMWKlhOJ52076996;     ERVMWKlhOJ52076996 = ERVMWKlhOJ64440843;     ERVMWKlhOJ64440843 = ERVMWKlhOJ11481793;     ERVMWKlhOJ11481793 = ERVMWKlhOJ99129632;     ERVMWKlhOJ99129632 = ERVMWKlhOJ95245089;     ERVMWKlhOJ95245089 = ERVMWKlhOJ61227233;     ERVMWKlhOJ61227233 = ERVMWKlhOJ80517704;     ERVMWKlhOJ80517704 = ERVMWKlhOJ41117423;     ERVMWKlhOJ41117423 = ERVMWKlhOJ34233952;     ERVMWKlhOJ34233952 = ERVMWKlhOJ97283866;     ERVMWKlhOJ97283866 = ERVMWKlhOJ42080167;     ERVMWKlhOJ42080167 = ERVMWKlhOJ62023199;     ERVMWKlhOJ62023199 = ERVMWKlhOJ43996988;     ERVMWKlhOJ43996988 = ERVMWKlhOJ29978127;     ERVMWKlhOJ29978127 = ERVMWKlhOJ6642862;     ERVMWKlhOJ6642862 = ERVMWKlhOJ3024223;     ERVMWKlhOJ3024223 = ERVMWKlhOJ61607351;     ERVMWKlhOJ61607351 = ERVMWKlhOJ59556945;     ERVMWKlhOJ59556945 = ERVMWKlhOJ82216212;     ERVMWKlhOJ82216212 = ERVMWKlhOJ7097840;     ERVMWKlhOJ7097840 = ERVMWKlhOJ52389222;     ERVMWKlhOJ52389222 = ERVMWKlhOJ55255076;     ERVMWKlhOJ55255076 = ERVMWKlhOJ15588941;     ERVMWKlhOJ15588941 = ERVMWKlhOJ26263267;     ERVMWKlhOJ26263267 = ERVMWKlhOJ50579404;     ERVMWKlhOJ50579404 = ERVMWKlhOJ90771850;     ERVMWKlhOJ90771850 = ERVMWKlhOJ99898969;     ERVMWKlhOJ99898969 = ERVMWKlhOJ12018184;     ERVMWKlhOJ12018184 = ERVMWKlhOJ27909191;     ERVMWKlhOJ27909191 = ERVMWKlhOJ37619579;     ERVMWKlhOJ37619579 = ERVMWKlhOJ63608415;     ERVMWKlhOJ63608415 = ERVMWKlhOJ38758603;     ERVMWKlhOJ38758603 = ERVMWKlhOJ43917020;     ERVMWKlhOJ43917020 = ERVMWKlhOJ65650056;     ERVMWKlhOJ65650056 = ERVMWKlhOJ14274850;     ERVMWKlhOJ14274850 = ERVMWKlhOJ42709210;     ERVMWKlhOJ42709210 = ERVMWKlhOJ4416435;     ERVMWKlhOJ4416435 = ERVMWKlhOJ32316830;     ERVMWKlhOJ32316830 = ERVMWKlhOJ50073478;     ERVMWKlhOJ50073478 = ERVMWKlhOJ67067601;     ERVMWKlhOJ67067601 = ERVMWKlhOJ75598757;     ERVMWKlhOJ75598757 = ERVMWKlhOJ29814099;     ERVMWKlhOJ29814099 = ERVMWKlhOJ41418379;     ERVMWKlhOJ41418379 = ERVMWKlhOJ63486664;     ERVMWKlhOJ63486664 = ERVMWKlhOJ46433686;     ERVMWKlhOJ46433686 = ERVMWKlhOJ20342775;     ERVMWKlhOJ20342775 = ERVMWKlhOJ40711574;     ERVMWKlhOJ40711574 = ERVMWKlhOJ78937847;     ERVMWKlhOJ78937847 = ERVMWKlhOJ51685108;     ERVMWKlhOJ51685108 = ERVMWKlhOJ36357022;     ERVMWKlhOJ36357022 = ERVMWKlhOJ60755876;     ERVMWKlhOJ60755876 = ERVMWKlhOJ50863858;}
// Junk Finished

// Junk Code By Troll Face & Thaisen's Gen
void LJEnYlHRij6356954() {     long CwSvsZbFrf27911845 = -929522498;    long CwSvsZbFrf1180661 = -554693991;    long CwSvsZbFrf82405470 = 14498728;    long CwSvsZbFrf46345988 = -316417861;    long CwSvsZbFrf11460692 = -830095810;    long CwSvsZbFrf60236374 = -752955478;    long CwSvsZbFrf86340940 = -481939737;    long CwSvsZbFrf31924884 = -632935040;    long CwSvsZbFrf15990293 = -68197710;    long CwSvsZbFrf96678505 = -710124839;    long CwSvsZbFrf88886362 = -909014177;    long CwSvsZbFrf9251498 = -456400462;    long CwSvsZbFrf9483628 = -337841047;    long CwSvsZbFrf44951264 = -424229318;    long CwSvsZbFrf45107985 = -535349283;    long CwSvsZbFrf90366308 = -479863481;    long CwSvsZbFrf70161828 = 30551167;    long CwSvsZbFrf65349591 = -945499495;    long CwSvsZbFrf55019340 = -99661237;    long CwSvsZbFrf99557488 = 98709516;    long CwSvsZbFrf64203014 = -426908344;    long CwSvsZbFrf7117696 = 43099590;    long CwSvsZbFrf70175192 = -37231445;    long CwSvsZbFrf44260854 = 64376754;    long CwSvsZbFrf50550525 = -319452367;    long CwSvsZbFrf18564567 = -287193410;    long CwSvsZbFrf82818856 = -438506021;    long CwSvsZbFrf21391772 = -409413935;    long CwSvsZbFrf463637 = -745828765;    long CwSvsZbFrf34652772 = -984959421;    long CwSvsZbFrf78481882 = 87476910;    long CwSvsZbFrf35255406 = 91917711;    long CwSvsZbFrf8560692 = -876843032;    long CwSvsZbFrf82056576 = -282065198;    long CwSvsZbFrf84491945 = -592498098;    long CwSvsZbFrf57122743 = -888099351;    long CwSvsZbFrf31855163 = -583042549;    long CwSvsZbFrf57456192 = -899692447;    long CwSvsZbFrf52278037 = -878330979;    long CwSvsZbFrf54665191 = -152670558;    long CwSvsZbFrf81569666 = -216606922;    long CwSvsZbFrf48401362 = -250216092;    long CwSvsZbFrf94594984 = -767303016;    long CwSvsZbFrf88587864 = -850946155;    long CwSvsZbFrf66312505 = -390310067;    long CwSvsZbFrf7097852 = -156309516;    long CwSvsZbFrf67909415 = -631342947;    long CwSvsZbFrf52686476 = -897502663;    long CwSvsZbFrf50129072 = -811449688;    long CwSvsZbFrf73956980 = -670412117;    long CwSvsZbFrf35164229 = -747725700;    long CwSvsZbFrf60167942 = -868159243;    long CwSvsZbFrf33723691 = -654506658;    long CwSvsZbFrf2697122 = -224405871;    long CwSvsZbFrf42153940 = -333594625;    long CwSvsZbFrf20794149 = -872622089;    long CwSvsZbFrf31005469 = -417462546;    long CwSvsZbFrf38144617 = 50121974;    long CwSvsZbFrf95795462 = -996965495;    long CwSvsZbFrf92896125 = -442902401;    long CwSvsZbFrf77417518 = -214449457;    long CwSvsZbFrf64949169 = 27474198;    long CwSvsZbFrf31461247 = -887106276;    long CwSvsZbFrf81337520 = -83238290;    long CwSvsZbFrf18196624 = -697601750;    long CwSvsZbFrf53630957 = -900931889;    long CwSvsZbFrf690806 = -579557431;    long CwSvsZbFrf27427051 = 44224151;    long CwSvsZbFrf60459319 = -831731221;    long CwSvsZbFrf87985241 = -647249933;    long CwSvsZbFrf58511145 = -896820932;    long CwSvsZbFrf12705637 = -69756386;    long CwSvsZbFrf13071554 = 32831484;    long CwSvsZbFrf354149 = -946990680;    long CwSvsZbFrf17987823 = -684683562;    long CwSvsZbFrf15801652 = -76692253;    long CwSvsZbFrf12522711 = -189597394;    long CwSvsZbFrf81587328 = -186285290;    long CwSvsZbFrf77948348 = -545313179;    long CwSvsZbFrf43452673 = -63142851;    long CwSvsZbFrf50655151 = -655850463;    long CwSvsZbFrf30132380 = -541003359;    long CwSvsZbFrf71262699 = -597964247;    long CwSvsZbFrf26506656 = 24583351;    long CwSvsZbFrf99488542 = -137233721;    long CwSvsZbFrf18313940 = -44363847;    long CwSvsZbFrf1531716 = -253575631;    long CwSvsZbFrf5863571 = -552437161;    long CwSvsZbFrf39902637 = -948470573;    long CwSvsZbFrf63697796 = -719876010;    long CwSvsZbFrf26117275 = -370636805;    long CwSvsZbFrf93710546 = -533164523;    long CwSvsZbFrf61660729 = -902726953;    long CwSvsZbFrf59381912 = -335428579;    long CwSvsZbFrf77247673 = -938221102;    long CwSvsZbFrf16620497 = -144081120;    long CwSvsZbFrf16940115 = -363109817;    long CwSvsZbFrf13257464 = -584064727;    long CwSvsZbFrf70391240 = -53344406;    long CwSvsZbFrf12681549 = -929522498;     CwSvsZbFrf27911845 = CwSvsZbFrf1180661;     CwSvsZbFrf1180661 = CwSvsZbFrf82405470;     CwSvsZbFrf82405470 = CwSvsZbFrf46345988;     CwSvsZbFrf46345988 = CwSvsZbFrf11460692;     CwSvsZbFrf11460692 = CwSvsZbFrf60236374;     CwSvsZbFrf60236374 = CwSvsZbFrf86340940;     CwSvsZbFrf86340940 = CwSvsZbFrf31924884;     CwSvsZbFrf31924884 = CwSvsZbFrf15990293;     CwSvsZbFrf15990293 = CwSvsZbFrf96678505;     CwSvsZbFrf96678505 = CwSvsZbFrf88886362;     CwSvsZbFrf88886362 = CwSvsZbFrf9251498;     CwSvsZbFrf9251498 = CwSvsZbFrf9483628;     CwSvsZbFrf9483628 = CwSvsZbFrf44951264;     CwSvsZbFrf44951264 = CwSvsZbFrf45107985;     CwSvsZbFrf45107985 = CwSvsZbFrf90366308;     CwSvsZbFrf90366308 = CwSvsZbFrf70161828;     CwSvsZbFrf70161828 = CwSvsZbFrf65349591;     CwSvsZbFrf65349591 = CwSvsZbFrf55019340;     CwSvsZbFrf55019340 = CwSvsZbFrf99557488;     CwSvsZbFrf99557488 = CwSvsZbFrf64203014;     CwSvsZbFrf64203014 = CwSvsZbFrf7117696;     CwSvsZbFrf7117696 = CwSvsZbFrf70175192;     CwSvsZbFrf70175192 = CwSvsZbFrf44260854;     CwSvsZbFrf44260854 = CwSvsZbFrf50550525;     CwSvsZbFrf50550525 = CwSvsZbFrf18564567;     CwSvsZbFrf18564567 = CwSvsZbFrf82818856;     CwSvsZbFrf82818856 = CwSvsZbFrf21391772;     CwSvsZbFrf21391772 = CwSvsZbFrf463637;     CwSvsZbFrf463637 = CwSvsZbFrf34652772;     CwSvsZbFrf34652772 = CwSvsZbFrf78481882;     CwSvsZbFrf78481882 = CwSvsZbFrf35255406;     CwSvsZbFrf35255406 = CwSvsZbFrf8560692;     CwSvsZbFrf8560692 = CwSvsZbFrf82056576;     CwSvsZbFrf82056576 = CwSvsZbFrf84491945;     CwSvsZbFrf84491945 = CwSvsZbFrf57122743;     CwSvsZbFrf57122743 = CwSvsZbFrf31855163;     CwSvsZbFrf31855163 = CwSvsZbFrf57456192;     CwSvsZbFrf57456192 = CwSvsZbFrf52278037;     CwSvsZbFrf52278037 = CwSvsZbFrf54665191;     CwSvsZbFrf54665191 = CwSvsZbFrf81569666;     CwSvsZbFrf81569666 = CwSvsZbFrf48401362;     CwSvsZbFrf48401362 = CwSvsZbFrf94594984;     CwSvsZbFrf94594984 = CwSvsZbFrf88587864;     CwSvsZbFrf88587864 = CwSvsZbFrf66312505;     CwSvsZbFrf66312505 = CwSvsZbFrf7097852;     CwSvsZbFrf7097852 = CwSvsZbFrf67909415;     CwSvsZbFrf67909415 = CwSvsZbFrf52686476;     CwSvsZbFrf52686476 = CwSvsZbFrf50129072;     CwSvsZbFrf50129072 = CwSvsZbFrf73956980;     CwSvsZbFrf73956980 = CwSvsZbFrf35164229;     CwSvsZbFrf35164229 = CwSvsZbFrf60167942;     CwSvsZbFrf60167942 = CwSvsZbFrf33723691;     CwSvsZbFrf33723691 = CwSvsZbFrf2697122;     CwSvsZbFrf2697122 = CwSvsZbFrf42153940;     CwSvsZbFrf42153940 = CwSvsZbFrf20794149;     CwSvsZbFrf20794149 = CwSvsZbFrf31005469;     CwSvsZbFrf31005469 = CwSvsZbFrf38144617;     CwSvsZbFrf38144617 = CwSvsZbFrf95795462;     CwSvsZbFrf95795462 = CwSvsZbFrf92896125;     CwSvsZbFrf92896125 = CwSvsZbFrf77417518;     CwSvsZbFrf77417518 = CwSvsZbFrf64949169;     CwSvsZbFrf64949169 = CwSvsZbFrf31461247;     CwSvsZbFrf31461247 = CwSvsZbFrf81337520;     CwSvsZbFrf81337520 = CwSvsZbFrf18196624;     CwSvsZbFrf18196624 = CwSvsZbFrf53630957;     CwSvsZbFrf53630957 = CwSvsZbFrf690806;     CwSvsZbFrf690806 = CwSvsZbFrf27427051;     CwSvsZbFrf27427051 = CwSvsZbFrf60459319;     CwSvsZbFrf60459319 = CwSvsZbFrf87985241;     CwSvsZbFrf87985241 = CwSvsZbFrf58511145;     CwSvsZbFrf58511145 = CwSvsZbFrf12705637;     CwSvsZbFrf12705637 = CwSvsZbFrf13071554;     CwSvsZbFrf13071554 = CwSvsZbFrf354149;     CwSvsZbFrf354149 = CwSvsZbFrf17987823;     CwSvsZbFrf17987823 = CwSvsZbFrf15801652;     CwSvsZbFrf15801652 = CwSvsZbFrf12522711;     CwSvsZbFrf12522711 = CwSvsZbFrf81587328;     CwSvsZbFrf81587328 = CwSvsZbFrf77948348;     CwSvsZbFrf77948348 = CwSvsZbFrf43452673;     CwSvsZbFrf43452673 = CwSvsZbFrf50655151;     CwSvsZbFrf50655151 = CwSvsZbFrf30132380;     CwSvsZbFrf30132380 = CwSvsZbFrf71262699;     CwSvsZbFrf71262699 = CwSvsZbFrf26506656;     CwSvsZbFrf26506656 = CwSvsZbFrf99488542;     CwSvsZbFrf99488542 = CwSvsZbFrf18313940;     CwSvsZbFrf18313940 = CwSvsZbFrf1531716;     CwSvsZbFrf1531716 = CwSvsZbFrf5863571;     CwSvsZbFrf5863571 = CwSvsZbFrf39902637;     CwSvsZbFrf39902637 = CwSvsZbFrf63697796;     CwSvsZbFrf63697796 = CwSvsZbFrf26117275;     CwSvsZbFrf26117275 = CwSvsZbFrf93710546;     CwSvsZbFrf93710546 = CwSvsZbFrf61660729;     CwSvsZbFrf61660729 = CwSvsZbFrf59381912;     CwSvsZbFrf59381912 = CwSvsZbFrf77247673;     CwSvsZbFrf77247673 = CwSvsZbFrf16620497;     CwSvsZbFrf16620497 = CwSvsZbFrf16940115;     CwSvsZbFrf16940115 = CwSvsZbFrf13257464;     CwSvsZbFrf13257464 = CwSvsZbFrf70391240;     CwSvsZbFrf70391240 = CwSvsZbFrf12681549;     CwSvsZbFrf12681549 = CwSvsZbFrf27911845;}
// Junk Finished

// Junk Code By Troll Face & Thaisen's Gen
void TZwlglInMs90767225() {     float FpgLhlrbwx48558658 = -77064734;    float FpgLhlrbwx73783416 = -458283634;    float FpgLhlrbwx88359298 = -670642192;    float FpgLhlrbwx75087169 = -96961873;    float FpgLhlrbwx39541090 = -414768649;    float FpgLhlrbwx76340211 = -229111238;    float FpgLhlrbwx31130339 = -534117797;    float FpgLhlrbwx90677801 = -207885118;    float FpgLhlrbwx73910470 = -771779591;    float FpgLhlrbwx17124602 = 86174657;    float FpgLhlrbwx46375978 = -363520160;    float FpgLhlrbwx65516827 = -23773027;    float FpgLhlrbwx63200700 = -538621961;    float FpgLhlrbwx94181463 = 65877683;    float FpgLhlrbwx11707086 = -657133926;    float FpgLhlrbwx3818360 = -24443806;    float FpgLhlrbwx5904225 = -293344890;    float FpgLhlrbwx27893485 = -407469407;    float FpgLhlrbwx77048324 = -720335879;    float FpgLhlrbwx62399520 = -352675127;    float FpgLhlrbwx79090836 = -668437265;    float FpgLhlrbwx2235660 = -945679628;    float FpgLhlrbwx99777798 = -124292971;    float FpgLhlrbwx60847972 = 11285456;    float FpgLhlrbwx61006930 = -40377444;    float FpgLhlrbwx36592705 = -866632301;    float FpgLhlrbwx68745828 = 40716419;    float FpgLhlrbwx55726641 = -821262944;    float FpgLhlrbwx92114959 = -699739718;    float FpgLhlrbwx55892270 = -199928022;    float FpgLhlrbwx10613388 = -501276701;    float FpgLhlrbwx68309760 = 19754227;    float FpgLhlrbwx5595371 = -150262803;    float FpgLhlrbwx47633679 = -28106182;    float FpgLhlrbwx74284710 = -633930321;    float FpgLhlrbwx49237144 = -752150209;    float FpgLhlrbwx9602675 = -21375790;    float FpgLhlrbwx12686940 = -680252564;    float FpgLhlrbwx7296524 = 6429322;    float FpgLhlrbwx28673159 = -752352685;    float FpgLhlrbwx32534701 = -941583725;    float FpgLhlrbwx66157599 = -164949223;    float FpgLhlrbwx13774465 = -623404759;    float FpgLhlrbwx14520089 = -590644459;    float FpgLhlrbwx75280103 = -228901013;    float FpgLhlrbwx43824663 = -48680174;    float FpgLhlrbwx64813963 = -558373213;    float FpgLhlrbwx49838372 = -981203975;    float FpgLhlrbwx44564317 = -195036554;    float FpgLhlrbwx47418534 = -643689661;    float FpgLhlrbwx63298081 = 21785923;    float FpgLhlrbwx62949889 = -848988276;    float FpgLhlrbwx20385609 = -226345207;    float FpgLhlrbwx50612129 = -459227256;    float FpgLhlrbwx11676389 = -262209998;    float FpgLhlrbwx46322998 = -131385107;    float FpgLhlrbwx74005617 = -233990663;    float FpgLhlrbwx27511327 = -581927648;    float FpgLhlrbwx14080239 = 43415571;    float FpgLhlrbwx2948385 = -548136348;    float FpgLhlrbwx7594383 = -169827657;    float FpgLhlrbwx75403697 = -712854854;    float FpgLhlrbwx98562842 = -508145400;    float FpgLhlrbwx18018200 = -471851570;    float FpgLhlrbwx6511214 = -412548642;    float FpgLhlrbwx78066217 = -283274388;    float FpgLhlrbwx59921457 = -873510225;    float FpgLhlrbwx15567021 = -410515779;    float FpgLhlrbwx19896753 = -300191996;    float FpgLhlrbwx62469941 = -904983717;    float FpgLhlrbwx94215684 = 96931984;    float FpgLhlrbwx93217284 = -613092327;    float FpgLhlrbwx20596962 = -313898729;    float FpgLhlrbwx48375165 = -967983195;    float FpgLhlrbwx29864820 = -411091403;    float FpgLhlrbwx12933238 = -403488042;    float FpgLhlrbwx88461194 = -222274869;    float FpgLhlrbwx85257710 = -533648512;    float FpgLhlrbwx85567868 = -759813532;    float FpgLhlrbwx17182268 = -991697271;    float FpgLhlrbwx71778741 = -208259088;    float FpgLhlrbwx18907457 = 21920394;    float FpgLhlrbwx11162324 = -526226391;    float FpgLhlrbwx44696425 = 43949942;    float FpgLhlrbwx92594188 = -121713946;    float FpgLhlrbwx47663498 = -652288426;    float FpgLhlrbwx47924151 = -753900567;    float FpgLhlrbwx54983241 = -691035547;    float FpgLhlrbwx35957291 = -765896185;    float FpgLhlrbwx27961712 = -402545214;    float FpgLhlrbwx75231527 = -418159546;    float FpgLhlrbwx82091348 = -439448143;    float FpgLhlrbwx98606700 = -623668135;    float FpgLhlrbwx4348139 = -445434330;    float FpgLhlrbwx21078776 = -482525028;    float FpgLhlrbwx57131003 = -128728871;    float FpgLhlrbwx67594756 = -656803824;    float FpgLhlrbwx95756265 = -51553189;    float FpgLhlrbwx8008875 = -78095818;    float FpgLhlrbwx97213885 = -77064734;     FpgLhlrbwx48558658 = FpgLhlrbwx73783416;     FpgLhlrbwx73783416 = FpgLhlrbwx88359298;     FpgLhlrbwx88359298 = FpgLhlrbwx75087169;     FpgLhlrbwx75087169 = FpgLhlrbwx39541090;     FpgLhlrbwx39541090 = FpgLhlrbwx76340211;     FpgLhlrbwx76340211 = FpgLhlrbwx31130339;     FpgLhlrbwx31130339 = FpgLhlrbwx90677801;     FpgLhlrbwx90677801 = FpgLhlrbwx73910470;     FpgLhlrbwx73910470 = FpgLhlrbwx17124602;     FpgLhlrbwx17124602 = FpgLhlrbwx46375978;     FpgLhlrbwx46375978 = FpgLhlrbwx65516827;     FpgLhlrbwx65516827 = FpgLhlrbwx63200700;     FpgLhlrbwx63200700 = FpgLhlrbwx94181463;     FpgLhlrbwx94181463 = FpgLhlrbwx11707086;     FpgLhlrbwx11707086 = FpgLhlrbwx3818360;     FpgLhlrbwx3818360 = FpgLhlrbwx5904225;     FpgLhlrbwx5904225 = FpgLhlrbwx27893485;     FpgLhlrbwx27893485 = FpgLhlrbwx77048324;     FpgLhlrbwx77048324 = FpgLhlrbwx62399520;     FpgLhlrbwx62399520 = FpgLhlrbwx79090836;     FpgLhlrbwx79090836 = FpgLhlrbwx2235660;     FpgLhlrbwx2235660 = FpgLhlrbwx99777798;     FpgLhlrbwx99777798 = FpgLhlrbwx60847972;     FpgLhlrbwx60847972 = FpgLhlrbwx61006930;     FpgLhlrbwx61006930 = FpgLhlrbwx36592705;     FpgLhlrbwx36592705 = FpgLhlrbwx68745828;     FpgLhlrbwx68745828 = FpgLhlrbwx55726641;     FpgLhlrbwx55726641 = FpgLhlrbwx92114959;     FpgLhlrbwx92114959 = FpgLhlrbwx55892270;     FpgLhlrbwx55892270 = FpgLhlrbwx10613388;     FpgLhlrbwx10613388 = FpgLhlrbwx68309760;     FpgLhlrbwx68309760 = FpgLhlrbwx5595371;     FpgLhlrbwx5595371 = FpgLhlrbwx47633679;     FpgLhlrbwx47633679 = FpgLhlrbwx74284710;     FpgLhlrbwx74284710 = FpgLhlrbwx49237144;     FpgLhlrbwx49237144 = FpgLhlrbwx9602675;     FpgLhlrbwx9602675 = FpgLhlrbwx12686940;     FpgLhlrbwx12686940 = FpgLhlrbwx7296524;     FpgLhlrbwx7296524 = FpgLhlrbwx28673159;     FpgLhlrbwx28673159 = FpgLhlrbwx32534701;     FpgLhlrbwx32534701 = FpgLhlrbwx66157599;     FpgLhlrbwx66157599 = FpgLhlrbwx13774465;     FpgLhlrbwx13774465 = FpgLhlrbwx14520089;     FpgLhlrbwx14520089 = FpgLhlrbwx75280103;     FpgLhlrbwx75280103 = FpgLhlrbwx43824663;     FpgLhlrbwx43824663 = FpgLhlrbwx64813963;     FpgLhlrbwx64813963 = FpgLhlrbwx49838372;     FpgLhlrbwx49838372 = FpgLhlrbwx44564317;     FpgLhlrbwx44564317 = FpgLhlrbwx47418534;     FpgLhlrbwx47418534 = FpgLhlrbwx63298081;     FpgLhlrbwx63298081 = FpgLhlrbwx62949889;     FpgLhlrbwx62949889 = FpgLhlrbwx20385609;     FpgLhlrbwx20385609 = FpgLhlrbwx50612129;     FpgLhlrbwx50612129 = FpgLhlrbwx11676389;     FpgLhlrbwx11676389 = FpgLhlrbwx46322998;     FpgLhlrbwx46322998 = FpgLhlrbwx74005617;     FpgLhlrbwx74005617 = FpgLhlrbwx27511327;     FpgLhlrbwx27511327 = FpgLhlrbwx14080239;     FpgLhlrbwx14080239 = FpgLhlrbwx2948385;     FpgLhlrbwx2948385 = FpgLhlrbwx7594383;     FpgLhlrbwx7594383 = FpgLhlrbwx75403697;     FpgLhlrbwx75403697 = FpgLhlrbwx98562842;     FpgLhlrbwx98562842 = FpgLhlrbwx18018200;     FpgLhlrbwx18018200 = FpgLhlrbwx6511214;     FpgLhlrbwx6511214 = FpgLhlrbwx78066217;     FpgLhlrbwx78066217 = FpgLhlrbwx59921457;     FpgLhlrbwx59921457 = FpgLhlrbwx15567021;     FpgLhlrbwx15567021 = FpgLhlrbwx19896753;     FpgLhlrbwx19896753 = FpgLhlrbwx62469941;     FpgLhlrbwx62469941 = FpgLhlrbwx94215684;     FpgLhlrbwx94215684 = FpgLhlrbwx93217284;     FpgLhlrbwx93217284 = FpgLhlrbwx20596962;     FpgLhlrbwx20596962 = FpgLhlrbwx48375165;     FpgLhlrbwx48375165 = FpgLhlrbwx29864820;     FpgLhlrbwx29864820 = FpgLhlrbwx12933238;     FpgLhlrbwx12933238 = FpgLhlrbwx88461194;     FpgLhlrbwx88461194 = FpgLhlrbwx85257710;     FpgLhlrbwx85257710 = FpgLhlrbwx85567868;     FpgLhlrbwx85567868 = FpgLhlrbwx17182268;     FpgLhlrbwx17182268 = FpgLhlrbwx71778741;     FpgLhlrbwx71778741 = FpgLhlrbwx18907457;     FpgLhlrbwx18907457 = FpgLhlrbwx11162324;     FpgLhlrbwx11162324 = FpgLhlrbwx44696425;     FpgLhlrbwx44696425 = FpgLhlrbwx92594188;     FpgLhlrbwx92594188 = FpgLhlrbwx47663498;     FpgLhlrbwx47663498 = FpgLhlrbwx47924151;     FpgLhlrbwx47924151 = FpgLhlrbwx54983241;     FpgLhlrbwx54983241 = FpgLhlrbwx35957291;     FpgLhlrbwx35957291 = FpgLhlrbwx27961712;     FpgLhlrbwx27961712 = FpgLhlrbwx75231527;     FpgLhlrbwx75231527 = FpgLhlrbwx82091348;     FpgLhlrbwx82091348 = FpgLhlrbwx98606700;     FpgLhlrbwx98606700 = FpgLhlrbwx4348139;     FpgLhlrbwx4348139 = FpgLhlrbwx21078776;     FpgLhlrbwx21078776 = FpgLhlrbwx57131003;     FpgLhlrbwx57131003 = FpgLhlrbwx67594756;     FpgLhlrbwx67594756 = FpgLhlrbwx95756265;     FpgLhlrbwx95756265 = FpgLhlrbwx8008875;     FpgLhlrbwx8008875 = FpgLhlrbwx97213885;     FpgLhlrbwx97213885 = FpgLhlrbwx48558658;}
// Junk Finished

// Junk Code By Troll Face & Thaisen's Gen
void IWspVUXfum88045054() {     float YYhmKRmrne6411216 = -305221456;    float YYhmKRmrne32830678 = -244783170;    float YYhmKRmrne53201842 = -618834497;    float YYhmKRmrne77237352 = -103979646;    float YYhmKRmrne85160510 = -643069174;    float YYhmKRmrne92640768 = -652634699;    float YYhmKRmrne65396147 = -496294370;    float YYhmKRmrne59229171 = -618538586;    float YYhmKRmrne89851042 = -330660967;    float YYhmKRmrne27151385 = -784446803;    float YYhmKRmrne1778967 = -202680219;    float YYhmKRmrne7696248 = -403160729;    float YYhmKRmrne36590612 = -587693311;    float YYhmKRmrne36092859 = -93041431;    float YYhmKRmrne61464770 = -825323802;    float YYhmKRmrne57051025 = -809127305;    float YYhmKRmrne55710752 = -177984693;    float YYhmKRmrne10629560 = -535568544;    float YYhmKRmrne98452015 = -554019159;    float YYhmKRmrne62679259 = -419846196;    float YYhmKRmrne13391383 = -606732888;    float YYhmKRmrne91176137 = 72909648;    float YYhmKRmrne36718105 = -579047905;    float YYhmKRmrne37490596 = -82611483;    float YYhmKRmrne76122439 = -359227914;    float YYhmKRmrne19019794 = 9102597;    float YYhmKRmrne6034242 = -322354502;    float YYhmKRmrne66962991 = -170522583;    float YYhmKRmrne32997288 = -972021325;    float YYhmKRmrne94492990 = -479299685;    float YYhmKRmrne40074746 = -747480106;    float YYhmKRmrne78950112 = -13466780;    float YYhmKRmrne13399629 = -625258059;    float YYhmKRmrne36487956 = -540245113;    float YYhmKRmrne33964490 = -389473069;    float YYhmKRmrne99662667 = -102345145;    float YYhmKRmrne40512836 = -619366404;    float YYhmKRmrne47610765 = -535270938;    float YYhmKRmrne50681619 = -432811570;    float YYhmKRmrne41257513 = -744407574;    float YYhmKRmrne6888012 = -26016406;    float YYhmKRmrne58333456 = -423611123;    float YYhmKRmrne48424675 = -604985874;    float YYhmKRmrne94863647 = -535376075;    float YYhmKRmrne59261776 = -491296250;    float YYhmKRmrne57458305 = -457788231;    float YYhmKRmrne68758732 = -39489204;    float YYhmKRmrne36779778 = -295504203;    float YYhmKRmrne43258498 = -244726271;    float YYhmKRmrne48986446 = -589711849;    float YYhmKRmrne39678652 = -629011904;    float YYhmKRmrne5739362 = -11314156;    float YYhmKRmrne91872812 = -862247982;    float YYhmKRmrne16640251 = -394496713;    float YYhmKRmrne74653672 = -992504981;    float YYhmKRmrne15235079 = -278131104;    float YYhmKRmrne96112573 = -665735265;    float YYhmKRmrne15711247 = -436223015;    float YYhmKRmrne1114914 = -744751732;    float YYhmKRmrne66140717 = -552171771;    float YYhmKRmrne86606527 = -230280198;    float YYhmKRmrne98433156 = -225771787;    float YYhmKRmrne26231883 = -646517261;    float YYhmKRmrne95358051 = -851361283;    float YYhmKRmrne87076639 = 63033302;    float YYhmKRmrne22828855 = -89213439;    float YYhmKRmrne94296618 = -777902671;    float YYhmKRmrne102657 = 52551802;    float YYhmKRmrne2128369 = -703568363;    float YYhmKRmrne61802102 = -622978657;    float YYhmKRmrne16538190 = -89760901;    float YYhmKRmrne8099988 = -642713755;    float YYhmKRmrne59947940 = -2756975;    float YYhmKRmrne57194502 = -809611585;    float YYhmKRmrne55791247 = -293829790;    float YYhmKRmrne55057927 = -83121766;    float YYhmKRmrne42751462 = -322104478;    float YYhmKRmrne41854458 = 56328169;    float YYhmKRmrne78228820 = -591315233;    float YYhmKRmrne18664135 = -901439684;    float YYhmKRmrne50261062 = -951408199;    float YYhmKRmrne69254463 = 73149701;    float YYhmKRmrne23704493 = -925796313;    float YYhmKRmrne84010841 = -282309477;    float YYhmKRmrne54814339 = -850287782;    float YYhmKRmrne34335384 = -636165950;    float YYhmKRmrne87077299 = -151218798;    float YYhmKRmrne96759378 = -130761346;    float YYhmKRmrne61834284 = -547740132;    float YYhmKRmrne18729412 = -11341965;    float YYhmKRmrne3550095 = -436609880;    float YYhmKRmrne24801590 = -83143390;    float YYhmKRmrne46495852 = -790519206;    float YYhmKRmrne84540902 = -880639799;    float YYhmKRmrne54650986 = -414127377;    float YYhmKRmrne8454855 = -800244619;    float YYhmKRmrne32101573 = -777093863;    float YYhmKRmrne53066624 = -753624592;    float YYhmKRmrne7787009 = -498409378;    float YYhmKRmrne36432921 = -305221456;     YYhmKRmrne6411216 = YYhmKRmrne32830678;     YYhmKRmrne32830678 = YYhmKRmrne53201842;     YYhmKRmrne53201842 = YYhmKRmrne77237352;     YYhmKRmrne77237352 = YYhmKRmrne85160510;     YYhmKRmrne85160510 = YYhmKRmrne92640768;     YYhmKRmrne92640768 = YYhmKRmrne65396147;     YYhmKRmrne65396147 = YYhmKRmrne59229171;     YYhmKRmrne59229171 = YYhmKRmrne89851042;     YYhmKRmrne89851042 = YYhmKRmrne27151385;     YYhmKRmrne27151385 = YYhmKRmrne1778967;     YYhmKRmrne1778967 = YYhmKRmrne7696248;     YYhmKRmrne7696248 = YYhmKRmrne36590612;     YYhmKRmrne36590612 = YYhmKRmrne36092859;     YYhmKRmrne36092859 = YYhmKRmrne61464770;     YYhmKRmrne61464770 = YYhmKRmrne57051025;     YYhmKRmrne57051025 = YYhmKRmrne55710752;     YYhmKRmrne55710752 = YYhmKRmrne10629560;     YYhmKRmrne10629560 = YYhmKRmrne98452015;     YYhmKRmrne98452015 = YYhmKRmrne62679259;     YYhmKRmrne62679259 = YYhmKRmrne13391383;     YYhmKRmrne13391383 = YYhmKRmrne91176137;     YYhmKRmrne91176137 = YYhmKRmrne36718105;     YYhmKRmrne36718105 = YYhmKRmrne37490596;     YYhmKRmrne37490596 = YYhmKRmrne76122439;     YYhmKRmrne76122439 = YYhmKRmrne19019794;     YYhmKRmrne19019794 = YYhmKRmrne6034242;     YYhmKRmrne6034242 = YYhmKRmrne66962991;     YYhmKRmrne66962991 = YYhmKRmrne32997288;     YYhmKRmrne32997288 = YYhmKRmrne94492990;     YYhmKRmrne94492990 = YYhmKRmrne40074746;     YYhmKRmrne40074746 = YYhmKRmrne78950112;     YYhmKRmrne78950112 = YYhmKRmrne13399629;     YYhmKRmrne13399629 = YYhmKRmrne36487956;     YYhmKRmrne36487956 = YYhmKRmrne33964490;     YYhmKRmrne33964490 = YYhmKRmrne99662667;     YYhmKRmrne99662667 = YYhmKRmrne40512836;     YYhmKRmrne40512836 = YYhmKRmrne47610765;     YYhmKRmrne47610765 = YYhmKRmrne50681619;     YYhmKRmrne50681619 = YYhmKRmrne41257513;     YYhmKRmrne41257513 = YYhmKRmrne6888012;     YYhmKRmrne6888012 = YYhmKRmrne58333456;     YYhmKRmrne58333456 = YYhmKRmrne48424675;     YYhmKRmrne48424675 = YYhmKRmrne94863647;     YYhmKRmrne94863647 = YYhmKRmrne59261776;     YYhmKRmrne59261776 = YYhmKRmrne57458305;     YYhmKRmrne57458305 = YYhmKRmrne68758732;     YYhmKRmrne68758732 = YYhmKRmrne36779778;     YYhmKRmrne36779778 = YYhmKRmrne43258498;     YYhmKRmrne43258498 = YYhmKRmrne48986446;     YYhmKRmrne48986446 = YYhmKRmrne39678652;     YYhmKRmrne39678652 = YYhmKRmrne5739362;     YYhmKRmrne5739362 = YYhmKRmrne91872812;     YYhmKRmrne91872812 = YYhmKRmrne16640251;     YYhmKRmrne16640251 = YYhmKRmrne74653672;     YYhmKRmrne74653672 = YYhmKRmrne15235079;     YYhmKRmrne15235079 = YYhmKRmrne96112573;     YYhmKRmrne96112573 = YYhmKRmrne15711247;     YYhmKRmrne15711247 = YYhmKRmrne1114914;     YYhmKRmrne1114914 = YYhmKRmrne66140717;     YYhmKRmrne66140717 = YYhmKRmrne86606527;     YYhmKRmrne86606527 = YYhmKRmrne98433156;     YYhmKRmrne98433156 = YYhmKRmrne26231883;     YYhmKRmrne26231883 = YYhmKRmrne95358051;     YYhmKRmrne95358051 = YYhmKRmrne87076639;     YYhmKRmrne87076639 = YYhmKRmrne22828855;     YYhmKRmrne22828855 = YYhmKRmrne94296618;     YYhmKRmrne94296618 = YYhmKRmrne102657;     YYhmKRmrne102657 = YYhmKRmrne2128369;     YYhmKRmrne2128369 = YYhmKRmrne61802102;     YYhmKRmrne61802102 = YYhmKRmrne16538190;     YYhmKRmrne16538190 = YYhmKRmrne8099988;     YYhmKRmrne8099988 = YYhmKRmrne59947940;     YYhmKRmrne59947940 = YYhmKRmrne57194502;     YYhmKRmrne57194502 = YYhmKRmrne55791247;     YYhmKRmrne55791247 = YYhmKRmrne55057927;     YYhmKRmrne55057927 = YYhmKRmrne42751462;     YYhmKRmrne42751462 = YYhmKRmrne41854458;     YYhmKRmrne41854458 = YYhmKRmrne78228820;     YYhmKRmrne78228820 = YYhmKRmrne18664135;     YYhmKRmrne18664135 = YYhmKRmrne50261062;     YYhmKRmrne50261062 = YYhmKRmrne69254463;     YYhmKRmrne69254463 = YYhmKRmrne23704493;     YYhmKRmrne23704493 = YYhmKRmrne84010841;     YYhmKRmrne84010841 = YYhmKRmrne54814339;     YYhmKRmrne54814339 = YYhmKRmrne34335384;     YYhmKRmrne34335384 = YYhmKRmrne87077299;     YYhmKRmrne87077299 = YYhmKRmrne96759378;     YYhmKRmrne96759378 = YYhmKRmrne61834284;     YYhmKRmrne61834284 = YYhmKRmrne18729412;     YYhmKRmrne18729412 = YYhmKRmrne3550095;     YYhmKRmrne3550095 = YYhmKRmrne24801590;     YYhmKRmrne24801590 = YYhmKRmrne46495852;     YYhmKRmrne46495852 = YYhmKRmrne84540902;     YYhmKRmrne84540902 = YYhmKRmrne54650986;     YYhmKRmrne54650986 = YYhmKRmrne8454855;     YYhmKRmrne8454855 = YYhmKRmrne32101573;     YYhmKRmrne32101573 = YYhmKRmrne53066624;     YYhmKRmrne53066624 = YYhmKRmrne7787009;     YYhmKRmrne7787009 = YYhmKRmrne36432921;     YYhmKRmrne36432921 = YYhmKRmrne6411216;}
// Junk Finished

// Junk Code By Troll Face & Thaisen's Gen
void BcECjKJdGO46285936() {     float OkDxSSacDR45068343 = -562127493;    float OkDxSSacDR58152399 = -432269499;    float OkDxSSacDR94698215 = -947309264;    float OkDxSSacDR80175948 = 73953921;    float OkDxSSacDR12613531 = -204533305;    float OkDxSSacDR30989800 = -109355220;    float OkDxSSacDR89218659 = -1304005;    float OkDxSSacDR60736997 = -172857961;    float OkDxSSacDR19534809 = -331020380;    float OkDxSSacDR69448894 = 59046008;    float OkDxSSacDR16752396 = -318491434;    float OkDxSSacDR87679102 = -460245114;    float OkDxSSacDR84397854 = -285236861;    float OkDxSSacDR38178686 = -616220377;    float OkDxSSacDR39192898 = -489817062;    float OkDxSSacDR4057457 = -499462846;    float OkDxSSacDR61521453 = -115999220;    float OkDxSSacDR25529240 = -673531283;    float OkDxSSacDR19185435 = -418454313;    float OkDxSSacDR39161661 = -568212183;    float OkDxSSacDR26604404 = -734220050;    float OkDxSSacDR32285550 = 71306738;    float OkDxSSacDR7713960 = -84691603;    float OkDxSSacDR6497797 = -988292210;    float OkDxSSacDR3503283 = -329150130;    float OkDxSSacDR70904108 = 99969472;    float OkDxSSacDR1753304 = 63134716;    float OkDxSSacDR65678766 = -781502537;    float OkDxSSacDR80650154 = -338152859;    float OkDxSSacDR44191713 = -502420551;    float OkDxSSacDR36413807 = -996791472;    float OkDxSSacDR96421871 = -508162674;    float OkDxSSacDR65766021 = -691331679;    float OkDxSSacDR60434766 = -336957048;    float OkDxSSacDR24948568 = -978252809;    float OkDxSSacDR14322493 = -519109790;    float OkDxSSacDR46130326 = -131626437;    float OkDxSSacDR20772754 = -499015873;    float OkDxSSacDR73687628 = -140980137;    float OkDxSSacDR94127004 = -543416410;    float OkDxSSacDR58733529 = -967097844;    float OkDxSSacDR3199782 = -769286227;    float OkDxSSacDR81458081 = -299242458;    float OkDxSSacDR6344146 = -510760435;    float OkDxSSacDR83275844 = -447685051;    float OkDxSSacDR61719673 = -837030305;    float OkDxSSacDR75574952 = 22465827;    float OkDxSSacDR18020186 = -898678653;    float OkDxSSacDR22502580 = -58422679;    float OkDxSSacDR30242285 = -786741559;    float OkDxSSacDR21716406 = -396870787;    float OkDxSSacDR42632156 = 27773600;    float OkDxSSacDR253162 = -414280855;    float OkDxSSacDR41244461 = -625388186;    float OkDxSSacDR19681533 = -573381429;    float OkDxSSacDR12782794 = -533434232;    float OkDxSSacDR50438440 = -247577897;    float OkDxSSacDR88200419 = -959017055;    float OkDxSSacDR76672665 = -596895949;    float OkDxSSacDR41709423 = -204502778;    float OkDxSSacDR29236496 = -72489936;    float OkDxSSacDR23539894 = -219801469;    float OkDxSSacDR80086843 = -834705102;    float OkDxSSacDR75343096 = -828599830;    float OkDxSSacDR33035088 = 55837480;    float OkDxSSacDR20330524 = -810328760;    float OkDxSSacDR21913082 = -768913436;    float OkDxSSacDR23963089 = -948279814;    float OkDxSSacDR13230119 = -637967569;    float OkDxSSacDR24870405 = -970707272;    float OkDxSSacDR57927131 = -267836410;    float OkDxSSacDR40748699 = -616983348;    float OkDxSSacDR51841611 = -432551147;    float OkDxSSacDR25058431 = -875037903;    float OkDxSSacDR80428131 = -601114340;    float OkDxSSacDR23404623 = -964933823;    float OkDxSSacDR50827469 = -629450804;    float OkDxSSacDR1369814 = -573931168;    float OkDxSSacDR23221952 = -440607159;    float OkDxSSacDR41783609 = -492119826;    float OkDxSSacDR95329155 = -922496356;    float OkDxSSacDR83733118 = -38186632;    float OkDxSSacDR43176186 = -623079858;    float OkDxSSacDR50407869 = -551411300;    float OkDxSSacDR22475307 = -5549765;    float OkDxSSacDR93781650 = -924565072;    float OkDxSSacDR96168710 = 6118181;    float OkDxSSacDR24521561 = 34056507;    float OkDxSSacDR40753234 = -763575619;    float OkDxSSacDR12165775 = -344818578;    float OkDxSSacDR63884053 = -171531894;    float OkDxSSacDR57929906 = -172609383;    float OkDxSSacDR44100088 = -902119924;    float OkDxSSacDR31978206 = -936477359;    float OkDxSSacDR64890509 = -370926475;    float OkDxSSacDR35193636 = -647296376;    float OkDxSSacDR23112938 = -934581125;    float OkDxSSacDR6114985 = -470642629;    float OkDxSSacDR73309057 = -466597915;    float OkDxSSacDR62945320 = -562127493;     OkDxSSacDR45068343 = OkDxSSacDR58152399;     OkDxSSacDR58152399 = OkDxSSacDR94698215;     OkDxSSacDR94698215 = OkDxSSacDR80175948;     OkDxSSacDR80175948 = OkDxSSacDR12613531;     OkDxSSacDR12613531 = OkDxSSacDR30989800;     OkDxSSacDR30989800 = OkDxSSacDR89218659;     OkDxSSacDR89218659 = OkDxSSacDR60736997;     OkDxSSacDR60736997 = OkDxSSacDR19534809;     OkDxSSacDR19534809 = OkDxSSacDR69448894;     OkDxSSacDR69448894 = OkDxSSacDR16752396;     OkDxSSacDR16752396 = OkDxSSacDR87679102;     OkDxSSacDR87679102 = OkDxSSacDR84397854;     OkDxSSacDR84397854 = OkDxSSacDR38178686;     OkDxSSacDR38178686 = OkDxSSacDR39192898;     OkDxSSacDR39192898 = OkDxSSacDR4057457;     OkDxSSacDR4057457 = OkDxSSacDR61521453;     OkDxSSacDR61521453 = OkDxSSacDR25529240;     OkDxSSacDR25529240 = OkDxSSacDR19185435;     OkDxSSacDR19185435 = OkDxSSacDR39161661;     OkDxSSacDR39161661 = OkDxSSacDR26604404;     OkDxSSacDR26604404 = OkDxSSacDR32285550;     OkDxSSacDR32285550 = OkDxSSacDR7713960;     OkDxSSacDR7713960 = OkDxSSacDR6497797;     OkDxSSacDR6497797 = OkDxSSacDR3503283;     OkDxSSacDR3503283 = OkDxSSacDR70904108;     OkDxSSacDR70904108 = OkDxSSacDR1753304;     OkDxSSacDR1753304 = OkDxSSacDR65678766;     OkDxSSacDR65678766 = OkDxSSacDR80650154;     OkDxSSacDR80650154 = OkDxSSacDR44191713;     OkDxSSacDR44191713 = OkDxSSacDR36413807;     OkDxSSacDR36413807 = OkDxSSacDR96421871;     OkDxSSacDR96421871 = OkDxSSacDR65766021;     OkDxSSacDR65766021 = OkDxSSacDR60434766;     OkDxSSacDR60434766 = OkDxSSacDR24948568;     OkDxSSacDR24948568 = OkDxSSacDR14322493;     OkDxSSacDR14322493 = OkDxSSacDR46130326;     OkDxSSacDR46130326 = OkDxSSacDR20772754;     OkDxSSacDR20772754 = OkDxSSacDR73687628;     OkDxSSacDR73687628 = OkDxSSacDR94127004;     OkDxSSacDR94127004 = OkDxSSacDR58733529;     OkDxSSacDR58733529 = OkDxSSacDR3199782;     OkDxSSacDR3199782 = OkDxSSacDR81458081;     OkDxSSacDR81458081 = OkDxSSacDR6344146;     OkDxSSacDR6344146 = OkDxSSacDR83275844;     OkDxSSacDR83275844 = OkDxSSacDR61719673;     OkDxSSacDR61719673 = OkDxSSacDR75574952;     OkDxSSacDR75574952 = OkDxSSacDR18020186;     OkDxSSacDR18020186 = OkDxSSacDR22502580;     OkDxSSacDR22502580 = OkDxSSacDR30242285;     OkDxSSacDR30242285 = OkDxSSacDR21716406;     OkDxSSacDR21716406 = OkDxSSacDR42632156;     OkDxSSacDR42632156 = OkDxSSacDR253162;     OkDxSSacDR253162 = OkDxSSacDR41244461;     OkDxSSacDR41244461 = OkDxSSacDR19681533;     OkDxSSacDR19681533 = OkDxSSacDR12782794;     OkDxSSacDR12782794 = OkDxSSacDR50438440;     OkDxSSacDR50438440 = OkDxSSacDR88200419;     OkDxSSacDR88200419 = OkDxSSacDR76672665;     OkDxSSacDR76672665 = OkDxSSacDR41709423;     OkDxSSacDR41709423 = OkDxSSacDR29236496;     OkDxSSacDR29236496 = OkDxSSacDR23539894;     OkDxSSacDR23539894 = OkDxSSacDR80086843;     OkDxSSacDR80086843 = OkDxSSacDR75343096;     OkDxSSacDR75343096 = OkDxSSacDR33035088;     OkDxSSacDR33035088 = OkDxSSacDR20330524;     OkDxSSacDR20330524 = OkDxSSacDR21913082;     OkDxSSacDR21913082 = OkDxSSacDR23963089;     OkDxSSacDR23963089 = OkDxSSacDR13230119;     OkDxSSacDR13230119 = OkDxSSacDR24870405;     OkDxSSacDR24870405 = OkDxSSacDR57927131;     OkDxSSacDR57927131 = OkDxSSacDR40748699;     OkDxSSacDR40748699 = OkDxSSacDR51841611;     OkDxSSacDR51841611 = OkDxSSacDR25058431;     OkDxSSacDR25058431 = OkDxSSacDR80428131;     OkDxSSacDR80428131 = OkDxSSacDR23404623;     OkDxSSacDR23404623 = OkDxSSacDR50827469;     OkDxSSacDR50827469 = OkDxSSacDR1369814;     OkDxSSacDR1369814 = OkDxSSacDR23221952;     OkDxSSacDR23221952 = OkDxSSacDR41783609;     OkDxSSacDR41783609 = OkDxSSacDR95329155;     OkDxSSacDR95329155 = OkDxSSacDR83733118;     OkDxSSacDR83733118 = OkDxSSacDR43176186;     OkDxSSacDR43176186 = OkDxSSacDR50407869;     OkDxSSacDR50407869 = OkDxSSacDR22475307;     OkDxSSacDR22475307 = OkDxSSacDR93781650;     OkDxSSacDR93781650 = OkDxSSacDR96168710;     OkDxSSacDR96168710 = OkDxSSacDR24521561;     OkDxSSacDR24521561 = OkDxSSacDR40753234;     OkDxSSacDR40753234 = OkDxSSacDR12165775;     OkDxSSacDR12165775 = OkDxSSacDR63884053;     OkDxSSacDR63884053 = OkDxSSacDR57929906;     OkDxSSacDR57929906 = OkDxSSacDR44100088;     OkDxSSacDR44100088 = OkDxSSacDR31978206;     OkDxSSacDR31978206 = OkDxSSacDR64890509;     OkDxSSacDR64890509 = OkDxSSacDR35193636;     OkDxSSacDR35193636 = OkDxSSacDR23112938;     OkDxSSacDR23112938 = OkDxSSacDR6114985;     OkDxSSacDR6114985 = OkDxSSacDR73309057;     OkDxSSacDR73309057 = OkDxSSacDR62945320;     OkDxSSacDR62945320 = OkDxSSacDR45068343;}
// Junk Finished

// Junk Code By Troll Face & Thaisen's Gen
void DxgTuqVhKP43563765() {     float AXvSEXacfP2920901 = -790284214;    float AXvSEXacfP17199662 = -218769035;    float AXvSEXacfP59540759 = -895501570;    float AXvSEXacfP82326131 = 66936149;    float AXvSEXacfP58232952 = -432833830;    float AXvSEXacfP47290356 = -532878682;    float AXvSEXacfP23484469 = 36519421;    float AXvSEXacfP29288367 = -583511428;    float AXvSEXacfP35475381 = -989901756;    float AXvSEXacfP79475677 = -811575451;    float AXvSEXacfP72155385 = -157651492;    float AXvSEXacfP29858523 = -839632816;    float AXvSEXacfP57787767 = -334308212;    float AXvSEXacfP80090081 = -775139492;    float AXvSEXacfP88950582 = -658006938;    float AXvSEXacfP57290122 = -184146345;    float AXvSEXacfP11327981 = -639023;    float AXvSEXacfP8265315 = -801630420;    float AXvSEXacfP40589126 = -252137593;    float AXvSEXacfP39441400 = -635383252;    float AXvSEXacfP60904949 = -672515673;    float AXvSEXacfP21226028 = -10103986;    float AXvSEXacfP44654266 = -539446537;    float AXvSEXacfP83140420 = 17810852;    float AXvSEXacfP18618791 = -648000600;    float AXvSEXacfP53331197 = -124295630;    float AXvSEXacfP39041717 = -299936205;    float AXvSEXacfP76915116 = -130762176;    float AXvSEXacfP21532483 = -610434466;    float AXvSEXacfP82792433 = -781792214;    float AXvSEXacfP65875164 = -142994877;    float AXvSEXacfP7062224 = -541383681;    float AXvSEXacfP73570279 = -66326935;    float AXvSEXacfP49289043 = -849095979;    float AXvSEXacfP84628347 = -733795557;    float AXvSEXacfP64748016 = -969304726;    float AXvSEXacfP77040486 = -729617051;    float AXvSEXacfP55696579 = -354034247;    float AXvSEXacfP17072725 = -580221028;    float AXvSEXacfP6711359 = -535471300;    float AXvSEXacfP33086841 = -51530526;    float AXvSEXacfP95375638 = 72051873;    float AXvSEXacfP16108292 = -280823573;    float AXvSEXacfP86687704 = -455492050;    float AXvSEXacfP67257517 = -710080289;    float AXvSEXacfP75353315 = -146138362;    float AXvSEXacfP79519720 = -558650164;    float AXvSEXacfP4961592 = -212978881;    float AXvSEXacfP21196761 = -108112396;    float AXvSEXacfP31810197 = -732763747;    float AXvSEXacfP98096976 = 52331386;    float AXvSEXacfP85421628 = -234552281;    float AXvSEXacfP71740365 = 49816369;    float AXvSEXacfP7272582 = -560657643;    float AXvSEXacfP82658815 = -203676413;    float AXvSEXacfP81694873 = -680180229;    float AXvSEXacfP72545396 = -679322499;    float AXvSEXacfP76400338 = -813312422;    float AXvSEXacfP63707340 = -285063252;    float AXvSEXacfP4901755 = -208538201;    float AXvSEXacfP8248640 = -132942477;    float AXvSEXacfP46569353 = -832718403;    float AXvSEXacfP7755884 = -973076963;    float AXvSEXacfP52682948 = -108109542;    float AXvSEXacfP13600514 = -568580575;    float AXvSEXacfP65093161 = -616267811;    float AXvSEXacfP56288244 = -673305882;    float AXvSEXacfP8498724 = -485212233;    float AXvSEXacfP95461733 = 58656065;    float AXvSEXacfP24202566 = -688702212;    float AXvSEXacfP80249636 = -454529295;    float AXvSEXacfP55631402 = -646604777;    float AXvSEXacfP91192590 = -121409392;    float AXvSEXacfP33877768 = -716666294;    float AXvSEXacfP6354560 = -483852726;    float AXvSEXacfP65529311 = -644567547;    float AXvSEXacfP5117736 = -729280413;    float AXvSEXacfP57966561 = 16045513;    float AXvSEXacfP15882904 = -272108860;    float AXvSEXacfP43265475 = -401862238;    float AXvSEXacfP73811476 = -565645467;    float AXvSEXacfP34080126 = 13042675;    float AXvSEXacfP55718355 = 77350220;    float AXvSEXacfP89722285 = -877670719;    float AXvSEXacfP84695456 = -734123601;    float AXvSEXacfP80453536 = -908442596;    float AXvSEXacfP35321859 = -491200051;    float AXvSEXacfP66297697 = -505669292;    float AXvSEXacfP66630227 = -545419567;    float AXvSEXacfP2933475 = 46384671;    float AXvSEXacfP92202620 = -189982228;    float AXvSEXacfP640148 = -916304629;    float AXvSEXacfP91989239 = 31029005;    float AXvSEXacfP12170970 = -271682828;    float AXvSEXacfP98462718 = -302528823;    float AXvSEXacfP86517487 = -218812124;    float AXvSEXacfP87619754 = 45128836;    float AXvSEXacfP63425343 = -72714032;    float AXvSEXacfP73087191 = -886911475;    float AXvSEXacfP2164356 = -790284214;     AXvSEXacfP2920901 = AXvSEXacfP17199662;     AXvSEXacfP17199662 = AXvSEXacfP59540759;     AXvSEXacfP59540759 = AXvSEXacfP82326131;     AXvSEXacfP82326131 = AXvSEXacfP58232952;     AXvSEXacfP58232952 = AXvSEXacfP47290356;     AXvSEXacfP47290356 = AXvSEXacfP23484469;     AXvSEXacfP23484469 = AXvSEXacfP29288367;     AXvSEXacfP29288367 = AXvSEXacfP35475381;     AXvSEXacfP35475381 = AXvSEXacfP79475677;     AXvSEXacfP79475677 = AXvSEXacfP72155385;     AXvSEXacfP72155385 = AXvSEXacfP29858523;     AXvSEXacfP29858523 = AXvSEXacfP57787767;     AXvSEXacfP57787767 = AXvSEXacfP80090081;     AXvSEXacfP80090081 = AXvSEXacfP88950582;     AXvSEXacfP88950582 = AXvSEXacfP57290122;     AXvSEXacfP57290122 = AXvSEXacfP11327981;     AXvSEXacfP11327981 = AXvSEXacfP8265315;     AXvSEXacfP8265315 = AXvSEXacfP40589126;     AXvSEXacfP40589126 = AXvSEXacfP39441400;     AXvSEXacfP39441400 = AXvSEXacfP60904949;     AXvSEXacfP60904949 = AXvSEXacfP21226028;     AXvSEXacfP21226028 = AXvSEXacfP44654266;     AXvSEXacfP44654266 = AXvSEXacfP83140420;     AXvSEXacfP83140420 = AXvSEXacfP18618791;     AXvSEXacfP18618791 = AXvSEXacfP53331197;     AXvSEXacfP53331197 = AXvSEXacfP39041717;     AXvSEXacfP39041717 = AXvSEXacfP76915116;     AXvSEXacfP76915116 = AXvSEXacfP21532483;     AXvSEXacfP21532483 = AXvSEXacfP82792433;     AXvSEXacfP82792433 = AXvSEXacfP65875164;     AXvSEXacfP65875164 = AXvSEXacfP7062224;     AXvSEXacfP7062224 = AXvSEXacfP73570279;     AXvSEXacfP73570279 = AXvSEXacfP49289043;     AXvSEXacfP49289043 = AXvSEXacfP84628347;     AXvSEXacfP84628347 = AXvSEXacfP64748016;     AXvSEXacfP64748016 = AXvSEXacfP77040486;     AXvSEXacfP77040486 = AXvSEXacfP55696579;     AXvSEXacfP55696579 = AXvSEXacfP17072725;     AXvSEXacfP17072725 = AXvSEXacfP6711359;     AXvSEXacfP6711359 = AXvSEXacfP33086841;     AXvSEXacfP33086841 = AXvSEXacfP95375638;     AXvSEXacfP95375638 = AXvSEXacfP16108292;     AXvSEXacfP16108292 = AXvSEXacfP86687704;     AXvSEXacfP86687704 = AXvSEXacfP67257517;     AXvSEXacfP67257517 = AXvSEXacfP75353315;     AXvSEXacfP75353315 = AXvSEXacfP79519720;     AXvSEXacfP79519720 = AXvSEXacfP4961592;     AXvSEXacfP4961592 = AXvSEXacfP21196761;     AXvSEXacfP21196761 = AXvSEXacfP31810197;     AXvSEXacfP31810197 = AXvSEXacfP98096976;     AXvSEXacfP98096976 = AXvSEXacfP85421628;     AXvSEXacfP85421628 = AXvSEXacfP71740365;     AXvSEXacfP71740365 = AXvSEXacfP7272582;     AXvSEXacfP7272582 = AXvSEXacfP82658815;     AXvSEXacfP82658815 = AXvSEXacfP81694873;     AXvSEXacfP81694873 = AXvSEXacfP72545396;     AXvSEXacfP72545396 = AXvSEXacfP76400338;     AXvSEXacfP76400338 = AXvSEXacfP63707340;     AXvSEXacfP63707340 = AXvSEXacfP4901755;     AXvSEXacfP4901755 = AXvSEXacfP8248640;     AXvSEXacfP8248640 = AXvSEXacfP46569353;     AXvSEXacfP46569353 = AXvSEXacfP7755884;     AXvSEXacfP7755884 = AXvSEXacfP52682948;     AXvSEXacfP52682948 = AXvSEXacfP13600514;     AXvSEXacfP13600514 = AXvSEXacfP65093161;     AXvSEXacfP65093161 = AXvSEXacfP56288244;     AXvSEXacfP56288244 = AXvSEXacfP8498724;     AXvSEXacfP8498724 = AXvSEXacfP95461733;     AXvSEXacfP95461733 = AXvSEXacfP24202566;     AXvSEXacfP24202566 = AXvSEXacfP80249636;     AXvSEXacfP80249636 = AXvSEXacfP55631402;     AXvSEXacfP55631402 = AXvSEXacfP91192590;     AXvSEXacfP91192590 = AXvSEXacfP33877768;     AXvSEXacfP33877768 = AXvSEXacfP6354560;     AXvSEXacfP6354560 = AXvSEXacfP65529311;     AXvSEXacfP65529311 = AXvSEXacfP5117736;     AXvSEXacfP5117736 = AXvSEXacfP57966561;     AXvSEXacfP57966561 = AXvSEXacfP15882904;     AXvSEXacfP15882904 = AXvSEXacfP43265475;     AXvSEXacfP43265475 = AXvSEXacfP73811476;     AXvSEXacfP73811476 = AXvSEXacfP34080126;     AXvSEXacfP34080126 = AXvSEXacfP55718355;     AXvSEXacfP55718355 = AXvSEXacfP89722285;     AXvSEXacfP89722285 = AXvSEXacfP84695456;     AXvSEXacfP84695456 = AXvSEXacfP80453536;     AXvSEXacfP80453536 = AXvSEXacfP35321859;     AXvSEXacfP35321859 = AXvSEXacfP66297697;     AXvSEXacfP66297697 = AXvSEXacfP66630227;     AXvSEXacfP66630227 = AXvSEXacfP2933475;     AXvSEXacfP2933475 = AXvSEXacfP92202620;     AXvSEXacfP92202620 = AXvSEXacfP640148;     AXvSEXacfP640148 = AXvSEXacfP91989239;     AXvSEXacfP91989239 = AXvSEXacfP12170970;     AXvSEXacfP12170970 = AXvSEXacfP98462718;     AXvSEXacfP98462718 = AXvSEXacfP86517487;     AXvSEXacfP86517487 = AXvSEXacfP87619754;     AXvSEXacfP87619754 = AXvSEXacfP63425343;     AXvSEXacfP63425343 = AXvSEXacfP73087191;     AXvSEXacfP73087191 = AXvSEXacfP2164356;     AXvSEXacfP2164356 = AXvSEXacfP2920901;}
// Junk Finished

// Junk Code By Troll Face & Thaisen's Gen
void VJgSPShEop30373839() {     double ACoVlFqayJ47977584 = -142186456;    double ACoVlFqayJ97334510 = -998827245;    double ACoVlFqayJ58600321 = -921027414;    double ACoVlFqayJ54155279 = -396690593;    double ACoVlFqayJ23601422 = -871850873;    double ACoVlFqayJ72488991 = -728628047;    double ACoVlFqayJ89363523 = -586789782;    double ACoVlFqayJ74941698 = -105912615;    double ACoVlFqayJ80121388 = -707494145;    double ACoVlFqayJ78243026 = -783319585;    double ACoVlFqayJ30551900 = -921333642;    double ACoVlFqayJ61524954 = -314905246;    double ACoVlFqayJ8813747 = 37915384;    double ACoVlFqayJ3143729 = -19372986;    double ACoVlFqayJ83159877 = -863280261;    double ACoVlFqayJ43944541 = -367131931;    double ACoVlFqayJ89161830 = 49073787;    double ACoVlFqayJ91943703 = -320126688;    double ACoVlFqayJ41474592 = -3325078;    double ACoVlFqayJ45177287 = 78653142;    double ACoVlFqayJ34535575 = -785194593;    double ACoVlFqayJ48563084 = -136644187;    double ACoVlFqayJ98151871 = -101634339;    double ACoVlFqayJ80751077 = -637121859;    double ACoVlFqayJ20504075 = -846449925;    double ACoVlFqayJ29300757 = -520438426;    double ACoVlFqayJ20246966 = -480500415;    double ACoVlFqayJ73903828 = -219674194;    double ACoVlFqayJ64815428 = 12395696;    double ACoVlFqayJ32776843 = -944424784;    double ACoVlFqayJ1019545 = -693421383;    double ACoVlFqayJ91469538 = -83617653;    double ACoVlFqayJ63507222 = -418383731;    double ACoVlFqayJ61491202 = -721503291;    double ACoVlFqayJ84784653 = -268277312;    double ACoVlFqayJ84191709 = -100585178;    double ACoVlFqayJ39098639 = -37178381;    double ACoVlFqayJ37792900 = -282326548;    double ACoVlFqayJ7652830 = -596633467;    double ACoVlFqayJ50840322 = -647256872;    double ACoVlFqayJ27792345 = -542405062;    double ACoVlFqayJ58395531 = -578986815;    double ACoVlFqayJ56300072 = 22333375;    double ACoVlFqayJ61250573 = -494498088;    double ACoVlFqayJ97257778 = -139594668;    double ACoVlFqayJ76000781 = -749994985;    double ACoVlFqayJ47429159 = -264172036;    double ACoVlFqayJ25538402 = -835068363;    double ACoVlFqayJ53814477 = -989845931;    double ACoVlFqayJ16495823 = -548286802;    double ACoVlFqayJ76039108 = -813414643;    double ACoVlFqayJ21855440 = -48911445;    double ACoVlFqayJ71914941 = -798164137;    double ACoVlFqayJ43976383 = -54355135;    double ACoVlFqayJ95838263 = -574875826;    double ACoVlFqayJ99414499 = 94457730;    double ACoVlFqayJ99182639 = -797192906;    double ACoVlFqayJ77849243 = -183905556;    double ACoVlFqayJ33651204 = -550240668;    double ACoVlFqayJ94300665 = -251412447;    double ACoVlFqayJ52242025 = -148127633;    double ACoVlFqayJ15459696 = -267115589;    double ACoVlFqayJ10126271 = -18308311;    double ACoVlFqayJ47344546 = -763069362;    double ACoVlFqayJ77223481 = 10101797;    double ACoVlFqayJ39082362 = -737715990;    double ACoVlFqayJ98017731 = -896521516;    double ACoVlFqayJ47322544 = -240581326;    double ACoVlFqayJ18359076 = -751095674;    double ACoVlFqayJ98968167 = -662695084;    double ACoVlFqayJ4845903 = -229953550;    double ACoVlFqayJ51368931 = -668599666;    double ACoVlFqayJ84290874 = -723493221;    double ACoVlFqayJ90634269 = -356068206;    double ACoVlFqayJ17384942 = -378941796;    double ACoVlFqayJ76140044 = -106207778;    double ACoVlFqayJ92263012 = -58977562;    double ACoVlFqayJ36901298 = -607136252;    double ACoVlFqayJ83493299 = -397527191;    double ACoVlFqayJ44503294 = 3545060;    double ACoVlFqayJ81871598 = -156266390;    double ACoVlFqayJ94708564 = -645432052;    double ACoVlFqayJ20089351 = -229828263;    double ACoVlFqayJ48319606 = -439317503;    double ACoVlFqayJ56737735 = -31010141;    double ACoVlFqayJ79164105 = -544509938;    double ACoVlFqayJ19554598 = -285453517;    double ACoVlFqayJ19530839 = -264028596;    double ACoVlFqayJ65652939 = -46627465;    double ACoVlFqayJ85370153 = -262735043;    double ACoVlFqayJ85009070 = -303392272;    double ACoVlFqayJ61249395 = -853272826;    double ACoVlFqayJ4141696 = -732085880;    double ACoVlFqayJ13352165 = -245221020;    double ACoVlFqayJ98598297 = -399129240;    double ACoVlFqayJ12332650 = -175289474;    double ACoVlFqayJ48269261 = -460678505;    double ACoVlFqayJ8955526 = -214597264;    double ACoVlFqayJ84027091 = -404599886;    double ACoVlFqayJ58175416 = -142186456;     ACoVlFqayJ47977584 = ACoVlFqayJ97334510;     ACoVlFqayJ97334510 = ACoVlFqayJ58600321;     ACoVlFqayJ58600321 = ACoVlFqayJ54155279;     ACoVlFqayJ54155279 = ACoVlFqayJ23601422;     ACoVlFqayJ23601422 = ACoVlFqayJ72488991;     ACoVlFqayJ72488991 = ACoVlFqayJ89363523;     ACoVlFqayJ89363523 = ACoVlFqayJ74941698;     ACoVlFqayJ74941698 = ACoVlFqayJ80121388;     ACoVlFqayJ80121388 = ACoVlFqayJ78243026;     ACoVlFqayJ78243026 = ACoVlFqayJ30551900;     ACoVlFqayJ30551900 = ACoVlFqayJ61524954;     ACoVlFqayJ61524954 = ACoVlFqayJ8813747;     ACoVlFqayJ8813747 = ACoVlFqayJ3143729;     ACoVlFqayJ3143729 = ACoVlFqayJ83159877;     ACoVlFqayJ83159877 = ACoVlFqayJ43944541;     ACoVlFqayJ43944541 = ACoVlFqayJ89161830;     ACoVlFqayJ89161830 = ACoVlFqayJ91943703;     ACoVlFqayJ91943703 = ACoVlFqayJ41474592;     ACoVlFqayJ41474592 = ACoVlFqayJ45177287;     ACoVlFqayJ45177287 = ACoVlFqayJ34535575;     ACoVlFqayJ34535575 = ACoVlFqayJ48563084;     ACoVlFqayJ48563084 = ACoVlFqayJ98151871;     ACoVlFqayJ98151871 = ACoVlFqayJ80751077;     ACoVlFqayJ80751077 = ACoVlFqayJ20504075;     ACoVlFqayJ20504075 = ACoVlFqayJ29300757;     ACoVlFqayJ29300757 = ACoVlFqayJ20246966;     ACoVlFqayJ20246966 = ACoVlFqayJ73903828;     ACoVlFqayJ73903828 = ACoVlFqayJ64815428;     ACoVlFqayJ64815428 = ACoVlFqayJ32776843;     ACoVlFqayJ32776843 = ACoVlFqayJ1019545;     ACoVlFqayJ1019545 = ACoVlFqayJ91469538;     ACoVlFqayJ91469538 = ACoVlFqayJ63507222;     ACoVlFqayJ63507222 = ACoVlFqayJ61491202;     ACoVlFqayJ61491202 = ACoVlFqayJ84784653;     ACoVlFqayJ84784653 = ACoVlFqayJ84191709;     ACoVlFqayJ84191709 = ACoVlFqayJ39098639;     ACoVlFqayJ39098639 = ACoVlFqayJ37792900;     ACoVlFqayJ37792900 = ACoVlFqayJ7652830;     ACoVlFqayJ7652830 = ACoVlFqayJ50840322;     ACoVlFqayJ50840322 = ACoVlFqayJ27792345;     ACoVlFqayJ27792345 = ACoVlFqayJ58395531;     ACoVlFqayJ58395531 = ACoVlFqayJ56300072;     ACoVlFqayJ56300072 = ACoVlFqayJ61250573;     ACoVlFqayJ61250573 = ACoVlFqayJ97257778;     ACoVlFqayJ97257778 = ACoVlFqayJ76000781;     ACoVlFqayJ76000781 = ACoVlFqayJ47429159;     ACoVlFqayJ47429159 = ACoVlFqayJ25538402;     ACoVlFqayJ25538402 = ACoVlFqayJ53814477;     ACoVlFqayJ53814477 = ACoVlFqayJ16495823;     ACoVlFqayJ16495823 = ACoVlFqayJ76039108;     ACoVlFqayJ76039108 = ACoVlFqayJ21855440;     ACoVlFqayJ21855440 = ACoVlFqayJ71914941;     ACoVlFqayJ71914941 = ACoVlFqayJ43976383;     ACoVlFqayJ43976383 = ACoVlFqayJ95838263;     ACoVlFqayJ95838263 = ACoVlFqayJ99414499;     ACoVlFqayJ99414499 = ACoVlFqayJ99182639;     ACoVlFqayJ99182639 = ACoVlFqayJ77849243;     ACoVlFqayJ77849243 = ACoVlFqayJ33651204;     ACoVlFqayJ33651204 = ACoVlFqayJ94300665;     ACoVlFqayJ94300665 = ACoVlFqayJ52242025;     ACoVlFqayJ52242025 = ACoVlFqayJ15459696;     ACoVlFqayJ15459696 = ACoVlFqayJ10126271;     ACoVlFqayJ10126271 = ACoVlFqayJ47344546;     ACoVlFqayJ47344546 = ACoVlFqayJ77223481;     ACoVlFqayJ77223481 = ACoVlFqayJ39082362;     ACoVlFqayJ39082362 = ACoVlFqayJ98017731;     ACoVlFqayJ98017731 = ACoVlFqayJ47322544;     ACoVlFqayJ47322544 = ACoVlFqayJ18359076;     ACoVlFqayJ18359076 = ACoVlFqayJ98968167;     ACoVlFqayJ98968167 = ACoVlFqayJ4845903;     ACoVlFqayJ4845903 = ACoVlFqayJ51368931;     ACoVlFqayJ51368931 = ACoVlFqayJ84290874;     ACoVlFqayJ84290874 = ACoVlFqayJ90634269;     ACoVlFqayJ90634269 = ACoVlFqayJ17384942;     ACoVlFqayJ17384942 = ACoVlFqayJ76140044;     ACoVlFqayJ76140044 = ACoVlFqayJ92263012;     ACoVlFqayJ92263012 = ACoVlFqayJ36901298;     ACoVlFqayJ36901298 = ACoVlFqayJ83493299;     ACoVlFqayJ83493299 = ACoVlFqayJ44503294;     ACoVlFqayJ44503294 = ACoVlFqayJ81871598;     ACoVlFqayJ81871598 = ACoVlFqayJ94708564;     ACoVlFqayJ94708564 = ACoVlFqayJ20089351;     ACoVlFqayJ20089351 = ACoVlFqayJ48319606;     ACoVlFqayJ48319606 = ACoVlFqayJ56737735;     ACoVlFqayJ56737735 = ACoVlFqayJ79164105;     ACoVlFqayJ79164105 = ACoVlFqayJ19554598;     ACoVlFqayJ19554598 = ACoVlFqayJ19530839;     ACoVlFqayJ19530839 = ACoVlFqayJ65652939;     ACoVlFqayJ65652939 = ACoVlFqayJ85370153;     ACoVlFqayJ85370153 = ACoVlFqayJ85009070;     ACoVlFqayJ85009070 = ACoVlFqayJ61249395;     ACoVlFqayJ61249395 = ACoVlFqayJ4141696;     ACoVlFqayJ4141696 = ACoVlFqayJ13352165;     ACoVlFqayJ13352165 = ACoVlFqayJ98598297;     ACoVlFqayJ98598297 = ACoVlFqayJ12332650;     ACoVlFqayJ12332650 = ACoVlFqayJ48269261;     ACoVlFqayJ48269261 = ACoVlFqayJ8955526;     ACoVlFqayJ8955526 = ACoVlFqayJ84027091;     ACoVlFqayJ84027091 = ACoVlFqayJ58175416;     ACoVlFqayJ58175416 = ACoVlFqayJ47977584;}
// Junk Finished

// Junk Code By Troll Face & Thaisen's Gen
void bNKVtMpBLK27651668() {     double JyYqiLOaUy5830141 = -370343178;    double JyYqiLOaUy56381773 = -785326781;    double JyYqiLOaUy23442864 = -869219719;    double JyYqiLOaUy56305461 = -403708365;    double JyYqiLOaUy69220843 = -151398;    double JyYqiLOaUy88789547 = -52151509;    double JyYqiLOaUy23629333 = -548966355;    double JyYqiLOaUy43493068 = -516566082;    double JyYqiLOaUy96061960 = -266375521;    double JyYqiLOaUy88269809 = -553941045;    double JyYqiLOaUy85954889 = -760493700;    double JyYqiLOaUy3704375 = -694292949;    double JyYqiLOaUy82203659 = -11155966;    double JyYqiLOaUy45055124 = -178292101;    double JyYqiLOaUy32917561 = 68529863;    double JyYqiLOaUy97177206 = -51815430;    double JyYqiLOaUy38968359 = -935566016;    double JyYqiLOaUy74679779 = -448225826;    double JyYqiLOaUy62878282 = -937008358;    double JyYqiLOaUy45457026 = 11482074;    double JyYqiLOaUy68836121 = -723490217;    double JyYqiLOaUy37503562 = -218054911;    double JyYqiLOaUy35092178 = -556389273;    double JyYqiLOaUy57393701 = -731018797;    double JyYqiLOaUy35619583 = -65300395;    double JyYqiLOaUy11727846 = -744703528;    double JyYqiLOaUy57535379 = -843571335;    double JyYqiLOaUy85140178 = -668933834;    double JyYqiLOaUy5697757 = -259885911;    double JyYqiLOaUy71377563 = -123796447;    double JyYqiLOaUy30480903 = -939624788;    double JyYqiLOaUy2109891 = -116838660;    double JyYqiLOaUy71311480 = -893378987;    double JyYqiLOaUy50345479 = -133642222;    double JyYqiLOaUy44464433 = -23820061;    double JyYqiLOaUy34617233 = -550780114;    double JyYqiLOaUy70008799 = -635168995;    double JyYqiLOaUy72716724 = -137344922;    double JyYqiLOaUy51037926 = 64125641;    double JyYqiLOaUy63424676 = -639311761;    double JyYqiLOaUy2145656 = -726837744;    double JyYqiLOaUy50571388 = -837648715;    double JyYqiLOaUy90950282 = 40752259;    double JyYqiLOaUy41594133 = -439229704;    double JyYqiLOaUy81239451 = -401989906;    double JyYqiLOaUy89634422 = -59103042;    double JyYqiLOaUy51373927 = -845288027;    double JyYqiLOaUy12479808 = -149368591;    double JyYqiLOaUy52508659 = 60464352;    double JyYqiLOaUy18063735 = -494308991;    double JyYqiLOaUy52419679 = -364212471;    double JyYqiLOaUy64644911 = -311237326;    double JyYqiLOaUy43402144 = -334066912;    double JyYqiLOaUy10004505 = 10375408;    double JyYqiLOaUy58815547 = -205170810;    double JyYqiLOaUy68326579 = -52288267;    double JyYqiLOaUy21289596 = -128937508;    double JyYqiLOaUy66049162 = -38200922;    double JyYqiLOaUy20685879 = -238407971;    double JyYqiLOaUy57492997 = -255447870;    double JyYqiLOaUy31254169 = -208580174;    double JyYqiLOaUy38489155 = -880032522;    double JyYqiLOaUy37795311 = -156680172;    double JyYqiLOaUy24684398 = -42579074;    double JyYqiLOaUy57788907 = -614316258;    double JyYqiLOaUy83844999 = -543655041;    double JyYqiLOaUy32392894 = -800913962;    double JyYqiLOaUy31858180 = -877513745;    double JyYqiLOaUy590691 = -54472040;    double JyYqiLOaUy98300328 = -380690024;    double JyYqiLOaUy27168407 = -416646435;    double JyYqiLOaUy66251634 = -698221095;    double JyYqiLOaUy23641853 = -412351467;    double JyYqiLOaUy99453606 = -197696597;    double JyYqiLOaUy43311370 = -261680182;    double JyYqiLOaUy18264733 = -885841502;    double JyYqiLOaUy46553279 = -158807170;    double JyYqiLOaUy93498045 = -17159570;    double JyYqiLOaUy76154250 = -229028892;    double JyYqiLOaUy45985160 = 93802647;    double JyYqiLOaUy60353919 = -899415501;    double JyYqiLOaUy45055572 = -594202744;    double JyYqiLOaUy32631520 = -629398186;    double JyYqiLOaUy87634022 = -765576921;    double JyYqiLOaUy18957885 = -759583977;    double JyYqiLOaUy65835991 = -528387462;    double JyYqiLOaUy58707746 = -782771748;    double JyYqiLOaUy61306976 = -803754395;    double JyYqiLOaUy91529932 = -928471413;    double JyYqiLOaUy76137854 = -971531794;    double JyYqiLOaUy13327638 = -321842606;    double JyYqiLOaUy3959638 = -496968073;    double JyYqiLOaUy52030846 = -898936951;    double JyYqiLOaUy93544928 = -680426489;    double JyYqiLOaUy32170508 = -330731588;    double JyYqiLOaUy63656501 = -846805222;    double JyYqiLOaUy12776078 = -580968544;    double JyYqiLOaUy66265884 = -916668667;    double JyYqiLOaUy83805225 = -824913446;    double JyYqiLOaUy97394452 = -370343178;     JyYqiLOaUy5830141 = JyYqiLOaUy56381773;     JyYqiLOaUy56381773 = JyYqiLOaUy23442864;     JyYqiLOaUy23442864 = JyYqiLOaUy56305461;     JyYqiLOaUy56305461 = JyYqiLOaUy69220843;     JyYqiLOaUy69220843 = JyYqiLOaUy88789547;     JyYqiLOaUy88789547 = JyYqiLOaUy23629333;     JyYqiLOaUy23629333 = JyYqiLOaUy43493068;     JyYqiLOaUy43493068 = JyYqiLOaUy96061960;     JyYqiLOaUy96061960 = JyYqiLOaUy88269809;     JyYqiLOaUy88269809 = JyYqiLOaUy85954889;     JyYqiLOaUy85954889 = JyYqiLOaUy3704375;     JyYqiLOaUy3704375 = JyYqiLOaUy82203659;     JyYqiLOaUy82203659 = JyYqiLOaUy45055124;     JyYqiLOaUy45055124 = JyYqiLOaUy32917561;     JyYqiLOaUy32917561 = JyYqiLOaUy97177206;     JyYqiLOaUy97177206 = JyYqiLOaUy38968359;     JyYqiLOaUy38968359 = JyYqiLOaUy74679779;     JyYqiLOaUy74679779 = JyYqiLOaUy62878282;     JyYqiLOaUy62878282 = JyYqiLOaUy45457026;     JyYqiLOaUy45457026 = JyYqiLOaUy68836121;     JyYqiLOaUy68836121 = JyYqiLOaUy37503562;     JyYqiLOaUy37503562 = JyYqiLOaUy35092178;     JyYqiLOaUy35092178 = JyYqiLOaUy57393701;     JyYqiLOaUy57393701 = JyYqiLOaUy35619583;     JyYqiLOaUy35619583 = JyYqiLOaUy11727846;     JyYqiLOaUy11727846 = JyYqiLOaUy57535379;     JyYqiLOaUy57535379 = JyYqiLOaUy85140178;     JyYqiLOaUy85140178 = JyYqiLOaUy5697757;     JyYqiLOaUy5697757 = JyYqiLOaUy71377563;     JyYqiLOaUy71377563 = JyYqiLOaUy30480903;     JyYqiLOaUy30480903 = JyYqiLOaUy2109891;     JyYqiLOaUy2109891 = JyYqiLOaUy71311480;     JyYqiLOaUy71311480 = JyYqiLOaUy50345479;     JyYqiLOaUy50345479 = JyYqiLOaUy44464433;     JyYqiLOaUy44464433 = JyYqiLOaUy34617233;     JyYqiLOaUy34617233 = JyYqiLOaUy70008799;     JyYqiLOaUy70008799 = JyYqiLOaUy72716724;     JyYqiLOaUy72716724 = JyYqiLOaUy51037926;     JyYqiLOaUy51037926 = JyYqiLOaUy63424676;     JyYqiLOaUy63424676 = JyYqiLOaUy2145656;     JyYqiLOaUy2145656 = JyYqiLOaUy50571388;     JyYqiLOaUy50571388 = JyYqiLOaUy90950282;     JyYqiLOaUy90950282 = JyYqiLOaUy41594133;     JyYqiLOaUy41594133 = JyYqiLOaUy81239451;     JyYqiLOaUy81239451 = JyYqiLOaUy89634422;     JyYqiLOaUy89634422 = JyYqiLOaUy51373927;     JyYqiLOaUy51373927 = JyYqiLOaUy12479808;     JyYqiLOaUy12479808 = JyYqiLOaUy52508659;     JyYqiLOaUy52508659 = JyYqiLOaUy18063735;     JyYqiLOaUy18063735 = JyYqiLOaUy52419679;     JyYqiLOaUy52419679 = JyYqiLOaUy64644911;     JyYqiLOaUy64644911 = JyYqiLOaUy43402144;     JyYqiLOaUy43402144 = JyYqiLOaUy10004505;     JyYqiLOaUy10004505 = JyYqiLOaUy58815547;     JyYqiLOaUy58815547 = JyYqiLOaUy68326579;     JyYqiLOaUy68326579 = JyYqiLOaUy21289596;     JyYqiLOaUy21289596 = JyYqiLOaUy66049162;     JyYqiLOaUy66049162 = JyYqiLOaUy20685879;     JyYqiLOaUy20685879 = JyYqiLOaUy57492997;     JyYqiLOaUy57492997 = JyYqiLOaUy31254169;     JyYqiLOaUy31254169 = JyYqiLOaUy38489155;     JyYqiLOaUy38489155 = JyYqiLOaUy37795311;     JyYqiLOaUy37795311 = JyYqiLOaUy24684398;     JyYqiLOaUy24684398 = JyYqiLOaUy57788907;     JyYqiLOaUy57788907 = JyYqiLOaUy83844999;     JyYqiLOaUy83844999 = JyYqiLOaUy32392894;     JyYqiLOaUy32392894 = JyYqiLOaUy31858180;     JyYqiLOaUy31858180 = JyYqiLOaUy590691;     JyYqiLOaUy590691 = JyYqiLOaUy98300328;     JyYqiLOaUy98300328 = JyYqiLOaUy27168407;     JyYqiLOaUy27168407 = JyYqiLOaUy66251634;     JyYqiLOaUy66251634 = JyYqiLOaUy23641853;     JyYqiLOaUy23641853 = JyYqiLOaUy99453606;     JyYqiLOaUy99453606 = JyYqiLOaUy43311370;     JyYqiLOaUy43311370 = JyYqiLOaUy18264733;     JyYqiLOaUy18264733 = JyYqiLOaUy46553279;     JyYqiLOaUy46553279 = JyYqiLOaUy93498045;     JyYqiLOaUy93498045 = JyYqiLOaUy76154250;     JyYqiLOaUy76154250 = JyYqiLOaUy45985160;     JyYqiLOaUy45985160 = JyYqiLOaUy60353919;     JyYqiLOaUy60353919 = JyYqiLOaUy45055572;     JyYqiLOaUy45055572 = JyYqiLOaUy32631520;     JyYqiLOaUy32631520 = JyYqiLOaUy87634022;     JyYqiLOaUy87634022 = JyYqiLOaUy18957885;     JyYqiLOaUy18957885 = JyYqiLOaUy65835991;     JyYqiLOaUy65835991 = JyYqiLOaUy58707746;     JyYqiLOaUy58707746 = JyYqiLOaUy61306976;     JyYqiLOaUy61306976 = JyYqiLOaUy91529932;     JyYqiLOaUy91529932 = JyYqiLOaUy76137854;     JyYqiLOaUy76137854 = JyYqiLOaUy13327638;     JyYqiLOaUy13327638 = JyYqiLOaUy3959638;     JyYqiLOaUy3959638 = JyYqiLOaUy52030846;     JyYqiLOaUy52030846 = JyYqiLOaUy93544928;     JyYqiLOaUy93544928 = JyYqiLOaUy32170508;     JyYqiLOaUy32170508 = JyYqiLOaUy63656501;     JyYqiLOaUy63656501 = JyYqiLOaUy12776078;     JyYqiLOaUy12776078 = JyYqiLOaUy66265884;     JyYqiLOaUy66265884 = JyYqiLOaUy83805225;     JyYqiLOaUy83805225 = JyYqiLOaUy97394452;     JyYqiLOaUy97394452 = JyYqiLOaUy5830141;}
// Junk Finished

// Junk Code By Troll Face & Thaisen's Gen
void DZjPVdootX33650018() {     double pIGHVatwlX15145407 = -272874204;    double pIGHVatwlX11804696 = -801861284;    double pIGHVatwlX43594945 = -582860332;    double pIGHVatwlX14095484 = -595372357;    double pIGHVatwlX9284083 = 23333445;    double pIGHVatwlX82332909 = -611020369;    double pIGHVatwlX97283404 = -348215233;    double pIGHVatwlX32629224 = -713525230;    double pIGHVatwlX88870618 = -646436192;    double pIGHVatwlX56894267 = -374736713;    double pIGHVatwlX22856874 = -951734773;    double pIGHVatwlX14942124 = -231662764;    double pIGHVatwlX77985374 = -696578101;    double pIGHVatwlX77143765 = -157346205;    double pIGHVatwlX48193599 = -883000548;    double pIGHVatwlX2147731 = -79338908;    double pIGHVatwlX82779126 = -963151103;    double pIGHVatwlX86872419 = -178283139;    double pIGHVatwlX44891214 = -890649187;    double pIGHVatwlX8652374 = -67225767;    double pIGHVatwlX57515472 = 50809855;    double pIGHVatwlX61712473 = -40951243;    double pIGHVatwlX71153444 = -170918273;    double pIGHVatwlX16600331 = -248179908;    double pIGHVatwlX98682125 = -384433445;    double pIGHVatwlX77635663 = -843915169;    double pIGHVatwlX91293645 = -552460180;    double pIGHVatwlX56234654 = -76070566;    double pIGHVatwlX72814664 = -374305347;    double pIGHVatwlX84158511 = 70118990;    double pIGHVatwlX479282 = -550276605;    double pIGHVatwlX79369253 = -548009955;    double pIGHVatwlX21170811 = -704469999;    double pIGHVatwlX39374427 = -717202108;    double pIGHVatwlX31114709 = -200232655;    double pIGHVatwlX66442127 = -967005561;    double pIGHVatwlX7476408 = -236368857;    double pIGHVatwlX28753523 = -989940284;    double pIGHVatwlX17263627 = 12909854;    double pIGHVatwlX39524573 = -346497274;    double pIGHVatwlX26208790 = -863994162;    double pIGHVatwlX38223044 = -758920806;    double pIGHVatwlX12667288 = -181128122;    double pIGHVatwlX351033 = -929624592;    double pIGHVatwlX17189526 = -193765700;    double pIGHVatwlX59873658 = -384637601;    double pIGHVatwlX56623558 = -418122172;    double pIGHVatwlX83950844 = -595911980;    double pIGHVatwlX18535803 = -175232165;    double pIGHVatwlX87701636 = -287322693;    double pIGHVatwlX89387388 = -100088467;    double pIGHVatwlX28861954 = -959223543;    double pIGHVatwlX31409802 = 78156181;    double pIGHVatwlX14585749 = -349477710;    double pIGHVatwlX76134545 = -140088894;    double pIGHVatwlX53432933 = -131922961;    double pIGHVatwlX40651251 = -530943011;    double pIGHVatwlX26994614 = -234680425;    double pIGHVatwlX15413358 = -110938912;    double pIGHVatwlX31648419 = -132751387;    double pIGHVatwlX91039264 = 41439811;    double pIGHVatwlX41048751 = -172144668;    double pIGHVatwlX59814560 = -239219884;    double pIGHVatwlX4712107 = -616555183;    double pIGHVatwlX56414985 = -824460109;    double pIGHVatwlX43487620 = -303724818;    double pIGHVatwlX93771313 = -527192765;    double pIGHVatwlX38610948 = -979375993;    double pIGHVatwlX46029056 = -957113551;    double pIGHVatwlX81751472 = -915994988;    double pIGHVatwlX94671323 = -842970051;    double pIGHVatwlX54025603 = -973210819;    double pIGHVatwlX69608793 = -91192993;    double pIGHVatwlX5366642 = -444151913;    double pIGHVatwlX82443583 = -203231606;    double pIGHVatwlX19292429 = -190269339;    double pIGHVatwlX49045186 = -859823121;    double pIGHVatwlX70802411 = -241293682;    double pIGHVatwlX99410804 = 45585792;    double pIGHVatwlX38808468 = -999795845;    double pIGHVatwlX21012105 = -325792997;    double pIGHVatwlX7342801 = -956548201;    double pIGHVatwlX37698851 = -900838402;    double pIGHVatwlX85113027 = 13017346;    double pIGHVatwlX94771123 = -829792543;    double pIGHVatwlX71617328 = -591053062;    double pIGHVatwlX47959451 = -526166137;    double pIGHVatwlX6585063 = -254992290;    double pIGHVatwlX63239881 = -477113215;    double pIGHVatwlX77681775 = 31690306;    double pIGHVatwlX25790876 = -336062550;    double pIGHVatwlX80481793 = 98311567;    double pIGHVatwlX13340165 = -779001372;    double pIGHVatwlX85615208 = -854338760;    double pIGHVatwlX48485309 = -287937085;    double pIGHVatwlX85160039 = -591849495;    double pIGHVatwlX78408483 = -419700923;    double pIGHVatwlX7955182 = -564572940;    double pIGHVatwlX43936048 = -5164483;    double pIGHVatwlX73701905 = -272874204;     pIGHVatwlX15145407 = pIGHVatwlX11804696;     pIGHVatwlX11804696 = pIGHVatwlX43594945;     pIGHVatwlX43594945 = pIGHVatwlX14095484;     pIGHVatwlX14095484 = pIGHVatwlX9284083;     pIGHVatwlX9284083 = pIGHVatwlX82332909;     pIGHVatwlX82332909 = pIGHVatwlX97283404;     pIGHVatwlX97283404 = pIGHVatwlX32629224;     pIGHVatwlX32629224 = pIGHVatwlX88870618;     pIGHVatwlX88870618 = pIGHVatwlX56894267;     pIGHVatwlX56894267 = pIGHVatwlX22856874;     pIGHVatwlX22856874 = pIGHVatwlX14942124;     pIGHVatwlX14942124 = pIGHVatwlX77985374;     pIGHVatwlX77985374 = pIGHVatwlX77143765;     pIGHVatwlX77143765 = pIGHVatwlX48193599;     pIGHVatwlX48193599 = pIGHVatwlX2147731;     pIGHVatwlX2147731 = pIGHVatwlX82779126;     pIGHVatwlX82779126 = pIGHVatwlX86872419;     pIGHVatwlX86872419 = pIGHVatwlX44891214;     pIGHVatwlX44891214 = pIGHVatwlX8652374;     pIGHVatwlX8652374 = pIGHVatwlX57515472;     pIGHVatwlX57515472 = pIGHVatwlX61712473;     pIGHVatwlX61712473 = pIGHVatwlX71153444;     pIGHVatwlX71153444 = pIGHVatwlX16600331;     pIGHVatwlX16600331 = pIGHVatwlX98682125;     pIGHVatwlX98682125 = pIGHVatwlX77635663;     pIGHVatwlX77635663 = pIGHVatwlX91293645;     pIGHVatwlX91293645 = pIGHVatwlX56234654;     pIGHVatwlX56234654 = pIGHVatwlX72814664;     pIGHVatwlX72814664 = pIGHVatwlX84158511;     pIGHVatwlX84158511 = pIGHVatwlX479282;     pIGHVatwlX479282 = pIGHVatwlX79369253;     pIGHVatwlX79369253 = pIGHVatwlX21170811;     pIGHVatwlX21170811 = pIGHVatwlX39374427;     pIGHVatwlX39374427 = pIGHVatwlX31114709;     pIGHVatwlX31114709 = pIGHVatwlX66442127;     pIGHVatwlX66442127 = pIGHVatwlX7476408;     pIGHVatwlX7476408 = pIGHVatwlX28753523;     pIGHVatwlX28753523 = pIGHVatwlX17263627;     pIGHVatwlX17263627 = pIGHVatwlX39524573;     pIGHVatwlX39524573 = pIGHVatwlX26208790;     pIGHVatwlX26208790 = pIGHVatwlX38223044;     pIGHVatwlX38223044 = pIGHVatwlX12667288;     pIGHVatwlX12667288 = pIGHVatwlX351033;     pIGHVatwlX351033 = pIGHVatwlX17189526;     pIGHVatwlX17189526 = pIGHVatwlX59873658;     pIGHVatwlX59873658 = pIGHVatwlX56623558;     pIGHVatwlX56623558 = pIGHVatwlX83950844;     pIGHVatwlX83950844 = pIGHVatwlX18535803;     pIGHVatwlX18535803 = pIGHVatwlX87701636;     pIGHVatwlX87701636 = pIGHVatwlX89387388;     pIGHVatwlX89387388 = pIGHVatwlX28861954;     pIGHVatwlX28861954 = pIGHVatwlX31409802;     pIGHVatwlX31409802 = pIGHVatwlX14585749;     pIGHVatwlX14585749 = pIGHVatwlX76134545;     pIGHVatwlX76134545 = pIGHVatwlX53432933;     pIGHVatwlX53432933 = pIGHVatwlX40651251;     pIGHVatwlX40651251 = pIGHVatwlX26994614;     pIGHVatwlX26994614 = pIGHVatwlX15413358;     pIGHVatwlX15413358 = pIGHVatwlX31648419;     pIGHVatwlX31648419 = pIGHVatwlX91039264;     pIGHVatwlX91039264 = pIGHVatwlX41048751;     pIGHVatwlX41048751 = pIGHVatwlX59814560;     pIGHVatwlX59814560 = pIGHVatwlX4712107;     pIGHVatwlX4712107 = pIGHVatwlX56414985;     pIGHVatwlX56414985 = pIGHVatwlX43487620;     pIGHVatwlX43487620 = pIGHVatwlX93771313;     pIGHVatwlX93771313 = pIGHVatwlX38610948;     pIGHVatwlX38610948 = pIGHVatwlX46029056;     pIGHVatwlX46029056 = pIGHVatwlX81751472;     pIGHVatwlX81751472 = pIGHVatwlX94671323;     pIGHVatwlX94671323 = pIGHVatwlX54025603;     pIGHVatwlX54025603 = pIGHVatwlX69608793;     pIGHVatwlX69608793 = pIGHVatwlX5366642;     pIGHVatwlX5366642 = pIGHVatwlX82443583;     pIGHVatwlX82443583 = pIGHVatwlX19292429;     pIGHVatwlX19292429 = pIGHVatwlX49045186;     pIGHVatwlX49045186 = pIGHVatwlX70802411;     pIGHVatwlX70802411 = pIGHVatwlX99410804;     pIGHVatwlX99410804 = pIGHVatwlX38808468;     pIGHVatwlX38808468 = pIGHVatwlX21012105;     pIGHVatwlX21012105 = pIGHVatwlX7342801;     pIGHVatwlX7342801 = pIGHVatwlX37698851;     pIGHVatwlX37698851 = pIGHVatwlX85113027;     pIGHVatwlX85113027 = pIGHVatwlX94771123;     pIGHVatwlX94771123 = pIGHVatwlX71617328;     pIGHVatwlX71617328 = pIGHVatwlX47959451;     pIGHVatwlX47959451 = pIGHVatwlX6585063;     pIGHVatwlX6585063 = pIGHVatwlX63239881;     pIGHVatwlX63239881 = pIGHVatwlX77681775;     pIGHVatwlX77681775 = pIGHVatwlX25790876;     pIGHVatwlX25790876 = pIGHVatwlX80481793;     pIGHVatwlX80481793 = pIGHVatwlX13340165;     pIGHVatwlX13340165 = pIGHVatwlX85615208;     pIGHVatwlX85615208 = pIGHVatwlX48485309;     pIGHVatwlX48485309 = pIGHVatwlX85160039;     pIGHVatwlX85160039 = pIGHVatwlX78408483;     pIGHVatwlX78408483 = pIGHVatwlX7955182;     pIGHVatwlX7955182 = pIGHVatwlX43936048;     pIGHVatwlX43936048 = pIGHVatwlX73701905;     pIGHVatwlX73701905 = pIGHVatwlX15145407;}
// Junk Finished

// Junk Code By Troll Face & Thaisen's Gen
void HfotCsnLdK91890899() {     double yKZyJQCriz53802535 = -529780241;    double yKZyJQCriz37126417 = -989347613;    double yKZyJQCriz85091318 = -911335099;    double yKZyJQCriz17034080 = -417438790;    double yKZyJQCriz36737102 = -638130686;    double yKZyJQCriz20681941 = -67740890;    double yKZyJQCriz21105917 = -953224869;    double yKZyJQCriz34137051 = -267844606;    double yKZyJQCriz18554385 = -646795605;    double yKZyJQCriz99191776 = -631243902;    double yKZyJQCriz37830303 = 32454012;    double yKZyJQCriz94924979 = -288747149;    double yKZyJQCriz25792618 = -394121651;    double yKZyJQCriz79229593 = -680525151;    double yKZyJQCriz25921727 = -547493808;    double yKZyJQCriz49154162 = -869674449;    double yKZyJQCriz88589826 = -901165630;    double yKZyJQCriz1772100 = -316245877;    double yKZyJQCriz65624634 = -755084340;    double yKZyJQCriz85134775 = -215591755;    double yKZyJQCriz70728493 = -76677306;    double yKZyJQCriz2821887 = -42554154;    double yKZyJQCriz42149299 = -776561971;    double yKZyJQCriz85607531 = -53860634;    double yKZyJQCriz26062969 = -354355662;    double yKZyJQCriz29519977 = -753048294;    double yKZyJQCriz87012708 = -166970963;    double yKZyJQCriz54950429 = -687050520;    double yKZyJQCriz20467530 = -840436881;    double yKZyJQCriz33857234 = 46998125;    double yKZyJQCriz96818342 = -799587970;    double yKZyJQCriz96841013 = 57294151;    double yKZyJQCriz73537203 = -770543619;    double yKZyJQCriz63321237 = -513914043;    double yKZyJQCriz22098786 = -789012395;    double yKZyJQCriz81101952 = -283770206;    double yKZyJQCriz13093897 = -848628891;    double yKZyJQCriz1915513 = -953685219;    double yKZyJQCriz40269636 = -795258713;    double yKZyJQCriz92394064 = -145506110;    double yKZyJQCriz78054307 = -705075600;    double yKZyJQCriz83089369 = -4595910;    double yKZyJQCriz45700694 = -975384706;    double yKZyJQCriz11831531 = -905008952;    double yKZyJQCriz41203594 = -150154501;    double yKZyJQCriz64135027 = -763879675;    double yKZyJQCriz63439779 = -356167141;    double yKZyJQCriz65191253 = -99086429;    double yKZyJQCriz97779884 = 11071427;    double yKZyJQCriz68957475 = -484352403;    double yKZyJQCriz71425142 = -967947350;    double yKZyJQCriz65754748 = -920135787;    double yKZyJQCriz39790151 = -573876691;    double yKZyJQCriz39189958 = -580369183;    double yKZyJQCriz21162406 = -820965342;    double yKZyJQCriz50980649 = -387226088;    double yKZyJQCriz94977117 = -112785642;    double yKZyJQCriz99483787 = -757474466;    double yKZyJQCriz90971110 = 36916871;    double yKZyJQCriz7217125 = -885082393;    double yKZyJQCriz33669233 = -900769928;    double yKZyJQCriz66155487 = -166174349;    double yKZyJQCriz13669521 = -427407725;    double yKZyJQCriz84697151 = -593793730;    double yKZyJQCriz2373435 = -831655932;    double yKZyJQCriz40989289 = 75159860;    double yKZyJQCriz21387776 = -518203530;    double yKZyJQCriz62471380 = -880207608;    double yKZyJQCriz57130807 = -891512757;    double yKZyJQCriz44819774 = -163723602;    double yKZyJQCriz36060265 = 78954441;    double yKZyJQCriz86674314 = -947480412;    double yKZyJQCriz61502464 = -520987165;    double yKZyJQCriz73230570 = -509578231;    double yKZyJQCriz7080468 = -510516156;    double yKZyJQCriz87639124 = 27918603;    double yKZyJQCriz57121193 = -67169448;    double yKZyJQCriz30317768 = -871553020;    double yKZyJQCriz44403937 = -903706134;    double yKZyJQCriz61927942 = -590475987;    double yKZyJQCriz66080198 = -296881154;    double yKZyJQCriz21821456 = 32115466;    double yKZyJQCriz57170545 = -598121947;    double yKZyJQCriz51510055 = -256084478;    double yKZyJQCriz62432091 = 14945474;    double yKZyJQCriz31063594 = -879452184;    double yKZyJQCriz57050862 = -368829158;    double yKZyJQCriz34347245 = -90174437;    double yKZyJQCriz42158832 = -692948702;    double yKZyJQCriz71118137 = -301786307;    double yKZyJQCriz86124834 = -70984564;    double yKZyJQCriz13610110 = 8845574;    double yKZyJQCriz10944402 = -890602090;    double yKZyJQCriz33052511 = -910176320;    double yKZyJQCriz58724832 = -244736183;    double yKZyJQCriz11898820 = -438901251;    double yKZyJQCriz69419849 = -577188185;    double yKZyJQCriz61003543 = -281590977;    double yKZyJQCriz9458097 = 26646979;    double yKZyJQCriz214305 = -529780241;     yKZyJQCriz53802535 = yKZyJQCriz37126417;     yKZyJQCriz37126417 = yKZyJQCriz85091318;     yKZyJQCriz85091318 = yKZyJQCriz17034080;     yKZyJQCriz17034080 = yKZyJQCriz36737102;     yKZyJQCriz36737102 = yKZyJQCriz20681941;     yKZyJQCriz20681941 = yKZyJQCriz21105917;     yKZyJQCriz21105917 = yKZyJQCriz34137051;     yKZyJQCriz34137051 = yKZyJQCriz18554385;     yKZyJQCriz18554385 = yKZyJQCriz99191776;     yKZyJQCriz99191776 = yKZyJQCriz37830303;     yKZyJQCriz37830303 = yKZyJQCriz94924979;     yKZyJQCriz94924979 = yKZyJQCriz25792618;     yKZyJQCriz25792618 = yKZyJQCriz79229593;     yKZyJQCriz79229593 = yKZyJQCriz25921727;     yKZyJQCriz25921727 = yKZyJQCriz49154162;     yKZyJQCriz49154162 = yKZyJQCriz88589826;     yKZyJQCriz88589826 = yKZyJQCriz1772100;     yKZyJQCriz1772100 = yKZyJQCriz65624634;     yKZyJQCriz65624634 = yKZyJQCriz85134775;     yKZyJQCriz85134775 = yKZyJQCriz70728493;     yKZyJQCriz70728493 = yKZyJQCriz2821887;     yKZyJQCriz2821887 = yKZyJQCriz42149299;     yKZyJQCriz42149299 = yKZyJQCriz85607531;     yKZyJQCriz85607531 = yKZyJQCriz26062969;     yKZyJQCriz26062969 = yKZyJQCriz29519977;     yKZyJQCriz29519977 = yKZyJQCriz87012708;     yKZyJQCriz87012708 = yKZyJQCriz54950429;     yKZyJQCriz54950429 = yKZyJQCriz20467530;     yKZyJQCriz20467530 = yKZyJQCriz33857234;     yKZyJQCriz33857234 = yKZyJQCriz96818342;     yKZyJQCriz96818342 = yKZyJQCriz96841013;     yKZyJQCriz96841013 = yKZyJQCriz73537203;     yKZyJQCriz73537203 = yKZyJQCriz63321237;     yKZyJQCriz63321237 = yKZyJQCriz22098786;     yKZyJQCriz22098786 = yKZyJQCriz81101952;     yKZyJQCriz81101952 = yKZyJQCriz13093897;     yKZyJQCriz13093897 = yKZyJQCriz1915513;     yKZyJQCriz1915513 = yKZyJQCriz40269636;     yKZyJQCriz40269636 = yKZyJQCriz92394064;     yKZyJQCriz92394064 = yKZyJQCriz78054307;     yKZyJQCriz78054307 = yKZyJQCriz83089369;     yKZyJQCriz83089369 = yKZyJQCriz45700694;     yKZyJQCriz45700694 = yKZyJQCriz11831531;     yKZyJQCriz11831531 = yKZyJQCriz41203594;     yKZyJQCriz41203594 = yKZyJQCriz64135027;     yKZyJQCriz64135027 = yKZyJQCriz63439779;     yKZyJQCriz63439779 = yKZyJQCriz65191253;     yKZyJQCriz65191253 = yKZyJQCriz97779884;     yKZyJQCriz97779884 = yKZyJQCriz68957475;     yKZyJQCriz68957475 = yKZyJQCriz71425142;     yKZyJQCriz71425142 = yKZyJQCriz65754748;     yKZyJQCriz65754748 = yKZyJQCriz39790151;     yKZyJQCriz39790151 = yKZyJQCriz39189958;     yKZyJQCriz39189958 = yKZyJQCriz21162406;     yKZyJQCriz21162406 = yKZyJQCriz50980649;     yKZyJQCriz50980649 = yKZyJQCriz94977117;     yKZyJQCriz94977117 = yKZyJQCriz99483787;     yKZyJQCriz99483787 = yKZyJQCriz90971110;     yKZyJQCriz90971110 = yKZyJQCriz7217125;     yKZyJQCriz7217125 = yKZyJQCriz33669233;     yKZyJQCriz33669233 = yKZyJQCriz66155487;     yKZyJQCriz66155487 = yKZyJQCriz13669521;     yKZyJQCriz13669521 = yKZyJQCriz84697151;     yKZyJQCriz84697151 = yKZyJQCriz2373435;     yKZyJQCriz2373435 = yKZyJQCriz40989289;     yKZyJQCriz40989289 = yKZyJQCriz21387776;     yKZyJQCriz21387776 = yKZyJQCriz62471380;     yKZyJQCriz62471380 = yKZyJQCriz57130807;     yKZyJQCriz57130807 = yKZyJQCriz44819774;     yKZyJQCriz44819774 = yKZyJQCriz36060265;     yKZyJQCriz36060265 = yKZyJQCriz86674314;     yKZyJQCriz86674314 = yKZyJQCriz61502464;     yKZyJQCriz61502464 = yKZyJQCriz73230570;     yKZyJQCriz73230570 = yKZyJQCriz7080468;     yKZyJQCriz7080468 = yKZyJQCriz87639124;     yKZyJQCriz87639124 = yKZyJQCriz57121193;     yKZyJQCriz57121193 = yKZyJQCriz30317768;     yKZyJQCriz30317768 = yKZyJQCriz44403937;     yKZyJQCriz44403937 = yKZyJQCriz61927942;     yKZyJQCriz61927942 = yKZyJQCriz66080198;     yKZyJQCriz66080198 = yKZyJQCriz21821456;     yKZyJQCriz21821456 = yKZyJQCriz57170545;     yKZyJQCriz57170545 = yKZyJQCriz51510055;     yKZyJQCriz51510055 = yKZyJQCriz62432091;     yKZyJQCriz62432091 = yKZyJQCriz31063594;     yKZyJQCriz31063594 = yKZyJQCriz57050862;     yKZyJQCriz57050862 = yKZyJQCriz34347245;     yKZyJQCriz34347245 = yKZyJQCriz42158832;     yKZyJQCriz42158832 = yKZyJQCriz71118137;     yKZyJQCriz71118137 = yKZyJQCriz86124834;     yKZyJQCriz86124834 = yKZyJQCriz13610110;     yKZyJQCriz13610110 = yKZyJQCriz10944402;     yKZyJQCriz10944402 = yKZyJQCriz33052511;     yKZyJQCriz33052511 = yKZyJQCriz58724832;     yKZyJQCriz58724832 = yKZyJQCriz11898820;     yKZyJQCriz11898820 = yKZyJQCriz69419849;     yKZyJQCriz69419849 = yKZyJQCriz61003543;     yKZyJQCriz61003543 = yKZyJQCriz9458097;     yKZyJQCriz9458097 = yKZyJQCriz214305;     yKZyJQCriz214305 = yKZyJQCriz53802535;}
// Junk Finished
