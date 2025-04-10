#ifndef UNICODE
#define UNICODE
#endif

#ifndef _UNICODE
#define _UNICODE
#endif

//==============================================================================
// Windows API ヘッダー
//==============================================================================
#include <windows.h>
#include <lmcons.h>
#include <commctrl.h>
#include <windowsx.h>
#include <shlobj.h>
#include <objbase.h>
#include <shobjidl.h>

//==============================================================================
// C++ 標準ライブラリ
//==============================================================================
#include <string>
#include <vector>
#include <map>
#include <set>
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <regex>
#include <codecvt>
#include <locale>
#include <system_error>
#include <iomanip>

//==============================================================================
// プロジェクト固有ヘッダー
//==============================================================================
#include "tinyxml2.h"
#include "../resource/resource.h"

//==============================================================================
// プラグマディレクティブ
//==============================================================================
#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "uuid.lib")
#pragma comment(linker, "/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")
#pragma comment(linker, "/SUBSYSTEM:WINDOWS /ENTRY:wWinMainCRTStartup")

//==============================================================================
// 定数定義
//==============================================================================
#define IDC_TREEVIEW 1001
#define IDC_OK_BUTTON 1002
#define IDC_CANCEL_BUTTON 1003

namespace fs = std::filesystem;

//==============================================================================
// 関数プロトタイプ
//==============================================================================

/**
 * @brief ワイド文字列をUTF-8に変換する
 * @param wstr 変換元のワイド文字列
 * @return UTF-8形式の文字列
 */
std::string WStringToUtf8(const std::wstring &wstr);

//==============================================================================
// 構造体定義
//==============================================================================

/**
 * @brief プロジェクト内のファイル情報を保持する構造体
 */
struct FileInfo
{
    std::wstring m_name;   ///< ファイル名（相対パス）
    std::wstring m_filter; ///< フィルターパス
};

/**
 * @brief フィルタ階層構造のノード
 */
struct FilterNode
{
    std::wstring m_name;                ///< ノード名（ファイルまたはディレクトリ名）
    std::vector<FilterNode> m_children; ///< 子ノードのリスト
    bool m_bIsFile;                     ///< ファイルかディレクトリかを示すフラグ
};

//==============================================================================
// グローバル変数
//==============================================================================
HWND g_hWndDirectoryEdit, g_hWndProjectNameEdit, g_hWndGenerateButton, g_hWndStatusText, g_hWndBrowseButton;
WNDPROC g_oldEditProc;
HFONT g_hFont;
HWND g_hTreeView;
HWND g_hConfirmDialog;

//==============================================================================
// ユーティリティ関数
//==============================================================================

/**
 * @brief 現在のユーザー名を取得する
 * @return ユーザー名（ワイド文字列）
 */
std::wstring GetCurrentUserName()
{
    wchar_t username[UNLEN + 1];
    DWORD username_len = UNLEN + 1;
    if (GetUserNameW(username, &username_len))
    {
        return std::wstring(username);
    }
    return L"Unknown";
}

/**
 * @brief GUIDを生成する
 * @return フォーマット済みのGUID文字列（ブレースなし）
 * @throw std::runtime_error GUID生成に失敗した場合
 */
std::wstring GenerateGuid()
{
    GUID guid;
    HRESULT hr = CoCreateGuid(&guid);

    if (FAILED(hr))
    {
        throw std::runtime_error("Failed to create GUID");
    }

    wchar_t guidString[39];
    int result = StringFromGUID2(guid, guidString, sizeof(guidString) / sizeof(wchar_t));

    if (result == 0)
    {
        throw std::runtime_error("Failed to convert GUID to string");
    }

    // StringFromGUID2 returns the GUID in the format {xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx}
    // We need to remove the curly braces for the .vcxproj.filters file format
    std::wstring guidStr(guidString);
    return guidStr.substr(1, guidStr.length() - 2);
}

/**
 * @brief プロジェクト内のソースファイルとヘッダーファイルを取得する
 * @param directory 検索対象のディレクトリパス
 * @return ファイル情報のベクタ
 */
