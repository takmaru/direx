#include <Windows.h>

#include <vector>
#include <iostream>
#include <string>
#include <sstream>
#include <iomanip>

#include "lib/ParamParser.h"
#include "lib/Path.h"

#define MAKE_I64(hi, lo)    ( (LONGLONG(DWORD(hi) & 0xffffffff) << 32 ) | LONGLONG(DWORD(lo) & 0xffffffff) )

template<typename T>
std::wstring toSeparatedNumStr(T num) {
    std::vector<int> sepnum;
    bool isNegative = false;
    T number = num;
    if (number < 0) {
        isNegative = true;
        number *= -1;
    }

    while (number / 1000) {
        sepnum.push_back(number % 1000);
        number /= 1000;
    }

    std::wstringstream ss;
    ss << number * (isNegative ? -1 : 1);
    for (std::vector<int>::reverse_iterator i = sepnum.rbegin(); i < sepnum.rend(); i++) {
        ss << L"," << std::setfill(L'0') << std::setw(3) << *i;
    }

    return std::wstring(ss.str());
}

void MinOutputDirList(const std::wstring& path, int path_idx, bool isRecursive, bool isAllOutput);
void OutputDirList(const std::wstring& path, bool isRecursive, bool isAllOutput);
void Output(const std::wstring& str);

std::string toMultibyte(const std::wstring& wc, UINT codepage) {
    static std::vector<unsigned char> mb(1024);

    int ret = 0;
    while((ret = ::WideCharToMultiByte(codepage, 0, wc.c_str(), -1, (LPSTR)&mb[0], mb.size(), NULL, NULL)) == 0) {
        if (mb.size() >= 32 * 1024) {
            mb[0] = '\0';
            break;
        }
        mb.resize(max(mb.size() * 2, 32 * 1024));
    }

    return std::string((char*)&mb[0]);
}

int64_t g_filecount = 0;
int64_t g_sum_filesize = 0;
int64_t g_dircount = 0;
//UINT g_consoleOutputCP = 0;
HANDLE g_writefile = INVALID_HANDLE_VALUE;
UINT g_writeCP = CP_UTF8;

