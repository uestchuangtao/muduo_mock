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
                M_buffer.swap(rhs.M_buffer);
                std::swap(M_readerIndex, rhs.M_readerIndex);
                std::swap(M_writerIndex, rhs.M_writerIndex);
              }

              size_t readableBytes() const 
              {
                return M_writerIndex - M_readerIndex;
              }

              size_t writableBytes() const 
              {
                return M_buffer.size() - M_writerIndex;
              }

              size_t prependableBytes() const 
              {
                return M_readerIndex;
              }

              const char* peek() const 
              {
                return begin() + M_readerIndex;
              }

              const char* findCRLF() const 
              {
                const char *crlf = std::search(peek(), beginWrite(), KCRLF, KCRLF+2);
                return crlf == beginWrite() ? NULL : crlf;
              }

              const char* findCRLF(const char* start) const
              {
                assert(peek() <= start);
                assert(start <= bneginWrite());
                const char* crlf = std::search(start, beginWrite(), KCRLF, KCRLF+2);
                return crlf == beginWrite() ? : NULL : crlf;
              } 

              const char* findEOL() const 
              {
                const void* eol = memchr(peek(), '\n', readableBytes());
                return static_cast<const char*>(eol);
              }

              const char* findEOL(const char* start) const
              {
                assert(peek() <= start);
                assert(start <= beginWrite());
                const void* eol = memchr(start, '\n', beginWrite() - start);
                return static_cast<const char*>(eol);
              }

              //retrieve returns void, to prevent
              //string str(retrieve(readableBytes()), readableBytes());
              //the evaluation of two function are unspecified

              void retrieve(size_t len)
              {
                assert(len <= readableBytes());
                if(len < readableBytes())
                {
                  M_readerIndex += len;
                }
                else
                {
                  retrieveAll();
                }
              }

              void retrieveUntil(const char* end)
              {
                assert(peek() <= end);
                assert(end <= beginWrite());
                retrieve(end - peek());
              }

              void retrieveInt64()
              {
                retrieve(sizeof(int64_t));
              }

              void retrieveInt32()
              {
                retrieve(sizeof(int32_t));
              }

              void retrieveInt16()
              {
                retrieve(sizeof(int16_t));
              }

              void retrieveInt8()
              {
                retrieve(sizeof(int8_t));
              }

              void retrieveAll()
              {
                M_readerIndex = kCheapPrepend;
                M_writerIndex = kCheapPrepend;
              }

              string retrieveAsString(size_t len)
              {
                assert(len <= readableBytes());
                string result(peek(), len);
                retrieve(len);
                return result;
              }

              stringPiece toStringPiece() const
              {
                return StringPiece(peek(), static_cast<int>(readableBytes()));
              } 

              void append(const StringPiece& str)
              {
                append(str.data(), str.size());
              }

              void append(const char* data, size_t len)
              {
                ensureWritableBytes(len);
                std::copy(data, data+len, beginWrite());
                hasWritten(len);
              }

              void append(const void* data, size_t len)
              {
                append(static_cast<const char*>(data), len);
              }

              void ensureWritableBytes(size_t len)
              {
                if( writableBytes() < len)
                {
                  makeSpace(len);
                }
                assert(writableBytes() >= len);
              }

              char* beginWrite()
              {
                return begin() + M_writerIndex;
              }

              const char* beginWrite() const
              {
                return begin() + M_writerIndex;
              }

              void hasWritten(size_t len)
              {
                assert(len <= writableBytes());
                M_writerIndex += len；
              }

              void unwrite(size_t len)
              {
                assert(len <= readableBytes());
                M_writerIndex -= len;
              }


              ///
              /// Append int64_t using network endian
              ///
              void appendInt64(int64_t x)
              {
                int64_t be64 = sockets::hostToNetwork64(x);
                append(&be64,sizeof(x));
              }

              ///
              /// Append int32_t using network endian
              ///
              void appendInt32(int32_t x)
              {
                int32_t be32 = sockets::hostToNetwork32(x);
                append(&be32,sizeof(x));
              }

              ///
              /// Append int16_t using network endian
              ///
              void appendInt16(int16_t x)
              {
                int64_t be16 = sockets::hostToNetwork16(x);
                append(&be16,sizeof(x));
              }

              ///
              /// Append int8_t using network endian
              ///
              void appendInt8(int8_t x)
              {
                int8_t be8 = sockets::hostToNetwork8(x);
                append(&be8,sizeof(x));
              }

              ///
              ///Read int64_t from network endian
              ///
              ///Require: buf->readableBytes() >= sizeof(int64_t)
              int64_t readInt64()
              {
                int64_t result = peekInt64();
                retrieveInt64();
                return result;
              }

              ///
              ///Read int32_t from network endian
              ///
              ///Require: buf->readableBytes() >= sizeof(int32_t)
              int64_t readInt32()
              {
                int64_t result = peekInt32();
                retrieveInt32();
                return result;
              }

              ///
              ///Read int16_t from network endian
              ///
              ///Require: buf->readableBytes() >= sizeof(int16_t)
              int64_t readInt16()
              {
                int64_t result = peekInt16();
                retrieveInt16();
                return result;
              }

              ///
              ///Read int8_t from network endian
              ///
              ///Require: buf->readableBytes() >= sizeof(int8_t)
              int64_t readInt8()
              {
                int64_t result = peekInt8();
                retrieveInt8();
                return result;
              }

              ///
              ///Peek int64_t from network endian
              ///
              ///Require : buf->readableBytes() >= sizeof(int64_t)
              int32_t peekInt64() const 
              {
                assert(readableBytes() >= sizeof(int64_t));
                int64_t be64 = 0;
                ::memcpy(&be64, peek(), sizeof(be64));
                return sockets::networkToHost64(be64);
              }

              ///
              ///Peek int32_t from network endian
              ///
              ///Require : buf->readableBytes() >= sizeof(int32_t)
              int32_t peekInt32() const 
              {
                assert(readableBytes() >= sizeof(int32_t));
                int32_t be32 = 0;
                ::memcpy(&be32, peek(), sizeof(be32));
                return sockets::networkToHost32(be32);
              }

              ///
              ///Peek int16_t from network endian
              ///
              ///Require : buf->readableBytes() >= sizeof(int16_t)
              int16_t peekInt16() const 
              {
                assert(readableBytes() >= sizeof(int16_t));
                int16_t be16 = 0;
                ::memcpy(&be16, peek(), sizeof(be16));
                return sockets::networkToHost16(be16);
              }

              ///
              ///Peek int8_t from network endian
              ///
              ///Require : buf->readableBytes() >= sizeof(int8_t)
              int16_t peekInt8() const 
              {
                assert(readableBytes() >= sizeof(int8_t));
                int8_t x = *peek();
                return x;
              }

              ///
              ///Prepend int32_t using network endian
              ///
              void prependInt32(int32_t x)
              {
                int32_t be32 = sockets::hostToNetwork32(x);
                prepend(&be32,sizeof(be32));
              }

               ///
              ///Prepend int16_t using network endian
              ///
              void prependInt16(int16_t x)
              {
                int32_t be16 = sockets::hostToNetwork16(x);
                prepend(&be16,sizeof(be16));
              }

               ///
              ///Prepend int8_t using network endian
              ///
              void prependInt8(int8_t x)
              {
                prepend(&x,sizeof(x));
              }

              void prepend(const void* data, size_t len)
              {
                assert(len <= prependableBytes());
                M_readerIndex -= len;
                const char* d = static_cast<const char*>(data);
                std::copy(d, d+len, begin()+M_readerIndex);
              }

              void shrink(size_t reserve)
              {
                Buffer other;
                other.ensureWritableBytes(readableBytes() + reserve);
                other.append(toStringPiece());
                swap(other);
              }

              size_t internalCapacity() const 
              {
                return M_buffer.capacity();
              }

              ///Read data directly into buffer
              ///
              ///It may implement with readv(2)
              ///@return result of read(2), @c errno is saved
              ssize_t readFd(int fd, int* savedErrno);

    private:

        char* begin()
        {
          return &*M_buffer.begin();
        }

        const char* begin() const 
        {
          return &*M_buffer.begin();
        }

        void makeSpace(size_t len)
        {
          if(writableBytes() + prependableBytes() < len + kCheapPrepend)
          {
            buffer.resize(M_writerIndex+len);
          }
          else
          {
            assert(kCheapPrepend < M_readerIndex);
            size_t readable = readableBytes();
            std::copy(begin()+M_readerIndex, begin()+M_writerIndex, begin()+kCheapPrepend);
            M_readerIndex = kCheapPrepend;
            M_writerIndex = M_readerIndex + readable;
            assert(readable == readableBytes());
          }
        }

    private:
        std::vector<char> M_buffer;
        size_t M_readerIndex;
        size_t M_writerIndex;

        static const char kCRLF[];
};

#endif // BUFFER_H
