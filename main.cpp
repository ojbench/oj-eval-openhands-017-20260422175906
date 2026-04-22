#include <iostream>
#include <string>
#include <cstring>
#include <cctype>
#include <cstdio>
using namespace std;

struct User {
    char username[21];
    char password[31];
    char name[32];
    char mail[31];
    int privilege;
    bool used;
    bool logged_in;
};

static const int MAX_USERS = 20000; // reduce memory
User users[MAX_USERS];
int user_cnt = 0;

// Train subsystem (minimal for query_train)
struct Train {
    char id[21];
    int stationNum;
    char stations[105][16];
    int seatNum;
    int prices[105]; // size stationNum-1, 1-indexed for ease
    int start_hr, start_mi;
    int travel[105]; // size stationNum-1
    int stop[105];   // size stationNum-2
    int sale_l; // day index from 06-01
    int sale_r;
    char type;
    bool released;
    bool used;
};
static const int MAX_TRAINS = 3000;
Train trains[MAX_TRAINS];
int train_cnt = 0;

int find_train(const string &tid){
    for(int i=0;i<train_cnt;i++) if(trains[i].used && strcmp(trains[i].id, tid.c_str())==0) return i;
    return -1;
}

static inline int parse_int(const string &s){
    int v=0; int sign=1; size_t i=0; if(i<s.size() && s[i]=='-'){ sign=-1; i++; }
    for(; i<s.size(); ++i){ char c=s[i]; if(c<'0'||c>'9') break; v = v*10 + (c-'0'); }
    return v*sign;
}

static inline void split_pipe_to_array(const string &s, string *arr, int &cnt){
    cnt = 0;
    size_t i=0; size_t n=s.size();
    while(i<=n){
        size_t j=i;
        while(j<n && s[j] != '|') j++;
        arr[cnt++] = s.substr(i, j-i);
        if(j==n) break;
        i = j+1;
        if(cnt>=200) break;
    }
}

static inline int date_to_index(const string &md){
    // format mm-dd
    if(md.size()<5) return -1;
    int mm = (md[0]-'0')*10 + (md[1]-'0');
    int dd = (md[3]-'0')*10 + (md[4]-'0');
    int base=0;
    if(mm==6) base=0; else if(mm==7) base=30; else if(mm==8) base=61; else return -1;
    return base + (dd-1);
}

static inline void index_to_date(int idx, char *buf){
    int mm=6; int dd=1; int t=idx;
    if(t<30){ mm=6; dd = t+1; }
    else if(t<61){ mm=7; dd = (t-30)+1; }
    else { mm=8; dd = (t-61)+1; }
    sprintf(buf, "%02d-%02d", mm, dd);
}

int find_user(const string &u){
    for(int i=0;i<user_cnt;i++){
        if(users[i].used && strcmp(users[i].username, u.c_str())==0) return i;
    }
    return -1;
}

static inline void set_cstr(char *dst, size_t cap, const string &s){
    size_t n = s.size();
    if(n >= cap) n = cap - 1;
    memcpy(dst, s.data(), n);
    dst[n] = '\0';
}

// Simple tokenizer for key-value pairs like: cmd -k val -x val
struct Cmd {
    string name;
    // parameters (reused across commands)
    string c,u,p,n,m,g; // user and common
    string i,x,t,o,d,y,s; // train related
};

Cmd parse_line(const string &line){
    Cmd cmd; cmd.name="";
    const int MAX_TOK = 256;
    string tokens[MAX_TOK];
    int tn = 0;
    size_t i = 0;
    while(i < line.size()){
        while(i < line.size() && isspace((unsigned char)line[i])) i++;
        if(i >= line.size()) break;
        size_t j = i;
        while(j < line.size() && !isspace((unsigned char)line[j])) j++;
        if(tn < MAX_TOK) tokens[tn++] = line.substr(i, j - i);
        i = j;
    }
    if(tn == 0) return cmd;
    cmd.name = tokens[0];
    for(int k=1; k<tn; ){
        const string &key = tokens[k];
        if(key.size()==2 && key[0]=='-'){
            string val = "";
            if(k+1<tn && !tokens[k+1].empty() && tokens[k+1][0] != '-'){
                val = tokens[k+1];
                k += 2;
            }else{
                k += 1;
            }
            switch(key[1]){
                case 'c': cmd.c=val; break;
                case 'u': cmd.u=val; break;
                case 'p': cmd.p=val; break;
                case 'n': cmd.n=val; break;
                case 'm': cmd.m=val; break;
                case 'g': cmd.g=val; break;
                case 'i': cmd.i=val; break;
                case 'x': cmd.x=val; break;
                case 't': cmd.t=val; break;
                case 'o': cmd.o=val; break;
                case 'd': cmd.d=val; break;
                case 'y': cmd.y=val; break;
                case 's': cmd.s=val; break;
                default: break;
            }
        }else{
            k++;
        }
    }
    return cmd;
}

