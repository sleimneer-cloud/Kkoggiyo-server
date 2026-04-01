# 🗂️ 1. 현재 코드 구조 (한눈에 보기)

이 프로젝트는 **"TCP(지속 연결) + epoll(비동기) + [4바이트 길이 + JSON]"** 규약으로 동작하는 C++ 서버입니다.
파일이 여러 개여도 실제 요청 흐름은 아래 6단계로 고정입니다.

```
[흐름 요약]
1) src/server.cpp 시작
2) ConnectionManager가 accept/epoll/read 수행 + TCP 스트림을 "패킷 단위"로 프레이밍
3) 완성된 패킷을 WorkQueue에 push → Worker 스레드가 꺼내서 처리
4) MessageRouter가 JSON의 "type" 필드를 읽어 Handler 호출
5) Handler가 Service 계층을 통해 DB 조회/저장
6) PacketHandler::sendPacket()으로 응답/알림 전송 (fd별 mutex + writeAll로 스레드 안전 보장)
```

---

## 📡 통신 포맷

헤더 구조체 없이 **앞 4바이트(int32)가 뒤따라오는 JSON의 바이트 길이**를 나타냅니다.

```
[ 4바이트 (int32, little-endian) ] [ JSON body (UTF-8) ]
```

**요청 예시 (클라이언트 → 서버)**
```json
{"type": 100, "clientType": 1, "userId": "hong", "password": "1234"}
```

**응답 예시 (서버 → 클라이언트)**
```json
{"type": 101, "status": "success", "message": "로그인 환영합니다!"}
```

> ⚠️ 이전에는 `[8바이트 고정 헤더(payloadSize + PacketType) + JSON]` 방식을 사용했으나,
> 현재는 헤더 구조체를 제거하고 `type` 필드를 JSON 안에 포함하는 방식으로 변경되었습니다.
> `protocol.hpp`에서 `PacketHeader` 구조체와 `#pragma pack`은 삭제되었습니다.

---

## 📚 추천 읽기 순서 (처음 보는 사람용, 10~15분 코스)

**0) 큰 그림 (프로토콜)**
- `include/protocol.hpp` 먼저 보기. `PacketType` enum(타입 번호표)과 `ClientType` enum만 이해합니다.
  - `PacketHeader` 구조체는 삭제되었으므로 없습니다.

**1) 실행 시작점**
- `src/server.cpp`: 서버가 어떻게 시작되는지 확인합니다. (내부는 ConnectionManager로 위임)

**2) 네트워크 / 프레이밍 (가장 중요)**
- `src/net/ConnectionManager.cpp`: accept/epoll/read + fd별 버퍼 누적 + "완성된 패킷 추출" 흐름을 봅니다.
  - 앞 4바이트로 길이를 읽고, 그만큼 JSON을 읽어 WorkQueue에 push합니다.
- `include/net/ConnectionState.hpp`: fd별 수신 버퍼 구조체 (짧아서 같이 보면 좋음)

**3) 스레드 풀**
- `include/thread/WorkQueue.hpp`: Worker 스레드 8개가 큐에서 WorkItem을 꺼내 처리합니다.
  - `WorkItem`은 `{client_fd, jsonStr}` 두 필드만 가집니다.

**4) 라우팅**
- `src/router/MessageRouter.cpp`: `j.value("type", -1)`로 JSON에서 타입을 읽어 Handler로 분기합니다.
  - `authHandler_`, `chatHandler_`를 멤버로 보유합니다. (매번 임시 객체를 만들지 않음)

**5) 비즈니스 로직**
- `src/handlers/AuthHandler.cpp`: 로그인 / 회원가입 처리 (clientType에 따라 Service 위임)
- `src/handlers/ChatHandler.cpp`: 채팅 중재 (고객/사장→관리자, 관리자→targetId)

**6) 상태 / 송신 유틸**
- `src/SessionManager.cpp`: 로그인한 유저의 fd, userId, clientType을 메모리에 보관
- `src/PacketHandler.cpp`: 응답 전송 — JSON에 `"type"` 필드 추가 후 `[4바이트 길이 + JSON]` 전송.
  **fd별 `unique_ptr<mutex>`로 동시 write를 방지합니다.**
- `src/net/SocketIO.cpp`: partial write 방어 (`writeAll`)

