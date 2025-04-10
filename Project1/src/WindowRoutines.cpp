#include "../include/WindowRoutines.h"
#include "../include/FileUtils.h"
#include "../resource/resource.h"

#include <windows.h>
#include <lmcons.h>
#include <commctrl.h>
#include <windowsx.h>
#include <shlobj.h>
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
#include <codecvt>
#include <locale>
#include <system_error>
#include <iomanip>
#include <ctime>

#include "tinyxml2.h"

namespace fs = std::filesystem;

// 現在のMainWindowRoutineインスタンスへの静的ポインタ
// サブクラス化されたウィンドウプロシージャがMainWindowRoutineのメンバーにアクセスするために必要
static MainWindowRoutine *g_pMainWindowRoutine = nullptr;

//==============================================================================
// MainWindowRoutine の実装
//==============================================================================

MainWindowRoutine::MainWindowRoutine()
    : m_hWndDirectoryEdit(NULL),
      m_hWndProjectNameEdit(NULL),
      m_hWndGenerateButton(NULL),
      m_hWndStatusText(NULL),
      m_hWndBrowseButton(NULL),
      m_hFont(NULL),
      m_oldEditProc(NULL)
{
    g_pMainWindowRoutine = this;
}

MainWindowRoutine::~MainWindowRoutine()
{
    if (m_hFont != NULL)
    {
        DeleteObject(m_hFont);
    }
    g_pMainWindowRoutine = nullptr;
}

void MainWindowRoutine::OnCreate(IWindow *window)
{
    // フォント作成
    HDC hdc = GetDC(window->GetHandle());
    int dpi = GetDeviceCaps(hdc, LOGPIXELSY);
    ReleaseDC(window->GetHandle(), hdc);

    // DPIに合わせたフォントサイズを設定
    m_hFont = CreateFontW(
        -MulDiv(11, dpi, 72), 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE,
        L"Segoe UI");

    CreateControls(window->GetHandle());
}

void MainWindowRoutine::OnDestroy(IWindow *window)
{
    // リソースのクリーンアップ
    if (m_hFont != NULL)
    {
        DeleteObject(m_hFont);
        m_hFont = NULL;
    }
}

void MainWindowRoutine::OnUpdate(IWindow *window, float deltaTime)
{
    // 定期的な更新処理が必要な場合はここに実装
}

LRESULT MainWindowRoutine::OnMessage(IWindow *window, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WM_COMMAND:
        if (LOWORD(wParam) == 1)
        { // Generate Filters ボタン
            OnGenerateButtonClick(window->GetHandle());
            return 0;
        }
        else if (LOWORD(wParam) == 2)
        { // Browse ボタン
            OnBrowseButtonClick(window->GetHandle());
            return 0;
        }
        break;
    }

    return DefWindowProc(window->GetHandle(), msg, wParam, lParam);
}

