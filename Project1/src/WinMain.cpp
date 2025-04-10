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
#include <objbase.h>

//==============================================================================
// C++ 標準ライブラリ
//==============================================================================
#include <string>
#include <memory>
#include <locale>

//==============================================================================
// プロジェクト固有ヘッダー
//==============================================================================
#include "WinApplication.h"
#include "Win32WindowContext.h"

//==============================================================================
// プラグマディレクティブ
//==============================================================================
#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "uuid.lib")
#pragma comment(linker, "/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")
#pragma comment(linker, "/SUBSYSTEM:WINDOWS /ENTRY:wWinMainCRTStartup")

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
    try
    {
        // アプリケーションインスタンスを作成
        std::shared_ptr<WinApplication> app = std::make_shared<WinApplication>();

        // アプリケーションを初期化
        if (!app->Initialize(hInstance, L"VS Filter Generator", 400, 250, nCmdShow))
        {
            MessageBoxW(NULL, L"Failed to initialize application", L"Error", MB_OK | MB_ICONERROR);
            return 1;
        }

        // アプリケーションを実行（内部でメッセージループを処理）
        int result = app->Run();

        // 終了コードを返す
        return result;
    }
    catch (const std::exception &e)
    {
        // 例外をキャッチして表示
        std::string errorMsg = "Error: " + std::string(e.what());
        MessageBoxA(NULL, errorMsg.c_str(), "Exception", MB_OK | MB_ICONERROR);
        return 1;
    }

    return 0;
}