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
#include <chrono>
#include <cassert>
#include <algorithm>

template<class StreamType>
class CombinedLogger : public StreamType
{
public:
	explicit CombinedLogger() : StreamType(&buffer), buffer(*this) {}
	virtual ~CombinedLogger() = default;

	void Add(std::unique_ptr<StreamType> log);
	void Add(StreamType& log);

private:
	typedef typename StreamType::char_type CharType;
	class CombinedStreamBuffer : public std::basic_streambuf<CharType>
	{
	public:
		explicit CombinedStreamBuffer(CombinedLogger &log) : log(log) {}
		virtual ~CombinedStreamBuffer() = default;

	protected:
		typedef typename std::basic_streambuf<typename StreamType::char_type>::int_type IntType;
		typename IntType overflow(IntType c) override;
		int sync() override;

	private:
		CombinedLogger &log;

		typedef std::chrono::steady_clock Clock;
		struct Buffer
		{
			Buffer(std::unique_ptr<std::lock_guard<std::mutex>>& mutex);

			std::basic_ostringstream<CharType> ss;
			std::mutex mutex;// protectes ss and this
			Clock::time_point lastFlushTime = Clock::now();
		};

		typedef std::map<std::thread::id, std::unique_ptr<Buffer>> BufferMap;
		BufferMap threadBuffer;
		std::mutex bufferMapMutex;// Protects threadBuffer
		std::unique_ptr<std::lock_guard<std::mutex>> CreateThreadBuffer(const bool& bufferMapMutexAlreadyLocked = false);

		static const Clock::duration idleThreadTimeThreshold;
		static const unsigned int maxCleanupCount;
		unsigned int cleanupCount = 0;

		void CleanupBuffers();
	} buffer;

	std::mutex logMutex;// Protects ownedLogs and allLogs

	std::vector<std::unique_ptr<StreamType>> ownedLogs;
	std::vector<StreamType*> allLogs;
};

//==========================================================================
// Class:			CombinedLogger
// Function:		Add
//
// Description:		Adds a log sink to the vector.  Takes ownership of the log.
//
// Input Arguments:
//		log		= std::unique_ptr<std::ostream>
//
// Output Arguments:
//		None
//
// Return Value:
//		None
//
//==========================================================================
template<class StreamType>
void CombinedLogger<StreamType>::Add(std::unique_ptr<StreamType> log)
{
	assert(log);
	std::lock_guard<std::mutex> lock(logMutex);
	ownedLogs.push_back(std::move(log));
	allLogs.push_back(ownedLogs.back().get());
}

//==========================================================================
// Class:			CombinedLogger
// Function:		Add
//
// Description:		Adds a log sink to the vector.  Caller retains ownership.
//
// Input Arguments:
//		log		= std::ostream&
//
// Output Arguments:
//		None
//
// Return Value:
//		None
//
//==========================================================================
template<class StreamType>
void CombinedLogger<StreamType>::Add(StreamType& log)
{
	assert(log);
	std::lock_guard<std::mutex> lock(logMutex);
	allLogs.push_back(&log);
}

//==========================================================================
// Class:			CombinedLogger::CombinedStreamBuffer
// Function:		Static definitions
//
// Description:		Static member definitions.
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
template<class StreamType>
const unsigned int CombinedLogger<StreamType>::CombinedStreamBuffer::maxCleanupCount(100);
template<class StreamType>
const CombinedLogger<StreamType>::CombinedStreamBuffer::Clock::duration
	CombinedLogger<StreamType>::CombinedStreamBuffer::idleThreadTimeThreshold(std::chrono::minutes(2));

//==========================================================================
// Class:			CombinedLogger::CombinedStreamBuffer::Buffer
// Function:		Buffer
//
// Description:		Constructor for buffer class.
//
// Input Arguments:
//		lock	= std::unique_ptr<std::lock_guard<std::mutex>>&
//
// Output Arguments:
//		None
//
// Return Value:
//		None
//
//==========================================================================
template<class StreamType>
CombinedLogger<StreamType>::CombinedStreamBuffer::Buffer::Buffer(
	std::unique_ptr<std::lock_guard<std::mutex>>& lock)
{
	lock = std::make_unique<std::lock_guard<std::mutex>>(mutex);
}

