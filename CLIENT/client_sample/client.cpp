#include <SFML/Graphics.hpp>
#include <SFML/Network.hpp>
#include <iostream>
#include <unordered_map>
#include <Windows.h>
#include <chrono>
#include <string>
#include <vector>
using namespace std;

//#pragma comment (lib, "opengl32.lib")
//#pragma comment (lib, "winmm.lib")
//#pragma comment (lib, "ws2_32.lib")

#include "..\..\SERVER\protocol.h"

sf::TcpSocket s_socket;

constexpr int BUF_SIZE = 200;

constexpr auto SCREEN_WIDTH = 20;
constexpr auto SCREEN_HEIGHT = 20;

constexpr auto TILE_WIDTH = 65 / 2;
constexpr auto WINDOW_WIDTH = SCREEN_WIDTH * TILE_WIDTH;   // size of window
constexpr auto WINDOW_HEIGHT = SCREEN_WIDTH * TILE_WIDTH;

constexpr auto MAP_SIZE = 200;
constexpr auto MAP_WIDTH = WINDOW_WIDTH - MAP_SIZE;
constexpr auto MAP_HEIGHT = 0;

int g_left_x;
int g_top_y;
int g_myid;

sf::RenderWindow* g_window;
sf::Font g_font;

bool isChatActive = false;
std::string chatInput;
std::vector<std::string> chatHistory;

class OBJECT {
private:
	bool m_showing;
	sf::Sprite m_sprite;

	sf::Text m_name;
	sf::Text m_chat;
	chrono::system_clock::time_point m_mess_end_time;

public:
	int m_x, m_y;
	char name[NAME_SIZE];

	sf::RectangleShape hp_bar;	// hp 표시 사각형

	OBJECT(sf::Texture& t, int x, int y, int x2, int y2) {
		m_showing = false;
		m_sprite.setTexture(t);
		m_sprite.setTextureRect(sf::IntRect(x, y, x2, y2));
		m_sprite.setScale(0.5, 0.5);

		hp_bar.setSize(sf::Vector2f(TILE_WIDTH, 5));	// hp바 크기 설정
		hp_bar.setFillColor(sf::Color::Red);			// hp바 색상 설정
		hp_bar.setOutlineColor(sf::Color::Black);		// hp바 테두리 색상 설정
		
	}
	OBJECT() {
		m_showing = false;
	}
	void show()
	{
		m_showing = true;
	}
	void hide()
	{
		m_showing = false;
	}

	void a_move(int x, int y) {
		m_sprite.setPosition((float)x, (float)y);
	}

	void a_draw() {
		g_window->draw(m_sprite);
	}

	void move(int x, int y) {
		m_x = x;
		m_y = y;
	}
	void draw() {
		if (false == m_showing) return;
		float rx = (m_x - g_left_x) * TILE_WIDTH + 1;
		float ry = (m_y - g_top_y) * TILE_WIDTH + 1;
		m_sprite.setPosition(rx, ry);
		g_window->draw(m_sprite);
		auto size = m_name.getGlobalBounds();

		if (m_mess_end_time < chrono::system_clock::now()) {
			m_name.setPosition(rx + TILE_WIDTH/2 - size.width / 2, ry - 10);
			g_window->draw(m_name);
		}
		else {
			m_chat.setPosition(rx + TILE_WIDTH/2 - size.width / 2, ry - 10);
			g_window->draw(m_chat);
		}

		hp_bar.setPosition(rx, ry - 10);	// hp바 위치 설정
		g_window->draw(hp_bar);	// hp바 그리기
	}
	void set_name(const char str[]) {
		strcpy_s(name, str);
		m_name.setFont(g_font);
		m_name.setString(str);
		m_name.setFillColor(sf::Color(255, 255, 0));
		m_name.setStyle(sf::Text::Bold);
		m_name.setScale(0.5, 0.5);
	}
	void set_chat(const char str[])
	{
		m_chat.setFont(g_font);
		m_chat.setString(str);
		m_chat.setFillColor(sf::Color(255, 255, 255));
		m_chat.setStyle(sf::Text::Bold);
		m_chat.setScale(0.5, 0.5);
		m_mess_end_time = chrono::system_clock::now() + chrono::seconds(3);	// 3초동안 메세지 출력
	}
};

OBJECT avatar;
unordered_map <int, OBJECT> players;

OBJECT white_tile;
OBJECT black_tile;

sf::Texture* board;
sf::Texture* pieces;
sf::RectangleShape mapRectangle(sf::Vector2f(MAP_SIZE, MAP_SIZE));	// 맵 표시 사각형
sf::CircleShape playerDot(5.f);										// 플레이어 표시


