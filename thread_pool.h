#pragma once

#include <atomic>
#include <condition_variable>
#include <functional>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>

class ThreadPool
{
	bool terminate;
	std::mutex synchronize_mutex;
	std::condition_variable synchronize_condition;
	std::vector<std::thread> pool;

	std::atomic<int> active_jobs;
	std::mutex job_queue_mutex;
	std::condition_variable job_queue_condition;
	std::queue<std::function<void()>> job_queue;



	ThreadPool();
	~ThreadPool();

	void threadWork();
	void joinThreads();

public:
	int size;

	static ThreadPool& getInstance();
	void addJob(std::function<void()> job);
	void synchronize();

	ThreadPool(ThreadPool const&) = delete;
	void operator=(ThreadPool const&) = delete;
};

