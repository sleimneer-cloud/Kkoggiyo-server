import aiomysql
import os

pool = None

async def create_pool():
    global pool
    pool = await aiomysql.create_pool(
        host=os.environ["DB_HOST"],
        port=int(os.environ.get("DB_PORT", "3306")),
        user=os.environ["DB_USER"],
        password=os.environ["DB_PASS"],
        db=os.environ["DB_NAME"],
        autocommit=True,
        pool_recycle=3600,  # 1시간마다 커넥션 교체 (MySQL wait_timeout 8시간보다 짧게 설정)
    )

async def get_db():
    async with pool.acquire() as conn:
        try:
            await conn.ping(reconnect=True)  # 죽은 커넥션이면 자동 재연결
        except Exception:
            pass
        async with conn.cursor() as cur:
            yield cur