std::vector<FileInfo> GetProjectFiles(const fs::path &directory)
{
    std::vector<FileInfo> files;
    std::wstring baseDir = directory.wstring();

    for (const auto &entry : fs::recursive_directory_iterator(directory))
    {
        if (entry.is_regular_file())
        {
            std::wstring ext = entry.path().extension().wstring();
            std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

            // Only include C++ source files and header files
            if (ext == L".cpp" || ext == L".h" || ext == L".hpp" || ext == L".c")
            {
                std::wstring relativePath = fs::relative(entry.path(), directory).wstring();
                std::wstring filter = fs::relative(entry.path().parent_path(), directory).wstring();

                // If the root directory, set filter to empty
                if (filter == L".")
                {
                    filter = L"";
                }

                // Convert Windows paths to Unix-style paths
                std::replace(relativePath.begin(), relativePath.end(), L'\\', L'/');
                std::replace(filter.begin(), filter.end(), L'\\', L'/');

                files.push_back({relativePath, filter});
            }
        }
    }

    return files;
}

/**
 * @brief XML特殊文字をエスケープする
 * @param input エスケープ対象の文字列
 * @return エスケープ済みの文字列
 */
std::wstring XmlEscape(const std::wstring &input)
{
    std::wstring escaped;
    for (wchar_t ch : input)
    {
        switch (ch)
        {
        case L'&':
            escaped += L"&amp;";
            break;
        case L'<':
            escaped += L"&lt;";
            break;
        case L'>':
            escaped += L"&gt;";
            break;
        case L'"':
            escaped += L"&quot;";
            break;
        case L'\'':
            escaped += L"&apos;";
            break;
        case L'/':
            escaped += L"&#x2F;";
            break;
        default:
            escaped += ch;
            break;
        }
    }
    return escaped;
}

/**
 * @brief Visual Studio用のフィルターファイルを生成する
 * @param projectDirectory プロジェクトディレクトリ
 * @param projectName プロジェクト名
 * @param files ファイル情報のリスト
 * @throw std::runtime_error ファイル生成に失敗した場合
 */
void GenerateFiltersFile(const std::wstring &projectDirectory, const std::wstring &projectName, const std::vector<FileInfo> &files)
{
    fs::path filtersPath = fs::path(projectDirectory) / (std::wstring(projectName) + L".vcxproj.filters");

    try
    {
        // Create and open the filters file
        std::wofstream filtersFile(filtersPath, std::ios::out | std::ios::trunc);
        if (!filtersFile.is_open())
        {
            throw std::runtime_error("Failed to open filters file: " + WStringToUtf8(filtersPath.wstring()));
        }

        filtersFile << L"<?xml version=\"1.0\" encoding=\"utf-8\"?>\n";
        filtersFile << L"<Project ToolsVersion=\"4.0\" xmlns=\"http://schemas.microsoft.com/developer/msbuild/2003\">\n";

        // Create filter hierarchy
        std::map<std::wstring, std::wstring> filterHierarchy;
        for (const auto &file : files)
        {
            fs::path filePath(file.m_name);
            std::wstring currentPath;
            for (const auto &part : filePath.parent_path())
            {
                if (!currentPath.empty())
                {
                    std::wstring parentPath = currentPath;
                    currentPath += L"\\" + part.wstring();
                    if (filterHierarchy.find(currentPath) == filterHierarchy.end())
                    {
                        filterHierarchy[currentPath] = parentPath;
                    }
                }
                else
                {
                    currentPath = part.wstring();
                    if (filterHierarchy.find(currentPath) == filterHierarchy.end())
                    {
                        filterHierarchy[currentPath] = L"";
                    }
                }
            }
        }

        // Write filters
        filtersFile << L"  <ItemGroup>\n";
        for (const auto &[filter, parentFilter] : filterHierarchy)
        {
            filtersFile << L"    <Filter Include=\"" << XmlEscape(filter) << L"\">\n";
            if (!parentFilter.empty())
            {
                filtersFile << L"      <Filter>" << XmlEscape(parentFilter) << L"</Filter>\n";
            }
            filtersFile << L"      <UniqueIdentifier>{" << GenerateGuid() << L"}</UniqueIdentifier>\n";
            filtersFile << L"    </Filter>\n";
        }
        filtersFile << L"  </ItemGroup>\n";

        // Write files
        filtersFile << L"  <ItemGroup>\n";
        for (const auto &file : files)
        {
            fs::path filePath(file.m_name);
            std::wstring ext = filePath.extension().wstring();
            std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

            std::wstring itemType;
            if (ext == L".cpp" || ext == L".c")
            {
                itemType = L"ClCompile";
            }
            else if (ext == L".h" || ext == L".hpp")
            {
                itemType = L"ClInclude";
            }
            else
            {
                itemType = L"None";
            }

            std::wstring escapedFilePath = XmlEscape(file.m_name);
            filtersFile << L"    <" << itemType << L" Include=\"" << escapedFilePath << L"\">\n";
            if (!filePath.parent_path().empty())
            {
                filtersFile << L"      <Filter>" << XmlEscape(filePath.parent_path().wstring()) << L"</Filter>\n";
            }
            filtersFile << L"    </" << itemType << L">\n";
        }
        filtersFile << L"  </ItemGroup>\n";

        filtersFile << L"</Project>\n";

        filtersFile.close();
        if (filtersFile.fail())
        {
            throw std::runtime_error("Failed to close filters file");
        }

        // Verify that the file was created successfully
        if (!fs::exists(filtersPath) || fs::file_size(filtersPath) == 0)
        {
            throw std::runtime_error("Filters file was not created successfully or is empty");
        }
    }
    catch (const std::exception &e)
    {
        std::stringstream ss;
        ss << "Error in GenerateFiltersFile: " << e.what() << "\n";
        ss << "Partial file contents:\n";
        std::wifstream partialFile(filtersPath);
        if (partialFile.is_open())
        {
            std::wstring line;
            for (int i = 0; i < 20 && std::getline(partialFile, line); ++i)
            {
                ss << WStringToUtf8(line) << "\n";
            }
            partialFile.close();
        }
        throw std::runtime_error(ss.str());
    }
}

