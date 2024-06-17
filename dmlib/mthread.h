#ifndef __INC_MTHREAD_H__
#define __INC_MTHREAD_H__
#if defined(_WIN32) || defined(_WIN64)
#include <windows.h>
#include <process.h>
typedef HANDLE mthread_t;
#else
#include <pthread.h>
typedef pthread_t mthread_t;
#endif

class MSyncObject{
	void* m_obj;
	bool m_locked;
public:
	MSyncObject();
	void Lock();
	void Unlock();
	~MSyncObject();
};

// ƒXƒŒƒbƒh
class MThread{
	mthread_t m_tid;
	unsigned m_result;
	bool m_running;
	MSyncObject m_sync;
protected:
#if defined(_WIN32) || defined(_WIN64)
	static unsigned __stdcall TFunc(void* param);
	unsigned (__stdcall *m_func)(void*);
#else
	static void* TFunc(void* param);
	void* (*m_func)(void*);
#endif
public:
	MThread();
	virtual ~MThread();
	virtual void Start();
	virtual void Run() = 0;
	virtual void Join();
	virtual mthread_t GetThreadId()const{ return m_tid; } // tid:HANDLE/pthread_t
	virtual unsigned GetResult()const{ return m_result; }
	virtual void SetResult(unsigned result){ m_result = result; }
	virtual bool IsRunning();
};

#endif // __INC_MTHREAD_H__
