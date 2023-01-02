/*******************************************************************************
* Copyright © 2023 Kerry S. Martin, martin@wild-wood.net
* Free for usage without warranty, expressed or implied; attribution required
*
* Filename   : Socket_Instrument.cpp
* Class      : Socket_Instrument
* Description:
*   Implements the basic functionality to attach to and interface with an SCPI
*   instrument over a LAN using Winsock
*
* Created    : 11/05/2021
* Modified   : 01/02/2023
* Author     : Kerry S. Martin, martin@wild-wood.net
*******************************************************************************/

#include "Socket_Instrument.h"
#include <iostream>
#include <limits>

#pragma comment(lib, "Ws2_32.lib")

constexpr auto RECV_BUFLEN = 256;


using namespace std;

// define all static class variables
const double Socket_Instrument::DEFAULT_PARAM{ numeric_limits<double>::quiet_NaN() };
bool Socket_Instrument::bSocketsInitialized{ false };
int Socket_Instrument::nInstrAttached{ 0 };
WSADATA Socket_Instrument::wsaData{};


/*******************************************************************************
* Class      : Socket_Instrument
* Function   : InitSockets()
* Access     : private static
* Access     : private
* Arguments  : none
* Returns    : none
* Description:
*   Initializes the Windows Sockets DLL
*/
bool Socket_Instrument::InitSockets()
{
	if (!Socket_Instrument::bSocketsInitialized)
	{
		int iResult = WSAStartup(MAKEWORD(2, 2), &Socket_Instrument::wsaData);

		if (iResult == 0)
		{	// successfully initialized Windows Sockets DLL
			Socket_Instrument::bSocketsInitialized = true;
		}
	}

	return Socket_Instrument::bSocketsInitialized;
}



/*******************************************************************************
* Class      : Socket_Instrument
* Function   : CleanupSockets()
* Access     : private static
* Access     : private
* Arguments  : none
* Returns    : none
* Description:
*   Cleans up the Windows Sockets DLL
*/
bool Socket_Instrument::CleanupSockets()
{
	bool bresult = false;

	if (Socket_Instrument::nInstrAttached == 0)
	{
		WSACleanup();
		Socket_Instrument::bSocketsInitialized = false;

		bresult = true;
	}

	return bresult;
}


/*******************************************************************************
* Class      : Socket_Instrument
* Function   : Socket_Instrument() constructor
* Access     : public
* Arguments  : none
* Returns    : none
* Description:
*   Constructs a default (attached to no instrument) Socket_Instrument object
*/
Socket_Instrument::Socket_Instrument()
{
	Socket_Instrument::InitSockets();

	ZeroMemory(&hints, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;

	connected_socket = INVALID_SOCKET;

	bAttached = false;
}


/*******************************************************************************
* Class      : Socket_Instrument
* Function   : ~Socket_Instrument() destructor
* Access     : public
* Arguments  : none
* Returns    : none
* Description:
*   Destroys a Socket_Instrument object, first detaching any attached instruments
*/
Socket_Instrument::~Socket_Instrument()
{
	Detach();

	if (Socket_Instrument::nInstrAttached == 0)
		Socket_Instrument::CleanupSockets();
}


/*******************************************************************************
* Class      : Socket_Instrument
* Function   : Attach() using resource name
* Access     : public
* Arguments  : resource = resource name string for instrument (ex/ "192.168.0.197:5025")
* Returns    : true if successful (instrument was attached), false if not
* Description:
*   Attaches to an instrument using the given resource name
*/
bool Socket_Instrument::Attach(string resource)
{
	bool retval = false;

	if (bAttached)
		Detach();

	if (bSocketsInitialized)
	{
		string addr, port;

		if (Socket_Instrument::Extract_Addr_Port(resource, addr, port))
		{
			struct addrinfo* result;

			if (getaddrinfo(addr.c_str(), port.c_str(), &hints, &result) == 0)
			{
				struct addrinfo* ptr = result;
				connected_socket = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);

				if (connected_socket != INVALID_SOCKET)
				{	// socket was created, connect to it
					if (connect(connected_socket, ptr->ai_addr, int(ptr->ai_addrlen)) != SOCKET_ERROR)
					{
						bAttached = true;
						Socket_Instrument::nInstrAttached += 1;
						retval = true;
					}
				}

				freeaddrinfo(result);
			}
		}
	}

	return retval;
}



