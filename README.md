## Project Highlights

✔ Direct3D 11 Rendering Pipeline

✔ Component-based Architecture

✔ Object Pool (150 + 540 Objects)

✔ 10-Phase Boss Pattern System

✔ Graze Collision

✔ High Score Serialization


**1. [SuperDodgeEX] - 2D 액션 슈팅 완성작 프로젝트**
C++/Win32 + Direct3D 11 기반으로 직접 구현한 컴포넌트 기반 2D 탄막 슈팅 게임입니다.
렌더링 파이프라인부터 게임 루프, 충돌 판정, 오브젝트 풀링, 보스 패턴 시스템까지 직접 구현했습니다.

**2. 플레이 링크**
[Play SuperDodgeEX on itch.io](https://versum.itch.io/superdodgeex)

**3. 기술 스택 및 담당 역할**
* Engine & Language : Visual Studio / C++ / Direct3D 11 / Direct2D / DirectWrite
* Design Pattern / Architecture: Component Based Design
* Version Control : Git / Github desktop

Role
- EX 버전 단독 개발
- 게임 시스템 설계
- Direct3D 11 렌더링 파이프라인
- 컴포넌트 기반 아키텍처
- 보스 패턴 시스템
- UI
- 랭킹 저장 시스템
- 메인 기획

※ 초기 프로젝트는 2인으로 시작했으며
Window 초기화와 일부 장애물 컴포넌트를 공동 작업했습니다.

**4. 클래스 및 컴포넌트 아키텍처 다이어그램**
<img width="2714" height="900" alt="Image" src="https://github.com/user-attachments/assets/6f739c89-09df-428a-982d-dec66ced8e09" />
  <p>프로젝트 아키텍처 구조도</p>
</div>
<img width="1885" height="120" alt="Image" src="https://github.com/user-attachments/assets/62c68b1a-c464-4401-ad3e-bc9807ed3769" />
<p> Game Flow 구조도 </p>
</div>

**5. 핵심 기술 구현 사항**

 - **Direct3D 11 렌더링 파이프라인 직접 구성**
    디디바이스, 스왑체인, 렌더 타겟, Vertex Buffer, Constant Buffer, Input Layout, 블렌딩, 뷰포트를 직접 초기화하고 HLSL 셰이더를 컴파일해 메쉬를 렌더링합니다. 텍스트 UI는 Direct2D/DirectWrite로 처리했습니다.

 -  **컴포넌트 기반 게임 오브젝트 구조**
    GameObject가 여러 Component를 소유하고, GameWorld가 전체 GameObject의 Start / Update / Render를 일괄 관리합니다.
    
 - **오브젝트 풀링 기반 탄막/장애물 관리**
    빈번한 동적 메모리 할당과 해제를 줄여 프레임 드롭을 최소화하기 위해 Object Pool을 적용했습니다. 일반 장애물 150개, 보스 투사체 540개 규모의 풀을 운영합니다.

 - **정밀 피격 판정과 그레이즈 시스템**
    플레이어는 작은 실제 피격 반경을 갖고, 외곽 반경으로 그레이즈를 판정합니다. 보스 탄막은 그레이즈 성공 시 보스 페이즈 클리어 조건에 반영합니다.

 - **10단계 보스 페이즈와 탄막 패턴 시스템**
    생존 시간에 따라 보스가 등장하고, 페이즈별로 원형 탄막, 조준 부채꼴 탄막, 유도탄, 십자 버스트 패턴을 조합합니다. 그레이즈 횟수로 페이즈를 넘기는 구조입니다.

 - **점수, 랭킹, 하이스코어 저장**
    생존 시간, 그레이즈 보너스, 클리어 보너스를 합산하고 Top 10 랭킹을 highscore.dat에 저장합니다. 구버전 단일 점수 파일도 읽을 수 있게 처리합니다.


6. **트러블 슈팅 히스토리**

- **보스 페이즈 종료 후 투사체 잔존 문제** 
 
보스 페이즈가 종료되어 보스가 사라진 이후에도 이전에 발사된 투사체가 GameWorld에 활성 상태로 남아 플레이어와 계속 충돌하는 문제가 발생했습니다.
보스와 투사체가 서로 독립적인 GameObject로 관리되고 있었기 때문에, 보스의 페이즈 종료만으로 이미 활성화된 투사체의 상태가 변경되지 않았습니다. 투사체는 화면 이탈이나 충돌 등의 반환 조건을 만족할 때까지 계속 Update/Render 대상에 남았습니다.

보스 페이즈 종료 시 보스 투사체 풀의 활성 객체를 일괄적으로 SetActive(false) 처리하도록 수정했습니다. 비활성화된 객체는 GameWorld의 Update/Render 대상에서 제외되며, 이후 GetObject()를 통해 다시 대여될 수 있도록 했습니다.
오브젝트 풀에서는 객체를 삭제하는 대신 상태를 통해 생명주기를 관리하므로, 객체를 생성한 시스템의 상태 전환과 해당 객체들의 반환 시점을 함께 설계해야 한다는 점을 배웠습니다.


- **Constant Buffer 크기 불일치로 인한 런타임 오류**
  
원인을 파악하기 어려운 런타임 오류가 발생하여 디버깅에 많은 시간을 소요했습니다.
AI의 도움으로 Constant Buffer의 C++ 구조체와 HLSL Constant Buffer의 크기가 일치하지 않는 것이 원인임을 확인했고, 이후 메모리 레이아웃과 버퍼 생성 과정을 다시 학습하여 문제를 해결했습니다.
이 경험을 통해 DirectX에서 CPU와 GPU 간 데이터 레이아웃 일치의 중요성을 이해하게 되었습니다.

- **Shader 초기화 실패**
  
Shader 초기화 과정에서 발생한 단순 오타로 인해 셰이더가 정상적으로 생성되지 않는 문제가 있었습니다.
그래픽스 경험이 있는 선배와 함께 초기화 과정을 단계적으로 검토하며 원인을 찾아 수정했습니다.
이후에는 Shader 생성 및 초기화 코드를 체크리스트 형태로 정리하여 동일한 실수를 방지하고 있습니다.

## 프로젝트를 통해 배운 것들

- Direct3D 11 렌더링 파이프라인의 전체 흐름
- Constant Buffer와 CPU/GPU 데이터 레이아웃 관리
- 컴포넌트 기반 게임 오브젝트 설계
- Object Pool을 활용한 메모리 관리
- HLSL Shader 작성 및 초기화 과정
- 런타임 디버깅과 원인 분석 방법
