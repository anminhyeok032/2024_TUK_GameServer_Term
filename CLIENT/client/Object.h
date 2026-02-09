#pragma once
#include <SFML/Graphics.hpp>
#include <chrono>
#include <iostream>
#include "Constants.h"

// [Game Object Class]
// 캐릭터, NPC, 타일 등 게임 내 모든 시각적 객체를 표현
class OBJECT {
private:
	bool m_showing;
	sf::Sprite m_sprite;

	sf::Text m_name;
	sf::Text m_chat;
	std::chrono::system_clock::time_point m_mess_end_time;

public:
	int m_x, m_y;
	char name[NAME_SIZE];

	sf::RectangleShape hp_bar;	// hp 표시 사각형
	int max_hp = 50;
	int hp = max_hp;
	int exp = 0;
	int level = 1;


	OBJECT(sf::Texture& t, int x, int y, int x2, int y2);
	OBJECT();
	
	void show();
	void hide();
	void a_move(int x, int y);
	void a_draw(sf::RenderWindow* window); // g_window 대신 인자로 받음
	void move(int x, int y);
	void draw(sf::RenderWindow* window, int g_left_x, int g_top_y); // 렌더링에 필요한 정보 인자로 받음
	void set_name(const char str[], sf::Font& font);
	void set_chat(const char str[], sf::Font& font);
};
