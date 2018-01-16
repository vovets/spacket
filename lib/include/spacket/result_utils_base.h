#pragma once

#include <spacket/result.h>

#define returnOnFail(var, failType) \
    if (isFail(var)) { return fail<failType>(getFailUnsafe(var)); }
