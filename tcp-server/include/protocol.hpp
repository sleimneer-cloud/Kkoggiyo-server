#pragma once
#include <stdint.h>

// 1. 패킷 타입 (우리가 앞으로 구현할 기능들의 번호표)
// CS: Client -> Server / SC: Server -> Client
enum class PacketType : int32_t // 4바이트 정수형으로 저장
{
    NONE = 0,

    // [로그인/회원가입 관련]
    CS_LOGIN_REQ = 100, // 클라 -> 서버: 로그인 요청
    SC_LOGIN_RES = 101, // 서버 -> 클라: 로그인 결과

    CS_REGISTER_REQ = 102, // 클라 -> 서버: 회원가입 요청
    SC_REGISTER_RES = 103, // 서버 -> 클라: 회원가입 결과

    // [채팅 관련 - 관리자 중재용]
    CS_CHAT_REQ = 200,  // 클라 -> 서버: 채팅 전송
    SC_CHAT_NOTI = 201, // 서버 -> 클라: 채팅 수신

    // [주문 관련]
    CS_ORDER_REQ = 300,    // 고객 -> 서버: 주문 요청
    SC_ORDER_NOTI = 301,   // 서버 -> 가게/관리자: 새 주문 알림
    CS_ORDER_ACCEPT = 302, // 가게 -> 서버: 주문 수락(조리시간 포함)
    SC_ORDER_STATUS = 303  // 서버 -> 전체: 주문 상태 변경 알림
};

// 3. 클라이언트 종류 (관리자, 고객, 가게, 라이더 구분용)

enum class ClientType : int
{
    CUSTOMER = 1,
    OWNER = 2,
    RIDER = 3,
    ADMIN = 0
};
