#include "KamataEngine.h"
#include <Windows.h>
#include <d3dcompiler.h>
#include <system_error>

using namespace KamataEngine;

ID3DBlob* CompileShader(const std::wstring& filePath, const std::string& shaderModel);

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
	// ルートシグネチャの設定
	D3D12_ROOT_SIGNATURE_DESC descriptionRootSignature{};
	descriptionRootSignature.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

	ID3DBlob* signatureBlob = nullptr;
	ID3DBlob* errorBlob = nullptr;

	HRESULT hr = D3D12SerializeRootSignature(&descriptionRootSignature, D3D_ROOT_SIGNATURE_VERSION_1, &signatureBlob, &errorBlob);
	if (FAILED(hr)) {
		DebugText::GetInstance()->ConsolePrintf(reinterpret_cast<char*>(errorBlob->GetBufferPointer()));
		assert(false);
	}

	// ルートシグネチャを生成
	ID3D12RootSignature* rootSignature = nullptr;
	hr = dxCommon->GetDevice()->CreateRootSignature(0, signatureBlob->GetBufferPointer(), signatureBlob->GetBufferSize(), IID_PPV_ARGS(&rootSignature));
	assert(SUCCEEDED(hr));
#pragma endregion

#pragma region InputLayout
	// 入力レイアウトの設定
	D3D12_INPUT_ELEMENT_DESC inputElementDescs[1] = {};
	inputElementDescs[0].SemanticName = "POSITION";
	inputElementDescs[0].SemanticIndex = 0;
	inputElementDescs[0].Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
	inputElementDescs[0].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;

	D3D12_INPUT_LAYOUT_DESC inputLayoutDesc{};
	inputLayoutDesc.pInputElementDescs = inputElementDescs;
	inputLayoutDesc.NumElements = _countof(inputElementDescs);
#pragma endregion

#pragma region BlendState
	// ブレンドステートの設定
	D3D12_BLEND_DESC blendDesc{};
	blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
#pragma endregion

#pragma region RasterizerState
	// ラスタライザーステートの設定
	D3D12_RASTERIZER_DESC rasterizerDesc{};
	rasterizerDesc.CullMode = D3D12_CULL_MODE_BACK;  // 裏面をカリング
	rasterizerDesc.FillMode = D3D12_FILL_MODE_SOLID; // ソリッド塗りつぶし
#pragma endregion

#pragma region ShaderCompile
	// 頂点シェーダの読み込みとコンパイル
	ID3DBlob* vsBlob = CompileShader(L"Resources/shaders/TestVertexShader.hlsl", "vs_5_0");
	ID3DBlob* psBlob = CompileShader(L"Resources/shaders/TestPixelShader.hlsl", "ps_5_0");
#pragma endregion

#pragma region PipelineStateObject
	// PSO(PipelineStateObject)の生成
	D3D12_GRAPHICS_PIPELINE_STATE_DESC graphicsPipelineStateDesc{};
	graphicsPipelineStateDesc.pRootSignature = rootSignature;
	graphicsPipelineStateDesc.InputLayout = inputLayoutDesc;
	graphicsPipelineStateDesc.VS = {vsBlob->GetBufferPointer(), vsBlob->GetBufferSize()};
	graphicsPipelineStateDesc.PS = {psBlob->GetBufferPointer(), psBlob->GetBufferSize()};
	graphicsPipelineStateDesc.BlendState = blendDesc;
	graphicsPipelineStateDesc.RasterizerState = rasterizerDesc;
	graphicsPipelineStateDesc.NumRenderTargets = 1;
	graphicsPipelineStateDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
	graphicsPipelineStateDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	graphicsPipelineStateDesc.SampleDesc.Count = 1;
	graphicsPipelineStateDesc.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;

	ID3D12PipelineState* graphicsPipelineState = nullptr;
	hr = dxCommon->GetDevice()->CreateGraphicsPipelineState(&graphicsPipelineStateDesc, IID_PPV_ARGS(&graphicsPipelineState));
	assert(SUCCEEDED(hr));
