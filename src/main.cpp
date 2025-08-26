#include <bits/stdc++.h>
#include <sys/ioctl.h>

//lib
#include <rgb.h>
#include <img.h>
#include <OBJ_Loader.h>

//glm
#define GLM_FORCE_AVX2
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

using namespace std;

constexpr float FP_EPSILON = 1e-6f;
constexpr int CORES = 12;

objl::Mesh mesh;
glm::vec4 A={0.0f,0.0f,0.0f,1.0f},B={0.0f,1.0f,0.0f,1.0f},C={1.0f,1.0f,0.0f,1.0f},D={1.0f,0.0f,0.0f,1.0f};
glm::vec4 A2,B2,C2,D2;

Image sbovo("../data/sbovo.png");

int SCREEN_WIDTH,SCREEN_HEIGHT;
chrono::steady_clock::time_point start_time;
chrono::steady_clock::time_point delta_time_clock;

const int threshold = 0;

array<binary_semaphore,CORES>semaphore_full=[]<size_t...Is>(index_sequence<Is...>){return array<binary_semaphore,sizeof...(Is)>{((void)Is,binary_semaphore{0})...};}(make_index_sequence<CORES>());
array<binary_semaphore,CORES>semaphore_empty=[]<size_t...Is>(index_sequence<Is...>){return array<binary_semaphore,sizeof...(Is)>{((void)Is,binary_semaphore{0})...};}(make_index_sequence<CORES>());

void getTerminalSize(int &x, int &y) {
    struct winsize w;
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
    y = w.ws_row;
    x = w.ws_col;
}

inline float PointIsOnRightSideOfLine(glm::vec2 a, glm::vec2 b, glm::vec2 p){ // res<0 == true
    b-=a;
    p-=a;
    return (b.x*p.y) - (p.x*b.y);
}

inline bool EdgeIsTopLeft(const glm::vec2 &a, const glm::vec2 &b) {
    float dx = b.x - a.x;
    float dy = b.y - a.y;
    return (dy > 0.0f) || (fabsf(dy) < FP_EPSILON && dx < 0.0f);
}

inline bool PointIsInsideTriangle(glm::vec2 a, glm::vec2 b, glm::vec2 c, glm::vec2 p){
    float ab = PointIsOnRightSideOfLine(a, b, p);
    float bc = PointIsOnRightSideOfLine(b, c, p);
    float ca = PointIsOnRightSideOfLine(c, a, p);

    return (ab<0||(EdgeIsTopLeft(a,b)&&ab==0)) &&
           (bc<0||(EdgeIsTopLeft(b,c)&&bc==0)) &&
           (ca<0||(EdgeIsTopLeft(c,a)&&ca==0));
}

inline float AreaDouble(glm::vec2 a, glm::vec2 b, glm::vec2 c){
    return fabs(a.x*(b.y-c.y)+b.x*(c.y-a.y)+c.x*(a.y-b.y));
}

inline glm::vec2 PerspectiveUV(glm::vec3 b, glm::vec3 w, glm::vec2 pA, glm::vec2 pB, glm::vec2 pC) {
    glm::vec3 num = b/w;
    float denom = num.x + num.y + num.z;
    glm::vec3 res = num/denom;
    return glm::vec2(res.x*pA.x+res.y*pB.x+res.z*pC.x,res.x*pA.y+res.y*pB.y+res.z*pC.y);
}

inline color CalculateFragment(float x, float y){
    if(PointIsInsideTriangle(A2, B2, C2, {x,y})){
        float ar = AreaDouble(A2, B2, C2);
        float b0 = AreaDouble({x,y}, B2, C2) / ar;
        float b1 = AreaDouble(A2, {x,y}, C2) / ar;
        float b2 = AreaDouble(A2, B2, {x,y}) / ar;
        glm::vec2 uvCord = PerspectiveUV({b0,b1,b2},{A2.w,B2.w,C2.w},A,B,C);
        return sbovo.Sample(uvCord.x, uvCord.y);
    }
    if(PointIsInsideTriangle(A2, C2, D2, {x,y})){
        float ar = AreaDouble(A2, C2, D2);
        float b0 = AreaDouble({x,y}, C2, D2) / ar;
        float b1 = AreaDouble(A2, {x,y}, D2) / ar;
        float b2 = AreaDouble(A2, C2, {x,y}) / ar;
        glm::vec2 uvCord = PerspectiveUV({b0,b1,b2},{A2.w,C2.w,D2.w},A,C,D);
        return sbovo.Sample(uvCord.x, uvCord.y);
    }
    return color(0,0,0);
}