int wmain(int argc, wchar_t* argv[]) {
    std::wcout.imbue(std::locale("", std::locale::ctype));

    MyLib::App::CParamPerser params(argc, argv);

    if ( params.isOptionExist(L"?") ||
         params.isOptionExist(L"HELP") ) {
        std::wcout
            << L"direx [path] [/A] [/M] [/S] [/W outputfile] [/UTF8|/SJIS|/UTF16] [/BOM] [/?|/HELP]" << std::endl
            << L"  dir と同様の出力を行いますが、dir より４割ほど早いです。" << std::endl
            << L"  /M オプションで、サイズを抑えた出力も可能です。" << std::endl
            << std::endl
            << L" path       ファイルを列挙するパスです。" << std::endl
            << L"            指定されていなければカレントディレクトリで列挙します。" << std::endl
            << L" /A         隠しファイル、システムファイルも表示します。" << std::endl
            << L" /M         サイズを抑えた形式で出力します。平均で４割ほどサイズが抑えられます。" << std::endl
            << L" /S         サブディレクトリのすべてのファイルを表示します。" << std::endl
            << L" /W outputfile" << std::endl
            << L"            outputfile に出力します。" << std::endl
            << L"            ファイルが既に存在している場合は上書きします。" << std::endl
            << L" /UTF8, /SJIS, /UTF16" << std::endl
            << L"            /W オプションで出力する際の文字コードを指定します。" << std::endl
            << L"            デフォルトは UTF-8 です。" << std::endl
            << L" /BOM       /W オプションを用い、UTF-8、UTF-16で出力する際に、BOMを付けます。" << std::endl
            << L" /?, /HELP  このヘルプを表示します。" << std::endl
            << std::endl
            << L"  dir だと時間がかかる＆ファイルサイズが大きくなってしまうディレクトリを列挙する為に作成しました。" << std::endl
            << L"  JUNCTION など、dir とは異なる表示になるケースがあります。" << std::endl
            ;

        return 0;
    }

    std::wstring paramDir;
    {
        auto paramlist = params[L""];
        if (paramlist.size() >= 2) {
            paramDir = paramlist[1];
        } else {
            WCHAR currentDir[MAX_PATH + 1] = { 0 };
            ::GetCurrentDirectoryW(_countof(currentDir), currentDir);
            paramDir = currentDir;
        }
    }

    bool isRecursive = params.isOptionExist(L"S");

    bool isAllOutput = params.isOptionExist(L"A");

    {
        auto paramlist = params[L"W"];
        if (paramlist.size() >= 1) {
            g_writefile = ::CreateFile(paramlist[0].c_str(), GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
            if (g_writefile == INVALID_HANDLE_VALUE) {
                DWORD err = ::GetLastError();
                std::wcout << L"エラー：ファイルを作成できませんでした(" << err << L")" << std::endl
                    << L"file : " << paramlist[0] << std::endl;
                return 0;
            }
        }
    }
    {
        if (g_writefile == INVALID_HANDLE_VALUE) {
            // 出力ファイルがない場合
            // 現在のコンソールの設定に従う
            g_writeCP = ::GetConsoleOutputCP();
        } else if (params.isOptionExist(L"UTF8")) {
            g_writeCP = CP_UTF8;    // UTF-8
        } else if (params.isOptionExist(L"SJIS")) {
            g_writeCP = 932;        // Shift-JIS
        } else if (params.isOptionExist(L"UTF16")) {
            g_writeCP = 1200;       // UTF-16
        } else {
            // 出力ファイルはあるが、文字コードが指定されていない場合（出力文字コードのデフォルト）
            // UTF-8
            g_writeCP = CP_UTF8;
        }
    }
    if ( (g_writefile != INVALID_HANDLE_VALUE) &&
         params.isOptionExist(L"BOM") ) {
        switch (g_writeCP) {
        case CP_UTF8: { // UTF-8
            unsigned char bom[3] = { 0xEF, 0xBB, 0xBF };
            DWORD written = 0;
            ::WriteFile(g_writefile, bom, _countof(bom), &written, NULL);
            }
            break;
        case 1200: {    // UTF-16
            unsigned char bom[2] = { 0xFF, 0xFE };
            DWORD written = 0;
            ::WriteFile(g_writefile, bom, _countof(bom), &written, NULL);
            }
            break;
        }
    }

    WCHAR fullPath[MAX_PATH + 1] = { 0 };
    ::GetFullPathName(paramDir.c_str(), _countof(fullPath), fullPath, NULL);

    if (params.isOptionExist(L"M")) {
        // 最小限の出力
        MinOutputDirList(fullPath, 0, isRecursive, isAllOutput);
    } else {
        // dir と同じ出力
        {
            WCHAR volumePath[MAX_PATH + 1] = { 0 };
            ::GetVolumePathNameW(fullPath, volumePath, _countof(volumePath));
            WCHAR volumeName[MAX_PATH + 1] = { 0 };
            DWORD serialNumber = 0;
            ::GetVolumeInformationW(volumePath, volumeName, _countof(volumeName), &serialNumber, NULL, NULL, NULL, 0);

            std::wostringstream oss;
            oss << L" ドライブ C のボリューム ラベルは " << volumeName << L" です" << std::endl
                << L" ボリューム シリアル番号は "
                << std::hex << std::uppercase << std::setw(4) << std::setfill(L'0') << HIWORD(serialNumber)
                << L"-"
                << std::hex << std::uppercase << std::setw(4) << std::setfill(L'0') << LOWORD(serialNumber)
                << L" です" << std::endl
                << std::dec << std::nouppercase
                << std::endl
                ;
            Output(oss.str());
        }

        OutputDirList(fullPath, isRecursive, isAllOutput);

        ULARGE_INTEGER totalNumberOfFreeBytes = { 0 };
        ::GetDiskFreeSpaceExW(fullPath, NULL, NULL, &totalNumberOfFreeBytes);

        if (isRecursive) {
            std::wostringstream oss;
            oss << L"     ファイルの総数:" << std::endl
                << L"           "
                << std::setw(4) << std::setfill(L' ') << g_filecount << L" 個のファイル"
                << L"      " << std::setw(12) << std::setfill(L' ') << toSeparatedNumStr(g_sum_filesize) << L" バイト" << std::endl
                ;
            Output(oss.str());
        }

        {
            std::wostringstream oss;
            oss << L"           "
                << std::setw(4) << std::setfill(L' ') << g_dircount << L" 個のディレクトリ"
                << L" " << std::setw(16) << std::setfill(L' ') << toSeparatedNumStr(totalNumberOfFreeBytes.QuadPart) << L" バイトの空き領域" << std::endl
                ;
            Output(oss.str());
        }
    }
 
    if (g_writefile != INVALID_HANDLE_VALUE) {
        ::CloseHandle(g_writefile);
    }

    return 0;
}

void MinOutputDirList(const std::wstring& path, int path_idx, bool isRecursive, bool isAllOutput) {

    {
        std::wostringstream oss;
        oss << L"P|";
        if (path_idx > 0)   oss << L"<" << path_idx - 1 << L">\\" << MyLib::Path::filename(path);
        else                oss << path;
        oss << L"|" << path_idx << std::endl;
        Output(oss.str());
    }

    std::list<std::wstring> childDirs;
    WIN32_FIND_DATAW findData = { 0 };
    HANDLE find = ::FindFirstFileExW(MyLib::Path::append(path, L"*").c_str(), FindExInfoBasic, &findData, FindExSearchNameMatch, NULL, 0);
    if (find != INVALID_HANDLE_VALUE) {
        do {

            // システムファイル、隠しファイルは除外（全出力フラグがOFFの場合）
            if (!isAllOutput &&
                (((findData.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN) != 0) ||
                    ((findData.dwFileAttributes & FILE_ATTRIBUTE_SYSTEM) != 0))) continue;

            std::wostringstream oss_dt;
            {   // date & time
                FILETIME ft = { 0 };
                ::FileTimeToLocalFileTime(&findData.ftLastWriteTime, &ft);
                SYSTEMTIME st = { 0 };
                ::FileTimeToSystemTime(&ft, &st);
                oss_dt
                    << std::setw(4) << std::setfill(L'0') << st.wYear << L"/"
                    << std::setw(2) << std::setfill(L'0') << st.wMonth << L"/"
                    << std::setw(2) << std::setfill(L'0') << st.wDay << L" "
                    << std::setw(2) << std::setfill(L'0') << st.wHour << L":"
                    << std::setw(2) << std::setfill(L'0') << st.wMinute << L":"
                    << std::setw(2) << std::setfill(L'0') << st.wSecond << L"."
                    << std::setw(3) << std::setfill(L'0') << st.wMilliseconds
                    ;
            }

            std::wostringstream oss;
            if ((findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0) {
                // dir
                if ((wcscmp(findData.cFileName, L".") != 0) &&
                    (wcscmp(findData.cFileName, L"..") != 0))
                    childDirs.push_back(findData.cFileName);
                oss << L"D|" << oss_dt.str() << L"|" << findData.cFileName << std::endl;
            } else {
                // file
                oss << L"F|" << oss_dt.str() << L"|" << MAKE_I64(findData.nFileSizeHigh, findData.nFileSizeLow) << L"|" << findData.cFileName << std::endl;
            }
            Output(oss.str());
        } while (::FindNextFileW(find, &findData));
    }

    if (isRecursive) {
        for (std::list<std::wstring>::const_iterator it = childDirs.begin(); it != childDirs.end(); ++it) {
            MinOutputDirList(MyLib::Path::append(path, (*it)), path_idx + 1, isRecursive, isAllOutput);
        }
    }
}

void OutputDirList(const std::wstring& path, bool isRecursive, bool isAllOutput) {

    {
        std::wostringstream oss;
        oss << L" " << path << L" のディレクトリ" << std::endl << std::endl;
        Output(oss.str());
    }

    std::list<std::wstring> childDirs;
    int64_t filecount = 0;
    int64_t sum_filesize = 0;
    int64_t dircount = 0;
    WIN32_FIND_DATAW findData = { 0 };
    HANDLE find = ::FindFirstFileExW(MyLib::Path::append(path, L"*").c_str(), FindExInfoBasic, &findData, FindExSearchNameMatch, NULL, 0);
    if (find != INVALID_HANDLE_VALUE) {
        do {

            // システムファイル、隠しファイルは除外（全出力フラグがOFFの場合）
            if ( !isAllOutput &&
                 ( ((findData.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN) != 0) ||
                   ((findData.dwFileAttributes & FILE_ATTRIBUTE_SYSTEM) != 0) ) ) continue;

            std::wostringstream oss;
            {   // date & time
                FILETIME ft = { 0 };
                ::FileTimeToLocalFileTime(&findData.ftLastWriteTime, &ft);
                SYSTEMTIME st = { 0 };
                ::FileTimeToSystemTime(&ft, &st);

                oss << std::setw(4) << std::setfill(L'0') << st.wYear << L"/"
                    << std::setw(2) << std::setfill(L'0') << st.wMonth << L"/"
                    << std::setw(2) << std::setfill(L'0') << st.wDay << L"  "
                    << std::setw(2) << std::setfill(L'0') << st.wHour << L":"
                    << std::setw(2) << std::setfill(L'0') << st.wMinute
                    ;
            }
            {   // dir or filesize
                if ((findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0) {
                    // dir
                    dircount++;
                    if ((wcscmp(findData.cFileName, L".") != 0) &&
                        (wcscmp(findData.cFileName, L"..") != 0))
                        childDirs.push_back(findData.cFileName);
                    oss << L"    <DIR>          ";
                } else {
                    // file
                    filecount++;
                    sum_filesize += MAKE_I64(findData.nFileSizeHigh, findData.nFileSizeLow);
                    oss << L"  " << std::setw(16) << std::setfill(L' ') << toSeparatedNumStr(MAKE_I64(findData.nFileSizeHigh, findData.nFileSizeLow)) << L" ";
                }
            }
            {   // name
                oss << findData.cFileName << std::endl;
            }
            Output(oss.str());
        } while (::FindNextFileW(find, &findData));
    }

    {
        std::wostringstream oss;
        oss << L"            "
            << std::setw(4) << std::setfill(L' ') << filecount << L" 個のファイル"
            << L"    " << std::setw(16) << std::setfill(L' ') << toSeparatedNumStr(sum_filesize) << L" バイト" << std::endl
            ;
        Output(oss.str());
    }

    g_filecount += filecount;
    g_sum_filesize += sum_filesize;
    g_dircount += dircount;

    if (isRecursive) {
        std::wostringstream oss;
        oss << std::endl;
        Output(oss.str());

        for (std::list<std::wstring>::const_iterator it = childDirs.begin(); it != childDirs.end(); ++it) {
            OutputDirList(MyLib::Path::append(path, (*it)), isRecursive, isAllOutput);
        }
    }
}

void Output(const std::wstring& str) {
    if (g_writefile == INVALID_HANDLE_VALUE) {
        if (g_writeCP == 1200) std::wcout << str;
        else                   std::cout << toMultibyte(str, g_writeCP);
    } else {
        DWORD written = 0;
        LPCVOID buffer = NULL;
        DWORD bytesToWrite = 0;
        std::string mb_str;
        switch (g_writeCP) {
        case 1200:
            buffer = str.c_str();
            bytesToWrite = str.size() * sizeof(wchar_t);
            break;
        case 932:
            mb_str = toMultibyte(str, 932);
            buffer = mb_str.c_str();
            bytesToWrite = mb_str.size();
            break;
        case CP_UTF8:
        default:
            mb_str = toMultibyte(str, CP_UTF8);
            buffer = mb_str.c_str();
            bytesToWrite = mb_str.size();
            break;
        }
        ::WriteFile(g_writefile, buffer, bytesToWrite, &written, NULL);
    }
}