/*******************************************************************************
* Class      : Socket_Instrument
* Function   : Detach()
* Access     : public
* Arguments  : none
* Returns    : always returns true
* Description:
*   Detaches from an instrument
*/
bool Socket_Instrument::Detach()
{
	bool bResult = true;

	if (bAttached)
	{
		shutdown(connected_socket, SD_SEND);
		closesocket(connected_socket);

		bAttached = false;
		Socket_Instrument::nInstrAttached -= 1;
	}

	return bResult;
}


/*******************************************************************************
* Class      : Socket_Instrument
* Function   : Write()
* Access     : public
* Arguments  : command = string with the command to write to the instrument
* Returns    : returns true if the write was successful
* Description:
*   Writes the given command to the instrument. Appends \n if necessary.
*/
bool Socket_Instrument::Write(std::string command)
{
	bool retval = false;

	if (!EndsWithNewline(command))
		command = command + '\n';

	if (send(connected_socket, command.c_str(), (int)command.length(), 0) != SOCKET_ERROR)
		retval = true;

	return retval;
}


/*******************************************************************************
* Class      : Socket_Instrument
* Function   : WriteEx()
* Access     : public
* Arguments  : command = string with the command to write to the instrument
* Returns    : returns true if the write was successful
* Description:
*   Writes the given command to the instrument exactly as provided. Newline is
*   not appended.
*/
bool Socket_Instrument::WriteEx(std::string exact_command)
{
	bool retval = false;

	if (send(connected_socket, exact_command.c_str(), (int)exact_command.length(), 0) != SOCKET_ERROR)
		retval = true;

	return retval;
}


/*******************************************************************************
* Class      : Socket_Instrument
* Function   : Query()
* Access     : public
* Arguments  : command  = string with the command to write to the instrument
*              response = (reference) receives the response from the instrument
* Returns    : returns true if the query was successful
* Description:
*   Writes the given command to the instrument, and receives the response.
*   Appends \n to the command if necessary.
*/
bool Socket_Instrument::Query(std::string command, std::string& response)
{
	bool retval = false;
	char recv_buffer[RECV_BUFLEN];

	if (Write(command))
	{
		int bytes_received = recv(connected_socket, recv_buffer, RECV_BUFLEN, 0);
		if (bytes_received > 0)
		{
			response = std::string(recv_buffer, bytes_received);
			retval = true;
		}
	}

	return retval;
}


/*******************************************************************************
* Class      : Socket_Instrument
* Function   : EndsWithNewline()
* Access     : protected static
* Arguments  : input = string to check
* Returns    : returns true if the string was terminated with a newline (\n)
* Description:
*   Checks to see if a string is terminated with a newline (\n) character.
*/
bool Socket_Instrument::EndsWithNewline(std::string const input)
{
	size_t pos = input.find_last_of('\n');

	if (pos == string::npos)
		return false;
	else if (pos + 1 == input.length())
		return true;
	else
		return false;
}


/*******************************************************************************
* Class      : Socket_Instrument
* Function   : Extract_Addr_Port()
* Access     : protected static
* Arguments  : resource = resource IP:port identifier (see Notes)
*              addr     = reference to string receiving IP address
*              port     = reference to string receiving port
* Returns    : returns true if the IP address and port could be extracted
* Description:
*   Extracts the IP address and port from a resource identifer with one of the
*   following accepted formats:
*      192.168.0.197:5025
*      http://192.168.0.197:5025
*      http://192.168.0.197:5025/
*/
bool Socket_Instrument::Extract_Addr_Port(std::string const resource, std::string& addr, std::string& port)
{
	bool bresult = false;

	const regex reIP_Port("^(?:[a-zA-Z]+://)?([0-9]{1,3}\\.[0-9]{1,3}\\.[0-9]{1,3}\\.[0-9]{1,3}):([0-9]{1,5})/?$");
	smatch smMatch;

	if (regex_match(resource, smMatch, reIP_Port))
	{
		addr = smMatch[1];
		port = smMatch[2];
		bresult = true;
	}

	return bresult;
}


/*******************************************************************************
* Copyright © 2023 Kerry S. Martin, martin@wild-wood.net
* Free for usage without warranty, expressed or implied; attribution required
*******************************************************************************/