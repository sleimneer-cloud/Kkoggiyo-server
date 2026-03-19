import os
import uuid
from fastapi import UploadFile

BASE_DIR = os.path.dirname(os.path.abspath(__file__))
UPLOAD_DIR = os.path.join(BASE_DIR, "uploads")


async def save_image(file: UploadFile, content: bytes) -> str:
    ext = os.path.splitext(file.filename)[1].lower()
    unique_name = f"{uuid.uuid4().hex}{ext}"
    save_path = os.path.join(UPLOAD_DIR, unique_name)
    with open(save_path, "wb") as f:
        f.write(content)
    return f"/images/{unique_name}"

async def delete_old_image(img_path: str):
    if not img_path:
        return
    # os.path.basename()으로 경로 탐색 공격 방지
    filename = os.path.basename(img_path)
    file_path = os.path.join(UPLOAD_DIR, filename)
    if os.path.exists(file_path):
        os.remove(file_path)

async def update_menu_image(cur, menu_id: int, store_id: int, img_path: str, old_path: str = None):
    # router에서 이미 menu_id 존재 확인 및 old_path를 넘겨받으므로 여기서는 UPDATE만 수행
    await cur.execute(
        "UPDATE Menu SET img_path = %s WHERE menu_id = %s AND store_id = %s",
        (img_path, menu_id, store_id)
    )

    # 기존 파일 삭제
    if old_path:
        await delete_old_image(old_path)
    return True

async def verify_store_owner(cur, user_id: str, store_id: int) -> bool:
    await cur.execute(
        """
        SELECT s.store_id
        FROM Store s
        JOIN members m ON s.owner_id = m.user_id
        WHERE s.store_id = %s AND m.user_id = %s AND m.role = 2
        """,
        (store_id, user_id)
    )
    row = await cur.fetchone()
    return row is not None
