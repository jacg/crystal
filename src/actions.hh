#pragma once

#include "config.hh"

#include <n4-mandatory.hh>

auto my_generator(const config& my);

n4::actions* create_actions(const config& my, unsigned& n_event);
