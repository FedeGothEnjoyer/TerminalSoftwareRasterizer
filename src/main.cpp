#include <bits/stdc++.h>
#include <sys/ioctl.h>

//lib
#include "vec.h"
#include "rgb.h"
#include "img.h"

using namespace std;

constexpr float FP_EPSILON = 1e-6f;
constexpr int CORES = 12;

float2 A={15,5},B={60, 50},C={80, 25},D={150,15};



int SCREEN_WIDTH,SCREEN_HEIGHT;
chrono::steady_clock::time_point start_time;
chrono::steady_clock::time_point delta_time_clock;

const int threshold = 80000;

array<binary_semaphore,CORES>semaphore_full=[]<size_t...Is>(index_sequence<Is...>){return array<binary_semaphore,sizeof...(Is)>{((void)Is,binary_semaphore{0})...};}(make_index_sequence<CORES>());
array<binary_semaphore,CORES>semaphore_empty=[]<size_t...Is>(index_sequence<Is...>){return array<binary_semaphore,sizeof...(Is)>{((void)Is,binary_semaphore{0})...};}(make_index_sequence<CORES>());

void getTerminalSize(int &x, int &y) {
    struct winsize w;
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
    y = w.ws_row;
    x = w.ws_col;
}

inline float PointIsOnRightSideOfLine(float2 a, float2 b, float2 p){ // res<0 == true
    b-=a;
    p-=a;
    return (b.x*p.y) - (p.x*b.y);
}

inline bool EdgeIsTopLeft(const float2 &a, const float2 &b) {
    float dx = b.x - a.x;
    float dy = b.y - a.y;
    return (dy > 0.0f) || (fabsf(dy) < FP_EPSILON && dx < 0.0f);
}

inline bool PointIsInsideTriangle(float2 a, float2 b, float2 c, float2 p){
    float ab = PointIsOnRightSideOfLine(a, b, p);
    float bc = PointIsOnRightSideOfLine(b, c, p);
    float ca = PointIsOnRightSideOfLine(c, a, p);

    return (ab<0||(EdgeIsTopLeft(a,b)&&ab==0)) &&
           (bc<0||(EdgeIsTopLeft(b,c)&&bc==0)) &&
           (ca<0||(EdgeIsTopLeft(c,a)&&ca==0));
}


