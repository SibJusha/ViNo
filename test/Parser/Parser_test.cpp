#include "Parser.h"

#include <exception>
#include <filesystem>
#include <fstream>
#include <functional>
#include <ios>
#include <iostream>
#include <stdexcept>
#include <vector>

#include "SemanticAnalyzer.h"
#include "TokenEnum.h"
#include "boost/ut.hpp"
#include "custom_errors.h"

namespace fs = std::filesystem;

namespace test_utils {

class TestTokenizer {
    fs::path tmp{std::filesystem::temp_directory_path()};

public:
    explicit TestTokenizer(const std::vector<vino::ScriptToken>& tokens) :
        tokens_vec(tokens)
    {
        fs::create_directory(tmp / "dir");
        std::ofstream ostrm(tmp / "test.png", std::ios::binary | std::ios::out);
        ostrm.put('a');
        ostrm.close();
        ostrm.open(tmp / "test.txt", std::ios::binary | std::ios::out);
        ostrm.put('a');
        ostrm.close();
    }

    vino::PairTokenId test_get_token()
    {
        if (pos >= tokens_vec.size()) {
            return vino::PairTokenId(vino::ScriptToken::EXIT, "");
        }
        if (tokens_vec[pos] == vino::ScriptToken::PATH) {
            previous_is_path = true;
        } else if (tokens_vec[pos] == vino::ScriptToken::TEXT_TYPE) {
            previous_is_text_type = true;
        }
        if (previous_is_text_type
            && tokens_vec[pos] == vino::ScriptToken::TEXT_LINE)
        {
            previous_is_text_type = false;
            return vino::PairTokenId(tokens_vec[pos++],
                                     tmp.string() + "/test.txt");
        }
        if (previous_is_path && tokens_vec[pos] == vino::ScriptToken::TEXT_LINE)
        {
            previous_is_path = false;
            return vino::PairTokenId(tokens_vec[pos++], tmp.string() + "/dir");
        }
        return vino::PairTokenId(tokens_vec[pos++], tmp.string() + "/test.png");
    }

    ~TestTokenizer()
    {
        fs::remove(tmp / "dir");
        fs::remove(tmp / "test.png");
        fs::remove(tmp / "test.txt");
    }

private:
    size_t pos = 0;
    bool   previous_is_path = false;
    bool   previous_is_text_type = false;

    const std::vector<vino::ScriptToken>& tokens_vec;
};

}  // namespace test_utils

