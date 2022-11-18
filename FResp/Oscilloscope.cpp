/*******************************************************************************
* Copyright © 2021 Kerry S. Martin, martin@wild-wood.net
* Free for usage without warranty, expressed or implied; attribution required
*
* Filename   : Oscilloscope.cpp
* Class      : Oscilloscope
* Description:
*   Implements an interface to a Siglent SDS 1000 X-E oscilloscope.
*
* Created    : 05/25/2020
* Modified   : 11/05/2021
* Author     : Kerry S. Martin, martin@wild-wood.net
*******************************************************************************/

#include <regex>
#include <limits>
#include "Oscilloscope.h"
using namespace std;


/*******************************************************************************
* Class      : Oscilloscope
* Member     : VoltagePairs[][] table
* Access     : private static constant
* Arguments  : n/a
* Returns    : n/a
* Description:
*   Table of voltage settings and the corresponding v/div and string
*   and enhance the functions that use it to work with other attenuations)
*/
constexpr auto PAIR_TABLE_1X = 0;
constexpr auto PAIR_TABLE_10X = 1;
const Oscilloscope::VoltagePair Oscilloscope::VoltagePairs[N_ATTENUATION_TABLES][N_VOLTAGE_PAIRS]
{
	{
		// 1x attenuation pairs
		{ VoltsPerDiv::V_500uV,	5.00E-04, "500UV" },
		{ VoltsPerDiv::V_1mV,	1.00E-03, "1MV" },
		{ VoltsPerDiv::V_2mV,	2.00E-03, "2MV" },
		{ VoltsPerDiv::V_5mV,	5.00E-03, "5MV" },
		{ VoltsPerDiv::V_10mV,	1.00E-02, "10MV" },
		{ VoltsPerDiv::V_20mV,	2.00E-02, "20MV" },
		{ VoltsPerDiv::V_50mV,	5.00E-02, "50MV" },
		{ VoltsPerDiv::V_100mV,	1.00E-01, "100MV" },
		{ VoltsPerDiv::V_200mV,	2.00E-01, "200MV" },
		{ VoltsPerDiv::V_500mV,	5.00E-01, "500MV" },
		{ VoltsPerDiv::V_1V,	1.00E+00, "1V" },
		{ VoltsPerDiv::V_2V,	2.00E+00, "2V" },
		{ VoltsPerDiv::V_5V,	5.00E+00, "5V" },
		{ VoltsPerDiv::V_10V,	1.00E+01, "10V" }
	},
	{
		// 10x attenuation pairs
		{ VoltsPerDiv::V_5mV,	5.00E-03, "5MV" },
		{ VoltsPerDiv::V_10mV,	1.00E-02, "10MV" },
		{ VoltsPerDiv::V_20mV,	2.00E-02, "20MV" },
		{ VoltsPerDiv::V_50mV,	5.00E-02, "50MV" },
		{ VoltsPerDiv::V_100mV,	1.00E-01, "100MV" },
		{ VoltsPerDiv::V_200mV,	2.00E-01, "200MV" },
		{ VoltsPerDiv::V_500mV,	5.00E-01, "500MV" },
		{ VoltsPerDiv::V_1V,	1.00E+00, "1V" },
		{ VoltsPerDiv::V_2V,	2.00E+00, "2V" },
		{ VoltsPerDiv::V_5V,	5.00E+00, "5V" },
		{ VoltsPerDiv::V_10V,	1.00E+01, "10V" },
		{ VoltsPerDiv::V_20V,	2.00E+01, "20V" },
		{ VoltsPerDiv::V_50V,	5.00E+01, "50V" },
		{ VoltsPerDiv::V_100V,	1.00E+02, "100V" }
	}
};


/*******************************************************************************
* Class      : Oscilloscope
* Member     : nVoltagePairs constant
* Access     : private static constant
* Arguments  : n/a
* Returns    : n/a
* Description:
*   Number of entries in the VoltagePairs[] table
*/
const unsigned int Oscilloscope::nVoltagePairs{ N_VOLTAGE_PAIRS };


