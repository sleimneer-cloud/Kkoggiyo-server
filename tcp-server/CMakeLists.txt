# 1. CMake 최소 요구 버전
cmake_minimum_required(VERSION 3.10)

# 2. 프로젝트 이름
project(YogiyoServer)

# 3. C++ 17 표준 사용
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 4. 패키지 찾기 (멀티스레딩 및 cURL)
find_package(Threads REQUIRED)
find_package(CURL REQUIRED)

# ==========================================
# [서버 (Server) 타겟 설정]
# ==========================================
add_executable(server
    src/server.cpp
    src/DBConnectionPool.cpp
    src/DBHelper.cpp 
    src/net/ConnectionManager.cpp
    src/net/SocketIO.cpp
    src/router/MessageRouter.cpp
    src/handlers/AuthHandler.cpp
    src/handlers/ChatHandler.cpp
    src/handlers/AdminHandler.cpp
    src/PacketHandler.cpp
    src/SessionManager.cpp
    src/repo/InMemoryUserRepository.cpp
    src/services/CustomerAuthService.cpp
    src/services/BossAuthService.cpp
    src/services/OrderService.cpp
    src/services/AdminAuthService.cpp
    src/services/NotifyService.cpp
    src/services/RiderAuthService.cpp
    src/services/AdminService.cpp       # ← 추가!
    src/usecase/SearchUserUseCase.cpp   # ← 추가!
    src/usecase/PlaceOrderUseCase.cpp
    src/usecase/CancelOrderUseCase.cpp
    src/usecase/AcceptOrderUseCase.cpp
    src/usecase/UpdateStatusUseCase.cpp
    src/handlers/OrderHandler.cpp
)

target_include_directories(server PRIVATE include)
target_link_libraries(server PRIVATE Threads::Threads mariadb crypt ssl crypto)



# ==========================================
# [관리자 클라이언트 (Admin Client) 타겟 설정]
# ==========================================
add_executable(admin_client
    clients/admin_client.cpp
    clients/NetClient.cpp
)

target_include_directories(admin_client PRIVATE clients include)
target_link_libraries(admin_client PRIVATE Threads::Threads)


# ==========================================
# [사용자 클라이언트 (User Client) 타겟 설정]
# ==========================================
add_executable(user_client
    clients/user_client.cpp
    clients/NetClient.cpp
)

target_include_directories(user_client PRIVATE clients include)
target_link_libraries(user_client PRIVATE Threads::Threads)


# ==========================================
# [사장님 클라이언트 (Boss Client) 타겟 설정]
# ==========================================
add_executable(boss_client
    clients/boss_client.cpp
    clients/NetClient.cpp
    clients/HttpClient.cpp
)

target_include_directories(boss_client PRIVATE clients include)
target_link_libraries(boss_client PRIVATE Threads::Threads ${CURL_LIBRARIES})

# ==========================================
# [라이더 클라이언트 (Rider Client) 타겟 설정]
# ==========================================
add_executable(rider_client
    clients/rider_client.cpp
    clients/NetClient.cpp
)

target_include_directories(rider_client PRIVATE clients include)
target_link_libraries(rider_client PRIVATE Threads::Threads)
