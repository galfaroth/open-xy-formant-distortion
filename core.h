#pragma once
#include <opxy/effect.hpp>
namespace formantdist {
struct State {
 opxy::i32 band[2][8][2], envelope[8], gain[8], control[8];
 opxy::i32 input[2][2], low[2], detector;
 opxy::u32 phase, cursor, written, started;
};
void initialize(State&);
void process(State&, opxy::Buffer, opxy::Channel, opxy::Channel, opxy::Parameters);
}
