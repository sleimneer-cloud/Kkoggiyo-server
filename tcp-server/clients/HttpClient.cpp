#include "HttpClient.hpp"
#include "json.hpp"

#include <iostream>
#include <cstdio>

using json = nlohmann::json;

const char *HttpClient::IMG_SERVER_HOST = "http://127.0.0.1:8000";

size_t HttpClient::writeCallback(
    void *ptr, size_t size, size_t nmemb, std::string *data)
{
    data->append(static_cast<char *>(ptr), size * nmemb);
    return size * nmemb;
}

size_t HttpClient::writeFileCallback(
    void *ptr, size_t size, size_t nmemb, FILE *file)
{
    return fwrite(ptr, size, nmemb, file);
}

// ── 이미지 업로드 ─────────────────────────────────────────
std::string HttpClient::uploadImage(
    const std::string &userId,
    int menuId,
    int storeId,
    const std::string &filePath)
{
    CURL *curl = curl_easy_init();
    if (!curl)
    {
        std::cerr << "[HttpClient] curl 초기화 실패\n";
        return "";
    }

    std::string url = std::string(IMG_SERVER_HOST) + "/menu/image";
    std::string response;

    // multipart/form-data 구성
    curl_mime *form = curl_mime_init(curl);

    // user_id 필드
    curl_mimepart *part = curl_mime_addpart(form);
    curl_mime_name(part, "user_id");
    curl_mime_data(part, userId.c_str(), CURL_ZERO_TERMINATED);

    // menu_id 필드
    part = curl_mime_addpart(form);
    curl_mime_name(part, "menu_id");
    curl_mime_data(part, std::to_string(menuId).c_str(), CURL_ZERO_TERMINATED);

    // store_id 필드
    part = curl_mime_addpart(form);
    curl_mime_name(part, "store_id");
    curl_mime_data(part, std::to_string(storeId).c_str(), CURL_ZERO_TERMINATED);

    // file 필드 (실제 파일 첨부)
    part = curl_mime_addpart(form);
    curl_mime_name(part, "file");
    curl_mime_filedata(part, filePath.c_str());

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_MIMEPOST, form);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);

    CURLcode res = curl_easy_perform(curl);

    curl_mime_free(form);
    curl_easy_cleanup(curl);

    if (res != CURLE_OK)
    {
        std::cerr << "[HttpClient] 업로드 실패: "
                  << curl_easy_strerror(res) << "\n";
        return "";
    }

    // 응답 JSON 파싱
    try
    {
        auto j = json::parse(response);
        if (j.value("success", false))
        {
            std::string imgPath = j.value("img_path", "");
            std::cout << "[HttpClient] 업로드 성공: " << imgPath << "\n";
            return imgPath;
        }
        else
        {
            std::cerr << "[HttpClient] 업로드 실패: "
                      << j.value("detail", "알 수 없는 오류") << "\n";
            return "";
        }
    }
    catch (...)
    {
        std::cerr << "[HttpClient] 응답 파싱 실패: " << response << "\n";
        return "";
    }
}

// ── 이미지 다운로드 ───────────────────────────────────────
std::string HttpClient::downloadImage(
    int menuId,
    const std::string &savePath)
{
    CURL *curl = curl_easy_init();
    if (!curl)
    {
        std::cerr << "[HttpClient] curl 초기화 실패\n";
        return "";
    }

    std::string url = std::string(IMG_SERVER_HOST) + "/menu/image/" + std::to_string(menuId);

    FILE *fp = fopen(savePath.c_str(), "wb");
    if (!fp)
    {
        std::cerr << "[HttpClient] 파일 열기 실패: " << savePath << "\n";
        curl_easy_cleanup(curl);
        return "";
    }

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeFileCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);

    CURLcode res = curl_easy_perform(curl);
    long httpCode = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &httpCode);

    fclose(fp);
    curl_easy_cleanup(curl);

    if (res != CURLE_OK)
    {
        std::cerr << "[HttpClient] 다운로드 실패: "
                  << curl_easy_strerror(res) << "\n";
        return "";
    }

    if (httpCode == 200)
    {
        std::cout << "[HttpClient] 다운로드 성공: " << savePath << "\n";
        return savePath;
    }
    else
    {
        std::cerr << "[HttpClient] HTTP " << httpCode << " 오류\n";
        return "";
    }
}