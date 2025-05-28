#include "KamataEngine.h"
#include "Shader.h"
#include "RootSignature.h"
#include "PipelineState.h"
#include "VertexBuffer.h"

#include <Windows.h>
#include <system_error>

using namespace KamataEngine;

// 関数プロトタイプ宣言
void SetupPipelineState(PipelineState& pipelineState, RootSignature& rs, Shader& vs, Shader& ps);

// Windowsアプリのエントリーポイント
int WINAPI WinMain(_In_ HINSTANCE, _In_opt_ HINSTANCE, _In_ LPSTR, _In_ int) {

	KamataEngine::Initialize(L"LE3D_12_ヒガ_ワタル");

	DirectXCommon* dxCommon = DirectXCommon::GetInstance();

	int32_t w = dxCommon->GetBackBufferWidth();
	int32_t h = dxCommon->GetBackBufferHeight();
	DebugText::GetInstance()->ConsolePrintf(std::format("width: {}, height: {}\n", w, h).c_str());

	// DirectXCommonクラスが管理しているコマンドリストを取得
	ID3D12GraphicsCommandList* commandList = dxCommon->GetCommandList();

#pragma region GraphicsPipelineSetup

#pragma region RootSignature
	RootSignature rs;
	rs.Create();
#pragma endregion

#pragma region ShaderCompile
	Shader vs;
	vs.LoadDxc(L"Resources/shaders/TestVertexShader.hlsl", L"vs_6_0");
	assert(vs.GetDxcBlob() != nullptr);

	Shader ps;
	ps.LoadDxc(L"Resources/shaders/TestPixelShader.hlsl", L"ps_6_0");
	assert(ps.GetDxcBlob() != nullptr);
#pragma endregion

#pragma region PipelineStateObject
	PipelineState pipelineState;
	SetupPipelineState(pipelineState, rs, vs, ps);
#pragma endregion

#pragma endregion

#pragma region VertexBuffer(VertexResource, VertexResourceView)
	VertexBuffer vb;
	vb.Create(sizeof(Vector4) * 3, sizeof(Vector4));
#pragma endregion

#pragma region VertexData
	// 頂点データの書き込み
	Vector4* vertexData = nullptr;
	vb.Get()->Map(0, nullptr, reinterpret_cast<void**>(&vertexData));
	vertexData[0] = {-0.5f, -0.5f, 0.0f, 1.0f}; // 左下
	vertexData[1] = {0.0f, 0.5f, 0.0f, 1.0f};   // 上
	vertexData[2] = {0.5f, -0.5f, 0.0f, 1.0f};  // 右下
	vb.Get()->Unmap(0, nullptr);
#pragma endregion

	while (true) {
		if (KamataEngine::Update()) {
			break;
		}

		dxCommon->PreDraw();

		// コマンドの設定
		commandList->SetGraphicsRootSignature(rs.Get());
		commandList->SetPipelineState(pipelineState.Get());
		commandList->IASetVertexBuffers(0, 1, vb.GetView());
		commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		commandList->DrawInstanced(3, 1, 0, 0);

		dxCommon->PostDraw();
	}

	KamataEngine::Finalize();

	return 0;
}

// インプットレイアウト、ブレンドステート、ラスタライザステート
// 引数として 空のpipelineState、RootSignature、頂点シェーダーvs、ピクセルシェーダーps を参照で受け取る
void SetupPipelineState(PipelineState& pipelineState, RootSignature& rs, Shader& vs, Shader& ps) {
	// InputLayout ----------------------------------------------------
	D3D12_INPUT_ELEMENT_DESC inputElementDescs[1] = {};
	inputElementDescs[0].SemanticName = "POSITION";
	inputElementDescs[0].SemanticIndex = 0;
	inputElementDescs[0].Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
	inputElementDescs[0].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;

	D3D12_INPUT_LAYOUT_DESC inputLayoutDesc{};
	inputLayoutDesc.pInputElementDescs = inputElementDescs;
	inputLayoutDesc.NumElements = _countof(inputElementDescs);

	// blendDesc
	D3D12_BLEND_DESC blendDesc{};
	blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
	
	// ラスタライザーステートの設定
	D3D12_RASTERIZER_DESC rasterizerDesc{};
	rasterizerDesc.CullMode = D3D12_CULL_MODE_BACK;  // 裏面をカリング
	rasterizerDesc.FillMode = D3D12_FILL_MODE_SOLID; // ソリッド塗りつぶし

	// PSO(PipelineStateObject)の生成
	D3D12_GRAPHICS_PIPELINE_STATE_DESC graphicsPipelineStateDesc{};
	graphicsPipelineStateDesc.pRootSignature = rs.Get();
	// RootSignature
	graphicsPipelineStateDesc.InputLayout = inputLayoutDesc;
	// InputLayout
	graphicsPipelineStateDesc.VS = {vs.GetDxcBlob()->GetBufferPointer(), vs.GetDxcBlob()->GetBufferSize()};
	// VertexShader
	graphicsPipelineStateDesc.PS = {ps.GetDxcBlob()->GetBufferPointer(), ps.GetDxcBlob()->GetBufferSize()};
	// PixelShader
	graphicsPipelineStateDesc.BlendState = blendDesc;
	// BlendState
	graphicsPipelineStateDesc.RasterizerState = rasterizerDesc;
	// RasterizerState

	// 書き込むRTVの情報
	graphicsPipelineStateDesc.NumRenderTargets = 1; // 1つのRTVに書き込む ※2つ同時も可能
	graphicsPipelineStateDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;

	// 利用するトポロジ（形状）のタイプ。三角形
	graphicsPipelineStateDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;

	// どのように画面に色を打ち込むかの設定（今は気にしなくていい）
	graphicsPipelineStateDesc.SampleDesc.Count = 1;
	graphicsPipelineStateDesc.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;

	// 準備は整った。PSOを生成する
	pipelineState.Create(graphicsPipelineStateDesc);
}