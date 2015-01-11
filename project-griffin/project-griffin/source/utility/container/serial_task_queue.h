/**
* @see https://github.com/sean-parent/scratch/blob/master/scratch/main.cpp
*/
#pragma once

#include <memory>
#include <functional>
#include <future>
#include <utility/container/concurrent_queue.h>

namespace griffin {

	template <typename T>
	class concurrent_list
	{
	public:
		using queue_type = list<T>;

		bool enqueue(T x)
		{
			bool was_dry;

			queue_type tmp;
			tmp.push_back(std::move(x));

			{
				std::lock_guard<std::mutex> lock(mutex);
				was_dry = was_dry_;
				was_dry_ = false;
				q_.splice(end(q_), tmp);
			}
			return was_dry;
		}

		queue_type dequeue()
		{
			queue_type result;

			{
				std::lock_guard<std::mutex> lock(mutex);
				if (q_.empty()) {
					was_dry_ = true;
				}
				else result.splice(end(result), q_, begin(q_));
			}

			return result;
		}

	private:
		std::mutex  mutex_;
		queue_type  q_;
		bool        was_dry_ = true;
	};


	class serial_task_queue {
	public:
		template <typename F, typename... Args>
		auto async(F&& f, Args&&... args) -> std::future<typename std::result_of<F(Args...)>::type>
		{
			using result_type = typename std::result_of<F(Args...)>::type;
			using packaged_type = std::packaged_task<result_type()>;

			auto p = std::make_shared<packaged_type>(std::bind(std::forward<F>(f), std::forward<Args>(args)...));
			auto result = p->get_future();

			if (shared_->enqueue([=](){
					(*p)();
					auto tmp = shared_->dequeue();
					if (!tmp.empty()) {
						std::async(std::move(tmp.front()));
					}
				}))
			{
				std::async(std::move(shared_->dequeue().front()));
			}

			return result;
		}

	private:
		using shared_type = concurrent_list<std::function<void()>>;

		std::shared_ptr<shared_type> shared_ = std::make_shared<shared_type>();
	};


	/**
	 */
	class task {
	public:
		void run() {}

	private:
	};


	/**
	 */
	class parallel_task_group {
	public:
		void run()
		{
			int count = 0;
			for (;;) {
				int index = m_index++;
				if (index >= m_tasks.size()) { break; }
				m_tasks[index].run();
				++count;
			}
			if (count > 0 && (m_index - count) == 0) {
				// add dependent tasks to run
			}
		}

	private:
		std::vector<task> m_tasks;
		std::atomic_int   m_index;
		std::atomic_int   m_remainingCount;
	};


	class task_scheduler {
	public:
	private:
		// has a thread pool

	};
}