/**
 * @brief カスタムフォントを作成する
 * @param size フォントサイズ
 * @return フォントハンドル
 */
HFONT CreateCustomFont(int size = 16)
{
    // The font name for CreateFontW needs to be an LPCWSTR (wide string)
    return CreateFontW(
        size, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE,
        L"Segoe UI" // Specify as a wide string literal
    );
}

/**
 * @brief ディレクトリ内のプロジェクトファイルを検索し、プロジェクト名を取得する
 * @param directory 検索対象ディレクトリ
 * @return プロジェクト名（見つからない場合は空文字列）
 */
std::wstring FindProjectFile(const std::wstring &directory)
{
    for (const auto &entry : fs::directory_iterator(directory))
    {
        if (entry.path().extension() == L".vcxproj")
        {
            std::wifstream file(entry.path());
            std::wstring line;
            std::wregex projectNameRegex(L"<ProjectName>(.+?)</ProjectName>");
            while (std::getline(file, line))
            {
                std::wsmatch match;
                if (std::regex_search(line, match, projectNameRegex))
                {
                    return match[1].str();
                }
            }
            // If no ProjectName tag is found, return the file name (without extension)
            return entry.path().stem().wstring();
        }
    }
    return L""; // No project file found
}

/**
 * @brief ワイド文字列をstd::stringに変換する
 * @param wstr 変換元のワイド文字列
 * @return 変換後の文字列
 */
std::string Ws2s(const std::wstring &wstr)
{
    std::string str(wstr.begin(), wstr.end());
    return str;
}

/**
 * @brief ワイド文字列をUTF-8エンコードの文字列に変換する
 * @param wstr 変換元のワイド文字列
 * @return UTF-8エンコードの文字列
 */
std::string WStringToUtf8(const std::wstring &wstr)
{
    if (wstr.empty())
        return std::string();
    int size_needed = WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), NULL, 0, NULL, NULL);
    std::string strTo(size_needed, 0);
    WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), &strTo[0], size_needed, NULL, NULL);
    return strTo;
}

/**
 * @brief プロジェクトファイルを更新する
 * @param projectDirectory プロジェクトディレクトリ
 * @param projectName プロジェクト名
 * @param files ファイル情報のリスト
 * @throw std::runtime_error ファイル操作に失敗した場合
 */