sf::RectangleShape chatBox(sf::Vector2f(800, 30));


sf::Text chatText("", g_font, 20);


void client_initialize()
{
	board = new sf::Texture;
	pieces = new sf::Texture;
	board->loadFromFile("chessmap.bmp");
	pieces->loadFromFile("chess2.png");
	if (false == g_font.loadFromFile("cour.ttf")) {
		cout << "Font Loading Error!\n";
		exit(-1);
	}
	white_tile = OBJECT{ *board, 5, 5, 64, 64 };
	black_tile = OBJECT{ *board, 69, 5, 64, 64 };
	avatar = OBJECT{ *pieces, 128, 0, 64, 64 };
	avatar.move(4, 4);
	

	// MAP 생성 및 크기 설정
	mapRectangle.setPosition(MAP_WIDTH, MAP_HEIGHT);
	mapRectangle.setFillColor(sf::Color(255, 255, 255, 128));	// 맵 알파값 조정

	playerDot.setFillColor(sf::Color::Red); // 플레이어 색상을 빨간색으로 설정

	avatar.hp_bar.setPosition(avatar.m_x - (WINDOW_WIDTH / 2), avatar.m_y - (WINDOW_HEIGHT / 2) - 10);	// hp바 위치 설정

	// 채팅창 설정
	chatBox.setFillColor(sf::Color(0, 0, 0, 150));
	chatBox.setPosition(0, 570);
	chatText.setFillColor(sf::Color::White);
	chatText.setPosition(5, 575);
}

void client_finish()
{
	players.clear();
	delete board;
	delete pieces;
}

void ProcessPacket(char* ptr)
{
	static bool first_time = true;
	switch (ptr[2])
	{
	case SC_LOGIN_INFO:
	{
		SC_LOGIN_INFO_PACKET * packet = reinterpret_cast<SC_LOGIN_INFO_PACKET*>(ptr);
		g_myid = packet->id;
		avatar.m_x = packet->x;
		avatar.m_y = packet->y;
		g_left_x = packet->x - (SCREEN_WIDTH / 2);
		g_top_y = packet->y - (SCREEN_HEIGHT / 2);
		avatar.show();
	}
	break;

	case SC_ADD_OBJECT:
	{
		SC_ADD_OBJECT_PACKET * my_packet = reinterpret_cast<SC_ADD_OBJECT_PACKET*>(ptr);
		int id = my_packet->id;

		if (id == g_myid) {
			avatar.move(my_packet->x, my_packet->y);
			g_left_x = my_packet->x - (SCREEN_WIDTH / 2);
			g_top_y = my_packet->y - (SCREEN_HEIGHT / 2);
			avatar.show();
		}
		else {
			players[id] = OBJECT{ *pieces, 0, 0, 64, 64 };
			players[id].move(my_packet->x, my_packet->y);
			players[id].set_name(my_packet->name);
			players[id].show();
		}
		break;
	}
	case SC_MOVE_OBJECT:
	{
		SC_MOVE_OBJECT_PACKET* my_packet = reinterpret_cast<SC_MOVE_OBJECT_PACKET*>(ptr);
		int other_id = my_packet->id;
		if (other_id == g_myid) {
			avatar.move(my_packet->x, my_packet->y);
			g_left_x = my_packet->x - (SCREEN_WIDTH / 2);
			g_top_y = my_packet->y - (SCREEN_HEIGHT / 2);
		}
		else  {
			players[other_id].move(my_packet->x, my_packet->y);
		}
		break;
	}

	case SC_REMOVE_OBJECT:
	{
		SC_REMOVE_OBJECT_PACKET* my_packet = reinterpret_cast<SC_REMOVE_OBJECT_PACKET*>(ptr);
		int other_id = my_packet->id;
		if (other_id == g_myid) {
			avatar.hide();
		}
		else {
			players.erase(other_id);
		}

		break;
	}
	case SC_STAT_CHANGE:
	{
		SC_STAT_CHANGE_PACKET* my_packet = reinterpret_cast<SC_STAT_CHANGE_PACKET*>(ptr);
		avatar.hp_bar.setSize(sf::Vector2f(my_packet->hp, 5));	// hp바 크기 설정
		break;
	}
	case SC_ATTACK:
	{
		SC_ATTACK_PACKET* p = reinterpret_cast<SC_ATTACK_PACKET*>(ptr);
		bool is_dead = static_cast<bool>(p->damaged_state);

		std::string mess;

		if(is_dead == false) // 데미지를 받았을 때
		{
			// 내가 때렸을 때
			if(p->attacker_id == g_myid)
				mess.append("You attack ").append(players[p->damaged_id].name).append(" to give 10 damage.");
			else if(p->damaged_id == g_myid) // 내가 맞았을 때
				mess.append(players[p->attacker_id].name).append("attack you to give 10 damage.");
			else
				mess.append(players[p->attacker_id].name).append("attack ").append(players[p->damaged_id].name).append(" to give 10 damage.");
		}
		else // 죽었을 때
		{
			mess.append(players[p->attacker_id].name).append("가 ").append(players[p->damaged_id].name).append("를 때려서 죽였습니다.");
		}
		chatHistory.push_back(mess);
		break;
	}
	case SC_CHAT:
	{
		SC_CHAT_PACKET* my_packet = reinterpret_cast<SC_CHAT_PACKET*>(ptr);
		int other_id = my_packet->id;

		chatInput.clear(); 
		std::string playerName(players[other_id].name, strnlen(players[other_id].name, sizeof(players[other_id].name)));
		chatInput.append("[").append(playerName).append("] : ").append(my_packet->mess);
		chatHistory.push_back(chatInput);
		chatInput.clear();

		if (other_id == g_myid)
		{
			avatar.set_chat(my_packet->mess);
		}
		else if (other_id == -1)
		{

		}
		else
		{
			players[other_id].set_chat(my_packet->mess);
		}
		break;
	}
	/*case SC_LOGIN_OK:
		std::cout << "Login 성공!!\n";
		break;*/
	case SC_LOGIN_FAIL:
		std::cout << "ID가 틀렸습니다\n";
		std::cout << "프로그램을 종료합니다.\n";
		return;
		break;
	default:
		printf("Unknown PACKET type [%d]\n", ptr[1]);
	}
}

