#pragma once

// [공통 상수 정의]
// 서버와 클라이언트, 그리고 내부 로직에서 공통으로 사용하는 상수들을 모아둡니다.

constexpr int PORT_NUM = 4000;

// 문자열 크기 제한
constexpr int NAME_SIZE = 20;
constexpr int CHAT_SIZE = 300;

// 최대 동시 접속자 및 NPC 수
constexpr int MAX_USER = 20000;
constexpr int MAX_NPC = 200000;
constexpr int MAX_MAP_ITEM = 10000; // 필드 아이템 최대 개수
constexpr int MAX_OBJECTS = MAX_NPC + MAX_USER + MAX_MAP_ITEM;

// 맵 크기
constexpr int W_WIDTH = 2000;
constexpr int W_HEIGHT = 2000;

// 시야 범위
constexpr int VIEW_RANGE = 15;
constexpr int SEC_RANGE = VIEW_RANGE;
constexpr int SEC_ROW = 15;
constexpr int SEC_COL = 15;

// 인벤토리 크기
constexpr int INV_MAX_COL = 10;
constexpr int INV_MAX_ROW = 10;
