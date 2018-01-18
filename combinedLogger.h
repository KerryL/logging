// File:  combinedLogger.h
// Date:  9/3/2013
// Auth:  K. Loux
// Copy:  (c) Copyright 2013
// Desc:  Logging object that permits writing to multiple logs simultaneously
//        and from multiple threads.

#ifndef COMBINED_LOGGER_H_
#define COMBINED_LOGGER_H_

// Standard C++ headers
#include <iostream>
#include <map>
#include <vector>
#include <sstream>
#include <thread>
#include <mutex>
#include <memory>

class CombinedLogger : public std::ostream
{
public:
	explicit CombinedLogger() : std::ostream(&buffer), buffer(*this) {}
	virtual ~CombinedLogger() = default;

	void Add(std::unique_ptr<std::ostream> log);
	void Add(std::ostream& log);

private:
	class CombinedStreamBuffer : public std::stringbuf
	{
	public:
		explicit CombinedStreamBuffer(CombinedLogger &log);
		virtual ~CombinedStreamBuffer() = default;

	protected:
		int overflow(int c) override;
		int sync() override;

	private:
		CombinedLogger &log;
		std::map<std::thread::id, std::unique_ptr<std::ostringstream>> threadBuffer;
		std::mutex bufferMutex;
		void CreateThreadBuffer();
	} buffer;

	std::mutex logMutex;

	std::vector<std::unique_ptr<std::ostream>> ownedLogs;
	std::vector<std::ostream*> allLogs;
};

#endif// COMBINED_LOGGER_H_
