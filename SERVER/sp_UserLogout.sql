CREATE PROCEDURE sp_UserLogout
    @user_id   NVARCHAR(50),
    @user_x    INT,
    @user_y    INT,
    @user_max_hp INT,
    @user_exp  INT,
    @user_level INT
AS
BEGIN
    SET NOCOUNT ON;

    UPDATE user_term_table
    SET 
        user_x = @user_x,
        user_y = @user_y,
        user_max_hp = @user_max_hp,
        user_exp = @user_exp,
        user_level = @user_level
    WHERE user_id = @user_id;
END