#ifndef GB_ERROR_H
#define GB_ERROR_H

namespace gb {

class Error
{
    enum Code {
        None = 0,
        InvalidFile,  // invalid file
        OutOfMemory,  // out of memory
        InvalidArgument,  // invalid argument
        NotImplemented,  // not implemented
        FreeTypeError,  // freetype2 error
        Count
    };

    static const char* ToString(Code code);
};

}

#endif // GB_ERROR_H