void MainWindowRoutine::CreateControls(HWND hwnd)
{
    // Project directory label
    HWND hWndStatic = CreateWindowW(L"STATIC", L"Project Directory:", WS_VISIBLE | WS_CHILD,
                                    10, 10, 150, 20, hwnd, NULL, NULL, NULL);
    SendMessage(hWndStatic, WM_SETFONT, (WPARAM)m_hFont, TRUE);

    // ディレクトリ入力フィールド
    m_hWndDirectoryEdit = CreateWindowW(L"EDIT", L"", WS_VISIBLE | WS_CHILD | WS_BORDER | ES_AUTOHSCROLL,
                                        10, 30, 250, 20, hwnd, NULL, NULL, NULL);
    SendMessage(m_hWndDirectoryEdit, WM_SETFONT, (WPARAM)m_hFont, TRUE);

    // Browse ボタン
    m_hWndBrowseButton = CreateWindowW(L"BUTTON", L"Browse", WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
                                       270, 30, 60, 20, hwnd, (HMENU)2, NULL, NULL);
    SendMessage(m_hWndBrowseButton, WM_SETFONT, (WPARAM)m_hFont, TRUE);

    // Project name label
    HWND hWndStatic2 = CreateWindowW(L"STATIC", L"Project Name:", WS_VISIBLE | WS_CHILD,
                                     10, 60, 150, 20, hwnd, NULL, NULL, NULL);
    SendMessage(hWndStatic2, WM_SETFONT, (WPARAM)m_hFont, TRUE);

    // プロジェクト名入力フィールド（読み取り専用）
    m_hWndProjectNameEdit = CreateWindowW(L"EDIT", L"", WS_VISIBLE | WS_CHILD | WS_BORDER | ES_AUTOHSCROLL | ES_READONLY,
                                          10, 80, 300, 20, hwnd, NULL, NULL, NULL);
    SendMessage(m_hWndProjectNameEdit, WM_SETFONT, (WPARAM)m_hFont, TRUE);

    // Generate ボタン
    m_hWndGenerateButton = CreateWindowW(L"BUTTON", L"Generate Filters", WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
                                         10, 110, 150, 30, hwnd, (HMENU)1, NULL, NULL);
    SendMessage(m_hWndGenerateButton, WM_SETFONT, (WPARAM)m_hFont, TRUE);

    // ステータステキスト
    m_hWndStatusText = CreateWindowW(L"STATIC", L"", WS_VISIBLE | WS_CHILD,
                                     10, 150, 300, 20, hwnd, NULL, NULL, NULL);
    SendMessage(m_hWndStatusText, WM_SETFONT, (WPARAM)m_hFont, TRUE);

    // ドラッグ&ドロップを有効化
    DragAcceptFiles(m_hWndDirectoryEdit, TRUE);

    // エディットコントロールのサブクラス化
    m_oldEditProc = (WNDPROC)SetWindowLongPtr(m_hWndDirectoryEdit, GWLP_WNDPROC, (LONG_PTR)EditSubclassProc);
}

void MainWindowRoutine::OnBrowseButtonClick(HWND hwnd)
{
    std::wstring selectedPath = BrowseFolder();
    if (!selectedPath.empty())
    {
        SetWindowTextW(m_hWndDirectoryEdit, selectedPath.c_str());
        UpdateProjectName(hwnd);
    }
}

void MainWindowRoutine::OnGenerateButtonClick(HWND hwnd)
{
    wchar_t directoryPath[MAX_PATH], projectName[MAX_PATH];
    GetWindowTextW(m_hWndDirectoryEdit, directoryPath, MAX_PATH);
    GetWindowTextW(m_hWndProjectNameEdit, projectName, MAX_PATH);

    // 入力チェック
    if (!fs::is_directory(directoryPath))
    {
        SetWindowTextW(m_hWndStatusText, L"Invalid directory path. Please select a valid directory.");
        return;
    }

    if (wcslen(projectName) == 0)
    {
        SetWindowTextW(m_hWndStatusText, L"Project name is empty. Please select a valid directory with a .vcxproj file.");
        return;
    }

    // フィルター構造を構築
    FileUtils::FilterNode root = FileUtils::BuildFilterStructure(directoryPath);

    // 確認ダイアログを表示
    auto onConfirm = [this, hwnd, directoryPath, projectName]()
    {
        try
        {
            auto files = FileUtils::GetProjectFiles(directoryPath);
            FileUtils::GenerateFiltersFile(directoryPath, projectName, files);
            FileUtils::UpdateProjectFile(directoryPath, projectName, files);
            SetWindowTextW(m_hWndStatusText, L"Filters generated and project file updated successfully.");
        }
        catch (const std::exception &e)
        {
            std::string errorMsg = "Error: " + std::string(e.what());
            std::wstring wideErrorMsg = std::wstring(errorMsg.begin(), errorMsg.end());
            SetWindowTextW(m_hWndStatusText, wideErrorMsg.c_str());

            // エラーメッセージボックスを表示
            MessageBoxA(hwnd, errorMsg.c_str(), "Error", MB_OK | MB_ICONERROR);
        }
    };

    auto onCancel = [this]()
    {
        SetWindowTextW(m_hWndStatusText, L"Filter generation cancelled.");
    };

    // 確認ダイアログ用に新しいウィンドウを作成
    HWND hDlg = CreateDialogParam(
        GetModuleHandle(NULL),
        MAKEINTRESOURCE(IDD_CONFIRM_DIALOG),
        hwnd,
        NULL, // ダイアログプロシージャはウィンドウルーチンで処理
        0);

    if (hDlg)
    {
        // コンストラクタに直接渡す代わりに一度変数に格納
        std::function<void()> confirmCallback = onConfirm;
        std::function<void()> cancelCallback = onCancel;
        ConfirmDialogRoutine dialogRoutine(root, confirmCallback, cancelCallback);
        dialogRoutine.OnCreate(nullptr); // ウィンドウ作成処理
    }
}

