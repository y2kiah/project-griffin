/**
* @file	ResourceSource.h
* @author	Jeff Kiah
*/
#pragma once
#ifndef GRIFFIN_RESOURCE_SOURCE_
#define GRIFFIN_RESOURCE_SOURCE_

#include <cstdint>
#include <memory>
#include <string>

namespace griffin {
	namespace resource {

		using std::wstring;
		using std::unique_ptr;
		using std::shared_ptr;

		typedef unique_ptr<unsigned char[]> DataPtr;

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
			static bool directoryExists(wstring relativePath);

			wstring m_dataPath;
		};

		typedef shared_ptr<IResourceSource>		IResourceSourcePtr;
	}
}

#endif