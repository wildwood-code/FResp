/*******************************************************************************
* Copyright © 2021 Kerry S. Martin, martin@wild-wood.net
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
* Modified   : 11/05/2021
* Author     : Kerry S. Martin, martin@wild-wood.net
*******************************************************************************/
#include "FreqResp.h"
#include <string>
#include <regex>
#include <cmath>
#include <WinSock2.h>
#include <windows.h>

#pragma comment(lib, "Ws2_32.lib")

using namespace std;


// auto-voltage-scale limits, as % of full-scale peak-to-peak voltage
const double FreqResp::SEEK_MAX{ 1.000 };
const double FreqResp::SEEK_MID{ 0.390 };
const double FreqResp::SEEK_MIN{ 0.200 };
const double FreqResp::SEEK_MARGIN{ 0.0275 };
const double FreqResp::FREQ_FUDGE{ 1.001 };
const double FreqResp::MEAS_CYCLES{ 4.0 };


/*******************************************************************************
* Class      : FreqResp
* Function   : FreqResp() constructor
* Access     : public
* Arguments  : none
* Returns    : none
* Description:
*   Constructor for a FreqResp object, constructs an empty object. Most of the
*   setup is done with a subsequent call to Init().
*/
FreqResp::FreqResp() :
	stimulus(), oscope()
{
	data = FRST();
	initialized = false;
	completed = false;
}


/*******************************************************************************
* Class      : FreqResp
* Function   : ~FreqResp() destructor
* Access     : public
* Arguments  : none
* Returns    : none
* Description:
*   Destructor for a FreqResp object.
*/
FreqResp::~FreqResp()
{
	Close();
}


/*******************************************************************************
* Class      : FreqResp
* Typedef    : FRRET
* Access     : n/a
* Arguments  : n/a
* Returns    : used as return from FreqResp function calls
* Description:
*   FRRET as a return value is used to indicate success or failure of
*   any of the calls to FreqResp functions.
*     failure test:  if (retval <  FRRET_SUCCESS) { failed }
*     success test:  if (retval >= FRRET_SUCCESS) { succeeded }
*/


/*******************************************************************************
* Class      : FreqResp
* Function   : Close()
* Access     : public
* Arguments  : none
* Returns    : FRRET result (see documentation for FRRET above)
* Description:
*   Closes the FreqResp instruments, allowing Init() to be done again to start
*   another response measurement.
*/
FRRET FreqResp::Close()
{
	// detach from the instruments - these will do nothing if they had failed to attach
	oscope.Detach();
	stimulus.Detach();

	// reset the data set to empty
	data = FRST();

	// clear these flags to prevent starting a sweep (without initializing first) or grabbing data
	initialized = false;
	completed = false;

	return FRRET_SUCCESS;
}


