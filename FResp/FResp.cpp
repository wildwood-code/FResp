/*******************************************************************************
* Copyright © 2021 Kerry S. Martin, martin@wild-wood.net
* Free for usage without warranty, expressed or implied; attribution required
*
* Filename   : MeasureResponse.cpp
* Description:
*   Entry point to measure the frequency response of a circuit using an
*   oscillosope and function generator.
*   The command-line passed to MeasureResponse() is parsed to get instrument settings and
*   the measurement is initiated using class FreqResp.
*
* Created    : 07/03/2020
* Modified   : 11/05/2021
* Author     : Kerry S. Martin, martin@wild-wood.net
*
* History    : Ver    Date         Notes
*              0.00    2020-05-29  Initial release as main()
*              0.01    2020-06-21  Fixed cend() bug, added delay/phase
*              1.00    2020-07-03  Implemented as MeasureResponse()
*              2.00    2021-11-05  Changed from VISA to Winsock
*              2.01    2021-11-11  Fixed filename parsing
*******************************************************************************/


#include "MeasureResponse.h"
#include "Oscilloscope.h"


int main(int argc, char* argv[])
{
    return MeasureResponse(argc, argv);
}


/*******************************************************************************
* Copyright © 2021 Kerry S. Martin, martin@wild-wood.net
* Free for usage without warranty, expressed or implied; attribution required
*******************************************************************************/