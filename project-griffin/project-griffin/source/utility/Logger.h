#pragma once
#ifndef GRIFFIN_LOGGER_H_
#define GRIFFIN_LOGGER_H_

#include "container/concurrent_queue.h"
#include <vector>

namespace griffin {

	class Logger {
	public:
		static Logger& getInstance()
		{
			static Logger instance; // instantiated on first use, guaranteed to be destroyed
			return instance;
		}

		/**
		* Logging mode. Normally you'd set Immediate_Thread_Unsafe only for early initialization
		* before multiple threads are running, and Deferred_Thread_Safe while multiple threads are
		* running. If you use Immediate mode, SDL logging must have already been initialized before
		* calling any logging function. With Deferred mode, SDL must be initialized before the
		* first flush.
		*/
		enum Mode : uint8_t {
			Mode_Deferred_Thread_Safe = 0,	//<! (default) messages are queued and must be flushed, thread safe
			Mode_Immediate_Thread_Unsafe	//<! message written immediately, not thread safe
		};

		/**
		* Logging categories. Each category can target a different write destination and priority
		* level. Mirroring SDL, by default the Application category is enabled at the Info level,
		* the Assert category is enabled at the Warn level, Test is enabled at the Verbose level,
		* and all other categories are enabled at the Critical level.
		*/
		enum Category : uint8_t {
			Category_Application = 0, Category_Error, Category_Assert, Category_System,
			Category_Audio, Category_Video, Category_Render, Category_Input, Category_Test,
			_Category_Count
		};

		enum Priority : uint8_t {
			Priority_Off = 0, Priority_Critical, Priority_Error, Priority_Warn, Priority_Info, Priority_Debug, Priority_Verbose
		};

		inline void critical(const char *s) { log(Category_Application, Priority_Critical, s); }
		inline void error(const char *s)    { log(Category_Application, Priority_Error, s); }
		inline void warn(const char *s)     { log(Category_Application, Priority_Warn, s); }
		inline void info(const char *s)     { log(Category_Application, Priority_Info, s); }
		#ifdef _DEBUG
		inline void debug(const char *s)    { log(Category_Application, Priority_Debug, s); }
		#else
		inline void debug(const char *s)    {}
		#endif
		inline void verbose(const char *s)  { log(Category_Application, Priority_Verbose, s); }

		inline void critical(Category c, const char *s) { log(c, Priority_Critical, s); }
		inline void error(Category c, const char *s)    { log(c, Priority_Error, s); }
		inline void warn(Category c, const char *s)     { log(c, Priority_Warn, s); }
		inline void info(Category c, const char *s)     { log(c, Priority_Info, s); }
		#ifdef _DEBUG
		inline void debug(Category c, const char *s)    { log(c, Priority_Debug, s); }
		#else
		inline void debug(Category c, const char *s)    {}
		#endif
		inline void verbose(Category c, const char *s)  { log(c, Priority_Verbose, s); }

		inline void test(const char *s)					{ log(Category_Test, Priority_Info, s); }

		void log(Category c, Priority p, const char *s);

		/**
		* Empties the thread-safe queue and writes all messages. Call this at least once per frame
		* while in Deferred_Thread_Safe mode.
		*/
		void flush();

		/**
		* Setting the mode will also flush the queue. This call is not thread safe, call this only
		* from the same thread that normally calls flush.
		*/
		inline void setMode(Mode mode) {
			flush();
			m_mode = mode;
		}

		/**
		* Sets the priority for a category. This call is not thread safe, call this only
		* from the same thread that normally calls flush.
		*/
		void setPriority(Category c, Priority p);

		/**
		* Sets the priority for all categories. This call is not thread safe, call this only
		* from the same thread that normally calls flush.
		*/
		void setAllPriority(Priority p);

		/**
		* Does capacity check and final flush. Call prior to calling SDL quit.
		*/
		void deinit();

		/**
		* Prevent copy.
		*/
		Logger(const Logger &) = delete;
		void operator=(const Logger &) = delete;

	private:
		struct LogInfo {
			Category    category;
			Priority    priority;
			//uint8_t     level;
			//uint8_t     _padding_1[1];
			std::string message;
		};
		
		// Functions

		explicit Logger();

		void write(const LogInfo& l) const;

		// Variables

		Mode     m_mode = Mode_Deferred_Thread_Safe;
		Priority m_priority[_Category_Count];

		std::vector<LogInfo>      m_popArray;
		concurrent_queue<LogInfo> m_q;
	};

	#define log Logger::getInstance()
}

#endif