/*******************************************************************************
* Class      : FreqResp
* Function   : Init()
* Access     : public
* Arguments  : _freq   = config of frequency sweep
*              _stim   = config of stimulus generator
*              _input  = config of oscope channel connected to DUT input
*              _output = config of oscope channel connected to DUT output
*              _trig   = config of oscope triggering
*              _meas   = config of oscope measurement
*              _dwell  = config of algorithm dwell time at each frequency
* Returns    : FRRET result (see documentation for FRRET above)
* Description:
*   Initializes the instruments and prepares for subsequent measurements.
*/
FRRET FreqResp::Init(char const* szOscope, char const* szSigGen, Freq_Config const& _freq, Stim_Config const& _stim, Channel_Config const& _input, Channel_Config const& _output, Trig_Config const& _trig, Meas_Config const& _meas, Dwell_Config const& _dwell)
{
	if (initialized)
		return FRRET_ALREADY_INITIALIZED;

	FRRET nReturnVal = FRRET_SUCCESS;

	// --------------------
	// store the parameters
	// --------------------
	freq = _freq;
	stim = _stim;
	input = _input;
	output = _output;
	trig = _trig;
	meas = _meas;
	dwell = _dwell;

	// ---------------
	// sanity checking
	// ---------------

	if (isnan(freq.fStart) || isnan(freq.fStop))
		nReturnVal = FRRET_INVALID_FREQUENCY;
	if (freq.fStart <= 0.0)
		nReturnVal = FRRET_INVALID_FREQUENCY;
	if (freq.fStop <= freq.fStart)
		nReturnVal = FRRET_INVALID_FREQUENCY;

	if (isnan(stim.vdc) || isnan(stim.vstim))
		nReturnVal = FRRET_INVALID_STIM;
	if (stim.vstim <= 0.0)
		nReturnVal = FRRET_INVALID_STIM;

	if (isnan(trig.vTrig))
		nReturnVal = FRRET_INVALID_TRIG;

	if (nReturnVal < FRRET_SUCCESS)
		return nReturnVal;
	
	// ----------------------------
	// default value initialization
	// ----------------------------

	measEdge = Oscilloscope::MeasDelParam::FRR;   // only used if measurement is set to delay (not for phase)
	avMeasure = 1.0;

	// -----------------------
	// stimulus initialization
	// -----------------------

	switch (stim.ch)
	{
	case 1: default:
		sgChannel = SineGenerator::Channel::CH1;
		break;
	case 2:
		sgChannel = SineGenerator::Channel::CH2;
		break;
	}

	switch (stim.vtStim)
	{
	case Vtype_t::VPK:
		vStim = 2.0 * abs(stim.vstim);  // express in Vpp = 2.0*Vpk
		break;
	case Vtype_t::VPP: default:
		vStim = abs(stim.vstim);
		break;
	}

	// attach to and configure the sine wave generator
	if (stimulus.Attach(szSigGen))
	{
		stimulus.SetChannel(sgChannel, freq.fStart, vStim, stim.vdc, 0.0);
		stimulus.SetChannelOutput(sgChannel, true);
	}
	else
	{
		nReturnVal = FRRET_INIT_SINEGEN;
	}

	if (nReturnVal < FRRET_SUCCESS)
		return nReturnVal;

	// ---------------------------
	// oscilloscope initialization
	// ---------------------------
	if (oscope.Attach(szOscope))
	{
		// initialize oscilloscope measurement
		switch (input.ch)
		{
		case 1: default:
			osChannelInput = Oscilloscope::Channel::CH1;
			break;
		case 2:
			osChannelInput = Oscilloscope::Channel::CH2;
			break;
		case 3:
			osChannelInput = Oscilloscope::Channel::CH3;
			break;
		case 4:
			osChannelInput = Oscilloscope::Channel::CH4;
			break;
		}

		switch (output.ch)
		{
		case 1:
			osChannelOutput = Oscilloscope::Channel::CH1;
			break;
		case 2: default:
			osChannelOutput = Oscilloscope::Channel::CH2;
			break;
		case 3:
			osChannelOutput = Oscilloscope::Channel::CH3;
			break;
		case 4:
			osChannelOutput = Oscilloscope::Channel::CH4;
			break;
		}

		switch (trig.ch)
		{
		case 1:
			osChannelTrig = Oscilloscope::Channel::CH1;
			break;
		case 2: default:
			osChannelTrig = Oscilloscope::Channel::CH2;
			break;
		case 3:
			osChannelTrig = Oscilloscope::Channel::CH3;
			break;
		case 4:
			osChannelTrig = Oscilloscope::Channel::CH4;
			break;
		}

		oscope.SetChannelEnable(osChannelInput, true);
		if (input.atten == 10.0)
			oscope.SetChannelAtten(osChannelInput, Oscilloscope::ChAtten::AT_10X);
		else
			oscope.SetChannelAtten(osChannelInput, Oscilloscope::ChAtten::AT_1X);
		oscope.SetChannelVoltsEx(osChannelInput, 1.0, 0.0);
		oscope.SetChannelEnable(osChannelOutput, true);
		if (output.atten == 10.0)
			oscope.SetChannelAtten(osChannelOutput, Oscilloscope::ChAtten::AT_10X);
		else
			oscope.SetChannelAtten(osChannelOutput, Oscilloscope::ChAtten::AT_1X);

		oscope.SetChannelVoltsEx(osChannelOutput, 1.0, 0.0);
		switch (input.coup)
		{
		case Ctype_t::AC: default:
			oscope.SetChannelCoupling(osChannelInput, Oscilloscope::Coupling::AC);
			break;
		case Ctype_t::DC:
			oscope.SetChannelCoupling(osChannelInput, Oscilloscope::Coupling::DC);
			break;
		}
		switch (output.coup)
		{
		case Ctype_t::AC: default:
			oscope.SetChannelCoupling(osChannelOutput, Oscilloscope::Coupling::AC);
			break;
		case Ctype_t::DC:
			oscope.SetChannelCoupling(osChannelOutput, Oscilloscope::Coupling::DC);
			break;
		}

		Oscilloscope::EdgeType trigEdge;
		switch (trig.edge)
		{
		case Etype_t::RISE: default:
			trigEdge = Oscilloscope::EdgeType::RISING;
			measEdge = Oscilloscope::MeasDelParam::FRR;
			break;
		case Etype_t::FALL:
			trigEdge = Oscilloscope::EdgeType::FALLING;
			measEdge = Oscilloscope::MeasDelParam::FFF;
			break;
		}

		Oscilloscope::Coupling trigCoup;
		switch (trig.coup)
		{
		case Ctype_t::AC: default:
			trigCoup = Oscilloscope::Coupling::AC;
			break;
		case Ctype_t::DC:
			trigCoup = Oscilloscope::Coupling::DC;
			break;

		}
		oscope.SetTriggerMode(Oscilloscope::TriggerMode::AUTO);
		oscope.SetEdgeTrigger(osChannelTrig, trigEdge, trig.vTrig, trigCoup, false);

		// both VPP and VPK use AMPL, which is essentially peak-to-peak with some noise reduction
		// but VPK returns 0.5 x AMPL whereas VPP returns 1.0 x AMPL
		mpMeasure = Oscilloscope::MeasParam::AMPL;

		switch (meas.vtMeas)
		{
		case Vtype_t::VPK: default:
			avMeasure = 0.5;
			break;
		case Vtype_t::VPP:
			avMeasure = 1.0;
			break;
		}
	}
	else
	{
		nReturnVal = FRRET_INIT_OSCILLOSCOPE;
	}

	if (nReturnVal < FRRET_SUCCESS)
		return nReturnVal;

	// ----------------------
	// initialization wrap-up
	// ----------------------

	initialized = true;

	// set the phase/delay type
	if (meas.ttMeas == Ttype_t::DELAY)
		tunit = TUNIT::DELAY;
	else
		tunit = TUNIT::PHASE;

	// get initial scale settings (call with adjust == 0)
	oscope.AdjustChannelVolts(osChannelOutput, 0, osScaleOutput);
	oscope.AdjustChannelVolts(osChannelInput, 0, osScaleInput);

	// set the initial sweep frequency
	f = freq.fStart;

	// perform and discard one measurement at the initial frequency
	// (the initial measurement is often incorrect)
	// TODO: this is just a temporary work-around until the root-cause
	// can be fixed
	FRS unused;
	MeasureFreq(f, unused);

	return nReturnVal;
}


