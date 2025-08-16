CREATE PROCEDURE sp_UserLogin
    @user_id NCHAR(20)
AS
BEGIN
    SET NOCOUNT ON;

    -- 기존 유저 확인
    IF EXISTS (SELECT 1 FROM user_term_table WHERE user_id = @user_id)
    BEGIN
        SELECT user_id, user_x, user_y, user_max_hp, user_exp, user_level, user_visual
        FROM user_term_table
        WHERE user_id = @user_id;
    END
    ELSE
    BEGIN
        -- 신규 유저일시 삽입 후 반환
        INSERT INTO user_term_table (user_id, user_x, user_y, user_max_hp, user_exp, user_level, user_visual)
        VALUES (@user_id, 0, 0, 100, 0, 1, 0);

        SELECT user_id, user_x, user_y, user_max_hp, user_exp, user_level, user_visual
        FROM user_term_table
        WHERE user_id = @user_id;
    END
END