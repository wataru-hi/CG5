#pragma once

#include <d3d12.h> // ID3D12Resource, D3D12_INDEX_BUFFER_VIEW
#include <wrl/client.h>
using Microsoft::WRL::ComPtr;

class IndexBuffer {
public:
	// IndexBuffer生成
	void Create(const UINT size, const UINT stride);

	// ゲッター
	ID3D12Resource* Get();              // インデックスバッファー
	D3D12_INDEX_BUFFER_VIEW* GetView(); // インデックスバッファービュー
public:
	// デストラクタ
	~IndexBuffer();

private:
	ComPtr<ID3D12Resource> indexBuffer_ = nullptr;     // インデックスバッファ
	D3D12_INDEX_BUFFER_VIEW indexBufferView_{}; // インデックスバッファビュー
};		