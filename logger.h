// File:  logger.h
// Date:  9/3/2013
// Auth:  K. Loux
// Copy:  (c) Copyright 2013
// Desc:  Generic logging object.  Note that this class is NOT thread-safe!

#ifndef LOGGER_H_
#define LOGGER_H_

// Standard C++ headers
#include <iostream>
#include <sstream>

class Logger : public std::ostream
{
private:
	class LoggerStreamBuffer : public std::stringbuf
	{
	public:
		LoggerStreamBuffer(std::ostream &str) : output(str) {};
		~LoggerStreamBuffer() {};

		virtual int sync(void);

	private:
		std::ostream& output;
		static std::string GetTimeStamp(void);
	} buffer;

public:
	Logger(std::ostream& str) : std::ostream(&buffer), buffer(str) {};
	virtual ~Logger() {};
};

#endif// LOGGER_H_