/*******************************************************************************
* Class      : Oscilloscope
* Member     : TimePairs[] table
* Access     : private static constant
* Arguments  : n/a
* Returns    : n/a
* Description:
*   Table of time settings and corresponding time/div and string
*/
const Oscilloscope::TimePair Oscilloscope::TimePairs[]
{
	{ TimeDiv::T_1nS,	1.00E-09, "1NS" },
	{ TimeDiv::T_2nS,	2.00E-09, "2NS" },
	{ TimeDiv::T_5nS,	5.00E-09, "5NS" },
	{ TimeDiv::T_10nS,	1.00E-08, "10NS" },
	{ TimeDiv::T_20nS,	2.00E-08, "20NS" },
	{ TimeDiv::T_50nS,	5.00E-08, "50NS" },
	{ TimeDiv::T_100nS,	1.00E-07, "100NS" },
	{ TimeDiv::T_200nS,	2.00E-07, "200NS" },
	{ TimeDiv::T_500nS,	5.00E-07, "500NS" },
	{ TimeDiv::T_1uS,	1.00E-06, "1US" },
	{ TimeDiv::T_2uS,	2.00E-06, "2US" },
	{ TimeDiv::T_5uS,	5.00E-06, "5US" },
	{ TimeDiv::T_10uS,	1.00E-05, "10US" },
	{ TimeDiv::T_20uS,	2.00E-05, "20US" },
	{ TimeDiv::T_50uS,	5.00E-05, "50US" },
	{ TimeDiv::T_100uS,	1.00E-04, "100US" },
	{ TimeDiv::T_200uS,	2.00E-04, "200US" },
	{ TimeDiv::T_500uS,	5.00E-04, "500US" },
	{ TimeDiv::T_1mS,	1.00E-03, "1MS" },
	{ TimeDiv::T_2mS,	2.00E-03, "2MS" },
	{ TimeDiv::T_5mS,	5.00E-03, "5MS" },
	{ TimeDiv::T_10mS,	1.00E-02, "10MS" },
	{ TimeDiv::T_20mS,	2.00E-02, "20MS" },
	{ TimeDiv::T_50mS,	5.00E-02, "50MS" },
	{ TimeDiv::T_100mS,	1.00E-01, "100MS" },
	{ TimeDiv::T_200mS,	2.00E-01, "200MS" },
	{ TimeDiv::T_500mS,	5.00E-01, "500MS" },
	{ TimeDiv::T_1S,	1.00E+00, "1S" },
	{ TimeDiv::T_2S,	2.00E+00, "2S" },
	{ TimeDiv::T_5S,	5.00E+00, "5S" },
	{ TimeDiv::T_10S,	1.00E+01, "10S" },
	{ TimeDiv::T_20S,	2.00E+01, "20S" },
	{ TimeDiv::T_50S,	5.00E+01, "50S" },
	{ TimeDiv::T_100S,	1.00E+02, "100S" }
};


/*******************************************************************************
* Class      : Oscilloscope
* Member     : nTimePairs constant
* Access     : private static constant
* Arguments  : n/a
* Returns    : n/a
* Description:
*   Number of entries in the TimePairs[] table
*/
const unsigned int Oscilloscope::nTimePairs{ sizeof(TimePairs) / sizeof(TimePair) };


/*******************************************************************************
* Class      : Oscilloscope
* Member     : nTimeDivisions constant
* Access     : private static constant
* Arguments  : n/a
* Returns    : n/a
* Description:
*   Number of time divisions across the screen
*/
const unsigned int Oscilloscope::nTimeDivisions{ 14 };


/*******************************************************************************
* Class      : Oscilloscope
* Member     : nVoltageDivisions constant
* Access     : private static constant
* Arguments  : n/a
* Returns    : n/a
* Description:
*   Number of voltage divisions across the screen
*/
const unsigned int Oscilloscope::nVoltageDivisions{ 8 };


/*******************************************************************************
* Class      : Oscilloscope
* Member     : vUnscaledMin and vUnscaledMax constants
* Access     : private static constant
* Arguments  : n/a
* Returns    : n/a
* Description:
*   The smallest (min) and largest (max) volts/div settings at 1x (unscaled) input
*/
const double Oscilloscope::vUnscaledMin{ VoltagePairs[PAIR_TABLE_1X][0].volts };
const double Oscilloscope::vUnscaledMax{ VoltagePairs[PAIR_TABLE_1X][N_VOLTAGE_PAIRS-1].volts };


/*******************************************************************************
* Class      : Oscilloscope
* Member     : MeasPairs[] table
* Access     : private static constant
* Arguments  : n/a
* Returns    : n/a
* Description:
*   Table of measurement settings and corresponding strings
*/
const Oscilloscope::MeasPair Oscilloscope::MeasPairs[]
{
	{ MeasParam::PKPK,	"PKPK"	},
	{ MeasParam::MAX,	"MAX"	},
	{ MeasParam::MIN,	"MIN"	},
	{ MeasParam::AMPL,	"AMPL"	},
	{ MeasParam::TOP,	"TOP"	},
	{ MeasParam::BASE,	"BASE"	},
	{ MeasParam::CMEAN,	"CMEAN"	},
	{ MeasParam::MEAN,	"MEAN"	},
	{ MeasParam::RMS,	"RMS"	},
	{ MeasParam::CRMS,	"CRMS"	},
	{ MeasParam::OVSN,	"OVSN"	},
	{ MeasParam::FPRE,	"FPRE"	},
	{ MeasParam::OVSP,	"OVSP"	},
	{ MeasParam::RPRE,	"RPRE"	},
	{ MeasParam::PER,	"PER"	},
	{ MeasParam::FREQ,	"FREQ"	},
	{ MeasParam::PWID,	"PWID"	},
	{ MeasParam::NWID,	"NWID"	},
	{ MeasParam::RISE,	"RISE"	},
	{ MeasParam::FALL,	"FALL"	},
	{ MeasParam::WID,	"WID"	},
	{ MeasParam::DUTY,	"DUTY"	},
	{ MeasParam::NDUTY,	"NDUTY"	}
};


