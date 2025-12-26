#include "arkanoid_impl.h"
#include <random>
#include <GLFW/glfw3.h>
#include <string>

#ifdef USE_ARKANOID_IMPL
Arkanoid* create_arkanoid()
{
    return new ArkanoidImpl();
}
#endif

ArkanoidSettings settings;

int lives = 3;
bool game_over = false;
bool game_won = false;
int destroyed_bricks = 0;

std::vector<Bonus> active_bonuses;
float bonus_timer = 0.0f;
std::string bonus_active_type = "";
int bonus_mult = 2;


void ArkanoidImpl::reset(const ArkanoidSettings & new_settings)
{
    //general
    destroyed_bricks = 0;
    settings = new_settings;
    lives = 3;
    bonus_timer = 0.0f;
    bonus_active_type = "";
    game_over = false;
    game_won = false;
    active_bonuses.clear();

    //bricks
    bricks.clear();
    int columns = settings.bricks_columns_count;
    int rows = settings.bricks_rows_count;
    float pad_x = settings.bricks_columns_padding;
    float pad_y = settings.bricks_rows_padding;
    Vect size;
    size.x = (settings.world_size[0] - (columns + 1) * pad_x) / columns;
    size.y = size.x /2.5;
    int random_hp;
    std::string random_bonus;
    for (int row = 0; row < rows; ++row) {
        for (int col = 0; col < columns; ++col) {
            random_hp = 1;
            random_bonus = "";
            int random = rand() % 100;
            if (random < 15) random_hp= 3;
            else if (random < 40) random_hp= 2;
            if (random < 10)
            {
                if (random % 3 + 1 == 1) random_bonus = "wide";
                if (random % 3 + 1 == 2) random_bonus = "speed";
                if (random % 3 + 1 == 3) random_bonus = "bigball";
            }
            float x = pad_x + col * (size.x + pad_x) + size.x / 2.0f;
            float y = pad_y + row * (size.y + pad_y) + size.y / 2.0f;
            bricks.emplace_back(Vect(x, y), size, random_hp, random_bonus);
        }
    }

    //carriage
    carriage_position = Vect(settings.world_size[0] / 2, settings.world_size[1] - 50);
    carriage_size[0] = settings.carriage_width;
    if (carriage_size[0] < settings.carriage_width_min) carriage_size[0] = settings.carriage_width_min;
    carriage_size[1] = carriage_size[0] / 6;
    carriage_speed = settings.carriage_speed;

    //world
    world_size.x = settings.world_size[0];
    world_size.y = settings.world_size[1];

    //ball
    ball_position = world_size * 0.5f;
    ball_initial_speed = settings.ball_speed;
    ball_radius = settings.ball_radius;
    ball_velocity = Vect(ball_initial_speed);
}