/*******************************************************************************
* Class      : FreqResp
* Function   : Sweep()
* Access     : public
* Arguments  : none
* Returns    : FRRET result (see documentation for FRRET above)
* Description:
*   Performs the entire frequency response sweep. The results can be obtained
*   afterward by a call to the FRST conversion operator.
*/
FRRET FreqResp::Sweep()
{
	if (!initialized)
		return FRRET_NOT_INITIALIZED;

	FRRET nReturnVal = FRRET_SUCCESS;

	// restart from the first frequency
	completed = false;
	f = freq.fStart;

	while (!completed)
	{
		FRS unused;
		nReturnVal = MeasureNext(unused);

		if (nReturnVal < FRRET_SUCCESS)
			break;
	}

	return nReturnVal;
}


/*******************************************************************************
* Class      : FreqResp
* Function   : MeasureNext()
* Access     : public
* Arguments  : result = ref to FRS object to receive the freq measurement result
* Returns    : FRRET result (see documentation for FRRET above)
* Description:
*   Performs one step of the frequency response measurement. Used when intermediate
*   results are needed, for instance if updating a plot real-time.
*/
FRRET FreqResp::MeasureNext(FRS& result)
{
	FRRET nReturnVal = FRRET_SUCCESS;

	if (!initialized)
	{
		nReturnVal = FRRET_NOT_INITIALIZED;
	}
	else if (completed)
	{
		nReturnVal = FRRET_COMPLETE;
	}
	else
	{
		FRS frs_result;

		nReturnVal = MeasureFreq(f, frs_result);

		if (nReturnVal >= FRRET_SUCCESS)
		{
			result = frs_result;
			data.push_back(frs_result);

			// get the next frequency, depending upon the sweep type
			if (freq.sweep == Sweep_t::LOG)
			{
				f = f * exp(log(10.0) / freq.Npoints);

				// look for completion condition
				if (f > FREQ_FUDGE * freq.fStop)
					completed = true;
			}
			else if (freq.sweep == Sweep_t::LIN)
			{
				f = f + (freq.fStop - freq.fStart) / (freq.Npoints - 1);

				// look for completion condition
				if (f > FREQ_FUDGE * freq.fStop)
					completed = true;
			}
			else
			{
				// unknown test - no change to frequency - completed now
				completed = true;
			}

			if (completed)
				nReturnVal = FRRET_COMPLETE;
		}
	}

	return nReturnVal;
}


