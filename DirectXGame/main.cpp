#include "KamataEngine.h"
#include "PipelineState.h"
#include "RootSignature.h"
#include "Shader.h"
#include "VertexBuffer.h"
#include "indexBuffer.h"

#include <Windows.h>
#include <system_error>

using namespace KamataEngine;

// 関数プロトタイプ宣言
void SetupPipelineState(PipelineState& pipelineState, RootSignature& rs, Shader& vs, Shader& ps);

// リソースの確保含め、頂点情報を柔軟に対応できるように VertexData構造体を新たに作成する
// Vertex4 => VertexData に変更して利用する
struct VertexData {
	Vector4 position;
	Vector2 texcoord;
};

// 関数プロトタイプ宣言 ----------------------------------------------------
// PipelineStateObjectの生成
void SetupPipelineState(PipelineState& pipelineState, RootSignature& rs, Shader& vs, Shader& ps);
// RenderTextureResourceの生成
ID3D12Resource* CreateRenderTextureResource(ID3D12Device* device, uint32_t width, uint32_t height, DXGI_FORMAT format, const FLOAT* clearColor);

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
#pragma region VertexData

	VertexData vertices[] = {
	    {{-1.0f, 1.0f, 0.0f, 1.0f},  {0.0f, 0.0f}}, // 左上
	    {{1.0f, 1.0f, 0.0f, 1.0f},   {1.0f, 0.0f}}, // 右上
	    {{-1.0f, -1.0f, 0.0f, 1.0f}, {0.0f, 1.0f}}, // 左下
	    {{1.0f, -1.0f, 0.0f, 1.0f},  {1.0f, 1.0f}}, // 右下
	};

#pragma endregion

	VertexBuffer vb;
	vb.Create(sizeof(vertices), sizeof(vertices[0]));
	// vb.Create(sizeof(Vector4) * 3, sizeof(Vector4));

	// 頂点リソースにデータを書き込む -------- ★00_07 追加
	VertexData* pGpuVertices = nullptr;
	vb.Get()->Map(0, nullptr, reinterpret_cast<void**>(&pGpuVertices));

	for (int i = 0; i < _countof(vertices); ++i) {
		pGpuVertices[i] = vertices[i];
	}
#pragma endregion

#pragma region IndexBuffer
	uint16_t indices[] = {
	    0, 1, 2, // 1枚目の三角形（左上, 右上, 左下）
	    2, 1, 3  // 2枚目の三角形（左下, 右上, 右下）
	};

	// IndexBuffer(IndexResource, IndexResourceView)の生成
	IndexBuffer ib;
	ib.Create(sizeof(indices), sizeof(indices[0]));

	// 頂点インデックスリソースにデータを書き込む
	uint16_t* pGpuIndices = nullptr;
	ib.Get()->Map(0, nullptr, reinterpret_cast<void**>(&pGpuIndices));

	for (int i = 0; i < _countof(indices); ++i) {
		pGpuIndices[i] = indices[i];
	}
#pragma endregion

