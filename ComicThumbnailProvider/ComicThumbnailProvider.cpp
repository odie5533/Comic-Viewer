#define USE_STREAM 0
#define USE_FILE 1
#define USE_ITEM 0
#define _7Z_LOCATION "\\Comic Viewer\\7z.exe"

#include <shlwapi.h>
#include <Wincrypt.h>
#include <thumbcache.h>
#include <wincodec.h>
#include <msxml6.h>
#include <stdio.h>
#include <new>
#include <sstream>
#include <iostream>
#include <vector>
#include <algorithm>
#include <ShlObj.h>

using std::wcout;
using std::cout;
using std::endl;

#pragma comment(lib, "shlwapi.lib")
#pragma comment(lib, "windowscodecs.lib")
#pragma comment(lib, "Crypt32.lib")
#pragma comment(lib, "msxml6.lib")

class ComicThumbnailProvider :
#if USE_STREAM
							 public IInitializeWithStream,
#endif
#if USE_FILE
  							 public IInitializeWithFile,
#endif
#if USE_ITEM
							 public IInitializeWithItem,
#endif
                             public IThumbnailProvider
{
public:
    ComicThumbnailProvider() : _cRef(1), _pStream(NULL)
    {
    }

    IFACEMETHODIMP QueryInterface(REFIID riid, void **ppv)
    {
        static const QITAB qit[] =
        {
#if USE_STREAM
			QITABENT(ComicThumbnailProvider, IInitializeWithStream),
#endif
#if USE_FILE
            QITABENT(ComicThumbnailProvider, IInitializeWithFile),
#endif
#if USE_ITEM
			QITABENT(ComicThumbnailProvider, IInitializeWithItem),
#endif
            QITABENT(ComicThumbnailProvider, IThumbnailProvider),
            { 0 },
        };
        return QISearch(this, qit, riid, ppv);
    }

    IFACEMETHODIMP_(ULONG) AddRef()
    {
        return InterlockedIncrement(&_cRef);
    }

    IFACEMETHODIMP_(ULONG) Release()
    {
        ULONG cRef = InterlockedDecrement(&_cRef);
        if (!cRef)
        {
            delete this;
        }
        return cRef;
    }

	// IInitializeWithFile
#if USE_FILE
    IFACEMETHODIMP Initialize(LPCWSTR pszFilePath, DWORD grfMode);
#endif

	// IInitializeWithStream
#if USE_STREAM
	IFACEMETHODIMP Initialize(IStream *pStream, DWORD);
#endif

#if USE_ITEM
	IFACEMETHODIMP Initialize(IShellItem *psi, DWORD);
#endif

    IFACEMETHODIMP GetThumbnail(UINT cx, HBITMAP *phbmp, WTS_ALPHATYPE *pdwAlpha);

private:
	HRESULT _GetStreamFromFile(PCWSTR pszFilePath, IStream **ppImageStream);
	HRESULT _GetStreamFromArchive(const std::wstring &pszFilePath, IStream **ppImageStream);

    long _cRef;
    std::wstring _pszFilePath;
	IStream *_pStream;
};

HRESULT CRecipeThumbProvider_CreateInstance(REFIID riid, void **ppv)
{
    ComicThumbnailProvider *pNew = new (std::nothrow) ComicThumbnailProvider();
    HRESULT hr = pNew ? S_OK : E_OUTOFMEMORY;
    if (SUCCEEDED(hr))
    {
        hr = pNew->QueryInterface(riid, ppv);
        pNew->Release();
    }
    return hr;
}

// IInitializeWithFile
#if USE_FILE
IFACEMETHODIMP ComicThumbnailProvider::Initialize(LPCWSTR pszFilePath, DWORD)
{
	_pszFilePath = std::wstring(pszFilePath);
    return S_OK;
}
#endif

#if USE_ITEM
IFACEMETHODIMP ComicThumbnailProvider::Initialize(IShellItem *psi, DWORD)
{
	HRESULT hr = psi->GetDisplayName(SIGDN_FILESYSPATH, &_pszFilePath);
	return hr;
}
#endif

// IInitializeWithStream
#if USE_STREAM
IFACEMETHODIMP ComicThumbnailProvider::Initialize(IStream *pStream, DWORD)
{
    HRESULT hr = E_UNEXPECTED;  // can only be inited once
    if (_pStream == NULL)
    {
        // take a reference to the stream if we have not been inited yet
        hr = pStream->QueryInterface(&_pStream);
    }
    return hr;
}
#endif

