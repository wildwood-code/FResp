/*******************************************************************************
* Copyright © 2023 Kerry S. Martin, martin@wild-wood.net
* Free for usage without warranty, expressed or implied; attribution required
*
* Filename   : EchoDualStream.cpp
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
* Modified   : 01/01/2023
* Author     : Kerry S. Martin, martin@wild-wood.net
*******************************************************************************/
#include "EchoDualStream.h"

EchoDualStream::NullBuffer EchoDualStream::null_buffer;
std::ostream EchoDualStream::null_stream(&EchoDualStream::null_buffer);


/*******************************************************************************
* Copyright © 2023 Kerry S. Martin, martin@wild-wood.net
* Free for usage without warranty, expressed or implied; attribution required
*******************************************************************************/