**7) 실제 동작 확인 (테스트 클라이언트)**
- `./build/user_client` 와 `./build/admin_client`를 실행해 1:1 상담이 동작하는지 확인합니다.

---

## 🧩 파일 / 폴더 맵

```
[프로토콜]
  include/protocol.hpp
    - PacketType enum (타입 번호표), ClientType enum
    - ※ PacketHeader 구조체 및 #pragma pack 삭제됨

[엔트리포인트]
  src/server.cpp

[네트워크 계층: accept/epoll + 프레이밍]
  include/net/ConnectionManager.hpp
  src/net/ConnectionManager.cpp
  include/net/ConnectionState.hpp     ← fd별 수신 버퍼

[스레드 풀]
  include/thread/WorkQueue.hpp        ← WorkItem = {client_fd, jsonStr}
  src/thread/WorkQueue.cpp

[라우터]
  include/router/MessageRouter.hpp    ← authHandler_, chatHandler_ 멤버 보유
  src/router/MessageRouter.cpp

[핸들러 (비즈니스 로직)]
  include/handlers/AuthHandler.hpp
  src/handlers/AuthHandler.cpp
  include/handlers/ChatHandler.hpp
  src/handlers/ChatHandler.cpp

[서비스 계층 (DB 연동)]
  include/services/CustomerAuthService.hpp
  src/services/CustomerAuthService.cpp
  include/services/BossAuthService.hpp
  src/services/BossAuthService.cpp

[세션 (온라인 유저 / FD 맵)]
  include/SessionManager.hpp
  src/SessionManager.cpp

[송신 유틸]
  include/PacketHandler.hpp           ← fd별 unique_ptr<mutex> 보유
  src/PacketHandler.cpp
  include/net/SocketIO.hpp
  src/net/SocketIO.cpp

[DB 커넥션 풀]
  include/DBConnectionPool.h
  src/DBConnectionPool.cpp            ← 서버 시작 시 10개 커넥션 미리 생성

[DB 스텁 (현재 직접 사용하지 않음)]
  include/repo/IUserRepository.hpp
  include/repo/InMemoryUserRepository.hpp
  src/repo/InMemoryUserRepository.cpp

[C++ 클라이언트]
  clients/admin_client.cpp            ← 관리자
  clients/user_client.cpp             ← 고객
  clients/boss_client.cpp             ← 사장님
  clients/NetClient.*                 ← 공용 TCP 클라이언트
  clients/HttpClient.*                ← 이미지 업로드/다운로드 (FastAPI 연동)
```

---

# 🟢 2. 현재 구현된 기능

지금 서버를 켜면 딱 여기까지 완벽하게 동작합니다.

1. **비동기 다중 접속 처리**: 클라이언트가 중간에 끊기거나 강제 종료해도 서버가 죽지 않고 해당 소켓만 정리합니다.
2. **TCP 스트림 프레이밍**: 데이터가 쪼개지거나 뭉쳐 들어와도 `[4바이트 길이 + JSON]` 단위로 정확하게 읽어냅니다.
3. **Worker Thread Pool**: 8개 Worker 스레드가 WorkQueue에서 패킷을 꺼내 처리합니다. 메인 스레드는 I/O만 담당합니다.
4. **상태 유지 (Stateful)**: 로그인한 유저의 권한(관리자/고객/사장님)을 SessionManager가 기억합니다.
5. **관리자 중재 채팅**: 고객/사장님이 채팅을 보내면 서버가 관리자 FD로 정확하게 전달합니다. 관리자가 답장할 때는 `targetId`로 지정한 유저에게 직접 전달합니다.
6. **스레드 안전 송신**: `PacketHandler`가 fd별 mutex를 보유하여 여러 Worker가 동시에 같은 클라이언트에게 write하는 것을 방지합니다.
7. **고객/사장님 인증**: DB(MySQL)에서 비밀번호를 조회해 로그인을 처리합니다. 고객 회원가입도 DB에 INSERT합니다.

---

# 🚀 3. 코드 실행 Flow (A to Z 상세 시나리오)

## [1단계: 서버 부팅 및 대기]

1. `src/server.cpp` 실행
2. `DBConnectionPool` 초기화 — MySQL 커넥션 10개 미리 생성
3. `ConnectionManager` 생성 — 포트 9000 바인딩, epoll 시작, Worker 8개 가동
4. `epoll_wait()`로 이벤트 대기

