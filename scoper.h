//
// Created by higgsboson on 1/3/24.
//

#ifndef CC_SCOPER_H
#define CC_SCOPER_H

#include <map>
#include <string>
#include <vector>

using namespace std;

union value{
    int i;
    float f;
    string s;
    bool b;
    void *p;

    value() : p(nullptr) {}
    ~value() {
        if (p) delete p;
        if (s.size()) s.clear();
    }

    value(const value& other) {
        i = other.i;
        f = other.f;
        s = other.s;
        b = other.b;
        p = other.p;
    }

    value& operator=(const value& other) {
        i = other.i;
        f = other.f;
        s = other.s;
        b = other.b;
        p = other.p;
        return *this;
    }

};

class Scoper {
    public:
        map<string, value> scope;

        Scoper();
        Scoper(const Scoper& other){
            scope = other.scope;
        }

        ~Scoper();

        void add(string name, value val = value()){
            scope[name] = val;
        }

        bool exists(string name){
            return scope.find(name) != scope.end();
        }
};

class ScoperStack {
    public:
        vector<Scoper> stack;

        ScoperStack() {
            stack.push_back(Scoper());
        }

        ~ScoperStack() {
            stack.clear();
        }

        void push() {
            stack.push_back(Scoper());
        }

        void pop() {
            stack.pop_back();
        }

        bool add(string name, value val = value()){
            if(stack.back().exists(name)) return false;
            stack.back().add(name, val);
            return true;
        }

        bool exists(string name){
            for(auto scope: stack){
                if(scope.exists(name)){
                    return true;
                }
            }
            return false;
        }

};




#endif //CC_SCOPER_H
