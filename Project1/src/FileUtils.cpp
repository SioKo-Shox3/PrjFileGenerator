#include "FileUtils.h"
#include "tinyxml2.h"

#include <windows.h>
#include <lmcons.h>
#include <objbase.h>
#include <shobjidl.h>

#include <string>
#include <vector>
#include <map>
#include <set>
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <regex>
#include <system_error>

namespace fs = std::filesystem;

std::wstring FileUtils::GetCurrentUserName()
{
    wchar_t username[UNLEN + 1];
    DWORD username_len = UNLEN + 1;
    if (GetUserNameW(username, &username_len))
    {
        return std::wstring(username);
    }
    return L"Unknown";
}

std::wstring FileUtils::GenerateGuid()
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

std::wstring FileUtils::FindProjectFile(const std::wstring &directory)
{
    try
    {
        for (const auto &entry : fs::directory_iterator(directory))
        {
            if (entry.path().extension() == L".vcxproj")
            {
                std::wifstream file(entry.path());
                if (!file.is_open())
                {
                    continue;
                }

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
    }
    catch (const std::exception &)
    {
        // ディレクトリが存在しないなどのエラーを無視して空文字を返す
    }
    return L""; // No project file found
}

void FileUtils::UpdateProjectFile(const std::wstring &projectDirectory, const std::wstring &projectName, const std::vector<FileInfo> &files)
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
        eResult = doc.SaveFile(utf8FileName.c_str());
        if (eResult != tinyxml2::XML_SUCCESS)
        {
            throw std::runtime_error("Failed to save project file");
        }
    }
}

void FileUtils::GenerateFiltersFile(const std::wstring &projectDirectory, const std::wstring &projectName, const std::vector<FileInfo> &files)
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

std::vector<FileUtils::FileInfo> FileUtils::GetProjectFiles(const std::wstring &directory)
{
    std::vector<FileInfo> files;
    std::wstring baseDir = directory;

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

FileUtils::FilterNode FileUtils::BuildFilterStructure(const std::wstring &directory)
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

std::wstring FileUtils::XmlEscape(const std::wstring &input)
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

std::string FileUtils::WStringToUtf8(const std::wstring &wstr)
{
    if (wstr.empty())
        return std::string();
    int size_needed = WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), NULL, 0, NULL, NULL);
    std::string strTo(size_needed, 0);
    WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), &strTo[0], size_needed, NULL, NULL);
    return strTo;
}

std::string FileUtils::Ws2s(const std::wstring &wstr)
{
    std::string str(wstr.begin(), wstr.end());
    return str;
}
