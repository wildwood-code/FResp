/*******************************************************************************
* Copyright © 2021 Kerry S. Martin, martin@wild-wood.net
* Free for usage without warranty, expressed or implied; attribution required
*
* Filename   : Oscilloscope.h
* Class      : Oscilloscope
* Description:
*   Implements an interface to a Siglent SDS 1000 X-E oscilloscope.
*
* Created    : 05/25/2020
* Modified   : 11/05/2021
* Author     : Kerry S. Martin, martin@wild-wood.net
*******************************************************************************/
#pragma once
#include "Socket_Instrument.h"


constexpr auto N_ATTENUATION_TABLES = 2;
constexpr auto N_VOLTAGE_PAIRS = 14;


class Oscilloscope :
	protected Socket_Instrument
{
public:
	// construction/destruction
	Oscilloscope();
	virtual ~Oscilloscope();

	// connection to an instrument
	virtual bool Attach(std::string resource);
	//virtual bool Attach(std::regex pattern);
	virtual bool Detach();

	// many setting types
	enum class Channel { CH1, CH2, CH3, CH4 };
	enum class VoltsPerDiv { UNSPEC, V_500uV, V_1mV, V_2mV, V_5mV, V_10mV, V_20mV, V_50mV, V_100mV, V_200mV, V_500mV, V_1V, V_2V, V_5V, V_10V, V_20V, V_50V, V_100V }; // 500uV only at 1x, 100V at 10x
	enum class BWLimit { UNSPEC, BWL_FULL, BWL_ON };
	enum class ChInvert { UNSPEC, INV_OFF, INV_ON };
	enum class Coupling { UNSPEC, DC, AC };
	enum class ChAtten { UNSPEC, AT_1X, AT_10X };
	enum class ChUnit { UNSPEC, V /*, A */ };  // for now, A is not supported TODO: support eventually
	enum class EdgeType { RISING, FALLING };
	enum class TriggerMode { STOP, AUTO, NORMAL, SINGLE };
	enum class MeasParam { PKPK, MAX, MIN, AMPL, TOP, BASE, CMEAN, MEAN, RMS, CRMS, OVSN, FPRE, OVSP, RPRE, PER, FREQ, PWID, NWID, RISE, FALL, WID, DUTY, NDUTY };
	enum class MeasDelParam { PHA, FRR, FRF, FFR, FFF, LRR, LRF, LFR, LFF, SKEW };
	enum class TimeDiv { UNSPEC, T_1nS, T_2nS, T_5nS, T_10nS, T_20nS, T_50nS, T_100nS, T_200nS, T_500nS, T_1uS, T_2uS, T_5uS, T_10uS, T_20uS, T_50uS, T_100uS, T_200uS, T_500uS, T_1mS, T_2mS, T_5mS, T_10mS, T_20mS, T_50mS, T_100mS, T_200mS, T_500mS, T_1S, T_2S, T_5S, T_10S, T_20S, T_50S, T_100S };
	struct ScaleValues { double max; double min; double pp; double offset; double vdiv; };

	// channel configuration
	bool SetChannelEx(Channel ch, bool enabled = true, VoltsPerDiv vdiv=VoltsPerDiv::UNSPEC, double offset=DEFAULT_PARAM, Coupling coup = Coupling::UNSPEC, BWLimit bwl=BWLimit::UNSPEC, ChAtten atten=ChAtten::UNSPEC, ChInvert inv=ChInvert::UNSPEC);
	bool SetChannelEnable(Channel ch, bool enabled);
	bool SetChannelVolts(Channel ch, VoltsPerDiv vdiv, double offset=DEFAULT_PARAM);
	bool SetChannelVoltsEx(Channel ch, double vdiv, double offset = DEFAULT_PARAM);
	bool SetChannelOffset(Channel ch, double offset);
	bool SetChannelBWL(Channel ch, BWLimit bwl);
	bool SetChannelInvert(Channel ch, ChInvert inv);
	bool SetChannelAtten(Channel ch, ChAtten atten);
	bool SetChannelCoupling(Channel ch, Coupling coup);
	bool SetChannelUnit(Channel ch, ChUnit unit);
	bool SetChannelSkew(Channel ch, double skew);
	int AdjustChannelVolts(Channel ch, int adjust);
	int AdjustChannelVolts(Channel ch, int adjust, ScaleValues& scale);

	// timebase configuration
	bool SetTimebase(TimeDiv tdiv, double delay=DEFAULT_PARAM);
	double SetTimebase(double tcapture, double delay = DEFAULT_PARAM);
	bool SetTimeDelay(double delay);

	// trigger configuration
	bool SetTriggerMode(TriggerMode mode);
	bool SetEdgeTrigger(Channel ch, EdgeType edge, double voltage, Coupling coup, bool holdoff=false, double tHoldoff = 0.0);

	// measurements
	double Measure(Channel ch, MeasParam param);
	double MeasureDelay(Channel ch1, Channel ch2, MeasDelParam param);

private:
	// helper functions
	void SetupOscilloscopeDefault();
	double ReadChannelAtten(Channel ch);
	static std::string GetChannelString(Channel ch);
	static Channel GetChannel(int i);

	struct VoltagePair { VoltsPerDiv vdiv; double volts; char str[6]; };
	static const VoltagePair VoltagePairs[N_ATTENUATION_TABLES][N_VOLTAGE_PAIRS];
	static const unsigned int nVoltagePairs;

	struct TimePair { TimeDiv tdiv; double sec; char str[6]; };
	static const TimePair TimePairs[];
	static const unsigned int nTimePairs;

	struct MeasPair { MeasParam par; char str[6]; };
	static const MeasPair MeasPairs[];
	static const unsigned int nMeasPairs;

	struct MeasDelPair { MeasDelParam par; char str[6]; };
	static const MeasDelPair MeasDelPairs[];
	static const unsigned int nMeasDelPairs;

	static const unsigned int nTimeDivisions;
	static const unsigned int nVoltageDivisions;
	static const double vUnscaledMin;
	static const double vUnscaledMax;
};


// shortcuts for oscope channels
constexpr auto OSCOPE_CH_1 = Oscilloscope::Channel::CH1;
constexpr auto OSCOPE_CH_2 = Oscilloscope::Channel::CH2;
constexpr auto OSCOPE_CH_3 = Oscilloscope::Channel::CH3;
constexpr auto OSCOPE_CH_4 = Oscilloscope::Channel::CH4;


/*******************************************************************************
* Copyright © 2021 Kerry S. Martin, martin@wild-wood.net
* Free for usage without warranty, expressed or implied; attribution required
*******************************************************************************/