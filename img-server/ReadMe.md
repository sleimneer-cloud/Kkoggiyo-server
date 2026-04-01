

```markdown
# [cite_start]🍗 꼬끼요 이미지 서버 [cite: 1]

[cite_start]Menu Image Upload / Retrieval API [cite: 2]
[cite_start]FastAPI · aiomysql · Python 3.12 [cite: 3]

---

## [cite_start]1. 프로젝트 개요 [cite: 4]
[cite_start]꼬끼요 이미지 서버는 배달앱 사장님 전용 앱과 연동하여, 메뉴 이미지를 업로드·조회·교체하는 기능을 제공하는 비동기 REST API 서버입니다. [cite: 5]
[cite_start]FastAPI와 aiomysql을 기반으로 하며, MySQL delivery 데이터베이스와 연동됩니다. [cite: 6]

* [cite_start]**웹 프레임워크**: FastAPI (자동 OpenAPI(Swagger) 문서 생성) [cite: 7]
* [cite_start]**DB 드라이버**: aiomysql (비동기 MySQL 커넥션 풀) [cite: 7]
* [cite_start]**스키마 검증**: Pydantic v2 (요청/응답 자동 직렬화) [cite: 7]
* [cite_start]**Python 버전**: Python 3.12 (asyncio 완전 지원) [cite: 7]
* [cite_start]**파일 저장**: 로컬 `uploads/` 디렉토리 (UUID hex 기반 고유 파일명) [cite: 7]

---

## [cite_start]2. 프로젝트 구조 [cite: 8]
```text
[cite_start]yogiyo_img_server/ [cite: 9]
[cite_start]├── main.py                  # FastAPI 앱 진입점, CORS, lifespan [cite: 10]
[cite_start]├── db.py                    # aiomysql 커넥션 풀 관리 [cite: 11]
[cite_start]├── schemas.py               # Pydantic 응답/요청 모델 [cite: 12]
[cite_start]├── routers/ [cite: 13]
[cite_start]│   └── menu_image.py        # POST /menu/image, GET /menu/image/{id} [cite: 14]
[cite_start]├── services/ [cite: 15]
[cite_start]│   └── image_service.py     # 파일 저장·삭제, DB 업데이트, 권한 검증 [cite: 16]
[cite_start]└── uploads/                 # 업로드된 이미지 파일 저장 디렉토리 [cite: 17]
```

---

## [cite_start]3. 파일별 상세 설명 [cite: 18]

### 3-1. [cite_start]`main.py` — FastAPI 앱 진입점 [cite: 19]
* [cite_start]**역할**: 서버 초기화, 미들웨어 등록, 라우터 연결 [cite: 20]
* **주요 동작**:
  * [cite_start]앱 시작 시 `lifespan` 컨텍스트 매니저를 통해 `create_pool()`을 호출하고 DB 커넥션 풀을 초기화합니다. [cite: 22]
  * `CORSMiddleware`를 등록하여 모든 오리진(`allow_origins=["*"]`), 모든 메서드, 모든 헤더를 허용합니다. [cite_start]프로덕션 배포 전 오리진 제한 필요. [cite: 23]
  * [cite_start]`menu_image.router`를 앱에 포함하여 이미지 관련 엔드포인트를 활성화합니다. [cite: 24]
  * [cite_start]FastAPI의 Swagger UI(`/docs`)와 ReDoc(`/redoc`)을 통해 자동 API 문서가 생성됩니다. [cite: 25]

### 3-2. [cite_start]`db.py` — 데이터베이스 연결 관리 [cite: 32]
* [cite_start]**역할**: aiomysql 비동기 커넥션 풀 생성 및 의존성 주입 제공 [cite: 33]
* **주요 함수**:
  * `create_pool()`: 서버 시작 시 1회 호출. [cite_start]전역 pool 변수에 커넥션 풀 저장. [cite: 34]
  * `get_db()`: FastAPI Depends로 사용되는 DB 커서 제공자. [cite_start]요청마다 커넥션을 획득하고 커서를 yield. [cite: 34]
* **접속 정보 (현재 설정)**:
  * [cite_start]`host`: 10.10.10.116 [cite: 36]
  * [cite_start]`port`: 3306 [cite: 37]
  * [cite_start]`user`: HS [cite: 38]
  * [cite_start]`database`: delivery [cite: 39]
  * [cite_start]`autocommit`: True (각 쿼리가 즉시 커밋됨) [cite: 40]
  > ⚠ **주의**: 접속 정보와 비밀번호가 소스에 하드코딩되어 있습니다. [cite_start]환경변수(`.env`) 또는 시크릿 매니저로 분리할 것을 권장합니다. [cite: 41]

### 3-3. [cite_start]`schemas.py` — Pydantic 응답 모델 [cite: 42]
* [cite_start]**역할**: API 응답 데이터의 타입 정의 및 자동 직렬화/검증 [cite: 43]
* **모델 상세**:
  * [cite_start]`ImageUploadResponse`: `success`, `menu_id`, `img_path` (이미지 업로드 성공 시 반환. `img_path`는 `/images/{uuid}.ext` 형식) [cite: 44]
  * [cite_start]`ErrorResponse`: `success=False`, `detail` (400/403/404 오류 응답 공통 포맷) [cite: 44]
  * [cite_start]`MenuImageInfo`: `menu_id`, `store_id`, `menu_name`, `img_path?` (메뉴 이미지 정보 조회용 모델. `img_path`는 Optional) [cite: 44]

### 3-4. [cite_start]`routers/menu_image.py` — API 라우터 [cite: 45]
* [cite_start]**역할**: HTTP 엔드포인트 정의, 요청 유효성 검사, 오류 처리 [cite: 46]
* [cite_start]**업로드 제약 조건**[cite: 47]:
  * [cite_start]**허용 확장자**: `.jpg`, `.jpeg`, `.png`, `.webp` [cite: 48]
  * [cite_start]**최대 파일 크기**: 5 MB (5 × 1024 × 1024 bytes) [cite: 49]
  * [cite_start]**전송 방식**: `multipart/form-data` [cite: 50]

#### 엔드포인트 세부 정보
* [cite_start]**`POST /menu/image`** (업로드 처리 5단계)[cite: 51, 52]:
  1. [cite_start]**확장자 검사** (400): 허용 확장자 외 파일 거부 [cite: 53]
  2. [cite_start]**파일 크기 검사** (400): 5 MB 초과 시 거부 [cite: 53]
  3. [cite_start]**사장님 권한 검증** (403): Store JOIN members (role=2) 쿼리로 소유권 확인 [cite: 53]
  4. [cite_start]**파일 저장**: `uploads/` 에 UUID hex + 확장자로 저장 [cite: 53]
  5. [cite_start]**DB 업데이트 & 파일 교체** (404): `Menu.img_path` 갱신, 기존 파일 삭제 [cite: 53]
* [cite_start]**`GET /menu/image/{menu_id}`** (이미지 조회)[cite: 51, 54]:
  * [cite_start]DB에서 `img_path`를 조회한 후 `FileResponse`로 바이너리 스트림을 직접 반환합니다. [cite: 55]
  * [cite_start]이미지가 DB에 없거나 파일이 실제로 존재하지 않을 경우 404를 반환합니다. [cite: 56]
  * [cite_start]Content-Type은 파일 확장자에 따라 `image/jpeg` 또는 `image/png` 등으로 자동 설정됩니다. [cite: 57]

### 3-5. [cite_start]`services/image_service.py` — 비즈니스 로직 [cite: 58]
* [cite_start]**역할**: 파일 I/O, DB 조회·갱신, 사장님 소유권 검증 로직 캡슐화 [cite: 59]
* **주요 함수**:
  * `save_image(file, content)`: UUID hex + 확장자로 파일명 생성 후 `uploads/` 에 저장. [cite_start]`/images/{name}` 경로 문자열 반환. [cite: 60]
  * `delete_old_image(img_path)`: 기존 `img_path`에 해당하는 파일이 존재하면 삭제. [cite_start]None이거나 파일 미존재 시 무시. [cite: 60]
  * `update_menu_image(cur, menu_id, store_id, img_path)`: Menu 테이블에서 menu_id + store_id 확인 후 img_path 업데이트. 기존 파일 삭제 포함. [cite_start]메뉴 미존재 시 False. [cite: 60]
  * [cite_start]`verify_store_owner(cur, user_id, store_id)`: Store JOIN members WHERE role=2 AND user_id 쿼리로 사장님 소유권 확인. [cite: 60]

---

## [cite_start]4. 연동 데이터베이스 스키마 [cite: 68]
[cite_start]서버가 실제로 참조하는 컬럼만 정리한 요약본입니다. [cite: 69]

* [cite_start]**Menu 테이블**[cite: 70]:
  * [cite_start]`menu_id`: 메뉴 고유 ID (PK) [cite: 70]
  * [cite_start]`store_id`: 해당 메뉴가 속한 가게 ID (FK) [cite: 70]
  * [cite_start]`img_path`: 이미지 경로 문자열 (`/images/{uuid}.ext`). nullable. [cite: 70]
* [cite_start]**Store 테이블**[cite: 70]:
  * [cite_start]`store_id`: 가게 고유 ID (PK) [cite: 70]
  * [cite_start]`owner_id`: 사장님 `user_id` (`members.user_id` FK) [cite: 70]
* [cite_start]**members 테이블**[cite: 70]:
  * [cite_start]`user_id`: 회원 고유 ID [cite: 70]
  * `role`: 권한 코드. [cite_start]2 = 사장님 (점주) [cite: 70]

---

## [cite_start]5. 권한 모델 [cite: 71]
[cite_start]현재 구현된 권한 체계는 다음과 같습니다. [cite: 72]

* **`POST /menu/image`**: 사장님 (`role=2`) 권한 필요. `user_id` + `store_id`로 DB JOIN 검증. [cite_start]토큰 없음 — form 파라미터로 직접 수신. [cite: 73]
* **`GET /menu/image/{id}`**: 권한 필요 없음 (공개). [cite_start]별도 인증 없이 누구나 이미지 조회 가능. [cite: 73]

> [cite_start]⚠ **보안 개선 권고**: 현재 `user_id`를 폼 파라미터로 직접 수신하는 방식은 위조가 가능합니다. [cite: 74] [cite_start]JWT Bearer 토큰 또는 세션 쿠키 기반 인증으로 교체할 것을 강력히 권장합니다. [cite: 75]

---

## [cite_start]6. 파일 저장 규칙 [cite: 76]
* [cite_start]**저장 위치**: `uploads/` 디렉토리 (서버 루트 기준 상대 경로) [cite: 77]
* [cite_start]**파일명 규칙**: `{uuid4().hex}{확장자}` (예: `64e38afc9c864371b009c48f39ec9f9b.jpeg`) [cite: 78]
* [cite_start]**URL 경로**: `/images/{파일명}` (DB에 저장되는 `img_path` 형식) [cite: 79]
* [cite_start]**이미지 교체 정책**: 새 이미지 저장 → DB 업데이트 → 기존 파일 삭제 순서로 처리하여 고아 파일 최소화 [cite: 80]
* [cite_start]**지원 포맷**: `.jpg` / `.jpeg` / `.png` / `.webp` (대소문자 구분 없음, 소문자 변환 후 비교) [cite: 81]

---

## [cite_start]7. 서버 실행 방법 [cite: 82]

[cite_start]**의존성 설치** [cite: 83]
```bash
pip install fastapi uvicorn aiomysql python-multipart
```
[cite_start]*[cite: 84]*

[cite_start]**개발 서버 실행** [cite: 85]
```bash
cd yogiyo_img_server
uvicorn main:app --reload --host 0.0.0.0 --port 8000
```
[cite_start]*[cite: 86, 87]*

[cite_start]**API 문서 확인** [cite: 88]
* [cite_start]`http://localhost:8000/docs` — Swagger UI (인터랙티브 테스트 가능) [cite: 89]
* [cite_start]`http://localhost:8000/redoc` — ReDoc (읽기 전용 문서) [cite: 90]

