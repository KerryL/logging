// File:  combinedLogger.cpp
// Date:  9/3/2013
// Auth:  K. Loux
// Copy:  (c) Kerry Loux 2013
// Desc:  Logging object that permits writing to multiple logs simultaneously
//        and from multiple threads.

// Standard C++ headers
#include <cassert>
#include <algorithm>

// Local headers
#include "combinedLogger.h"

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
void CombinedLogger::Add(std::unique_ptr<std::ostream> log)
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
void CombinedLogger::Add(std::ostream& log)
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
const unsigned int CombinedLogger::CombinedStreamBuffer::maxCleanupCount(1000);
const CombinedLogger::CombinedStreamBuffer::Clock::duration
	CombinedLogger::CombinedStreamBuffer::idleThreadTimeThreshold(std::chrono::minutes(5));

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
int CombinedLogger::CombinedStreamBuffer::overflow(int c)
{
	CreateThreadBuffer();// TODO:  after this call, our local mutex should be locked!!
	if (c != traits_type::eof())
	{
		// Allow other threads to continue to buffer to the stream, even if
		// another thread is writing to the logs in sync() (so we don't lock
		// the buffer mutex here)
		// TODO:  lock local buffer mutex first
		const auto& tb(threadBuffer);
		tb.find(std::this_thread::get_id())->second->ss << static_cast<char>(c);
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
int CombinedLogger::CombinedStreamBuffer::sync()
{
	assert(log.allLogs.size() > 0);// Make sure we didn't forget to add logs

	int result(0);
	const std::thread::id id(std::this_thread::get_id());

	CreateThreadBuffer();// Before locking bufferMutex, because this might lock the mutex, too// TODO:  after this call, our local mutex should be locked!!
	std::lock_guard<std::mutex> lock(bufferMutex);

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
		CleanupBuffers();

	// Clear out the buffer
	threadBuffer[id]->ss.str("");

	// Verify that we didn't do something to allow the normal internal
	// buffer to be touched - all input should go to our controlled buffers
	assert(str().empty());

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
//		None
//
// Output Arguments:
//		None
//
// Return Value:
//		None
//
//==========================================================================
void CombinedLogger::CombinedStreamBuffer::CreateThreadBuffer()
{
	const std::thread::id id(std::this_thread::get_id());
	if (threadBuffer.find(id) == threadBuffer.end())
	{
		std::lock_guard<std::mutex> lock(bufferMutex);
		if (threadBuffer.find(id) == threadBuffer.end())// TODO:  Not sure we need this check
			threadBuffer[id] = std::make_unique<Buffer>();
	}
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
void CombinedLogger::CombinedStreamBuffer::CleanupBuffers()
{
	// NOTE:  Caller is assumed to have already locked bufferMutex
	std::vector<std::lock_guard<std::mutex>> locks;// TODO:  Is this OK?  Or do we also need to move the associated mutex objects?
	const Clock::time_point now(Clock::now());
	threadBuffer.erase(std::remove_if(threadBuffer.begin(), threadBuffer.end(),
		[&locks, &now, this](const std::pair<std::thread::id, std::unique_ptr<Buffer>>& b)
	{
		std::lock_guard<std::mutex> lock(b.second->mutex);
		if (b.second->ss.str().empty() && now - b.second->lastFlushTime > idleThreadTimeThreshold)
		{
			locks.push_back(std::move(lock));
			return true;
		}

		return false;
	}));
}