void UpdateProjectFile(const std::wstring &projectDirectory, const std::wstring &projectName, const std::vector<FileInfo> &files)
{
    fs::path projectPath = fs::path(projectDirectory) / (projectName + L".vcxproj");
    std::wstring projectFileName = projectPath.wstring();

    // Check if the file exists
    if (!fs::exists(projectFileName))
    {
        throw std::runtime_error("Project file does not exist: " + WStringToUtf8(projectFileName));
    }

    // Check file permissions
    std::error_code ec;
    fs::file_status status = fs::status(projectFileName, ec);
    if (ec)
    {
        throw std::runtime_error("Failed to get file status: " + ec.message());
    }
    if ((status.permissions() & fs::perms::owner_read) == fs::perms::none)
    {
        throw std::runtime_error("No read permission for project file: " + WStringToUtf8(projectFileName));
    }

    tinyxml2::XMLDocument doc;

    // Convert the wide string to UTF-8
    std::string utf8FileName = WStringToUtf8(projectFileName);

    tinyxml2::XMLError eResult = doc.LoadFile(utf8FileName.c_str());
    if (eResult != tinyxml2::XML_SUCCESS)
    {
        throw std::runtime_error("Failed to load project file: " + std::string(doc.ErrorStr()));
    }

    tinyxml2::XMLElement *projectElement = doc.FirstChildElement("Project");
    if (!projectElement)
    {
        throw std::runtime_error("Invalid project file structure");
    }

    // Collect existing files
    std::set<std::string> existingFiles;
    for (tinyxml2::XMLElement *itemGroup = projectElement->FirstChildElement("ItemGroup");
         itemGroup;
         itemGroup = itemGroup->NextSiblingElement("ItemGroup"))
    {
        for (tinyxml2::XMLElement *item = itemGroup->FirstChildElement();
             item;
             item = item->NextSiblingElement())
        {
            const char *include = item->Attribute("Include");
            if (include)
            {
                existingFiles.insert(include);
            }
        }
    }

    // Add new files
    tinyxml2::XMLElement *newItemGroup = nullptr;
    for (const auto &file : files)
    {
        std::string fileName = Ws2s(file.m_name);
        if (existingFiles.find(fileName) == existingFiles.end())
        {
            if (!newItemGroup)
            {
                newItemGroup = doc.NewElement("ItemGroup");
                projectElement->InsertEndChild(newItemGroup);
            }

            std::wstring ext = fs::path(file.m_name).extension().wstring();
            std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

            tinyxml2::XMLElement *newItem = nullptr;
            if (ext == L".cpp" || ext == L".c")
            {
                newItem = doc.NewElement("ClCompile");
            }
            else if (ext == L".h" || ext == L".hpp")
            {
                newItem = doc.NewElement("ClInclude");
            }

            if (newItem)
            {
                newItem->SetAttribute("Include", fileName.c_str());
                newItemGroup->InsertEndChild(newItem);
            }
        }
    }

    // Save the file if changes were made
    if (newItemGroup)
    {
        eResult = doc.SaveFile(Ws2s(projectFileName).c_str());
        if (eResult != tinyxml2::XML_SUCCESS)
        {
            throw std::runtime_error("Failed to save project file");
        }
    }
}

/**
 * @brief プロジェクト名を更新する
 * @param hwnd ウィンドウハンドル
 */
void UpdateProjectName(HWND hwnd)
{
    wchar_t directoryPath[MAX_PATH];
    GetWindowTextW(g_hWndDirectoryEdit, directoryPath, MAX_PATH);

    // Check if the directory exists
    if (!fs::is_directory(directoryPath))
    {
        SetWindowTextW(g_hWndProjectNameEdit, L"");
        SetWindowTextW(g_hWndStatusText, L"Invalid directory path.");
        return;
    }

    std::wstring projectName = FindProjectFile(directoryPath);
    if (!projectName.empty())
    {
        SetWindowTextW(g_hWndProjectNameEdit, projectName.c_str());
        SetWindowTextW(g_hWndStatusText, L""); // Clear status message
    }
    else
    {
        SetWindowTextW(g_hWndProjectNameEdit, L"");
        SetWindowTextW(g_hWndStatusText, L"No .vcxproj file found in the selected directory.");
    }
}

/**
 * @brief フォルダ選択ダイアログを表示する
 * @return 選択されたパス（キャンセル時は空文字列）
 */
