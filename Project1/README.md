# Windows アプリケーションフレームワーク

このプロジェクトは、Windowsデスクトップアプリケーション開発のための軽量フレームワークを提供します。Win32 APIをベースにした抽象化レイヤーを使用して、モダンなC++でWindowsアプリケーションを簡単に開発することができます。

## 概要

このフレームワークは以下の機能を提供します：

- アプリケーションライフサイクル管理
- ウィンドウ作成と管理
- イベント処理システム
- リソース管理

## アーキテクチャ

フレームワークは以下の主要コンポーネントで構成されています：

### IApplication

アプリケーションの基本動作を定義するインターフェースです。初期化、実行、シャットダウンなどの基本的なライフサイクル機能を提供します。

### WinApplication

`IApplication`インターフェースのWindows実装です。Win32 APIを使用してWindowsアプリケーションを実装します。

### IWindow

ウィンドウの基本動作を定義するインターフェースです。

### TWindow

テンプレートベースのウィンドウ実装で、さまざまなウィンドウタイプを簡単に作成できます。

### IWindowContext

ウィンドウコンテキストを提供するインターフェースです。

### Win32WindowContext

Windows環境でのウィンドウコンテキスト実装です。

### WindowRoutines

ウィンドウのメッセージ処理やイベント処理を行うクラスです。

## ビルド方法

### 必要条件

- Windows 10以降
- Visual Studio 2019以降
- CMake 3.14以降

### ビルド手順

1. リポジトリをクローンします：

```bash
git clone <repository-url>
cd Project1
```

2. CMakeを使用してビルド：

```bash
mkdir build
cd build
cmake ..
cmake --build . --config Debug
```

または、Visual Studioのタスク「Build Project1」を実行します。

## 使用方法

基本的なウィンドウアプリケーションを作成する例：

```cpp
#include "WinApplication.h"
#include <Windows.h>

int APIENTRY wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow)
{
    // アプリケーションインスタンスを作成
    WinApplication app(hInstance, nCmdShow);
    
    // 初期化
    if (!app.Initialize(hInstance, L"サンプルアプリケーション", 800, 600, nCmdShow))
    {
        return -1;
    }
    
    // アプリケーションを実行
    return app.Run();
}
```

## ドキュメント

詳細なAPIドキュメントは、Doxygenを使用して生成できます：

```bash
doxygen Doxyfile
```

生成されたドキュメントは `docs/html/index.html` から閲覧できます。

## ライセンス

このプロジェクトはMITライセンスの下で公開されています。詳細については `LICENSE` ファイルを参照してください。