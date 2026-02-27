#pragma once
#include "Constants.h"

// Packet ID
constexpr char CS_LOGIN = 0;
constexpr char CS_MOVE = 1;
constexpr char CS_CHAT = 2;
constexpr char CS_ATTACK = 3;			// 4 방향 공격
constexpr char CS_TELEPORT = 4;			// RANDOM한 위치로 Teleport, Stress Test때 핫스팟으로 이동하기 위해 사용
constexpr char CS_LOGOUT = 5;			// 클라이언트에서 정상적으로 종료를 알리는 패킷
constexpr char CS_RANKING_REQ = 6;
constexpr char CS_ITEM_MOVE = 7;
constexpr char CS_ITEM_DROP = 8;        // 아이템 버리기 요청
constexpr char CS_ITEM_PICKUP = 9;      // 아이템 줍기 요청

constexpr char SC_LOGIN_INFO = 2;
constexpr char SC_LOGIN_FAIL = 3;
constexpr char SC_ADD_OBJECT = 4;
constexpr char SC_REMOVE_OBJECT = 5;
constexpr char SC_MOVE_OBJECT = 6;
constexpr char SC_CHAT = 7;
constexpr char SC_STAT_CHANGE = 8;
constexpr char SC_ATTACK = 9;			// 공격 표시는 별도 추가
constexpr char SC_RANKING = 10;
constexpr char SC_ITEM_MOVE_RESULT = 11; // 아이템 이동 결과
constexpr char SC_ADD_MAP_ITEM = 12;     // 필드 아이템 생성 알림
constexpr char SC_REMOVE_MAP_ITEM = 13;  // 필드 아이템 삭제 알림
constexpr char SC_GET_ITEM = 14;         // 아이템 획득 알림 (인벤토리 추가)
constexpr char SC_INVENTORY_SYNC = 15;   // 로그인 시 인벤토리 전체 동기화

#pragma pack (push, 1)

// 패킷 맨 앞 구조 (~28 바이트)
struct RankInfo {
	char name[NAME_SIZE]; // 이름
	int rank;             // 등수
	int level;            // 레벨
};

struct CS_LOGIN_PACKET {
	unsigned short size;
	char	type;
	char	name[NAME_SIZE];
};

struct CS_MOVE_PACKET {
	unsigned short size;
	char	type;
	char	direction;  // 0 : UP, 1 : DOWN, 2 : LEFT, 3 : RIGHT
	unsigned	move_time;
};

struct CS_CHAT_PACKET {
	unsigned short size;			// 크기가 가변이다, mess의 실제 size를 보낸다.
	char	type;
	char	mess[CHAT_SIZE];
};

struct CS_TELEPORT_PACKET {			// 강제로 좌표이동 하는 패킷, 디버깅용
	unsigned short size;
	char	type;
};

struct CS_LOGOUT_PACKET {
	unsigned short size;
	char	type;
};

// 공격 패킷 방향 정보 추가
struct CS_ATTACK_PACKET {
	unsigned short size;
	char	type;
	char	attack_type;	// 0: 평타, 1: 범위공격
	char	attack_direction;  // 0 : UP, 1 : DOWN, 2 : LEFT, 3 : RIGHT, 4 : 4방향
};

// 랭킹 요청 패킷
struct CS_RANKING_REQ_PACKET {
	unsigned short size;
	char type;
};

// 아이템 이동 요청 패킷
struct CS_ITEM_MOVE_PACKET {
	unsigned short size;
	char type;
	long long item_uid;	// 이동할 아이템의 고유 ID
	short new_x;		// 목표 인벤토리 x 좌표
	short new_y;		// 목표 인벤토리 y 좌표
	bool is_rotated;	// 회전 여부
};

// 아이템 버리기 패킷
struct CS_ITEM_DROP_PACKET {
	unsigned short size;
	char type;
	long long item_uid; // 버릴 아이템 UID
};

// 아이템 줍기 패킷
struct CS_ITEM_PICKUP_PACKET {
	unsigned short size;
	char type;
};

struct SC_LOGIN_INFO_PACKET {
	unsigned short size;
	char	type;
	int		visual;				// 외형, 나중에 장비로 바뀔 수 있음
	int		id;
	int		hp;
	int		max_hp;
	int		exp;
	int		level;
	short	x, y;
};

struct SC_ADD_OBJECT_PACKET {
	unsigned short size;
	char	type;
	int		id;
	int		visual;				// 어떤 종류의 OBJECT인지 구분
	short	x, y;
	char	name[NAME_SIZE];
};

struct SC_REMOVE_OBJECT_PACKET {
	unsigned short size;
	char	type;
	int		id;
};

struct SC_MOVE_OBJECT_PACKET {
	unsigned short size;
	char	type;
	int		id;
	short	x, y;
	unsigned int move_time;
};

struct SC_CHAT_PACKET {
	unsigned short size;
	char	type;
	int		id;
	char	mess[CHAT_SIZE];
};

struct SC_LOGIN_FAIL_PACKET {
	unsigned short size;
	char	type;
};

struct SC_STAT_CHANGE_PACKET {
	unsigned short size;
	char	type;
	int		hp;
	int		max_hp;
	int		exp;
	int		level;

};

struct SC_ATTACK_PACKET {
	unsigned short size;
	char	type;
	int		attacker_id;
	int		damaged_id;
	int		max_hp;
	int		hp;
	int     exp;
	int		damage;	

	// 공격 시각화를 위한 정보
	char	attack_type;	// 0: 평타, 1: 범위공격
	char	direction;		// 평타일 때 방향 (0:UP, 1:DOWN, 2:LEFT, 3:RIGHT)
	short	center_x;		// 공격자 위치 (이펙트 기준점)
	short	center_y;
};

// 랭킹 전송 패킷
struct SC_RANKING_PACKET {
	unsigned short size;
	char type; // SC_RANKING
	int count; // 실제 랭킹 개수
	RankInfo ranks[100]; // 최대 100명까지
};

// 아이템 이동 결과 패킷
struct SC_ITEM_MOVE_RESULT_PACKET {
	unsigned short size;
	char type;
	long long item_uid;
	bool success;		// 성공 여부
	short x;			// 최종 확정된 x (실패 시 원래 위치)
	short y;			// 최종 확정된 y
	bool is_rotated;
};

// 필드 아이템 생성 알림
struct SC_ADD_MAP_ITEM_PACKET {
	unsigned short size;
	char type;
	int object_id;    // 맵상의 오브젝트 ID (Player/NPC ID와 겹치지 않게 관리 필요)
	long long item_uid; // 실제 아이템 데이터 ID
	int template_id;
	int count;
	short x, y;
};

// 필드 아이템 삭제 알림
struct SC_REMOVE_MAP_ITEM_PACKET {
	unsigned short size;
	char type;
	int object_id;
};

// 아이템 획득 알림
struct SC_GET_ITEM_PACKET {
	unsigned short size;
	char type;
	long long item_uid;
	int template_id;
	int count;
	short x, y;
	bool is_rotated;
};

// 인벤토리 단일 슬롯 데이터 (SC_INVENTORY_SYNC 내부 배열용)
struct InventorySlot {
	long long item_uid;
	int template_id;
	int count;
	short x, y;
	bool is_rotated;
};

// 로그인 시 인벤토리 전체 동기화 패킷 (가변 크기)
struct SC_INVENTORY_SYNC_PACKET {
	unsigned short size;
	char type;
	int item_count;
	InventorySlot items[1]; // 가변 배열
};

#pragma pack (pop)
