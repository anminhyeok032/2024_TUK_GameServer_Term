# IOCP 기반 MMORPG 게임 서버

> 대규모 동시 접속과 실시간 전투 처리를 위한 C++ IOCP MMORPG 서버
> ##  프로젝트 게임 플레이영상
[![Watch the Demo Video](https://github.com/user-attachments/assets/657a8f98-df12-4bb2-904b-fcf83112358d)](https://youtu.be/MOAtFs1bvG0)


## 프로젝트 개요

- **프로젝트명**: IOCP 대규모 처리를 통한 MMORPG 서버
- **개발 기간**: 24.04.06. ~ 24.07.18. 26.01.14. ~  26.04.14.
- **개발 인원**: 1명
- **개발 환경**:  
  - C++ / Visual Studio 2022  
  - WinSock2.h / IOCP  
  - MSSQL / ODBC  
  - Redis / cpp_redis (캐시 및 랭킹)


## 실행 방법

[![build](https://github.com/anminhyeok032/2024_TUK_GameServer_Term/actions/workflows/build.yml/badge.svg)](https://github.com/anminhyeok032/2024_TUK_GameServer_Term/actions/workflows/build.yml)

### 다운로드 (빌드 없이 실행)

| 종류 | 받는 곳 |
|---|---|
| 개발 빌드 (main 최신 커밋, push할 때마다 자동 갱신) | [GameServer.zip](https://github.com/anminhyeok032/2024_TUK_GameServer_Term/releases/download/latest/GameServer.zip) · [GameClient.zip](https://github.com/anminhyeok032/2024_TUK_GameServer_Term/releases/download/latest/GameClient.zip) |
| 정식 버전 | [Releases](https://github.com/anminhyeok032/2024_TUK_GameServer_Term/releases)에서 `v`로 시작하는 릴리스 |

- 두 프로그램 모두 [Visual C++ 재배포 패키지 (x64)](https://aka.ms/vs/17/release/vc_redist.x64.exe)가 필요합니다. (대부분의 PC에는 이미 설치되어 있습니다)

### 서버 실행

1. **Redis Server** 실행 (필수)  
   - 로컬(`127.0.0.1`)의 기본 포트(`6379`)에서 Redis 서버가 동작 중이어야 합니다.  
2. `GameServer.zip`을 푼 폴더에서 `2024_GameServer_Term.exe` 실행 (`npc.lua`와 같은 폴더에 있어야 합니다)  

SQL Server는 **선택**입니다. 시작할 때 아래 조건을 확인해서 모드를 정합니다.

| 상황 | 모드 | 진행 상황 저장 |
|---|---|---|
| ODBC DSN `2024_GameServer`가 있고, 서버가 쓰는 테이블·저장 프로시저가 모두 있음 | SQL 모드 | SQL (영구) + Redis |
| DSN이 없음 / SQL Server에 연결할 수 없음 (3회 시도) / 테이블·프로시저가 없음 | 오프라인 모드 | Redis만 (7일, 접속할 때마다 연장) |
| `2024_GameServer_Term.exe --offline`으로 실행 | 오프라인 모드 | Redis만 |

### 클라이언트 실행

1. `GameClient.zip`을 푼 폴더에서 `client.exe` 실행 (`Resources` 폴더와 같은 폴더에 있어야 합니다)
2. 실행 시 콘솔 창에서 **서버 IP 입력**
3. **사용할 ID 입력**
   - 존재하지 않으면 자동 생성 → DB에 INSERT
   - 존재할 경우 → 마지막 좌표/레벨/체력으로 이어서 접속
   - **이미 접속 중인 ID는 차단됨**

### 직접 빌드

1. Visual Studio 2022 (C++ 데스크톱 개발, v143 도구 집합)
2. [vcpkg](https://github.com/microsoft/vcpkg) 설치 후 `vcpkg integrate install` 1회 실행
3. `SERVER/2024_GameServer_Term.sln`, `CLIENT/client.sln`을 열고 `Release | x64`로 빌드
   - 서버의 cpp_redis는 `SERVER/vcpkg.json`에 따라 첫 빌드 때 자동으로 설치됩니다.
   - 클라이언트의 SFML은 NuGet 패키지로 자동 복원됩니다.



## 게임 조작법

- **이동:** `방향키 (상하좌우)`
- **기본 공격:** `S 키` (바라보는 방향으로 단일 공격, 데미지 10)
- **범위 공격:** `A 키` (주변 4방향 전체 공격, 데미지 5)
- **인벤토리 열기 / 닫기:** `I 키`
- **아이템 줍기:** `G 키`
- **인벤토리 자동 정렬:** `O 키` (인벤토리가 열려 있을 때 작동)
- **랭킹 확인:** `P 키` (마우스 휠로 상하 스크롤 가능)
- **채팅 입력 / 전송:** `Enter 키`



## 프로젝트 구조
```text
2024_TUK_GameServer_Term/
├── .github/workflows/build.yml   # 빌드·배포 (GitHub Actions → Releases)
├── SERVER/
│ ├── DB           # ODBC + MSSQL 연동 관련 DB 작업 및 쿼리 처리
│ ├── sql/         # 저장 프로시저
│ ├── vcpkg.json   # 서버 외부 라이브러리 (cpp_redis)
│ ├── MainServer   # 서버 진입점. IOCP 초기화 및 쓰레드풀, accept 처리 등
│ ├── Player       # 플레이어 상태, 입력, 충돌, 이동 및 전투 처리
│ ├── Session      # 세션 구조 관리 (세션별 플레이어 컨테이너 및 시야처리 등)
│ ├── NPC          # 몬스터 및 NPC AI, 이동, 상태
│ ├── Timer        # 서버 타이머 시스템 (AI 이동, 타임 이벤트 등 주기 처리)
│ ├── Redis        # 인메모리 데이터 저장소 접근 및 관리 (cpp_redis)
│ ├── Inventory    # 플레이어 인벤토리 및 아이템 관리 
│ └── RankingManager # 실시간 랭킹 캐싱 및 주기적 DB 동기화 (Redis ZSET)
├── CLIENT/client_sample
│ └── client.cpp   # 사용자 클라이언트. 입력 송신 및 위치 렌더링
└── README.md
```

---

## 구현 기능

### 이동

- 방향키 입력으로 이동 처리 (앞/뒤/좌/우)
- 이동 시 1초에 한 번 처리 (공격과 동일 주기)
- **시야 동기화 시스템 적용**  
  - 전체 맵 크기: 2000x2000  
  - 15칸 단위로 섹터 구분하여 주변 동기화



### 전투 시스템

- `A 키`: 4방향 전체 공격 (상하좌우, 데미지 5)
- `S 키`: 마지막 이동 방향 기준 단일 방향 공격 (데미지 10)
- 몬스터는 20만 마리 소환 가능  
  - 각 몬스터는 랜덤한 레벨과 체력을 가짐
- 몬스터 처치 시 경험치 획득 → 클라이언트 UI 실시간 반영



### 전투 이벤트 처리

- 공격 시 메시지 브로드캐스팅:  
  **"누가 누구에게 몇 데미지를 입힘"**
- 메시지는 **대상 오브젝트의 View List 내 클라이언트에게만 전송**
- 오브젝트 사망 시:
  - 죽은 객체 제거 처리
  - 처치한 플레이어는 `레벨 * 레벨 * 2` 경험치 획득



### 데이터베이스 연동 (MSSQL)

- `ODBC`를 통한 DB 연결  
- **스레드 기반 DB 작업 큐** (concurrent_queue)
- 로그인 / 로그아웃 시 DB 연동 처리
- 로그인 시 처리:
  - **존재하지 않는 ID**: 새로 INSERT
  - **기존 ID**: 마지막 좌표, 레벨 등 상태 복원
  - **이미 접속 중인 ID**: 중복 로그인 차단



### Redis 인메모리 스토리지 연동

- **리더보드 (Ranking)**
  - 주기적으로 MSSQL 데이터를 동기화하여 Redis의 `ZSET` 자료구조에 기록
  - 빠른 탐색으로 실시간 상위 100위 랭킹 제공 및 캐싱
- **인벤토리 캐싱**
  - 유저의 인벤토리 아이템 상태 및 배치를 Redis에 캐싱하여 DB 부하 감소



### 채팅 시스템

- Enter 입력 → 채팅 입력 → Enter 입력으로 서버 전송
- 모든 클라이언트에게 브로드캐스트
- 채팅 메시지는:
  - 클라이언트 하단 채팅창에 누적 표시
  - 메시지 보낸 유저 머리 위에 **3초간 출력**



### UI 구성

- 클라이언트 상단:
  - 체력 / 경험치 / 레벨
- 캐릭터 머리 위: 실시간 체력 표시
- 채팅창: 채팅 + 전투 로그 표시
- 우측 상단 미니맵
- 체력/경험치 바 실시간 갱신


