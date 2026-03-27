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
    CS_ADMIN_REFUND_REQ = 1300,         // 관리자 -> 서버: 환불 처리 요청
    SC_ADMIN_REFUND_RES = 1301,         // 서버 -> 관리자: 환불 처리 결과
    CS_ADMIN_BAN_USER = 1400,           // 관리자 -> 서버: 유저 차단 요청
    SC_ADMIN_BAN_USER_RES = 1401,       // 서버 -> 관리자: 유저 차단 결과

    // [주문 관련]
    CS_ORDER_REQ = 300,    // 고객 -> 서버: 주문 요청
    SC_ORDER_NOTI = 301,   // 서버 -> 가게/관리자: 새 주문 알림
    CS_ORDER_ACCEPT = 305, // 가게 -> 서버: 주문 수락(조리시간 포함)
    SC_ORDER_STATUS = 303, // 서버 -> 전체: 주문 상태 변경 알림

    CS_ORDER_CANCEL = 304, // 주문 취소
    CS_ORDER_PICKUP = 306, // 라이더 -> 서버: 주문 픽업 완료
    CS_ORDER_READY = 307,  // 가게 -> 서버: 음식 준비 완료
    CS_ORDER_DONE = 308,   // 라이더 -> 서버: 주문 배달 완료

    CS_STORE_OPEN_REQ = 4910, // 클라 → 서버 : 가게 오픈 알림
    SC_STORE_OPEN_RES = 4911, // 서버 → 클라 : 가게 오픈 응답

    // [고객 파트]
    USER_REGISTER_REQ = 2001,    // 클라 → 서버 : 회원가입 요청 (인증번호 발송 요청)
    USER_REGISTER_RES = 2002,    // 서버 → 클라 : 회원가입 응답 (인증번호 발송 완료)
    USER_ID_CHECK_REQ = 2003,    // 클라 → 서버 : 아이디 중복 확인 요청
    USER_ID_CHECK_RES = 2004,    // 서버 → 클라 : 아이디 중복 확인 응답
    USER_VERIFY_REQ = 2005,      // 클라 → 서버 : 인증번호 확인 요청
    USER_VERIFY_RES = 2006,      // 서버 → 클라 : 인증번호 확인 응답
    USER_LOGIN_REQ = 2007,       // 클라 → 서버 : 로그인 요청
    USER_LOGIN_RES = 2008,       // 서버 → 클라 : 로그인 응답
    USER_NAME_ALT_REQ = 2009,    // 클라 → 서버 : 닉네임 수정 요청
    USER_NAME_ALT_RES = 2010,    // 서버 → 클라 : 닉네임 수정 응답
    USER_ADDR_LIST_REQ = 2011,   // 클라 → 서버 : 주소 목록 요청
    USER_ADDR_LIST_RES = 2012,   // 서버 → 클라 : 주소 목록 응답
    USER_ADDR_ADD_REQ = 2013,    // 클라 → 서버 : 주소 추가 요청
    USER_ADDR_ADD_RES = 2014,    // 서버 → 클라 : 주소 추가 응답
    USER_ADDR_ALT_REQ = 2015,    // 클라 → 서버 : 주소 수정 요청
    USER_ADDR_ALT_RES = 2016,    // 서버 → 클라 : 주소 수정 응답
    USER_ADDR_DEL_REQ = 2017,    // 클라 → 서버 : 주소 삭제 요청
    USER_ADDR_DEL_RES = 2018,    // 서버 → 클라 : 주소 삭제 응답
    USER_WITHDRAW_PW_REQ = 2019, // 클라 → 서버 : 탈퇴 비밀번호 검증 요청
    USER_WITHDRAW_PW_RES = 2020, // 서버 → 클라 : 탈퇴 비밀번호 검증 응답
    USER_WITHDRAW_REQ = 2021,    // 클라 → 서버 : 회원탈퇴 요청
    USER_WITHDRAW_RES = 2022,    // 서버 → 클라 : 회원탈퇴 응답
    USER_ORDER_INFO_REQ = 2023,  // 클라 → 서버 : 주문 정보 전송
    USER_ORDER_INFO_RES = 2024,  // 서버 → 클라 : 주문 정보 응답
    USER_ORDER_CHECK_REQ = 2025, // 클라 → 서버 : 주문 상태 확인
    USER_ORDER_CHECK_RES = 2026, // 서버 → 클라 : 주문 상태 응답
    USER_CHAT_REQ = 2027,        // 클라 → 서버 : 관리자 채팅 요청
    USER_CHAT_RES = 2028,        // 서버 → 클라 : 관리자 채팅 응답
    USER_MESSAGE_REQ = 2029,     // 클라 → 서버 : 채팅 메시지 전송
    USER_MESSAGE_RES = 2030,     // 서버 → 클라 : 채팅 메시지 응답
};

// 3. 클라이언트 종류 (관리자, 고객, 가게, 라이더 구분용)

enum class ClientType : int
{
    CUSTOMER = 1,
    OWNER = 2,
    RIDER = 3,
    ADMIN = 0
};
