#include "WindowRoutines.h"
#include "FileUtils.h"
#include "resource.h"

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

/**
 * @brief コンストラクタ
 *
 * メンバ変数を初期化し、グローバルインスタンスポインタを設定します
 */
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

/**
 * @brief デストラクタ
 *
 * フォントリソースを解放し、グローバルインスタンスポインタをリセットします
 */
MainWindowRoutine::~MainWindowRoutine()
{
    if (m_hFont != NULL)
    {
        DeleteObject(m_hFont);
    }
    g_pMainWindowRoutine = nullptr;
}

/**
 * @brief ウィンドウ作成時の処理
 *
 * フォントを作成し、UIコントロールを初期化します
 *
 * @param window 作成されるウィンドウオブジェクト
 */
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

/**
 * @brief ウィンドウ破棄時の処理
 *
 * フォントなどのリソースを解放します
 *
 * @param window 破棄されるウィンドウオブジェクト
 */
void MainWindowRoutine::OnDestroy(IWindow *window)
{
    // リソースのクリーンアップ
    if (m_hFont != NULL)
    {
        DeleteObject(m_hFont);
        m_hFont = NULL;
    }
}

/**
 * @brief 定期的な更新処理
 *
 * フレームごとの更新処理を行います
 *
 * @param window 更新するウィンドウオブジェクト
 * @param deltaTime 前回の更新からの経過時間（秒）
 */
void MainWindowRoutine::OnUpdate(IWindow *window, float deltaTime)
{
    // 定期的な更新処理が必要な場合はここに実装
}

/**
 * @brief ウィンドウメッセージ処理
 *
 * ボタンクリックなどのウィンドウメッセージを処理します
 *
 * @param window ウィンドウオブジェクト
 * @param msg メッセージID
 * @param wParam 追加のメッセージ情報
 * @param lParam 追加のメッセージ情報
 * @return メッセージ処理結果
 */
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

/**
 * @brief UIコントロールの作成
 *
 * ウィンドウ内の各UIコントロール（ラベル、テキストボックス、ボタンなど）を作成します
 *
 * @param hwnd 親ウィンドウのハンドル
 */
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

/**
 * @brief Browseボタンのクリックイベント処理
 *
 * フォルダ選択ダイアログを表示し、選択されたパスをテキストボックスに設定します
 *
 * @param hwnd 親ウィンドウのハンドル
 */
void MainWindowRoutine::OnBrowseButtonClick(HWND hwnd)
{
    std::wstring selectedPath = BrowseFolder();
    if (!selectedPath.empty())
    {
        SetWindowTextW(m_hWndDirectoryEdit, selectedPath.c_str());
        UpdateProjectName(hwnd);
    }
}

/**
 * @brief Generate Filtersボタンのクリックイベント処理
 *
 * プロジェクトファイルとフィルターファイルの生成処理を実行します
 *
 * @param hwnd 親ウィンドウのハンドル
 */
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

    try
    {
        // フィルター構造を構築
        FileUtils::FilterNode root = FileUtils::BuildFilterStructure(directoryPath);

        // 確認ダイアログを表示するコールバック関数
        std::function<void()> onConfirm = [this, hwnd, directoryPath, projectName]()
        {
            try
            {
                std::vector<FileUtils::FileInfo> files = FileUtils::GetProjectFiles(directoryPath);
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

        std::function<void()> onCancel = [this]()
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
            try
            {
                // コンストラクタに直接渡す代わりに一度変数に格納
                std::function<void()> confirmCallback = onConfirm;
                std::function<void()> cancelCallback = onCancel;
                ConfirmDialogRoutine dialogRoutine(root, confirmCallback, cancelCallback);
                dialogRoutine.OnCreate(nullptr); // ウィンドウ作成処理
            }
            catch (const std::exception &e)
            {
                std::string errorMsg = "Error initializing dialog: " + std::string(e.what());
                MessageBoxA(hwnd, errorMsg.c_str(), "Error", MB_OK | MB_ICONERROR);
                SetWindowTextW(m_hWndStatusText, L"Failed to create confirmation dialog.");
                DestroyWindow(hDlg);
            }
        }
        else
        {
            // ダイアログの作成に失敗した場合
            DWORD error = GetLastError();
            std::string errorMsg = "Failed to create dialog. Error code: " + std::to_string(error);
            MessageBoxA(hwnd, errorMsg.c_str(), "Error", MB_OK | MB_ICONERROR);
            SetWindowTextW(m_hWndStatusText, L"Failed to create confirmation dialog.");
        }
    }
    catch (const std::exception &e)
    {
        std::string errorMsg = "Error: " + std::string(e.what());
        std::wstring wideErrorMsg = std::wstring(errorMsg.begin(), errorMsg.end());
        SetWindowTextW(m_hWndStatusText, wideErrorMsg.c_str());

        // エラーメッセージボックスを表示
        MessageBoxA(hwnd, errorMsg.c_str(), "Error", MB_OK | MB_ICONERROR);
    }
}

