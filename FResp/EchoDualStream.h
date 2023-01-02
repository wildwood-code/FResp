/*******************************************************************************
* Copyright © 2023 Kerry S. Martin, martin@wild-wood.net
* Free for usage without warranty, expressed or implied; attribution required
*
* Filename   : EchoDualStream.h
* Class      : EchoDualStream
* Description:
*   EchoDualStream is a class that allows streaming one text to two different
*   ostreams with one << operation. For instance, one ostream can be the console
*   and one can be a file, allowing echo to the console while saving in a file.
*
*   A null_stream is included if one (or both) of the two streams are not
*   needed.
*
* Created    : 05/26/2020
* Modified   : 01/02/2023
* Author     : Kerry S. Martin, martin@wild-wood.net
*******************************************************************************/
#pragma once
#include <ostream>
#include <string>

class EchoDualStream
{
private:

    class NullBuffer : public std::streambuf
    {
    public:
        int overflow(int c) { return c; }
    };
    static NullBuffer null_buffer;
    std::ostream& os1;
    std::ostream& os2;

public:

    static std::ostream null_stream;

    EchoDualStream(std::ostream& os1, std::ostream& os2) : os1(os1), os2(os2) {}

    template<class T>
    EchoDualStream& operator<<(const T& x)
    {
        os1 << x;
        os2 << x;
        return *this;
    }
};


/*******************************************************************************
* Copyright © 2023 Kerry S. Martin, martin@wild-wood.net
* Free for usage without warranty, expressed or implied; attribution required
*******************************************************************************/