short CreateChildProcess(const WCHAR* cmd, PROCESS_INFORMATION &piProcInfo,
		const HANDLE &pipe_stdout_write, const HANDLE &pipe_stdin_read,
		const HANDLE &pipe_stderr_write)
// Create a child process that uses the previously created pipes for STDIN and STDOUT.
{ 
	WCHAR* szCmdline = LPWSTR(cmd);
	wcout << szCmdline << endl;
   STARTUPINFO siStartInfo;
   BOOL bSuccess = FALSE; 
 
// Set up members of the PROCESS_INFORMATION structure. 
 
   ZeroMemory(&piProcInfo, sizeof(PROCESS_INFORMATION) );
 
// Set up members of the STARTUPINFO structure. 
// This structure specifies the STDIN and STDOUT handles for redirection.
 
   ZeroMemory( &siStartInfo, sizeof(STARTUPINFO) );
   siStartInfo.cb = sizeof(STARTUPINFO); 
   siStartInfo.hStdError = pipe_stderr_write;
   siStartInfo.hStdOutput = pipe_stdout_write;
   siStartInfo.hStdInput = pipe_stdin_read;
   siStartInfo.dwFlags |= STARTF_USESTDHANDLES;
 
// Create the child process. 
    
   bSuccess = CreateProcess(NULL, 
      szCmdline,     // command line 
      NULL,          // process security attributes 
      NULL,          // primary thread security attributes 
      TRUE,          // handles are inherited 
      CREATE_NO_WINDOW,             // creation flags 
      NULL,          // use parent's environment 
      NULL,          // use parent's current directory 
      &siStartInfo,  // STARTUPINFO pointer 
      &piProcInfo);  // receives PROCESS_INFORMATION 
   
   // If an error occurs, exit the application. 
   if ( ! bSuccess ) 
      return 1;
   else 
   {
      // Close handles to the child process and its primary thread.
	  // Some applications might keep these handles to monitor the status
	  // of the child process, for example. 

      CloseHandle(piProcInfo.hProcess);
      CloseHandle(piProcInfo.hThread);
	  return 0;
   }
}

struct FileListing {
	std::string name;
	std::string attrib;
	size_t size;

	bool operator<(FileListing const &rhs) const
	{
		return _stricmp(name.c_str(), rhs.name.c_str()) < 0;
	}
};

short _GetFileListingFromArchive(const std::wstring &pszFilePath, std::vector<FileListing> &files) {
	std::wostringstream cmd;
	PWSTR ppszPath;
	SHGetKnownFolderPath(FOLDERID_ProgramFilesX86, 0, NULL, &ppszPath);
	cmd << ppszPath << _7Z_LOCATION << " l \"" << pszFilePath << "\"";
	std::wstring wsCmd(cmd.str());

	HANDLE pipe_stdout_read, pipe_stdout_write;

	PROCESS_INFORMATION piProcInfo;
	SECURITY_ATTRIBUTES saAttr;
	saAttr.nLength = sizeof(SECURITY_ATTRIBUTES); 
	saAttr.bInheritHandle = TRUE;
	saAttr.lpSecurityDescriptor = NULL;

	// Create a pipe for the child process's STDOUT. 
    if ( ! CreatePipe(&pipe_stdout_read, &pipe_stdout_write, &saAttr, 0) ) 
      return 1;
	// Ensure the read handle to the pipe for STDOUT is not inherited.
   if ( ! SetHandleInformation(pipe_stdout_read, HANDLE_FLAG_INHERIT, 0) )
      return 1;

	wcout << L"Creating process: " << wsCmd << endl;
	
	if (CreateChildProcess(cmd.str().c_str(), piProcInfo, pipe_stdout_write, 0, 0)) {
		cout << "Error creating child process" << endl;
	}
	cout << "Process started" << endl;
	CloseHandle(pipe_stdout_write); // After the child process inherits the write handle, the parent process no longer needs its copy.

	short files_listing = 0; // flag

	std::string name;
	std::string attrib;

	char lBuf[300] = {0};
	std::string::size_type index;
	DWORD dwRead = 0;
	std::string line; //invariant: line contains trailing part of buffer
	std::vector<std::string> lines; // invariant: lines contains read lines

	while (ReadFile(pipe_stdout_read, lBuf, sizeof(lBuf), &dwRead, NULL) && dwRead > 0) {
		line.append(lBuf, dwRead);
		while ((index = line.find('\n')) != std::string::npos) {
			lines.push_back(line.substr(0, index+1));
			line.erase(0, index+1);
		}
	}

	CloseHandle(pipe_stdout_read);

	cout << "Read " << lines.size() << " lines" << endl;
	std::vector<std::string>::iterator it;

	for (it=lines.begin(); it < lines.end(); it++)
	{
		std::string line = *it;
		cout << line;
		if (line.compare(0, 4, "----") == 0) {
			cout << "file listing!" << endl;
			files_listing = !files_listing;
		} else if (files_listing) {
			// This section is only executed between --- and --- while listing
			FileListing file;
			std::istringstream i(line);
			i.ignore(19);
			i >> file.attrib >> file.size;
			i.seekg(53);
			getline(i, name);
			name.erase(name.end()-1); // last character is malformed
			file.name = name;
			files.push_back(file);
		}
	}
	return 0;
}

