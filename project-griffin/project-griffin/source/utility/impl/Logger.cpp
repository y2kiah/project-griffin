#include "../Logger.h"
#include <SDL_log.h>
#include "../memory_reserve.h"

using namespace griffin;

Logger::Logger(Mode mode) :
	m_mode{ mode },
	m_q(RESERVE_LOGGER_QUEUE)
{
	m_popArray.reserve(RESERVE_LOGGER_QUEUE);

	// VS2013 doesn't support array initializers (yet)
	m_priority[Category_Application] = Priority_Info;
	m_priority[Category_Error]       = Priority_Critical;
	m_priority[Category_Assert]      = Priority_Warn;
	m_priority[Category_System]      = Priority_Critical;
	m_priority[Category_Audio]       = Priority_Critical;
	m_priority[Category_Video]       = Priority_Critical;
	m_priority[Category_Render]      = Priority_Critical;
	m_priority[Category_Input]       = Priority_Critical;
	m_priority[Category_Test]        = Priority_Verbose;
}

Logger::~Logger() {
	flush();

	if (m_q.unsafe_capacity() > RESERVE_LOGGER_QUEUE) {
		SDL_Log("check RESERVE_LOGGER_QUEUE: original=%d, highest=%d", RESERVE_LOGGER_QUEUE, m_q.unsafe_capacity());
	}
}

void Logger::log(Category c, Priority p, const char *s)
{
	if (m_priority[c] >= p) {
		if (m_mode == Mode_Deferred_Thread_Safe) {
			m_q.push({ c, p, s });
		}
		else {
			write({ c, p, s });
		}
	}
}

void Logger::flush()
{
	m_q.try_pop_all(m_popArray);
	for (auto& li : m_popArray) {
		write(li);
	}
	m_popArray.clear();
}

void Logger::setPriority(Category c, Priority p)
{
	m_priority[c] = p;
}

void Logger::setAllPriority(Priority p)
{
	memset(m_priority, p, sizeof(m_priority));
}

void Logger::write(const LogInfo& li) const
{
	auto sdlPriority = static_cast<SDL_LogPriority>(li.priority == 0 ? 0 : SDL_NUM_LOG_PRIORITIES - li.priority);
	SDL_LogMessage(li.category, sdlPriority, li.message.c_str());
}
