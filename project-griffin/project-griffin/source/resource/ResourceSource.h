/**
 * @file	ResourceSource.h
 * @author	Jeff Kiah
 */
#pragma once
#ifndef GRIFFIN_RESOURCE_SOURCE_H
#define GRIFFIN_RESOURCE_SOURCE_H

#include <cstdint>
#include <memory>
#include <string>

namespace griffin {
	namespace resource {

		using std::wstring;

		typedef std::unique_ptr<char[]> DataPtr;

		/**
		 *
		 */
		class IResourceSource {
		public:
			virtual bool	open() = 0;
			virtual void	close() = 0;
			virtual bool	hasResource(const wstring &name) const = 0;
			virtual size_t	getSizeBytes(const wstring &name) const = 0;
			virtual DataPtr	load(const wstring &name, size_t *outSizeBytes = nullptr) = 0;

			virtual ~IResourceSource() {}
		};

		/**
		 *
		 */
		class FileSystemSource : IResourceSource {
		public:
			virtual bool	open() override;
			virtual void	close() override {}
			virtual bool	hasResource(const wstring &name) const override;
			virtual size_t	getSizeBytes(const wstring &name) const override;
			virtual DataPtr	load(const wstring &name, size_t *outSizeBytes = nullptr) override;

			explicit FileSystemSource(wstring dataPath = L"data/") :
				m_dataPath(std::move(dataPath))
			{}

		private:
			static bool directoryExists(const wstring &relativePath);

			wstring m_dataPath;
		};
	}
}

#endif