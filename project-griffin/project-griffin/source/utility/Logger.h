#pragma once
#ifndef GRIFFIN_LOGGER_H_
#define GRIFFIN_LOGGER_H_

#include "container/concurrent_queue.h"
#include <vector>

namespace griffin {

	class Logger {
	public:
		/**
		* Logging mode. Normally you'd set Immediate_Thread_Unsafe for early initialization before
		* multiple threads are running, and Deferred_Thread_Safe while multiple threads are running.
		*/
		enum Mode : uint8_t {
			Mode_Immediate_Thread_Unsafe = 0, //<! (default) message written immediately, not thread safe
			Mode_Deferred_Thread_Safe         //<! messages are queued and must be flushed, thread safe
		};

		/**
		* Logging categories. Each category can target a different write destination, and 
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
		* Must be constructed after SDL init has been called.
		* Mirroring SDL, by default the Application category is enabled at the Info level,
		* the Assert category is enabled at the Warn level, Test is enabled at the Verbose level,
		* and all other categories are enabled at the Critical level.
		*/
		explicit Logger(Mode mode = Mode_Immediate_Thread_Unsafe);

		/**
		* Logger should be destructed prior to calling SDL quit.
		*/
		~Logger();

	private:
		struct LogInfo {
			Category    category;
			Priority    priority;
			//uint8_t     level;
			//uint8_t     _padding_1[1];
			std::string message;
		};

		void write(const LogInfo& l) const;

		// Variables

		Mode     m_mode;
		Priority m_priority[_Category_Count];

		std::vector<LogInfo>      m_popArray;
		concurrent_queue<LogInfo> m_q;
	};


	/**
	* Class for hierarchical logging support. This object can be created on the stack to manage the
	* logging of a single process, on a single thread. This object should not be shared between
	* different processes or threads since the level state (set by calls to push/pop) will start to
	* conflict. The explicit level setting makes it unnecessary to manually indent your logged
	* messages, and makes it possible for a GUI to render collapsible trees. 
	*/
	// TODO: need a thread-safe static counter with mutex for unique identity to maintain hierarchy
	/*
	class LevelLogger {
	public:
		explicit LevelLogger(Logger *pLogger) :
			m_pLogger{ pLogger }
		{}

		inline uint8_t pushLevel() {
			m_level += (m_level == 255 ? 0 : 1);
		}

		inline uint8_t popLevel() {
			m_level -= (m_level == 0 ? 0 : 1);
		}

	private:
		Logger* m_pLogger = nullptr;
		uint8_t m_level = 0;
	};
	*/
}

#endif