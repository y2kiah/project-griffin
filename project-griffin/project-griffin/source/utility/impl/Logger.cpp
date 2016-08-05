#include "../Logger.h"
#include "../memory_reserve.h"
#include <cstdarg>
#include <SDL_log.h>

using namespace griffin;

Logger::Logger() :
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

void Logger::log(Category c, Priority p, const char *s, va_list args)
{
	// write formatted string
	int len = _vscprintf(s, args);
	std::string str(len, 0);
	vsnprintf_s(&str[0], len+1, len, s, args);

	if (m_priority[c] >= p) {
		if (m_mode == Mode_Deferred_Thread_Safe) {
			m_q.push({ c, p, std::move(str) });
		}
		else {
			write({ c, p, std::move(str) });
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

void Logger::write(const LogInfo& li) const
{
	auto sdlPriority = static_cast<SDL_LogPriority>(li.priority == 0 ? 0 : SDL_NUM_LOG_PRIORITIES - li.priority);
	SDL_LogMessage(li.category, sdlPriority, li.message.c_str());
}

void Logger::setPriority(Category c, Priority p)
{
	m_priority[c] = p;
}

void Logger::setAllPriority(Priority p)
{
	memset(m_priority, p, sizeof(m_priority));
}

void Logger::deinit()
{
	flush();

	if (m_q.unsafe_capacity() > RESERVE_LOGGER_QUEUE) {
		SDL_Log("check RESERVE_LOGGER_QUEUE: original=%d, highest=%d", RESERVE_LOGGER_QUEUE, m_q.unsafe_capacity());
	}
}


#define pass_args(f)	va_list args;\
						va_start(args, s);\
						f;\
						va_end(args);


void Logger::critical(const char *s, ...) {
	pass_args(log(Category_Application, Priority_Critical, s, args));
}

void Logger::error(const char *s, ...) {
	pass_args(log(Category_Application, Priority_Error, s, args));
}

void Logger::warn(const char *s, ...) {
	pass_args(log(Category_Application, Priority_Warn, s, args));
}

void Logger::info(const char *s, ...) {
	pass_args(log(Category_Application, Priority_Info, s, args));
}

#ifdef _DEBUG
void Logger::debug(const char *s, ...) {
	pass_args(log(Category_Application, Priority_Debug, s, args));
}
#endif

void Logger::verbose(const char *s, ...) {
	pass_args(log(Category_Application, Priority_Verbose, s, args));
}

void Logger::critical(Category c, const char *s, ...) {
	pass_args(log(c, Priority_Critical, s, args));
}

void Logger::error(Category c, const char *s, ...) {
	pass_args(log(c, Priority_Error, s, args));
}

void Logger::warn(Category c, const char *s, ...) {
	pass_args(log(c, Priority_Warn, s, args));
}

void Logger::info(Category c, const char *s, ...) {
	pass_args(log(c, Priority_Info, s, args));
}

#ifdef _DEBUG
void Logger::debug(Category c, const char *s, ...) {
	pass_args(log(c, Priority_Debug, s, args));
}
#endif

void Logger::verbose(Category c, const char *s, ...) {
	pass_args(log(c, Priority_Verbose, s, args));
}

void Logger::test(const char *s, ...) {
	pass_args(log(Category_Test, Priority_Info, s, args));
}

void Logger::log(Category c, Priority p, const char *s, ...) {
	pass_args(log(c, p, s, args));
}