std::wstring BrowseFolder()
{
    IFileOpenDialog *pFileOpen;
    std::wstring selectedPath;

    // Create the FileOpenDialog object
    HRESULT hr = CoCreateInstance(CLSID_FileOpenDialog, NULL, CLSCTX_ALL,
                                  IID_IFileOpenDialog, reinterpret_cast<void **>(&pFileOpen));

    if (SUCCEEDED(hr))
    {
        // Set options
        DWORD dwOptions;
        pFileOpen->GetOptions(&dwOptions);
        pFileOpen->SetOptions(dwOptions | FOS_PICKFOLDERS);

        // Show the Open dialog box
        hr = pFileOpen->Show(NULL);

        if (SUCCEEDED(hr))
        {
            IShellItem *pItem;
            hr = pFileOpen->GetResult(&pItem);
            if (SUCCEEDED(hr))
            {
                PWSTR pszFilePath;
                hr = pItem->GetDisplayName(SIGDN_FILESYSPATH, &pszFilePath);
                if (SUCCEEDED(hr))
                {
                    selectedPath = pszFilePath;
                    CoTaskMemFree(pszFilePath);
                }
                pItem->Release();
            }
        }
        pFileOpen->Release();
    }
    return selectedPath;
}

/**
 * @brief フィルタ階層構造を構築する
 * @param directory 対象ディレクトリ
 * @return ルートノード
 */
FilterNode BuildFilterStructure(const std::wstring &directory)
{
    FilterNode root;
    root.m_name = fs::path(directory).filename().wstring();
    root.m_bIsFile = false;

    for (const auto &entry : fs::recursive_directory_iterator(directory))
    {
        std::wstring ext = entry.path().extension().wstring();
        std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

        if (entry.is_regular_file() && (ext == L".cpp" || ext == L".h" || ext == L".hpp"))
        {
            std::vector<std::wstring> path_parts;
            for (const auto &part : entry.path().lexically_relative(directory))
            {
                path_parts.push_back(part.wstring());
            }

            FilterNode *current = &root;
            for (size_t i = 0; i < path_parts.size(); ++i)
            {
                auto &part = path_parts[i];
                auto it = std::find_if(current->m_children.begin(), current->m_children.end(),
                                       [&part](const FilterNode &node)
                                       { return node.m_name == part; });

                if (it == current->m_children.end())
                {
                    current->m_children.emplace_back(FilterNode{part, {}, i == path_parts.size() - 1});
                    current = &current->m_children.back();
                }
                else
                {
                    current = &(*it);
                }
            }
        }
    }
    return root;
}

/**
 * @brief ツリービューにアイテムを追加する
 * @param hTreeView ツリービューのハンドル
 * @param hParent 親アイテムのハンドル
 * @param text 追加するテキスト
 * @return 追加されたアイテムのハンドル
 */
HTREEITEM AddItemToTree(HWND hTreeView, HTREEITEM hParent, const std::wstring &text)
{
    TVINSERTSTRUCT tvInsert = {0};
    tvInsert.hParent = hParent;
    tvInsert.hInsertAfter = TVI_LAST;
    tvInsert.item.mask = TVIF_TEXT;
    tvInsert.item.pszText = const_cast<LPWSTR>(text.c_str());
    return TreeView_InsertItem(hTreeView, &tvInsert);
}

/**
 * @brief ツリービューにフィルタ構造を反映する
 * @param hTreeView ツリービューのハンドル
 * @param hParent 親アイテムのハンドル
 * @param node 表示するフィルタノード
 */
void PopulateTreeView(HWND hTreeView, HTREEITEM hParent, const FilterNode &node)
{
    HTREEITEM hItem = AddItemToTree(hTreeView, hParent, node.m_name.c_str());
    for (const auto &child : node.m_children)
    {
        if (child.m_bIsFile)
        {
            // If it's a file, just add it
            AddItemToTree(hTreeView, hItem, child.m_name.c_str());
        }
        else
        {
            // If it's a directory, recursively add its children
            PopulateTreeView(hTreeView, hItem, child);
        }
    }
}

/**
 * @brief 確認ダイアログのウィンドウプロシージャ
 * @param hwnd ウィンドウハンドル
 * @param uMsg メッセージID
 * @param wParam メッセージの追加情報
 * @param lParam メッセージの追加情報
 * @return 処理結果
 */