int _GetFileFromArchive(const std::wstring &pszFilePath, const std::wstring& fileName, const size_t file_size,
						IStream **ppImageStream)
{
	std::wostringstream cmd;
	PWSTR ppszPath;
	SHGetKnownFolderPath(FOLDERID_ProgramFilesX86, 0, NULL, &ppszPath);
	cmd << ppszPath << _7Z_LOCATION << " e -so \"" << pszFilePath << "\" \"" << fileName << "\"";
    wcout << L"Creating process: " << endl;
	wcout << cmd.str() << endl;

	HANDLE pipe_stdout_read, pipe_stdout_write;

	PROCESS_INFORMATION piProcInfo;
	SECURITY_ATTRIBUTES saAttr;
	saAttr.nLength = sizeof(SECURITY_ATTRIBUTES); 
	saAttr.bInheritHandle = TRUE;
	saAttr.lpSecurityDescriptor = NULL;

	// Create a pipe for the child process's STDOUT. 
    if ( ! CreatePipe(&pipe_stdout_read, &pipe_stdout_write, &saAttr, 0) ) 
      return 1;
	// Ensure the read handle to the pipe for STDOUT is not inherited.
   if ( ! SetHandleInformation(pipe_stdout_read, HANDLE_FLAG_INHERIT, 0) )
      return 1;
	
	if (CreateChildProcess(cmd.str().c_str(), piProcInfo, pipe_stdout_write, 0, 0)) {
		cout << "Error creating child process" << endl;
	}
	cout << "Process started" << endl;
	CloseHandle(pipe_stdout_write); // After the child process inherits the write handle, the parent process no longer needs its copy.

	BYTE *pbStream = (BYTE*)LocalAlloc(LPTR, file_size);

	UINT total = 0;
	DWORD dwRead;
	while (ReadFile(pipe_stdout_read, pbStream+total, 128, &dwRead, NULL) && dwRead > 0)
	{
		total += dwRead;
	}
	CloseHandle(pipe_stdout_read);
	*ppImageStream = SHCreateMemStream(pbStream, total);
	LocalFree(pbStream);
	return 0;
}

/*
Loads an IStream for the first image in an archive located at the given path
Works by calling _GetFileListingFromArchive and then _GetFileFromArchive
*/
HRESULT ComicThumbnailProvider::_GetStreamFromArchive(const std::wstring &pszFilePath, IStream **ppImageStream)
{
	std::wstring first_file;
	size_t file_size = 0;
	std::vector<FileListing> files;

	if (_GetFileListingFromArchive(pszFilePath, files))
	{
		if (first_file.empty())
			wcout << "_GetStreamFromArchive: Did not find a valid image." << endl;
		else 
			wcout << "_GetStreamFromArchive Fail." << endl;
		wcout << "File size: " << file_size << endl;
		return E_FAIL;
	}

	std::sort(files.begin(), files.end());
	std::vector<FileListing>::iterator it;
	for (it = files.begin(); it < files.end(); it++)
	{
		FileListing file = *it;
		if (file.attrib.compare(0, 1, "D") == 0)
			continue;
		std::string::size_type last = file.name.rfind(".");
		if (last == std::string::npos)
			continue;
		std::string ext = file.name.substr(last+1, 4);
		if (_strnicmp(ext.c_str(), "jpg", 3) == 0 || _strnicmp(ext.c_str(), "jpeg", 4) == 0 ||
			_strnicmp(ext.c_str(), "png", 3) == 0 || _strnicmp(ext.c_str(), "bmp", 3) == 0) {
				file_size = file.size;
				first_file = std::wstring(file.name.length(), L' ');
				std::copy(file.name.begin(), file.name.end(), first_file.begin());
				break;
		}
	}
	if (first_file.empty())
		return E_FAIL;
	_GetFileFromArchive(pszFilePath, first_file, file_size, ppImageStream);
	return 0;
}

