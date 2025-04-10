#pragma once

#include <windows.h>
#include <string>
#include <vector>
#include <filesystem>

// FilterNodeとFileInfo型を前方宣言
class FileUtils;

/**
 * @brief ファイル操作に関するユーティリティクラス
 */
class FileUtils
{
public:
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

    /**
     * @brief プロジェクト内のソースファイルとヘッダーファイルを取得する
     * @param directory 検索対象のディレクトリパス
     * @return ファイル情報のベクタ
     */
    static std::vector<FileInfo> GetProjectFiles(const std::wstring &directory);

    /**
     * @brief Visual Studio用のフィルターファイルを生成する
     * @param projectDirectory プロジェクトディレクトリ
     * @param projectName プロジェクト名
     * @param files ファイル情報のリスト
     * @throw std::runtime_error ファイル生成に失敗した場合
     */
    static void GenerateFiltersFile(const std::wstring &projectDirectory, const std::wstring &projectName, const std::vector<FileInfo> &files);

    /**
     * @brief プロジェクトファイルを更新する
     * @param projectDirectory プロジェクトディレクトリ
     * @param projectName プロジェクト名
     * @param files ファイル情報のリスト
     * @throw std::runtime_error ファイル操作に失敗した場合
     */
    static void UpdateProjectFile(const std::wstring &projectDirectory, const std::wstring &projectName, const std::vector<FileInfo> &files);

    /**
     * @brief フィルタ階層構造を構築する
     * @param directory 対象ディレクトリ
     * @return ルートノード
     */
    static FilterNode BuildFilterStructure(const std::wstring &directory);

    /**
     * @brief ディレクトリ内のプロジェクトファイルを検索し、プロジェクト名を取得する
     * @param directory 検索対象ディレクトリ
     * @return プロジェクト名（見つからない場合は空文字列）
     */
    static std::wstring FindProjectFile(const std::wstring &directory);

    /**
     * @brief GUIDを生成する
     * @return フォーマット済みのGUID文字列（ブレースなし）
     * @throw std::runtime_error GUID生成に失敗した場合
     */
    static std::wstring GenerateGuid();

    /**
     * @brief XML特殊文字をエスケープする
     * @param input エスケープ対象の文字列
     * @return エスケープ済みの文字列
     */
    static std::wstring XmlEscape(const std::wstring &input);

    /**
     * @brief 現在のユーザー名を取得する
     * @return ユーザー名（ワイド文字列）
     */
    static std::wstring GetCurrentUserName();

    /**
     * @brief ワイド文字列をstd::stringに変換する
     * @param wstr 変換元のワイド文字列
     * @return 変換後の文字列
     */
    static std::string Ws2s(const std::wstring &wstr);

    /**
     * @brief ワイド文字列をUTF-8エンコードの文字列に変換する
     * @param wstr 変換元のワイド文字列
     * @return UTF-8エンコードの文字列
     */
    static std::string WStringToUtf8(const std::wstring &wstr);
};

// グローバルな型の別名定義
using FilterNode = FileUtils::FilterNode;
using FileInfo = FileUtils::FileInfo;
