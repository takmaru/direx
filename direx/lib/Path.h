#pragma once

#include <Windows.h>
#include <string>

namespace MyLib {
namespace Path {

std::wstring append(const std::wstring& src, const std::wstring& appendpath);
std::wstring append(const std::wstring& src, const wchar_t* appendpath);

std::wstring removeFilespec(const std::wstring& src);
std::wstring removeExtention(const std::wstring& src);
std::wstring renameExtention(const std::wstring& src, const std::wstring& extention);
std::wstring renameExtention(const std::wstring& src, const wchar_t* extention);

std::wstring filename(const std::wstring& path);
std::wstring filename(const wchar_t* path);

}
}