---

## [cite_start]8. API 사용 예시 [cite: 91]

### [cite_start]이미지 업로드 (curl) [cite: 92]
```bash
curl -X POST http://localhost:8000/menu/image \
  -F "menu_id=42" \
  -F "store_id=7" \
  -F "user_id=owner01" \
  -F "file=@menu_photo.jpg"
```
[cite_start]*[cite: 93, 94, 95, 96, 97]*

[cite_start]**성공 응답 예시** [cite: 98]
```json
{
  "success": true,
  "menu_id": 42,
  "img_path": "/images/64e38afc9c864371b009c48f39ec9f9b.jpeg"
}
```
[cite_start]*[cite: 99, 100, 101, 102, 103]*

### [cite_start]이미지 조회 [cite: 104]
```http
GET http://localhost:8000/menu/image/42
# → 이미지 바이너리 스트림 (Content-Type: image/jpeg)
```
[cite_start]*[cite: 105, 106]*

### [cite_start]오류 응답 예시 [cite: 107]
```json
// 확장자 오류 (400)
{ "success": false, "detail": "jpg, jpeg, png, webp 형식만 허용됩니다" }

// 권한 없음 (403)
{ "success": false, "detail": "해당 가게에 대한 권한이 없습니다" }

// 메뉴 없음 (404)
{ "success": false, "detail": "해당 메뉴가 존재하지 않습니다" }
```
[cite_start]*[cite: 108, 109, 110, 111, 112, 113]*

