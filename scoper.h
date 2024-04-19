//
// Created by higgsboson on 1/3/24.
//

#ifndef CC_SCOPER_H
#define CC_SCOPER_H

#include <iostream>
#include <map>
#include <string>
#include <vector>

#define DEBUG 0

using namespace std;

enum ActualValueType { INTEGER, FLOATING, STRING, BOOLEAN, POINTER, NO_VALUE };

struct m_Value {
  ActualValueType type;

  int i;
  double f;
  std::string s;
  bool b;
  void *p;

  m_Value() : type(NO_VALUE), p(nullptr) {}

  ~m_Value() {}
  m_Value(int i) : type(INTEGER), i(i) {}
  m_Value(float f) : type(FLOATING), f(f) {}
  m_Value(double d) : type(FLOATING), f(d) {}
  m_Value(const std::string &s) : type(STRING), s(s) {}

  m_Value operator+(const m_Value &other) const {
    if (type == INTEGER && other.type == INTEGER)
      return m_Value(i + other.i);
    else if (type == INTEGER && other.type == FLOATING)
      return m_Value(i + other.f);
    else if (type == FLOATING && other.type == INTEGER)
      return m_Value(f + other.i);
    else if (type == FLOATING && other.type == FLOATING)
      return m_Value(f + other.f);
    else
      return m_Value();
  }

  m_Value operator-(const m_Value &other) const {
    if (type == INTEGER && other.type == INTEGER)
      return m_Value(i - other.i);
    else if (type == INTEGER && other.type == FLOATING)
      return m_Value(i - other.f);
    else if (type == FLOATING && other.type == INTEGER)
      return m_Value(f - other.i);
    else if (type == FLOATING && other.type == FLOATING)
      return m_Value(f - other.f);
    else if (!s.empty() && !other.s.empty() && s == other.s)
      return m_Value(0);
    else
      return m_Value();
  }

  m_Value operator*(const m_Value &other) const {
    if (type == INTEGER && other.type == INTEGER)
      return m_Value(i * other.i);
    else if (type == INTEGER && other.type == FLOATING)
      return m_Value(i * other.f);
    else if (type == FLOATING && other.type == INTEGER)
      return m_Value(f * other.i);
    else if (type == FLOATING && other.type == FLOATING)
      return m_Value(f * other.f);
    else if (type == FLOATING && f == 0)
      return m_Value(0.0);
    else if (other.type == FLOATING && other.f == 0)
      return m_Value(0.0);
    else if (type == INTEGER && i == 0)
      return m_Value(0);
    else if (other.type == INTEGER && other.i == 0)
      return m_Value(0);
    else
      return m_Value();
  }

  m_Value operator/(const m_Value &other) const {
    if (type == INTEGER && other.type == INTEGER && other.i != 0)
      return m_Value(i / other.i);
    else if (type == INTEGER && other.type == FLOATING && other.f != 0.0)
      return m_Value(i / other.f);
    else if (type == FLOATING && other.type == INTEGER && other.i != 0)
      return m_Value(f / other.i);
    else if (type == FLOATING && other.type == FLOATING && other.f != 0.0)
      return m_Value(f / other.f);

    else if (!s.empty() && !other.s.empty() && s == other.s)
      return m_Value(1);
    else
      return m_Value();
  }

  m_Value operator%(const m_Value &other) const {
    if (type == INTEGER && other.type == INTEGER && other.i != 0)
      return m_Value(i % other.i);

    else if (!s.empty() && !other.s.empty() && s == other.s)
      return m_Value(0);
    else
      return m_Value();
  }

  m_Value operator<<(const m_Value &other) const {
    if (type == INTEGER && other.type == INTEGER)

      return m_Value(i << other.i);
    else if (other.type == INTEGER && other.i > 64)
      return m_Value(0);

    else
      return m_Value();
  }

  m_Value operator>>(const m_Value &other) const {
    if (type == INTEGER && other.type == INTEGER)
      return m_Value(i >> other.i);

    else
      return m_Value();
  }

