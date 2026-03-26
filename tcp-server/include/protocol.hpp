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

    // [관리자  기능]
    CS_ADMIN_SEARCH_ID = 1003,          // 관리자 -> 서버: 아이디 검색
    SC_ADMIN_SEARCH_ID_RES = 1004,      // 서버 -> 관리자: 아이디 검색 결과
    CS_ADMIN_SEARCH_HISTORY = 1016,     // 관리자 -> 서버: 유저 주문 이력 검색
    SC_ADMIN_SEARCH_HISTORY_RES = 1017, // 서버 -> 관리자: 유저 주문 이력 검색 결과

    // [주문 관련]
    CS_ORDER_REQ = 300,    // 고객 -> 서버: 주문 요청
    SC_ORDER_NOTI = 301,   // 서버 -> 가게/관리자: 새 주문 알림
    CS_ORDER_ACCEPT = 305, // 가게 -> 서버: 주문 수락(조리시간 포함)
    SC_ORDER_STATUS = 303, // 서버 -> 전체: 주문 상태 변경 알림

    CS_ORDER_CANCEL = 304,
    CS_ORDER_PICKUP = 306,
    CS_ORDER_READY = 307,
    CS_ORDER_DONE = 308

};

// 3. 클라이언트 종류 (관리자, 고객, 가게, 라이더 구분용)

enum class ClientType : int
{
    CUSTOMER = 1,
    OWNER = 2,
    RIDER = 3,
    ADMIN = 0
};
