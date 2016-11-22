/*
 * Copyright (C) 2016 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#define LOG_TAG "LibHidlTest"

#include <android-base/logging.h>
#include <gtest/gtest.h>
#include <hidl/HidlSupport.h>
#include <hidl/TaskRunner.h>
#include <vector>

#define EXPECT_ARRAYEQ(__a1__, __a2__, __size__) EXPECT_TRUE(isArrayEqual(__a1__, __a2__, __size__))

template<typename T, typename S>
static inline bool isArrayEqual(const T arr1, const S arr2, size_t size) {
    for(size_t i = 0; i < size; i++)
        if(arr1[i] != arr2[i])
            return false;
    return true;
}

class LibHidlTest : public ::testing::Test {
public:
    virtual void SetUp() override {
    }
    virtual void TearDown() override {
    }
};

TEST_F(LibHidlTest, StringTest) {
    using android::hardware::hidl_string;
    hidl_string s; // empty constructor
    EXPECT_STREQ(s.c_str(), "");
    hidl_string s1 = "s1"; // copy = from cstr
    EXPECT_STREQ(s1.c_str(), "s1");
    hidl_string s2("s2"); // copy constructor from cstr
    EXPECT_STREQ(s2.c_str(), "s2");
    hidl_string s3 = hidl_string("s3"); // move =
    EXPECT_STREQ(s3.c_str(), "s3");
    hidl_string s4(hidl_string(hidl_string("s4"))); // move constructor
    EXPECT_STREQ(s4.c_str(), "s4");
    hidl_string s5(std::string("s5")); // copy constructor from std::string
    EXPECT_STREQ(s5, "s5");
    hidl_string s6 = std::string("s6"); // copy = from std::string
    EXPECT_STREQ(s6, "s6");
    hidl_string s7(s6); // copy constructor
    EXPECT_STREQ(s7, "s6");
    hidl_string s8 = s7; // copy =
    EXPECT_STREQ(s8, "s6");
    char myCString[20] = "myCString";
    s.setToExternal(&myCString[0], strlen(myCString));
    EXPECT_STREQ(s, "myCString");
    myCString[2] = 'D';
    EXPECT_STREQ(s, "myDString");
    s.clear(); // should not affect myCString
    EXPECT_STREQ(myCString, "myDString");

    // casts
    s = "great";
    std::string myString = s;
    const char *anotherCString = s;
    EXPECT_EQ(myString, "great");
    EXPECT_STREQ(anotherCString, "great");

    // Comparisons
    const char * cstr1 = "abc";
    std::string string1(cstr1);
    hidl_string hs1(cstr1);
    const char * cstrE = "abc";
    std::string stringE(cstrE);
    hidl_string hsE(cstrE);
    const char * cstrNE = "ABC";
    std::string stringNE(cstrNE);
    hidl_string hsNE(cstrNE);
    EXPECT_TRUE(hs1  == hsE);
    EXPECT_FALSE(hs1 != hsE);
    EXPECT_TRUE(hs1  != hsNE);
    EXPECT_FALSE(hs1 == hsNE);
    EXPECT_TRUE(hs1  == cstrE);
    EXPECT_FALSE(hs1 != cstrE);
    EXPECT_TRUE(hs1  != cstrNE);
    EXPECT_FALSE(hs1 == cstrNE);
    EXPECT_TRUE(hs1  == stringE);
    EXPECT_FALSE(hs1 != stringE);
    EXPECT_TRUE(hs1  != stringNE);
    EXPECT_FALSE(hs1 == stringNE);
}

TEST_F(LibHidlTest, VecTest) {
    using android::hardware::hidl_vec;
    using std::vector;
    int32_t array[] = {5, 6, 7};
    vector<int32_t> v(array, array + 3);

    hidl_vec<int32_t> hv1 = v; // copy =
    EXPECT_ARRAYEQ(hv1, array, 3);
    EXPECT_ARRAYEQ(hv1, v, 3);
    hidl_vec<int32_t> hv2(v); // copy constructor
    EXPECT_ARRAYEQ(hv2, v, 3);

    vector<int32_t> v2 = hv1; // cast
    EXPECT_ARRAYEQ(v2, v, 3);

    hidl_vec<int32_t> v3 = {5, 6, 7}; // initializer_list
    EXPECT_EQ(v3.size(), 3ul);
    EXPECT_ARRAYEQ(v3, array, v3.size());

    auto iter = hv1.begin();    // iterator begin()
    EXPECT_EQ(*iter, 5);
    iter++;                     // post increment
    EXPECT_EQ(*iter, 6);
    ++iter;                     // pre increment
    EXPECT_EQ(*iter, 7);

    int32_t sum = 0;            // range based for loop interoperability
    for (auto &&i: hv1) {
        sum += i;
    }
    EXPECT_EQ(sum, 5+6+7);
}

TEST_F(LibHidlTest, ArrayTest) {
    using android::hardware::hidl_array;
    int32_t array[] = {5, 6, 7};

    hidl_array<int32_t, 3> ha(array);
    EXPECT_ARRAYEQ(ha, array, 3);
}

TEST_F(LibHidlTest, TaskRunnerTest) {
    using android::hardware::TaskRunner;
    TaskRunner tr;
    bool flag = false;
    tr.push([&] {
        usleep(1000);
        flag = true;
    });
    usleep(500);
    EXPECT_FALSE(flag);
    usleep(1000);
    EXPECT_TRUE(flag);
}

TEST_F(LibHidlTest, StringCmpTest) {
    using android::hardware::hidl_string;
    const char * s = "good";
    hidl_string hs(s);
    EXPECT_NE(hs.c_str(), s);

    EXPECT_TRUE(hs == s); // operator ==
    EXPECT_TRUE(s == hs);

    EXPECT_FALSE(hs != s); // operator ==
    EXPECT_FALSE(s != hs);
}

template <typename T>
void great(android::hardware::hidl_vec<T>) {}

TEST_F(LibHidlTest, VecCopyTest) {
    android::hardware::hidl_vec<int32_t> v;
    great(v);
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
