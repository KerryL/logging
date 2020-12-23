// File:  logger.h
// Date:  9/3/2013
// Auth:  K. Loux
// Copy:  (c) Copyright 2013
// Desc:  Generic logging object.  Note that this class is NOT thread-safe!

#ifndef LOGGER_H_
#define LOGGER_H_

// utilities headers
#include <utilities/uString.h>

// Standard C++ headers
#include <iostream>
#include <sstream>

class Logger : public UString::OStream
{
private:
	class LoggerStreamBuffer : public UString::StringBuf
	{
	public:
		explicit LoggerStreamBuffer(UString::OStream &str) : output(str) {}

		int sync() override;

	private:
		UString::OStream& output;
		static UString::String GetTimeStamp();
	} buffer;

public:
	explicit Logger(UString::OStream& str) : UString::OStream(&buffer), buffer(str) {}
	virtual ~Logger() = default;
};

#endif// LOGGER_H_
