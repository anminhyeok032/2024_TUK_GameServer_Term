myid = 99999;
sec = 3000;

function set_uid(x)
   myid = x;
end

function event_player_search(player)
   player_x, player_y = API_get_xy(player);
   my_x, my_y = API_get_xy(myid);
   -- 플레이어가 좌, 우, 위, 아래 중 하나에 있는 경우 공격
   if (player_x == my_x and (player_y == my_y - 1 or player_y == my_y + 1)) or 
        (player_y == my_y and (player_x == my_x - 1 or player_x == my_x + 1)) then
        API_Attack(myid, player)
   end
end

