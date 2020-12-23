// File:  logger.h
// Date:  9/3/2013
// Auth:  K. Loux
// Copy:  (c) Copyright 2013
// Desc:  Generic logging object.  Note that this class is NOT thread-safe!

// Standard C++ headers
#include <ctime>
#include <iomanip>

// Local headers
#include "logger.h"

//==========================================================================
// Class:			Logger::LoggerStreamBuffer
// Function:		GetTimeStamp
//
// Description:		Returns a time stamp string to prepend to each message.
//
// Input Arguments:
//		None
//
// Output Arguments:
//		None
//
// Return Value:
//		UString::String containing the formatted time stamp
//
//==========================================================================
UString::String Logger::LoggerStreamBuffer::GetTimeStamp()
{
	const time_t now(time(nullptr));
	const struct tm* timeInfo(localtime(&now));

	UString::OStringStream timeStamp;
	timeStamp.fill('0');
	timeStamp << timeInfo->tm_year + 1900 << "-"
		<< std::setw(2) << timeInfo->tm_mon + 1 << "-"
		<< std::setw(2) << timeInfo->tm_mday << " "
		<< std::setw(2) << timeInfo->tm_hour << ":"
		<< std::setw(2) << timeInfo->tm_min << ":"
		<< std::setw(2) << timeInfo->tm_sec;

	return timeStamp.str();
}

//==========================================================================
// Class:			Logger::LoggerStreamBuffer
// Function:		sync
//
// Description:		Override of standard sync method.  Called when stream encounters
//					endl object.  Handles addition of timestamp to the stream.
//
// Input Arguments:
//		None
//
// Output Arguments:
//		None
//
// Return Value:
//		int
//
//==========================================================================
int Logger::LoggerStreamBuffer::sync()
{
	output << GetTimeStamp() << " : " << str();
	str(_T(""));
	output.flush();
	return 0;
}