/**
 * @brief プロジェクト名の更新
 *
 * 選択されたディレクトリ内の.vcxprojファイルからプロジェクト名を取得し表示します
 *
 * @param hwnd 親ウィンドウのハンドル
 */
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

/**
 * @brief フォルダ選択ダイアログの表示
 *
 * Common Item Dialogを使用してフォルダ選択ダイアログを表示します
 *
 * @return 選択されたフォルダのパス、キャンセルされた場合は空文字列
 */
std::wstring MainWindowRoutine::BrowseFolder()
{
    IFileOpenDialog *pFileOpen = nullptr;
    std::wstring selectedPath;

    try
    {
        // FileOpenDialogオブジェクトの作成
        HRESULT hr = CoCreateInstance(CLSID_FileOpenDialog, NULL, CLSCTX_ALL,
                                      IID_IFileOpenDialog, reinterpret_cast<void **>(&pFileOpen));
        if (FAILED(hr))
        {
            throw std::runtime_error("Failed to create FileOpenDialog instance");
        }

        // オプション設定
        DWORD dwOptions;
        hr = pFileOpen->GetOptions(&dwOptions);
        if (FAILED(hr))
        {
            throw std::runtime_error("Failed to get FileOpenDialog options");
        }

        hr = pFileOpen->SetOptions(dwOptions | FOS_PICKFOLDERS);
        if (FAILED(hr))
        {
            throw std::runtime_error("Failed to set folder picker option");
        }

        // ダイアログ表示
        hr = pFileOpen->Show(NULL);
        if (hr == HRESULT_FROM_WIN32(ERROR_CANCELLED))
        {
            // ユーザーがキャンセルした場合は正常な動作
            return selectedPath;
        }
        else if (FAILED(hr))
        {
            throw std::runtime_error("Failed to show folder picker dialog");
        }

        // 結果の取得
        IShellItem *pItem = nullptr;
        hr = pFileOpen->GetResult(&pItem);
        if (FAILED(hr))
        {
            throw std::runtime_error("Failed to get folder picker result");
        }

        // パスの取得
        PWSTR pszFilePath = nullptr;
        hr = pItem->GetDisplayName(SIGDN_FILESYSPATH, &pszFilePath);
        if (FAILED(hr))
        {
            pItem->Release();
            throw std::runtime_error("Failed to get file path from shell item");
        }

        // 取得したパスをstd::wstringに変換
        selectedPath = pszFilePath;

        // リソースの解放
        CoTaskMemFree(pszFilePath);
        pItem->Release();
    }
    catch (const std::exception &e)
    {
        // エラーメッセージをログに記録する（必要に応じて）
        OutputDebugStringA(e.what());

        // 親ウィンドウのステータステキストに表示
        if (m_hWndStatusText)
        {
            std::string errorMsg = e.what();
            std::wstring wideErrorMsg(errorMsg.begin(), errorMsg.end());
            SetWindowTextW(m_hWndStatusText, wideErrorMsg.c_str());
        }
    }

    // FileOpenDialogインスタンスの解放
    if (pFileOpen)
    {
        pFileOpen->Release();
    }

    return selectedPath;
}

