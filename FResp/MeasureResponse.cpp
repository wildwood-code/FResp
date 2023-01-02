/*******************************************************************************
* Copyright © 2023 Kerry S. Martin, martin@wild-wood.net
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
* Modified   : 01/01/2023
* Author     : Kerry S. Martin, martin@wild-wood.net
*
* History    : Ver    Date         Notes
*              0.00    2020-05-29  Initial release
*              0.01    2020-06-21  Fixed cend() bug, added delay/phase
*              1.00    2020-07-03  Implemented as MeasureResponse()
*              2.00    2021-11-95  Changed from VISA to Winsock
*              2.01    2021-11-11  Fixed filename parsing
*              2.02    2023-01-01  Added BWL switch for input, output
*******************************************************************************/

#include <algorithm>
#include <iostream>
#include <fstream>
#include <string>
#include <regex>
#include <cmath>
#include "EchoDualStream.h"
#include "FreqResp.h"
#include "MeasureResponse.h"
#include "FResp_Settings.h"

using namespace std;

constexpr auto VERSION = "2.02";

//#define DEBUG_WITHOUT_INSTRUMENTS			// uncomment this to run the code without connecting to the instruments (for debugging parsing, etc)


// HKEY_CURRENT_USER key location for program settings (WARNING! be aware of MAX_KEY_LENGTH)
constexpr auto REGISTRY_KEY = "SOFTWARE\\WWES\\FResp\\Settings\\";

// default resource addresses for instruments (will be written to registry if non-existent)
constexpr auto RESOURCE_DEFAULT_OSCOPE = "192.168.0.197:5025";
constexpr auto RESOURCE_DEFAULT_SIGGEN = "192.168.0.198:5555";

#define DEFAULT_DOUBLE std::nan("")			// double value returned when no value was specified in an command-line parameter

constexpr auto CH_TRIG_IN = -1;				// value that will be interpreted as "set it to the same channel as input"
constexpr auto CH_TRIG_OUT = -2;			// value that will be interpreted as "set it to the same channel as output"



/*******************************************************************************
* Function   : ExitPrintUsage
* Arguments  : strProgName  = name of the program (*.exe)
* Returns    : RETURN_SUCCESS always
* Description:
*   This function simply prints the command-line usage to std::cout
*/
int ExitPrintUsage(std::string strProgName)
{
	std::cout << strProgName << " ";
	std::cout << "freq:fstart-fstop,log|lin(npts) ";
	std::cout << "stim:ch,vampl+voffset ";
	std::cout << "in:ch,ac|dc,1x|10x,bwl|-bwl out:ch,ac|dc,1x|10x,bwl|-bwl ";
	std::cout << "trig:ch,ac|dc,rising|falling,vtrig ";
	std::cout << "meas:Vpk|Vpp,phase|delay ";
	std::cout << "dwell:fast|mid|slow file:filename,quiet|echo\n";
	std::cout << "  fstart and fstop may use suffix notation (ex/ 1k-10k)\n";
	std::cout << "  log sweep npts is points/decade\n";
	std::cout << "  lin sweep npts is the points/sweep\n";
	std::cout << "  stim vampl+voffset are optional, ch defaults to oscope in or may be S1-S2\n";
	std::cout << "  in, out ch is 1-4 (ex/ ch1, c1, or 1 are equivalent)\n";
	std::cout << "  in, out ac|dc coupling is optional, defaults to ac\n";
	std::cout << "  in, out bwl|-bwl  bandwidth limit is optional, defaults to bwl\n";
	std::cout << "  trig all parameters optional in any order\n";
	std::cout << "  trig ch may be 1-4, in, or out\n";
	std::cout << "  trig vtrig is the trigger voltage\n";
	std::cout << "  meas specifies the measurement type (VPP|VPK and phase|delay)\n";
	std::cout << "  file|log|report specifies a destination file for the output\n";
	std::cout << "  quiet or echo specifies output to the standard output\n\n";
	std::cout << "  " << strProgName << " Version " << VERSION << " (" << __DATE__ << " " << __TIME__ ")\n";
	std::cout << "  Copyright (c) 2023 Kerry S. Martin, martin@wild-wood.net\n\n";
	std::cout << "  Defaults:\n";
	std::cout << "  " + strProgName + " freq:1k-100k,log(10) stim:S1,1.0Vpp+0Vdc in:C1,ac,10x,bwl out:C2,ac,10x,bwl trig:in,0.0mV,ac,rising meas:Vpp dwell:mid\n\n";

	return RETURN_SUCCESS;
}