#pragma endregion

#pragma endregion

#pragma region VertexResource
	// 頂点リソースの作成
	D3D12_HEAP_PROPERTIES uploadHeapProperties{};
	uploadHeapProperties.Type = D3D12_HEAP_TYPE_UPLOAD;

	D3D12_RESOURCE_DESC vertexResourceDesc{};
	vertexResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	vertexResourceDesc.Width = sizeof(Vector4) * 3;
	vertexResourceDesc.Height = 1;
	vertexResourceDesc.DepthOrArraySize = 1;
	vertexResourceDesc.MipLevels = 1;
	vertexResourceDesc.SampleDesc.Count = 1;
	vertexResourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

	ID3D12Resource* vertexResource = nullptr;
	hr = dxCommon->GetDevice()->CreateCommittedResource(&uploadHeapProperties, D3D12_HEAP_FLAG_NONE, &vertexResourceDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&vertexResource));
	assert(SUCCEEDED(hr));
#pragma endregion

#pragma region VertexBufferView
	// 頂点バッファビューの生成
	D3D12_VERTEX_BUFFER_VIEW vertexBufferView{};
	vertexBufferView.BufferLocation = vertexResource->GetGPUVirtualAddress();
	vertexBufferView.SizeInBytes = sizeof(Vector4) * 3;
	vertexBufferView.StrideInBytes = sizeof(Vector4);
#pragma endregion

#pragma region VertexData
	// 頂点データの書き込み
	Vector4* vertexData = nullptr;
	vertexResource->Map(0, nullptr, reinterpret_cast<void**>(&vertexData));
	vertexData[0] = {-0.5f, -0.5f, 0.0f, 1.0f}; // 左下
	vertexData[1] = {0.0f, 0.5f, 0.0f, 1.0f};   // 上
	vertexData[2] = {0.5f, -0.5f, 0.0f, 1.0f};  // 右下
	vertexResource->Unmap(0, nullptr);
#pragma endregion

	while (true) {
		if (KamataEngine::Update()) {
			break;
		}

		dxCommon->PreDraw();

		// コマンドの設定
		commandList->SetGraphicsRootSignature(rootSignature);
		commandList->SetPipelineState(graphicsPipelineState);
		commandList->IASetVertexBuffers(0, 1, &vertexBufferView);
		commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		commandList->DrawInstanced(3, 1, 0, 0);

		dxCommon->PostDraw();
	}

	// リソースの解放
	vertexResource->Release();
	graphicsPipelineState->Release();
	signatureBlob->Release();
	rootSignature->Release();
	vsBlob->Release();
	psBlob->Release();

	KamataEngine::Finalize();

	return 0;
}

// シェーダーコンパイル関数
// filePath    : シェーダーファイルのパス   例 "Resources/shaders/TestVS.hlsl"
// shaderModel : シェーダーモデル         例 "vs_5_0"
ID3DBlob* CompileShader(const std::wstring& filePath, const std::string& shaderModel) {
	ID3DBlob* shaderBlob = nullptr;
	ID3DBlob* errorBlob = nullptr;

	HRESULT hr = D3DCompileFromFile(
		filePath.c_str(), // シェーダーファイル名
		nullptr,
		D3D_COMPILE_STANDARD_FILE_INCLUDE,// インクルード可能にする
		"main", shaderModel.c_str(),// エントリーポイント名、シェーダーモデル指定
		D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION, // デバッグ用設定
		0, &shaderBlob, &errorBlob);

	// エラーが発生した場合、止める
	if (FAILED(hr)) {
		if (errorBlob) {
			OutputDebugStringA(reinterpret_cast<char*>(errorBlob->GetBufferPointer()));
			errorBlob->Release();
		}
		assert(false);
	}

	// 生成したshaderBlobを返す
	return shaderBlob;
}
