#include "Exception.h"

#include <execinfo.h>
#include <stdlib.h>

Exception::Exception(const char* msg)
    :M_message(msg)
{
    fillStackTrace();
}

Exception::Exception(const string& msg)
    :M_message(msg)
{
    fillStackTrace();
}

Exception::~Exception() throw()
{   
}

const char* Exception::what() const throw()
{
    return M_message.c_str();
}

const char* Exception::stackTrace() const throw()
{
    return M_stack.c_str();
}

void Exception::fillStackTrace()
{
    const int len = 200;
    void* buffer[len];
    int nptrs = ::backtrace(buffer, len);
    char **strings = ::backstrace_symbols(buffer, nptrs);
    if(strings)
    {
        for(int i = 0, i < nptrs; ++i)
        {
            M_stack.append(strings[i]);
            M_stack.push_back('\n');
        }
        free(strings);
    }
}