INT_PTR CALLBACK ConfirmDialogProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
    case WM_INITDIALOG:
    {
        FilterNode *root = reinterpret_cast<FilterNode *>(lParam);

        g_hTreeView = CreateWindowExW(0, WC_TREEVIEW, L"",
                                      WS_VISIBLE | WS_CHILD | WS_BORDER | TVS_HASLINES | TVS_LINESATROOT | TVS_HASBUTTONS,
                                      10, 10, 380, 300, hwnd, (HMENU)IDC_TREEVIEW, GetModuleHandle(NULL), NULL);
        SendMessage(g_hTreeView, WM_SETFONT, (WPARAM)g_hFont, TRUE);

        PopulateTreeView(g_hTreeView, TVI_ROOT, *root);

        HWND hOkButton = CreateWindowW(L"BUTTON", L"OK", WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
                                       200, 320, 80, 30, hwnd, (HMENU)IDOK, GetModuleHandle(NULL), NULL);
        SendMessage(hOkButton, WM_SETFONT, (WPARAM)g_hFont, TRUE);

        HWND hCancelButton = CreateWindowW(L"BUTTON", L"Cancel", WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
                                           290, 320, 80, 30, hwnd, (HMENU)IDCANCEL, GetModuleHandle(NULL), NULL);
        SendMessage(hCancelButton, WM_SETFONT, (WPARAM)g_hFont, TRUE);
    }
        return TRUE;

    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        case IDOK:
        case IDCANCEL:
            EndDialog(hwnd, LOWORD(wParam));
            return TRUE;
        }
        break;

    case WM_CLOSE:
        EndDialog(hwnd, IDCANCEL);
        return TRUE;
    }

    return FALSE;
}

/**
 * @brief エディットコントロールのサブクラス化プロシージャ
 * @param hwnd ウィンドウハンドル
 * @param uMsg メッセージID
 * @param wParam メッセージの追加情報
 * @param lParam メッセージの追加情報
 * @return 処理結果
 */
LRESULT CALLBACK EditSubclassProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
    case WM_DROPFILES:
    {
        HDROP hDrop = (HDROP)wParam;
        wchar_t szFileName[MAX_PATH];
        if (DragQueryFileW(hDrop, 0, szFileName, MAX_PATH))
        {
            // Check if the dropped item is a directory
            DWORD fileAttributes = GetFileAttributesW(szFileName);
            if (fileAttributes != INVALID_FILE_ATTRIBUTES && (fileAttributes & FILE_ATTRIBUTE_DIRECTORY))
            {
                SetWindowTextW(hwnd, szFileName);
                UpdateProjectName(GetParent(hwnd));
            }
            else
            {
                SetWindowTextW(g_hWndStatusText, L"Please drop a directory, not a file.");
            }
        }
        DragFinish(hDrop);
        return 0;
    }
    case WM_KEYUP:
    case WM_KILLFOCUS:
        UpdateProjectName(GetParent(hwnd));
        break;
    }
    return CallWindowProc(g_oldEditProc, hwnd, uMsg, wParam, lParam);
}

