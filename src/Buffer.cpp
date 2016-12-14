#include "Buffer.h"
#include "SocketsOps.h"

#include <errno.h>
#include <sys/uio.h>

const char Buffer::kCRLF[] = "\r\n";

const size_t Buffer::kCheapPrepend;
const size_t Buffer::kInitialSize;

ssize_t Buffer::readFd(int fd, int *savedErrno)
{
    //saved an ioctl()/FIONREAD call to tell how much to read
    char extrabuf[65536];
    struct iovec vec[2];
    const size_t writable = writableBytes();
    vec[0].iov_base = begin() + M_writerIndex;
    vec[0].iov_len = writable;
    vec[1].iov_base = extrabuf;
    vec[1].iov_len = sizeof(extrabuf);

    const int iovcnt = (writable < sizeof(extrabuf)) ? 2 : 1;
    const ssize_t n = sockets::readv(fd, vec, iocnt);
    if(n < 0)
    {
        *savedErrno = errno;
    }
    else if(implicit_cast<size_t>(n) <= writable)
    {
        M_writerIndex += n;
    }
    else 
    {
        M_writerIndex = M_buffer.size();
        append(extrabuf, n - writable);
    }

    return n;
} 