// Reads an image file into a stream
HRESULT ComicThumbnailProvider::_GetStreamFromFile(LPCWSTR pszFilePath, IStream **ppImageStream)
{
	WIN32_FIND_DATA FindFileData;
	HANDLE hFind = FindFirstFile(pszFilePath, &FindFileData);
	if (hFind == INVALID_HANDLE_VALUE)
		return E_FAIL;
	DWORD FileSize = FindFileData.nFileSizeLow;

	FILE* fhandle;
	_wfopen_s(&fhandle, pszFilePath, L"rb");
	BYTE *pbDecodedImage = (BYTE*)LocalAlloc(LPTR, FileSize+1);
	fread(pbDecodedImage, FileSize, 1, fhandle);
	fclose(fhandle);

	*ppImageStream = SHCreateMemStream(pbDecodedImage, FileSize);

	LocalFree(pbDecodedImage);
    return S_OK;
}

HRESULT ConvertBitmapSourceTo32BPPHBITMAP(IWICBitmapSource *pBitmapSource,
                                           IWICImagingFactory *pImagingFactory,
                                           HBITMAP *phbmp)
{
    *phbmp = NULL;

    IWICBitmapSource *pBitmapSourceConverted = NULL;
    WICPixelFormatGUID guidPixelFormatSource;
    HRESULT hr = pBitmapSource->GetPixelFormat(&guidPixelFormatSource);
    if (SUCCEEDED(hr) && (guidPixelFormatSource != GUID_WICPixelFormat32bppBGRA))
    {
        IWICFormatConverter *pFormatConverter;
        hr = pImagingFactory->CreateFormatConverter(&pFormatConverter);
        if (SUCCEEDED(hr))
        {
            // Create the appropriate pixel format converter
            hr = pFormatConverter->Initialize(pBitmapSource, GUID_WICPixelFormat32bppBGRA, WICBitmapDitherTypeNone, NULL, 0, WICBitmapPaletteTypeCustom);
            if (SUCCEEDED(hr))
            {
                hr = pFormatConverter->QueryInterface(&pBitmapSourceConverted);
            }
            pFormatConverter->Release();
        }
    }
    else
    {
        hr = pBitmapSource->QueryInterface(&pBitmapSourceConverted);  // No conversion necessary
    }

    if (SUCCEEDED(hr))
    {
        UINT nWidth, nHeight;
        hr = pBitmapSourceConverted->GetSize(&nWidth, &nHeight);
        if (SUCCEEDED(hr))
        {
            BITMAPINFO bmi = {};
            bmi.bmiHeader.biSize = sizeof(bmi.bmiHeader);
            bmi.bmiHeader.biWidth = nWidth;
            bmi.bmiHeader.biHeight = -static_cast<LONG>(nHeight);
            bmi.bmiHeader.biPlanes = 1;
            bmi.bmiHeader.biBitCount = 32;
            bmi.bmiHeader.biCompression = BI_RGB;

            BYTE *pBits;
            HBITMAP hbmp = CreateDIBSection(NULL, &bmi, DIB_RGB_COLORS, reinterpret_cast<void **>(&pBits), NULL, 0);
            hr = hbmp ? S_OK : E_OUTOFMEMORY;
            if (SUCCEEDED(hr))
            {
                WICRect rect = {0, 0, nWidth, nHeight};

                // Convert the pixels and store them in the HBITMAP.  Note: the name of the function is a little
                // misleading - we're not doing any extraneous copying here.  CopyPixels is actually converting the
                // image into the given buffer.
                hr = pBitmapSourceConverted->CopyPixels(&rect, nWidth * 4, nWidth * nHeight * 4, pBits);
                if (SUCCEEDED(hr))
                {
                    *phbmp = hbmp;
                }
                else
                {
                    DeleteObject(hbmp);
                }
            }
        }
        pBitmapSourceConverted->Release();
    }
    return hr;
}

