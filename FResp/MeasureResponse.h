/*******************************************************************************
* Copyright © 2023 Kerry S. Martin, martin@wild-wood.net
* Free for usage without warranty, expressed or implied; attribution required
*
* Filename   : MeasureResponse.h
* Class      : None
* Description:
*   Entry point function to measure a frequency response.
*
*   Implements a measurement of frequency response using a Rigol function
*   generator and a Siglent oscilloscope.
*
* Created    : 07/03/2020
* Modified   : 01/02/2023
* Author     : Kerry S. Martin, martin@wild-wood.net
*******************************************************************************/
#pragma once
#include "FreqResp.h"

constexpr auto RETURN_SUCCESS = 0;
constexpr auto RETURN_NO_CONNECT_SINEGEN = -1;
constexpr auto RETURN_NO_CONNECT_OSCOPE = -2;
constexpr auto RETURN_ERROR = -3;
constexpr auto RETURN_FILE_WRITE_ERROR = -4;
constexpr auto RETURN_SYNTAX_ERROR = -5;
constexpr auto RETURN_SETUP_ERROR = -6;
constexpr auto RETURN_BLOCKED_WRITE_EXE_FILE = -7;
constexpr auto RETURN_UNKNOWN_ERROR = -8;
constexpr auto RETURN_RESOURCE_ERROR = -9;

// automated full-response interface
int MeasureResponse(int argc, char* argv[]);

// semi-automatic/incremental response interface
int MeasureResponseParse(int argc, char* argv[], File_Config& file,Freq_Config& freq, Stim_Config& stim, Channel_Config& input, Channel_Config& output, Trig_Config& trig,Meas_Config& meas, Dwell_Config& dwell,std::string& error);
int MeasureResponseAttach(char const* szOscope, char const* szSigGen, FreqResp& response, Freq_Config const& freq, Stim_Config const& stim, Channel_Config const& input, Channel_Config const& output, Trig_Config const& trig, Meas_Config const& meas, Dwell_Config const& dwell);
int MeasureResponseNext(FreqResp& response, FRS& result);
int MeasureResponseClose(FreqResp& response);


/*******************************************************************************
* Copyright © 2023 Kerry S. Martin, martin@wild-wood.net
* Free for usage without warranty, expressed or implied; attribution required
*******************************************************************************/