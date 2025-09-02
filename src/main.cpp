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
constexpr int MAX_VBO_SIZE = 1000002;
constexpr float NEAR_PLANE = 0.1f;
constexpr float FAR_PLANE = 100.0f;
const int threshold = 0;

struct vertex{
    glm::vec4 position;
    glm::vec3 normal;
    glm::vec2 uv;
    vertex() : position(0,0,0,1), normal(0,0,0), uv(0,0) {}
    vertex(glm::vec4 _position) : position(_position), normal(0,0,1), uv(0,0) {}
    vertex(glm::vec3 _position, glm::vec2 _uv) : position(glm::vec4(_position,1.0f)), normal(0,0,0), uv(_uv) {}
    vertex(glm::vec3 _position, glm::vec3 _normal, glm::vec2 _uv) : position(glm::vec4(_position,1.0f)), normal(_normal), uv(_uv) {}
};

struct fragment{
    color col;
    float z;
};

vector<vertex>VBO;
int VBO_size = 0;

vector<vector<color>>FRAME_BUFFER;
vector<vector<float>>Z_BUFFER;

objl::Mesh mesh;

Image sbovo("../data/material16.png");

int SCREEN_WIDTH,SCREEN_HEIGHT;
chrono::steady_clock::time_point start_time;
chrono::steady_clock::time_point delta_time_clock;

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

    return (ab<0||(EdgeIsTopLeft(a,b)&&ab<FP_EPSILON)) &&
           (bc<0||(EdgeIsTopLeft(b,c)&&bc<FP_EPSILON)) &&
           (ca<0||(EdgeIsTopLeft(c,a)&&ca<FP_EPSILON));
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

inline glm::vec3 PerspectiveNormal(glm::vec3 b, glm::vec3 w, glm::vec3 nA, glm::vec3 nB, glm::vec3 nC) {
    glm::vec3 num = b/w;
    float denom = num.x + num.y + num.z;
    glm::vec3 res = num/denom;
    return glm::vec3(res.x*nA.x+res.y*nB.x+res.z*nC.x,res.x*nA.y+res.y*nB.y+res.z*nC.y,res.x*nA.z+res.y*nB.z+res.z*nC.z);
}

inline float Linear01FromNDC(float z_ndc, float n, float f) {
    return (n * (1.0f + z_ndc)) / (f + n - z_ndc * (f - n));
}

inline optional<fragment> CalculateFragment(float x, float y, vertex v1, vertex v2, vertex v3){
    if(PointIsInsideTriangle(v1.position, v2.position, v3.position, {x,y})){
        float ar = AreaDouble(v1.position, v2.position, v3.position);
        float b0 = AreaDouble({x,y}, v2.position, v3.position) / ar;
        float b1 = AreaDouble(v1.position, {x,y}, v3.position) / ar;
        float b2 = AreaDouble(v1.position, v2.position, {x,y}) / ar;
        float z = b0*v1.position.z + b1*v2.position.z + b2*v3.position.z;

        //LINEARIZED DEPTH SHADING
        //float colz = 1-Linear01FromNDC(z, NEAR_PLANE, FAR_PLANE);
        //return fragment{{colz,0,0},z};

        //SMOOTH NORMAL SHADING
        auto v = glm::normalize(PerspectiveNormal({b0,b1,b2},{v1.position.w,v2.position.w,v3.position.w},v1.normal, v2.normal, v3.normal));
        //return fragment{{v.x/2+0.5f,v.y/2+0.5f,1},z};

        //FLAT NORMAL SHADING
        //return fragment{{v1.normal.x/2+0.5f,v1.normal.y/2+0.5f,1},z};

        glm::vec2 uvCord = PerspectiveUV({b0,b1,b2},{v1.position.w,v2.position.w,v3.position.w},v1.uv, v2.uv, v3.uv);
        return fragment{sbovo.Sample(uvCord.x, uvCord.y)*(v.x+0.5f),z};
    }
    return nullopt;
}


