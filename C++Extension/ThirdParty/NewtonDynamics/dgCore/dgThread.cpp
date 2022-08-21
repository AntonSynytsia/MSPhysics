/* Copyright (c) <2003-2019> <Julio Jerez, Newton Game Dynamics>
* 
* This software is provided 'as-is', without any express or implied
* warranty. In no event will the authors be held liable for any damages
* arising from the use of this software.
* 
* Permission is granted to anyone to use this software for any purpose,
* including commercial applications, and to alter it and redistribute it
* freely, subject to the following restrictions:
* 
* 1. The origin of this software must not be misrepresented; you must not
* claim that you wrote the original software. If you use this software
* in a product, an acknowledgment in the product documentation would be
* appreciated but is not required.
* 
* 2. Altered source versions must be plainly marked as such, and must not be
* misrepresented as being the original software.
* 
* 3. This notice may not be removed or altered from any source distribution.
*/

#include "dgStdafx.h"
#include "dgThread.h"
#include "dgProfiler.h"

dgThread::dgThread ()
	:m_id(0)
	,m_terminate(0)
	,m_threadRunning(0)
{
	m_name[0] = 0;
}

dgThread::dgThread (const char* const name, dgInt32 id)
	:m_id(id)
	,m_terminate(0)
	,m_threadRunning(0)
{
	strncpy (m_name, name, sizeof (m_name) - 1);
}

void dgThread::Init (const char* const name, dgInt32 id)
{
	m_id = id;
	strncpy (m_name, name, sizeof (m_name) - 1);
	Init ();
}


bool dgThread::IsThreadActive() const
{
	return m_threadRunning ? true : false;
}

#ifdef DG_USE_THREAD_EMULATION

dgThread::dgSemaphore::dgSemaphore ()
{
	m_sem = 0;
}

dgThread::dgSemaphore::~dgSemaphore ()
{
}

void dgThread::dgSemaphore::Release ()
{
}

void dgThread::dgSemaphore::Wait()
{
}

dgThread::~dgThread ()
{
}

void dgThread::Init ()
{
}

void dgThread::Close ()
{
}

//void dgThread::Wait (dgSemaphore& mutex)
//{
//}

void dgThread::Wait (dgInt32 count, dgSemaphore* const semArray)
{
}

void* dgThread::dgThreadSystemCallback(void* threadData)
{
	return 0;
}

#else  

dgThread::dgSemaphore::dgSemaphore ()
	:m_count(0)
{
}

dgThread::dgSemaphore::~dgSemaphore ()
{
}

void dgThread::dgSemaphore::Release ()
{
	std::unique_lock <std::mutex> lck(m_mutex);
	m_count ++;
	m_sem.notify_one();
}

void dgThread::dgSemaphore::Wait()
{
	std::unique_lock <std::mutex> lck(m_mutex);
	dgAssert (m_count >= 0);
	while (m_count == 0)
	{
		m_sem.wait(lck);
	}
	m_count --;
}

dgThread::~dgThread ()
{
}


void dgThread::SetName()
{
#if defined(_MSC_VER)
	// a hideous way to set the thread name, bu this is how Microsoft does it
	const DWORD MS_VC_EXCEPTION = 0x406D1388;
	#pragma pack(push,8)  
	struct THREADNAME_INFO
	{
		DWORD dwType; // Must be 0x1000.  
		LPCSTR szName; // Pointer to name (in user addr space).  
		DWORD dwThreadID; // Thread ID (-1=caller thread).  
		DWORD dwFlags; // Reserved for future use, must be zero.  
	};
	#pragma pack(pop)  


	THREADNAME_INFO info;
	info.dwType = 0x1000;
	info.szName = m_name;
	info.dwThreadID = GetThreadId(m_handle.native_handle());
	info.dwFlags = 0;
	__try {
		RaiseException(MS_VC_EXCEPTION, 0, sizeof(info) / sizeof(ULONG_PTR), (ULONG_PTR*)&info);
	}
	__except (EXCEPTION_EXECUTE_HANDLER)
	{
	}
#endif
}


void dgThread::Init ()
{
	// This must be set now because otherwise if this thread is
	// immediately closed the Terminate method won't detect that the
	// thread is running
	dgInterlockedExchange(&m_threadRunning, 0);
	m_handle = std::thread(dgThreadSystemCallback, this);
	// wait until the thread procedure is called.
	while (!m_threadRunning) {
		dgThreadYield();
	}

	SetName();
}

void dgThread::Close ()
{
	m_handle.join();
}

void dgThread::Wait (dgInt32 count, dgSemaphore* const semArray)
{
	for (dgInt32 i = 0; i < count; i ++) {
		semArray[i].Wait();
	}
}


void* dgThread::dgThreadSystemCallback(void* threadData)
{
	dgFloatExceptions exception;
	dgSetPrecisionDouble precision;

	dgThread* const me = (dgThread*) threadData;

	D_SET_TRACK_NAME(me->m_name);
	dgInterlockedExchange(&me->m_threadRunning, 1);
	me->Execute(me->m_id);
	dgInterlockedExchange(&me->m_threadRunning, 0);

	return 0;
}


#endif



