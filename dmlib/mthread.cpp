#include "mthread.h"
#include <cstring>

#if defined(_WIN32) || defined(_WIN64)
#define mcs_t CRITICAL_SECTION
#define mcs_initialize(o) ::InitializeCriticalSection(o)
#define mcs_lock(o) ::EnterCriticalSection(o)
#define mcs_unlock(o) ::LeaveCriticalSection(o)
#define mcs_destroy(o) ::DeleteCriticalSection(o)
#else
#define mcs_t pthread_mutex_t
#ifndef TEXT
#define TEXT(text) text
#endif
#define mcs_initialize(o) pthread_mutex_init(o,NULL)
#define mcs_lock(o) pthread_mutex_lock(o)
#define mcs_unlock(o) pthread_mutex_unlock(o)
#define mcs_destroy(o) pthread_mutex_destroy(o)
#endif

MSyncObject::MSyncObject() : m_obj(NULL), m_locked(false)
{
	m_obj = new mcs_t;
	mcs_initialize(reinterpret_cast<mcs_t*>(m_obj));
}

MSyncObject::~MSyncObject()
{
	Unlock();
	mcs_destroy(reinterpret_cast<mcs_t*>(m_obj));
	delete reinterpret_cast<mcs_t*>(m_obj);
	m_obj = NULL;
}

void MSyncObject::Lock()
{
	mcs_lock(reinterpret_cast<mcs_t*>(m_obj));
	m_locked = true;
}

void MSyncObject::Unlock()
{
	if(m_locked){
		m_locked = false;
		mcs_unlock(reinterpret_cast<mcs_t*>(m_obj));
	}
}

#if defined(_WIN32) || defined(_WIN64)
unsigned __stdcall MThread::TFunc(void* param)
{
	MThread* pThread = reinterpret_cast<MThread*>(param);
	pThread->Run();
	::_endthreadex(pThread->m_result);
	return pThread->m_result;
}
#else
void* MThread::TFunc(void* param)
{
	MThread* pThread = reinterpret_cast<MThread*>(param);
	pThread->Run();
	return reinterpret_cast<void*>(pThread->m_result);
}
#endif

MThread::MThread() : m_result(0), m_running(false)
{
	std::memset(&m_tid, 0, sizeof(mthread_t));
}

MThread::~MThread()
{
	Join();
}

void MThread::Start()
{
	if(m_running) return;
	m_sync.Lock();
	m_running = true;
	m_sync.Unlock();
#if defined(_WIN32) || defined(_WIN64)
	unsigned th;
	m_tid = reinterpret_cast<mthread_t>(::_beginthreadex(NULL,0,TFunc,this,0,&th));
	if(!m_tid){
		m_sync.Lock();
		m_running = false;
		m_sync.Unlock();
		throw TEXT("::_beginthreadex() error!!");
	}
#else
	if(pthread_create(&m_tid,NULL,TFunc,this)) throw TEXT("pthread_create() error!!");
#endif
}

void MThread::Join()
{
	if(!m_running) return;
#if defined(_WIN32) || defined(_WIN64)
	DWORD dwReturn;
	if(m_tid){
		dwReturn = ::WaitForSingleObject(m_tid, INFINITE);
		::CloseHandle(m_tid);
		m_tid = 0;
		if(dwReturn == WAIT_FAILED) throw TEXT("WaitForSingleObject() error!!");
	}
#else
	if(pthread_join(m_tid, NULL)) throw TEXT("pthread_join() error!!");
	std::memset(&m_tid, 0, sizeof(pthread_t));
#endif
	
	m_sync.Lock();
	m_running = false;
	m_sync.Unlock();
}

bool MThread::IsRunning()
{
	m_sync.Lock();
	bool result = m_running;
	m_sync.Unlock();
	return result;
}
