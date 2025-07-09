#pragma once
#include "kamataEngine.h"

class WorldTransformEx : public KamataEngine::WorldTransform {
public:
	// アフィン変換行列の生成と定数バッファへの転送を行う
	void UpdateMatrix();

	// アフィン変換行列の生成
	KamataEngine::Matrix4x4 MakeAffineMatrix();
};