void ArkanoidImpl::update(ImGuiIO& io, ArkanoidDebugData& debug_data, float elapsed)
{
    if (game_over || game_won)
        elapsed = 0.0f;

    world_to_screen = Vect(io.DisplaySize.x / world_size.x, io.DisplaySize.y / world_size.y);
    

    if (destroyed_bricks >= bricks.size())
        game_won = true;
    
    // update ball position according
    // its velocity and elapsed time
    ball_position += ball_velocity * elapsed;

    if (ball_position.x < ball_radius)
    {
        ball_position.x += (ball_radius - ball_position.x) * 2.0f;
        ball_velocity.x *= -1.0f;

        add_debug_hit(debug_data, Vect(0, ball_position.y), Vect(1, 0));
    }
    else if (ball_position.x > (world_size.x - ball_radius))
    {
        ball_position.x -= (ball_position.x - (world_size.x - ball_radius)) * 2.0f;
        ball_velocity.x *= -1.0f;

        add_debug_hit(debug_data, Vect(world_size.x, ball_position.y), Vect(-1, 0));
    }

    if (ball_position.y < ball_radius)
    {
        ball_position.y += (ball_radius - ball_position.y) * 2.0f;
        ball_velocity.y *= -1.0f;
        


        add_debug_hit(debug_data, Vect(ball_position.x, 0), Vect(0, 1));
    }
    else if (ball_position.y > (world_size.y - ball_radius))
    {
        //defeat
        lives--;
        ball_position = Vect(settings.world_size.x * 0.5f, settings.world_size.y * 0.6f);
        ball_velocity = Vect(0, -settings.ball_speed);
        if (lives <= 0) {
            game_over = true;
        }
        add_debug_hit(debug_data, Vect(ball_position.x, world_size.y), Vect(0, -1));
    }

    //key logic
    if (io.KeysDown[GLFW_KEY_A])
    {
        carriage_position.x -= carriage_speed * elapsed;
    }
    if (io.KeysDown[GLFW_KEY_D])
    {
        carriage_position.x += carriage_speed * elapsed;
    }
    if (io.KeysDown[GLFW_KEY_ESCAPE]) {
        reset(settings);
        
    }

    //world border for carriage
    if (carriage_position.x > world_size.x) carriage_position.x = world_size.x;
    else if (carriage_position.x < 0) carriage_position.x = 0;





    //ball to cariage collision
    float carriage_top = carriage_position.y - carriage_size.y * 0.5f;
    float carriage_left = carriage_position.x - carriage_size.x * 0.5f;
    float carriage_right = carriage_position.x + carriage_size.x * 0.5f;
    float ball_bottom = ball_position.y + ball_radius;

    bool vertically_colliding =
        ball_bottom >= carriage_top &&
        ball_position.y < carriage_position.y;

    bool horizontally_inside =
        ball_position.x + ball_radius >= carriage_left &&
        ball_position.x - ball_radius <= carriage_right;

    if (vertically_colliding && horizontally_inside)
    {
        ball_position.y = carriage_top - ball_radius;
        ball_velocity.y *= -1.0f;

        float hit_offset = ball_position.x - carriage_position.x;
        float influence = hit_offset / (carriage_size.x * 0.5f);
        ball_velocity.x += influence * ball_initial_speed;


        add_debug_hit(debug_data, carriage_position, Vect(0, -1));
    }
    ball_velocity = ball_velocity.Normalized() * ball_initial_speed;

    //bricks
    for (int i = 0; i < bricks.size(); i++)
    {
        Brick& b = bricks[i];
        if (b.check_collision(ball_position, ball_radius))
        {
            b.hp--;

            if (b.hp <= 0)
            {
                b.alive = false;
                destroyed_bricks++;
                if (b.bonus_type != "") {
                    active_bonuses.push_back(Bonus{ b.position, 60.0f, b.bonus_type });
                }

            }
            ball_velocity.y *= -1.0f;
            add_debug_hit(debug_data, b.position, Vect(0, 1));
            break;
        }
    }

   //bonus logic
    for (Bonus& bonus : active_bonuses)
        bonus.update(elapsed);

    for (Bonus& bonus : active_bonuses)
    {
        if (bonus.active && bonus.check_pickup(carriage_position, carriage_size)) {
            bonus.active = false;
            bonus_timer = 10.0f;
            bonus_active_type = bonus.type;

\
            if (bonus.type == "wide")
                carriage_size.x *= bonus_mult;
            else if (bonus.type == "speed")
                carriage_speed *= bonus_mult;
            else if (bonus.type == "bigball")
                ball_radius *= bonus_mult;
        }
    }

    if (bonus_timer > 0.0f) {
        bonus_timer -= elapsed;

        if (bonus_timer <= 0.0f) {

           
            bonus_active_type = "";

            carriage_size.x = settings.carriage_width;
            carriage_speed = settings.carriage_speed;
            ball_radius = settings.ball_radius;
        }
    }


}

void ArkanoidImpl::draw(ImGuiIO& io, ImDrawList &draw_list)
{
   

    //carriage
    Vect screen_carriage_pos = carriage_position * world_to_screen;
    Vect half_size = carriage_size * 0.5f * world_to_screen;
    ImVec2 a(screen_carriage_pos.x - half_size.x, screen_carriage_pos.y - half_size.y);
    ImVec2 b(screen_carriage_pos.x + half_size.x, screen_carriage_pos.y + half_size.y);
    draw_list.AddRectFilled(a, b, ImColor(84, 179, 224), 30.0f);

    //ball
    Vect screen_ball_pos = ball_position * world_to_screen;
    float screen_ball_radius = ball_radius * world_to_screen.x;
    draw_list.AddCircleFilled(screen_ball_pos, screen_ball_radius, ImColor(100, 255, 100));

    //bricks
    for (int i = 0; i < bricks.size(); i++)
    {
        Brick& b = bricks[i];
        b.draw(draw_list, world_to_screen);
    }
    std::string count_text = "Bricks destroyed: " + std::to_string(destroyed_bricks);
    ImVec2 pos(460,560);
    draw_list.AddText(nullptr, 40.0f, pos, ImColor(84, 179, 224), count_text.c_str());

    //win and defeat screen
    if (game_over) {
        ImVec2 center(io.DisplaySize.x * 0.5f, io.DisplaySize.y * 0.4f);
        draw_list.AddText(nullptr, 40.0f, center, ImColor(255, 80, 80), u8"Game over... press ESC");

    }
    else if (game_won) {
        ImVec2 center(io.DisplaySize.x * 0.5f, io.DisplaySize.y * 0.4f);
        draw_list.AddText(nullptr, 40.0f, center, ImColor(80, 255, 80), u8"You win!");
    }
    //hp
    for (int i = 0; i < 3; ++i)
    {
        ImU32 color = (i < lives) ? IM_COL32(255, 60, 60, 255) : IM_COL32(80, 80, 80, 100);
        float x = 1000 + i * (2 * 20 + 10);
        draw_list.AddCircleFilled(ImVec2(x, 650), 20, color);
    }

    //bonus
    for (const Bonus& b : active_bonuses)
        b.draw(draw_list, world_to_screen);


}




void ArkanoidImpl::add_debug_hit(ArkanoidDebugData& debug_data, const Vect& world_pos, const Vect& normal)
{
    ArkanoidDebugData::Hit hit;
    hit.screen_pos = world_pos * world_to_screen;
    hit.normal = normal;
    debug_data.hits.push_back(std::move(hit));
}

