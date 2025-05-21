#pragma once

#include <string>
#include <d3dcompiler.h>
#include <dxcapi.h>

class Shader {
public:
	// シェーダーファイルを読み込み、コンパイルデータを作成
	void Load(const std::wstring& filePath, const std::wstring& shaderModel);
	void LoadDxc(const std::wstring& filePath, const std::wstring& shaderModel);

	// 生成したコンパイル済みのデータを取得する
	ID3DBlob* GetBlob();
	IDxcBlob* GetDxcBlob();

	Shader();
	~Shader();

private:

	ID3DBlob* blob_ = nullptr;

	IDxcBlob* dxcBlob_ = nullptr;
};