myid = 99999;
sec = 3000;

function set_uid(x)
   myid = x;
end

function event_player_search(player)
   player_x, player_y = API_get_xy(player);
   my_x, my_y = API_get_xy(myid);
   -- �÷��̾ ��, ��, ��, �Ʒ� �� �ϳ��� �ִ� ��� ����
   if (player_x == my_x and (player_y == my_y - 1 or player_y == my_y + 1)) or 
        (player_y == my_y and (player_x == my_x - 1 or player_x == my_x + 1)) then
        API_Attack(myid, player)
   end
end

