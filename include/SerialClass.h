#ifndef SERIALCLASS_H_INCLUDED
#define SERIALCLASS_H_INCLUDED

#define ARDUINO_WAIT_TIME 2000

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string>

class Serial
{
private:
	//Serial comm handler
	HANDLE hSerial;
	//Connection status
	bool connected = false;
	//Get various information about the connection
	COMSTAT status;
	//Keep track of last error
	DWORD errors;
	//Keep COM port name
	std::string port;

public:
	//Initialize Serial communication with the given COM port
	Serial(const char *portName);
	//Close the connection
	~Serial();
	//Read data in a buffer, if nbChar is greater than the
	//maximum number of bytes available, it will return only the
	//bytes available. The function return -1 when nothing could
	//be read, the number of bytes actually read.
	int ReadData(char *buffer, unsigned int nbChar);
	//Writes data from a buffer through the Serial connection
	//return true on success.
	bool WriteData(char *buffer, unsigned int nbChar);
	//Check if we are actually connected
	bool IsConnected();
	//Get port name
	std::string GetName();
	// Available bytes
	unsigned int availableBytes();

};

#endif // SERIALCLASS_H_INCLUDED