---

## 9. 버그 수정 내역 (완료)

### [수정됨] 고아 파일 문제 — `routers/menu_image.py`
기존 코드는 이미지를 디스크에 저장한 뒤 DB에서 menu_id를 검증했습니다.
menu_id가 존재하지 않아 404가 반환될 경우, 저장된 파일이 uploads/에 영구적으로 남는 버그가 있었습니다.

수정 후 처리 순서:
1. 확장자 검사 → 2. 파일 크기 검사 → 3. 사장님 권한 검증
→ 4. DB에서 menu_id 존재 확인 (파일 저장 전) ← 핵심 변경
→ 5. 이미지 파일 저장
→ 6. DB img_path 업데이트 + 기존 파일 삭제

### [수정됨] 경로 탐색 취약점 — `routers/menu_image.py`, `services/image_service.py`
GET /menu/image/{menu_id} 엔드포인트에서 DB의 img_path 값을 파일 경로로 직접 사용했습니다.
DB에 ../../etc/passwd 같은 값이 들어있을 경우 uploads/ 디렉토리 밖의 파일을 읽을 수 있었습니다.
img_path.split("/")[-1] → os.path.basename(img_path) 로 변경하여 경로 탐색을 차단했습니다.

### [수정됨] uploads/ 디렉토리 부재 시 크래시 — `main.py`
uploads/ 폴더가 없는 환경(최초 배포, 클린 clone 등)에서 서버 시작 후 첫 업로드 요청 시
FileNotFoundError로 크래시되는 문제가 있었습니다.
lifespan에 os.makedirs("uploads", exist_ok=True)를 추가하여 서버 시작 시 자동으로 생성합니다.

