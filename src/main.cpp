#include <bits/stdc++.h>
#include <sys/ioctl.h>
#include <thread>
#include <boost/algorithm/string/join.hpp>

//lib
//#include "vec.h"

using namespace std;

int SCREEN_WIDTH,SCREEN_HEIGHT;
chrono::steady_clock::time_point start_time;
chrono::steady_clock::time_point delta_time_clock;

const int threshold = 80000;

void getTerminalSize(int &x, int &y) {
    struct winsize w;
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
    y = w.ws_row;
    x = w.ws_col;
}

void build_line (int yb, int ye, vector<string>& buffer) {
    int lr=1,lg=1,lb=1;
    for(int y = yb; y < ye; y++){
        string &line = buffer[SCREEN_HEIGHT - 1 - y];
        line.clear();
        for(int x = 0; x < SCREEN_WIDTH; x++){
            int r = (int)((float)x/SCREEN_WIDTH*255);
            int g = (int)((float)y/SCREEN_HEIGHT*255);
            //int g2 = (int)((float)(y+0.5f)/SCREEN_HEIGHT*255);
            int b = (sin(std::chrono::duration_cast<std::chrono::milliseconds>(delta_time_clock-start_time).count()/1000.0)+1.0)*255/2;
            if(x==0){
                lr=r;lg=g;lb=b;
            }
            else{
                int dr = r - lr;
                int dg = g - lg;
                int db = b - lb;
                int rmean = (r + lr) >> 1;
                int dist2 = ((512 + rmean) * dr * dr) + (1024 * dg * dg) + ((512 + (255 - rmean)) * db * db);

                if(dist2<threshold){
                    line += "█";
                    continue;
                }
                else{
                    lr=r;lg=g;lb=b;
                }
            }
            line += "\x1b[38;2;"+to_string(r)+";"+to_string(g)+";"+to_string(b)+"m█";
        }
    }
};

int main(){
    std::cout << "\x1b[?25l"; //hide cursor

    string output;
    int cur_fps=0;
    chrono::steady_clock::time_point fps_timer = std::chrono::steady_clock::now();
    start_time = std::chrono::steady_clock::now();


    for(int cur_frame = 0;;cur_frame++){
        getTerminalSize(SCREEN_WIDTH, SCREEN_HEIGHT);
        delta_time_clock = std::chrono::steady_clock::now();
        int delta_time = std::chrono::duration_cast<std::chrono::milliseconds>(delta_time_clock - fps_timer).count();
        if(delta_time>=500){
            fps_timer = delta_time_clock;
            cur_fps = cur_frame*2;
            cur_frame = 0;
        }

        vector<string> buffer(SCREEN_HEIGHT);

        int CORES = 12;

        int renderheight = SCREEN_HEIGHT-1;
        int block_size = renderheight / CORES;

        vector<thread> threads;

        for(int y = 0; y < CORES; y++){
            threads.emplace_back(build_line, y*block_size, (y==CORES-1?renderheight:(y+1)*block_size), std::ref(buffer));
        }
        for(auto &t : threads) t.join();

        output.clear();
        output.reserve(SCREEN_WIDTH * (SCREEN_HEIGHT - 1) * 23 + 196); // guess a safe size

        output += "\x1b[H\x1b[?25l";
        output += "\x1b[39;49m" + to_string(SCREEN_WIDTH) + "x" + to_string(SCREEN_HEIGHT*2) + " " + to_string(cur_fps) + " \x1b[K\n";
        for(auto &i:buffer) output += i;
        cout << output;
        cout.flush();
    }

    return 0;
}