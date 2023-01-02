/*******************************************************************************
* Copyright © 2023 Kerry S. Martin, martin@wild-wood.net
* Free for usage without warranty, expressed or implied; attribution required
*
* Filename   : FreqResp.h
* Class      : FreqResp
* Description:
*   FreqResp is a class that implements frequency response measurement either
*   as a full sweep, or as a series of calls or each frequency step.
*
*   Implements a measurement of frequency response using a Rigol function
*   generator and a Siglent oscilloscope.
*
* Created    : 05/26/2020
* Modified   : 01/01/2023
* Author     : Kerry S. Martin, martin@wild-wood.net
*******************************************************************************/
#pragma once
#include "Oscilloscope.h"
#include "SineGenerator.h"
#include <vector>

enum class Sweep_t { LOG, LIN };
enum class Vtype_t { VPP, VPK };
enum class Ttype_t { PHASE, DELAY };
enum class Ctype_t { DC, AC };
enum class Etype_t { RISE, FALL };
enum class TUNIT { PHASE, DELAY };

struct File_Config
{
	bool is_echo;
	std::string filename;
};


struct Freq_Config
{
	double fStart;
	double fStop;
	Sweep_t sweep;
	unsigned int Npoints;
};

struct Stim_Config
{
	unsigned int ch;
	Vtype_t vtStim;
	double vstim;
	double vdc;
};

struct Channel_Config
{
	unsigned int ch;
	Ctype_t coup;
	double atten;
	bool bwl;
};

struct Trig_Config
{
	int ch;
	Etype_t edge;
	Ctype_t coup;
	double vTrig;
};

struct Meas_Config
{
	Vtype_t vtMeas;
	Ttype_t ttMeas;
};

struct Dwell_Config
{
	double stable_screens; // number of stable full-captures 
	unsigned long  minDwell_msec;
};

class FRS
{
public:
	double freq;
	double mag_in;
	double mag_out;
	double dBgain;
	double time;
	TUNIT tunit;
};

typedef std::vector<FRS> FRST;

// value will be < 0 for any failure, and >= 0 for any success
typedef signed int FRRET;
constexpr auto FRRET_SUCCESS = 0;
constexpr auto FRRET_COMPLETE = 1;
constexpr auto FRRET_NOT_INITIALIZED = -1;
constexpr auto FRRET_ALREADY_INITIALIZED = -2;
constexpr auto FRRET_INVALID_FREQUENCY = -3;
constexpr auto FRRET_INVALID_STIM = -4;
constexpr auto FRRET_INVALID_TRIG = -5;
constexpr auto FRRET_INIT_OSCILLOSCOPE = -10;
constexpr auto FRRET_INIT_SINEGEN = -11;


class FreqResp
{
public:
	FreqResp();
	~FreqResp();
	FreqResp(FreqResp const&) = delete;
	FreqResp& operator = (FreqResp const&) = delete;
	operator FRST const& () const;

	FRRET Init(char const* szOscope, char const* szSigGen, Freq_Config const& freq, Stim_Config const& stim, Channel_Config const& input, Channel_Config const& output, Trig_Config const& trig, Meas_Config const& meas, Dwell_Config const& dwell);
	FRRET MeasureNext(FRS& result);
	FRRET Sweep();
	FRRET Close();

private:
	// status indicators
	bool initialized;
	bool completed;

	// frequency response data
	FRST data;

	// parameters supplied to init
	Freq_Config freq;
	Stim_Config stim;
	Channel_Config input;
	Channel_Config output;
	Trig_Config trig;
	Meas_Config meas;
	Dwell_Config dwell;

	// instruments
	SineGenerator stimulus;
	Oscilloscope oscope;

	// algorithm variables
	double f;
	SineGenerator::Channel sgChannel;
	Oscilloscope::Channel osChannelInput;
	Oscilloscope::Channel osChannelOutput;
	Oscilloscope::Channel osChannelTrig;
	Oscilloscope::MeasParam mpMeasure;
	Oscilloscope::MeasDelParam measEdge;
	double avMeasure;
	double vStim;
	TUNIT tunit;
	Oscilloscope::ScaleValues osScaleOutput;
	Oscilloscope::ScaleValues osScaleInput;

	// constant settings
	static const double SEEK_MAX;
	static const double SEEK_MID;
	static const double SEEK_MIN;
	static const double SEEK_MARGIN;
	static const double FREQ_FUDGE;
	static const double MEAS_CYCLES;

private:
	FRRET MeasureFreq(double f, FRS& result);
	static double MeasureAndScaleInput(Oscilloscope& oscope, Oscilloscope::Channel ch, Oscilloscope::MeasParam mpMeasure, Oscilloscope::ScaleValues& scale, int& adjust);
};


/*******************************************************************************
* Copyright © 2023 Kerry S. Martin, martin@wild-wood.net
* Free for usage without warranty, expressed or implied; attribution required
*******************************************************************************/