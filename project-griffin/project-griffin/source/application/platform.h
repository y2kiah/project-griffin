#pragma once
#ifndef GRIFFIN_PLATFORM_H_
#define GRIFFIN_PLATFORM_H_

//#include <functional>
#include <string>

namespace griffin {
	namespace platform {

		/*class FileSystemWatcher {
		public:
			void onFileRenamed(std::function<void(const std::wstring&, const std::wstring&)> f);
			void onFileModified(std::function<void(const std::wstring&)> f);
			void onFileAdded(std::function<void(const std::wstring&)> f);
			void onFileRemoved(std::function<void(const std::wstring&)> f);

		};*/

		std::string getCurrentWorkingDirectory();
		std::wstring getCurrentWorkingDirectoryW();

		std::string getPreferencesPath();
		std::wstring getPreferencesPathW();

		void yieldThread();

		void showErrorBox(const char *text, const char *caption);
	}
}

#endif