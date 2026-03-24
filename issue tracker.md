# Kkokkiyo Server Issue Tracker & Refactoring Checklist

현재 파악된 서버의 구조적 결함 및 개선 사항을 추적하기 위한 체크리스트입니다.

## 🚨 Phase 1: 치명적 장애(Crash) 방지 (Hotfix)
*서버가 즉시 다운될 수 있는 크리티컬 버그 수정 (가장 시급함)*

- [x] **1-1. `SIGPIPE` 시그널 예외 처리 누락** (`src/server.cpp`)
  - **문제:** 클라이언트가 앱을 강제 종료했을 때, 끊어진 소켓에 서버가 데이터를 쓰려고 하면 OS가 서버 프로세스 전체를 강제 종료(`SIGPIPE`)시킴.
  - **해결 목표:** `main` 함수 시작 지점에 `signal(SIGPIPE, SIG_IGN);` 적용.
- [x] **1-2. DB 커넥션 풀 고갈 및 무한 펜딩** (`src/DBConnectionPool.cpp`)
  - **문제:** `getConnection()` 시 `ping` 실패하면 커넥션을 버리기만 하고 재생성하지 않음. 일시적 네트워크 장애 시 풀(Pool)이 0개가 되어 전체 스레드가 영원히 대기(Deadlock)함.
  - **해결 목표:** 끊어진 연결 감지 시 `mysql_real_connect`를 통해 내부에서 재연결(Reconnect)하는 로직 추가.
- [x] **1-3. `PacketHandler` 메모리 참조 오류 (Use-After-Free)** (`src/PacketHandler.cpp`)
  - **문제:** 클라이언트 접속 종료로 `removeFd`가 실행되어 Mutex가 삭제되는 찰나에, 다른 스레드가 파괴된 쓰레기 메모리에 Lock을 시도하여 Segmentation Fault 발생.
  - **해결 목표:** Mutex 관리 구조를 `std::shared_ptr`로 변경하여 안전한 참조 카운트 유지.

## 🛡️ Phase 2: 보안 강화 및 메모리 누수/폭발 방어
*악의적인 요청과 비정상적인 트래픽으로부터 서버 보호*

- [ ] **2-1. SQL Injection 취약점 제거** (`src/services/OrderService.cpp` 등)
  - **문제:** 사용자 입력값(요청사항 등)을 단순 문자열 덧셈으로 SQL 쿼리에 직접 삽입 중.
  - **해결 목표:** `DBHelper`의 Prepare Statement(파라미터 바인딩) 방식으로 쿼리 전면 교체.
- [ ] **2-2. OOM(Out of Memory) 공격 무방비** (`src/net/ConnectionManager.cpp`)
  - **문제:** 소켓 버퍼(`buf`)에 수신 데이터를 검증 없이 무한정 쌓아두어, 악의적 대용량 패킷에 메모리가 터질 위험 존재.
  - **해결 목표:** 버퍼 및 패킷의 최대 허용 크기(Max Size)를 강제하고 초과 시 연결 즉시 차단.
- [ ] **2-3. `SessionManager` 세션 충돌 버그** (`src/SessionManager.cpp`)
  - **문제:** 유저 ID(`std::string`)만으로 소켓(FD)을 매핑하여, 일반 고객과 라이더의 ID가 우연히 같을 경우 패킷 오배송 발생.
  - **해결 목표:** `Role + UserID` 조합 키 사용 또는 고유 Session Key 발급으로 식별자 고도화.

## 🚀 Phase 3: 성능 최적화 및 아키텍처 개선
*대규모 트래픽 처리를 위한 병목 제거 및 유지보수성 향상*

- [ ] **3-1. FastAPI 이미지 서버 비동기 병목 현상** (`img-server/services/image_service.py`)
  - **문제:** `async def` 라우터 안에서 일반 동기식 File I/O(`open`)를 사용하여 저장하는 동안 이벤트 루프가 멈춤(블로킹).
  - **해결 목표:** `aiofiles` 라이브러리 도입 또는 일반 `def`를 사용하여 스레드 풀에서 디스크 I/O 처리.
- [ ] **3-2. `MessageRouter` OCP(개방-폐쇄 원칙) 위반** (`src/router/MessageRouter.cpp`)
  - **문제:** 거대한 `switch-case` 문으로 모든 라우팅을 처리하여, 새로운 API 추가 시마다 기존 코드를 수정해야 함.
  - **해결 목표:** Command Pattern 및 `std::function` 맵을 활용하여 동적 라우팅 테이블(Registry) 구축.
- [ ] **3-3. Worker Thread Pool DB 병목 완화** (`src/thread/WorkQueue.cpp`)
  - **문제:** 8개의 워커 스레드가 DB 쿼리 응답을 동기적으로 기다리면서 전체 통신 성능 저하 유발.
  - **해결 목표:** 네트워크 I/O 스레드와 DB 처리 스레드 풀 분리 혹은 최적화 방안 검토.
