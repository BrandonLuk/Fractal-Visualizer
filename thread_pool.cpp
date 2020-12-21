#include "thread_pool.h"

#include <functional>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>

ThreadPool::ThreadPool()
{
	terminate = false;
	for (int i = 0; i < std::thread::hardware_concurrency() - 1; ++i)
		pool.push_back(std::thread(&ThreadPool::threadWork, this));
	size = pool.size();
}

ThreadPool::~ThreadPool()
{
	joinThreads();
}


// Each thread will loop forever, waiting on the job queue for its next task.
// When it sees a task it will execute it.
void ThreadPool::threadWork()
{
	std::function<void()> job;

	while (true)
	{
		{
			std::unique_lock<std::mutex> lock(job_queue_mutex);
			job_queue_condition.wait(lock, [this] {return !job_queue.empty() || terminate; });
			if (terminate)
				return;

			job = job_queue.front();
			job_queue.pop();

		}
		job();
		active_jobs -= 1;
		if (active_jobs.load() == 0)
			synchronize_condition.notify_one();
	}
}

void ThreadPool::joinThreads()
{
	{
		std::unique_lock<std::mutex> lock(job_queue_mutex);
		terminate = true;
	}
	job_queue_condition.notify_all();

	for (std::thread& t : pool)
	{
		t.join();
	}
	pool.clear();
}

ThreadPool& ThreadPool::getInstance()
{
	static ThreadPool instance;
	return instance;
}

void ThreadPool::addJob(const std::function<void()> job)
{
	{
		std::unique_lock<std::mutex> lock(job_queue_mutex);
		job_queue.push(job);
	}
	active_jobs += 1;
	job_queue_condition.notify_one();
}

void ThreadPool::synchronize()
{
	std::unique_lock<std::mutex> lock(synchronize_mutex);
	synchronize_condition.wait(lock, [this] {return active_jobs.load() == 0 || terminate; });
}
