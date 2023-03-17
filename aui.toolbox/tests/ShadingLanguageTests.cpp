// AUI Framework - Declarative UI toolkit for modern C++20
// Copyright (C) 2020-2023 Alex2772
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation; either
// version 2 of the License, or (at your option) any later version.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the GNU
// Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public
// License along with this library. If not, see <http://www.gnu.org/licenses/>.

#include <gmock/gmock.h>
#include "AUI/Common/AString.h"
#include "AUI/IO/AStringStream.h"
#include "ShadingLanguage/Lang/Frontend/CppFrontend.h"
#include "ShadingLanguage/Lang/SL.h"
#include "ShadingLanguage/Lang/Frontend/GLSLFrontend.h"
#include "ShadingLanguage/Lang/Lexer.h"
#include "ShadingLanguage/Lang/Parser.h"


class ShadingLanguage : public ::testing::Test {
protected:

    AString codeBlockToCpp(const AString& input) {
        CppFrontend compiler;
        for (const auto& v : aui::sl::parseCodeBlock(input)) {
            v->acceptVisitor(compiler);
        }
        return compiler.shaderCode();
    }

    template<aui::derived_from<IFrontend> T>
    AString vertexTo(const AString& input) {
        T compiler;
        compiler.parseShader(aui::sl::parseCode(_new<AStringStream>(input)));
        return compiler.shaderCode();
    }
};

TEST_F(ShadingLanguage, Simplest) {
    EXPECT_EQ(codeBlockToCpp("vec4 kek = vec4(1, 2, 3, 4)"), "glm::vec4 kek = glm::vec4(1.0f,2.0f,3.0f,4.0f);");
}
TEST_F(ShadingLanguage, TwoLines) {
    EXPECT_EQ(codeBlockToCpp("vec4 kek = vec4(1, 2, 3, 4)\nvec4 kek2 = vec4(4, 5, 6, 7)"), "glm::vec4 kek = glm::vec4(1.0f,2.0f,3.0f,4.0f);glm::vec4 kek2 = glm::vec4(4.0f,5.0f,6.0f,7.0f);");
}
TEST_F(ShadingLanguage, BasicShader) {
    const auto code = R"(
input {
  [0] vec4 pos
}

entry {
  sl_position = input.pos
}
)";
    EXPECT_EQ(vertexTo<CppFrontend>(code), "struct Input{/* 0 */glm::vec4 pos;};struct Output{glm::vec4 __vertexOutput;};Output entry(Input input){Output output;output.__vertexOutput=input.pos;return output;}");
    EXPECT_EQ(vertexTo<GLSLFrontend>(code), "#version 120\n"
                                            "/* 0 */ attribute vec4 pos;void main(){gl_Position=pos;}");
}
TEST_F(ShadingLanguage, Math1) {
    Lexer l(_new<AStringStream>("x = 1 + 2 * 3"));
    Parser p(l.performLexAnalysis());
    auto expr = p.parseExpression();
}

TEST_F(ShadingLanguage, Math2) {
    Lexer l(_new<AStringStream>("x = 1.0 + (0.278393 + (0.230389 + 0.078108 * (a * a)) * a) * a\n"));
    Parser p(l.performLexAnalysis());
    auto expr = p.parseExpression();
}