void build_line (int yb, int ye, vector<string>& buffer, int id) {
    for(;;){
        semaphore_empty[id].acquire();

        const int sw = SCREEN_WIDTH;
        color last_pixel, last_pixel2;

        for(int y = yb; y < ye; ++y){
            string &line = buffer[buffer.size() - 1 - y];
            line.clear();

            line.append("\x1b[38;2;1;1;1m\x1b[48;2;1;1;1m");

            char numbuf[16];
            for(int x = 0; x < sw; ++x){
                color pixel(1,1,1), pixel2(1,1,1);

                if(PointIsInsideTriangle(A, B, C, {(float)x+0.5f,(float)y+0.25f})){
                    pixel = color(1,1,255);
                }
                if(PointIsInsideTriangle(A, B, C, {(float)x+0.5f,(float)y+0.75f})){
                    pixel2 = color(1,1,255);
                }
                if(PointIsInsideTriangle(B, D, C, {(float)x+0.5f,(float)y+0.25f})){
                    pixel = color(255,1,1);
                }
                if(PointIsInsideTriangle(B, D, C, {(float)x+0.5f,(float)y+0.75f})){
                    pixel2 = color(255,1,1);
                }
                if(PointIsInsideTriangle(A, C, D, {(float)x+0.5f,(float)y+0.25f})){
                    pixel = color(1,255,1);
                }
                if(PointIsInsideTriangle(A, C, D, {(float)x+0.5f,(float)y+0.75f})){
                    pixel2 = color(1,255,1);
                }
                

                if(x == 0){
                    last_pixel = pixel;
                    last_pixel2 = pixel2;
                } else {
                    int dist_sq  = ColorDifferenceSquared(pixel, last_pixel);
                    int dist_sq2 = ColorDifferenceSquared(pixel2, last_pixel2);

                    if(dist_sq > threshold){
                        line.append("\x1b[38;2;");
                        auto res = std::to_chars(numbuf, numbuf + sizeof(numbuf), pixel.r);
                        line.append(numbuf, res.ptr - numbuf);
                        line.push_back(';');
                        res = std::to_chars(numbuf, numbuf + sizeof(numbuf), pixel.g);
                        line.append(numbuf, res.ptr - numbuf);
                        line.push_back(';');
                        res = std::to_chars(numbuf, numbuf + sizeof(numbuf), pixel.b);
                        line.append(numbuf, res.ptr - numbuf);
                        line.append("m");
                        last_pixel = pixel;
                    }
                    if(dist_sq2 > threshold){
                        line.append("\x1b[48;2;");
                        auto res = std::to_chars(numbuf, numbuf + sizeof(numbuf), pixel2.r);
                        line.append(numbuf, res.ptr - numbuf);
                        line.push_back(';');
                        res = std::to_chars(numbuf, numbuf + sizeof(numbuf), pixel2.g);
                        line.append(numbuf, res.ptr - numbuf);
                        line.push_back(';');
                        res = std::to_chars(numbuf, numbuf + sizeof(numbuf), pixel2.b);
                        line.append(numbuf, res.ptr - numbuf);
                        line.append("m");
                        last_pixel2 = pixel2;
                    }
                }

                line+="▄";
            } // x
        } // y

        semaphore_full[id].release();
    }
}

int main(){
    ios::sync_with_stdio(false);
    cout << "\x1b[?25l"; //hide cursor

    getTerminalSize(SCREEN_WIDTH, SCREEN_HEIGHT);

    string output;
    int cur_fps=0;
    chrono::steady_clock::time_point fps_timer = std::chrono::steady_clock::now();
    start_time = std::chrono::steady_clock::now();

    int renderheight = SCREEN_HEIGHT-1;
    int block_size = renderheight / CORES;

    vector<string> buffer(SCREEN_HEIGHT);
    for(auto &line:buffer) line.reserve(SCREEN_WIDTH * 34 + 32);
    array<thread,CORES> threads;

    output.reserve(SCREEN_WIDTH * (SCREEN_HEIGHT - 1) * 34 + 196);

    for(int y = 0; y < CORES; y++){
        threads[y] = thread(build_line, y*block_size, (y==CORES-1?renderheight:(y+1)*block_size), std::ref(buffer), y);
    }


    for(int cur_frame = 0;;cur_frame++){
        delta_time_clock = std::chrono::steady_clock::now();
        if(cur_frame==1024){
            int delta_time = std::chrono::duration_cast<std::chrono::milliseconds>(delta_time_clock - fps_timer).count();
            fps_timer = delta_time_clock;
            cur_fps = 1024/(float)delta_time*1000;
            cur_frame=0;
        }

        ////////////////////////////////////////////


        C.x = 70.0f+(sin(std::chrono::duration_cast<std::chrono::milliseconds>(delta_time_clock - start_time).count()/100.0f))*20.0f;
        C.y = 25.0f+(cos(std::chrono::duration_cast<std::chrono::milliseconds>(delta_time_clock - start_time).count()/100.0f))*10.0f;


        ////////////////////////////////////////////

        for(auto &s:semaphore_empty) s.release();

        
        output.clear();

        output += "\x1b[H\x1b[?25l";
        output += "\x1b[39;49m" + to_string(SCREEN_WIDTH) + "x" + to_string(SCREEN_HEIGHT*2) + " " + to_string(cur_fps) + "\x1b[K\n";

        for(auto &s:semaphore_full) s.acquire();

        for(auto &i:buffer) output += i;
        cout << output;
        cout.flush();
    }

    return 0;
}