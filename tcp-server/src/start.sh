#!/bin/bash
# img-server 실행 스크립트
# 사용법: ./start.sh

# 스크립트 위치 기준으로 .env 탐색 (상위 디렉토리)
ENV_FILE="$(dirname "$0")/../.env"

if [ -f "$ENV_FILE" ]; then
    # .env 파일에서 주석·빈 줄 제외하고 환경변수로 export
    set -a
    source "$ENV_FILE"
    set +a
    echo "[설정] .env 로드 완료"
else
    echo "[경고] .env 파일 없음 — 시스템 환경변수를 사용합니다."
fi

# 필수 환경변수 체크
for var in DB_HOST DB_USER DB_PASS DB_NAME; do
    if [ -z "${!var}" ]; then
        echo "[오류] 필수 환경변수 누락: $var"
        exit 1
    fi
done

uvicorn main:app --reload --host 0.0.0.0 --port 8000