#pragma region レンダーターゲット、ヒープ、ビューの作成
	// Resource生成、Heap生成、View生成 で再利用される変数の準備
	ID3D12Device* device = dxCommon->GetDevice();
	HRESULT hr;

	// 0. RenderTextureResourceの作成

	// 画面クリア色 ※分かりやすいように赤とする
	const FLOAT kRenderTargetClearColor[4] = {1.0f, 0.0f, 0.0f, 1.0f};

	ID3D12Resource* renderTextureResource = CreateRenderTextureResource(device, WinApp::kWindowWidth, WinApp::kWindowHeight, DXGI_FORMAT_R8G8B8A8_UNORM_SRGB, kRenderTargetClearColor);

	// 1. RTV用のDescriptorHeapを作成する
	ID3D12DescriptorHeap* rtvDescriptorHeap = nullptr;

	D3D12_DESCRIPTOR_HEAP_DESC rtvDescriptorHeapDesc{};
	rtvDescriptorHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV; // RTV
	rtvDescriptorHeapDesc.NumDescriptors = 1;                    // Descriptorの個数は 1

	hr = device->CreateDescriptorHeap(&rtvDescriptorHeapDesc, IID_PPV_ARGS(&rtvDescriptorHeap));
	assert(SUCCEEDED(hr));

	// CPU側からみたHANDLEを取得しておく
	D3D12_CPU_DESCRIPTOR_HANDLE rtvHandleCPU = rtvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();

	// 2. RTV用のViewの生成
	device->CreateRenderTargetView(
		renderTextureResource, // Viewと関連付けたいリソース
		nullptr,               // RTVの詳細情報(Desc:Description、構成内容の記述)
								// ※RTVの場合 nullptrにするとDirectX12が自動で推測してくれる
		rtvHandleCPU           // RTV用ディスクリプタヒープの CPU Handle
	);
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
		commandList->IASetIndexBuffer(ib.GetView());
		commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		commandList->DrawIndexedInstanced(_countof(indices), 1, 0, 0, 0);

		dxCommon->PostDraw();
	}

	KamataEngine::Finalize();

	return 0;
}

// インプットレイアウト、ブレンドステート、ラスタライザステート
// 引数として 空のpipelineState、RootSignature、頂点シェーダーvs、ピクセルシェーダーps を参照で受け取る
void SetupPipelineState(PipelineState& pipelineState, RootSignature& rs, Shader& vs, Shader& ps) {
	// InputLayout ----------------------------------------------------
	D3D12_INPUT_ELEMENT_DESC inputElementDescs[2] = {};
	inputElementDescs[0].SemanticName = "POSITION";
	inputElementDescs[0].SemanticIndex = 0;
	inputElementDescs[0].Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
	inputElementDescs[0].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
	inputElementDescs[1].SemanticName = "TEXCOORD";
	inputElementDescs[1].SemanticIndex = 0;
	inputElementDescs[1].Format = DXGI_FORMAT_R32G32_FLOAT;
	inputElementDescs[1].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
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

// RenderTextureResourceの生成
ID3D12Resource* CreateRenderTextureResource(ID3D12Device* device, uint32_t width, uint32_t height, DXGI_FORMAT clearFormat, const FLOAT* clearColor) {

	// 1. 生成するRenderTextureのDescの設定
	D3D12_RESOURCE_DESC resourceDesc{};
	resourceDesc.Width = UINT(width);                             // RenderTextureの幅
	resourceDesc.Height = UINT(height);                           // Textureの高さ
	resourceDesc.MipLevels = 1;                                   // mipmapの数
	resourceDesc.DepthOrArraySize = 1;                            // 奥行 or 配列Textureの配列数
	resourceDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;        // TextureのFormat
	resourceDesc.SampleDesc.Count = 1;                            // サンプリングカウント1固定
	resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;  // Textureの次元数。普段使っているのは2次元
	resourceDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET; // RenderTargetとして使う通知

	// 2. 利用するHeapの設定
	D3D12_HEAP_PROPERTIES heapProperties{};
	heapProperties.Type = D3D12_HEAP_TYPE_DEFAULT; // VRAM上に作る

	// 3. ClaarValueの用意
	D3D12_CLEAR_VALUE clearValue;
	clearValue.Format = clearFormat;
	clearValue.Color[0] = clearColor[0];
	clearValue.Color[1] = clearColor[1];
	clearValue.Color[2] = clearColor[2];
	clearValue.Color[3] = clearColor[3];

	// 4. RenderTextureResourceの生成
	ID3D12Resource* resource = nullptr;
	HRESULT hr = device->CreateCommittedResource(
	    &heapProperties,                            // Heapの設定
	    D3D12_HEAP_FLAG_NONE,                       // Heapの特殊な設定
	    &resourceDesc,                              // Resourceの設定
	    D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, // Pixel Shader でアクセスできるようにする
	    &clearValue,                                // Clear最適値
	    IID_PPV_ARGS(&resource)                     // 作成するResourceポインタへのポインタ
	);
	assert(SUCCEEDED(hr));

	return resource;
}