void MainWindowRoutine::UpdateProjectName(HWND hwnd)
{
    wchar_t directoryPath[MAX_PATH];
    GetWindowTextW(m_hWndDirectoryEdit, directoryPath, MAX_PATH);

    // ディレクトリの存在確認
    if (!fs::is_directory(directoryPath))
    {
        SetWindowTextW(m_hWndProjectNameEdit, L"");
        SetWindowTextW(m_hWndStatusText, L"Invalid directory path.");
        return;
    }

    std::wstring projectName = FileUtils::FindProjectFile(directoryPath);
    if (!projectName.empty())
    {
        SetWindowTextW(m_hWndProjectNameEdit, projectName.c_str());
        SetWindowTextW(m_hWndStatusText, L""); // ステータスメッセージをクリア
    }
    else
    {
        SetWindowTextW(m_hWndProjectNameEdit, L"");
        SetWindowTextW(m_hWndStatusText, L"No .vcxproj file found in the selected directory.");
    }
}

std::wstring MainWindowRoutine::BrowseFolder()
{
    IFileOpenDialog *pFileOpen;
    std::wstring selectedPath;

    // FileOpenDialogオブジェクトの作成
    HRESULT hr = CoCreateInstance(CLSID_FileOpenDialog, NULL, CLSCTX_ALL,
                                  IID_IFileOpenDialog, reinterpret_cast<void **>(&pFileOpen));

    if (SUCCEEDED(hr))
    {
        // オプション設定
        DWORD dwOptions;
        pFileOpen->GetOptions(&dwOptions);
        pFileOpen->SetOptions(dwOptions | FOS_PICKFOLDERS);

        // ダイアログ表示
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

LRESULT CALLBACK MainWindowRoutine::EditSubclassProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    if (!g_pMainWindowRoutine)
        return DefWindowProc(hwnd, uMsg, wParam, lParam);

    switch (uMsg)
    {
    case WM_DROPFILES:
    {
        HDROP hDrop = (HDROP)wParam;
        wchar_t szFileName[MAX_PATH];
        if (DragQueryFileW(hDrop, 0, szFileName, MAX_PATH))
        {
            // ドロップされたアイテムがディレクトリかチェック
            DWORD fileAttributes = GetFileAttributesW(szFileName);
            if (fileAttributes != INVALID_FILE_ATTRIBUTES && (fileAttributes & FILE_ATTRIBUTE_DIRECTORY))
            {
                SetWindowTextW(hwnd, szFileName);
                g_pMainWindowRoutine->UpdateProjectName(GetParent(hwnd));
            }
            else
            {
                SetWindowTextW(g_pMainWindowRoutine->m_hWndStatusText, L"Please drop a directory, not a file.");
            }
        }
        DragFinish(hDrop);
        return 0;
    }
    case WM_KEYUP:
    case WM_KILLFOCUS:
        g_pMainWindowRoutine->UpdateProjectName(GetParent(hwnd));
        break;
    }
    return CallWindowProc(g_pMainWindowRoutine->m_oldEditProc, hwnd, uMsg, wParam, lParam);
}

//==============================================================================
// ConfirmDialogRoutine の実装
//==============================================================================

ConfirmDialogRoutine::ConfirmDialogRoutine(
    const FileUtils::FilterNode &root,
    std::function<void()> onConfirm,
    std::function<void()> onCancel)
    : m_root(root), m_onConfirm(onConfirm), m_onCancel(onCancel),
      m_hTreeView(NULL), m_hFont(NULL)
{
}

ConfirmDialogRoutine::~ConfirmDialogRoutine()
{
    if (m_hFont != NULL)
    {
        DeleteObject(m_hFont);
    }
}

void ConfirmDialogRoutine::OnCreate(IWindow *window)
{
    if (window)
    {
        HWND hwnd = window->GetHandle();

        // フォントの作成
        m_hFont = CreateFontW(
            16, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
            CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE,
            L"Segoe UI");

        CreateTreeView(hwnd);
        CreateButtons(hwnd);
        PopulateTreeView(hwnd);
    }
}

void ConfirmDialogRoutine::OnDestroy(IWindow *window)
{
    // リソースのクリーンアップ
    if (m_hFont != NULL)
    {
        DeleteObject(m_hFont);
        m_hFont = NULL;
    }
}

void ConfirmDialogRoutine::OnUpdate(IWindow *window, float deltaTime)
{
    // 定期的な更新処理が必要な場合はここに実装
}

LRESULT ConfirmDialogRoutine::OnMessage(IWindow *window, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        case IDOK:
            if (m_onConfirm)
            {
                m_onConfirm();
            }
            EndDialog(window->GetHandle(), IDOK);
            return TRUE;

        case IDCANCEL:
            if (m_onCancel)
            {
                m_onCancel();
            }
            EndDialog(window->GetHandle(), IDCANCEL);
            return TRUE;
        }
        break;

    case WM_CLOSE:
        if (m_onCancel)
        {
            m_onCancel();
        }
        EndDialog(window->GetHandle(), IDCANCEL);
        return TRUE;
    }

    return DefWindowProc(window->GetHandle(), msg, wParam, lParam);
}