void process_data(char* net_buf, size_t io_byte)
{
	char* ptr = net_buf;
	static size_t in_packet_size = 0;
	static size_t saved_packet_size = 0;
	static char packet_buffer[BUF_SIZE];

	while (0 != io_byte) {
		if (0 == in_packet_size) in_packet_size = ptr[0];
		if (io_byte + saved_packet_size >= in_packet_size) {
			memcpy(packet_buffer + saved_packet_size, ptr, in_packet_size - saved_packet_size);
			ProcessPacket(packet_buffer);
			ptr += in_packet_size - saved_packet_size;
			io_byte -= in_packet_size - saved_packet_size;
			in_packet_size = 0;
			saved_packet_size = 0;
		}
		else {
			memcpy(packet_buffer + saved_packet_size, ptr, io_byte);
			saved_packet_size += io_byte;
			io_byte = 0;
		}
	}
}

void client_main()
{
	char net_buf[BUF_SIZE];
	size_t	received;

	auto recv_result = s_socket.receive(net_buf, BUF_SIZE, received);
	if (recv_result == sf::Socket::Error)
	{
		wcout << L"Recv 에러!";
		exit(-1);
	}
	if (recv_result == sf::Socket::Disconnected) {
		wcout << L"Disconnected\n";
		exit(-1);
	}
	if (recv_result != sf::Socket::NotReady)
		if (received > 0) process_data(net_buf, received);

	for (int i = 0; i < SCREEN_WIDTH; ++i)
		for (int j = 0; j < SCREEN_HEIGHT; ++j)
		{
			int tile_x = i + g_left_x;
			int tile_y = j + g_top_y;
			if ((tile_x < 0) || (tile_y < 0)) continue;
			if (0 ==(tile_x /3 + tile_y /3) % 2) {
				white_tile.a_move(TILE_WIDTH * i, TILE_WIDTH * j);
				white_tile.a_draw();
			}
			else
			{
				black_tile.a_move(TILE_WIDTH * i, TILE_WIDTH * j);
				black_tile.a_draw();
			}
		}
	avatar.draw();
	for (auto& pl : players) pl.second.draw();
	sf::Text text;
	text.setFont(g_font);
	char buf[100];
	sprintf_s(buf, "(%d, %d)", avatar.m_x, avatar.m_y);
	text.setString(buf);
	g_window->draw(text);


	g_window->draw(mapRectangle);	// MAP 그리기
	playerDot.setPosition(MAP_WIDTH + (avatar.m_x / (2.1 * (W_WIDTH / 400))) , MAP_HEIGHT + (avatar.m_y / (2.1 * (W_HEIGHT / 400))) ); // 플레이어 위치 설정
	g_window->draw(playerDot);		// 플레이어 위치 나타내는 점 그리기

	// 채팅창 그리기
	float yOffset = 530;
	// 5개 이상의 채팅이 쌓이면 가장 오래된 채팅부터 지워줌
	if (chatHistory.size() > 5) chatHistory.erase(chatHistory.begin());
	for (auto it = chatHistory.rbegin(); it != chatHistory.rend() && yOffset > 0; ++it) 
	{
		sf::Text historyText(*it, g_font, 20);
		historyText.setFillColor(sf::Color::White);
		historyText.setPosition(5, yOffset);
		g_window->draw(historyText);
		yOffset -= 25;
	}
	if (isChatActive) 
	{
		chatText.setString(chatInput);
		g_window->draw(chatText);
		g_window->draw(chatBox);
	}
}

