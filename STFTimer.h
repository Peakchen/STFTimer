#pragma once
#ifndef __STF_TIMER__
#define __STF_TIMER__

#include <map>
#if defined(__LINUX__)
#include <pthread.h>
#include <sys/time.h>
#elif defined(__WINDOWS__)
#include <windows.h>
#include <mutex>
#include <time.h>
#endif

struct TTimerProcess
{
#if defined(__LINUX__)
	virtual void OnTimer(int nTimerID) = 0;
#elif defined(__WINDOWS__)
	virtual void OnTimer(int nTimerID) = 0;
#endif
};

struct TSTFTimer
{
	struct TTimerNode
	{
		int nTimerID;
		char szModuleName[128];
		unsigned long ulInterval;	//首次时间	当前时间 + 时间间隔
		int deltatimes;				//时间间隔
		int nRunTimes;
		bool isUnlimitedNum;
		TTimerProcess* pTimer;
		TTimerNode* pNext;

		TTimerNode(){
			nTimerID = 0;
			memset(szModuleName, 0, sizeof(szModuleName));
			pTimer = NULL;
			ulInterval = 0;
			nRunTimes = 0;
			pNext = NULL;
			isUnlimitedNum = false;
		}

	};

	struct TTimerManager
	{
		TTimerNode* phead;
		TTimerManager()
		{
			phead = NULL;
		}
	};

	TSTFTimer()
	{
		//m_mapTimer.clear;
	}

#if defined(__LINUX__)
	void OnTimerLoop(int nTimerID) {

	}
#elif defined(__WINDOWS__)

	DWORD WINAPI OnTimerLoop(LPVOID lparamer) {
		TTimerNode* pNode = m_timerInfo.phead;
		while (pNode)
		{
			int isubTime = sleepTime();
			bool bcheckTimes = (pNode->nRunTimes > 0 || pNode->isUnlimitedNum);
			if (isubTime <= 0 && bcheckTimes)
			{
				m_timerMutex.lock();
				TTimerProcess* pTimer = pNode->pTimer;
				pTimer->OnTimer(pNode->nTimerID);
				--pNode->nRunTimes;
				pNode->ulInterval = time(NULL) + pNode->deltatimes;
				m_timerMutex.unlock();
			}
		}
	}

#endif

	inline int sleepTime() {
		TTimerNode* phead = m_timerInfo.phead;
		SYSTEMTIME tv;
		GetLocalTime(&tv);
		return (int)(phead->ulInterval - tv.wSecond - tv.wMilliseconds / 1000);
	}

	inline bool compareMintime(TTimerNode* dstnode, TTimerNode* pnewNode) {
		return pnewNode->ulInterval > dstnode->ulInterval;
	}

	inline void insertTimerNode(TTimerNode* node) {

		TTimerNode* pNewNode = m_timerInfo.phead;
		if (!pNewNode || compareMintime(node, pNewNode))
		{
			node->pNext = pNewNode;
			m_timerInfo.phead = node;

			m_timerMutex.unlock();
			return;
		}

		// as first node
		while (pNewNode->pNext)
		{
			if (compareMintime(node, pNewNode->pNext))
			{
				break;
			}

			pNewNode = pNewNode->pNext;
		}

		node->pNext = pNewNode->pNext;
		pNewNode->pNext = node;
		return;
	}

	inline void SetTimer(int nTimerID, unsigned long ulInterval, int nRunTimes, TTimerProcess* pTimer, const char* szModuleName) {
		
		if (nTimerID < 0 || ulInterval <= 0 || nRunTimes < 0 || pTimer == NULL)
		{
			return;
		}

		// 1. set and save timer
		TTimerNode stTimerInfo;
		stTimerInfo.nTimerID	= nTimerID;
		stTimerInfo.pTimer		= pTimer;
		stTimerInfo.ulInterval	= ulInterval + time(NULL);
		stTimerInfo.deltatimes  = ulInterval;
		stTimerInfo.nRunTimes	= nRunTimes;
		stTimerInfo.isUnlimitedNum = nRunTimes == 0 ? true : false;
		strcpy_s(stTimerInfo.szModuleName, szModuleName);
		stTimerInfo.pNext		= NULL;
		
		// 2. then start timer thread
		m_timerMutex.lock();
		insertTimerNode(&stTimerInfo);
		m_timerMutex.unlock();

#if defined(__LINUX__)
		pthread_t threadTimer;
		pthread_create(&threadTimer, NULL, OnTimerLoop, NULL);
#elif defined(__WINDOWS__)
		HANDLE hTimerthread = CreateThread(NULL, 0, this->OnTimerLoop, (LPVOID)this, 0, NULL);
#endif
			
	}

	inline void KillTimer(int nTimerID, TSTFTimer* pTimer) {

	}

public:
	typedef std::map<int, TTimerNode> TModuleTimerMap;
	TModuleTimerMap m_mapTimer;							// 弃用，使用 链表

	std::mutex m_timerMutex;
	TTimerManager m_timerInfo;
};

#endif