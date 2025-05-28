#pragma once
#include <d3d12.h> // ID3D12RootSignature

class RootSignature {
public:
	// 生成 (Generate/Create)
	void Create();

	// ゲッター (Getter)
	ID3D12RootSignature* Get();

	// コンストラクタ (Constructor)
	RootSignature();
	// デストラクタ (Destructor)
	~RootSignature();

private:
	ID3D12RootSignature* rootSignature_ = nullptr;
};