#pragma once

#include <vector>
#include <string>
#include <list>
#include <map>

namespace MyLib {
namespace App {

class CParamPerser {
private:
	struct comparenocase {
		bool operator()(const std::wstring& s1, const std::wstring& s2) const {
			return (_wcsicmp(s1.c_str(), s2.c_str()) < 0);
		}
	};

public:
	typedef std::vector<std::wstring> paramlist;
private:
	typedef std::map<std::wstring, paramlist, comparenocase> parammap;
	typedef std::pair<std::wstring, paramlist> parampair;

public:
	CParamPerser(int argc, wchar_t* argv[]);
	~CParamPerser();

public:
	int count() const;
	std::wstring operator[](int idx) const;
	int optionCount() const;
	bool isOptionExist(const wchar_t* option) const;
	bool isOptionExist(const std::wstring& option) const {
		return isOptionExist(option.c_str());
	}
	paramlist operator[](const wchar_t* option) const;
	paramlist operator[](const std::wstring& option) const {
		return (*this)[option.c_str()];
	}
	bool isOptionValueExist(const wchar_t* option) const;
	bool isOptionValueExist(const std::wstring& option) const {
		return isOptionValueExist(option.c_str());
	}

private:
	paramlist m_params;
	parammap m_options;
};

}
}