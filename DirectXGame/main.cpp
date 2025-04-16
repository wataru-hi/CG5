#include <Windows.h>

#include "KamataEngine.h"

using namespace KamataEngine;

// Windowsアプリでのエントリーポイント(main関数)
int WINAPI WinMain(_In_ HINSTANCE, _In_opt_ HINSTANCE, _In_ LPSTR, _In_ int) {
	
	KamataEngine::Initialize(L"LE3D_12_ヒガ_ワタル");

	DirectXCommon* dxCommon = DirectXCommon::GetInstance();

	while (true)
	{

		if (KamataEngine::Update())
		{
			break;
		}

		dxCommon->PreDraw();



		dxCommon->PostDraw();

	}

	KamataEngine::Finalize();

	return 0;
}
