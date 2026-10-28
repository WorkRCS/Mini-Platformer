#include "WildboundCore.h"
#include <algorithm>
#include <array>
#include <cmath>
namespace {
constexpr int M=8, B=4;
constexpr float PI=3.14159265358979323846f;
enum Anim { Idle=0, Walk=1, Run=2, Jump=3, Attack=4, Hit=5, Dead=6 };
struct Actor {
 float x=0,y=0,z=0,yaw=0,health=100,maxHealth=100,vy=0;
 float speed=2,attackCooldown=0,attackClock=0,hitClock=0,abilityCooldown=0,abilityClock=0,phase=0;
 int alive=1,animation=Idle,kind=0;
};
Actor player; std::array<Actor,M> monsters; std::array<Actor,B> bosses;
int kills=0,xp=0,defeated=0;
float clampf(float x,float a,float b){return std::max(a,std::min(x,b));}
float dist(const Actor&a,const Actor&b){float x=a.x-b.x,z=a.z-b.z;return std::sqrt(x*x+z*z);}
void face(Actor&a,float x,float z){if(std::fabs(x)+std::fabs(z)>0.0001f)a.yaw=std::atan2(x,z)*180.0f/PI;}
bool unlocked(int i){return i==0?kills>=3:bosses[(size_t)i-1].alive==0;}
void resetActor(Actor&a,float x,float z,float hp,float speed,int kind){a=Actor{};a.x=x;a.z=z;a.health=a.maxHealth=hp;a.speed=speed;a.kind=kind;}
WB_ActorState pub(const Actor&a){return {a.x,a.y,a.z,a.yaw,a.health,a.maxHealth,a.alive,a.animation,a.kind};}
void damagePlayer(float v){if(!player.alive||player.hitClock>0)return;player.health=std::max(0.0f,player.health-v);player.hitClock=.35f;if(player.health<=0){player.alive=0;player.animation=Dead;}}
void killMonster(Actor&a){if(!a.alive)return;a.health=0;a.alive=0;a.animation=Dead;++kills;xp+=25;}
void killBoss(Actor&a,int i){if(!a.alive)return;a.health=0;a.alive=0;a.animation=Dead;++defeated;xp+=300+i*100;player.health=std::min(player.maxHealth,player.health+35);}
void attackTargets(){
 float fx=std::sin(player.yaw*PI/180),fz=std::cos(player.yaw*PI/180);
 for(auto&m:monsters){if(!m.alive||dist(player,m)>3.1f)continue;float dx=m.x-player.x,dz=m.z-player.z;if(dx*fx+dz*fz<-.35f)continue;m.health-=34;m.hitClock=.18f;if(m.health<=0)killMonster(m);}
 for(int i=0;i<B;i++){auto&b=bosses[(size_t)i];if(!b.alive||!unlocked(i)||dist(player,b)>4)continue;float dx=b.x-player.x,dz=b.z-player.z;if(dx*fx+dz*fz<-.35f)continue;b.health-=27;b.hitClock=.18f;if(b.health<=0)killBoss(b,i);}
}
void ability(){for(auto&m:monsters){if(!m.alive||dist(player,m)>5.2f)continue;m.health-=65;m.hitClock=.18f;if(m.health<=0)killMonster(m);}for(int i=0;i<B;i++){auto&b=bosses[(size_t)i];if(!b.alive||!unlocked(i)||dist(player,b)>5.8f)continue;b.health-=65;b.hitClock=.18f;if(b.health<=0)killBoss(b,i);}}
void updateEnemy(Actor&a,float dt,float atk,float damage){
 if(!a.alive){a.animation=Dead;return;}a.phase+=dt;a.hitClock=std::max(0.0f,a.hitClock-dt);a.attackCooldown=std::max(0.0f,a.attackCooldown-dt);
 float d=dist(a,player);bool moving=false;
 if(player.alive&&d<22&&d>atk){float dx=player.x-a.x,dz=player.z-a.z,len=std::max(.001f,std::sqrt(dx*dx+dz*dz));dx/=len;dz/=len;a.x+=dx*a.speed*dt;a.z+=dz*a.speed*dt;face(a,dx,dz);moving=true;}
 else if(d>=22){a.x+=std::sin(a.phase*.65f)*.25f*dt;a.z+=std::cos(a.phase*.9f)*.3f*dt;moving=true;}
 if(player.alive&&d<atk&&a.attackCooldown<=0){a.attackCooldown=1.45f;a.animation=Attack;damagePlayer(damage);}
 else if(a.hitClock>0)a.animation=Hit;else a.animation=moving?Walk:Idle;
}
}
extern "C" {
int wb_core_version(void){return 1;}
void wb_reset(void){
 resetActor(player,-6,0,100,7,0);
 for(int i=0;i<M;i++){float row=float(i/4),col=float(i%4);resetActor(monsters[(size_t)i],4+col*5,(row==0?-5:6)+(col-1.5f)*1.2f,48,2.1f+.08f*col,i%4);}
 resetActor(bosses[0],20,-12,180,2.2f,0);resetActor(bosses[1],42,13,260,2.45f,1);resetActor(bosses[2],64,-14,360,2.6f,2);resetActor(bosses[3],86,14,480,2.8f,3);
 kills=xp=defeated=0;
}
void wb_update(float dt,float mx,float mz,int jump,int attack,int skill,int sprint){
 if(!player.alive){player.animation=Dead;return;}dt=clampf(dt,0,.05f);
 player.attackCooldown=std::max(0.0f,player.attackCooldown-dt);player.attackClock=std::max(0.0f,player.attackClock-dt);
 player.abilityCooldown=std::max(0.0f,player.abilityCooldown-dt);player.abilityClock=std::max(0.0f,player.abilityClock-dt);player.hitClock=std::max(0.0f,player.hitClock-dt);
 float n=std::sqrt(mx*mx+mz*mz);if(n>1){mx/=n;mz/=n;}bool moving=n>.1f,running=moving&&sprint!=0;
 if(moving)face(player,mx,mz);float speed=running?10:6.6f;
 player.x=clampf(player.x+mx*speed*dt,-48,105);player.z=clampf(player.z+mz*speed*dt,-27,27);
 if(jump&&player.y<=.001f)player.vy=8.2f;player.vy-=22*dt;player.y+=player.vy*dt;if(player.y<=0){player.y=0;player.vy=0;}
 if(attack&&player.attackCooldown<=0){player.attackCooldown=.42f;player.attackClock=.28f;attackTargets();}
 if(skill&&player.abilityCooldown<=0){player.abilityCooldown=5;player.abilityClock=.72f;ability();}
 for(auto&m:monsters)updateEnemy(m,dt,1.8f,8);
 for(int i=0;i<B;i++){auto&b=bosses[(size_t)i];if(!b.alive){b.animation=Dead;continue;}b.phase+=dt;b.hitClock=std::max(0.0f,b.hitClock-dt);b.attackCooldown=std::max(0.0f,b.attackCooldown-dt);bool can=unlocked(i);float d=dist(b,player);bool movingBoss=false;
 if(can&&player.alive&&d<42&&d>3){float dx=player.x-b.x,dz=player.z-b.z,len=std::max(.001f,std::sqrt(dx*dx+dz*dz));dx/=len;dz/=len;b.x+=dx*b.speed*dt;b.z+=dz*b.speed*dt;face(b,dx,dz);movingBoss=true;}
 if(can&&player.alive&&d<3.4f&&b.attackCooldown<=0){b.attackCooldown=2.1f+i*.18f;b.animation=Attack;damagePlayer(14+i*3);}
 else if(b.hitClock>0)b.animation=Hit;else b.animation=movingBoss?Run:Idle;
 }
 if(!player.alive)player.animation=Dead;else if(player.y>.01f)player.animation=Jump;else if(player.hitClock>0)player.animation=Hit;else if(player.attackClock>0)player.animation=Attack;else if(running)player.animation=Run;else if(moving)player.animation=Walk;else player.animation=Idle;
}
void wb_get_player(WB_ActorState*out){if(out)*out=pub(player);}
int wb_get_monster_count(void){return M;}void wb_get_monster(int i,WB_ActorState*out){if(out&&i>=0&&i<M)*out=pub(monsters[(size_t)i]);}
int wb_get_boss_count(void){return B;}void wb_get_boss(int i,WB_ActorState*out){if(out&&i>=0&&i<B)*out=pub(bosses[(size_t)i]);}
int wb_get_kills(void){return kills;}int wb_get_xp(void){return xp;}int wb_get_defeated_bosses(void){return defeated;}
int wb_get_active_boss(void){for(int i=0;i<B;i++)if(bosses[(size_t)i].alive&&unlocked(i))return i;return -1;}
}
