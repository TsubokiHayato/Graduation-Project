#include"engine/Framework/Framework.h"
#include"application/Order.h"
#include"D3DResourceLeakChecker.h"
#include <memory>

// エントリーポイント（共有エンジン EngineCore を使うゲーム）
int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int) {
#ifdef _DEBUG
	TuboEngine::D3DResourceLeakChecker leakChecker;
#endif
	std::unique_ptr<TuboEngine::Framework> framework = std::make_unique<TuboEngine::Order>();
	framework->Run();
	return 0;
}
