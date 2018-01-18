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
		explicit LoggerStreamBuffer(std::ostream &str) : output(str) {}

		int sync() override;

	private:
		std::ostream& output;
		static std::string GetTimeStamp();
	} buffer;

public:
	explicit Logger(std::ostream& str) : std::ostream(&buffer), buffer(str) {}
	virtual ~Logger() = default;
};

#endif// LOGGER_H_
