#include "gb_error.h"

const char* s_errorStrings[Error::Count] = {
    "None",
    "InvalidFile",
    "OutOfMemory",
    "InvalidArgument",
    "NotImplemented",
    "FreeTypeError"
};

const char* Error::ToString(Code code)
{
    if (err >= 0 && err < Count) {
        return s_errorStrings[code];
    } else {
        return "???";
    }
}