## [2단계: 관리자 로그인]

5. 관리자 클라이언트 접속 → epoll이 감지하여 새 소켓(예: FD 5) 발급
6. 관리자가 아래 데이터를 전송합니다.
   ```
   [4바이트: JSON 길이] + {"type":100, "clientType":0, "userId":"admin", "password":"1234"}
   ```
7. `ConnectionManager`가 4바이트 길이 읽기 → JSON 수신 → `WorkQueue.push({5, jsonStr})`
8. Worker 스레드가 꺼내서 `MessageRouter.route()` 호출
9. `type=100` → `authHandler_.handleLogin()` 호출
10. `clientType=0`(관리자) → 별도 DB 검증 없이 성공 처리
11. `SessionManager`에 "FD 5 = 관리자(0), userId=admin" 등록
12. `PacketHandler::sendPacket(5, SC_LOGIN_RES, {"status":"success", ...})` 응답

## [3단계: 고객 로그인]

13. 고객 클라이언트 접속 → FD 6 발급
14. 고객이 로그인 패킷 전송
    ```
    [4바이트 길이] + {"type":100, "clientType":1, "userId":"hong", "password":"1234"}
    ```
15. `CustomerAuthService`가 DB에서 비밀번호 조회 → 일치하면 성공
16. `SessionManager`에 "FD 6 = 고객(1), userId=hong" 등록 후 응답

## [4단계: 고객의 채팅 전송 ✨]

17. 고객(FD 6)이 "단무지 많이 주세요" 전송
    ```
    [4바이트 길이] + {"type":200, "clientType":1, "senderId":"hong", "message":"단무지 많이 주세요"}
    ```
18. `WorkQueue` → `MessageRouter` → `type=200` → `chatHandler_.handleChat()` 호출
19. `clientType=1`(고객) → `SessionManager.getAdminFds()`로 관리자 FD 목록 조회
20. `PacketHandler::sendPacket(5, SC_CHAT_NOTI, {...})` — 관리자에게 전달
    - fd별 mutex로 잠금 후 `[4바이트 길이] + {"type":201, "senderId":"hong", "message":"단무지 많이 주세요"}` 전송

## [5단계: 관리자 답장]

21. 관리자가 답장 전송
    ```
    [4바이트 길이] + {"type":200, "clientType":0, "senderId":"admin", "targetId":"hong", "message":"네 알겠습니다"}
    ```
22. `chatHandler_.handleChat()` → `clientType=0`(관리자) → `getFdByUserId("hong")`으로 FD 6 조회
23. `PacketHandler::sendPacket(6, SC_CHAT_NOTI, {...})` — 고객에게 전달

## [6단계: 접속 종료]

24. 고객이 앱을 끔 → `read()` 결과 0 → 종료 감지
25. `SessionManager.removeSession(6)` → 명부에서 삭제, 소켓 close

---

# 📋 4. 패킷 타입 / JSON 필드 명세

## 패킷 타입 번호표 (`include/protocol.hpp`)

| 상수명 | 번호 | 방향 | 설명 |
|---|---|---|---|
| CS_LOGIN_REQ | 100 | C→S | 로그인 요청 |
| SC_LOGIN_RES | 101 | S→C | 로그인 응답 |
| CS_REGISTER_REQ | 102 | C→S | 회원가입 요청 |
| SC_REGISTER_RES | 103 | S→C | 회원가입 응답 |
| CS_CHAT_REQ | 200 | C→S | 채팅 전송 |
| SC_CHAT_NOTI | 201 | S→C | 채팅 수신 알림 |
| CS_ORDER_REQ | 300 | C→S | 주문 요청 (미구현) |
| SC_ORDER_NOTI | 301 | S→C | 주문 알림 (미구현) |
| CS_ORDER_ACCEPT | 302 | C→S | 주문 수락 (미구현) |
| SC_ORDER_STATUS | 303 | S→C | 주문 상태 변경 (미구현) |

## clientType 값

| 값 | 의미 |
|---|---|
| 0 | 관리자 (ADMIN) |
| 1 | 고객 (CUSTOMER) |
| 2 | 사장님 (OWNER) |
| 3 | 라이더 (RIDER) |

## 주요 JSON 필드

