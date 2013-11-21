// File:  timeHistorylog.cpp
// Date:  9/4/2013
// Auth:  K. Loux
// Copy:  (c) Copyright 2013
// Desc:  Class for generating plottable time-history data, using measured system time
//        (down to microseconds - unsure of accuracy) to generate time column.  NOT thread-safe.

// Standard C++ headers
#include <sstream>
#include <cassert>

// *nix headers
#include <unistd.h>

// Local headers
#include "timeHistoryLog.h"

//==========================================================================
// Class:			TimeHistoryLog
// Function:		TimeHistoryLog
//
// Description:		Constructor for TimeHistoryLog class.
//
// Input Arguments:
//		str			= std::ostream& to write to
//		delimiter	= char
//
// Output Arguments:
//		None
//
// Return Value:
//		TimeHistoryLog&
//
//==========================================================================
TimeHistoryLog::TimeHistoryLog(std::ostream& str, char delimiter) : std::ostream(&buffer),
	buffer(str, delimiter), delimiter(delimiter)
{
	headerWritten = false;
}

//==========================================================================
// Class:			TimeHistoryLog
// Function:		WriteHeader
//
// Description:		Writes the header information to the stream.
//
// Input Arguments:
//		None
//
// Output Arguments:
//		None
//
// Return Value:
//		None
//
//==========================================================================
void TimeHistoryLog::WriteHeader(void)
{
	assert(!headerWritten);
	headerWritten = true;

	unsigned int i;

	static_cast<std::ostream&>(*this) << "Time";
	for (i = 0; i < columnHeadings.size(); i++)
		*this << columnHeadings[i].first;
	*this << std::endl;

	static_cast<std::ostream&>(*this) << "[sec]";
	for (i = 0; i < columnHeadings.size(); i++)
		*this << columnHeadings[i].second;
	*this << std::endl;

	buffer.MarkStartTime();
}

//==========================================================================
// Class:			TimeHistoryLog
// Function:		AddColumn
//
// Description:		Adds information for the next column to the string.
//
// Input Arguments:
//		title	= std::string
//		units	= std::string
//
// Output Arguments:
//		None
//
// Return Value:
//		None
//
//==========================================================================
void TimeHistoryLog::AddColumn(std::string title, std::string units)
{
	assert(!headerWritten);
	columnHeadings.push_back(std::make_pair(title, "[" + units + "]"));
	buffer.IncrementColumns();
}

//==========================================================================
// Class:			TimeHistoryLog::TimeHistoryStreamBuffer
// Function:		TimeHistoryStreamBuffer
//
// Description:		Constructor for TimeHistoryStreamBuffer class.
//
// Input Arguments:
//		str			= std::ostream&
//		delimiter	= const char
//
// Output Arguments:
//		None
//
// Return Value:
//		None
//
//==========================================================================
TimeHistoryLog::TimeHistoryStreamBuffer::TimeHistoryStreamBuffer(std::ostream &str,
	const char delimiter) : output(str), delimiter(delimiter)
{
	started = false;
	columns = 1;
}

//==========================================================================
// Class:			TimeHistoryLog::TimeHistoryStreamBuffer
// Function:		GetTime
//
// Description:		Returns a string containing the current time value.
//
// Input Arguments:
//		None
//
// Output Arguments:
//		None
//
// Return Value:
//		double
//
//==========================================================================
double TimeHistoryLog::TimeHistoryStreamBuffer::GetTime(void)
{
	double elapsed;
	struct timeval now;
	if (gettimeofday(&now, NULL) == -1)
		std::cout << "Failed to get current time" << std::endl;

	// All in one statement to prevent underflows in the usec calculation
	elapsed = now.tv_sec - start.tv_sec + now.tv_usec * 1.0e-6 - start.tv_usec * 1.0e-6;

	return elapsed;
}

//==========================================================================
// Class:			TimeHistoryLog::TimeHistoryStreamBuffer
// Function:		MarkStartTime
//
// Description:		Indicates that the log should mark the current time as
//					"zero".
//
// Input Arguments:
//		None
//
// Output Arguments:
//		None
//
// Return Value:
//		None
//
//==========================================================================
void TimeHistoryLog::TimeHistoryStreamBuffer::MarkStartTime(void)
{
	if (gettimeofday(&start, NULL) == -1)
		std::cout << "Failed to mark start time" << std::endl;
	started = true;
}

//==========================================================================
// Class:			TimeHistoryLog::TimeHistoryStreamBuffer
// Function:		GetColumnCount
//
// Description:		Returns the number of columns in the current buffer.
//
// Input Arguments:
//		None
//
// Output Arguments:
//		None
//
// Return Value:
//		unsigned int
//
//==========================================================================
unsigned int TimeHistoryLog::TimeHistoryStreamBuffer::GetColumnCount(void) const
{
	std::string line(str());
	unsigned int i(1), next(0);
	while ((next = line.find(delimiter, next)) != std::string::npos)
	{
		i++;
		next++;
	}

	return i;
}

//==========================================================================
// Class:			TimeHistoryLog::TimeHistoryStreamBuffer
// Function:		IncrementColumns
//
// Description:		Increments the number of columns we should be looking for.
//
// Input Arguments:
//		None
//
// Output Arguments:
//		None
//
// Return Value:
//		unsigned int
//
//==========================================================================
void TimeHistoryLog::TimeHistoryStreamBuffer::IncrementColumns(void)
{
	assert(!started);
	columns++;
}

//==========================================================================
// Class:			TimeHistoryLog::TimeHistoryStreamBuffer
// Function:		sync
//
// Description:		Overrides the standard sync function.  It is called when
//					an endl object is passed to the stream.  This handles the
//					insertion of the timestamp.
//
// Input Arguments:
//		None
//
// Output Arguments:
//		None
//
// Return Value:
//		unsigned int
//
//==========================================================================
int TimeHistoryLog::TimeHistoryStreamBuffer::sync(void)
{
	assert(GetColumnCount() == columns);

	// NOTE:  Time stamp is generated when std::endl is passed (or stream is flushed),
	//        thus, when used, it is recommended to pass all data and the endl as close
	//        together as possible (i.e. the data in the first column may be older than
	//        the data in the last column, but they will share a common time stamp).
	if (started)
		output << GetTime() << str();
	else
		output << str();
	str("");
	output.flush();
	return 0;
}
