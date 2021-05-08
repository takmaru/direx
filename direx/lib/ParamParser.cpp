#include "ParamParser.h"

MyLib::App::CParamPerser::CParamPerser(int argc, wchar_t* argv[]) :
	m_params(), m_options() {

	std::wstring option;
	for(int i = 0; i < argc; i++) {

		m_params.push_back(std::wstring(argv[i]));

		if(	(argv[i][0] == L'-') ||
			(argv[i][0] == L'/')	) {
			option = &(argv[i][1]);
			parammap::iterator find = m_options.find(option);
			if (find == m_options.end()) {
				paramlist params;
				m_options.insert(parampair(option, params));
			}
		} else {
			parammap::iterator find = m_options.find(option);
			if(find != m_options.end()) {
				find->second.push_back(std::wstring(argv[i]));
			} else {
				// Å‰‚¾‚¯
				paramlist params;
				params.push_back(std::wstring(argv[i]));
				m_options.insert(parampair(option, params));
			}
		}
	}
}

MyLib::App::CParamPerser::~CParamPerser() {
}

int MyLib::App::CParamPerser::count() const {
	return m_params.size();
}
std::wstring MyLib::App::CParamPerser::operator[](int idx) const {
	return m_params[idx];
}

int MyLib::App::CParamPerser::optionCount() const {
	return m_options.size();
}
bool MyLib::App::CParamPerser::isOptionExist(const wchar_t* option) const {
	return (m_options.find(option) != m_options.end());
}
MyLib::App::CParamPerser::paramlist MyLib::App::CParamPerser::operator[](const wchar_t* option) const {
	paramlist params;
	parammap::const_iterator find = m_options.find(option);
	if(find != m_options.end()) {
		params = find->second;
	}
	return params;
}
bool MyLib::App::CParamPerser::isOptionValueExist(const wchar_t* option) const {
	parammap::const_iterator find = m_options.find(option);
	return (	(find != m_options.end()) &&
				(find->second.size() > 0)	);
}
