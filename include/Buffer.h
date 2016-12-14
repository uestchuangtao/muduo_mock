#ifndef BUFFER_H
#define BUFFER_H

#include "StringPiece.h"

#include <algorithm>
#include <vector>

#include <assert.h>
#include <string.h>

class Buffer
{
    public:
        static const size_t kCheapPrepend = 8;
        static const size_t kInitialSize = 1024;

        explicit Buffer(size_t initialSize = KInitialSize)
            : M_buffer(kCheapPrepend + initialSize),
              M_readerIndex(kCheapPrepend),
              M_writerIndex(kCheapPrepend)
              {
                assert(readableBytes() == 0);
                assert(writableBytes() == initialSize);
                assert(prependableBytes() == kCheapPrepend);
              }

              void swap(Buffer &rhs)
              {
                M_buffer.swap()
              }

    private:
        std::vector<char> M_buffer;
        size_t M_readerIndex;
        size_t M_writerIndex;

        static const char kCRLF[];
};

#endif // BUFFER_H
