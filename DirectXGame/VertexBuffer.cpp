#include "VertexBuffer.h"
#include "KamataEngine.h"

#include "indexBuffer.h"
#include <cassert>  // assert
#include <d3dx12.h> // ID3D~、D3D~

using namespace KamataEngine;

// 生成
void VertexBuffer::Create(const UINT size, const UINT stride) {
	D3D12_HEAP_PROPERTIES uploadHeapProperties{};
	uploadHeapProperties.Type = D3D12_HEAP_TYPE_UPLOAD;

	D3D12_RESOURCE_DESC vertexResourceDesc{};
	vertexResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	vertexResourceDesc.Width = size;
	vertexResourceDesc.Height = 1;
	vertexResourceDesc.DepthOrArraySize = 1;
	vertexResourceDesc.MipLevels = 1;
	vertexResourceDesc.SampleDesc.Count = 1;
	vertexResourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

	ComPtr<ID3D12Resource> vertexResource = nullptr;

	DirectXCommon* dxCommon = DirectXCommon::GetInstance();
	HRESULT hr =
	    dxCommon->GetDevice()->CreateCommittedResource(&uploadHeapProperties, D3D12_HEAP_FLAG_NONE, &vertexResourceDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&vertexResource));
	assert(SUCCEEDED(hr)); // デバッグ時はアサート
	if (FAILED(hr)) {
		// エラー処理を追加
		throw std::runtime_error("Failed to create committed resource.");
	}

	vertexBuffer_ = vertexResource;

	D3D12_VERTEX_BUFFER_VIEW vertexBufferView{};
	vertexBufferView.BufferLocation = vertexResource->GetGPUVirtualAddress();
	vertexBufferView.SizeInBytes = size;
	vertexBufferView.StrideInBytes = stride;
	vertexBufferView_ = vertexBufferView;
}

// 生成した頂点バッファーを返す
ID3D12Resource* VertexBuffer::Get() { return vertexBuffer_.Get(); }

// 用意済みの頂点バッファービューを返す
D3D12_VERTEX_BUFFER_VIEW* VertexBuffer::GetView() { return &vertexBufferView_; }

// コンストラクタ
VertexBuffer::VertexBuffer() {}

// デストラクタ
VertexBuffer::~VertexBuffer() {}
