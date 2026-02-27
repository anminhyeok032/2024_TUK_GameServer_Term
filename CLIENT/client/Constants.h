#pragma once

// [공통 상수 정의 - 클라이언트용]
// 서버와 동일한 상수를 사용해야 합니다.

constexpr int PORT_NUM = 4000;

// 문자열 크기 제한
constexpr int NAME_SIZE = 20;
constexpr int CHAT_SIZE = 300;

// 최대 동시 접속자 및 NPC 수
constexpr int MAX_USER = 20000;
constexpr int MAX_NPC = 200000;

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


constexpr int BUF_SIZE = 4096;

// 클라이언트 화면 설정값
constexpr auto SCREEN_WIDTH = 20;
constexpr auto SCREEN_HEIGHT = 20;

constexpr auto TILE_WIDTH = 65 / 2;
constexpr auto WINDOW_WIDTH = SCREEN_WIDTH * TILE_WIDTH;   // size of window
constexpr auto WINDOW_HEIGHT = SCREEN_WIDTH * TILE_WIDTH;

constexpr auto MAP_SIZE = 200;
constexpr auto MAP_WIDTH = WINDOW_WIDTH - MAP_SIZE;
constexpr auto MAP_HEIGHT = 0;