/*******************************************************************************
* Function   : str_compare_icase()
* Arguments  : str1, str2 = strings to compare
* Returns    : true if they match (ignoring case), false otherwise
* Description:
*   This function compares two strings in a case-insensitive way.
*/
bool str_compare_icase(string str1, string str2)
{
	// convert the strings to uppercase for comparison
	std::transform(str1.begin(), str1.end(), str1.begin(), ::toupper);
	std::transform(str2.begin(), str2.end(), str2.begin(), ::toupper);

	if (str1.compare(str2) == 0)
		return true;
	else
		return false;
}


/*******************************************************************************
* Function   : to_value()
* Arguments  : strBase   = base number
*              strSuffix = suffix multiplier
*                "" = none  = 10^0  multiplier
*                m  = milli = 10^-3 multiplier
*                k  = kilo  = 10^3  multiplier
*                M  = mega  = 10^6  multiplier
*              strSign   = sign, either "+", "-", or empty
* Returns    : value of the strings, converted to double
* Description:
*   This function converts strings with optional suffix multiplier (m, k, or M)
*   and an optional sign into a double.
*/
double to_value(string strBase, string strSuffix, string strSign = "")
{
	double vResult;

	vResult = stod(strBase);

	if (!strSuffix.empty())
	{
		const char suf = strSuffix[0];

		switch (suf)
		{
		case 'k': case 'K':
			vResult = vResult * 1.0e3;
			break;
		case 'm':
			vResult = vResult * 1.0e-3;
			break;
		case 'M':
			vResult = vResult * 1.0e6;
			break;
		}
	}

	if (!strSign.empty())
	{
		const char sign = strSign[0];

		switch (sign)
		{
		case '-':
			vResult = -1.0 * vResult;
			break;
		}
	}

	return vResult;
}


/*******************************************************************************
* Function   : strip_path()
* Arguments  : filename = full filename to the file
* Returns    : just the base filename, without the path
* Description:
*   This function strips the pathname from a filename.
*   ex/ C:\Projects\MyProject\Release\MyProject.exe => MyProject.exe
*/
string strip_path(string filename)
{
	const regex re("^(?:.*)(?:\\/|\\\\)(.+?)$");
	smatch smMatch;

	if (regex_match(filename, smMatch, re))
		return smMatch[1];
	else
		return filename;
}


/*******************************************************************************
* Function   : get_suffix()
* Arguments  : filename = full filename to the file
* Returns    : just the suffix, including the '.'
* Description:
*   This function returns the suffixe from a filename
*   ex/ C:\Projects\MyProject\Release\MyProject.exe => .exe
*/
string get_suffix(string filename)
{
	const regex re("^.+(\\.[a-zA-Z0-9]+?)$");
	smatch smMatch;

	if (regex_match(filename, smMatch, re))
		return smMatch[1];
	else
		return string("");
}


/*******************************************************************************
* Structure  : Log_Spec
* Members    : strFilename = specified filename (blank for unspecified)
*              logConsole  = console log is quiet, echo, or unspecified
* Description:
*   An object of this structue is passed by reference to EvalLogSpec() to
*   receive the file/report/log parameters.
*/
enum class Logfile_Console_Spec { UNSPEC, ECHO, QUIET };
struct Log_Spec
{
	std::string strFilename;
	Logfile_Console_Spec  logConsole;

	Log_Spec() : strFilename(""), logConsole(Logfile_Console_Spec::UNSPEC) {};
};


/*******************************************************************************
* Function   : EvalLogSpec
* Arguments  : strSpec = string containing a comma-separated stimulus specification
*              spec    = reference to target for specs contained in strSpec
* Returns    : true = success, false = failure
* Description:
*   This function evaluates the command line specification of the logfile.
*   ex/ "C:\Tools\Data\out.txt",echo
*/
bool EvalLogSpec(string strSpec, Log_Spec& spec)
{
	bool bResult = true;
	const regex reQuoted("^\"([^""]+)\"(?:,(.*))?$");
	const regex reNonQuoted("^([^,\"]+?)(?:,(.*))?$");
	const regex regex_echo_quiet("^(?:(echo)|(quiet))$", regex::icase);

	// defaults
	spec.strFilename = "";
	spec.logConsole = Logfile_Console_Spec::UNSPEC;

	while (!strSpec.empty())
	{
		smatch smMatch;

		if (regex_match(strSpec, smMatch, reQuoted))
		{
			spec.strFilename = smMatch[1];
			strSpec = smMatch[2];
		}
		else if (regex_match(strSpec, smMatch, reNonQuoted))
		{
			string strMatch = smMatch[1];
			strSpec = smMatch[2];

			if (regex_match(strMatch, smMatch, regex_echo_quiet))
			{
				if (smMatch[1].matched)
					spec.logConsole = Logfile_Console_Spec::ECHO;
				else
					spec.logConsole = Logfile_Console_Spec::QUIET;
			}
			else
			{
				spec.strFilename = strMatch;
			}
		}
		else
		{
			bResult = false;
			break;
		}
	}

	return bResult;
}