/**
 * @brief メインウィンドウのウィンドウプロシージャ
 * @param hwnd ウィンドウハンドル
 * @param uMsg メッセージID
 * @param wParam メッセージの追加情報
 * @param lParam メッセージの追加情報
 * @return 処理結果
 */
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
    case WM_CREATE:
    {
        // Create custom font
        g_hFont = CreateCustomFont();

        // Project directory label
        HWND hWndStatic = CreateWindowW(L"STATIC", L"Project Directory:", WS_VISIBLE | WS_CHILD,
                                        10, 10, 150, 20, hwnd, NULL, NULL, NULL);
        SendMessage(hWndStatic, WM_SETFONT, (WPARAM)g_hFont, TRUE);

        g_hWndDirectoryEdit = CreateWindowW(L"EDIT", L"", WS_VISIBLE | WS_CHILD | WS_BORDER | ES_AUTOHSCROLL,
                                            10, 30, 250, 20, hwnd, NULL, NULL, NULL);
        SendMessage(g_hWndDirectoryEdit, WM_SETFONT, (WPARAM)g_hFont, TRUE);

        // Browse button
        g_hWndBrowseButton = CreateWindowW(L"BUTTON", L"Browse", WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
                                           270, 30, 60, 20, hwnd, (HMENU)2, NULL, NULL);
        SendMessage(g_hWndBrowseButton, WM_SETFONT, (WPARAM)g_hFont, TRUE);

        // Project name label
        HWND hWndStatic2 = CreateWindowW(L"STATIC", L"Project Name:", WS_VISIBLE | WS_CHILD,
                                         10, 60, 150, 20, hwnd, NULL, NULL, NULL);
        SendMessage(hWndStatic2, WM_SETFONT, (WPARAM)g_hFont, TRUE);

        g_hWndProjectNameEdit = CreateWindowW(L"EDIT", L"", WS_VISIBLE | WS_CHILD | WS_BORDER | ES_AUTOHSCROLL | ES_READONLY,
                                              10, 80, 300, 20, hwnd, NULL, NULL, NULL);
        SendMessage(g_hWndProjectNameEdit, WM_SETFONT, (WPARAM)g_hFont, TRUE);

        // Generate button
        g_hWndGenerateButton = CreateWindowW(L"BUTTON", L"Generate Filters", WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
                                             10, 110, 150, 30, hwnd, (HMENU)1, NULL, NULL);
        SendMessage(g_hWndGenerateButton, WM_SETFONT, (WPARAM)g_hFont, TRUE);

        // Status text
        g_hWndStatusText = CreateWindowW(L"STATIC", L"", WS_VISIBLE | WS_CHILD,
                                         10, 150, 300, 20, hwnd, NULL, NULL, NULL);
        SendMessage(g_hWndStatusText, WM_SETFONT, (WPARAM)g_hFont, TRUE);

        // Enable drag-and-drop for the directory edit control
        DragAcceptFiles(g_hWndDirectoryEdit, TRUE);

        // Subclass the Edit control
        g_oldEditProc = (WNDPROC)SetWindowLongPtr(g_hWndDirectoryEdit, GWLP_WNDPROC, (LONG_PTR)EditSubclassProc);

        break;
    }
    case WM_COMMAND:
        if (LOWORD(wParam) == 1)
        { // Generate Filters button clicked
            wchar_t directoryPath[MAX_PATH], projectName[MAX_PATH];
            GetWindowTextW(g_hWndDirectoryEdit, directoryPath, MAX_PATH);
            GetWindowTextW(g_hWndProjectNameEdit, projectName, MAX_PATH);

            if (!fs::is_directory(directoryPath))
            {
                SetWindowTextW(g_hWndStatusText, L"Invalid directory path. Please select a valid directory.");
                return 0;
            }

            if (wcslen(projectName) == 0)
            {
                SetWindowTextW(g_hWndStatusText, L"Project name is empty. Please select a valid directory with a .vcxproj file.");
                return 0;
            }

            FilterNode root = BuildFilterStructure(directoryPath);

            // Show confirmation dialog
            INT_PTR result = DialogBoxParam(
                GetModuleHandle(NULL),
                MAKEINTRESOURCE(IDD_CONFIRM_DIALOG),
                hwnd,
                ConfirmDialogProc,
                reinterpret_cast<LPARAM>(&root));

            if (result == IDOK)
            {
                auto files = GetProjectFiles(directoryPath);
                try
                {
                    GenerateFiltersFile(directoryPath, projectName, files);
                    UpdateProjectFile(directoryPath, projectName, files);
                    SetWindowTextW(g_hWndStatusText, L"Filters generated and project file updated successfully.");
                }
                catch (const std::exception &e)
                {
                    std::stringstream ss;
                    ss << "Error: " << e.what() << "\n";

                    fs::path filtersPath = fs::path(directoryPath) / (std::wstring(projectName) + L".vcxproj.filters");
                    if (fs::exists(filtersPath))
                    {
                        try
                        {
                            fs::file_status s = fs::status(filtersPath);
                            ss << "File permissions: " << static_cast<int>(s.permissions()) << "\n";

                            // Read the file contents for debugging
                            std::wifstream readFile(filtersPath);
                            if (readFile.is_open())
                            {
                                ss << "File contents:\n";
                                std::wstring line;
                                while (std::getline(readFile, line))
                                {
                                    ss << WStringToUtf8(line) << "\n";
                                }
                                readFile.close();
                            }
                            else
                            {
                                ss << "Unable to read file contents.\n";
                            }
                        }
                        catch (const fs::filesystem_error &fe)
                        {
                            ss << "Error checking file status: " << fe.what() << "\n";
                        }
                    }
                    else
                    {
                        ss << "Filters file does not exist.\n";
                    }

                    ss << "Project Directory: " << WStringToUtf8(directoryPath) << "\n"
                       << "Project Name: " << WStringToUtf8(projectName) << "\n"
                       << "Current Directory: " << WStringToUtf8(fs::current_path().wstring()) << "\n"
                       << "Free Disk Space: " << fs::space(directoryPath).available / (1024 * 1024) << " MB\n"
                       << "User Name: " << WStringToUtf8(GetCurrentUserName()) << "\n"
                       << "Process ID: " << GetCurrentProcessId();

                    std::string errorMsg = ss.str();
                    OutputDebugStringA(errorMsg.c_str()); // Output to debug console

                    // Write the error message to a log file
                    std::ofstream logFile("error_log.txt", std::ios::out | std::ios::trunc);
                    if (logFile.is_open())
                    {
                        logFile << "Error occurred at " << std::time(nullptr) << ":\n"
                                << errorMsg << "\n";
                        logFile.close();
                    }

                    std::wstring wideErrorMsg = std::wstring(errorMsg.begin(), errorMsg.end());
                    SetWindowTextW(g_hWndStatusText, wideErrorMsg.c_str());

                    // Show error message box
                    MessageBoxA(hwnd, errorMsg.c_str(), "Error", MB_OK | MB_ICONERROR);
                }
            }
            else
            {
                SetWindowTextW(g_hWndStatusText, L"Filter generation cancelled.");
            }
        }
        else if (LOWORD(wParam) == 2)
        { // Browse button clicked
            std::wstring selectedPath = BrowseFolder();
            if (!selectedPath.empty())
            {
                SetWindowTextW(g_hWndDirectoryEdit, selectedPath.c_str());
                UpdateProjectName(hwnd);
            }
        }
        break;
    case WM_DESTROY:
        DeleteObject(g_hFont); // Clean up font resource
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

