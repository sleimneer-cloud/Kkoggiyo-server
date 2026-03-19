#pragma once

#include <string>
#include <vector>
#include <curl/curl.h>

// FastAPI 이미지 서버와 HTTP로 통신하는 클라이언트
class HttpClient
{
public:
    static const char *IMG_SERVER_HOST; // FastAPI 서버 주소

    // 이미지 업로드 (POST /menu/image)
    // 반환: img_path 문자열 (실패 시 빈 문자열)
    static std::string uploadImage(
        const std::string &userId,
        int menuId,
        int storeId,
        const std::string &filePath);

    // 이미지 다운로드 (GET /menu/image/{menu_id})
    // 반환: 저장된 로컬 파일 경로 (실패 시 빈 문자열)
    static std::string downloadImage(
        int menuId,
        const std::string &savePath);

private:
    // curl 응답 데이터를 string에 쌓는 콜백
    static size_t writeCallback(
        void *ptr, size_t size, size_t nmemb, std::string *data);

    // curl 응답 데이터를 파일에 쓰는 콜백
    static size_t writeFileCallback(
        void *ptr, size_t size, size_t nmemb, FILE *file);
};