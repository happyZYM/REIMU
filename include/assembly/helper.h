// Should be included only in assembly.cpp
#include "assembly.h"
#include "storage.h"
#include <utility.h>
#include "exception.h"

// Some useful helper functions.

namespace dark {

/* Whether the character is a valid token character. */
bool is_label_char(char c) {
    return std::isalnum(c) || c == '_' || c == '.' || c == '@';
}

/* Whether the character is a valid split character. */
static bool is_split_char(char c) {
    return std::isspace(c) || c == ',';
}

/* Remove the front whitespace of the string. */
static auto remove_front_whitespace(std::string_view str) -> std::string_view {
    while (!str.empty() && std::isspace(str.front()))
        str.remove_prefix(1);
    return str;
}

/* Remove the back whitespace of the string. */
static auto remove_back_whitespace(std::string_view str) -> std::string_view {
    while (!str.empty() && std::isspace(str.back()))
        str.remove_suffix(1);
    return str;
}

/* Whether the string part contains no token. */
static bool contain_no_token(std::string_view str) {
    std::string_view tmp = remove_front_whitespace(str);
    return tmp.empty() || tmp.front() == '#';
}

/* Check whether the label is valid. */
static bool is_valid_label(std::string_view str) {
    return std::ranges::find_if_not(str, is_label_char) == str.end();
}

/* Whether the string part starts with a label. */
static auto start_with_label(std::string_view str) -> std::optional <std::string_view> {
    auto pos1 = str.find_first_of('\"');
    auto pos2 = str.find_first_of(':');
    if (pos2 != str.npos && pos2 < pos1) {
        auto label = str.substr(0, pos2);
        auto rest  = str.substr(pos2 + 1);
        throw_if <FailToParse> (!is_valid_label(label), "Invalid label: \"{}\"", label);
        throw_if <FailToParse> (!contain_no_token(rest), "Unexpected token after label");
        return label;
    }
    return std::nullopt;
}

/* Find the first token in the string. */
static auto find_first_token(std::string_view str)
    -> std::pair <std::string_view, std::string_view> {
    str = remove_front_whitespace(str);
    auto pos = std::ranges::find_if(str, [](char c) {
        return is_split_char(c) || c == '#';
    }) - str.begin();
    return { str.substr(0, pos), str.substr(pos) };
}

/* Find and extract the first asciz string. */
static auto find_first_asciz(std::string_view str)
    -> std::pair <std::string, std::string_view> {
    str = remove_front_whitespace(str);

    enum class _Error_t {
        INVALID = 0,
        ESCAPE  = 1,
        NOEND   = 2,
    };
    using enum _Error_t;

    constexpr auto __throw_invalid = [][[noreturn]](_Error_t num) {
        switch (num) {
            case _Error_t::INVALID: throw FailToParse { "Invalid ascii string" };
            case _Error_t::ESCAPE:  throw FailToParse { "Invalid escape character" };
            case _Error_t::NOEND:   throw FailToParse { "Missing end of string" };
            default: unreachable();
        }
    };

    if (str.empty() || str.front() != '\"') __throw_invalid(INVALID);

    // Parse the string
    std::string ret;
    for (std::size_t i = 1; i < str.size() ; ++i) {
        switch (str[i]) {
            case '\\':
                switch (str[++i]) { // Even safe when out of bound ('\0' case)
                    case 'n': ret.push_back('\n'); break;
                    case 't': ret.push_back('\t'); break;
                    case 'r': ret.push_back('\r'); break;
                    case '0': ret.push_back('\0'); break;
                    case '\\': ret.push_back('\\'); break;
                    case '\"': ret.push_back('\"'); break;
                    default: __throw_invalid(ESCAPE);
                } break;
            case '\"': return { ret, str.substr(i + 1) };
            default: ret.push_back(str[i]);
        }
    }
    __throw_invalid(NOEND);
}

static bool match_string(std::string_view str, std::initializer_list <std::string_view> list) {
    for (std::string_view s : list) if (str == s) return true;
    return false;
}

static bool match_prefix(std::string_view str, std::initializer_list <std::string_view> list) {
    for (std::string_view s : list) if (str.starts_with(s)) return true;
    return false;
}

/* Remove the comment and make sure there is no '\"' in the string." */
static auto remove_comments_when_no_string(std::string_view str) -> std::string_view {
    throw_if <FailToParse> (str.find('\"') != str.npos);
    auto pos = str.find_first_of('#');
    return str.substr(0, pos == str.npos ? str.size() : pos);
}

template <std::size_t _N, char _Indent = ','>
static auto split_command(std::string_view str) {
    if constexpr (_N == 0) {
        return throw_if <FailToParse> (!contain_no_token(str));
    } else {
        std::array <std::string_view, _N> ret;
        str = remove_front_whitespace(remove_comments_when_no_string(str));

        if constexpr (_N == 1) {
            ret[0] = remove_back_whitespace(str);
            return ret; 
        }

        std::size_t pos = str.find_first_of(_Indent);
        std::size_t i = 0;

        while (pos != str.npos) {
            ret[i++] = remove_back_whitespace(str.substr(0, pos));
            str.remove_prefix(pos + 1);
            str = remove_front_whitespace(str);
            if (i == _N - 1) {
                ret[i] = remove_back_whitespace(str);
                return ret;
            }
            pos = str.find_first_of(_Indent);
        }

        throw FailToParse { "Too few arguments" }; 
    }
}

static auto split_offset_and_register(std::string_view str)
-> std::pair <std::string_view, std::string_view> {
    do {
        if (str.empty() || str.back() != ')') break;
        str.remove_suffix(1);

        auto pos = str.find_last_of('(');
        if (pos == str.npos) break;

        return { str.substr(0, pos), str.substr(pos + 1) };
    } while (false);
    throw FailToParse { std::format("Invalid immediate and offset: \"{}\"", str) };
}

} // namespace dark
