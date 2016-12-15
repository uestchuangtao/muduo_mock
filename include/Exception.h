#ifndef EXCEPTION_H
#define EXCEPTION_H

#include "Types.h"
#include <exception>

class Exception : public std::exception
{
public:
    explicit Exception(const char* what);
    explicit Exception(const string& what);
    virtual ~Exception() throw();
    virtual const char* what() const throw();
    const char* stackTrace() const throw();
    
private:
    void fillStackTrace();

    string M_message;
    string M_stack;
};

#endif