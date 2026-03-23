import os
from fastapi import FastAPI
from fastapi.middleware.cors import CORSMiddleware
from fastapi.staticfiles import StaticFiles
from contextlib import asynccontextmanager
from db import create_pool
from routers.menu_image import router

# 🌟 핵심 수정 1: image_service에서 정확한 절대 경로(UPLOAD_DIR)를 가져옵니다.
from services.image_service import UPLOAD_DIR

@asynccontextmanager
async def lifespan(app: FastAPI):
    # 🌟 핵심 수정 2: 'uploads' 글자 대신 정확한 UPLOAD_DIR 경로를 사용해 폴더를 만듭니다.
    os.makedirs(UPLOAD_DIR, exist_ok=True)
    await create_pool()
    yield

app = FastAPI(
    title="꼬끼요 이미지 서버",
    description="메뉴 이미지 업로드 / 조회 API — 사장님 앱 연동용",
    version="1.0.0",
    lifespan=lifespan
)

app.add_middleware(
    CORSMiddleware,
    allow_origins=["*"],
    allow_methods=["*"],
    allow_headers=["*"],
)

app.include_router(router)

# 🌟 핵심 수정 3: 'uploads' 글자 대신 UPLOAD_DIR을 연결합니다.
app.mount("/images", StaticFiles(directory=UPLOAD_DIR), name="images")