/**
 * @brief エディットコントロールのサブクラスプロシージャ
 *
 * エディットコントロールのメッセージを処理するサブクラス化されたウィンドウプロシージャです。
 * 特にドラッグ＆ドロップ処理を実装しています。
 *
 * @param hwnd エディットコントロールのハンドル
 * @param uMsg メッセージID
 * @param wParam 追加のメッセージ情報
 * @param lParam 追加のメッセージ情報
 * @return メッセージ処理結果
 */
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

/**
 * @brief コンストラクタ
 *
 * 確認ダイアログの初期設定を行います
 *
 * @param root フィルター構造のルートノード
 * @param onConfirm OKボタン押下時のコールバック
 * @param onCancel キャンセル時のコールバック
 */
ConfirmDialogRoutine::ConfirmDialogRoutine(
    const FileUtils::FilterNode &root,
    std::function<void()> onConfirm,
    std::function<void()> onCancel)
    : m_root(root), m_onConfirm(onConfirm), m_onCancel(onCancel),
      m_hTreeView(NULL), m_hFont(NULL)
{
}

/**
 * @brief デストラクタ
 *
 * フォントリソースを解放します
 */
ConfirmDialogRoutine::~ConfirmDialogRoutine()
{
    if (m_hFont != NULL)
    {
        DeleteObject(m_hFont);
    }
}

/**
 * @brief ダイアログ作成時の処理
 *
 * フォントを作成し、ツリービューとボタンを初期化します
 *
 * @param window ウィンドウオブジェクト
 */
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

/**
 * @brief ダイアログ破棄時の処理
 *
 * フォントなどのリソースを解放します
 *
 * @param window 破棄されるウィンドウオブジェクト
 */
void ConfirmDialogRoutine::OnDestroy(IWindow *window)
{
    // リソースのクリーンアップ
    if (m_hFont != NULL)
    {
        DeleteObject(m_hFont);
        m_hFont = NULL;
    }
}

/**
 * @brief ダイアログの定期的な更新処理
 *
 * フレームごとの更新処理を行います
 *
 * @param window 更新するウィンドウオブジェクト
 * @param deltaTime 前回の更新からの経過時間（秒）
 */
void ConfirmDialogRoutine::OnUpdate(IWindow *window, float deltaTime)
{
    // 定期的な更新処理が必要な場合はここに実装
}

/**
 * @brief ダイアログのメッセージ処理
 *
 * OKボタンやキャンセルボタンなどのメッセージを処理します
 *
 * @param window ウィンドウオブジェクト
 * @param msg メッセージID
 * @param wParam 追加のメッセージ情報
 * @param lParam 追加のメッセージ情報
 * @return メッセージ処理結果
 */
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

/**
 * @brief ツリービューコントロールの作成
 *
 * フィルター構造を表示するためのツリービューコントロールを作成します
 *
 * @param hwnd 親ウィンドウのハンドル
 */
void ConfirmDialogRoutine::CreateTreeView(HWND hwnd)
{
    m_hTreeView = CreateWindowExW(
        0, WC_TREEVIEW, L"",
        WS_VISIBLE | WS_CHILD | WS_BORDER | TVS_HASLINES | TVS_LINESATROOT | TVS_HASBUTTONS,
        10, 10, 380, 300, hwnd, (HMENU)IDC_TREEVIEW, GetModuleHandle(NULL), NULL);

    SendMessage(m_hTreeView, WM_SETFONT, (WPARAM)m_hFont, TRUE);
}

/**
 * @brief ダイアログのボタン作成
 *
 * OKボタンとキャンセルボタンを作成します
 *
 * @param hwnd 親ウィンドウのハンドル
 */
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

/**
 * @brief ツリービューにフィルター構造を表示
 *
 * 再帰的にフィルター構造をツリービュー内に構築します
 *
 * @param hwnd 親ウィンドウのハンドル
 */
void ConfirmDialogRoutine::PopulateTreeView(HWND hwnd)
{
    // ツリービューにアイテムを追加するヘルパー関数
    std::function<HTREEITEM(HTREEITEM, const std::wstring &)> addItemToTree =
        [this](HTREEITEM hParent, const std::wstring &text) -> HTREEITEM
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
        for (const FileUtils::FilterNode &child : node.m_children)
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