//==========================================================================
// Class:			CombinedLogger::CombinedStreamBuffer
// Function:		overflow
//
// Description:		Override of the standard overflow method.  Called when
//					new data is inserted into the stream.  This is overridden
//					so we can control which buffer the data is inserted into.
//					The buffers are controlled on a per-thread basis.
//
// Input Arguments:
//		c	= int
//
// Output Arguments:
//		None
//
// Return Value:
//		int
//
//==========================================================================
template<class StreamType>
typename CombinedLogger<StreamType>::CombinedStreamBuffer::IntType CombinedLogger<StreamType>::CombinedStreamBuffer::overflow(IntType c)
{
	auto lock(CreateThreadBuffer());// TODO:  Improve efficiency?  Instead of locking/unlocking for every character?
	if (c != traits_type::eof())
	{
		// Allow other threads to continue to buffer to the stream, even if
		// another thread is writing to the logs in sync() (so we don't lock
		// the buffer mutex here)
		const auto& tb(threadBuffer);
		tb.find(std::this_thread::get_id())->second->ss << static_cast<CharType>(c);
	}

	return c;
}

//==========================================================================
// Class:			CombinedLogger::CombinedStreamBuffer
// Function:		sync
//
// Description:		Override of the standard sync method.  Called when an
//					endl object is passed to the stream.  This prints the
//					contents of the current thread's buffer to the output
//					stream, then clears the buffer.
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
template<class StreamType>
int CombinedLogger<StreamType>::CombinedStreamBuffer::sync()
{
	assert(log.allLogs.size() > 0);// Make sure we didn't forget to add logs

	int result(0);
	const std::thread::id id(std::this_thread::get_id());

	std::lock_guard<std::mutex> lock(bufferMapMutex);
	auto localLock(CreateThreadBuffer(true));

	{
		std::lock_guard<std::mutex> logLock(log.logMutex);

		for (auto& l : log.allLogs)
		{
			if ((*l << threadBuffer[id]->ss.str()).fail())
				result = -1;
			l->flush();
		}
	}

	if (++cleanupCount == maxCleanupCount)
	{
		CleanupBuffers();
		cleanupCount = 0;
	}

	// Clear out the buffer
	threadBuffer[id]->ss.str(_T(""));

	return result;
}

//==========================================================================
// Class:			CombinedLogger::CombinedStreamBuffer
// Function:		CreateThreadBuffer
//
// Description:		Checks for existance of a buffer for the current thread,
//					and creates the buffer if it doesn't exist.
//
// Input Arguments:
//		bufferMapMutexAlreadyLocked = const bool&
//
// Output Arguments:
//		None
//
// Return Value:
//		None
//
//==========================================================================
template<class StreamType>
std::unique_ptr<std::lock_guard<std::mutex>> CombinedLogger<StreamType>::CombinedStreamBuffer::CreateThreadBuffer(const bool& bufferMapMutexAlreadyLocked)
{
	std::unique_ptr<std::lock_guard<std::mutex>> returnLock;

	const std::thread::id id(std::this_thread::get_id());
	if (threadBuffer.find(id) == threadBuffer.end())
	{
		if (!bufferMapMutexAlreadyLocked)
		{
			std::lock_guard<std::mutex> lock(bufferMapMutex);
			threadBuffer[id] = std::make_unique<Buffer>(returnLock);
		}
		else
			threadBuffer[id] = std::make_unique<Buffer>(returnLock);
	}
	else
		returnLock = std::make_unique<std::lock_guard<std::mutex>>(threadBuffer[id]->mutex);

	return returnLock;
}

//==========================================================================
// Class:			CombinedLogger::CombinedStreamBuffer
// Function:		CleanupBuffers
//
// Description:		Removes buffers from the map if the thread is no joinable.  This
//					risks an unnecessary removal/recration of buffers for threads
//					that are still actively writing to the log, but it reduces
//					the risk of growing the size of the map as a result of dead
//					threads.
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
template<class StreamType>
void CombinedLogger<StreamType>::CombinedStreamBuffer::CleanupBuffers()
{
	// NOTE:  Caller is assumed to have already locked bufferMutex
	std::vector<std::unique_lock<std::mutex>> locks;// TODO:  Is this OK?  Or do we also need to move the associated mutex objects?
	const Clock::time_point now(Clock::now());

	auto needsCleanup([&locks, &now, this](const BufferMap::value_type& b)
	{
		std::unique_lock<std::mutex> lock(b.second->mutex, std::try_to_lock);
		if (!lock.owns_lock())// Failed to lock mutex - buffer must still be active
			return false;

		if (b.second->ss.str().empty() && now - b.second->lastFlushTime > idleThreadTimeThreshold)
		{
			locks.push_back(std::move(lock));
			return true;
		}

		return false;
	});

	auto it(threadBuffer.begin());
	for (; it != threadBuffer.end();)
	{
		if (needsCleanup(*it))
			it = threadBuffer.erase(it);
		else
			++it;
	}
}

#endif// COMBINED_LOGGER_H_
