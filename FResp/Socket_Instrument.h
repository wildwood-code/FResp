/*******************************************************************************
* Copyright © 2021 Kerry S. Martin, martin@wild-wood.net
* Free for usage without warranty, expressed or implied; attribution required
*
* Filename   : Socket_Instrument.h
* Class      : Socket_Instrument
* Description:
*   Implements the basic functionality to attach to and interface with an SCPI
*   instrument over a LAN using Winsock
*
* Created    : 11/05/2021
* Modified   : 11/05/2021
* Author     : Kerry S. Martin, martin@wild-wood.net
*******************************************************************************/
#pragma once
#include <string>
#include <vector>
#include <regex>
#include <winsock2.h>
#include <ws2tcpip.h>



class Socket_Instrument
{
public:
	static const double DEFAULT_PARAM;

private:
	// data type definition
	struct addrinfo hints;
	bool bAttached;
	SOCKET connected_socket;

public:
	// Construction and destruction
	Socket_Instrument();
	virtual ~Socket_Instrument();

	// Deleted copy and assignment operators (they could break the RA table)
	// Copy and assignment operators in derived classes will also be deleted
	Socket_Instrument& operator = (const Socket_Instrument&) = delete;
	Socket_Instrument(const Socket_Instrument&) = delete;

	// These may be used in the Socket_Instrument class stand-alone
	// In a function derived from Socket_Instrument, these must be overloaded to be useful
	virtual bool Attach(std::string resource);
	virtual bool Detach();

	// these are to be hidden/protected in any derived class
	// command will be appended with a newline character if it is not already present
	// exact_command will be written exactly as passed, with no added newline
	bool Write(std::string command);
	bool WriteEx(std::string exact_command);
	bool Query(std::string command, std::string& response);

protected:
	//static bool FindInstrument(std::regex pattern, std::string& ident, std::string& resource);
	static bool EndsWithNewline(std::string const input);
	static bool Extract_Addr_Port(std::string const resource, std::string& addr, std::string& port);

private:
	static bool bSocketsInitialized;
	static int nInstrAttached;
	static WSADATA wsaData;

	static bool InitSockets();
	static bool CleanupSockets();
};


/*******************************************************************************
* Copyright © 2021 Kerry S. Martin, martin@wild-wood.net
* Free for usage without warranty, expressed or implied; attribution required
*******************************************************************************/