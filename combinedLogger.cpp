// File:  combinedLogger.cpp
// Date:  9/3/2013
// Auth:  K. Loux
// Copy:  (c) Kerry Loux 2013
// Desc:  Logging object that permits writing to multiple logs simultaneously
//        and from multiple threads.

// Standard C++ headers
#include <cassert>

// Local headers
#include "combinedLogger.h"

//==========================================================================
// Class:			CombinedLogger
// Function:		Static members
//
// Description:		Static members for CombinedLogger.
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
std::mutex CombinedLogger::logMutex;

//==========================================================================
// Class:			CombinedLogger
// Function:		Add
//
// Description:		Adds a log sink to the vector.
//
// Input Arguments:
//		log				= std::unique_ptr<std::ostream>
//		manageMemory	= bool
//
// Output Arguments:
//		None
//
// Return Value:
//		None
//
//==========================================================================
void CombinedLogger::Add(std::unique_ptr<std::ostream> log, bool manageMemory)
{
	assert(log);
	std::lock_guard<std::mutex> lock(logMutex);
	logs.push_back(std::make_pair(std::move(log), manageMemory));
}

//==========================================================================
// Class:			CombinedLogger::CombinedStreamBuffer
// Function:		Static members
//
// Description:		Static members for CombinedLogger::CombinedStreamBuffer.
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
std::mutex CombinedLogger::CombinedStreamBuffer::bufferMutex;

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
	CreateThreadBuffer();
	if (c != traits_type::eof())
		*threadBuffer[std::this_thread::get_id()] << (char)c;

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
	assert(log.logs.size() > 0);// Make sure we didn't forget to add logs

	CreateThreadBuffer();// Before mutex locker, because this might lock the mutex, too
	std::lock_guard<std::mutex> lock(bufferMutex);
			
	unsigned int i;
	for (i = 0; i < log.logs.size(); i++)
	{
		*log.logs[i].first << threadBuffer[std::this_thread::get_id()]->str();
		log.logs[i].first->flush();
	}

	// Clear out the buffers
	threadBuffer[std::this_thread::get_id()]->str("");
	str("");

	return 0;
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
	if (threadBuffer.find(std::this_thread::get_id()) == threadBuffer.end())
	{
		std::lock_guard<std::mutex> lock(bufferMutex);
		if (threadBuffer.find(std::this_thread::get_id()) == threadBuffer.end())
			threadBuffer[std::this_thread::get_id()] = std::make_unique<std::stringstream>();
	}
}
