#include "PipelineState.h"
#include "KamataEngine.h"

using namespace KamataEngine;

// PipelineStateを生成する
void PipelineState::Create(D3D12_GRAPHICS_PIPELINE_STATE_DESC graphicsPipelineStateDesc) {
	// クラス内で取得するために追加
	[[maybe_unused]] DirectXCommon* dxCommon = DirectXCommon::GetInstance();

	ID3D12PipelineState* graphicsPipelineState = nullptr;
	[[maybe_unused]] HRESULT hr = dxCommon->GetDevice()->CreateGraphicsPipelineState(&graphicsPipelineStateDesc, IID_PPV_ARGS(&graphicsPipelineState));
#ifdef _DEBUG

	assert(SUCCEEDED(hr));

#endif // _DEBUG
	
	// 生成した PipelineState をとっておく
	pipelineState_ = graphicsPipelineState;
}

// 生成した PipelineState を返す
ID3D12PipelineState* PipelineState::Get() { return pipelineState_.Get(); }

// コンストラクタ
PipelineState::PipelineState() {}

// デストラクタ
PipelineState::~PipelineState() {}