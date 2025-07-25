#include "KamataEngine.h"
#include "PipelineState.h"
#include "RootSignature.h"
#include "Shader.h"
#include "VertexBuffer.h"
#include "indexBuffer.h"
#include "WorldTransformEx.h"

#include <Windows.h>
#include <system_error>

using namespace KamataEngine;

// リソースの確保含め、頂点情報を柔軟に対応できるように VertexData構造体を新たに作成する
// Vertex4 => VertexData に変更して利用する
struct VertexData {
	Vector4 position;
	Vector2 texcoord;
};

struct PSConstants{
	BOOL gIsGrayScale;
};

// 関数プロトタイプ宣言 ----------------------------------------------------
// PipelineStateObjectの生成
void SetupPipelineState(PipelineState& pipelineState, RootSignature& rs, Shader& vs, Shader& ps);
// RenderTextureResourceの生成
ID3D12Resource* CreateRenderTextureResource(ID3D12Device* device, uint32_t width, uint32_t height, DXGI_FORMAT format, const FLOAT* clearColor);
// DepthStencilTextureResourceの生成
ID3D12Resource* CreateDepthStencilTextureResource(ID3D12Device* device, int32_t width, int32_t height);


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

	 // ディスクリプタレンジ (SRV用) を定義す
	D3D12_DESCRIPTOR_RANGE srvDescRange[1]{}; // 先にレンジを宣言
	srvDescRange[0].BaseShaderRegister = 0;
	srvDescRange[0].NumDescriptors = 1;
	srvDescRange[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	// 「0」から始まる
	srvDescRange[0].OffsetInDescriptorsFromTableStart = 0;

	// RootSignatureを修正: SRVに加えてCBVも追加
	// b0 (PSConstants) 用のRootParameterを追加
	D3D12_ROOT_PARAMETER rootParameters[2] = {};
	rootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE; // DescriptorTable
	rootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;           // PixelShaderで使う
	rootParameters[0].DescriptorTable.pDescriptorRanges = srvDescRange;           // Rangesは後で設定
	rootParameters[0].DescriptorTable.NumDescriptorRanges = _countof(srvDescRange); // SRV用
	// PSConstants用のRootParameter
	rootParameters[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;    // 定数バッファビュー
	rootParameters[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL; // PixelShaderで使う
	rootParameters[1].Descriptor.ShaderRegister = 0;                    // b0レジスタ

	rs.Create(2, rootParameters); // RootParameterの数を2に増やす

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

#pragma region RenderTarget_Heap_View_Creation
	// Resource生成、Heap生成、View生成 で再利用される変数の準備
	ID3D12Device* device = dxCommon->GetDevice();
	HRESULT hr;

	// 画面クリア色 ※分かりやすいように赤とする
	const FLOAT kRenderTargetClearColor[4] = {1.0f, 0.0f, 0.0f, 1.0f};

	Microsoft::WRL::ComPtr<ID3D12Resource> renderTextureResource =
	    CreateRenderTextureResource(device, WinApp::kWindowWidth, WinApp::kWindowHeight, DXGI_FORMAT_R8G8B8A8_UNORM_SRGB, kRenderTargetClearColor);

	// 1. RTV用のDescriptorHeapを作成する
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> rtvDescriptorHeap = nullptr;

	D3D12_DESCRIPTOR_HEAP_DESC rtvDescriptorHeapDesc{};
	rtvDescriptorHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV; // RTV
	rtvDescriptorHeapDesc.NumDescriptors = 1;                    // Descriptorの個数は 1

	hr = device->CreateDescriptorHeap(&rtvDescriptorHeapDesc, IID_PPV_ARGS(&rtvDescriptorHeap));
	assert(SUCCEEDED(hr));

	// CPU側からみたHANDLEを取得しておく
	D3D12_CPU_DESCRIPTOR_HANDLE rtvHandleCPU = rtvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();

	// 2. RTV用のViewの生成
	device->CreateRenderTargetView(
	    renderTextureResource.Get(), // Viewと関連付けたいリソース
		nullptr,               // RTVの詳細情報(Desc:Description、構成内容の記述)
								// ※RTVの場合 nullptrにするとDirectX12が自動で推測してくれる
		rtvHandleCPU           // RTV用ディスクリプタヒープの CPU Handle
	);
#pragma endregion

#pragma region depthStencil_Heap_View_Creation
	// 0. DepthStencilTextureResourceの作成
	Microsoft::WRL::ComPtr<ID3D12Resource> depthStencilResource = CreateDepthStencilTextureResource(device, WinApp::kWindowWidth, WinApp::kWindowHeight);

	// 1. DSV用のDescriptorHeapの作成
	Microsoft::WRL::ComPtr <ID3D12DescriptorHeap> dsvDescriptorHeap = nullptr;
	D3D12_DESCRIPTOR_HEAP_DESC dsvDescriptorHeapDesc{};
	dsvDescriptorHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV; // Heap Type
	dsvDescriptorHeapDesc.NumDescriptors = 1;                    // Heap の個数
	dsvDescriptorHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;          // DSVはShaderで触らないとする

	hr = device->CreateDescriptorHeap(&dsvDescriptorHeapDesc, IID_PPV_ARGS(&dsvDescriptorHeap));
	assert(SUCCEEDED(hr));

	// CPU側からみたHANDLEを取得しておく
	D3D12_CPU_DESCRIPTOR_HANDLE dsvHandleCPU = dsvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();

	// 2. DSV用の Viewの生成
	D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc{};
	dsvDesc.Format = DXGI_FORMAT_D32_FLOAT;                // 基本的にResourceに合わせる
	dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D; // 2D Texture

	// DSVHeapの先頭に DSVを作る
	device->CreateDepthStencilView(depthStencilResource.Get(), &dsvDesc, dsvHandleCPU);
#pragma endregion

#pragma region DescriptorHeap
	// 1. SRV用の DescriptorHeapの作成
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> srvDescriptorHeap = nullptr;

	D3D12_DESCRIPTOR_HEAP_DESC srvDescriptorHeapDesc = {};
	srvDescriptorHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;     // SRV
	srvDescriptorHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE; // PixelShader から見える
	srvDescriptorHeapDesc.NumDescriptors = 3;

	hr = device->CreateDescriptorHeap(&srvDescriptorHeapDesc, IID_PPV_ARGS(srvDescriptorHeap.GetAddressOf()));
	assert(SUCCEEDED(hr));

	// CPU側からみたHANDLE、GPU側からみたHANDLEを取得しておく
	D3D12_CPU_DESCRIPTOR_HANDLE srvHandleCPU = srvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
	D3D12_GPU_DESCRIPTOR_HANDLE srvHandleGPU = srvDescriptorHeap->GetGPUDescriptorHandleForHeapStart();

	// 2. SRV(Shader Resource View)の作成
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
	srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;                           // RenderTargetResource と同じにする
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING; // RGBA値をそのまま Shaderに対応させる
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;                      // 2Dテクスチャ
	srvDesc.Texture2D.MipLevels = 1;                                            // MipLevel は 1 しかない

	device->CreateShaderResourceView(
	    renderTextureResource.Get(), // Viewと関連付けたいリソース
	    &srvDesc,              // SRVの詳細情報(Desc:Description、構成内容の記述)
	    srvHandleCPU           // SRV用ディスクリプタヒープの CPU Handle
	);

	// 3. PSConstants 用の定数バッファリソースの作成
	Microsoft::WRL::ComPtr<ID3D12Resource> psConstantsResource = nullptr;
	D3D12_HEAP_PROPERTIES psConstantsHeapProp{};
	psConstantsHeapProp.Type = D3D12_HEAP_TYPE_UPLOAD; // CPUからGPUへの転送用
	D3D12_RESOURCE_DESC psConstantsResDesc{};
	psConstantsResDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	psConstantsResDesc.Width = (sizeof(PSConstants) + 0xff) & ~0xff; // 256バイトアラインメント
	psConstantsResDesc.Height = 1;
	psConstantsResDesc.DepthOrArraySize = 1;
	psConstantsResDesc.MipLevels = 1;
	psConstantsResDesc.SampleDesc.Count = 1;
	psConstantsResDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	hr = device->CreateCommittedResource(&psConstantsHeapProp, D3D12_HEAP_FLAG_NONE, &psConstantsResDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&psConstantsResource));
	assert(SUCCEEDED(hr));
	// PSConstants のマッピング
	PSConstants* pGpuPSConstants = nullptr;
	psConstantsResource->Map(0, nullptr, reinterpret_cast<void**>(&pGpuPSConstants));

	D3D12_CPU_DESCRIPTOR_HANDLE cbvPsHandleCPU = srvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
	cbvPsHandleCPU.ptr += device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV) * 1;
	D3D12_CONSTANT_BUFFER_VIEW_DESC cbvPsDesc{};
	cbvPsDesc.BufferLocation = psConstantsResource->GetGPUVirtualAddress();
	cbvPsDesc.SizeInBytes = (sizeof(PSConstants) + 0xff) & ~0xff;
	device->CreateConstantBufferView(&cbvPsDesc, cbvPsHandleCPU);
	D3D12_GPU_DESCRIPTOR_HANDLE cbvPsHandleGPU = srvDescriptorHeap->GetGPUDescriptorHandleForHeapStart();
	cbvPsHandleGPU.ptr += device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV) * 1;

