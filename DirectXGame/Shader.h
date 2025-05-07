#pragma once

#include <string>
#include <d3d12.h>

class Shader {
public:
	// シェーダーファイルを読み込み、コンパイルデータを作成
	void Load(const std::wstring& filePath, const std::string& shaderModel);

	// 生成したコンパイル済みのデータを取得する
	ID3DBlob* GetBlob();

	Shader();
	~Shader();

private:
	ID3DBlob* blob_ = nullptr;
};