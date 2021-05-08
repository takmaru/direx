#include "Path.h"

std::wstring MyLib::Path::append(const std::wstring& src, const std::wstring& appendpath) {
	return MyLib::Path::append(src, appendpath.c_str());
}

std::wstring MyLib::Path::append(const std::wstring& src, const wchar_t* appendpath) {
	if(	(appendpath == NULL) ||
		(wcslen(appendpath) == 0)	) {
		return src;
	}
	std::wstring src_copy(src);
	if((*(src_copy.rbegin())) != L'\\')	src_copy += L'\\';
	return src_copy.append(appendpath);
}

std::wstring MyLib::Path::removeFilespec(const std::wstring& src) {
	std::wstring::size_type index = src.find_last_of(L"\\");
	return ((index != std::wstring::npos) ? src.substr(0, index) : src);
}
std::wstring MyLib::Path::removeExtention(const std::wstring& src) {
	std::wstring::size_type index = src.find_last_of(L".");
	if(index == std::wstring::npos) {
		return src;
	}
	std::wstring::size_type path_div_index = src.find_last_of(L"\\");
	if(path_div_index != std::wstring::npos) {
		if(path_div_index > index) {
			return src;
		}
	}
	return src.substr(0, index);
}
std::wstring MyLib::Path::renameExtention(const std::wstring& src, const std::wstring& extention) {
	return MyLib::Path::renameExtention(src, extention.c_str());
}
std::wstring MyLib::Path::renameExtention(const std::wstring& src, const wchar_t* extention) {
	std::wstring removed = removeExtention(src);
	if(	(extention == NULL) ||
		(wcslen(extention) == 0)	) {
		return removed;
	}
	if(extention[0] != L'.') {
		removed += L'.';
	}
	return removed.append(extention);
}

std::wstring MyLib::Path::filename(const wchar_t* path) {
	return MyLib::Path::filename(std::wstring(path));
}
std::wstring MyLib::Path::filename(const std::wstring& path) {
	std::wstring::size_type pos = path.find_last_of(L'\\');
	if(pos == std::wstring::npos) {
		return std::wstring();
	}
	return path.substr(pos + 1);
}

