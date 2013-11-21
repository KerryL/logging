// File:  combinedLogger.h
// Date:  9/3/2013
// Auth:  K. Loux
// Copy:  (c) Copyright 2013
// Desc:  Logging object that permits writing to multiple logs simultaneously
//        and from multiple threads.

#ifndef COMBINED_LOGGER_H_
#define COMBINED_LOGGER_H_

// pThread headers (must be first!)
#include <pthread.h>

// Standard C++ headers
#include <iostream>
#include <map>
#include <vector>
#include <sstream>

// Local headers
#include "mutexLocker.h"

class CombinedLogger : public std::ostream
{
public:
	CombinedLogger() : std::ostream(&buffer), buffer(*this) {};
	virtual ~CombinedLogger();

	void Add(std::ostream* log, bool manageMemory = true);

private:
	class CombinedStreamBuffer : public std::stringbuf
	{
	public:
		CombinedStreamBuffer(CombinedLogger &log) : log(log) {};
		virtual ~CombinedStreamBuffer();

	protected:
		virtual int overflow(int c);
		virtual int sync(void);

	private:
		CombinedLogger &log;
		std::map<pthread_t, std::stringstream*> threadBuffer;
		static pthread_mutex_t mutex;
		void CreateThreadBuffer(void);
	} buffer;

	static CombinedLogger *logger;
	static pthread_mutex_t mutex;

	std::vector<std::pair<std::ostream*, bool> > logs;
};

#endif// COMBINED_LOGGER_H_
