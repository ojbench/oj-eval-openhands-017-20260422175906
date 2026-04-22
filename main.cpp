#include <iostream>
#include <string>
#include <cstring>
#include <cctype>
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

static const int MAX_USERS = 100000; // enough for tests
User users[MAX_USERS];
int user_cnt = 0;

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
    // fixed keys we care about for user subsystem
    string c,u,p,n,m,g;
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
