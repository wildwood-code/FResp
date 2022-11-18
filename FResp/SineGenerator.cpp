/*******************************************************************************
* Copyright © 2021 Kerry S. Martin, martin@wild-wood.net
* Free for usage without warranty, expressed or implied; attribution required
*
* Filename   : SineGenerator.cpp
* Class      : SineGenerator
* Description:
*   Implements an interface to a Rigol DG800 series signal generator used to
*   generate a sinusoidal waveform.
*
* Created    : 05/25/2020
* Modified   : 11/05/2021
* Author     : Kerry S. Martin, martin@wild-wood.net
*******************************************************************************/

#include <string>
#include <cmath>
#include "SineGenerator.h"
using namespace std;


/*******************************************************************************
* Class      : SineGenerator
* Function   : SineGenerator() constructor
* Access     : public
* Arguments  : none
* Returns    : none
* Description:
*   Constructs a default (attached to no instrument) SineGenerator object
*/
SineGenerator::SineGenerator()
	: Socket_Instrument()
{
}


/*******************************************************************************
* Class      : SineGenerator
* Function   : ~SineGenerator() destructor
* Access     : public
* Arguments  : none
* Returns    : none
* Description:
*   Destroys a SineGenerator object, first detaching any attached instruments
*/
SineGenerator::~SineGenerator()
{
}


/*******************************************************************************
* Class      : SineGenerator
* Function   : Attach()
* Access     : public
* Arguments  : resource = resource identifier string for instrument
* Returns    : true if successful (instrument was attached), false if not
* Description:
*   Attaches to an instrument using the given resource name
*/
bool SineGenerator::Attach(std::string resource)
{
	bool bResult = false;

	if (Socket_Instrument::Attach(resource))
		bResult = SetupSineGeneratorDefault();

	return bResult;
}


#if 0
/*******************************************************************************
* Class      : SineGenerator
* Function   : Attach()
* Access     : public
* Arguments  : pattern = regex pattern to identify instrument
* Returns    : true if successful (instrument was attached), false if not
* Description:
*   Attaches to an instrument having the given regex pattern
*/
bool SineGenerator::Attach(std::regex pattern)
{
	bool bResult = false;

	if (Socket_Instrument::Attach(pattern))
		bResult = SetupSineGeneratorDefault();

	return bResult;
}
#endif


/*******************************************************************************
* Class      : SineGenerator
* Function   : Detach()
* Access     : public
* Arguments  : none
* Returns    : true if successful (instrument was detached), false if not
* Description:
*   Detaches from the instrument
*/
bool SineGenerator::Detach()
{
	return Socket_Instrument::Detach();
}


/*******************************************************************************
* Class      : SineGenerator
* Function   : GetChannelString()
* Access     : private static
* Arguments  : ch = Channel enum
* Returns    : string designation of channel
* Description:
*   Get the string designation for the given channel
*/
string SineGenerator::GetChannelString(Channel ch)
{
	switch (ch)
	{
	case Channel::CH1: default:		return string("1");
	case Channel::CH2:				return string("2");
	}
}


/*******************************************************************************
* Class      : SineGenerator
* Function   : CoercePhase()
* Access     : private static
* Arguments  : phase = phase, in degrees
* Returns    : phase wrapped to be within the range 0.0 <= phase < 360.0
* Description:
*   Coerces (wraps) the phase to be within the range of 0 to 360 degrees.
*/
double SineGenerator::CoercePhase(double phase)
{
	phase = phase - 360.0*floor(phase / 360.0);

	return phase;
}


/*******************************************************************************
* Class      : SineGenerator
* Function   : SetupSineGeneratorDefault()
* Access     : private
* Arguments  : none
* Returns    : true if successful, false otherwise
* Description:
*   Puts the sine generator in a default instrument state:
*     For CH1: sine wave, 1kHz, 1.0Vpp, 0V offset,  0° phase, output off
*     For CH2: sine wave, 1kHz, 1.0Vpp, 0V offset, 90° phase, output off
*/
bool SineGenerator::SetupSineGeneratorDefault()
{
	bool bResult = false;

	if (Write(":SOUR1:APPL:SIN 1000,1,0,0"))
		bResult = Write(":SOUR2:APPL:SIN 1000,1,0,90");

	return bResult;
}


