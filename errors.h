#pragma once

#include <type_traits>
#include <string>

#ifdef ERROR_LIST
#error "ERROR_LIST macro should not be defined here"
#else
#define ERROR_LIST(ID, CODE, SEP)               \
    X(ID(ConfigNoDevSpecified), CODE(1), SEP()) \
    X(ID(DevInitFailed), CODE(2), )
#endif

#define ID(X) X
#define SEP_COMMA() ,

enum class Error {
#define X(ID, CODE, SEP) ID = CODE SEP
    ERROR_LIST(ID, ID, SEP_COMMA)
#undef X
};

constexpr typename std::underlying_type<Error>::type toInt(Error e) noexcept {
    return static_cast<typename std::underlying_type<Error>::type>(e);
}

std::string toString(Error e);

// never returns
void fatal(Error e);
