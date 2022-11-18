/*******************************************************************************
* Copyright © 2021 Kerry S. Martin, martin@wild-wood.net
* Free for usage without warranty, expressed or implied; attribution required
*
* Filename   : SineGenerator.h
* Class      : SineGenerator
* Description:
*   Implements an interface to a Rigol DG800 series signal generator used to
*   generate a sinusoidal waveform.
*
* Created    : 05/25/2020
* Modified   : 11/05/2021
* Author     : Kerry S. Martin, martin@wild-wood.net
*******************************************************************************/
#pragma once
#include "Socket_Instrument.h"

class SineGenerator :
	protected Socket_Instrument
{
public:
	SineGenerator();
	virtual ~SineGenerator();
	virtual bool Attach(std::string resource);
	//virtual bool Attach(std::regex pattern);
	virtual bool Detach();

	enum class Channel { CH1, CH2 };
	bool SetChannel(Channel ch, double freq=DEFAULT_PARAM, double Vpp = DEFAULT_PARAM, double Voffs=DEFAULT_PARAM, double phase=DEFAULT_PARAM);
	bool SetChannelFreq(Channel ch, double freq);
	bool SetChannelVpp(Channel ch, double Vpp);
	bool SetChannelVoffs(Channel ch, double Voffs);
	bool SetChannelPhase(Channel ch, double phase);
	bool SetChannelOutput(Channel ch, bool output);
	bool AlignChannel(Channel ch);

private:
	bool SetupSineGeneratorDefault();
	static std::string GetChannelString(Channel ch);
	static double CoercePhase(double phase);
};


constexpr auto SINEGEN_CH_1 = SineGenerator::Channel::CH1;
constexpr auto SINEGEN_CH_2 = SineGenerator::Channel::CH2;


/*******************************************************************************
* Copyright © 2021 Kerry S. Martin, martin@wild-wood.net
* Free for usage without warranty, expressed or implied; attribution required
*******************************************************************************/