#pragma once
#include<d3d12.h> // ID3D12PipelineState
#include <wrl/client.h>
using Microsoft::WRL::ComPtr;

    class PipelineState {
public:
	// 初期化
	void Create(D3D12_GRAPHICS_PIPELINE_STATE_DESC desc);
	// ゲッター
	ID3D12PipelineState* Get();

	// コンストラクタ
	PipelineState();
	// デストラクタ
	~PipelineState();

private:
	// パイプラインステート
	ComPtr<ID3D12PipelineState> pipelineState_ = nullptr;
};