HRESULT WICScaleBitmapSource(IWICBitmapSource **pBitmapSource, IWICImagingFactory *pImagingFactory, UINT cx)
{
	IWICBitmapScaler *pIScaler = NULL;
	HRESULT hr = pImagingFactory->CreateBitmapScaler(&pIScaler);
	if (SUCCEEDED(hr))
	{
		UINT uiWidth, uiHeight;
		hr = (*pBitmapSource)->GetSize(&uiWidth, &uiHeight);
		if (SUCCEEDED(hr))
		{
			if (uiWidth <= cx && uiHeight <= cx)
				return hr;
			if (uiWidth < uiHeight) {
				uiWidth = (UINT)((FLOAT)uiWidth * cx / uiHeight);
				uiHeight = cx;
			} else if (uiHeight < uiWidth) {
				uiHeight = (UINT)((FLOAT)uiHeight * cx / uiWidth);
				uiWidth = cx;
			} else {
				uiWidth = cx;
				uiHeight = cx;
			}
			hr = pIScaler->Initialize(*pBitmapSource, uiWidth, uiHeight,
					WICBitmapInterpolationModeFant);
			if (SUCCEEDED(hr)) {
				IWICBitmapSource *pBitmapSourceConverted = NULL;
				hr = pIScaler->QueryInterface(&pBitmapSourceConverted);
				if (SUCCEEDED(hr)) {
					(*pBitmapSource)->Release();
					*pBitmapSource = pBitmapSourceConverted;
				}
			}
		}
	}
	pIScaler->Release();
	return hr;
}

HRESULT WICCreate32BitsPerPixelHBITMAP(IStream *pstm, UINT cx, HBITMAP *phbmp, WTS_ALPHATYPE *pdwAlpha)
{
    *phbmp = NULL;

    IWICImagingFactory *pImagingFactory;
    HRESULT hr = CoCreateInstance(CLSID_WICImagingFactory, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&pImagingFactory));
    if (SUCCEEDED(hr))
    {
        IWICBitmapDecoder *pDecoder;
        hr = pImagingFactory->CreateDecoderFromStream(pstm, &GUID_VendorMicrosoft, WICDecodeMetadataCacheOnDemand, &pDecoder);
        if (SUCCEEDED(hr))
        {
            IWICBitmapFrameDecode *pBitmapFrameDecode;
            hr = pDecoder->GetFrame(0, &pBitmapFrameDecode);
            if (SUCCEEDED(hr))
            {
				hr = WICScaleBitmapSource((IWICBitmapSource**)&pBitmapFrameDecode, pImagingFactory, cx);
				if (SUCCEEDED(hr))
				{
					hr = ConvertBitmapSourceTo32BPPHBITMAP(pBitmapFrameDecode, pImagingFactory, phbmp);
					if (SUCCEEDED(hr))
					{
						*pdwAlpha = WTSAT_ARGB;
					}                
				}
				pBitmapFrameDecode->Release();
            }
            pDecoder->Release();
        }
        pImagingFactory->Release();
    }
    return hr;
}

IFACEMETHODIMP ComicThumbnailProvider::GetThumbnail(UINT cx, HBITMAP *phbmp, WTS_ALPHATYPE *pdwAlpha)
{
	HRESULT hr = S_OK;
#if USE_FILE | USE_ITEM
	if (_pszFilePath.empty()) {
		hr = E_FAIL;
	} else {
		//_GetStreamFromFile(_pszFilePath, &_pStream); // can be used if the file is actually just an image
		_GetStreamFromArchive(_pszFilePath, &_pStream);
	}
#endif
    if (SUCCEEDED(hr))
    {
        hr = WICCreate32BitsPerPixelHBITMAP(_pStream, cx, phbmp, pdwAlpha);
		if (SUCCEEDED(hr) && _pStream)
		{
			_pStream->Release();
		}
    }
    return hr;
}
