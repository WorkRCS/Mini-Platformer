#include "WildboundCore.h"
#include <cassert>
#include <iostream>
int main(){
 assert(wb_core_version()==1);wb_reset();WB_ActorState s{};wb_get_player(&s);
 assert(s.alive&&s.health==100);assert(wb_get_monster_count()==8);assert(wb_get_boss_count()==4);assert(wb_get_active_boss()==-1);
 wb_update(.016f,1,0,0,0,0,0);WB_ActorState m{};wb_get_player(&m);assert(m.x>s.x&&m.animationState==1);
 wb_update(.016f,0,0,1,0,0,0);WB_ActorState j{};wb_get_player(&j);assert(j.y>0&&j.animationState==3);
 wb_reset();wb_update(.016f,1,0,0,0,0,1);wb_get_player(&m);assert(m.animationState==2);
 std::cout<<"Wildbound native core tests passed\n";return 0;
}
