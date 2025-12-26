#pragma once

#include "arkanoid.h"
#include <string>

#define USE_ARKANOID_IMPL

class Brick
{
public:
    Vect position;
    Vect size;
    int hp = 0;
    std::string bonus_type;
    
    bool alive;

    Brick(Vect _pos, Vect _sz,  int _hp, std::string _bonus_type)
    {
        position = _pos;
        size = _sz;
        alive = true;
        hp = _hp;
        bonus_type = _bonus_type;
    }

    void draw(ImDrawList& draw_list, Vect world_to_screen) const {

        ImColor color;
        if (hp == 1) color = ImColor(164, 164, 164);
        if (hp == 2) color = ImColor(77, 77, 77);
        if (hp == 3) color = ImColor(32, 32, 32);
        if (bonus_type != "") color = ImColor(239, 50, 63);

        if (!alive) return;
        Vect screen_pos = position * world_to_screen;
        Vect half = size * 0.5f * world_to_screen;
        ImVec2 a(screen_pos.x - half.x, screen_pos.y - half.y);
        ImVec2 b(screen_pos.x + half.x, screen_pos.y + half.y);
        draw_list.AddRectFilled(a, b, color);
    }

    bool check_collision(const Vect& ball_pos, float ball_radius) {
        if (!alive) return false;
        float dx = std::max(std::abs(ball_pos.x - position.x) - size.x * 0.5f, 0.0f);
        float dy = std::max(std::abs(ball_pos.y - position.y) - size.y * 0.5f, 0.0f);
        float dist_sq = dx * dx + dy * dy;
        return dist_sq <= ball_radius * ball_radius;
    }
};



struct Bonus
{
    Vect position;
    float fall_speed = 60.0f;
    std::string type;   // "wide", "speed", "bigball"
    bool active = true;

    void update(float dt) {
        position.y += fall_speed * dt;
    }

    void draw(ImDrawList& draw_list, Vect to_screen) const {
        if (!active) return;
        Vect screen = position * to_screen;
        ImVec2 a(screen.x - 10.0f, screen.y - 15.0f);
        ImVec2 b(screen.x + 10.0f, screen.y + 15.0f);
        ImColor color = ImColor(100, 255, 255);
        draw_list.AddRectFilled(a, b, color, 2.0f);
    }

    bool check_pickup(const Vect& carriage_pos, const Vect& carriage_size) const {
        if (!active) return false;

        float half_w = carriage_size.x * 0.5f;
        float half_h = carriage_size.y * 0.5f;

        float left = carriage_pos.x - half_w;
        float right = carriage_pos.x + half_w;
        float top = carriage_pos.y - half_h;

        return (position.x >= left && position.x <= right &&
            position.y >= top); 
    }
};


class ArkanoidImpl : public Arkanoid
{
public:
    void reset(const ArkanoidSettings& settings) override;
    void update(ImGuiIO& io, ArkanoidDebugData& debug_data, float elapsed) override;
    void draw(ImGuiIO& io, ImDrawList& draw_list) override;

private:
    
    void add_debug_hit(ArkanoidDebugData& debug_data, const Vect& pos, const Vect& normal);
    
    Vect world_size = Vect(0.0f);
    Vect world_to_screen = Vect(0.0f);

    Vect ball_position = Vect(0.0f);
    Vect ball_velocity = Vect(0.0f);
    float ball_radius = 0.0f;
    float ball_initial_speed = 0.0f;

    Vect carriage_position = Vect(0.0f);
    Vect carriage_size = Vect(0.0f);
    float carriage_speed = 0.0f;

    Vect brick_size = Vect(0.0f);
    int rows = 0;
    int coloumns = 0;
    std::vector<Brick> bricks;
};