/*******************************************************************************
* Class      : Oscilloscope
* Member     : nMeasPairs constant
* Access     : private static constant
* Arguments  : n/a
* Returns    : n/a
* Description:
*   Number of entries in the MeasPairs[] table
*/
const unsigned int Oscilloscope::nMeasPairs{ sizeof(MeasPairs) / sizeof(MeasPair) };


/*******************************************************************************
* Class      : Oscilloscope
* Member     : MeasDelPairs[] table
* Access     : private static constant
* Arguments  : n/a
* Returns    : n/a
* Description:
*   Table of measurement delay settings and corresponding strings
*/
const Oscilloscope::MeasDelPair Oscilloscope::MeasDelPairs[]
{
	{ MeasDelParam::PHA,	"PHA" },
	{ MeasDelParam::FRR,	"FRR" },
	{ MeasDelParam::FRF,	"FRF" },
	{ MeasDelParam::FFR,	"FFR" },
	{ MeasDelParam::FFF,	"FFF" },
	{ MeasDelParam::LRR,	"LRR" },
	{ MeasDelParam::LRF,	"LRF" },
	{ MeasDelParam::LFR,	"LFR" },
	{ MeasDelParam::LFF,	"LFF" },
	{ MeasDelParam::SKEW,	"SKEW" }
};


/*******************************************************************************
* Class      : Oscilloscope
* Member     : nMeasDelPairs constant
* Access     : private static constant
* Arguments  : n/a
* Returns    : n/a
* Description:
*   Number of entries in the MeasDelPairs[] table
*/
const unsigned int Oscilloscope::nMeasDelPairs{ sizeof(MeasDelPairs) / sizeof(MeasDelPair) };


/*******************************************************************************
* Class      : Oscilloscope
* Function   : GetChannel()
* Access     : private static
* Arguments  : i = channel # (1-4)
* Returns    : Channel enum for the designated channel #
* Description:
*   Get the channel designator for the given channel # (integer)
*/
Oscilloscope::Channel Oscilloscope::GetChannel(int i)
{
	switch (i)
	{
	default: return Channel::CH1;
	case 1:  return Channel::CH1;
	case 2:  return Channel::CH2;
	case 3:  return Channel::CH3;
	case 4:  return Channel::CH4;
	}
}


/*******************************************************************************
* Class      : Oscilloscope
* Function   : GetChannelString()
* Access     : private static
* Arguments  : ch = Channel enum
* Returns    : string designation of channel
* Description:
*   Get the string designation for the given channel
*/
string Oscilloscope::GetChannelString(Channel ch)
{
	switch (ch)
	{
	case Channel::CH1: default:
		return string("C1");
	case Channel::CH2:
		return string("C2");
	case Channel::CH3:
		return string("C3");
	case Channel::CH4:
		return string("C4");
	}
}


/*******************************************************************************
* Class      : Oscilloscope
* Function   : Oscilloscope() constructor
* Access     : public
* Arguments  : none
* Returns    : none
* Description:
*   Constructs a default (attached to no instrument) Oscilloscope object
*/
Oscilloscope::Oscilloscope()
	: Socket_Instrument()
{
}


/*******************************************************************************
* Class      : Oscilloscope
* Function   : ~Oscilloscope() destructor
* Access     : public
* Arguments  : none
* Returns    : none
* Description:
*   Destroys a Oscilloscope object, first detaching any attached instruments
*/
Oscilloscope::~Oscilloscope()
{
}


/*******************************************************************************
* Class      : Oscilloscope
* Function   : Attach()
* Access     : public
* Arguments  : resource = resource identifier string for instrument
* Returns    : true if successful (instrument was attached), false if not
* Description:
*   Attaches to an instrument using the given resource name
*/
bool Oscilloscope::Attach(std::string resource)
{
	bool bResult = false;
	if (Socket_Instrument::Attach(resource))
	{
		SetupOscilloscopeDefault();
		bResult = true;
	}

	return bResult;
}


/*******************************************************************************
* Class      : Oscilloscope
* Function   : Detach()
* Access     : public
* Arguments  : none
* Returns    : true if successful (instrument was detached), false if not
* Description:
*   Detaches from the instrument
*/
bool Oscilloscope::Detach()
{
	return Socket_Instrument::Detach();
}


