from pydantic import BaseModel
from typing import Optional

class ImageUploadResponse(BaseModel):
    success: bool
    menu_id: int
    img_path: str

class ErrorResponse(BaseModel):
    success: bool = False
    detail: str

class MenuImageInfo(BaseModel):
    menu_id: int
    store_id: int
    menu_name: str
    img_path: Optional[str] = None