  m_Value operator!=(const m_Value &other) const {
    if (type == INTEGER && other.type == INTEGER)
      return m_Value(i != other.i);
    else if (type == INTEGER && other.type == FLOATING)
      return m_Value(i != other.f);
    else if (type == FLOATING && other.type == INTEGER)
      return m_Value(f != other.i);
    else if (type == FLOATING && other.type == FLOATING)
      return m_Value(f != other.f);
    else if (!s.empty() && !other.s.empty() && s == other.s)
      return m_Value(0);
    else
      return m_Value();
  }
  m_Value operator==(const m_Value &other) const {
    if (type == INTEGER && other.type == INTEGER)
      return m_Value(i == other.i);
    else if (type == INTEGER && other.type == FLOATING)
      return m_Value(i == other.f);
    else if (type == FLOATING && other.type == INTEGER)
      return m_Value(f == other.i);
    else if (type == FLOATING && other.type == FLOATING)
      return m_Value(f == other.f);

    else if (!s.empty() && !other.s.empty() && s == other.s)
      return m_Value(1);
    else
      return m_Value();
  }

  m_Value operator>(const m_Value &other) const {
    if (type == INTEGER && other.type == INTEGER)
      return m_Value(i > other.i);
    else if (type == INTEGER && other.type == FLOATING)
      return m_Value(i > other.f);
    else if (type == FLOATING && other.type == INTEGER)
      return m_Value(f > other.i);
    else if (type == FLOATING && other.type == FLOATING)
      return m_Value(f > other.f);
    else if (!s.empty() && !other.s.empty() && s == other.s)
      return m_Value(0);
    else
      return m_Value();
  }

  m_Value operator>=(const m_Value &other) const {
    if (type == INTEGER && other.type == INTEGER)
      return m_Value(i >= other.i);
    else if (type == INTEGER && other.type == FLOATING)
      return m_Value(i >= other.f);
    else if (type == FLOATING && other.type == INTEGER)
      return m_Value(f >= other.i);
    else if (type == FLOATING && other.type == FLOATING)
      return m_Value(f >= other.f);
    else if (!s.empty() && !other.s.empty() && s == other.s)
      return m_Value(1);
    else
      return m_Value();
  }

  m_Value operator<(const m_Value &other) const {
    if (type == INTEGER && other.type == INTEGER)
      return m_Value(i < other.i);
    else if (type == INTEGER && other.type == FLOATING)
      return m_Value(i < other.f);
    else if (type == FLOATING && other.type == INTEGER)
      return m_Value(f < other.i);
    else if (type == FLOATING && other.type == FLOATING)
      return m_Value(f < other.f);
    else if (!s.empty() && !other.s.empty() && s == other.s)
      return m_Value(0);
    else
      return m_Value();
  }

  m_Value operator<=(const m_Value &other) const {
    if (type == INTEGER && other.type == INTEGER)
      return m_Value(i <= other.i);
    else if (type == INTEGER && other.type == FLOATING)
      return m_Value(i <= other.f);
    else if (type == FLOATING && other.type == INTEGER)
      return m_Value(f <= other.i);
    else if (type == FLOATING && other.type == FLOATING)
      return m_Value(f <= other.f);
    else if (!s.empty() && !other.s.empty() && s == other.s)
      return m_Value(1);
    else
      return m_Value();
  }

  m_Value operator&(const m_Value &other) const {
    if (type == INTEGER && other.type == INTEGER)
      return m_Value(i & other.i);
    else
      return m_Value();
  }

  m_Value operator|(const m_Value &other) const {
    if (type == INTEGER && other.type == INTEGER)
      return m_Value(i | other.i);
    else
      return m_Value();
  }

  m_Value operator^(const m_Value &other) const {
    if (type == INTEGER && other.type == INTEGER)
      return m_Value(i ^ other.i);
    else if (!s.empty() && !other.s.empty() && s == other.s)
      return m_Value(0);

    else
      return m_Value();
  }

  m_Value operator-() const {
    if (type == INTEGER)
      return m_Value(-i);
    else if (type == FLOATING)
      return m_Value(-f);
    else
      return m_Value();
  }

  m_Value operator+() const { return *this; }

  m_Value operator!() const {
    if (type == INTEGER)
      return m_Value(!i);
    else if (type == FLOATING)
      return m_Value(!f);
    else
      return m_Value();
  }

  m_Value operator~() const {
    if (type == INTEGER)
      return m_Value(~i);
    else
      return m_Value();
  }

  m_Value operator++() {
    if (type == INTEGER) {
      ++i;
      return *this;
    } else if (type == FLOATING) {
      ++f;
      return *this;
    } else {
      return m_Value();
    }
  }

