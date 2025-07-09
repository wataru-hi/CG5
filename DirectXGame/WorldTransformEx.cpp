#include "WorldTransformEx.h"
using namespace KamataEngine;
using namespace KamataEngine::MathUtility; // Make~ Matrix Matrix4x4同士の積(*)の利用

// Scale, Rotation, Translate 行列から World行列を計算、
// そして定数バッファへの転送も行う
void WorldTransformEx::UpdateMatrix() {
	// World変換行列を計算し、matWorld_ に格納する
	matWorld_ = MakeAffineMatrix();
	// 定数バッファへ転送する
	TransferMatrix();
}