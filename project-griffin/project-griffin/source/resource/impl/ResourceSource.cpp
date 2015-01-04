/**
 * @file	ResourceSource.cpp
 * @author	Jeff Kiah
 */

#include "../ResourceSource.h"
#include <sys/stat.h>

namespace griffin {
	namespace resource {

		bool FileSystemSource::directoryExists(const wstring &relativePath)
		{
			wchar_t fullPath[1024] = L"\0";
			_wfullpath(fullPath, relativePath.c_str(), 1024);

			if (_waccess(fullPath, 0) == 0) {
				struct _stat status;
				_wstat(fullPath, &status);

				return (status.st_mode & S_IFDIR) != 0;
			}
			return false;
		}


		bool FileSystemSource::open()
		{
			return directoryExists(m_dataPath);
		}


		bool FileSystemSource::hasResource(const wstring &name) const
		{
			return true;
		}


		size_t FileSystemSource::getSizeBytes(const wstring &name) const
		{
			size_t size = 0;
			const wstring resPath(m_dataPath + name);

			FILE *inFile = _wfsopen(resPath.c_str(), L"rb", _SH_DENYWR);
			if (inFile) {
				// get size
				fseek(inFile, 0, SEEK_END);
				size_t size = ftell(inFile);
				fclose(inFile);
			}
			else {
				//debugWPrintf(L"FileSystemSource: file %s not found\n", resName.c_str());
			}

			return size;
		}


		DataPtr FileSystemSource::load(const wstring &name, size_t *outSizeBytes)
		{
			DataPtr bPtr;
			size_t size = 0;
			const wstring resPath(m_dataPath + name);

			FILE *inFile = _wfsopen(resPath.c_str(), L"rb", _SH_DENYWR);
			if (inFile) {
				// get size
				fseek(inFile, 0, SEEK_END);
				size = ftell(inFile);

				// create buffer of correct size
				bPtr = std::make_unique<char[]>(size);

				// read data from file
				rewind(inFile);
				auto buffer = reinterpret_cast<void*>(bPtr.get());
				size_t sizeRead = fread(buffer, 1, size, inFile);
				
				fclose(inFile);

				// check for file read error
				if (sizeRead != size) {
					size = 0;
					bPtr.release();
				}
			}
			else {
				std::string s(name.begin(), name.end());
				throw std::runtime_error(std::string("FileSystemSource: file ") + s + " not found");
			}

			if (outSizeBytes != nullptr) {
				*outSizeBytes = size;
			}

			return bPtr;
		}

	}
}