int main()
{
    using namespace boost::ut;
    using namespace boost::ut::literals;
    using namespace boost::ut::operators::terse;

    using vst = vino::ScriptToken;
    using vpt = vino::PairTokenId;

    // clang-format off
    /* Input:
     * #comment
     * background = "bg.png"
     * #comment
     * persona Human {path="./human", background="bg.png",
     * foreground = "human0.png", name = "Humanio"}
     * exit
     */
    const std::vector<vst> tokens_test = {
        vst::NEW_LINE,

        vst::BG, vst::SIGN_EQ, vst::TEXT_LINE,
        vst::NEW_LINE,

        vst::NEW_LINE,

        vst::PERSONA, vst::VAR, vst::BRACE_OP, 
        vst::PATH, vst::SIGN_EQ, vst::TEXT_LINE, vst::COMMA,
        vst::NAME, vst::SIGN_EQ, vst::TEXT_LINE, vst::COMMA, 
        vst::FG, vst::SIGN_EQ, vst::TEXT_LINE, vst::COMMA, 
        vst::VAR, vst::SIGN_EQ, vst::TEXT_LINE,
        vst::BRACE_CL,
        vst::NEW_LINE,

        vst::TEXT_TYPE, vst::TEXT_LINE, vst::NEW_LINE,

        vst::TEXT_TYPE, vst::SIGN_EQ, vst::TEXT_LINE, vst::NEW_LINE,

        vst::EXIT 
    };

    /* Input:
     * text = "text.txt"
     * text "text" 
    */
    const std::vector<vst> tokens_text = {
        vst::TEXT_TYPE, vst::SIGN_EQ, vst::TEXT_LINE,
        vst::NEW_LINE,

        vst::TEXT_TYPE, vst::TEXT_LINE,
        vst::NEW_LINE,

        vst::EXIT
    };

    const std::vector<vst> tokens_exit = {
        vst::EXIT
    };

    const std::vector<vst> WRONGtokens_0 = {
        vst::PERSONA,

        vst::EXIT
    };

    const std::vector<vst> WRONGtokens_1 = {
        vst::PERSONA, vst::VAR, vst::BRACE_OP,
        vst::NEW_LINE,

        vst::EXIT
    };

    const std::vector<vst> tokens_empty = {};

    // clang-format on

    /*std::function<vino::PairTokenId()> fn =
        std::bind(&test_utils::TestTokenizer::test_get_token, &tokenizer0);
    vino::Parser    parser(fn);
    vino::ScriptAst asstree = parser.run(true);

    vino::SymbolTableEnv   symbtable;
    vino::SemanticAnalyzer semanal(symbtable, asstree);
    semanal.run_analysis();
    */
    "parser_empty"_test = [&tokens_empty] {
        expect(nothrow([&tokens_empty] {
            test_utils::TestTokenizer tokenizer(tokens_empty);
            std::function<vpt()>      fn = std::bind(
                &test_utils::TestTokenizer::test_get_token, &tokenizer);
            vino::Parser parser(fn);
            parser.run(true);
        }));
    };

    "parser_not_empty"_test = [&tokens_test] {
        expect(nothrow([&tokens_test] {
            test_utils::TestTokenizer tokenizer(tokens_test);
            std::function<vpt()>      fn = std::bind(
                &test_utils::TestTokenizer::test_get_token, &tokenizer);
            vino::Parser parser(fn);
            parser.run(true);
        }));
    };
    
    "parser_input_set_on_init"_test = [&tokens_text] {
        expect(nothrow([&tokens_text] {
            test_utils::TestTokenizer tokenizer(tokens_text);
            std::function<vpt()>      fn = std::bind(
                &test_utils::TestTokenizer::test_get_token, &tokenizer);
            vino::Parser parser(fn);
            parser.run(true);
        }));
    };

    "parser_input_texts"_test = [&tokens_exit] {
        expect(nothrow([&tokens_exit] {
            test_utils::TestTokenizer tokenizer(tokens_exit);
            std::function<vpt()>      fn = std::bind(
                &test_utils::TestTokenizer::test_get_token, &tokenizer);
            vino::Parser parser(fn);
            parser.run(true);
        }));
    };

    "parser_persona_only"_test = [&WRONGtokens_0] {
        expect(throws<vino::ParsingError>([&WRONGtokens_0] {
            test_utils::TestTokenizer tokenizer(WRONGtokens_0);
            std::function<vpt()>      fn = std::bind(
                &test_utils::TestTokenizer::test_get_token, &tokenizer);
            vino::Parser parser(fn);
            parser.run();
        } ));
    };

    "parser_unclosed_persona"_test = [&WRONGtokens_1] {
        expect(throws<vino::ParsingError>([&WRONGtokens_1] {
            test_utils::TestTokenizer tokenizer(WRONGtokens_1);
            std::function<vpt()>      fn = std::bind(
                &test_utils::TestTokenizer::test_get_token, &tokenizer);
            vino::Parser parser(fn);
            parser.run();
        } ));
    };

    "parser_no_tokens"_test = [&tokens_empty] {
        expect(nothrow([&tokens_empty] {
            test_utils::TestTokenizer tokenizer(tokens_empty);
            std::function<vpt()>      fn = std::bind(
                &test_utils::TestTokenizer::test_get_token, &tokenizer);
            vino::Parser parser(fn);
            parser.run();
        } ));
    };

    return 0;
}
