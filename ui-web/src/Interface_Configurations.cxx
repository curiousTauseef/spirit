#include "Configurations.h"
#include "Interface_State.h"
#include "Interface_Configurations.h"

void Configuration_DomainWall(State *state, const double pos[3], double v[3], bool greater)
{
    Utility::Configurations::DomainWall(*state->c->images[state->c->active_image], pos, v, greater);
}

void Configuration_Homogeneous(State *state, double v[3])
{
    Utility::Configurations::Homogeneous(*state->c->images[state->c->active_image], v);
}

void Configuration_PlusZ(State *state)
{
    Utility::Configurations::PlusZ(*state->c->images[state->c->active_image]);
}

void Configuration_MinusZ(State *state)
{
    Utility::Configurations::MinusZ(*state->c->images[state->c->active_image]);
}

void Configuration_Random(State *state, bool external)
{
    Utility::Configurations::Random(*state->c->images[state->c->active_image]);
}

void Configuration_Skyrmion(State *state, std::vector<double> pos, double r, double speed, double order, bool upDown, bool achiral, bool rl, bool experimental)
{
    Utility::Configurations::Skyrmion(*state->c->images[state->c->active_image], pos, r, speed, order, upDown, achiral, rl, experimental);
}

void Configuration_SpinSpiral(State *state, std::string direction_type, double q[3], double axis[3], double theta)
{
    Utility::Configurations::SpinSpiral(*state->c->images[state->c->active_image], direction_type, q, axis, theta);
}