/*******************************************************************************
* Class      : Oscilloscope
* Function   : SetupOscilloscopeDefault()
* Access     : private
* Arguments  : none
* Returns    : none
* Description:
*   Puts the oscilloscope in a default instrument state
*/
void Oscilloscope::SetupOscilloscopeDefault()
{
	// response format
	Write("COMM_HEADER SHORT");

	// acquire
	Write("ACQUIRE_WAY SAMPLING");
	Write("MEMORY_SIZE 14M");
	Write("SINXX_SAMPLE ON");

	// display
	Write("XY_DISPLAY OFF");
	Write("DTJN OFF"); // vectors
	Write("PESU OFF"); // persistence off
	Write("MENU OFF");

	// zoom off
	// TODO: oscope SCPI command set currently does not support changing zoom on/off (fix this if the support is added)

	// cursors
	Write("CRMS OFF"); // cursors off

	// history
	Write("HSMD OFF"); // history off

	// decode
	Write("DCST OFF"); // decode off

	// digital
	Write("DI:SWITCH OFF");

	// math
	Write("MATH:TRACE OFF");

	// measure
	Write("MEASURE_CLEAR");

	// reference
	Write("REF_CLOSE"); // turn reference off

	// timebase
	SetTimebase(TimeDiv::T_1mS, 0.0);

	// vertical (channel 1 and channel 2 on)
	for (int i = 1; i <= 4; ++i)
	{
		const Channel ch = GetChannel(i);
		SetChannelEx(ch, false, VoltsPerDiv::V_1V, 0.0, Coupling::DC, BWLimit::BWL_FULL, ChAtten::AT_10X, ChInvert::INV_OFF);
		SetChannelUnit(ch, ChUnit::V);
		SetChannelSkew(ch, 0.0);
		if (i > 2)
		{
			// turn off channels 3 and 4
			SetChannelEnable(ch, false);
		}
	}

	// trigger = Auto, DC, Edge-triggered, CH1 rising @ 0V, no holdoff
	SetEdgeTrigger(Channel::CH1, EdgeType::RISING, 0.0, Coupling::DC, false, 0.0);
	SetTriggerMode(TriggerMode::AUTO);
}


/*******************************************************************************
* Class      : Oscilloscope
* Function   : SetTriggerMode()
* Access     : public
* Arguments  : mode = trigger mode to set (stop, auto, norm, single)
* Returns    : true if successful, false otherwise
* Description:
*   Sets the trigger mode of the oscilloscope
*/
bool Oscilloscope::SetTriggerMode(TriggerMode mode)
{
	bool bResult;
	string strTrigMode;

	switch (mode)
	{
	case TriggerMode::STOP:
		strTrigMode = "STOP";
		break;
	case TriggerMode::NORMAL:
		strTrigMode = "NORM";
		break;
	case TriggerMode::AUTO: default:
		strTrigMode = "AUTO";
		break;
	case TriggerMode::SINGLE:
		strTrigMode = "SINGLE";
		break;
	}

	bResult = Write("TRMD " + strTrigMode);

	return bResult;
}


/*******************************************************************************
* Class      : Oscilloscope
* Function   : SetEdgeTrigger()
* Access     : public
* Arguments  : ch       = trigger channel
*              edge     = trigger edge (rising or falling)
*              voltage  = trigger voltage (Volts) {DEFAULT_PARAM=>no change}
*              coup     = trigger coupling (AC|DC)
*              holdoff  = holdoff mode: false==close, true=holdoff
*              tHoldoff = holdoff time (seconds), used if holdoff==true, unused otherwise
* Returns    : true if successful, false otherwise
* Description:
*   Sets the trigger mode of the oscilloscope
*/
bool Oscilloscope::SetEdgeTrigger(Channel ch, EdgeType edge, double voltage, Coupling coup, bool holdoff, double tHoldoff)
{
	bool bResult = true;
	const string strCh = GetChannelString(ch);
	string strCoup;
	string strHoldoff;
	string strHoldValue;
	string strEdge;
	string strResponse;  // response from query
	double attn = 1.0;

	// trigger voltage is at 1x probe attenuation
	// read the probe attenuation and scale accordingly
	if (Query(strCh + ":ATTN?", strResponse))
	{
		// format should be C#:ATT(eNuation) xxxx\n
		smatch smMatch;
		const regex reAttn("^C[1-4]\\:ATT[A-Z]+ (.+)\n$", regex::icase);

		if (regex_match(strResponse, smMatch, reAttn))
		{
			attn = stod(smMatch[1]);
		}
		else
		{
			bResult = false;
		}
	}
	else
	{
		bResult = false;
	}

	if (bResult)
	{
		switch (coup)
		{
		case Coupling::AC: default:
			strCoup = "AC";
			break;
		case Coupling::DC:
			strCoup = "DC";
			break;
		}

		switch (holdoff)
		{
		case true:
			strHoldoff = "ON";
			strHoldValue = to_string(tHoldoff * 1.0e9) + "NS";
			break;
		case false:
			strHoldoff = "OFF";
			strHoldValue = "80NS";
			break;
		}

		switch (edge)
		{
		case EdgeType::RISING: default:
			strEdge = "POS";
			break;
		case EdgeType::FALLING:
			strEdge = "NEG";
			break;
		}

		bResult = Write("TRCP " + strCoup);
		if (bResult)
			bResult = Write(strCh + ":TRLV " + to_string(voltage / attn) + "V");
		if (bResult)
			bResult = Write("TRSE EDGE, SR, " + strCh + ", HT, " + strHoldoff + ", HV, " + strHoldValue);
		if (bResult)
			bResult = Write(strCh + ":TRSL " + strEdge);
	}

	return bResult;
}


