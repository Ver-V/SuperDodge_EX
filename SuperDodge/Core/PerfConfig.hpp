#pragma once

// 1: 오브젝트 풀 사용 / 0: 매번 new, 비활성화되면 다음 프레임에 delete
#define USE_OBJECT_POOL 1

// 1: 프레임마다 frame_pool.csv / frame_nopool.csv 파일에 기록 (0: off)
#define ENABLE_FRAME_LOG 1

// 0: Present가 모니터 주사율에 묶이지 않음 (프레임 시간 차이 비교용)
#define ENABLE_VSYNC 1

// 1: Debug/Release 모두 치트키 사용 (F5 보스 페이즈 즉시 시작, F6 BGM 변경)
#define ENABLE_CHEATS 1

// 1: 플레이어 무적 (피격 무시, 측정용 장시간 플레이) / 0: off
#define ENABLE_GOD_MODE 1

// 1: 텍스트 레이아웃(IDWriteTextLayout) 캐시 사용 / 0: 매번 DrawTextW (기존 방식)
#define USE_TEXT_LAYOUT_CACHE 0

struct FrameCounters
{
    int objectsCreated = 0;
    int objectsDestroyed = 0;
    int buffersCreated = 0;        // MeshRenderer constant buffer CreateBuffer count
    double bufferCreateMs = 0.0;   // time spent in those CreateBuffer calls
    int poolGetCalls = 0;          // ObjectPool::GetObject call count
    int poolScanSteps = 0;         // pool entries visited inside GetObject
    double poolGetMs = 0.0;        // time spent in GetObject
    double worldRenderMs = 0.0;    // GameManager::Draw - world + particles
    double uiMs = 0.0;             // GameManager::Draw - DrawUI (text)
    int textDrawCalls = 0;         // DrawString + DrawCenteredString count
    int textLayoutsCreated = 0;    // IDWriteTextLayout created (cache miss)
    double textEndDrawMs = 0.0;    // ID2D1RenderTarget::EndDraw (D2D flush)
};

inline FrameCounters& GetFrameCounters()
{
    static FrameCounters counters;
    return counters;
}
