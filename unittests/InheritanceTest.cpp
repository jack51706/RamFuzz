// Copyright 2016-2017 The RamFuzz contributors. All rights reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "gtest/gtest.h"

#include <algorithm>
#include <string>
#include <vector>

#include "ramfuzz/lib/Inheritance.hpp"

namespace {

using namespace std;
using namespace testing;
using ramfuzz::Inheritance;
using ramfuzz::InheritanceBuilder;

/// Returns success if findInheritance(code) equals expected, modulo ordering.
/// Otherwise, returns failure with an explanation.
AssertionResult hasInheritance(const string &code,
                               const map<string, vector<string>> &expected) {
  InheritanceBuilder builder(code);
  const auto &inh = builder.getInheritance();
  if (inh.size() != expected.size()) {
    return AssertionFailure()
           << "expected " << expected.size() << " elements, got " << inh.size();
  }
  for (const auto &entry : inh) {
    const auto &clsname = entry.first.qname();
    const auto found = expected.find(clsname);
    if (found == expected.end())
      return AssertionFailure() << "unexpected base class " << clsname;
    const auto &expected_subclasses = found->second;
    const auto &actual_subclasses = entry.second;
    if (expected_subclasses.size() != actual_subclasses.size())
      return AssertionFailure() << "expected " << expected_subclasses.size()
                                << " subclasses for base class " << clsname
                                << ", got " << actual_subclasses.size();
    for (const auto &subcls : actual_subclasses)
      if (find(expected_subclasses.cbegin(), expected_subclasses.cend(),
               subcls.qname()) == expected_subclasses.cend())
        return AssertionFailure() << "unexpected subclass " << subcls.qname()
                                  << " of class " << clsname;
  }
  return AssertionSuccess();
}

TEST(InheritanceTest, Empty) { EXPECT_TRUE(hasInheritance("", {})); }

TEST(InheritanceTest, NoInheritance) {
  EXPECT_TRUE(hasInheritance("class A{};", {}));
  EXPECT_TRUE(hasInheritance("class A1{}; class A2{};", {}));
}

TEST(InheritanceTest, OneInheritance) {
  EXPECT_TRUE(
      hasInheritance("class A {}; class B : public A {};", {{"A", {"B"}}}));
}

TEST(InheritanceTest, SeveralInheritances) {
  EXPECT_TRUE(hasInheritance("class A1 {}; class A2 : public A1 {};"
                             "class B1 {}; class B2 : public B1 {};",
                             {{"A1", {"A2"}}, {"B1", {"B2"}}}));
}

TEST(InheritanceTest, SubSubClass) {
  EXPECT_TRUE(hasInheritance(
      "class A1 {}; class A2 : public A1 {}; class A3 : public A2 {};",
      {{"A1", {"A2"}}, {"A2", {"A3"}}}));
}

TEST(InheritanceTest, MultipleSubclasses) {
  EXPECT_TRUE(hasInheritance(
      "class A {}; class B1 : public A {}; class B2 : public A {};",
      {{"A", {"B1", "B2"}}}));
}

TEST(InheritanceTest, MultipleBaseClasses) {
  EXPECT_TRUE(hasInheritance(
      "class A1 {}; class A2 {}; class B : public A1, public A2 {};",
      {{"A1", {"B"}}, {"A2", {"B"}}}));
}

TEST(InheritanceTest, NonPublic) {
  EXPECT_TRUE(hasInheritance(
      "class A1 {}; class A2 {}; class B1 : private A1, protected A2 {};", {}));
}

TEST(InheritanceTest, Namespace) {
  EXPECT_TRUE(hasInheritance(
      "namespace a1 {class A{};}"
      "namespace a2 {class A{}; class B : public A, public a1::A {};}"
      "namespace b1 {class B : public a1::A {};}",
      {{"a1::A", {"a2::B", "b1::B"}}, {"a2::A", {"a2::B"}}}));
}

TEST(InheritanceTest, Typedef) {
  EXPECT_TRUE(hasInheritance("class A{}; typedef A A2; class B: public A2 {};",
                             {{"A", {"B"}}}));
}

TEST(InheritanceTest, TypeAlias) {
  EXPECT_TRUE(hasInheritance("class A{}; using A2=A; class B: public A2 {};",
                             {{"A", {"B"}}}));
}

// Regressions only below this point.

TEST(InheritanceTest, Regression1) {
  // This once triggered the assertion "queried property of class with no
  // definition".
  EXPECT_TRUE(
      hasInheritance("template <class T> class init {};"
                     "template <class T> struct vector { vector(init<T>); };"
                     "struct A {vector<int> vi; };",
                     {}));
}

TEST(InheritanceTest, Regression2) {
  // This once triggered the assertion "cast<Ty>() argument of incompatible
  // type!"
  const auto code = "template <class T> struct A : public T {};";
  // Although T is a base class of A<T>, we can't make concrete use of that for
  // now.  Expect empty inheritance info.
  EXPECT_TRUE(hasInheritance(code, {}));
}

} // anonymous namespace
