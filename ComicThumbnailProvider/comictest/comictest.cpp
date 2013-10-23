/*
This program is used to test the functionality of the methods placed in the DLL
Debugging DLLs is difficult so the tests are done here
*/
#define UNICODE 1
#include <stdio.h>
#include <stdlib.h>
#include <istream>
#include <Shlwapi.h>
#include <ShlObj.h>
#include <wincodec.h>   // Windows Imaging Codecs
#include <string>
#include <iostream>
#include <sstream>
#include <vector>
#include <algorithm>

using std::cout;
using std::wcout;
using std::endl;

#pragma comment(lib, "shlwapi.lib")
#pragma comment(lib, "windowscodecs.lib")

HRESULT _GetStreamFromFile(LPCTSTR pszFilePath, IStream **ppImageStream)
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

short CreateChildProcess(const WCHAR* cmd, PROCESS_INFORMATION &piProcInfo,
		const HANDLE &pipe_stdout_write, const HANDLE &pipe_stdin_read,
		const HANDLE &pipe_stderr_write)
// Create a child process that uses the previously created pipes for STDIN and STDOUT.
{ 
	WCHAR* szCmdline = LPWSTR(cmd);
   STARTUPINFO siStartInfo;
   BOOL bSuccess = FALSE; 
 
// Set up members of the PROCESS_INFORMATION structure. 
 
   ZeroMemory( &piProcInfo, sizeof(PROCESS_INFORMATION) );
 
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

short _GetFileListingFromArchive(LPCWSTR pszFilePath, std::vector<FileListing> &files) {
	std::wostringstream cmd;
	cmd << "7z l \"" << pszFilePath << "\"";

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
		CloseHandle(pipe_stdout_read);
		CloseHandle(pipe_stdout_write);
		return 1;
	}
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

	std::vector<std::string>::iterator it;

	for (it=lines.begin(); it < lines.end(); it++)
	{
		std::string line = *it;
		if (line.compare(0, 4, "----") == 0) {
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

int _GetFileFromArchive(LPCWSTR pszFilePath, const std::wstring &fileName, const size_t file_size,
						IStream **ppImageStream)
{
	std::wostringstream cmd;
	cmd << "7z e -so \"" << pszFilePath << "\" \"" << fileName << "\"";
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

	size_t total = 0;
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

HRESULT _GetStreamFromArchive(LPCWSTR pszFilePath, IStream **ppImageStream)
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

	std::vector<FileListing>::iterator it1;
	for (it1 = files.begin(); it1 < files.end(); it1++)
	{
		std::cout << (*it1).name << endl;
	}

	std::sort(files.begin(), files.end());
	std::vector<FileListing>::iterator it;
	for (it = files.begin(); it < files.end(); it++)
	{
		std::cout << (*it).name << endl;
		FileListing file = *it;
		if (file.attrib[0] == 'D')
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

HRESULT WICScaleBitmapSource(IWICBitmapSource **pBitmapSource, IWICImagingFactory *pImagingFactory, UINT cx)
{
	IWICBitmapScaler *pIScaler = NULL;
	HRESULT hr = pImagingFactory->CreateBitmapScaler(&pIScaler);
	if (SUCCEEDED(hr))
	{
		unsigned int uiWidth, uiHeight;
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

int main( void )
{
	IStream *ppImageStream;
	//HRESULT hr = _GetStreamFromFile(L"iron.recipe", &ppImageStream);
	HRESULT hr = _GetStreamFromArchive(L"iron 2.cbz", &ppImageStream);
	
	IWICBitmapDecoder *pDecoder = NULL;
	IWICImagingFactory *pFactory = NULL;

	IWICBitmapFrameDecode *pFrame = NULL;

	if (SUCCEEDED(hr)) {
		printf("read file into IStream.\n");

		hr = CoInitialize(NULL);
		hr = CoCreateInstance(
			CLSID_WICImagingFactory,
			NULL,
			CLSCTX_INPROC_SERVER,
			IID_IWICImagingFactory,
			(LPVOID*)&pFactory
			);
	}
	if (SUCCEEDED(hr)) {
		printf("Created pFactory\n");
		hr = pFactory->CreateDecoderFromStream(
	        ppImageStream,
			&CLSID_WICJpegDecoder,
			WICDecodeMetadataCacheOnDemand,
			&pDecoder);
	}
	if (SUCCEEDED(hr)) {
		printf("Created pDecoder\n");
	}
	if (SUCCEEDED(hr)) {
		hr = pDecoder->GetFrame(0, &pFrame);
	}
	if (SUCCEEDED(hr)) {
		IWICBitmapSource* pBmp = pFrame;
		hr = WICScaleBitmapSource(&pBmp, pFactory, 256);
		UINT uiWidth, uiHeight;
		pBmp->GetSize(&uiWidth, &uiHeight);
		printf("New size: %d x %d\n", uiWidth, uiHeight);
	}
	printf("HRESULT: %X\n", hr);
	if (pFactory) pFactory->Release();
	if (pDecoder) pDecoder->Release();
	return hr;
}