/*******************************************************************************
* Class      : SineGenerator
* Function   : SetChannel()
* Access     : public
* Arguments  : ch    = channel to set
*              freq  = frequency (Hz) {DEFAULT_PARAM=>no change}
*              Vpp   = peak-to-peak voltage (V) {DEFAULT_PARAM=>no change}
*              Voffs = offset voltage (V) {DEFAULT_PARAM=>no change}
*              phase = phase (degrees) {DEFAULT_PARAM=>no change}
* Returns    : true if successful, false otherwise
* Description:
*   Applies the given settings to the channel.
*/
bool SineGenerator::SetChannel(Channel ch, double freq, double Vpp, double Voffs, double phase)
{
	bool bResult = true;

	const string strCh = GetChannelString(ch);
	string strCommand;

	if (!isnan(freq))
	{
		strCommand = ":SOUR" + strCh + ":FREQ " + std::to_string(freq);
		bResult = Write(strCommand);
	}

	if (bResult && !isnan(Vpp))
	{
		strCommand = ":SOUR" + strCh + ":VOLT " + std::to_string(Vpp);
		bResult = Write(strCommand);
	}

	if (bResult && !isnan(Voffs))
	{
		strCommand = ":SOUR" + strCh + ":VOLT:OFFS " + std::to_string(Voffs);
		bResult = Write(strCommand);
	}

	if (bResult && !isnan(phase))
	{
		phase = CoercePhase(phase);
		strCommand = ":SOUR" + strCh + ":PHAS " + std::to_string(phase);
		bResult = Write(strCommand);
	}

	return bResult;
}


/*******************************************************************************
* Class      : SineGenerator
* Function   : SetChannelFreq()
* Access     : public
* Arguments  : ch    = channel to set
*              freq  = frequency (Hz) {DEFAULT_PARAM=>no change}
* Returns    : true if successful, false otherwise
* Description:
*   Applies the given frequency setting to the channel.
*/
bool SineGenerator::SetChannelFreq(Channel ch, double freq)
{
	const string strCh = GetChannelString(ch);
	const string strCommand = ":SOUR" + strCh + ":FREQ " + std::to_string(freq);
	bool bResult = Write(strCommand);
	return bResult;
}


/*******************************************************************************
* Class      : SineGenerator
* Function   : SetChannelVpp()
* Access     : public
* Arguments  : ch    = channel to set
*              vpp   = peak-to-peak voltage (V)
* Returns    : true if successful, false otherwise
* Description:
*   Applies the given amplitude setting to the channel.
*/
bool SineGenerator::SetChannelVpp(Channel ch, double Vpp)
{
	const string strCh = GetChannelString(ch);
	const string strCommand = ":SOUR" + strCh + ":VOLT " + std::to_string(Vpp);
	bool bResult = Write(strCommand);
	return bResult;
}


/*******************************************************************************
* Class      : SineGenerator
* Function   : SetChannelVoffs()
* Access     : public
* Arguments  : ch     = channel to set
*              Voffs  = offset voltage (V)
* Returns    : true if successful, false otherwise
* Description:
*   Applies the given offset voltage setting to the channel.
*/
bool SineGenerator::SetChannelVoffs(Channel ch, double Voffs)
{
	const string strCh = GetChannelString(ch);
	const string strCommand = ":SOUR" + strCh + ":VOLT:OFFS " + std::to_string(Voffs);
	bool bResult = Write(strCommand);
	return bResult;
}


/*******************************************************************************
* Class      : SineGenerator
* Function   : SetChannelPhase()
* Access     : public
* Arguments  : ch     = channel to set
*              phase  = phase angle (degrees)
* Returns    : true if successful, false otherwise
* Description:
*   Applies the given phase angle setting to the channel.
*/
bool SineGenerator::SetChannelPhase(Channel ch, double phase)
{
	const string strCh = GetChannelString(ch);
	const string strCommand = ":SOUR" + strCh + ":PHAS " + std::to_string(CoercePhase(phase));
	bool bResult = Write(strCommand);
	return bResult;
}


/*******************************************************************************
* Class      : SineGenerator
* Function   : AlignChannel()
* Access     : public
* Arguments  : ch     = channel to set
* Returns    : true if successful, false otherwise
* Description:
*   Aligns the given channel with the other channel (phase relationship)
*/
bool SineGenerator::AlignChannel(Channel ch)
{
	const string strCh = GetChannelString(ch);
	const string strCommand = ":SOUR" + strCh + ":PHAS:SYNC";
	bool bResult = Write(strCommand);
	return bResult;
}


/*******************************************************************************
* Class      : SineGenerator
* Function   : SetChannelOutput()
* Access     : public
* Arguments  : ch     = channel to set
*              output = output state (true=on, false=off)
* Returns    : true if successful, false otherwise
* Description:
*   Turns the given channel output on or off
*/
bool SineGenerator::SetChannelOutput(Channel ch, bool output)
{
	const string strCh = GetChannelString(ch);
	const string strOutput = output?"ON":"OFF";
	const string strCommand = ":OUTP" + strCh + " " + strOutput;
	bool bResult = Write(strCommand);
	return bResult;
}


/*******************************************************************************
* Copyright © 2021 Kerry S. Martin, martin@wild-wood.net
* Free for usage without warranty, expressed or implied; attribution required
*******************************************************************************/