/*******************************************************************************
* Class      : Oscilloscope
* Function   : SetChannelEnable()
* Access     : public
* Arguments  : ch       = channel
*              enabled  = true (enabled/visible) or false (disabled/not visible)
* Returns    : true if successful, false otherwise
* Description:
*   Sets the visibility of the given channel.
*/
bool Oscilloscope::SetChannelEnable(Channel ch, bool enabled)
{
	bool bResult = false;
	const string strCh = GetChannelString(ch);

	if (enabled)
		bResult = Write(strCh + ":TRACE ON");
	else
		bResult = Write(strCh + ":TRACE OFF");

	return bResult;
}


/*******************************************************************************
* Class      : Oscilloscope
* Function   : SetChannelVolts()
* Access     : public
* Arguments  : ch       = channel
*              vdiv     = VoltsPerDiv enum
*              offset   = offset voltage
* Returns    : true if successful, false otherwise
* Description:
*   Sets the voltage scale and offset for the given channel to the VoltsPerDiv
*   setting given.
*/
bool Oscilloscope::SetChannelVolts(Channel ch, VoltsPerDiv vdiv, double offset)
{
	const string strCh = GetChannelString(ch);

	bool bResult = true;

	const double vAtten = ReadChannelAtten(ch);

	if (vAtten == 1.0 || vAtten == 10.0)
	{
		const int iPairTable = (vAtten == 1.0) ? PAIR_TABLE_1X : PAIR_TABLE_10X;

		if (bResult)
		{
			bResult = false;

			for (unsigned int i = 0; i < nVoltagePairs; ++i)
			{
				if (vdiv == VoltagePairs[iPairTable][i].vdiv)
				{
					bResult = Write(strCh + ":VDIV " + string(VoltagePairs[iPairTable][i].str));
					break;
				}
			}
		}

		if (bResult)
			bResult = SetChannelOffset(ch, offset);
	}
	else
	{
		bResult = false;
	}

	return bResult;
}


/*******************************************************************************
* Class      : Oscilloscope
* Function   : SetChannelVoltsEx()
* Access     : public
* Arguments  : ch       = channel
*              vdiv     = volts/division (at current probe attenuation)
*              offset   = offset voltage
* Returns    : true if successful, false otherwise
* Description:
*   Sets the voltage scale and offset for the given channel exactly as given.
*/
bool Oscilloscope::SetChannelVoltsEx(Channel ch, double vdiv, double offset)
{
	bool bResult = false;
	const string strCh = GetChannelString(ch);
	const double unscaled = vdiv/ReadChannelAtten(ch);

	// check that vdiv makes sense... positive and results within oscilloscope input range
	if (vdiv > 0.0 && unscaled>=vUnscaledMin && unscaled<=vUnscaledMax)
	{
		bResult = Write(strCh + ":VDIV " + to_string(vdiv));

		if (bResult)
			bResult = SetChannelOffset(ch, offset);
	}

	return bResult;
}


/*******************************************************************************
* Class      : Oscilloscope
* Function   : SetChannelOffset()
* Access     : public
* Arguments  : ch       = channel
*              offset   = offset voltage
* Returns    : true if successful, false otherwise
* Description:
*   Sets the offset voltage for the given channel.
*/
bool Oscilloscope::SetChannelOffset(Channel ch, double offset)
{
	bool bResult = false;
	const string strCh = GetChannelString(ch);

	if (!isnan(offset))
		bResult = Write(strCh + ":OFST " + to_string(offset) + "V");
	else
		bResult = false;

	return bResult;
}


/*******************************************************************************
* Class      : Oscilloscope
* Function   : SetChannelBWL()
* Access     : public
* Arguments  : ch       = channel
*              bwl      = bandwidth limit (BWL_FULL or BWL_ON)
* Returns    : true if successful, false otherwise
* Description:
*   Sets the bandwidth limit for the given channel.
*/
bool Oscilloscope::SetChannelBWL(Channel ch, BWLimit bwl)
{
	bool bResult = false;
	const string strCh = GetChannelString(ch);

	switch (bwl)
	{
	case BWLimit::BWL_FULL:
		bResult = Write(strCh + ":BWL OFF");
		break;
	case BWLimit::BWL_ON:
		bResult = Write(strCh + ":BWL ON");
		break;
	}
	return bResult;
}


/*******************************************************************************
* Class      : Oscilloscope
* Function   : SetChannelInvert()
* Access     : public
* Arguments  : ch       = channel
*              inv      = channel inversion (INV_OFF or INV_ON)
* Returns    : true if successful, false otherwise
* Description:
*   Sets the inversion for the given channel.
*/
bool Oscilloscope::SetChannelInvert(Channel ch, ChInvert inv)
{
	bool bResult = false;
	const string strCh = GetChannelString(ch);

	switch (inv)
	{
	case ChInvert::INV_OFF:
		bResult = Write(strCh + ":INVS OFF");
		break;
	case ChInvert::INV_ON:
		bResult = Write(strCh + ":INVS ON");
		break;
	}

	return bResult;
}


