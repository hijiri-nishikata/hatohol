/*
 * Copyright (C) 2013 Project Hatohol
 *
 * This file is part of Hatohol.
 *
 * Hatohol is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * Hatohol is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Hatohol. If not, see <http://www.gnu.org/licenses/>.
 */

#include <semaphore.h>
#include <errno.h>
#include <stdexcept>
#include <MutexLock.h>
#include "UnifiedDataStore.h"
#include "VirtualDataStoreZabbix.h"
#include "VirtualDataStoreNagios.h"

using namespace mlpl;

struct UnifiedDataStore::PrivateContext
{
	const static size_t      maxRunningArms    = 8;
	const static time_t      minUpdateInterval = 10;
	static UnifiedDataStore *instance;
	static MutexLock         mutex;

	VirtualDataStoreZabbix *vdsZabbix;
	VirtualDataStoreNagios *vdsNagios;

	sem_t         updatedSemaphore;
	ReadWriteLock rwlock;
	timespec      lastUpdateTime;
	size_t        remainingArmsCount;
	ArmBaseVector updateArmsQueue;

	PrivateContext()
	: remainingArmsCount(0)
	{
		sem_init(&updatedSemaphore, 0, 0);
		lastUpdateTime.tv_sec  = 0;
		lastUpdateTime.tv_nsec = 0;
	};

	virtual ~PrivateContext()
	{
		sem_destroy(&updatedSemaphore);
	};

	void updatedCallback(void);
	void wakeArm(ArmBase *arm);
	bool updateIsNeeded(void);
};

UnifiedDataStore *UnifiedDataStore::PrivateContext::instance = NULL;
MutexLock UnifiedDataStore::PrivateContext::mutex;

// ---------------------------------------------------------------------------
// Public static methods
// ---------------------------------------------------------------------------
UnifiedDataStore *UnifiedDataStore::getInstance(void)
{
	if (PrivateContext::instance)
		return PrivateContext::instance;

	PrivateContext::mutex.lock();
	if (!PrivateContext::instance)
		PrivateContext::instance = new UnifiedDataStore();
	PrivateContext::mutex.unlock();

	return PrivateContext::instance;
}


// ---------------------------------------------------------------------------
// Public methods
// ---------------------------------------------------------------------------
UnifiedDataStore::UnifiedDataStore(void)
: m_ctx(NULL)
{
	m_ctx = new PrivateContext();

	// start VirtualDataStoreZabbix
	m_ctx->vdsZabbix = VirtualDataStoreZabbix::getInstance();

	// start VirtualDataStoreNagios
	m_ctx->vdsNagios = VirtualDataStoreNagios::getInstance();
}

UnifiedDataStore::~UnifiedDataStore()
{
	if (m_ctx)
		delete m_ctx;
}

void UnifiedDataStore::start(void)
{
	m_ctx->vdsZabbix->start();
	m_ctx->vdsNagios->start();
}

void UnifiedDataStore::stop(void)
{
	m_ctx->vdsZabbix->stop();
	m_ctx->vdsNagios->stop();
}

void
UnifiedDataStore::PrivateContext::wakeArm(ArmBase *arm)
{
	Closure<UnifiedDataStore::PrivateContext> *closure =
	  new Closure<UnifiedDataStore::PrivateContext>(
	    this, &UnifiedDataStore::PrivateContext::updatedCallback);
	arm->fetchItems(closure);
}

bool
UnifiedDataStore::PrivateContext::updateIsNeeded(void)
{
	bool shouldUpdate = true;

	rwlock.readLock();

	if (remainingArmsCount > 0) {
		shouldUpdate = false;
	} else {
		timespec ts;
		if (clock_gettime(CLOCK_REALTIME, &ts) == -1) {
			MLPL_ERR("Failed to call clock_gettime: %d\n", errno);
		} else if (ts.tv_sec < lastUpdateTime.tv_sec + minUpdateInterval) {
			shouldUpdate = false;
		}
	}

	rwlock.unlock();

	return shouldUpdate;
}

void
UnifiedDataStore::PrivateContext::updatedCallback(void)
{
	rwlock.writeLock();

	if (!updateArmsQueue.empty()) {
		wakeArm(updateArmsQueue.front());
		updateArmsQueue.erase(updateArmsQueue.begin());
	}

	remainingArmsCount--;
	if (remainingArmsCount == 0) {
		if (sem_post(&updatedSemaphore) == -1)
			MLPL_ERR("Failed to call sem_post: %d\n", errno);
		if (clock_gettime(CLOCK_REALTIME, &lastUpdateTime) == -1)
			MLPL_ERR("Failed to call clock_gettime: %d\n", errno);
	}

	rwlock.unlock();
}

void UnifiedDataStore::fetchItems(void)
{
	DBClientConfig dbConfig;
	if (!dbConfig.isCopyOnDemandEnabled())
		return;

	if (!m_ctx->updateIsNeeded())
		return;

	ArmBaseVector arms;
	m_ctx->rwlock.readLock();
	m_ctx->vdsZabbix->collectArms(arms);
	m_ctx->vdsNagios->collectArms(arms);
	m_ctx->rwlock.unlock();
	if (arms.empty())
		return;

	m_ctx->rwlock.writeLock();
	m_ctx->remainingArmsCount = arms.size();
	ArmBaseVectorIterator arms_it = arms.begin();
	for (size_t i = 0; arms_it != arms.end(); i++, arms_it++) {
		ArmBase *arm = *arms_it;
		if (i < PrivateContext::maxRunningArms) {
			m_ctx->wakeArm(arm);
		} else {
			m_ctx->updateArmsQueue.push_back(arm);
		}
	}
	m_ctx->rwlock.unlock();

	if (sem_wait(&m_ctx->updatedSemaphore) == -1)
		MLPL_ERR("Failed to call sem_wait: %d\n", errno);
}

void UnifiedDataStore::getTriggerList(TriggerInfoList &triggerList)
{
	DBClientHatohol dbHatohol;
	dbHatohol.getTriggerInfoList(triggerList);
}

void UnifiedDataStore::getEventList(EventInfoList &eventList)
{
	DBClientHatohol dbHatohol;
	dbHatohol.getEventInfoList(eventList);
}

void UnifiedDataStore::getItemList(ItemInfoList &itemList)
{
	fetchItems();
	DBClientHatohol dbHatohol;
	dbHatohol.getItemInfoList(itemList);
}


// ---------------------------------------------------------------------------
// Protected methods
// ---------------------------------------------------------------------------