/*******************************************************************************
* Structure  : Stim_Spec
* Members    : ch  = channel (Stim_Channel_Spec)
*              Vpp = peak-to-peak voltage of the stimulus
*              Vdc = DC offset of the stimulus
* Description:
*   An object of this structue is passed by reference to EvalStimSpec() to
*   receive the stimulus specification parameters.
*/
enum class Stim_Channel_Spec { UNSPEC, S1, S2 };
struct Stim_Spec
{
	Stim_Channel_Spec ch;
	double Vpp;
	double Vdc;

	Stim_Spec() : ch(Stim_Channel_Spec::UNSPEC), Vpp(DEFAULT_DOUBLE), Vdc(DEFAULT_DOUBLE) {};
};


/*******************************************************************************
* Function   : EvalStimSpec
* Arguments  : strSpec = string containing a comma-separated stimulus specification
*              spec    = reference to target for specs contained in strSpec
* Returns    : true = success, false = failure
* Description:
*   This function evaluates the command line specification of the stimulus.
*   ex/ S1,750mVpk+0.0Vdc
*/
bool EvalStimSpec(string strSpec, Stim_Spec& spec)
{
	const regex reComma("^(.+?)(?:,(.*))?$");
	const regex reChannel("^(?:ST?|CH?)?([1-2])$", regex::icase);
	const regex reVoltage("^\\+?(\\d*\\.?\\d*(?:E(?:(?:\\+|-)?\\d{1,3}))?)(m)?(VPP|VPK?)(?:(\\+|-)(\\d*\\.?\\d*(?:E(?:\\+|-)?\\d{1,3})?)(m)?(?:V|VDC)?)?$", regex::icase);

	bool bResult = true;
	smatch smMatch;

	// initialize to default return values
	spec.ch = Stim_Channel_Spec::UNSPEC;
	spec.Vpp = DEFAULT_DOUBLE;
	spec.Vdc = DEFAULT_DOUBLE;

	while (!strSpec.empty())
	{
		if (!regex_match(strSpec, smMatch, reComma))
		{
			bResult = false;
			break;
		}

		// get the argument
		const string strArg = smMatch[1];

		// pass remainder on to the next loop until done
		strSpec = smMatch[2];

		if (regex_match(strArg, smMatch, reChannel))
		{
			const string strCh = smMatch[1];
			const char cCh = strCh[0];
			switch (cCh)
			{
			case '1':
				spec.ch = Stim_Channel_Spec::S1;
				break;
			case '2':
				spec.ch = Stim_Channel_Spec::S2;
				break;
			}
		}
		else if (regex_match(strArg, smMatch, reVoltage))
		{
			const string strBase = smMatch[1];
			const string strSuf = smMatch[2];
			const string strUnit = smMatch[3];

			double vStim = to_value(strBase, strSuf);

			if (!str_compare_icase(strUnit, "VPP"))
				vStim = 2.0 * vStim;

			spec.Vpp = vStim;

			if (smMatch[4].matched)
			{
				const string strSignDC = smMatch[4];
				const string strBaseDC = smMatch[5];
				const string strSufDC = smMatch[6];

				double vStimDC = to_value(strBaseDC, strSufDC, strSignDC);
				spec.Vdc = vStimDC;
			}
			else
			{
				spec.Vdc = 0.0;
			}
		}
		else
		{
			bResult = false;
			break;
		}
	}

	return bResult;
}


/*******************************************************************************
* Structure  : Trig_Spec
* Members    : voltage = trigger voltage
*              coup    = trigger coupling (Trig_Coupling_Spec)
*              edge    = trigger edge (Trig_Edge_Spec)
*              ch      = trigger channel (Trig_Channel_Spec)
* Description:
*   An object of this structue is passed by reference to EvalTrigSpec() to
*   receive the trigger specification parameters.
*/
enum class Trig_Coupling_Spec { UNSPEC, AC, DC };
enum class Trig_Edge_Spec { UNSPEC, RISING, FALLING };
enum class Trig_Channel_Spec { UNSPEC, CH1, CH2, CH3, CH4, CH_IN, CH_OUT };
struct Trig_Spec
{
	double voltage;
	Trig_Coupling_Spec coup;
	Trig_Edge_Spec edge;
	Trig_Channel_Spec ch;

