#pragma once
#include <SFML/Graphics.hpp>
#include <SFML/Network.hpp>
#include <vector>
#include <unordered_map>
#include <string>
#include "Object.h"
#include "Constants.h"

// [Game Manager]
// 클라이언트의 메인 로직, 렌더링 루프, 네트워크 연결 등을 총괄

struct ClientRankInfo {
	std::string name;
	int rank;
	int level;
};

class GameManager
{
public:
	sf::RenderWindow* window_;
	sf::Font font_;      // 영문 전용
	sf::Font fontKo_;    // 한글 지원 폰트 (채팅 히스토리용)
	sf::TcpSocket socket_;

	int myId_;
	int leftX_, topY_; // 카메라 오프셋

	// 공격 이펙트 구조체 및 리스트
	struct AttackEffect {
		int x, y; // 타일 좌표
		std::chrono::system_clock::time_point startTime; // 생성 시간
	};
	std::vector<AttackEffect> attackEffects_;

	// 마지막 이동 방향 저장 (공격용)
	int lastDirection_ = 1; // 0:UP, 1:DOWN, 2:LEFT, 3:RIGHT (기본값 DOWN)

	// 맵 아이템 맵 (Key: object_id)
	std::unordered_map<int, MapItemInfo> mapItems_;

	// 게임 객체들
	OBJECT avatar_;
	std::unordered_map<int, OBJECT> players_;

	// 맵 타일
	sf::Texture* boardTex_;
	sf::Texture* pieceTex_;
	sf::Texture* npcTex_;
	OBJECT whiteTile_;
	OBJECT blackTile_;

	// UI 요소
	sf::RectangleShape mapRect_;
	sf::CircleShape playerDot_;
	sf::RectangleShape hpBar_;
	sf::RectangleShape expBar_;
	sf::Text levelText_;

	// 채팅
	sf::RectangleShape chatBox_;
	sf::Text chatText_;
	std::string chatInput_;
	std::vector<sf::String> chatHistory_;
	bool isChatActive_;

	// 랭킹
	bool isRankingActive_;
	int rankingScrollIndex_;
	std::vector<ClientRankInfo> rankingData_;

public:
	GameManager();
	~GameManager();

	bool Initialize();
	bool Connect(const char* ip);
	void Run();

	void ProcessPacket(char* ptr);
	void ProcessData(char* net_buf, size_t io_byte);
	void SendPacket(void* packet);

	void HandleInput();
	void Draw();
	void DrawRanking();

private:
	void ClientInitialize();
	void ClientFinish();
};

extern GameManager g_gameManager;
