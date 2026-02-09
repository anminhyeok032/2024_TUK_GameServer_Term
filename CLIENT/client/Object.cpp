#include "Object.h"
#include <cstring> // strcpy_s

OBJECT::OBJECT(sf::Texture& t, int x, int y, int x2, int y2) {
	m_showing = false;
	m_sprite.setTexture(t);
	m_sprite.setTextureRect(sf::IntRect(x, y, x2, y2));
	m_sprite.setScale(0.5, 0.5);

	hp_bar.setSize(sf::Vector2f((static_cast<float>(hp) / max_hp) * TILE_WIDTH, 5));	// hp바 크기 설정
	hp_bar.setFillColor(sf::Color::Red);			// hp바 색상 설정
	hp_bar.setOutlineColor(sf::Color::Black);		// hp바 테두리 색상 설정

}

OBJECT::OBJECT() {
	m_showing = false;
}

void OBJECT::show()
{
	m_showing = true;
}

void OBJECT::hide()
{
	m_showing = false;
}

void OBJECT::a_move(int x, int y) {
	m_sprite.setPosition((float)x, (float)y);
}

void OBJECT::a_draw(sf::RenderWindow* window) {
	if(window) window->draw(m_sprite);
}

void OBJECT::move(int x, int y) {
	m_x = x;
	m_y = y;
}

void OBJECT::draw(sf::RenderWindow* window, int g_left_x, int g_top_y) {
	if (false == m_showing || !window) return;
	float rx = (m_x - g_left_x) * TILE_WIDTH + 1;
	float ry = (m_y - g_top_y) * TILE_WIDTH + 1;
	m_sprite.setPosition(rx, ry);
	window->draw(m_sprite);
	auto size = m_name.getGlobalBounds();

	if (m_mess_end_time < std::chrono::system_clock::now()) {
		m_name.setPosition(rx + static_cast<float>(TILE_WIDTH) / 2 - size.width / 2, ry - 10);
		window->draw(m_name);
	}
	else {
		m_chat.setPosition(rx + TILE_WIDTH / 2 - size.width / 2, ry - 10);
		window->draw(m_chat);
	}

	hp_bar.setPosition(rx, ry - 10);	// hp바 위치 설정
	window->draw(hp_bar);	// hp바 그리기
}

void OBJECT::set_name(const char str[], sf::Font& font) {
	strcpy_s(name, str);
	m_name.setFont(font);
	m_name.setString(str);
	m_name.setFillColor(sf::Color(255, 255, 0));
	m_name.setStyle(sf::Text::Bold);
	m_name.setScale(0.5, 0.5);
}

void OBJECT::set_chat(const char str[], sf::Font& font)
{
	m_chat.setFont(font);
	m_chat.setString(str);
	m_chat.setFillColor(sf::Color(255, 255, 255));
	m_chat.setStyle(sf::Text::Bold);
	m_chat.setScale(0.5, 0.5);
	m_mess_end_time = std::chrono::system_clock::now() + std::chrono::seconds(3);	// 3초뒤에 메세지 사라짐
}
