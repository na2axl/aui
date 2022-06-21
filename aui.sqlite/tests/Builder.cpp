/*
 * =====================================================================================================================
 * Copyright (c) 2021 Alex2772
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated
 * documentation files (the "Software"), to deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
 * WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 
 * Original code located at https://github.com/aui-framework/aui
 * =====================================================================================================================
 */

//
// Created by alex2 on 31.08.2020.
//

#include <gtest/gtest.h>
#include <AUI/Data/ASqlBuilder.h>
#include "AUI/Data/ASqlBlueprint.h"
#include "AUI/Data/AMigrationManager.h"


class Builder: public ::testing::Test {
protected:
    void SetUp() override {
        Test::SetUp();

        Autumn::put(ASqlDatabase::connect("sqlite", ":memory:"));
        AMigrationManager mm;
        mm.registerMigration("initial", [&]() {
            ASqlBlueprintTable t("users");
            t.varchar("name");
        });
        mm.doMigration();
    }
};


void seedDatabase() {
    table("users").ins("name").row({"Soso"}).rows({{"Kekos"}, {"Lol"}});
}
TEST_F(Builder, Raw) {
    Autumn::get<ASqlDatabase>()->execute("INSERT INTO users (name) VALUES (?)", {"John"});
    auto res = Autumn::get<ASqlDatabase>()->query("SELECT * FROM users");
    for (auto r : res) {
        ASSERT_EQ(r.getValue(1).toString(), "John");
    }
}

TEST_F(Builder, BuilderInsert) {
        seedDatabase();
}
TEST_F(Builder, BuilderInsertId) {
        id_t id = table("users").ins("name").row({"Soso"}).rowId();
        ASSERT_EQ(table("users").sel("name").where(col("id") == id).get().first().first(), "Soso");
}
TEST_F(Builder, BuilderSelect) {
        seedDatabase();
        AVector<AString> names = {
            "Soso",
                    "Kekos",
                    "Lol",
        };
        size_t index = 0;
        for (auto& row : table("users").sel("id", "name").get()) {
            ASSERT_EQ(row[0], unsigned(index + 1));
            ASSERT_EQ(row[1], names[index++]);
        }
}
TEST_F(Builder, BuilderSelectWhere1) {
        seedDatabase();

        auto validate = [](const AVector<AVector<AVariant>>& result) {
            EXPECT_EQ(result.size(), 1);
            EXPECT_EQ(result.first().size(), 1);
            return result.first().first();
        };
        ASSERT_EQ(validate(table("users").sel("name").where(col("id") == 1).get()), "Soso");
        ASSERT_EQ(validate(table("users").sel("name").where(col("id") == 2).get()), "Kekos");
        ASSERT_EQ(validate(table("users").sel("name").where(col("id") == 3).get()), "Lol");
        ASSERT_TRUE(table("users").sel("name").where(col("id") == 4).get().empty());
}
TEST_F(Builder, BuilderSelectWhere2) {
        seedDatabase();
        ASSERT_TRUE(table("users").sel("name").where(col("id") == 0 && col("id") == 1).get().empty());
        ASSERT_TRUE(table("users").sel("name").where(col("id") == 1 && col("name") == "Soso").get().size() == 1);
        auto r = table("users").sel("name").where(col("id") == 3 || col("name") == "Soso").get();
        ASSERT_EQ(r[0].first(), "Soso");
        ASSERT_EQ(r[1].first(), "Lol");

        ASSERT_TRUE(table("users").sel("name").where((col("id") == 1 && col("id") == 2) || col("name") == "Soso").get().size() == 1);
}
TEST_F(Builder, BuilderUpdate) {
        seedDatabase();

        table("users").update({{"name", "pisos"}});

        for (auto i : table("users").sel("name").get()) {
            ASSERT_EQ(i[0], "pisos");
        }
}
TEST_F(Builder, BuilderUpdateWhere) {
        seedDatabase();

        AVector<AString> names = {
            "pisos",
                    "Kekos",
                    "Lol",
        };

        table("users").update({{"name", "pisos"}}).where(col("id") == 1);

        for (auto i : table("users").sel("id", "name").get()) {
            ASSERT_EQ(i[1], names[i[0].toUInt() - 1]);
        }
}

TEST_F(Builder, BuilderDelete) {
        seedDatabase();

        table("users").remove();

        ASSERT_TRUE(table("users").select().get().empty());
}

TEST_F(Builder, BuilderDeleteWhere) {
        seedDatabase();

        table("users").remove().where(col("id") == 3);

        auto result = table("users").sel("name").get();
        ASSERT_EQ(result.size(), 2);

        ASSERT_EQ(result[0][0], "Soso");
        ASSERT_EQ(result[1][0], "Kekos");
}