	Trig_Spec() : voltage(DEFAULT_DOUBLE), coup(Trig_Coupling_Spec::UNSPEC), edge(Trig_Edge_Spec::UNSPEC), ch(Trig_Channel_Spec::UNSPEC) {};
};


/*******************************************************************************
* Function   : EvalTrigSpec
* Arguments  : strSpec = string containing a comma-separated trigger specification
*              spec    = reference to target for specs contained in strSpec
* Returns    : true = success, false = failure
* Description:
*   This function evaluates the command line specification of the trigger.
*   ex/ CH1,0.0V,rising,ac
*/
bool EvalTrigSpec(string strSpec, Trig_Spec& spec)
{
	const regex reComma("^(.+?)(?:,(.*))?$");
	const regex reVoltage("^((?:\\+|-)?\\d*\\.?\\d*(?:E(?:\\+|-)?\\d{1,3})?)(M)?V?(?:,(.+))?$", regex::icase);
	const regex reCoup("^(?:(A)C|(D)C)$", regex::icase);
	const regex reEdge("^(?:(?:(R)(?:ISE|ISING)?)|(?:(F)(?:ALL|ALLING)?))$", regex::icase);
	const regex reChannel("^(?:(I)N|(O)UT|CH?([1-4]))$", regex::icase);

	bool bResult = true;
	smatch smMatch;

	// initialize to default return values
	spec.ch = Trig_Channel_Spec::UNSPEC;
	spec.coup = Trig_Coupling_Spec::UNSPEC;
	spec.edge = Trig_Edge_Spec::UNSPEC;
	spec.voltage = DEFAULT_DOUBLE;

	while (!strSpec.empty())
	{
		if (!regex_match(strSpec, smMatch, reComma))
		{
			bResult = false;
			break;
		}

		// get the argument
		const string strArg = smMatch[1];

		// pass remainder on to the next loop until done
		strSpec = smMatch[2];

		// now, process the argument
		if (regex_match(strArg, smMatch, reChannel))
		{
			if (smMatch[1].matched)
			{
				spec.ch = Trig_Channel_Spec::CH_IN;
			}
			else if (smMatch[2].matched)
			{
				spec.ch = Trig_Channel_Spec::CH_OUT;
			}
			else if (smMatch[3].matched)
			{
				const string strChNum = smMatch[3];
				const char chNum = strChNum[0];

				switch (chNum)
				{
				case '1':
					spec.ch = Trig_Channel_Spec::CH1;
					break;
				case '2':
					spec.ch = Trig_Channel_Spec::CH2;
					break;
				case '3':
					spec.ch = Trig_Channel_Spec::CH3;
					break;
				case '4':
					spec.ch = Trig_Channel_Spec::CH4;
					break;
				}
			}
		}
		else if (regex_match(strArg, smMatch, reCoup))
		{
			if (smMatch[1].matched)
				spec.coup = Trig_Coupling_Spec::AC;
			else if (smMatch[2].matched)
				spec.coup = Trig_Coupling_Spec::DC;
		}
		else if (regex_match(strArg, smMatch, reEdge))
		{
			if (smMatch[1].matched)
				spec.edge = Trig_Edge_Spec::RISING;
			else if (smMatch[2].matched)
				spec.edge = Trig_Edge_Spec::FALLING;
		}
		else if (regex_match(strArg, smMatch, reVoltage))
		{
			const string strBase = smMatch[1];
			const string strSuf = smMatch[2];

			spec.voltage = to_value(strBase, strSuf);
		}
		else
		{
			bResult = false;
			break;
		}
	}

	return bResult;
}



/*******************************************************************************
* Structure  : Meas_Spec
* Members    : vspec   = voltage measurement type (Meas_Voltage_Spec)
*              tspec   = time measurement type (Meas_Time_Spec)
* Description:
*   An object of this structue is passed by reference to EvalMeasSpec() to
*   receive the measurement specification parameters.
*/
enum class Meas_Voltage_Spec { UNSPEC, VPP, VPK };
enum class Meas_Time_Spec { UNSPEC, PHASE, DELAY };
struct Meas_Spec
{
	Meas_Voltage_Spec vspec;
	Meas_Time_Spec tspec;

	Meas_Spec() : vspec(Meas_Voltage_Spec::UNSPEC), tspec(Meas_Time_Spec::UNSPEC) {};
};