/*******************************************************************************
* Class      : Oscilloscope
* Function   : SetChannelAtten()
* Access     : public
* Arguments  : ch       = channel
*              atten    = attenuation (AT_10X or AT_1X)
* Returns    : true if successful, false otherwise
* Description:
*   Sets the attenuation for the given channel. Only 10x and 1x are currently supported
*/
bool Oscilloscope::SetChannelAtten(Channel ch, ChAtten atten)
{
	bool bResult = false;
	const string strCh = GetChannelString(ch);

	switch (atten)
	{
	case ChAtten::AT_10X:
		bResult = Write(strCh + ":ATTN 10");
		break;
	case ChAtten::AT_1X:
		bResult = Write(strCh + ":ATTN 1");
		break;
	}

	return bResult;
}


/*******************************************************************************
* Class      : Oscilloscope
* Function   : SetChannelCoupling()
* Access     : public
* Arguments  : ch       = channel
*              coup     = coupling (DC or AC)
* Returns    : true if successful, false otherwise
* Description:
*   Sets the coupling mode for the given channel.
*/
bool Oscilloscope::SetChannelCoupling(Channel ch, Coupling coup)
{
	bool bResult = false;
	const string strCh = GetChannelString(ch);

	switch (coup)
	{
	case Coupling::DC:
		bResult = Write(strCh + ":CPL D1M");
		break;
	case Coupling::AC:
		bResult = Write(strCh + ":CPL A1M");
		break;
	}

	return bResult;
}


/*******************************************************************************
* Class      : Oscilloscope
* Function   : SetChannelUnit()
* Access     : public
* Arguments  : ch       = channel
*              unit     = vertical unit (V or A)
* Returns    : true if successful, false otherwise
* Description:
*   Sets the vertical scale unit for the given channel.
*/
bool Oscilloscope::SetChannelUnit(Channel ch, ChUnit unit)
{
	bool bResult = false;
	const string strCh = GetChannelString(ch);

	switch (unit)
	{
	case ChUnit::V:
		bResult = Write(strCh + ":UNIT V");
		break;
		//case ChUnit::A:  // TODO: implement A eventually
		//	Write(strCh + ":UNIT A");
		//	break;
	}

	return bResult;
}


/*******************************************************************************
* Class      : Oscilloscope
* Function   : SetChannelSkew()
* Access     : public
* Arguments  : ch       = channel
*              skew     = channel skew (seconds)
* Returns    : true if successful, false otherwise
* Description:
*   Sets the timing skew for the given channel.
*/
bool Oscilloscope::SetChannelSkew(Channel ch, double skew)
{
	bool bResult = true;
	const string strCh = GetChannelString(ch);

	if (!isnan(skew))
	{
		if (skew >= -100.0e-9 && skew <= 100.0e-9)
			bResult = Write(strCh + ":SKEW " + to_string(skew));
		else
			bResult = false;
	}

	return bResult;
}


/*******************************************************************************
* Class      : Oscilloscope
* Function   : SetChannel()
* Access     : public
* Arguments  : ch       = channel
*              enabled  = true (enabled/visible) or false (disabled/not visible)
*              vdiv     = volts/division
*              offset   = voltage offset
*              coup     = coupling (DC or AC)
*              bwl      = bandwidth limit (BWL_FULL or BWL_ON)
*              atten    = attenuation (AT_10X or AT_1X)
*              inv      = channel inversion (INV_OFF or INV_ON)
* Returns    : true if successful, false otherwise
* Description:
*   Sets several parameters for the given channel with one call.
*/
bool Oscilloscope::SetChannelEx(Channel ch, bool enabled, VoltsPerDiv vdiv, double offset, Coupling coup, BWLimit bwl, ChAtten atten, ChInvert inv)
{
	bool bResult = false;
	const string strCh = GetChannelString(ch);

	bResult = SetChannelInvert(ch, inv);
	if (bResult)	bResult = SetChannelAtten(ch, atten);
	if (bResult)	bResult = SetChannelBWL(ch, bwl);
	if (bResult)	bResult = SetChannelCoupling(ch, coup);
	if (bResult)	bResult = SetChannelOffset(ch, offset);
	if (bResult)	bResult = SetChannelVolts(ch, vdiv);
	if (bResult)	bResult = SetChannelEnable(ch, enabled);

	return bResult;
}


/*******************************************************************************
* Class      : Oscilloscope
* Function   : AdjustChannelVolts()
* Access     : public
* Arguments  : ch       = channel
*              adjust   = number of ranges to adjust by (-3 to +3)
*                           -3 is three steps (one full decade) lower (fewer V/div)
*                           -2 is two steps lower
*                           -1 is one step lower
*                            0 is no adjustment
*                           +1 is one step higher
*                           +2 is two steps higher
*                           +3 is three steps (one full decade) higher (more V/div)
* Returns    : actual adjustment steps, 0 if no adjustment was made
* Description:
*   Adjusts the voltage/division higher or lower for the given channel.
*/
int Oscilloscope::AdjustChannelVolts(Channel ch, int adjust)
{
	ScaleValues dummy;

	if (adjust != 0)
		return AdjustChannelVolts(ch, adjust, dummy);
	else
		return true;
}