#pragma endregion

#pragma region ３Dモデル
	// 被写体の準備
	Model* model = Model::CreateFromOBJ("terrain");

	WorldTransformEx worldTransform;
	worldTransform.Initialize();
	worldTransform.scale_ = Vector3(1.0f, 1.0f, 1.0f);

	// カメラの準備
	Camera camera;
	camera.Initialize();
	camera.translation_ = Vector3(0.0f, 1.0f, 0.0f);
#pragma endregion 

#pragma region PostEffect
	bool isGrayScale = true;
#pragma endregion

	while (true) {
		if (KamataEngine::Update()) {
			break;
		}

		Input::GetInstance()->GetAllKey();

		if (Input::GetInstance()->TriggerKey(DIK_G)) {
			isGrayScale = !isGrayScale;
		}

		// PSConstantsを更新
		pGpuPSConstants->gIsGrayScale = isGrayScale;

		// world変換行列の定数バッファへの転送
		worldTransform.rotation_.y += 0.005f;
		worldTransform.UpdateMatrix();

		// cameraの更新と定数バッファへの転送
		camera.UpdateMatrix();

		// TransitionBarrierを SRV => RTV に設定する
		D3D12_RESOURCE_BARRIER barrier{};
		barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION; // TranslationBarrierの設定
		barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;      // フラグは None にしておく
		barrier.Transition.pResource = renderTextureResource.Get();
		barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE; // 遷移前
		barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;          // 遷移後
		commandList->ResourceBarrier(1, &barrier);                                   // バリアを張る

		// 描画先の RTV と DSV を設定する
		commandList->OMSetRenderTargets(1, &rtvHandleCPU, false, &dsvHandleCPU);

		// Viewportの設定
		D3D12_VIEWPORT viewport{};
		viewport.Width = WinApp::kWindowWidth;
		viewport.Height = WinApp::kWindowHeight;
		viewport.TopLeftX = 0;
		viewport.TopLeftY = 0;
		viewport.MinDepth = 0.0f; // 深度の最小値
		viewport.MaxDepth = 1.0f; // 深度の最大値
		commandList->RSSetViewports(1, &viewport);

		// Scissorの設定
		D3D12_RECT scissorRect{};
		// 基本的にビューポートと同じ矩形が構成されるようにする
		scissorRect.left = 0;
		scissorRect.right = WinApp::kWindowWidth;
		scissorRect.top = 0;
		scissorRect.bottom = WinApp::kWindowHeight;

		

		commandList->RSSetScissorRects(1, &scissorRect);

		// 全画面クリア
		commandList->ClearRenderTargetView(rtvHandleCPU, kRenderTargetClearColor, 0, nullptr);
		// 指定した深度で画面全体をクリアする
		commandList->ClearDepthStencilView(dsvHandleCPU, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

		// --- モデルの描画処理 ---
		// RootSignatureをモデル描画用にする
		commandList->SetGraphicsRootSignature(rs.Get());
		// PSOをモデル描画用にする
		//commandList->SetPipelineState(pipelineState.Get());
		commandList->SetPipelineState(pipelineState.Get());
		// モデル描画に必要なVBV, IBV, トポロジを設定する
		commandList->IASetVertexBuffers(0, 1, vb.GetView());
		commandList->IASetIndexBuffer(ib.GetView());
		commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		// 使用するディスクリプタヒープを設定 (SRV, CBVをまとめて設定するヒープ)
		commandList->SetDescriptorHeaps(1, srvDescriptorHeap.GetAddressOf());
		// テクスチャのSRV (t0, RootParameter[0]) と PSConstantsのCBV (b0, RootParameter[1]) を設定
		commandList->SetGraphicsRootDescriptorTable(0, srvHandleGPU);
		commandList->SetGraphicsRootConstantBufferView(1, psConstantsResource->GetGPUVirtualAddress());

		Model::PreDraw(commandList);
		model->Draw(worldTransform, camera);
		Model::PostDraw();

		// TransitionBarrierを RTV => SRV に設定する
		barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION; // TranslationBarrierの設定
		barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;      // フラグは None にしておく
		barrier.Transition.pResource = renderTextureResource.Get();
		barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;        // 遷移前
		barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE; // 遷移後
		commandList->ResourceBarrier(1, &barrier);                                  // バリアを張る

		dxCommon->PreDraw();	

		commandList->SetGraphicsRootSignature(rs.Get());
		commandList->SetPipelineState(pipelineState.Get());

		commandList->IASetVertexBuffers(0, 1, vb.GetView());                      // VBVを設定する
		commandList->IASetIndexBuffer(ib.GetView());                              // IBVを設定する
		commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST); // トポロジを設定する

		commandList->SetDescriptorHeaps(1, srvDescriptorHeap.GetAddressOf());

		// ヴィネットシェーダーのt0に、モデルが描画されたレンダーテクスチャをSRVとして設定
		commandList->SetGraphicsRootDescriptorTable(0, srvHandleGPU);
		// RootParameter[1] にCBVを設定
		commandList->SetGraphicsRootConstantBufferView(1, psConstantsResource->GetGPUVirtualAddress());

		commandList->DrawIndexedInstanced(_countof(indices), 1, 0, 0, 0);

		// 描画終了
		dxCommon->PostDraw();
	}

	delete model;

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