/*******************************************************************************
* Class      : FreqResp
* Function   : MeasureFreq()
* Access     : private
* Arguments  : f      = frequency at which response will be measured
*              result = ref to FRS object to receive the freq measurement result
* Returns    : FRRET result (see documentation for FRRET above)
* Description:
*   Performs one step of the frequency response measurement.
*/
FRRET FreqResp::MeasureFreq(double f, FRS& result)
{
	FRRET nReturnVal = FRRET_SUCCESS;
	const double Tideal = MEAS_CYCLES / f;
	const double Tactual = oscope.SetTimebase(Tideal);

	// set the test frequency
	stimulus.SetChannelFreq(sgChannel, f);

	// dwell here to allow the circuit transient response to stablize
	DWORD dwDelay = DWORD(1000 * (dwell.stable_screens * Tactual));
	if (dwDelay < dwell.minDwell_msec)
		dwDelay = dwell.minDwell_msec;
	Sleep(dwDelay); // milliseconds

	bool bLoopDone = false;
	int adjust_in = 0;
	int adjust_out = 0;
	double mag_in = 0.0, mag_out = 0.0, time_meas = 0.0;

	int alternate_count = 0;
	do
	{
		// Detection of "hunting" for a scale
		int adjust_in_last = adjust_in;
		int adjust_out_last = adjust_out;

		// get the measurements and do an auto-scale step for input and output
		mag_in = avMeasure * MeasureAndScaleInput(oscope, osChannelInput, mpMeasure, osScaleInput, adjust_in);
		mag_out = avMeasure * MeasureAndScaleInput(oscope, osChannelOutput, mpMeasure, osScaleOutput, adjust_out);

		if (adjust_in_last * adjust_in < 0 || adjust_out_last * adjust_out < 0)
		{	// either in or out (or both) switched direction, count this towards limit
			alternate_count = alternate_count + 1;
		}

		if ((adjust_in == 0 && adjust_out == 0) || alternate_count >= 3)
		{	// no adjustments were made to the scaling or we are hunting for a scale...
			// either way, measure phase|delay and exit the loop
			if (meas.ttMeas == Ttype_t::DELAY)
				time_meas = oscope.MeasureDelay(osChannelInput, osChannelOutput, measEdge);
			else
				time_meas = oscope.MeasureDelay(osChannelInput, osChannelOutput, Oscilloscope::MeasDelParam::PHA);
			bLoopDone = true;
		}

	} while (!bLoopDone);

	const double mag_gain = abs(mag_out / mag_in);
	const double dB_gain = 20.0 * log10(mag_gain);
	
	result.freq = f;
	result.mag_in = mag_in;
	result.mag_out = mag_out;
	result.dBgain = dB_gain;
	result.time = time_meas;
	result.tunit = tunit;

	return nReturnVal;
}


/*******************************************************************************
* Class      : FreqResp
* Function   : operator FRST const&
* Access     : public
* Arguments  : none
* Returns    : reference to FRST object
* Description:
*   FRST conversion operator. Used to obtain the frequency response results.
*/
FreqResp::operator FRST const& () const
{
	return data;
}


/*******************************************************************************
* Function   : MeasureAndScaleInput()
* Arguments  : oscope    = reference to oscilloscope object
*              ch        = oscilloscope channel
*              mpMeasure = type of measurement parameter to return
*              scale     = reference to scale structure, holds scale info on return
*              adjust    = reference to adjustment value, holds actual applied adjust on return
* Returns    : measurement value; scale and adjust references receive data on return
* Description:
*   This function implements channel measurement and auto-scaling adjustment.
*   The adjust reference on entry contains the last adjustment (-3 to +3) made.
*   Initialize to 0 on the first call to this function.
*   On exit, it contanis the number of steps the vertical scale was adjusted, or contains
*   0 if no adjustment was made. Call this function repeatedly until the adjust parameter
*   is 0.
*   The return value is the actual measurement.
*/
double FreqResp::MeasureAndScaleInput(Oscilloscope& oscope, Oscilloscope::Channel ch, Oscilloscope::MeasParam mpMeasure, Oscilloscope::ScaleValues& scale, int& adjust)
{
	// adjust is set to 0 when all adjustments have been completed
	// keep calling this until adjustments have been completed

	// get the measurements
	const double mag = oscope.Measure(ch, mpMeasure);
	const int last_adjust = adjust;
	const double mag_pkpk = (mpMeasure == Oscilloscope::MeasParam::PKPK) ? mag : oscope.Measure(ch, Oscilloscope::MeasParam::PKPK);

	if (mag_pkpk > (SEEK_MAX - SEEK_MARGIN) * scale.pp)
		adjust = oscope.AdjustChannelVolts(ch, +1, scale);
	else if (mag_pkpk < (SEEK_MIN - SEEK_MARGIN) * scale.pp)
		adjust = oscope.AdjustChannelVolts(ch, -2, scale);
	else if (mag_pkpk < (SEEK_MID - SEEK_MARGIN) * scale.pp)
		adjust = oscope.AdjustChannelVolts(ch, -1, scale);
	else
		adjust = 0;

	return mag;
}


/*******************************************************************************
* Copyright © 2021 Kerry S. Martin, martin@wild-wood.net
* Free for usage without warranty, expressed or implied; attribution required
*******************************************************************************/