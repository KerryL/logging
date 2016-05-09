// File:  timeHistorylog.h
// Date:  9/4/2013
// Auth:  K. Loux
// Copy:  (c) Copyright 2013
// Desc:  Class for generating plottable time-history data, using measured system time
//        (accurate to milliseconds only) to generate time column.  NOT thread-safe.

#ifndef TIME_HISTORY_LOG_H_
#define TIME_HISTORY_LOG_H_

// Standard C++ headers
#include <iostream>
#include <vector>
#include <ctime>

// OS headers
#ifdef _WIN32
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <WinSock2.h>
#else
#include <sys/time.h>
#endif

// Local headers
#include "utilities/portable.h"

class TimeHistoryLog : public std::ostream
{
private:
	class TimeHistoryStreamBuffer : public std::stringbuf
	{
	public:
		TimeHistoryStreamBuffer(std::ostream &str, const char delimiter);
		~TimeHistoryStreamBuffer() {};

		virtual int sync();

		void MarkStartTime();
		void IncrementColumns();

		void SetNextTimeStamp(const double& time);

	private:
		std::ostream& output;
		const char delimiter;

		bool started;
		bool forcedTimeStamp;

		ULongLong start;
		unsigned int columns;
		double forcedTime;

		double GetTime();
		unsigned int GetColumnCount() const;
	} buffer;

	const char delimiter;
	bool headerWritten;
	std::vector<std::pair<std::string, std::string> > columnHeadings;

	void WriteHeader();

public:
	TimeHistoryLog(std::ostream& str, char delimiter = ',');
	~TimeHistoryLog() {};

	void AddColumn(std::string title, std::string units);
	void SetNextTimeStamp(const double& time);

	template<typename T>
	friend TimeHistoryLog& operator<<(TimeHistoryLog& log, T const& value);
};

//==========================================================================
// Class:			TimeHistoryLog (friend)
// Function:		operator<<
//
// Description:		Overload of stream insertion operator.  Handles writing
//					of column headings.
//
// Input Arguments:
//		log		= TimeHistoryLog&
//		value	= T const&
//
// Output Arguments:
//		None
//
// Return Value:
//		TimeHistoryLog&
//
//==========================================================================
template<typename T>
TimeHistoryLog& operator<<(TimeHistoryLog& log, T const& value)
{
	if (!log.headerWritten)
		log.WriteHeader();

	static_cast<std::ostream&>(log) << log.delimiter << value;

	return log;
}

#endif// TIME_HISTORY_LOG_H_
