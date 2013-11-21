// File:  combinedLogger.h
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
// Function:		None
//
// Description:		Static member variable initialization.
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
CombinedLogger *CombinedLogger::logger = NULL;
pthread_mutex_t CombinedLogger::mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t CombinedLogger::CombinedStreamBuffer::mutex = PTHREAD_MUTEX_INITIALIZER;

//==========================================================================
// Class:			CombinedLogger
// Function:		~CombinedLogger
//
// Description:		Destructor for CombinedLogger class.
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
CombinedLogger::~CombinedLogger()
{
	// No lock here - users must ensure that object is deleted by only one thread!
	unsigned int i;
	for (i = 0; i < logs.size(); i++)
	{
		if (logs[i].second)
			delete logs[i].first;
	}

	int errorNumber;
	if ((errorNumber = pthread_mutex_destroy(&mutex)) != 0)
		std::cout << "Error destroying mutex (" << errorNumber << ")" << std::endl;
}

//==========================================================================
// Class:			CombinedLogger
// Function:		Add
//
// Description:		Adds a log sink to the vector.
//
// Input Arguments:
//		log				= std::ostream*
//		manageMemory	= bool
//
// Output Arguments:
//		None
//
// Return Value:
//		None
//
//==========================================================================
void CombinedLogger::Add(std::ostream* log, bool manageMemory)
{
	assert(log);
	MutexLocker lock(mutex);
	logs.push_back(std::make_pair(log, manageMemory));
}

//==========================================================================
// Class:			CombinedLogger::CombinedStreamBuffer
// Function:		~CombinedStreamBuffer
//
// Description:		Destructor for CombinedStreamBuffer class.
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
CombinedLogger::CombinedStreamBuffer::~CombinedStreamBuffer()
{
	std::map<pthread_t, std::stringstream*>::iterator it;
	for (it = threadBuffer.begin(); it != threadBuffer.end(); it++)
		delete it->second;

	int errorNumber;
	if ((errorNumber = pthread_mutex_destroy(&mutex)) != 0)
		std::cout << "Error destroying mutex (" << errorNumber << ")" << std::endl;
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
int CombinedLogger::CombinedStreamBuffer::overflow(int c)
{
	CreateThreadBuffer();
	if (c != traits_type::eof())
		*threadBuffer[pthread_self()] << (char)c;

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
int CombinedLogger::CombinedStreamBuffer::sync(void)
{
	assert(log.logs.size() > 0);// Make sure we didn't forget to add logs

	CreateThreadBuffer();// Before mutex locker, because this might lock the mutex, too
	MutexLocker lock(mutex);
			
	unsigned int i;
	for (i = 0; i < log.logs.size(); i++)
	{
		*log.logs[i].first << threadBuffer[pthread_self()]->str();
		log.logs[i].first->flush();
	}

	// Clear out the buffers
	threadBuffer[pthread_self()]->str("");
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
void CombinedLogger::CombinedStreamBuffer::CreateThreadBuffer(void)
{
	if (threadBuffer.find(pthread_self()) == threadBuffer.end())
	{
		MutexLocker lock(mutex);
		if (threadBuffer.find(pthread_self()) == threadBuffer.end())
			threadBuffer[pthread_self()] = new std::stringstream;
	}
}