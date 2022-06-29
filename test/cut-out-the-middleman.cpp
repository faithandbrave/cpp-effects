// C++ Effects library
// Maciej Pirog, Huawei Edinburgh Research Centre, maciej.pirog@huawei.com
// License: MIT

// Test: Computations taken out of their original context:
// 
// 1. We use two nested handlers, and at some point remove the outer
//    one.
// 2. We handle a computation, but read the final result elsewhere.

#include <functional>
#include <iostream>

#include "cpp-effects/cpp-effects.h"

using namespace CppEffects;

// --
// 1.
// --

struct PingOuter : Command<> { };

struct PingInner : Command<> { };

struct CutMiddlemanAid : Command<> { };

struct CutMiddlemanAbet : Command<> {
  ResumptionData<void, void>* res;
};

class HInner : public Handler<void, void, PingInner, CutMiddlemanAid> {
  void CommandClause(PingInner, Resumption<void()> r) override
  {
    std::cout << "Inner!" << std::endl;
    std::move(r).TailResume();
  }
  void CommandClause(CutMiddlemanAid, Resumption<void()> r) override
  {
    OneShot::InvokeCmd(CutMiddlemanAbet{{}, r.Release()});
  }
  void ReturnClause() override { }
};

class HOuter : public Handler<void, void, PingOuter, CutMiddlemanAbet> {
  void CommandClause(PingOuter, Resumption<void()> r) override
  {
    std::cout << "Outer!" << std::endl;
    std::move(r).TailResume();
  }
  void CommandClause(CutMiddlemanAbet a, Resumption<void()>) override
  {
    Resumption<void()>(a.res).TailResume();
  }

  void ReturnClause() override { }
};

void test1()
{
    std::cout << "A+" << std::endl;
    OneShot::Handle<HOuter>([](){
      std::cout << "B+" << std::endl;
      OneShot::Handle<HInner>([&](){
        std::cout << "C+" << std::endl;
        OneShot::InvokeCmd(PingOuter{});
        OneShot::InvokeCmd(PingInner{});
        OneShot::InvokeCmd(CutMiddlemanAid{});
        // OneShot::InvokeCmd(PingOuter{}); // bad idea!
        OneShot::InvokeCmd(PingInner{});
        std::cout << "C-" << std::endl;
      });
      std::cout << "B-" << std::endl;
    });
    std::cout << "A-" << std::endl;
}

// Output:
// A+
// B+
// C+
// Outer!
// Inner!
// Inner!
// C-
// A-

// ----------------------------------------------------------------

// --
// 2.
// --

ResumptionData<void, int>* Res = nullptr;

struct Inc : Command<> { };

struct Break : Command<> { };

void inc()
{
  OneShot::InvokeCmd(Inc{});
}
void break_()
{
  OneShot::InvokeCmd(Break{});
}
int resume()
{
  return Resumption<int()>(Res).Resume();
}

class HIP : public Handler<int, int, Inc, Break> {
  int CommandClause(Break, Resumption<int()> r) override
  {
    Res = r.Release();
    return 0;
  }
  int CommandClause(Inc, Resumption<int()> r) override
  {
    return std::move(r).Resume() + 1;
  }
  int ReturnClause(int v) override { return v; }
};

int comp()
{
  inc();
  inc();
  break_();
  inc();
  break_();
  inc();
  return 100;
}

void test2()
{
  std::cout << OneShot::Handle<HIP>(comp) << std::endl;
  std::cout << resume() << std::endl;
  std::cout << resume() << std::endl;
}

// Output:
// 2
// 1
// 101

// ----------------------------------------------------------------

int main()
{
  std::cout << "--- cut-out-the-middleman ---" << std::endl;
  test1();
  std::cout << "***" << std::endl;
  test2();
}
