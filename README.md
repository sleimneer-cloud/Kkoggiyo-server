# 🍗 꼬끼요 (Kkokkiyo) - 배달 중개 관리 시스템 서버

## 📖 프로젝트 소개 (Project Overview)
**꼬끼요(Kkokkiyo)**는 사장님, 고객, 라이더 3가지 다중 클라이언트와 실시간으로 통신하며 배달 주문 및 관리를 처리하는 **배달 중개 플랫폼 서버**입니다. 
대규모 트래픽 처리와 안정적인 실시간 통신을 수행하기 위해 **C++ 기반의 TCP 소켓 통신 서버**와 **Python FastAPI 기반의 이미지 비동기 REST API 서버**로 아키텍처를 분리하여 설계하였습니다.

## 🛠 아키텍처 및 기술 스택 (Architecture & Tech Stack)

**Backend (TCP Server - 실시간 코어 로직)**
- **Language**: C++ (C++20)
- **Build Tool**: CMake
- **Network**: Native TCP Socket Programming
- **Architecture Pattern**: 3-Tier Layered Architecture (`Handler` → `UseCase` → `Service`)

**Backend (Image Server - 비동기 REST API)**
- **Framework**: Python 3.12, FastAPI, Uvicorn
- **Database Driver**: aiomysql (Async)
- **Data Validation**: Pydantic v2

**Database**
- MariaDB

---

## ✨ 주요 기능 (Key Features)

### 1. 멀티 클라이언트 맞춤 지원
- **사장님 (Owner)**: 가게 오픈/마감 관리, 메뉴 이미지 업로드, 실시간 주문 접수 / 채팅
- **고객 (Customer)**: 메뉴 조회, 주문 요청, 실시간 상태 조회 / 채팅
- **라이더 (Rider)**: 배달 접수 및 수행 상태 업데이트 / 채팅

### 2. 분산된 서버 역할 (Separation of Concerns)
- 성능 집약적인 실시간 통신은 **C++ 병렬 TCP 서버**가 전담하여 I/O 지연을 최소화합니다.
- 디스크 및 I/O 블로킹이 발생하기 쉬운 대용량 이미지 업로드와 다운로드는 **FastAPI 비동기 서버**로 책임을 분리하여 핵심 네트워크 스레드의 성능을 유지합니다.

### 3. 강력한 보안 및 관리자 제어 (Security & Admin Control)
- **보안 강화**: Bcrypt / Argon2 알고리즘을 사용한 패스워드 암호화 저장, SQL Injection 방어 로직 적용.
- **관리자 기능**: 관리자 전용 인증/인가 로직 처리, 실시간 유저 조회. 악성 유저 차단(Ban) 시 접속 중인 경우 즉시 안내 메세지 발송 및 소켓 강제 추방(Kick) 기능 구현.

---

## 🚀 문제 해결 및 성능 최적화 (Troubleshooting & Optimization)

### 1. 자체 구현 DB Connection Pool 기반 레이턴시 90% 개선
* **문제점**: 싱글 스레드에서 매번 DB 커넥션을 수립(TCP Handshake, DB Auth)할 때마다 27.29ms의 자원 낭비(Overhead)가 발생해 동시 접속 증가 시 심각한 병목 현상이 일어났습니다.
* **해결 방안**: 외부 라이브러리에 의존하지 않고 **C++ 기반의 Thread-safe한 커넥션 풀(Connection Pool)을 직접 구현**하여 자원을 재사용하도록 구조를 고도화하였습니다. 일시적 네트워크 장애로 인해 풀(Pool) 커넥션이 고갈(Deadlock)되는 현상을 막고자, Ping 실패 시 내부적으로 `mysql_real_connect`를 재시도하는 Auto-Reconnect 로직을 포함했습니다.
* **성능 효과**: 
  - 동시 접속 부하(50명 동시 요청, 1,000 커넥션 발생) 상황에서도 **초당 3,600 TPS**를 안정적으로 방어.
  - 평균 응답 시간을 `27.29ms` ➡️ **`2.48ms`** 로 극적으로 단축하며 **로딩 오버헤드를 90% 이상 획기적으로 낮춰 11배의 속도 향상**을 이루어냈습니다.

### 2. 치명적 장애(Crash) 방지 및 서버 운영 안정성 확보
* **소켓 `SIGPIPE` 시그널 방어**: 클라이언트가 앱을 강제 종료하여 끊어진 소켓에 서버가 데이터를 계속 보내려다 OS 정책상 프로세스 전체가 죽어버리는 패닉 에러를 발견하고, `signal(SIGPIPE, SIG_IGN)` 제어 처리를 통해 예상치 못한 서버 셧다운 문제를 방어했습니다.
* **메모리 참조 오류(Use-After-Free) 해결을 통한 Segmentation Fault 방지**: 클라이언트의 접속이 비동기적으로 종료되어 Mutex가 파괴되는 찰나에 다른 워커 스레드가 해당 쓰레기 메모리에 Lock을 시도하다 서버가 Crash되는 Race Condition을 디버깅했습니다. 이를 원시 포인터 체제에서 `std::shared_ptr`를 사용한 참조 카운트(Reference Count) 생명주기 관리 체제로 전환해 Thread-Safety를 달성했습니다.
* **OOM (Out of Memory) 공격 인지 및 무방비 개선**: 수신 버퍼 검증 누락으로 인해 무차별 용량의 패킷을 받아 서버 메모리가 폭주할 위협을 파악하고, 단일 패킷 버퍼의 Maximum Size를 제한하여 초과 크기의 비정상 패턴 연결을 즉각 차단하는 방어 로직을 적용했습니다.

### 3. FastAPI 비동기 통신 최적화와 파일 보안 강화
* `aiomysql` 비동기 DB 드라이버를 도입하여 메인 이벤트 루프의 병목을 없앴으며, 확장자와 파일 크기에 대한 서버 측 이중 검증을 추가하여 스토리지 낭비와 악성 스크립트를 방어했습니다.
* 클라이언트가 전송한 임의의 파일 경로로 인해 발생하는 Directory Traversal(경로 탐색 취약점) 문제를 식별하고, `os.path.basename`을 비롯한 검증을 통해 파일 시스템 탈취를 원천적으로 차단했습니다.
