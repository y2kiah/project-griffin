/**
* @see https://github.com/sean-parent/scratch/blob/master/scratch/main.cpp
*/
#pragma once

#include <memory>
#include <functional>
#include <future>

namespace griffin {

	// this uses GCD it appears so replace this with std::async or use MS PPL
	template <typename F, typename ...Args>
	auto async(F&& f, Args&&... args) -> std::future<typename std::result_of<F(Args...)>::type>
	{
		using result_type = typename std::result_of<F(Args...)>::type;
		using packaged_type = std::packaged_task<result_type()>;

		auto p = new packaged_type(std::forward<F>(f), std::forward<Args>(args)...);
		auto result = p->get_future();

		dispatch_async_f(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0),
			p, [](void* f_) {
			packaged_type* f = static_cast<packaged_type*>(f_);
			(*f)();
			delete f;
		});

		return result;
	}


	template <typename T>
	class concurrent_list
	{
	public:
		using queue_type = list<T>;

		bool enqueue(T x)
		{
			bool was_dry;

			queue_type tmp;
			tmp.push_back(move(x));

			{
				std::lock_guard<std::mutex> lock(mutex);
				was_dry = was_dry_; was_dry_ = false;
				q_.splice(end(q_), tmp);
			}
			return was_dry;
		}

		queue_type deque()
		{
			queue_type result;

			{
				std::lock_guard<std::mutex> lock(mutex);
				if (q_.empty()) was_dry_ = true;
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
					queue_type tmp = shared_->deque();
					if (!tmp.empty()) {
						async(std::move(tmp.front()));
					}
				}))
			{
				async(std::move(shared_->deque().front()));
			}

			return result;
		}

	private:
		using shared_type = concurrent_list<std::function<void()>>;

		std::shared_ptr<shared_type> shared_ = std::make_shared<shared_type>();
	};

}