import os
from fastapi import FastAPI
from fastapi.middleware.cors import CORSMiddleware
from contextlib import asynccontextmanager
from db import create_pool
from routers.menu_image import router

@asynccontextmanager
async def lifespan(app: FastAPI):
    # uploads 디렉토리가 없으면 서버 시작 시 FileNotFoundError 방지
    os.makedirs("uploads", exist_ok=True)
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