/*******************************************************************************
* Function   : EvalMeasSpec
* Arguments  : strSpec = string containing a comma-separated trigger specification
*              spec    = reference to target for specs contained in strSpec
* Returns    : true = success, false = failure
* Description:
*   This function evaluates the command line specification of the measurement.
*   ex/ VPP,phase
*/
bool EvalMeasSpec(string strSpec, Meas_Spec& spec)
{
	const regex reComma("^(.+?)(?:,(.*))?$");
	const regex reVtype("^(?:V?P(P)|V?P(K))$", regex::icase);  // VPP, PP, VPK, PK
	const regex reTtype("^(?:(P)HA(?:SE)?|(D)EL(?:AY)?)$", regex::icase);  // PHASE, PHA, DELAY, DEL

	bool bResult = true;
	smatch smMatch;

	// initialize to default return values
	spec.vspec = Meas_Voltage_Spec::UNSPEC;
	spec.tspec = Meas_Time_Spec::UNSPEC;

	while (!strSpec.empty())
	{
		if (!regex_match(strSpec, smMatch, reComma))
		{
			bResult = false;
			break;
		}

		// get the argument
		const string strArg = smMatch[1];

		// pass remainder on to the next loop until done
		strSpec = smMatch[2];

		// now, process the argument
		if (regex_match(strArg, smMatch, reVtype))
		{
			if (smMatch[1].matched)
			{
				spec.vspec = Meas_Voltage_Spec::VPP;
			}
			else if (smMatch[2].matched)
			{
				spec.vspec = Meas_Voltage_Spec::VPK;
			}
		}
		else if (regex_match(strArg, smMatch, reTtype))
		{
			if (smMatch[1].matched)
			{
				spec.tspec = Meas_Time_Spec::PHASE;
			}
			else if (smMatch[2].matched)
			{
				spec.tspec = Meas_Time_Spec::DELAY;
			}
		}
		else
		{
			bResult = false;
			break;
		}
	}

	return bResult;
}


