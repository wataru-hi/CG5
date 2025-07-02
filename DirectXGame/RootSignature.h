#pragma once
#include <d3d12.h> // ID3D12RootSignature
#include <wrl/client.h>
using Microsoft::WRL::ComPtr;

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
	ComPtr<ID3D12RootSignature> rootSignature_ = nullptr;
};