/*******************************************************************************
* Class      : Oscilloscope
* Function   : AdjustChannelVolts()
* Access     : public
* Arguments  : ch       = channel
*              adjust   = number of ranges to adjust by (-3 to +3)
*                           -3 is three steps (one full decade) lower (fewer V/div)
*                           -2 is two steps lower
*                           -1 is one step lower
*                            0 is no adjustment
*                           +1 is one step higher
*                           +2 is two steps higher
*                           +3 is three steps (one full decade) higher (more V/div)
*              scale    = reference to structure to receive the resulting scale parameters
*                           pp     = peak-to-peak voltage across the full scale range
*                           max    = highest voltage on the scale range
*                           min    = lowest voltage on the scale range
*                           vdiv   = V/division setting
*                           offset = offset voltage setting
* Returns    : actual adjustment steps, 0 if no adjustment was made
* Description:
*   Adjusts the voltage/division higher or lower for the given channel.
*/
int Oscilloscope::AdjustChannelVolts(Channel ch, int adjust, ScaleValues& scale)
{
	int iAdjActual = adjust;
	bool bNoIssues = true;

	const string strCh = GetChannelString(ch);
	string strResponse;
	smatch smMatch;

	// load scale with default values
	scale.max = 0.0;
	scale.min = 0.0;
	scale.pp = 0.0;
	scale.offset = 0.0;
	scale.vdiv = 0.0;

	// limit then number of scale steps that may be adjusted at one time
	if (iAdjActual < -3)
		iAdjActual = -3;
	if (iAdjActual > 3)
		iAdjActual = 3;

	// get the channel voltage scale and ofset
	if (Query(strCh + ":VDIV?", strResponse))
	{
		if (regex_match(strResponse, smMatch, regex("^C[1-4]\\:V[A-Z_]+ ([\\+\\-\\.0-9E]+)(?:V|A)\n$", regex::icase)))
		{
			scale.vdiv = stod(smMatch[1]);
		}

		if (Query(strCh + ":OFST?", strResponse))
		{
			if (regex_match(strResponse, smMatch, regex("^C[1-4]\\:O[A-Z]+ ([\\+\\-\\.0-9E]+)(?:V|A)\n$", regex::icase)))
			{
				scale.offset = stod(smMatch[1]);
			}
		}
		else
		{
			bNoIssues = false;
		}
	}
	else
	{
		bNoIssues = false;
	}

	if (bNoIssues)
	{
		// current scaling
		scale.pp = scale.vdiv * nVoltageDivisions;
		scale.max = scale.pp / 2.0 - scale.offset;
		scale.min = scale.pp / (-2.0) - scale.offset;
	}

	if (iAdjActual != 0)
	{
		// only volts/division is adjusted. offset is not adjusted
		// find the closest scale value, and adjust accordingly
		double vDeltaClose = std::numeric_limits<double>::infinity();
		int iChoose = -1;
		
		const double vAtten = ReadChannelAtten(ch);

		if (vAtten == 1.0 || vAtten == 10.0)
		{	// make an adjustment
			const int iPairTable = (vAtten == 1.0) ? PAIR_TABLE_1X : PAIR_TABLE_10X;

			for (int i = 0; i < nVoltagePairs; ++i)
			{
				double vDelta = abs(scale.vdiv - VoltagePairs[iPairTable][i].volts);

				if (vDelta < vDeltaClose)
				{
					vDeltaClose = vDelta;
					iChoose = i;
				}
			}

			if (iChoose >= 0)
			{
				// adjust by the selected number of steps, limiting within the usable range
				iChoose = iChoose + iAdjActual;
				if (iChoose < 0)
				{
					iAdjActual = iAdjActual - iChoose;  // since iChoose and iAdjActual were < 0, this will increase iAdjActual closer to 0
					iChoose = 0;
				}
				if (iChoose > nVoltagePairs - 1)
				{
					iAdjActual = iAdjActual - (iChoose - nVoltagePairs);
					iChoose = nVoltagePairs - 1;
				}

				// apply the new setting
				SetChannelVolts(ch, VoltagePairs[iPairTable][iChoose].vdiv);
			}

			// recursively call ourself (with adjust == 0) to refresh the scale per the newly applied adjustment
			AdjustChannelVolts(ch, 0, scale);
		}
		else
		{	// attenuation was not 1.0 or 10.0, so we don't know how to make an adjustment
			iAdjActual = 0;
		}
	}

	return iAdjActual;
}


/*******************************************************************************
* Class      : Oscilloscope
* Function   : ReadChannelAtten()
* Access     : private
* Arguments  : ch       = channel
* Returns    : channel attenuation
* Description:
*   Reads the attenuation for the given channel
*/
double Oscilloscope::ReadChannelAtten(Channel ch)
{
	const string strCh = GetChannelString(ch);
	string strAtten;
	smatch smMatch;

	Query(strCh + ":ATTN?", strAtten);
	// response format = "Cn:ATTN vv\n"
	if (regex_match(strAtten, smMatch, regex("^C[1-4]:ATTN ([0-9.]+)\n")))
		return stod(smMatch[1]);
	else
		return 0.0;
}


