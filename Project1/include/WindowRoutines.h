#pragma once

#include "TWindow.h"
#include "FileUtils.h"
#include <windows.h>
#include <commctrl.h>
#include <string>
#include <memory>
#include <vector>
#include <map>

/**
 * @brief フィルタージェネレータのメインウィンドウルーチン
 */
class MainWindowRoutine : public IWindowRoutine
{
public:
    /**
     * @brief コンストラクタ
     */
    MainWindowRoutine();

    /**
     * @brief デストラクタ
     */
    virtual ~MainWindowRoutine();

    /**
     * @brief 作成時の処理
     * @param window ウィンドウ
     */
    void OnCreate(IWindow *window) override;

    /**
     * @brief 破棄時の処理
     * @param window ウィンドウ
     */
    void OnDestroy(IWindow *window) override;

    /**
     * @brief 更新処理
     * @param window ウィンドウ
     * @param deltaTime フレーム間の経過時間
     */
    void OnUpdate(IWindow *window, float deltaTime) override;

    /**
     * @brief メッセージ処理
     * @param window ウィンドウ
     * @param msg メッセージID
     * @param wParam wParam
     * @param lParam lParam
     * @return 処理結果
     */
    LRESULT OnMessage(IWindow *window, UINT msg, WPARAM wParam, LPARAM lParam) override;

private:
    // カスタムコントロールハンドル
    HWND m_hWndDirectoryEdit;
    HWND m_hWndProjectNameEdit;
    HWND m_hWndGenerateButton;
    HWND m_hWndStatusText;
    HWND m_hWndBrowseButton;

    // フォントハンドル
    HFONT m_hFont;

    // エディットコントロールのサブクラス化
    WNDPROC m_oldEditProc;

    // 内部処理用のヘルパーメソッド
    void CreateControls(HWND hwnd);
    void OnBrowseButtonClick(HWND hwnd);
    void OnGenerateButtonClick(HWND hwnd);
    void UpdateProjectName(HWND hwnd);
    std::wstring BrowseFolder();

    // メッセージハンドラ
    static LRESULT CALLBACK EditSubclassProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
};

/**
 * @brief 確認ダイアログウィンドウルーチン
 */
class ConfirmDialogRoutine : public IWindowRoutine
{
public:
    /**
     * @brief コンストラクタ
     * @param root フィルタ構造のルートノード
     * @param onConfirm 確認時のコールバック
     * @param onCancel キャンセル時のコールバック
     */
    ConfirmDialogRoutine(
        const FileUtils::FilterNode &root,
        std::function<void()> onConfirm,
        std::function<void()> onCancel);

    /**
     * @brief デストラクタ
     */
    virtual ~ConfirmDialogRoutine();

    /**
     * @brief 作成時の処理
     * @param window ウィンドウ
     */
    void OnCreate(IWindow *window) override;

    /**
     * @brief 破棄時の処理
     * @param window ウィンドウ
     */
    void OnDestroy(IWindow *window) override;

    /**
     * @brief 更新処理
     * @param window ウィンドウ
     * @param deltaTime フレーム間の経過時間
     */
    void OnUpdate(IWindow *window, float deltaTime) override;

    /**
     * @brief メッセージ処理
     * @param window ウィンドウ
     * @param msg メッセージID
     * @param wParam wParam
     * @param lParam lParam
     * @return 処理結果
     */
    LRESULT OnMessage(IWindow *window, UINT msg, WPARAM wParam, LPARAM lParam) override;

private:
    FileUtils::FilterNode m_root;
    std::function<void()> m_onConfirm;
    std::function<void()> m_onCancel;
    HWND m_hTreeView;
    HFONT m_hFont;

    void CreateTreeView(HWND hwnd);
    void CreateButtons(HWND hwnd);
    void PopulateTreeView(HWND hwnd);
};
