#pragma once
#include <SFML/Graphics.hpp>
#include <vector>
#include <set>
#include <chrono>
#include <string>
#include <windows.h> // MultiByteToWideChar
#include "Constants.h"
#include "protocol.h"
#include "ItemDatabase.h"

// [CP949 → sf::String 변환 헬퍼]
// Visual Studio 기본 저장 인코딩(CP949)으로 컴파일된 std::string을 sf::String으로 안전하게 변환
// InventoryUI, GameManager 양쪽에서 공용으로 사용
inline sf::String ToSfString(const std::string& cp949str)
{
	if (cp949str.empty()) return sf::String();
	int wlen = MultiByteToWideChar(CP_ACP, 0, cp949str.c_str(), -1, nullptr, 0);
	if (wlen <= 0) return sf::String(cp949str);
	std::wstring wstr(wlen - 1, L'\0');
	MultiByteToWideChar(CP_ACP, 0, cp949str.c_str(), -1, &wstr[0], wlen);
	return sf::String(wstr);
}

// [Inventory UI System]
// 인벤토리 화면 그리기, 마우스 입력 처리, 서버 동기화(Lazy Sync)를 담당

struct ClientItem {
	long long item_uid;
	int template_id;
	int count;
	int x, y;
	bool is_rotated;
	sf::Sprite sprite;
};

// 아이템 크기 정보
struct ClientItemTemplate {
	int         w, h;        // 인벤토리 grid 크기
	std::string sprite_id;   // ItemSpriteSheet 키
	std::string name;        // 표시 이름
};

class InventoryUI
{
private:
	sf::RenderWindow* window_;
	sf::Font* font_;

	bool isActive_;
	std::vector<ClientItem> myItems_;

	// 드래그 관련
	bool isDragging_;
	long long draggingItemUID_;
	sf::Vector2f dragOffset_;

	// 동기화 관련
	bool isDirty_;
	std::chrono::system_clock::time_point lastSyncTime_;
	std::set<long long> dirtyItemUIDs_; // 변경된 아이템 UID 목록

	// 상수
	static constexpr int UI_X = 100;
	static constexpr int UI_Y = 100;
	static constexpr int SLOT_SIZE = 40;
	static constexpr int MARGIN = 2;

public:
	InventoryUI();
	~InventoryUI();

	void Initialize(sf::RenderWindow* window, sf::Font* font);
	
	// 토글 및 상태 확인
	void Toggle();
	bool IsActive() const { return isActive_; }

	// 그리기 및 업데이트
	void Draw();
	void UpdateSync(); // 5초 주기 체크

	// 입력 처리
	void HandleInput(sf::Event& event);

	// 아이템 관리
	void AddItem(long long uid, int tid, int cnt, int x, int y, bool rot);
	void RemoveItem(long long uid); // 아이템 제거 함수 추가
	ClientItem* FindItem(long long uid);
	ClientItemTemplate GetItemTemplate(int tid);
	bool CanPlace(int startX, int startY, int w, int h, long long excludeUID);
	
	// 서버로 변경사항 전송
	void SyncToServer();
};

extern InventoryUI g_inventoryUI; // 전역 인스턴스 선언

// 한글 인코딩 위해 sf::String으로 변경
extern void push_status_message(const sf::String& msg);
