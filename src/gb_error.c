#include "gb_error.h"

const char* s_error_strings[GB_ERROR_NUM_ERRORS] = {
    "GB_ERROR_NONE",
    "GB_ERROR_NOENT",
    "GB_ERROR_NOMEM",
    "GB_ERROR_INVAL",
    "GB_ERROR_NOIMP",
    "GB_ERROR_FTERR"
};

const char* GB_ErrorToString(GB_ERROR err)
{
    if (err >= 0 && err < GB_ERROR_NUM_ERRORS) {
        return s_error_strings[err];
    } else {
        return "???";
    }
}
