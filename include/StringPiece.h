#ifndef STRINGPIECE_H
#define STRINGPIECE_H


#include <string>

class StringArg
{
    public：
        StringArg(const char *str)
            :M_str(str)
        { }

        StringArg(const std::string& str)
            :M_str(str.c_str())
        { }

        const char *c_str() const
        {
            return M_str;
        }

    private:
        const char *M_str;
};


#endif // STRINGPIECE_H