---

## 10. 잔여 개선 권고사항 (미완료)

### [높음] 인증 방식 개선
현재 user_id를 multipart form 파라미터로 직접 수신합니다.
클라이언트가 다른 사용자의 user_id를 위조하여 요청을 보낼 수 있습니다.
DB JOIN으로 소유권은 검증하지만, user_id 자체의 신뢰성이 없습니다.
→ TCP 서버의 세션 테이블을 공유하거나, API Key 방식으로 교체하는 것을 권장합니다.

### [높음] 설정 분리
DB 접속 정보(host, user, password)가 db.py에 하드코딩되어 있습니다.
→ python-dotenv + .env 파일 또는 환경변수로 분리해야 합니다.
  pip install python-dotenv
  .env 예시:
    DB_HOST=10.10.10.116
    DB_PORT=3306
    DB_USER=HS
    DB_PASSWORD=1234
    DB_NAME=delivery

### [중간] CORS 제한
현재 allow_origins=["*"] 으로 모든 도메인의 요청을 허용합니다.
→ 배포 시 실제 앱 도메인만 허용하도록 제한하세요.
  예: allow_origins=["http://192.168.0.10:3000"]

### [중간] 파일 실제 타입 검사 미비
확장자만 검사하므로 .jpg 확장자를 가진 악성 파일 업로드가 가능합니다.
→ python-magic 또는 Pillow로 실제 MIME 타입을 검증하세요.
  pip install Pillow
  from PIL import Image
  try:
      Image.open(io.BytesIO(content)).verify()
  except Exception:
      raise HTTPException(status_code=400, detail="올바른 이미지 파일이 아닙니다")

### [낮음] 클라우드 스토리지 이전
현재 로컬 uploads/ 디렉토리에 저장합니다.
서버가 재배포되거나 재시작되면 파일이 유실될 수 있습니다.
→ AWS S3 또는 GCS 등 영구 스토리지 사용을 권장합니다.