/*******************************************************************************
* Class      : Oscilloscope
* Function   : Measure()
* Access     : public
* Arguments  : ch       = channel
*              param    = measurement parameter
* Returns    : the value of the measurement parameter or DEFAULT_PARAM=NaN, check with isnan()
* Description:
*   Reads the given measurement on the given channel
*/
double Oscilloscope::Measure(Channel ch, MeasParam param)
{
	double dResult = Socket_Instrument::DEFAULT_PARAM; // return NaN if no result is obtained

	const string strCh = GetChannelString(ch);

	string strResult = "";
	smatch smMatch;
	string strMeasure = MeasPairs[0].str;

	for (unsigned int i = 0; i < nMeasPairs; ++i)
	{
		if (param == MeasPairs[i].par)
		{
			strMeasure = MeasPairs[i].str;
			break;
		}
	}

	if (Query(strCh + ":PAVA? " + strMeasure, strResult))
	{
		if (regex_match(strResult, smMatch, regex("^C[1-4]:PAVA [A-Z]+,([0-9.E+-]+)(V|A)\\s*$")))
		{
			dResult = stod(smMatch[1]);
		}
	}

	return dResult;
}


/*******************************************************************************
* Class      : Oscilloscope
* Function   : MeasureDelay()
* Access     : public
* Arguments  : ch1      = reference channel
*              ch2      = measurement channel
*              param    = measurement delay parameter
* Returns    : the value of the measurement delay parameter or DEFAULT_PARAM=NaN, check with isnan()
* Description:
*   Reads the given delay measurement between the given channels
*/
double Oscilloscope::MeasureDelay(Channel ch1, Channel ch2, MeasDelParam param)
{
	double dResult = Socket_Instrument::DEFAULT_PARAM; // return NaN if no result is obtained

	const string strCh1 = GetChannelString(ch1);
	const string strCh2 = GetChannelString(ch2);

	string strResult = "";
	smatch smMatch;
	string strMeasure = MeasDelPairs[0].str;

	for (unsigned int i = 0; i < nMeasDelPairs; ++i)
	{
		if (param == MeasDelPairs[i].par)
		{
			strMeasure = MeasDelPairs[i].str;
			break;
		}
	}

	if (Query(strCh1 + "-" + strCh2 + ":MEAD? " + strMeasure, strResult))
	{
		// note: unknown result "****" will fall through and produce NaN
		if (regex_match(strResult, smMatch, regex("^C[1-4]-C[1-4]:MEAD " + strMeasure + ",([0-9.E+-]+)[a-zA-Z]*\\s*$")))
		{
			dResult = stod(smMatch[1]);
		}
	}

	return dResult;
}


/*******************************************************************************
* Class      : Oscilloscope
* Function   : SetTimeDelay()
* Access     : public
* Arguments  : delay  = horizontal delay (seconds)
* Returns    : true if successful, false otherwise
* Description:
*   Reads the given delay measurement between the given channels
*/
bool Oscilloscope::SetTimeDelay(double delay)
{
	bool bResult = true;

	if (!isnan(delay))
		bResult = Write("TRDL "+to_string(delay));

	return bResult;
}


/*******************************************************************************
* Class      : Oscilloscope
* Function   : SetTimebase()
* Access     : public
* Arguments  : tcapture = target capture time (seconds)
*              delay    = horizontal delay (seconds)
* Returns    : total horizontal time in the capture (returns NaN on failure)
* Description:
*   Finds the closest settable timebase that provides a total capture time
*   greater than or equal to that passed as an argument. Also sets delay
*/
double Oscilloscope::SetTimebase(double tcapture, double delay)
{
	const double tdiv = tcapture / nTimeDivisions; 

	// default if no others match
	TimeDiv tpick = TimePairs[nTimePairs - 1].tdiv;
	double tactual = TimePairs[nTimePairs - 1].sec;

	for (unsigned int i = 0; i < nTimePairs-1; ++i)   // -1 => no need to do the last one, since it was chosen as default
	{
		if (tdiv <= TimePairs[i].sec)
		{
			tpick = TimePairs[i].tdiv;
			tactual = TimePairs[i].sec;
			break;
		}
	}

	if (SetTimebase(tpick, delay))
		return tactual * nTimeDivisions;
	else
		return DEFAULT_PARAM;
}


/*******************************************************************************
* Class      : Oscilloscope
* Function   : SetTimebase()
* Access     : public
* Arguments  : tdiv  = time/division setting (TimeDiv)
*              delay = horizontal delay (seconds)
* Returns    : true if successful, false otherwise
* Description:
*   Sets the timebase to the given time/division and delay.
*/
bool Oscilloscope::SetTimebase(TimeDiv tdiv, double delay)
{
	bool bResult = false;
	string strTDiv = "";

	for (unsigned int i = 0; i < nTimePairs; ++i)
	{
		if (tdiv == TimePairs[i].tdiv)
		{
			strTDiv = TimePairs[i].str;
			bResult = true;
			break;
		}
	}

	if (bResult)
		bResult = Write("TDIV " + strTDiv);
	if (bResult)
		bResult = SetTimeDelay(delay);

	return bResult;
}


/*******************************************************************************
* Copyright © 2021 Kerry S. Martin, martin@wild-wood.net
* Free for usage without warranty, expressed or implied; attribution required
*******************************************************************************/