void send_packet(void *packet)
{
	unsigned char *p = reinterpret_cast<unsigned char *>(packet);
	size_t sent = 0;
	s_socket.send(packet, p[0], sent);
}

int main()
{
	wcout.imbue(locale("korean"));

	char SERVER_ADDR[BUF_SIZE];
	char PLAYER_ID[BUF_SIZE];
	//std::cout << "Enter IP Address : ";
	//std::cin.getline(SERVER_ADDR, BUF_SIZE);
	std::cout << "Enter Player ID : ";
	std::cin.getline(PLAYER_ID, BUF_SIZE);
	//sf::Socket::Status status = s_socket.connect(SERVER_ADDR, PORT_NUM);
	
	sf::Socket::Status status = s_socket.connect("127.0.0.1", PORT_NUM);
	s_socket.setBlocking(false);

	if (status == sf::Socket::Done)
	{
		std::cout << "Connect Success with : " << SERVER_ADDR << std::endl;
	}
	else {
		wcout << L"서버와 연결할 수 없습니다.\n";
		exit(-1);
	}

	client_initialize();
	CS_LOGIN_PACKET p;
	p.size = sizeof(p);
	p.type = CS_LOGIN;

	//string player_name{ "P" };
	//player_name += to_string(GetCurrentProcessId());
	//strcpy_s(p.name, player_name.c_str());

	strcpy_s(p.name, PLAYER_ID);
	send_packet(&p);


	avatar.set_name(p.name);

	sf::RenderWindow window(sf::VideoMode(WINDOW_WIDTH, WINDOW_HEIGHT), "2D CLIENT");
	g_window = &window;

	
	int attack_directioin = -1;
	while (window.isOpen())
	{
		sf::Event event;
		while (window.pollEvent(event))
		{
			if (event.type == sf::Event::Closed)
				window.close();
			if (event.type == sf::Event::KeyPressed) {
				int direction = -1;
				
				bool isAttacking = false;
				switch (event.key.code) {
				case sf::Keyboard::Left:
					direction = attack_directioin = 2;
					break;
				case sf::Keyboard::Right:
					direction = attack_directioin = 3;
					break;
				case sf::Keyboard::Up:
					direction = attack_directioin = 0;
					break;
				case sf::Keyboard::Down:
					direction = attack_directioin = 1;
					break;
				case sf::Keyboard::A:
					isAttacking = true;
					attack_directioin = 4;
					break;
				case sf::Keyboard::S:
					isAttacking = true;
					break;
				case sf::Keyboard::Escape:
					window.close();
					break;
				case sf::Keyboard::Enter:
					if (isChatActive) {
						
						// chatting packet
						CS_CHAT_PACKET p;
						p.size = sizeof(p);
						p.type = CS_CHAT;
						strcpy_s(p.mess, chatInput.c_str());
						send_packet(&p);

						// 머리위에 메세지 출력
						avatar.set_chat(p.mess);

						// 메세지 창에 저장
						chatInput.clear();
						std::string avatarName(avatar.name, strnlen(avatar.name, sizeof(avatar.name)));
						chatInput.append("[").append(avatarName).append("] : ").append(p.mess);
						chatHistory.push_back(chatInput);
						chatInput.clear();
						isChatActive = false;
					}
					else {
						isChatActive = true;
					}
					break;
				}
				// move packet
				if (direction != -1 && !isChatActive) {
					CS_MOVE_PACKET p;
					p.size = sizeof(p);
					p.type = CS_MOVE;
					p.direction = direction;
					send_packet(&p);
				}
				// attack packet
				if(attack_directioin != -1 && !isChatActive && isAttacking)
				{
					CS_ATTACK_PACKET p;
					p.size = sizeof(p);
					p.type = CS_ATTACK;
					p.attack_direction = attack_directioin;
					send_packet(&p);
					attack_directioin = -1;
				}

			}
			if (isChatActive && event.type == sf::Event::TextEntered) {
				if (event.text.unicode == '\b') {
					if (!chatInput.empty()) {
						chatInput.pop_back();
					}
				}
				else if (event.text.unicode < 128 && event.text.unicode != '\r') { // 엔터키 제외
					chatInput += static_cast<char>(event.text.unicode);
				}
			}
		}



		window.clear();
		client_main();
		window.display();
	}
	client_finish();

	return 0;
}