void build_line (int yb, int ye, vector<string>& buffer, int id) {
    for(;;){
        semaphore_empty[id].acquire();

        const int sw = SCREEN_WIDTH;// sh = SCREEN_HEIGHT;
        color last_pixel(0,0,0), last_pixel2(0,0,0);
        

        for(int screen_y = yb; screen_y < ye; screen_y++){
            string &line = buffer[buffer.size() - 1 - screen_y];
            line.clear();

            char numbuf[16];
            for(int screen_x = 0; screen_x < sw; screen_x++){
                color pixel = FRAME_BUFFER[screen_x][screen_y*2].Clamp();
                color pixel2 = FRAME_BUFFER[screen_x][screen_y*2+1].Clamp();

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

    VBO=vector<vertex>(MAX_VBO_SIZE);

    getTerminalSize(SCREEN_WIDTH, SCREEN_HEIGHT);

    Z_BUFFER = vector<vector<float>>(SCREEN_WIDTH,vector<float>(SCREEN_HEIGHT*2,0));
    FRAME_BUFFER = vector<vector<color>>(SCREEN_WIDTH,vector<color>(SCREEN_HEIGHT*2,{0,0,0}));
    
    glm::mat4 proj = glm::perspective(glm::radians(45.0f), (float)SCREEN_WIDTH/((float)SCREEN_HEIGHT*2), NEAR_PLANE, FAR_PLANE);
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
    if(!meshLoader.LoadFile("../data/st_bunny.obj")) return -1;
    mesh = meshLoader.LoadedMeshes[0];


    ////////////////////////////////

    for(int cur_frame = 0;;cur_frame++){
        delta_time_clock = std::chrono::steady_clock::now();
        if(cur_frame==60){
            int delta_time = std::chrono::duration_cast<std::chrono::milliseconds>(delta_time_clock - fps_timer).count();
            fps_timer = delta_time_clock;
            cur_fps = 60/(float)delta_time*1000;
            cur_frame=0;
        }

        //            UPDATE LOOP
        ////////////////////////////////////////////

        std::chrono::duration<float> curTime = delta_time_clock - start_time;


        ////////////////////////////////////////////
        //        VERTEX SHADER

        VBO_size = 0;
        for(auto &i:mesh.Indices){
            VBO[VBO_size] = {{mesh.Vertices[i].Position.Z,mesh.Vertices[i].Position.Y,mesh.Vertices[i].Position.X},
                             {mesh.Vertices[i].Normal.Z,mesh.Vertices[i].Normal.Y,mesh.Vertices[i].Normal.X},
                             {mesh.Vertices[i].TextureCoordinate.X, mesh.Vertices[i].TextureCoordinate.Y}};
            VBO_size++;
        }

        model = glm::mat4(1.0f);

        //preset for stanford dragon
        // model = glm::translate(model, glm::vec3(0.0f, -15.0f, -30.0f));
        // model = glm::scale(model, glm::vec3(.1f,.1f,.1f));
        // model = glm::rotate(model, glm::radians(-55.0f+curTime.count()*100), glm::vec3(0.0f, 1.0f, 0.0f));

        //preset for teapot
        // model = glm::translate(model, glm::vec3(0.0f, 0.0f, -10.0f));
        // model = glm::scale(model, glm::vec3(1,1,1));
        // model = glm::rotate(model, glm::radians(-55.0f+curTime.count()*200), glm::vec3(0.0f, 1.0f, 0.0f));

        //preset for low poly bunny
        // model = glm::translate(model, glm::vec3(0.0f, -6.0f+fabs(sin(curTime.count()*5))*2, -20.0f));
        // model = glm::scale(model, glm::vec3(.1f,.1f,.1f));
        // model = glm::rotate(model, glm::radians(-55.0f+curTime.count()*200), glm::vec3(0.0f, 1.0f, 0.0f));
        // model = glm::translate(model, glm::vec3(0.0f, 0.0f, -25.0f));
        // model = glm::rotate(model, glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));

        //preset for actual stanford bunny
        model = glm::translate(model, glm::vec3(0.0f, -13.0f, -40.0f));
        model = glm::scale(model, glm::vec3(.1f,.1f,.1f));
        model = glm::rotate(model, glm::radians(-55.0f+curTime.count()*100), glm::vec3(0.0f, 1.0f, 0.0f));

        glm::mat4 modelview = view * model;
        glm::mat3 normalMatrix = glm::transpose(glm::inverse(glm::mat3(modelview)));

        for(int i = 0; i < VBO_size; i++){
            VBO[i].position=proj*view*model*VBO[i].position;
            VBO[i].position.x/=VBO[i].position.w;
            VBO[i].position.y/=VBO[i].position.w;
            VBO[i].position.z/=VBO[i].position.w;
            VBO[i].normal=normalMatrix*VBO[i].normal;
        }


        ////////////////////////////////////////////
        //           RENDERING

        for (auto &col : Z_BUFFER)
            std::fill(col.begin(), col.end(), 1);
        for (auto &col : FRAME_BUFFER)
            std::fill(col.begin(), col.end(), color());


        for(int i = 0; i < VBO_size; i+=3){
            auto v1=VBO[i],v2=VBO[i+1],v3=VBO[i+2];

            if(v1.position.z<0||v2.position.z<0||v3.position.z<0) continue;

            int minX = max(0,(int)floor((min({v1.position.x, v2.position.x, v3.position.x})/2+0.5f)*SCREEN_WIDTH));
            int maxX = min(SCREEN_WIDTH-1,(int)ceil((max({v1.position.x, v2.position.x, v3.position.x})/2+0.5f)*SCREEN_WIDTH));
            int minY = max(0,(int)floor((min({v1.position.y, v2.position.y, v3.position.y})/2+0.5f)*SCREEN_HEIGHT*2));
            int maxY = min(SCREEN_HEIGHT*2-1,(int)ceil((max({v1.position.y, v2.position.y, v3.position.y})/2+0.5f)*SCREEN_HEIGHT*2));

            for(int screen_y = minY; screen_y <= maxY; screen_y++){
                for(int screen_x = minX; screen_x <= maxX; screen_x++){
                    float x = (screen_x+0.5f)*2/SCREEN_WIDTH-1.0f, y = (screen_y+0.5f)/SCREEN_HEIGHT-1.0f;
                    auto frag = CalculateFragment(x,y,v1,v2,v3);
                    if(frag){
                        if(frag->z<Z_BUFFER[screen_x][screen_y]){
                            Z_BUFFER[screen_x][screen_y] = frag->z;
                            FRAME_BUFFER[screen_x][screen_y] = frag->col;
                        }
                    }
                } // x
            } // y
        }


        for(auto &s:semaphore_empty) s.release();

        
        output.clear();

        output += "\x1b[H\x1b[?25l";
        output += "\x1b[39;49m" + to_string(SCREEN_WIDTH) + "x" + to_string(SCREEN_HEIGHT*2) + " [vertices:" + to_string(mesh.Vertices.size()) + "] fps:" + to_string(cur_fps) + "\x1b[K\n";

        for(auto &s:semaphore_full) s.acquire();

        for(auto &i:buffer) output += i;
        cout << output;
        cout.flush();
    }

    return 0;
}