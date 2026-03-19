from fastapi import APIRouter, UploadFile, File, Form, Depends, HTTPException
from fastapi.responses import FileResponse
from services.image_service import (
    save_image, update_menu_image, verify_store_owner
)
from schemas import ImageUploadResponse, ErrorResponse
from db import get_db
import os

router = APIRouter(tags=["메뉴 이미지"])

UPLOAD_DIR = "uploads"
ALLOWED_EXTENSIONS = {".jpg", ".jpeg", ".png", ".webp"}
MAX_FILE_SIZE = 5 * 1024 * 1024  # 5MB


@router.post(
    "/menu/image",
    response_model=ImageUploadResponse,
    responses={
        400: {"model": ErrorResponse, "description": "잘못된 요청"},
        403: {"model": ErrorResponse, "description": "권한 없음"},
        404: {"model": ErrorResponse, "description": "메뉴 없음"},
    },
    summary="메뉴 이미지 업로드",
    description="""
사장님이 메뉴 이미지를 업로드합니다.

**요청 형식:** multipart/form-data

**필드 설명:**
- `menu_id`: 이미지를 등록할 메뉴 ID
- `store_id`: 사장님의 가게 ID
- `user_id`: 사장님 로그인 ID (권한 검증용)
- `file`: 업로드할 이미지 파일 (jpg/jpeg/png/webp, 최대 5MB)

**동작:**
1. user_id와 store_id로 사장님 본인 가게인지 검증
2. 기존 이미지가 있으면 파일 삭제 후 교체
3. 새 이미지 저장 후 img_path 반환
    """
)
async def upload_menu_image(
    menu_id: int = Form(..., description="메뉴 ID"),
    store_id: int = Form(..., description="가게 ID"),
    user_id: str = Form(..., description="사장님 user_id (권한 검증용)"),
    file: UploadFile = File(..., description="이미지 파일 (jpg/jpeg/png/webp, 최대 5MB)"),
    cur=Depends(get_db)
):
    # 1. 확장자 검사
    ext = os.path.splitext(file.filename)[1].lower()
    if ext not in ALLOWED_EXTENSIONS:
        raise HTTPException(
            status_code=400,
            detail="jpg, jpeg, png, webp 형식만 허용됩니다"
        )

    # 2. 파일 크기 검사
    content = await file.read()
    if len(content) > MAX_FILE_SIZE:
        raise HTTPException(
            status_code=400,
            detail="파일 크기는 5MB 이하여야 합니다"
        )

    # 3. 사장님 권한 검증
    is_owner = await verify_store_owner(cur, user_id, store_id)
    if not is_owner:
        raise HTTPException(
            status_code=403,
            detail="해당 가게에 대한 권한이 없습니다"
        )

    # 4. menu_id 존재 여부를 DB에서 먼저 확인 (파일 저장 전에 검증)
    # ← 이 순서가 핵심: 파일을 저장하기 전에 메뉴가 존재하는지 확인해야
    #   DB 실패 시 저장된 파일이 고아(孤兒) 파일로 남는 문제를 방지할 수 있음
    await cur.execute(
        "SELECT img_path FROM Menu WHERE menu_id = %s AND store_id = %s",
        (menu_id, store_id)
    )
    menu_row = await cur.fetchone()
    if not menu_row:
        raise HTTPException(
            status_code=404,
            detail="해당 메뉴가 존재하지 않습니다"
        )

    # 5. 이미지 저장 (메뉴 존재 확인 후 저장)
    await file.seek(0)
    img_path = await save_image(file, content)

    # 6. DB 업데이트 (기존 이미지 교체 포함)
    await update_menu_image(cur, menu_id, store_id, img_path, old_path=menu_row[0])

    return ImageUploadResponse(
        success=True,
        menu_id=menu_id,
        img_path=img_path
    )


@router.get(
    "/menu/image/{menu_id}",
    responses={
        404: {"model": ErrorResponse, "description": "이미지 없음"},
    },
    summary="메뉴 이미지 조회",
    description="""
menu_id에 해당하는 메뉴 이미지를 반환합니다.

**동작:**
1. menu_id로 DB에서 img_path 조회
2. 해당 파일을 이미지 스트림으로 반환

**클라이언트 처리:**
- 응답 Content-Type: image/jpeg 또는 image/png
- 받은 바이너리 데이터를 직접 이미지로 렌더링
    """
)
async def get_menu_image(menu_id: int, cur=Depends(get_db)):
    await cur.execute(
        "SELECT img_path FROM Menu WHERE menu_id = %s", (menu_id,)
    )
    row = await cur.fetchone()

    if not row or not row[0]:
        raise HTTPException(status_code=404, detail="이미지가 없습니다")

    img_path = row[0]
    # os.path.basename()으로 ../../ 등 경로 탐색 공격 방지
    filename = os.path.basename(img_path)
    file_path = os.path.join(UPLOAD_DIR, filename)

    if not os.path.exists(file_path):
        raise HTTPException(
            status_code=404,
            detail="파일이 서버에 존재하지 않습니다"
        )

    return FileResponse(file_path)
