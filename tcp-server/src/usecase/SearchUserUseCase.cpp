#include "usecase/SearchUserUseCase.hpp"
#include "services/AdminService.hpp"

// 생성자를 통해 의존성 주입 (AdminService)
SearchUserUseCase::SearchUserUseCase(AdminService &adminSvc)
    : adminSvc_(adminSvc)
{
}

json SearchUserUseCase::execute(int targetId)
{
    // 추가적인 비즈니스 로직이 필요하다면 여기서 처리합니다.
    // (예: 관리자가 조회할 수 없는 블랙리스트 유저인지 필터링 등)

    // Service 계층을 호출하여 데이터베이스에서 유저 정보를 가져옵니다.
    return adminSvc_.getUserById(targetId);
}