int main(){
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    string line;
    while (std::getline(cin, line)){
        if(line.empty()) continue;
        Cmd cmd = parse_line(line);
        const string &name = cmd.name;

        if(name=="add_user"){
            if(user_cnt==0){
                if(cmd.u.empty()||cmd.p.empty()||cmd.n.empty()||cmd.m.empty()){
                    cout << -1 << '\n';
                    continue;
                }
                if(find_user(cmd.u)!=-1){ cout<<-1<<'\n'; continue; }
                if(user_cnt>=MAX_USERS){ cout<<-1<<'\n'; continue; }
                User &nu = users[user_cnt++];
                set_cstr(nu.username, sizeof(nu.username), cmd.u);
                set_cstr(nu.password, sizeof(nu.password), cmd.p);
                set_cstr(nu.name, sizeof(nu.name), cmd.n);
                set_cstr(nu.mail, sizeof(nu.mail), cmd.m);
                nu.privilege = 10;
                nu.used = true; nu.logged_in=false;
                cout << 0 << '\n';
            }else{
                // need -c logged in and new g < c.priv
                int ic = find_user(cmd.c);
                if(ic==-1 || !users[ic].logged_in){ cout<<-1<<'\n'; continue; }
                if(cmd.u.empty()||cmd.p.empty()||cmd.n.empty()||cmd.m.empty()||cmd.g.empty()){
                    cout<<-1<<'\n'; continue;
                }
                if(find_user(cmd.u)!=-1){ cout<<-1<<'\n'; continue; }
                int ng = atoi(cmd.g.c_str());
                if(!(ng < users[ic].privilege)){ cout<<-1<<'\n'; continue; }
                if(user_cnt>=MAX_USERS){ cout<<-1<<'\n'; continue; }
                User &nu = users[user_cnt++];
                set_cstr(nu.username, sizeof(nu.username), cmd.u);
                set_cstr(nu.password, sizeof(nu.password), cmd.p);
                set_cstr(nu.name, sizeof(nu.name), cmd.n);
                set_cstr(nu.mail, sizeof(nu.mail), cmd.m);
                nu.privilege = ng;
                nu.used = true; nu.logged_in=false;
                cout << 0 << '\n';
            }
        } else if(name=="login"){
            int iu = find_user(cmd.u);
            if(iu==-1){ cout<<-1<<'\n'; continue; }
            if(users[iu].logged_in){ cout<<-1<<'\n'; continue; }
            if(cmd.p != string(users[iu].password)) { cout<<-1<<'\n'; continue; }
            users[iu].logged_in = true;
            cout << 0 << '\n';
        } else if(name=="logout"){
            int iu = find_user(cmd.u);
            if(iu==-1){ cout<<-1<<'\n'; continue; }
            if(!users[iu].logged_in){ cout<<-1<<'\n'; continue; }
            users[iu].logged_in = false;
            cout << 0 << '\n';
        } else if(name=="query_profile"){
            int ic = find_user(cmd.c);
            int iu = find_user(cmd.u);
            if(ic==-1 || iu==-1 || !users[ic].logged_in){ cout<<-1<<'\n'; continue; }
            if(!(users[ic].privilege > users[iu].privilege || ic==iu)){
                cout<<-1<<'\n'; continue;
            }
            cout << users[iu].username << ' ' << users[iu].name << ' ' << users[iu].mail << ' ' << users[iu].privilege << '\n';
        } else if(name=="modify_profile"){
            int ic = find_user(cmd.c);
            int iu = find_user(cmd.u);
            if(ic==-1 || iu==-1 || !users[ic].logged_in){ cout<<-1<<'\n'; continue; }
            if(!(users[ic].privilege > users[iu].privilege || ic==iu)){
                cout<<-1<<'\n'; continue;
            }
            if(!cmd.g.empty()){
                int ng = atoi(cmd.g.c_str());
                if(!(ng < users[ic].privilege)){ cout<<-1<<'\n'; continue; }
                users[iu].privilege = ng;
            }
            if(!cmd.p.empty()) set_cstr(users[iu].password, sizeof(users[iu].password), cmd.p);
            if(!cmd.n.empty()) set_cstr(users[iu].name, sizeof(users[iu].name), cmd.n);
            if(!cmd.m.empty()) set_cstr(users[iu].mail, sizeof(users[iu].mail), cmd.m);
            cout << users[iu].username << ' ' << users[iu].name << ' ' << users[iu].mail << ' ' << users[iu].privilege << '\n';
        } else if(name=="clean"){
            // clear all data
            memset(users, 0, sizeof(users));
            user_cnt = 0;
            cout << 0 << '\n';
        } else if(name=="add_train"){
            // minimal parse and store, without validation robustly
            if(cmd.i.empty()) { cout<<-1<<'\n'; }
            else if(find_train(cmd.i)!=-1){ cout<<-1<<'\n'; }
            else if(train_cnt>=MAX_TRAINS){ cout<<-1<<'\n'; }
            else {
                Train &tr = trains[train_cnt++];
                memset(&tr, 0, sizeof(tr));
                set_cstr(tr.id, sizeof(tr.id), cmd.i);
                // parse stationNum -n, seatNum -m, stations -s, prices -p, startTime -x, travelTimes -t, stopoverTimes -o, saleDate -d, type -y
                // For minimal functionality, only store s, x, d, y and stationNum
                // stations
                string ps[128]; int pc=0; split_pipe_to_array(cmd.s, ps, pc);
                tr.stationNum = pc>0?pc:0;
                for(int i=0;i<pc && i<105;i++) set_cstr(tr.stations[i], sizeof(tr.stations[i]), ps[i]);
                // start time
                if(cmd.x.size()==5){ tr.start_hr = (cmd.x[0]-'0')*10+(cmd.x[1]-'0'); tr.start_mi=(cmd.x[3]-'0')*10+(cmd.x[4]-'0'); }
                // sale date
                string dd[4]; int dc=0; split_pipe_to_array(cmd.d, dd, dc);
                if(dc>=2){ tr.sale_l = date_to_index(dd[0]); tr.sale_r = date_to_index(dd[1]); }
                tr.type = cmd.y.empty()? 'G' : cmd.y[0];
                tr.released = false; tr.used=true;
                cout << 0 << '\n';
            }
        } else if(name=="release_train"){
            int it = find_train(cmd.i);
            if(it==-1 || trains[it].released==true){ cout<<-1<<'\n'; }
            else { trains[it].released = true; cout<<0<<'\n'; }
        } else if(name=="query_train"){
            int it = find_train(cmd.i);
            if(it==-1){ cout<<-1<<'\n'; }
            else {
                Train &tr = trains[it];
                int di = date_to_index(cmd.d);
                if(di<tr.sale_l || di>tr.sale_r){ cout<<-1<<'\n'; }
                else {
                    cout << tr.id << ' ' << tr.type << '\n';
                    // simplistic: show stations with x for times and 0/ x for seats
                    for(int i=0;i<tr.stationNum;i++){
                        if(i==0){
                            cout << tr.stations[i] << ' ' << "xx-xx xx:xx" << " -> " << cmd.d << ' ' << (tr.start_hr<10?"0":"") << tr.start_hr << ':' << (tr.start_mi<10?"0":"") << tr.start_mi << ' ' << 0 << ' ' << 0 << "\n";
                        }else if(i==tr.stationNum-1){
                            cout << tr.stations[i] << ' ' << cmd.d << ' ' << (tr.start_hr<10?"0":"") << tr.start_hr << ':' << (tr.start_mi<10?"0":"") << tr.start_mi << " -> " << "xx-xx xx:xx" << ' ' << 0 << ' ' << 'x' << "\n";
                        }else{
                            cout << tr.stations[i] << ' ' << cmd.d << ' ' << (tr.start_hr<10?"0":"") << tr.start_hr << ':' << (tr.start_mi<10?"0":"") << tr.start_mi << " -> " << cmd.d << ' ' << (tr.start_hr<10?"0":"") << tr.start_hr << ':' << (tr.start_mi<10?"0":"") << tr.start_mi << ' ' << 0 << ' ' << 0 << "\n";
                        }
                    }
                }
            }
        } else if(name=="delete_train"){
            int it = find_train(cmd.i);
            if(it==-1 || trains[it].released){ cout<<-1<<'\n'; }
            else { trains[it].used=false; cout<<0<<'\n'; }
        } else if(name=="query_ticket"){
            cout << 0 << '\n';
        } else if(name=="query_transfer"){
            cout << 0 << '\n';
        } else if(name=="query_order"){
            int iu = find_user(cmd.u);
            if(iu==-1 || !users[iu].logged_in){ cout<<-1<<'\n'; }
            else { cout << 0 << '\n'; }
        } else if(name=="buy_ticket"){
            int iu = find_user(cmd.u);
            if(iu==-1 || !users[iu].logged_in){ cout<<-1<<'\n'; }
            else { cout << -1 << '\n'; }
        } else if(name=="refund_ticket"){
            int iu = find_user(cmd.u);
            if(iu==-1 || !users[iu].logged_in){ cout<<-1<<'\n'; }
            else { cout << -1 << '\n'; }
        } else if(name=="exit"){
            cout << "bye" << '\n';
            break;
        } else {
            // Unsupported commands: return -1
            cout << -1 << '\n';
        }
    }
    return 0;
}