/**
 * @brief アプリケーションのエントリーポイント
 * @param hInstance 現在のインスタンスハンドル
 * @param hPrevInstance 常にNULL（過去の互換性のため）
 * @param pCmdLine コマンドライン引数
 * @param nCmdShow ウィンドウの表示方法
 * @return プログラムの終了コード
 */
int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow)
{
    // 日本語文字コードサポートを設定
    setlocale(LC_ALL, "Japanese");
    std::locale::global(std::locale(""));

    // COM initialization
    HRESULT hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
    if (FAILED(hr))
    {
        MessageBoxW(NULL, L"COM initialization failed", L"Error", MB_OK | MB_ICONERROR);
        return 1;
    }

    // Register the window class
    const wchar_t CLASS_NAME[] = L"Filter Generator Window Class";

    WNDCLASS wc = {};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;

    RegisterClassW(&wc);

    HWND hwnd = CreateWindowExW(
        0,                            // Optional window styles
        CLASS_NAME,                   // Window class
        L"VS Filter Generator",       // Window text
        WS_OVERLAPPEDWINDOW,          // Window style
        CW_USEDEFAULT, CW_USEDEFAULT, // Position
        400, 250,                     // Size
        NULL,                         // Parent window
        NULL,                         // Menu
        hInstance,                    // Instance handle
        NULL                          // Additional application data
    );

    wc.lpfnWndProc = ConfirmDialogProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = L"ConfirmDialogClass";
    RegisterClassW(&wc);

    if (hwnd == NULL)
    {
        return 0;
    }

    // Adjust font size based on DPI
    HDC hdc = GetDC(hwnd);
    int dpi = GetDeviceCaps(hdc, LOGPIXELSY);
    ReleaseDC(hwnd, hdc);
    g_hFont = CreateCustomFont(-MulDiv(11, dpi, 72)); // Create 11-point font

    ShowWindow(hwnd, nCmdShow);

    // Message loop
    MSG msg = {};
    while (GetMessage(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    // COM uninitialization
    CoUninitialize();

    return 0;
}