void build_line (int yb, int ye, vector<string>& buffer, int id) {
    for(;;){
        semaphore_empty[id].acquire();

        const int sw = SCREEN_WIDTH, sh = SCREEN_HEIGHT;
        color last_pixel(0,0,0), last_pixel2(0,0,0);
        

        for(int screen_y = yb; screen_y < ye; screen_y++){
            string &line = buffer[buffer.size() - 1 - screen_y];
            line.clear();

            char numbuf[16];
            for(int screen_x = 0; screen_x < sw; screen_x++){
                color pixel(1,1,1), pixel2(1,1,1);
                float x = (screen_x+0.5f)*2/sw-1.0f, y = (screen_y+0.25f)*2/sh-1.0f, y2 = (screen_y+0.75f)*2/sh-1.0f;
                
                pixel = CalculateFragment(x, y).Clamp();
                pixel2 = CalculateFragment(x, y2).Clamp();

                if(screen_x == 0){
                    last_pixel = pixel;
                    last_pixel2 = pixel2;
                    line.append("\x1b[38;2;");
                    auto res = std::to_chars(numbuf, numbuf + sizeof(numbuf), (int)(max(1.0f,pixel.r*255)));
                    line.append(numbuf, res.ptr - numbuf);
                    line.push_back(';');
                    res = std::to_chars(numbuf, numbuf + sizeof(numbuf), (int)(max(1.0f,pixel.g*255)));
                    line.append(numbuf, res.ptr - numbuf);
                    line.push_back(';');
                    res = std::to_chars(numbuf, numbuf + sizeof(numbuf), (int)(max(1.0f,pixel.b*255)));
                    line.append(numbuf, res.ptr - numbuf);
                    line.append("m");
                    last_pixel = pixel;
                    line.append("\x1b[48;2;");
                    res = std::to_chars(numbuf, numbuf + sizeof(numbuf), (int)(max(1.0f,pixel2.r*255)));
                    line.append(numbuf, res.ptr - numbuf);
                    line.push_back(';');
                    res = std::to_chars(numbuf, numbuf + sizeof(numbuf), (int)(max(1.0f,pixel2.g*255)));
                    line.append(numbuf, res.ptr - numbuf);
                    line.push_back(';');
                    res = std::to_chars(numbuf, numbuf + sizeof(numbuf), (int)(max(1.0f,pixel2.b*255)));
                    line.append(numbuf, res.ptr - numbuf);
                    line.append("m");
                } else {
                    int dist_sq  = ColorDifferenceSquared(pixel, last_pixel);
                    int dist_sq2 = ColorDifferenceSquared(pixel2, last_pixel2);

                    if(dist_sq > threshold){
                        line.append("\x1b[38;2;");
                        auto res = std::to_chars(numbuf, numbuf + sizeof(numbuf), (int)(max(1.0f,pixel.r*255)));
                        line.append(numbuf, res.ptr - numbuf);
                        line.push_back(';');
                        res = std::to_chars(numbuf, numbuf + sizeof(numbuf), (int)(max(1.0f,pixel.g*255)));
                        line.append(numbuf, res.ptr - numbuf);
                        line.push_back(';');
                        res = std::to_chars(numbuf, numbuf + sizeof(numbuf), (int)(max(1.0f,pixel.b*255)));
                        line.append(numbuf, res.ptr - numbuf);
                        line.append("m");
                        last_pixel = pixel;
                    }
                    if(dist_sq2 > threshold){
                        line.append("\x1b[48;2;");
                        auto res = std::to_chars(numbuf, numbuf + sizeof(numbuf), (int)(max(1.0f,pixel2.r*255)));
                        line.append(numbuf, res.ptr - numbuf);
                        line.push_back(';');
                        res = std::to_chars(numbuf, numbuf + sizeof(numbuf), (int)(max(1.0f,pixel2.g*255)));
                        line.append(numbuf, res.ptr - numbuf);
                        line.push_back(';');
                        res = std::to_chars(numbuf, numbuf + sizeof(numbuf), (int)(max(1.0f,pixel2.b*255)));
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

    glm::mat4 proj = glm::perspective(glm::radians(45.0f), (float)SCREEN_WIDTH/((float)SCREEN_HEIGHT*2), 0.1f, 100.0f);
    glm::mat4 model = glm::mat4(1.0f);
    model = glm::rotate(model, glm::radians(-55.0f), glm::vec3(1.0f, 0.0f, 0.0f));
    glm::mat4 view = glm::mat4(1.0f);
    view = glm::translate(view, glm::vec3(0.0f, 0.0f, -3.0f)); 

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

    //         AREA CAZZEGGIO
    ////////////////////////////////


    objl::Loader meshLoader;
    if(!meshLoader.LoadFile("../data/cube.obj")) return -1;
    mesh = meshLoader.LoadedMeshes[0];


    ////////////////////////////////

    for(int cur_frame = 0;;cur_frame++){
        delta_time_clock = std::chrono::steady_clock::now();
        if(cur_frame==512){
            int delta_time = std::chrono::duration_cast<std::chrono::milliseconds>(delta_time_clock - fps_timer).count();
            fps_timer = delta_time_clock;
            cur_fps = 512/(float)delta_time*1000;
            cur_frame=0;
        }

        //            UPDATE LOOP
        ////////////////////////////////////////////

        std::chrono::duration<float> curTime = delta_time_clock - start_time;







        ////////////////////////////////////////////
        //        VERTEX MANIPULATION

        model = glm::mat4(1.0f);
        model = glm::scale(model, glm::vec3(2,2,1.0f));
        model = glm::rotate(model, glm::radians(-55.0f+curTime.count()*100), glm::vec3(0.0f, 1.0f, 0.0f));
        model = glm::translate(model, glm::vec3(-0.5f, -0.5f, 0.0f));
        A2=proj*view*model*A;
        B2=proj*view*model*B;
        C2=proj*view*model*C;
        D2=proj*view*model*D;
        A2.x/=A2.w;A2.y/=A2.w;A2.z/=A2.w;
        B2.x/=B2.w;B2.y/=B2.w;B2.z/=B2.w;
        C2.x/=C2.w;C2.y/=C2.w;C2.z/=C2.w;
        D2.x/=D2.w;D2.y/=D2.w;D2.z/=D2.w;


        ////////////////////////////////////////////
        //           RENDERING

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