void ConfirmDialogRoutine::CreateTreeView(HWND hwnd)
{
    m_hTreeView = CreateWindowExW(
        0, WC_TREEVIEW, L"",
        WS_VISIBLE | WS_CHILD | WS_BORDER | TVS_HASLINES | TVS_LINESATROOT | TVS_HASBUTTONS,
        10, 10, 380, 300, hwnd, (HMENU)IDC_TREEVIEW, GetModuleHandle(NULL), NULL);

    SendMessage(m_hTreeView, WM_SETFONT, (WPARAM)m_hFont, TRUE);
}

void ConfirmDialogRoutine::CreateButtons(HWND hwnd)
{
    // OKボタン
    HWND hOkButton = CreateWindowW(
        L"BUTTON", L"OK", WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
        200, 320, 80, 30, hwnd, (HMENU)IDOK, GetModuleHandle(NULL), NULL);
    SendMessage(hOkButton, WM_SETFONT, (WPARAM)m_hFont, TRUE);

    // キャンセルボタン
    HWND hCancelButton = CreateWindowW(
        L"BUTTON", L"Cancel", WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
        290, 320, 80, 30, hwnd, (HMENU)IDCANCEL, GetModuleHandle(NULL), NULL);
    SendMessage(hCancelButton, WM_SETFONT, (WPARAM)m_hFont, TRUE);
}

void ConfirmDialogRoutine::PopulateTreeView(HWND hwnd)
{
    // ツリービューにアイテムを追加するヘルパー関数
    auto addItemToTree = [this](HTREEITEM hParent, const std::wstring &text) -> HTREEITEM
    {
        TVINSERTSTRUCT tvInsert = {0};
        tvInsert.hParent = hParent;
        tvInsert.hInsertAfter = TVI_LAST;
        tvInsert.item.mask = TVIF_TEXT;
        tvInsert.item.pszText = const_cast<LPWSTR>(text.c_str());
        return TreeView_InsertItem(m_hTreeView, &tvInsert);
    };

    // 再帰的にツリーを構築する関数
    std::function<void(HTREEITEM, const FileUtils::FilterNode &)> populateTree =
        [&](HTREEITEM hParent, const FileUtils::FilterNode &node)
    {
        HTREEITEM hItem = addItemToTree(hParent, node.m_name.c_str());
        for (const auto &child : node.m_children)
        {
            if (child.m_bIsFile)
            {
                // ファイルの場合は単にアイテムを追加
                addItemToTree(hItem, child.m_name.c_str());
            }
            else
            {
                // ディレクトリの場合は再帰的に処理
                populateTree(hItem, child);
            }
        }
    };

    // ルートからツリーを構築
    populateTree(TVI_ROOT, m_root);
}