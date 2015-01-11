/**
 * @file	ResourceSource.cpp
 * @author	Jeff Kiah
 */

#include "../ResourceSource.h"
#include <sys/stat.h>

namespace griffin {
	namespace resource {

		bool FileSystemSource::directoryExists(wstring relativePath)
		{
			// remove the last character of relativePath if it's a slash
			if (relativePath.size() > 0 &&
				relativePath.back() == L'\\' || relativePath.back() == L'/')
			{
				relativePath.pop_back();
			}

			wchar_t fullPath[_MAX_PATH] = L"\0";
			_wfullpath(fullPath, relativePath.c_str(), _MAX_PATH);

			if (_waccess_s(fullPath, 0) == 0) {
				struct _stat64 status;
				int err = _wstat64(fullPath, &status);
				if (err == 0) {
					return (status.st_mode & _S_IFDIR) != 0;
				}
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

			struct _stat64 st;
			if (_wstat64(resPath.c_str(), &st) == 0) {
				size = st.st_size;
			}
			else {
				std::string s(name.begin(), name.end());
				throw std::runtime_error(std::string("FileSystemSource: file ") + s + " not found");
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
				struct _stat64 st;
				_fstat64(_fileno(inFile), &st);
				size = st.st_size;

				// create buffer of correct size
				bPtr = std::make_unique<unsigned char[]>(size);

				// read data from file
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