ID3D12Resource* CreateDepthStencilTextureResource(ID3D12Device* device, int32_t width, int32_t height) { // 1. 生成するDepthStencilTextureのDescの設定
	D3D12_RESOURCE_DESC resourceDesc{};
	resourceDesc.Width = width;                                   // Textureの幅
	resourceDesc.Height = height;                                 // Textureの高さ
	resourceDesc.MipLevels = 1;                                   // mipmapの数 DepthStencilなので1でいい
	resourceDesc.DepthOrArraySize = 1;                            // Textureの配列数 DepthStencilは1でいい
	resourceDesc.Format = DXGI_FORMAT_D32_FLOAT;                  // DepthStencilとして利用可能なフォーマット
	                                                              // ※KamataEngineと合わせる
	resourceDesc.SampleDesc.Count = 1;                            // サンプリングカウント1固定
	resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;  // 2次元
	resourceDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL; // DepthStencilとして使う通知

	// 2. 利用するHeapの設定
	D3D12_HEAP_PROPERTIES heapProperties{};
	heapProperties.Type = D3D12_HEAP_TYPE_DEFAULT; // VRAM上に作る

	// 深度値のクリア設定
	D3D12_CLEAR_VALUE depthClearValue{};
	depthClearValue.DepthStencil.Depth = 1.0f;      // 1.0f(最大値)でクリア
	depthClearValue.Format = DXGI_FORMAT_D32_FLOAT; // Zバッファ形式、resourceと合わせる
	                                                // ※KamataEngineと合わせた

	// 3. Resourceの生成
	ID3D12Resource* resource = nullptr;
	HRESULT hr = device->CreateCommittedResource(
	    &heapProperties,                  // Heapの設定
	    D3D12_HEAP_FLAG_NONE,             // Heapの特殊な設定 ★後で変更？
	    &resourceDesc,                    // Resourceの設定
	    D3D12_RESOURCE_STATE_DEPTH_WRITE, // 深度値を書き込み状態にしておく
	    &depthClearValue,                 // Clear最適値
	    IID_PPV_ARGS(&resource)           // 作成するResourceポインタへのポインタ
	);
	assert(SUCCEEDED(hr));

	return resource;
}