  m_Value operator--() {
    if (type == INTEGER) {
      --i;
      return *this;
    } else if (type == FLOATING) {
      --f;
      return *this;
    } else {
      return m_Value();
    }
  }

  m_Value operator++(int) {
    if (type == INTEGER) {
      m_Value temp(i);
      i++;
      return temp;
    } else if (type == FLOATING) {
      m_Value temp(f);
      f++;
      return temp;
    } else {
      return m_Value();
    }
  }

  m_Value operator--(int) {
    if (type == INTEGER) {
      m_Value temp(i);
      i--;
      return temp;
    } else if (type == FLOATING) {
      m_Value temp(f);
      f--;
      return temp;
    } else {
      return m_Value();
    }
  }

  m_Value(const m_Value &other) : type(other.type) {
    switch (other.type) {
    case INTEGER:
      i = other.i;
      break;
    case FLOATING:
      f = other.f;
      break;
    case STRING:
      s = other.s;
      break;
    case BOOLEAN:
      b = other.b;
      break;
    case POINTER:
      p = other.p;
      break;
    default:
      p = nullptr;
      break;
    }
  }

  m_Value &operator=(const m_Value &other) {
    if (this != &other) {

      type = other.type;
      switch (other.type) {
      case INTEGER:
        i = other.i;
        break;
      case FLOATING:
        f = other.f;
        break;
      case STRING:
        s = other.s;
        break;
      case BOOLEAN:
        b = other.b;
        break;
      case POINTER:
        p = other.p;
        break;
      default:
        p = nullptr;
        break;
      }
    }
    return *this;
  }
};
class Scoper {
public:
  map<string, m_Value> scope;
  map<string, m_Value> carry_over_variables;

  Scoper() {}
  Scoper(const Scoper &other) { this->scope = other.carry_over_variables; }

  ~Scoper() {}

  void dump() {

    int size = 20;
    std::cout << std::string(size, '-') << std::endl;
    for (auto it = scope.begin(); it != scope.end(); it++) {
      auto str = it->first;

      std::cout << "|";
      if (str.length() > size) {
        std::cout << str.substr(0, size - 3) << "...";
      } else {
        std::cout << str;
        std::cout << std::string(size - str.length(),
                                 ' '); // Print excess spaces
      }
      std::cout << "|" << std::endl;
    }

    if (carry_over_variables.size()) {

      std::cout << std::string(size, '-') << std::endl;
      for (auto it = carry_over_variables.begin();
           it != carry_over_variables.end(); it++) {
        auto str = it->first;

        std::cout << "|";
        if (str.length() > size) {
          std::cout << str.substr(0, size - 3) << "...";
        } else {
          std::cout << str;
          std::cout << std::string(size - str.length(),
                                   ' '); // Print excess spaces
        }
        std::cout << "|" << std::endl;
      }
    }

    std::cout << std::string(size, '-') << std::endl;
  }

  void add(string name, m_Value val = m_Value()) { scope[name] = val; }

  bool exists(string name) { return scope.find(name) != scope.end(); }
};

class ScoperStack {
public:
  vector<Scoper> stack;

  ScoperStack() { stack.push_back(Scoper()); }

  ~ScoperStack() { stack.clear(); }

  void dump() {
    for (auto &scope : stack) {
      scope.dump();
    }
  }

  void push() {
    if (stack.size() == 0) {
      stack.push_back(Scoper());
    } else {
      stack.push_back(Scoper(stack.back()));
    }

    if (DEBUG) {
      cout << "Pushed a new scope" << endl;
      dump();
    }
  }

  void pop() { stack.pop_back(); }

  bool add_with_carry_over(string name, m_Value val = m_Value()) {
    if (!add(name, val)) {
      return false;
    }
    stack.back().carry_over_variables[name] = val;
    return true;
  }

  bool add(string name, m_Value val = m_Value()) {
    if (DEBUG) {
      cout << "Adding a new variable of the name " << name << endl;
      dump();
    }

    if (declared(name))
      return false;
    stack.back().add(name, val);

    return true;
  }

  bool declared(string name) {
    if (stack.back().exists(name))
      return true;
    return false;
  }

  bool exists(string name) {
    for (auto scope : stack) {
      if (scope.exists(name)) {
        return true;
      }
    }
    return false;
  }
};

#endif // CC_SCOPER_H
