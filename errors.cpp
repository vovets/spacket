#include <spacket/errors.h>

#include <boost/preprocessor/stringize.hpp>

#include <iostream>
#include <unordered_map>

static const std::unordered_map<typename std::underlying_type<Error>::type, std::string> errorToString{
#define X(ID, CODE, SEP) { CODE, ID } SEP
#define STR(X) BOOST_PP_STRINGIZE(X)
    ERROR_LIST(STR, ID, SEP_COMMA)
#undef X
#undef STR
};

std::string toString(Error e) {
    auto it = errorToString.find(toInt(e));
    if (it == errorToString.end()) {
        return { "no such error defined, something went terribly wrong" };
    }
    return it->second;
}