/*******************************************************************************
* Function   : MeasureResponseParse
* Arguments  : argc, argv   = command line input
*              file     = receives file name and echo configuration
*              freq     = receives frequency range configuration
*              stim     = receives stimulus configuration
*              input    = receives input channel configuration
*              output   = receives output channel configuration
*              trig     = receives trigger configuration
*              meas     = receives measurement configuration
*              dwell    = receives dwell configuration
*              error    = contains error text if an error occurs
* Returns    : RETURN_SUCCESS = success, RETURN_(...) = failure
* Description:
*   This function parses the command line to determine frequency response
*   measurement parameters.
*/
int MeasureResponseParse
(
	int argc,
	char* argv[],
	File_Config& file,
	Freq_Config& freq,
	Stim_Config& stim,
	Channel_Config& input,
	Channel_Config& output,
	Trig_Config& trig,
	Meas_Config& meas,
	Dwell_Config& dwell,
	std::string& error
)
{
	// default error (none) is empty
	error = "";

	// default parameters unless overridden on the command line
	file = { true, "" };
	freq = { 1000.0, 10000.0, Sweep_t::LOG, 10 };
	stim = { 1, Vtype_t::VPP, 1.00, 0.00 };
	input = { 1, Ctype_t::AC, 10.0, true };
	output = { 2, Ctype_t::AC, 10.0, true };
	trig = { CH_TRIG_IN, Etype_t::RISE, Ctype_t::AC, 0.0 };
	meas = { Vtype_t::VPP, Ttype_t::PHASE };
	dwell = { 2.0, 500 };

	// regex patterns for parsing the command-line arguments
	const string str_numeric_pos = "(\\+?\\d*\\.?\\d*(?:E(?:\\+|-)?\\d{1,3})?)(K|M)?";
	const regex regex_oscope_ch("^(IN?|O(?:UT)?)(?::|=)(?:C|CH)?([1-4])(?:,(AC|DC|1X|10X|-?BWL?))?(?:,(AC|DC|1X|10X|-?BWL?))?(?:,(AC|DC|1X|10X|-?BWL?))?$", regex::icase);
	const regex regex_stim_spec("^S(?:TIM)?(?::|=)(.+)$", regex::icase);
	const regex regex_freq_spec("^F(?:REQ)?(?::|=)" + str_numeric_pos + "(?:HZ)?\\-" + str_numeric_pos + "(?:HZ)?(?:\\,(LOG|LIN)(?:\\(|\\[)([0-9]+)(?:\\)|\\]))?$", regex::icase);
	const regex regex_meas_spec("^M(?:EAS)?(?::|=)(.+)$", regex::icase);
	const regex regex_trig_spec("^T(?:RIG)?(?::|=)(.+)$", regex::icase);
	const regex regex_dwell_spec("^D(?:WELL)?(?::|=)(SLOW|MID|FAST|NORM(?:AL)?|DEF(?:AULT)?)$", regex::icase);
	const regex regex_log_spec("^(?:FILE|LOG|REP(?:ORT)?)(?::|=)(.+)$", regex::icase);

	// logging
	file.filename = "";		// log to filename
	file.is_echo = true;		// echo to cout

	for (int i = 1; i < argc; ++i)
	{
		string arg = argv[i];
		smatch smMatch;

		if (regex_match(arg, smMatch, regex_oscope_ch))
		{
			// set input or output oscilloscope channel number
			const string strInOut = smMatch[1];
			const string strCh = smMatch[2];

			bool bIn = str_compare_icase(strInOut, "IN");

			if (bIn)
				input.ch = stoi(strCh);
			else
				output.ch = stoi(strCh);

			// process the remaining arguments, which may be in any order
			for (int idx_flag = 3 ; idx_flag <= 5 ; ++idx_flag)
			{
				if (smMatch[idx_flag].matched)
				{
					const string str = smMatch[idx_flag];

					if (str_compare_icase(str, "AC") || str_compare_icase(str, "DC"))
					{
						const Ctype_t coup = (toupper(str[0]) == 'A') ? Ctype_t::AC : Ctype_t::DC;
						if (bIn)
							input.coup = coup;
						else
							output.coup = coup;
					}
					else if (str_compare_icase(str, "1X") || str_compare_icase(str, "10X"))
					{
						// 1X or 10X
						const double atten = (toupper(str[1]) == 'X') ? 1.0 : 10.0;  // either 1X or 10X
						if (bIn)
							input.atten = atten;
						else
							output.atten = atten;
					}
					else
					{
						// BWL or -BWL
						const bool is_bwl = (str[0] == '-') ? false : true;
						if (bIn)
							input.bwl = is_bwl;
						else
							output.bwl = is_bwl;
					}
				}
				else
				{
					break;
				}
			}
		}
		else if (regex_match(arg, smMatch, regex_stim_spec))
		{
			const string strStimSpec = smMatch[1];
			Stim_Spec spec;

			if (EvalStimSpec(strStimSpec, spec))
			{
				if (!isnan(spec.Vpp))
				{
					stim.vstim = spec.Vpp;
					stim.vtStim = Vtype_t::VPP;  // it has already been converted to Vpp

					if (!isnan(spec.Vdc))
						stim.vdc = spec.Vdc;
				}

				switch (spec.ch)
				{
				case Stim_Channel_Spec::S1:
					stim.ch = 1;
					break;
				case Stim_Channel_Spec::S2:
					stim.ch = 2;
					break;
				}
			}
			else
			{
				error = arg;
				return RETURN_SYNTAX_ERROR;
			}
		}
		else if (regex_match(arg, smMatch, regex_freq_spec))
		{
			// set start and stop frequencies
			const string strFstart = smMatch[1];
			const string strFstartSuf = smMatch[2];
			const string strFstop = smMatch[3];
			const string strFstopSuf = smMatch[4];

			freq.fStart = to_value(strFstart, strFstartSuf);
			freq.fStop = to_value(strFstop, strFstopSuf);

			if (smMatch[5].matched)
			{
				const string strSwp = smMatch[5];
				const string strPts = smMatch[6];

				if (str_compare_icase(strSwp, "LOG"))
					freq.sweep = Sweep_t::LOG;
				else
					freq.sweep = Sweep_t::LIN;

				if (!strPts.empty())
					freq.Npoints = stoi(strPts);
			}
		}
		else if (regex_match(arg, smMatch, regex_meas_spec))
		{
			// set the measurement type, either peak to peak or amplitude
			const string strMeas = smMatch[1];

			Meas_Spec spec;
			if (EvalMeasSpec(strMeas, spec))
			{
				switch (spec.vspec)
				{
				case Meas_Voltage_Spec::VPK:
					meas.vtMeas = Vtype_t::VPK;
					break;
				case Meas_Voltage_Spec::VPP:
					meas.vtMeas = Vtype_t::VPP;
					break;
				}

				switch (spec.tspec)
				{
				case Meas_Time_Spec::PHASE:
					meas.ttMeas = Ttype_t::PHASE;
					break;
				case Meas_Time_Spec::DELAY:
					meas.ttMeas = Ttype_t::DELAY;
					break;
				}
			}
			else
			{
				error = arg;
				return RETURN_SYNTAX_ERROR;
			}
		}
		else if (regex_match(arg, smMatch, regex_log_spec))
		{
			Log_Spec log_spec;
			if (EvalLogSpec(smMatch[1], log_spec))
			{
				if (!log_spec.strFilename.empty())
				{
					file.filename = log_spec.strFilename;
				}

				switch (log_spec.logConsole)
				{
				case Logfile_Console_Spec::ECHO:
					file.is_echo = true;
					break;
				case Logfile_Console_Spec::QUIET:
					file.is_echo = false;
					break;
				}
			}
			else
			{
				error = arg;
				return RETURN_SYNTAX_ERROR;
			}
		}
		else if (regex_match(arg, smMatch, regex_dwell_spec))
		{
			const string strDwell = smMatch[1];

			if (str_compare_icase(strDwell, "FAST"))
			{
				dwell.stable_screens = 1.5;
				dwell.minDwell_msec = 250;
			}
			else if (str_compare_icase(strDwell, "SLOW"))
			{
				dwell.stable_screens = 2.5;
				dwell.minDwell_msec = 1000;
			}
			else
			{
				dwell.stable_screens = 2.0;
				dwell.minDwell_msec = 500;
			}
		}
		else if (regex_match(arg, smMatch, regex_trig_spec))
		{
			const string strTrigSpec = smMatch[1];

			Trig_Spec spec;

			if (EvalTrigSpec(strTrigSpec, spec))
			{
				// only change the specs that were specified
				switch (spec.ch)
				{
				case Trig_Channel_Spec::CH1:
					trig.ch = 1;
					break;
				case Trig_Channel_Spec::CH2:
					trig.ch = 2;
					break;
				case Trig_Channel_Spec::CH3:
					trig.ch = 3;
					break;
				case Trig_Channel_Spec::CH4:
					trig.ch = 4;
					break;
				case Trig_Channel_Spec::CH_IN:
					trig.ch = CH_TRIG_IN;
					break;
				case Trig_Channel_Spec::CH_OUT:
					trig.ch = CH_TRIG_OUT;
					break;
				}
				switch (spec.coup)
				{
				case Trig_Coupling_Spec::AC:
					trig.coup = Ctype_t::AC;
					break;
				case Trig_Coupling_Spec::DC:
					trig.coup = Ctype_t::DC;
					break;
				}
				switch (spec.edge)
				{
				case Trig_Edge_Spec::RISING:
					trig.edge = Etype_t::RISE;
					break;
				case Trig_Edge_Spec::FALLING:
					trig.edge = Etype_t::FALL;
					break;
				}
				if (!isnan(spec.voltage))
				{
					trig.vTrig = spec.voltage;
				}
			}
			else
			{
				error = arg;
				return RETURN_SYNTAX_ERROR;
			}
		}
		else
		{
			error = arg;
			return RETURN_SYNTAX_ERROR;
		}
	}

	// If trig was on Input or Output, now is the time to assign it to the correct channel
	if (trig.ch == CH_TRIG_IN)
		trig.ch = input.ch;
	else if (trig.ch == CH_TRIG_OUT)
		trig.ch = output.ch;

	// sanity checks
	if (input.ch == output.ch)
	{
		error = "Input and Output oscilloscope channels cannot be identical\n";
		return RETURN_SETUP_ERROR;
	}

	if (freq.fStop <= freq.fStart)
	{
		error = "Stop frequency must be greater than start frequency\n";
		return RETURN_SETUP_ERROR;
	}

	if (freq.Npoints < 2)
	{
		error = "There must be at least 2 sample points specified\n";
		return RETURN_SETUP_ERROR;
	}

	if (stim.vstim <= 0.0)
	{
		error = "The stimulus amplitude must be greater than 0.0V\n";
		return RETURN_SETUP_ERROR;
	}

	return RETURN_SUCCESS;
}




