#ifndef GB_ERROR_H
#define GB_ERROR_H

#ifdef __cplusplus
extern "C" {
#endif

typedef enum GB_Error_Code {
    GB_ERROR_NONE = 0,
    GB_ERROR_NOENT,  // invalid file
    GB_ERROR_NOMEM,  // out of memory
    GB_ERROR_INVAL,  // invalid argument
    GB_ERROR_NOIMP,  // not implemented
    GB_ERROR_FTERR,  // freetype2 error
    GB_ERROR_NUM_ERRORS
} GB_ERROR;

const char* GB_ErrorToString(GB_ERROR err);

#ifdef __cplusplus
}
#endif

#endif // GB_ERROR_H