**로그인 (type: 100)**
```json
{"type":100, "clientType":1, "userId":"hong", "password":"1234"}
```

**회원가입 (type: 102) — 고객 전용**
```json
{"type":102, "clientType":1, "userId":"hong", "password":"1234",
 "userName":"홍길동", "phone":"010-1234-5678", "address":"서울시"}
```

**채팅 전송 (type: 200)**
```json
{"type":200, "clientType":1, "senderId":"hong", "message":"내용"}
{"type":200, "clientType":0, "senderId":"admin", "targetId":"hong", "message":"내용"}
```
> 관리자가 보낼 때는 `targetId` 필드 필수

**서버 응답 공통 형식**
```json
{"type":101, "status":"success", "message":"로그인 환영합니다!"}
{"type":101, "status":"fail",    "message":"비밀번호가 틀렸습니다."}
```

---

# ✅ 5. 구현 완료 목록

- [x] TCP 비동기 다중 접속 (epoll)
- [x] 4바이트 길이 + JSON 통신 방식 (구 8바이트 헤더 방식에서 변경)
- [x] Worker Thread Pool (WorkQueue, 8개 스레드)
- [x] MessageRouter — 핸들러를 멤버로 보유
- [x] 고객 로그인 / 회원가입 (DB 연동)
- [x] 사장님 로그인 (DB 연동)
- [x] 관리자 채팅 중재 (고객/사장 → 관리자 → 고객/사장)
- [x] SessionManager — 멀티스레드 안전
- [x] PacketHandler — fd별 mutex로 스레드 안전 송신
- [x] DBConnectionPool — 커넥션 10개 미리 생성, 대여/반납

---

# 🚧 6. 미구현 / TODO 항목

## 🔴 보안 (우선순위 높음)

- [x] **Prepared Statement 적용** — `CustomerAuthService`, `BossAuthService`의 SQL이 현재 문자열 직접 조합 방식이라 SQL Injection에 취약합니다.
  ```cpp
  // 현재 (취약)
  "SELECT password FROM members WHERE user_id = '" + userId + "'"
  // 변경 필요 → mysql_stmt_prepare() + MYSQL_BIND 사용
  ```

- [ ] **비밀번호 해싱** — 현재 DB에 평문 비밀번호를 저장/비교합니다. bcrypt 또는 SHA-256 + salt 적용이 필요합니다.

## 🟡 핵심 기능 미구현

- [ ] **주문 기능** — `protocol.hpp`에 타입 번호(300~303)는 정의되어 있으나 Handler/Service가 없습니다.
  - `OrderHandler`, `OrderService`, `RiderService` 생성 필요
  - `MessageRouter`의 switch문에 `case PacketType::CS_ORDER_REQ:` 추가 필요

- [ ] **사장님 회원가입** — `BossAuthService::processRegister()`가 `return false`만 하는 빈 함수입니다.

- [ ] **라이더 로그인/인증** — `AuthHandler`에 `ClientType::RIDER` 케이스가 주석 처리되어 있습니다.

- [ ] **관리자 로그인 검증** — 현재 관리자는 비밀번호 검증 없이 무조건 로그인 성공 처리됩니다.
  ```cpp
  case ClientType::ADMIN:
      loginSuccess = true; // ← DB 검증 없음
  ```

## 🟢 개선 사항

- [x] **PacketHandler fd 정리** — 클라이언트 연결이 끊길 때 `fdMutexMap_`에서 해당 fd의 mutex를 제거하는 로직이 없어 메모리가 계속 누적됩니다. `ConnectionManager::disconnectClient()`에서 `PacketHandler::removeFd(fd)` 호출이 필요합니다.

- [x] **WorkQueue PacketType 로그** — 현재 WorkQueue 로그에 `PacketType: unknown`으로 표시됩니다. `item.jsonStr`을 파싱해서 type 값을 출력하면 디버깅이 편해집니다.


- [ ] **설정 파일 분리** — DB 접속 정보(IP, 비밀번호)가 `server.cpp`에 하드코딩되어 있습니다. `.env` 파일이나 config 구조체로 분리하는 것을 권장합니다.
  ```cpp
  // 현재 (하드코딩)
  DBConnectionPool::getInstance().init("10.10.10.116", "HS", "1234", "delivery", 3306, 10);
  ```