/*******************************************************************************
* Function   : MeasureResponse()
* Arguments  : argc   = number of arguments, including the program name
*              argv[] = array of arguments
* Returns    : 0 = success, non-zero = failure
* Description:
*   This function is the entry point for the program.
*   It is intended to be called directly from main() or from a DLL
*/
int MeasureResponse(int argc, char* argv[])
{
	// default parameters will be loaded in call to MeasureResponseParse()
	File_Config file;
	Freq_Config freq;
	Stim_Config stim;
	Channel_Config input;
	Channel_Config output;
	Trig_Config trig;
	Meas_Config meas;
	Dwell_Config dwell;

	char szOscope[32];
	char szSigGen[32];

	// Get resources from registry or use defaults
	if (!FResp_ReadRegSZ(REGISTRY_KEY, "OscopeResource", szOscope, RESOURCE_DEFAULT_OSCOPE))
	{
		std::cerr << "Unable to determine oscilloscope resource. Check the registry.\n";
		return RETURN_RESOURCE_ERROR;
	}

	if (!FResp_ReadRegSZ(REGISTRY_KEY, "StimulusResource", szSigGen, RESOURCE_DEFAULT_SIGGEN))
	{
		std::cerr << "Unable to determine signal generator resource. Check the registry.\n";
		return RETURN_RESOURCE_ERROR;
	}


	if (argc < 2)
	{
		const string strProgName = (argc == 1) ? strip_path(argv[0]) : "FRESP.exe";
		return ExitPrintUsage(strProgName);
	}
	else
	{
		string error;
		int retval = MeasureResponseParse(argc, argv, file, freq, stim, input, output, trig, meas, dwell, error);

		// error checking
		switch (retval)
		{
		case RETURN_SUCCESS:
			break;
		case RETURN_SYNTAX_ERROR:
			std::cerr << "syntax error with argument: \"" << error << "\"\n";
			return RETURN_SYNTAX_ERROR;
		default:
			std::cerr << error;
			return retval;
		}

		// setup dual stream output
		//   one to std::cout
		//   the other to either a log file or to a null_stream
		ofstream my_file;
		if (!file.filename.empty())
		{
			// Check for extension/suffix .exe. Exit if it is .exe
			if (str_compare_icase(get_suffix(file.filename), ".exe"))
			{
				std::cerr << "Blocked writing to .exe file \"" << file.filename << "\"\n";
				return RETURN_BLOCKED_WRITE_EXE_FILE;
			}

			my_file.open(file.filename, ios::out | ios::trunc);
			if (!my_file.is_open())
			{
				std::cerr << "Unable to open file \"" << file.filename << "\" for write.\n";
				return RETURN_FILE_WRITE_ERROR;
			}
		}

		EchoDualStream my_dualstream(file.is_echo ? std::cout : EchoDualStream::null_stream, my_file.is_open() ? my_file : EchoDualStream::null_stream);

#ifdef DEBUG_WITHOUT_INSTRUMENTS
		my_dualstream << "Eleanor is sweet and Daddy loves her!!!\n";
		my_dualstream << "Test 1, 2, 3\n";
#else
		FRRET nRetVal;
		FRS result;
		FreqResp response;
		nRetVal = MeasureResponseAttach(szOscope, szSigGen, response, freq, stim, input, output, trig, meas, dwell);

		switch (nRetVal)
		{
		case FRRET_SUCCESS:
			break;
		case FRRET_INIT_OSCILLOSCOPE:
			cerr << "Unable to connect to oscilloscope\n";
			return RETURN_NO_CONNECT_OSCOPE;
		case FRRET_INIT_SINEGEN:
			cerr << "Unable to connecto to function generator\n";
			return RETURN_NO_CONNECT_SINEGEN;
		default:
			cerr << "Unexpected error (" << nRetVal << ")\n";
			return RETURN_ERROR;
		}

		// emit a header line
		my_dualstream << "freq\tinput\toutput\tgain\tdB\t";
		if (meas.ttMeas == Ttype_t::DELAY)
			my_dualstream << "delay";
		else
			my_dualstream << "phase";
		my_dualstream << "\n";

		do
		{
			nRetVal = MeasureResponseNext(response, result);
			if (nRetVal >= FRRET_SUCCESS)
			{
				my_dualstream << result.freq << "\t" << result.mag_in << "\t" << result.mag_out << "\t" << (result.mag_out / result.mag_in) << "\t" << result.dBgain << "\t" << result.time << "\n";
			}

		} while (nRetVal == FRRET_SUCCESS);  // will exit when FRRET_COMPLETE, or on an error

		switch (nRetVal)
		{
		case FRRET_COMPLETE:
			break;
		default:
			std::cerr << "Unexpected error (" << nRetVal << ")\n";
			return RETURN_ERROR;
		}
#endif

		my_file.close();
	}

	return RETURN_SUCCESS;
}


int MeasureResponseAttach(char const* szOscope, char const* szSigGen, FreqResp& response, Freq_Config const& freq, Stim_Config const& stim, Channel_Config const& input, Channel_Config const& output, Trig_Config const& trig, Meas_Config const& meas, Dwell_Config const& dwell)
{
	return response.Init(szOscope, szSigGen, freq, stim, input, output, trig, meas, dwell);
}


int MeasureResponseNext(FreqResp& response, FRS& result)
{
	return response.MeasureNext(result);
}

int MeasureResponseClose(FreqResp& response)
{
	return response.Close();
}


/*******************************************************************************
* Copyright © 2023 Kerry S. Martin, martin@wild-wood.net
* Free for usage without warranty, expressed or implied; attribution required
*******************************************************************************/