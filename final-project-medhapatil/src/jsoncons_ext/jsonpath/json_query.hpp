// Copyright 2013 Daniel Parker
// Distributed under the Boost license, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// See https://github.com/danielaparker/jsoncons for latest version

#ifndef JSONCONS_JSONPATH_JSON_QUERY_HPP
#define JSONCONS_JSONPATH_JSON_QUERY_HPP

#include <array> // std::array
#include <string>
#include <vector>
#include <memory>
#include <unordered_map>
#include <type_traits> // std::is_const
#include <limits> // std::numeric_limits
#include <utility> // std::move
#include <regex>
#include <jsoncons/json.hpp>
#include <jsoncons_ext/jsonpath/jsonpath_filter.hpp>
#include <jsoncons_ext/jsonpath/jsonpath_error.hpp>
#include <jsoncons_ext/jsonpath/jsonpath_function.hpp>

namespace jsoncons { namespace jsonpath {

// work around for std::make_unique not being available until C++14
template<typename T, typename... Args>
std::unique_ptr<T> make_unique_ptr(Args&&... args)
{
    return std::unique_ptr<T>(new T(std::forward<Args>(args)...));
}

enum class result_type {value,path};

template<class Json>
Json json_query(const Json& root, const typename Json::string_view_type& path, result_type result_t = result_type::value)
{
    if (result_t == result_type::value)
    {
        jsoncons::jsonpath::detail::jsonpath_evaluator<Json,const Json&,detail::VoidPathConstructor<Json>> evaluator;
        evaluator.evaluate(root, path);
        return evaluator.get_values();
    }
    else
    {
        jsoncons::jsonpath::detail::jsonpath_evaluator<Json,const Json&,detail::PathConstructor<Json>> evaluator;
        evaluator.evaluate(root, path);
        return evaluator.get_normalized_paths();
    }
}

template<class Json, class T>
void json_replace(Json& root, const typename Json::string_view_type& path, T&& new_value)
{
    jsoncons::jsonpath::detail::jsonpath_evaluator<Json,Json&,detail::VoidPathConstructor<Json>> evaluator;
    evaluator.evaluate(root, path);
    evaluator.replace(std::forward<T>(new_value));
}

namespace detail {

template<class CharT>
bool try_string_to_index(const CharT *s, size_t length, size_t* value, bool* positive)
{
    static const size_t max_value = (std::numeric_limits<size_t>::max)();
    static const size_t max_value_div_10 = max_value / 10;

    size_t start = 0;
    size_t n = 0;
    if (length > 0)
    {
        if (s[start] == '-')
        {
            *positive = false;
            ++start;
        }
        else
        {
            *positive = true;
        }
    }
    if (length > start)
    {
        for (size_t i = start; i < length; ++i)
        {
            CharT c = s[i];
            switch (c)
            {
            case '0':case '1':case '2':case '3':case '4':case '5':case '6':case '7':case '8':case '9':
                {
                    size_t x = c - '0';
                    if (n > max_value_div_10)
                    {
                        return false;
                    }
                    n = n * 10;
                    if (n > max_value - x)
                    {
                        return false;
                    }

                    n += x;
                }
                break;
            default:
                return false;
                break;
            }
        }
        *value = n;
        return true;
    }
    else
    {
        return false;
    }
}

enum class path_state 
{
    start,
    cr,
    lf,
    expect_dot_or_left_bracket,
    expect_unquoted_name_or_left_bracket,
    unquoted_name,
    left_bracket_single_quoted_string,
    left_bracket_double_quoted_string,
    left_bracket,
    left_bracket_start,
    left_bracket_end,
    left_bracket_end2,
    left_bracket_step,
    left_bracket_step2,
    expect_comma_or_right_bracket,
    function_name,
    expect_arg_or_right_round_bracket,
    path_argument,
    unquoted_argument,
    single_quoted_argument,
    double_quoted_argument,
    expect_more_args_or_right_round_bracket,
    dot
};

template<class Json,
         class JsonReference,
         class PathCons>
class jsonpath_evaluator : private ser_context
{
    typedef typename Json::char_type char_type;
    typedef typename Json::char_traits_type char_traits_type;
    typedef std::basic_string<char_type,char_traits_type> string_type;
    typedef typename Json::string_view_type string_view_type;
    typedef JsonReference reference;
    using pointer = typename std::conditional<std::is_const<typename std::remove_reference<JsonReference>::type>::value,typename Json::const_pointer,typename Json::pointer>::type;

    static string_view_type length_literal()
    {
        static constexpr std::array<char_type, 6> length_k{ 'l','e','n','g','t','h' };
        return string_view_type{ length_k.data(),length_k.size() };
    }

    struct node_type
    {
        node_type() = default;
        node_type(const string_type& p, const pointer& valp)
            : skip_contained_object(false),path(p),val_ptr(valp)
        {
        }
        node_type(string_type&& p, pointer&& valp)
            : skip_contained_object(false),path(std::move(p)),val_ptr(valp)
        {
        }
        node_type(const node_type&) = default;
        node_type(node_type&&) = default;

        bool skip_contained_object;
        string_type path;
        pointer val_ptr;
    };
    typedef std::vector<node_type> node_set;

    class selector
    {
    public:
        virtual ~selector()
        {
        }
        virtual void select(jsonpath_evaluator& evaluator,
                            node_type& node, const string_type& path, reference val, node_set& nodes) = 0;
    };

    class expr_selector final : public selector
    {
    private:
         jsonpath_filter_expr<Json> result_;
    public:
        expr_selector(const jsonpath_filter_expr<Json>& result)
            : result_(result)
        {
        }

        void select(jsonpath_evaluator& evaluator,
                    node_type& node, const string_type& path, reference val, 
                    node_set& nodes) override
        {
            auto index = result_.eval(val);
            if (index.template is<size_t>())
            {
                size_t start = index.template as<size_t>();
                if (val.is_array() && start < val.size())
                {
                    nodes.emplace_back(PathCons()(path,start),std::addressof(val[start]));
                }
            }
            else if (index.is_string())
            {
                name_selector selector(index.as_string_view(),true);
                selector.select(evaluator, node, path, val, nodes);
            }
        }
    };

    class filter_selector final : public selector
    {
    private:
         jsonpath_filter_expr<Json> result_;
    public:
        filter_selector(const jsonpath_filter_expr<Json>& result)
            : result_(result)
        {
        }

        void select(jsonpath_evaluator&,
                    node_type& node, const string_type& path, reference val, 
                    node_set& nodes) override
        {
            if (val.is_array())
            {
                node.skip_contained_object =true;
                for (size_t i = 0; i < val.size(); ++i)
                {
                    if (result_.exists(val[i]))
                    {
                        nodes.emplace_back(PathCons()(path,i),std::addressof(val[i]));
                    }
                }
            }
            else if (val.is_object())
            {
                if (!node.skip_contained_object)
                {
                    if (result_.exists(val))
                    {
                        nodes.emplace_back(path, std::addressof(val));
                    }
                }
                else
                {
                    node.skip_contained_object = false;
                }
            }
        }
    };

    class name_selector final : public selector
    {
    private:
        string_type name_;
        bool positive_start_;
    public:
        name_selector(const string_view_type& name, bool positive_start)
            : name_(name), positive_start_(positive_start)
        {
        }

        void select(jsonpath_evaluator& evaluator,
                    node_type&, const string_type& path, reference val,
                    node_set& nodes) override
        {
            if (val.is_object() && val.contains(name_))
            {
                nodes.emplace_back(PathCons()(path,name_),std::addressof(val.at(name_)));
            }
            else if (val.is_array())
            {
                size_t pos = 0;
                if (try_string_to_index(name_.data(), name_.size(), &pos, &positive_start_))
                {
                    size_t index = positive_start_ ? pos : val.size() - pos;
                    if (index < val.size())
                    {
                        nodes.emplace_back(PathCons()(path,index),std::addressof(val[index]));
                    }
                }
                else if (name_ == length_literal() && val.size() > 0)
                {
                    pointer ptr = evaluator.create_temp(val.size());
                    nodes.emplace_back(PathCons()(path, name_), ptr);
                }
            }
            else if (val.is_string())
            {
                size_t pos = 0;
                string_view_type sv = val.as_string_view();
                if (try_string_to_index(name_.data(), name_.size(), &pos, &positive_start_))
                {
                    size_t index = positive_start_ ? pos : sv.size() - pos;
                    auto sequence = unicons::sequence_at(sv.data(), sv.data() + sv.size(), index);
                    if (sequence.length() > 0)
                    {
                        pointer ptr = evaluator.create_temp(sequence.begin(),sequence.length());
                        nodes.emplace_back(PathCons()(path, index), ptr);
                    }
                }
                else if (name_ == length_literal() && sv.size() > 0)
                {
                    size_t count = unicons::u32_length(sv.begin(),sv.end());
                    pointer ptr = evaluator.create_temp(count);
                    nodes.emplace_back(PathCons()(path, name_), ptr);
                }
            }
        }
    };

    class array_slice_selector final : public selector
    {
    private:
        size_t start_;
        bool positive_start_;
        size_t end_;
        bool positive_end_;
        bool undefined_end_;
        size_t step_;
        bool positive_step_;
    public:
        array_slice_selector(size_t start, bool positive_start, 
                             size_t end, bool positive_end,
                             size_t step, bool positive_step,
                             bool undefined_end)
            : start_(start), positive_start_(positive_start),
              end_(end), positive_end_(positive_end),undefined_end_(undefined_end),
              step_(step), positive_step_(positive_step) 
        {
        }

        void select(jsonpath_evaluator&,
                    node_type&, const string_type& path, reference val,
                    node_set& nodes) override
        {
            if (positive_step_)
            {
                end_array_slice1(path, val, nodes);
            }
            else
            {
                end_array_slice2(path, val, nodes);
            }
        }

        void end_array_slice1(const string_type& path, reference val, node_set& nodes)
        {
            if (val.is_array())
            {
                size_t start = positive_start_ ? start_ : val.size() - start_;
                size_t end;
                if (!undefined_end_)
                {
                    end = positive_end_ ? end_ : val.size() - end_;
                }
                else
                {
                    end = val.size();
                }
                for (size_t j = start; j < end; j += step_)
                {
                    if (j < val.size())
                    {
                        nodes.emplace_back(PathCons()(path,j),std::addressof(val[j]));
                    }
                }
            }
        }

        void end_array_slice2(const string_type& path, reference val, node_set& nodes)
        {
            if (val.is_array())
            {
                size_t start = positive_start_ ? start_ : val.size() - start_;
                size_t end;
                if (!undefined_end_)
                {
                    end = positive_end_ ? end_ : val.size() - end_;
                }
                else
                {
                    end = val.size();
                }

                size_t j = end + step_ - 1;
                while (j > (start+step_-1))
                {
                    j -= step_;
                    if (j < val.size())
                    {
                        nodes.emplace_back(PathCons()(path,j),std::addressof(val[j]));
                    }
                }
            }
        }
    };

    function_table<Json,pointer> functions_;

    default_parse_error_handler default_err_handler_;
    path_state state_;
    string_type buffer_;
    size_t start_;
    bool positive_start_;
    size_t end_;
    bool positive_end_;
    bool undefined_end_;
    size_t step_;
    bool positive_step_;
    bool recursive_descent_;
    node_set nodes_;
    std::vector<node_set> stack_;
    size_t line_;
    size_t column_;
    const char_type* begin_input_;
    const char_type* end_input_;
    const char_type* p_;
    std::vector<std::unique_ptr<selector>> selectors_;
    std::vector<std::unique_ptr<Json>> temp_json_values_;

    typedef std::vector<pointer> argument_type;
    std::vector<argument_type> function_stack_;

public:
    jsonpath_evaluator()
        : state_(path_state::start),
          start_(0), positive_start_(true), 
          end_(0), positive_end_(true), undefined_end_(false),
          step_(0), positive_step_(true),
          recursive_descent_(false),
          line_(0), column_(0),
          begin_input_(nullptr), end_input_(nullptr),
          p_(nullptr)
    {
    }

    Json get_values() const
    {
        Json result = typename Json::array();

        if (stack_.size() > 0)
        {
            result.reserve(stack_.back().size());
            for (const auto& p : stack_.back())
            {
                result.push_back(*(p.val_ptr));
            }
        }
        return result;
    }

    std::vector<pointer> get_pointers() const
    {
        std::vector<pointer> result;

        if (stack_.size() > 0)
        {
            result.reserve(stack_.back().size());
            for (const auto& p : stack_.back())
            {
                result.push_back(p.val_ptr);
            }
        }
        return result;
    }

    void invoke_function(const string_type& function_name, std::error_code& ec)
    {
        auto f = functions_.get(function_name, ec);
        if (ec)
        {
            return;
        }
        auto result = f(function_stack_, ec);
        if (ec)
        {
            return;
        }

        string_type s;
        s.push_back('$');

        node_set v;
        pointer ptr = create_temp(std::move(result));
        v.emplace_back(s,ptr);
        stack_.push_back(v);
    }

    template <typename... Args>
    pointer create_temp(Args&& ... args)
    {
        auto temp = make_unique_ptr<Json>(std::forward<Args>(args)...);
        pointer ptr = temp.get();
        temp_json_values_.emplace_back(std::move(temp));
        return ptr;
    }

    Json get_normalized_paths() const
    {
        Json result = typename Json::array();
        if (stack_.size() > 0)
        {
            result.reserve(stack_.back().size());
            for (const auto& p : stack_.back())
            {
                result.push_back(p.path);
            }
        }
        return result;
    }

    template <class T>
    void replace(T&& new_value)
    {
        if (stack_.size() > 0)
        {
            for (size_t i = 0; i < stack_.back().size(); ++i)
            {
                *(stack_.back()[i].val_ptr) = new_value;
            }
        }
    }

    void evaluate(reference root, const string_view_type& path)
    {
        std::error_code ec;
        evaluate(root, path.data(), path.length(), ec);
        if (ec)
        {
            throw jsonpath_error(ec, line_, column_);
        }
    }

    void evaluate(reference root, const string_view_type& path, std::error_code& ec)
    {
        try
        {
            evaluate(root, path.data(), path.length(), ec);
        }
        catch (...)
        {
            ec = jsonpath_errc::unidentified_error;
        }
    }

    void evaluate(reference root, 
                  const char_type* path, 
                  size_t length,
                  std::error_code& ec)
    {
        string_type function_name;
        path_state pre_line_break_state = path_state::start;

        begin_input_ = path;
        end_input_ = path + length;
        p_ = begin_input_;

        line_ = 1;
        column_ = 1;
        state_ = path_state::start;

        recursive_descent_ = false;

        clear_index();

        while (p_ < end_input_)
        {
            switch (state_)
            {
                case path_state::cr:
                    ++line_;
                    column_ = 1;
                    switch (*p_)
                    {
                    case '\n':
                        state_ = pre_line_break_state;
                        ++p_;
                        ++column_;
                        break;
                    default:
                        state_ = pre_line_break_state;
                        break;
                    }
                    break;
                case path_state::lf:
                    ++line_;
                    column_ = 1;
                    state_ = pre_line_break_state;
                    break;
                case path_state::start: 
                    switch (*p_)
                    {
                        case ' ':case '\t':
                            break;
                        case '$':
                        {
                            string_type s;
                            s.push_back(*p_);
                            node_set v;
                            v.emplace_back(std::move(s),std::addressof(root));
                            stack_.push_back(v);

                            state_ = path_state::expect_dot_or_left_bracket;
                            break;
                        }
                        default:
                        {
                            switch (*p_)
                            {
                                case '.':
                                case '[':
                                    ec = jsonpath_errc::expected_root;
                                    return;
                                default: // might be function, validate name later
                                    state_ = path_state::function_name;
                                    function_name.push_back(*p_);
                                    break;
                            }
                            break;
                        }

                        return;
                    };
                    ++p_;
                    ++column_;
                    break;
                case path_state::function_name:
                    switch (*p_)
                    {
                        case '(':
                            state_ = path_state::expect_arg_or_right_round_bracket;
                            break;
                        default:
                            function_name.push_back(*p_);
                            break;
                    }
                    ++p_;
                    ++column_;
                    break;
                case path_state::expect_arg_or_right_round_bracket:
                    switch (*p_)
                    {
                        case ' ':
                        case '\t':
                            break;
                        case '$':
                            buffer_.clear();
                            buffer_.push_back(*p_);
                            state_ = path_state::path_argument;
                            break;
                        case '\'':
                            buffer_.clear();
                            buffer_.push_back('\"');
                            state_ = path_state::single_quoted_argument;
                            break;
                        case '\"':
                            buffer_.clear();
                            buffer_.push_back('\"');
                            state_ = path_state::double_quoted_argument;
                            break;
                        default:
                            buffer_.clear();
                            state_ = path_state::unquoted_argument;
                            break;
                    }
                    ++p_;
                    ++column_;
                    break;
                case path_state::path_argument:
                    switch (*p_)
                    {
                        case ',':
                        {
                            jsonpath_evaluator<Json, JsonReference, PathCons> evaluator;
                            evaluator.evaluate(root, buffer_, ec);
                            if (ec)
                            {
                                return;
                            }
                            function_stack_.push_back(evaluator.get_pointers());
                            state_ = path_state::expect_arg_or_right_round_bracket;
                            break;
                        }
                        case ')':
                        {
                            jsonpath_evaluator<Json,JsonReference,PathCons> evaluator;
                            evaluator.evaluate(root, buffer_, ec);
                            if (ec)
                            {
                                return;
                            }
                            function_stack_.push_back(evaluator.get_pointers());

                            invoke_function(function_name, ec);
                            if (ec)
                            {
                                return;
                            }

                            state_ = path_state::expect_dot_or_left_bracket;
                            break;
                        }
                        default:
                            buffer_.push_back(*p_);
                            break;
                    }
                    ++p_;
                    ++column_;
                    break;
                case path_state::unquoted_argument:
                    switch (*p_)
                    {
                        case ',':
                            try
                            {
                                auto val = Json::parse(buffer_);
                                auto temp = create_temp(val);
                                function_stack_.push_back(std::vector<pointer>{temp});
                            }
                            catch (const ser_error&)
                            {
                                ec = jsonpath_errc::argument_parse_error;
                                return;
                            }
                            buffer_.clear();
                            state_ = path_state::expect_arg_or_right_round_bracket;
                            break;
                        case ')':
                        {
                            try
                            {
                                auto val = Json::parse(buffer_);
                                auto temp = create_temp(val);
                                function_stack_.push_back(std::vector<pointer>{temp});
                            }
                            catch (const ser_error&)
                            {
                                ec = jsonpath_errc::argument_parse_error;
                                return;
                            }
                            invoke_function(function_name, ec);
                            if (ec)
                            {
                                return;
                            }
                            state_ = path_state::expect_dot_or_left_bracket;
                            break;
                        }
                        default:
                            buffer_.push_back(*p_);
                            break;
                    }
                    ++p_;
                    ++column_;
                    break;
                case path_state::single_quoted_argument:
                    switch (*p_)
                    {
                        case '\'':
                            buffer_.push_back('\"');
                            state_ = path_state::expect_more_args_or_right_round_bracket;
                            break;
                        default:
                            buffer_.push_back(*p_);
                            break;
                    }
                    ++p_;
                    ++column_;
                    break;
                case path_state::double_quoted_argument:
                    switch (*p_)
                    {
                        case '\"':
                            buffer_.push_back('\"');
                            state_ = path_state::expect_more_args_or_right_round_bracket;
                            break;
                        default:
                            buffer_.push_back(*p_);
                            break;
                    }
                    ++p_;
                    ++column_;
                    break;
                case path_state::expect_more_args_or_right_round_bracket:
                    switch (*p_)
                    {
                        case ' ':
                        case '\t':
                            break;
                        case ',':
                            try
                            {
                                auto val = Json::parse(buffer_);
                                auto temp = create_temp(val);
                                function_stack_.push_back(std::vector<pointer>{temp});
                            }
                            catch (const ser_error&)
                            {
                                ec = jsonpath_errc::argument_parse_error;
                                return;
                            }
                            buffer_.clear();
                            state_ = path_state::expect_arg_or_right_round_bracket;
                            break;
                        case ')':
                        {
                            try
                            {
                                auto val = Json::parse(buffer_);
                                auto temp = create_temp(val);
                                function_stack_.push_back(std::vector<pointer>{temp});
                            }
                            catch (const ser_error&)
                            {
                                ec = jsonpath_errc::argument_parse_error;
                                return;
                            }
                            invoke_function(function_name, ec);
                            if (ec)
                            {
                                return;
                            }
                            state_ = path_state::expect_dot_or_left_bracket;
                            break;
                        }
                        default:
                            ec = jsonpath_errc::invalid_filter_unsupported_operator;
                            return;
                    }
                    ++p_;
                    ++column_;
                    break;
                case path_state::dot:
                    switch (*p_)
                    {
                    case '.':
                        recursive_descent_ = true;
                        ++p_;
                        ++column_;
                        state_ = path_state::expect_unquoted_name_or_left_bracket;
                        break;
                    default:
                        state_ = path_state::expect_unquoted_name_or_left_bracket;
                        break;
                    }
                    break;
                case path_state::expect_unquoted_name_or_left_bracket:
                    switch (*p_)
                    {
                    case '.':
                        ec = jsonpath_errc::expected_name;
                        return;
                    case '*':
                        end_all();
                        transfer_nodes();
                        state_ = path_state::expect_dot_or_left_bracket;
                        ++p_;
                        ++column_;
                        break;
                    case '[':
                        state_ = path_state::left_bracket;
                        ++p_;
                        ++column_;
                        break;
                    default:
                        buffer_.clear();
                        state_ = path_state::unquoted_name;
                        break;
                    }
                    break;
            case path_state::expect_dot_or_left_bracket: 
                switch (*p_)
                {
                case ' ':case '\t':
                    break;
                case '.':
                    state_ = path_state::dot;
                    break;
                case '[':
                    state_ = path_state::left_bracket;
                    break;
                default:
                    ec = jsonpath_errc::expected_separator;
                    return;
                };
                ++p_;
                ++column_;
                break;
            case path_state::expect_comma_or_right_bracket:
                switch (*p_)
                {
                case ',':
                    state_ = path_state::left_bracket;
                    break;
                case ']':
                    apply_selectors();
                    state_ = path_state::expect_dot_or_left_bracket;
                    break;
                case ' ':case '\t':
                    break;
                default:
                    ec = jsonpath_errc::expected_right_bracket;
                    return;
                }
                ++p_;
                ++column_;
                break;
            case path_state::left_bracket:
                switch (*p_)
                {
                case ' ':case '\t':
                    ++p_;
                    ++column_;
                    break;
                case '(':
                    {
                        jsonpath_filter_parser<Json> parser(line_,column_);
                        auto result = parser.parse(root, p_,end_input_,&p_);
                        line_ = parser.line();
                        column_ = parser.column();
                        selectors_.push_back(make_unique_ptr<expr_selector>(result));
                        state_ = path_state::expect_comma_or_right_bracket;
                    }
                    break;
                case '?':
                    {
                        jsonpath_filter_parser<Json> parser(line_,column_);
                        auto result = parser.parse(root,p_,end_input_,&p_);
                        line_ = parser.line();
                        column_ = parser.column();
                        selectors_.push_back(make_unique_ptr<filter_selector>(result));
                        state_ = path_state::expect_comma_or_right_bracket;
                    }
                    break;                   
                case ':':
                    clear_index();
                    state_ = path_state::left_bracket_end;
                    ++p_;
                    ++column_;
                    break;
                case '*':
                    end_all();
                    state_ = path_state::expect_comma_or_right_bracket;
                    ++p_;
                    ++column_;
                    break;
                case '\'':
                    state_ = path_state::left_bracket_single_quoted_string;
                    ++p_;
                    ++column_;
                    break;
                case '\"':
                    state_ = path_state::left_bracket_double_quoted_string;
                    ++p_;
                    ++column_;
                    break;
                default:
                    clear_index();
                    buffer_.push_back(*p_);
                    state_ = path_state::left_bracket_start;
                    ++p_;
                    ++column_;
                    break;
                }
                break;
            case path_state::left_bracket_start:
                switch (*p_)
                {
                case ':':
                    if (!try_string_to_index(buffer_.data(), buffer_.size(), &start_, &positive_start_))
                    {
                        ec = jsonpath_errc::expected_index;
                        return;
                    }
                    state_ = path_state::left_bracket_end;
                    break;
                case ',':
                    selectors_.push_back(make_unique_ptr<name_selector>(buffer_,positive_start_));
                    buffer_.clear();
                    state_ = path_state::left_bracket;
                    break;
                case ']':
                    selectors_.push_back(make_unique_ptr<name_selector>(buffer_,positive_start_));
                    buffer_.clear();
                    apply_selectors();
                    state_ = path_state::expect_dot_or_left_bracket;
                    break;
                default:
                    buffer_.push_back(*p_);
                    break;
                }
                ++p_;
                ++column_;
                break;
            case path_state::left_bracket_end:
                switch (*p_)
                {
                case '-':
                    positive_end_ = false;
                    state_ = path_state::left_bracket_end2;
                    break;
                case ':':
                    step_ = 0;
                    state_ = path_state::left_bracket_step;
                    break;
                case '0':case '1':case '2':case '3':case '4':case '5':case '6':case '7':case '8':case '9':
                    undefined_end_ = false;
                    end_ = static_cast<size_t>(*p_-'0');
                    state_ = path_state::left_bracket_end2;
                    break;
                case ',':
                    selectors_.push_back(make_unique_ptr<array_slice_selector>(start_,positive_start_,end_,positive_end_,step_,positive_step_,undefined_end_));
                    state_ = path_state::left_bracket;
                    break;
                case ']':
                    selectors_.push_back(make_unique_ptr<array_slice_selector>(start_,positive_start_,end_,positive_end_,step_,positive_step_,undefined_end_));
                    apply_selectors();
                    state_ = path_state::expect_dot_or_left_bracket;
                    break;
                }
                ++p_;
                ++column_;
                break;
            case path_state::left_bracket_end2:
                switch (*p_)
                {
                case ':':
                    step_ = 0;
                    state_ = path_state::left_bracket_step;
                    break;
                case '0':case '1':case '2':case '3':case '4':case '5':case '6':case '7':case '8':case '9':
                    undefined_end_ = false;
                    end_ = end_*10 + static_cast<size_t>(*p_-'0');
                    break;
                case ',':
                    selectors_.push_back(make_unique_ptr<array_slice_selector>(start_,positive_start_,end_,positive_end_,step_,positive_step_,undefined_end_));
                    state_ = path_state::left_bracket;
                    break;
                case ']':
                    selectors_.push_back(make_unique_ptr<array_slice_selector>(start_,positive_start_,end_,positive_end_,step_,positive_step_,undefined_end_));
                    apply_selectors();
                    state_ = path_state::expect_dot_or_left_bracket;
                    break;
                }
                ++p_;
                ++column_;
                break;
            case path_state::left_bracket_step:
                switch (*p_)
                {
                case '-':
                    positive_step_ = false;
                    state_ = path_state::left_bracket_step2;
                    break;
                case '0':case '1':case '2':case '3':case '4':case '5':case '6':case '7':case '8':case '9':
                    step_ = static_cast<size_t>(*p_-'0');
                    state_ = path_state::left_bracket_step2;
                    break;
                case ',':
                    selectors_.push_back(make_unique_ptr<array_slice_selector>(start_,positive_start_,end_,positive_end_,step_,positive_step_,undefined_end_));
                    state_ = path_state::left_bracket;
                    break;
                case ']':
                    selectors_.push_back(make_unique_ptr<array_slice_selector>(start_,positive_start_,end_,positive_end_,step_,positive_step_,undefined_end_));
                    apply_selectors();
                    state_ = path_state::expect_dot_or_left_bracket;
                    break;
                }
                ++p_;
                ++column_;
                break;
            case path_state::left_bracket_step2:
                switch (*p_)
                {
                case '0':case '1':case '2':case '3':case '4':case '5':case '6':case '7':case '8':case '9':
                    step_ = step_*10 + static_cast<size_t>(*p_-'0');
                    break;
                case ',':
                    selectors_.push_back(make_unique_ptr<array_slice_selector>(start_,positive_start_,end_,positive_end_,step_,positive_step_,undefined_end_));
                    state_ = path_state::left_bracket;
                    break;
                case ']':
                    selectors_.push_back(make_unique_ptr<array_slice_selector>(start_,positive_start_,end_,positive_end_,step_,positive_step_,undefined_end_));
                    apply_selectors();
                    state_ = path_state::expect_dot_or_left_bracket;
                    break;
                }
                ++p_;
                ++column_;
                break;
            case path_state::unquoted_name: 
                switch (*p_)
                {
                case '[':
                    apply_unquoted_string(buffer_);
                    transfer_nodes();
                    start_ = 0;
                    state_ = path_state::left_bracket;
                    break;
                case '.':
                    apply_unquoted_string(buffer_);
                    transfer_nodes();
                    state_ = path_state::dot;
                    break;
                case ' ':case '\t':
                    apply_unquoted_string(buffer_);
                    transfer_nodes();
                    state_ = path_state::expect_dot_or_left_bracket;
                    break;
                case '\r':
                    apply_unquoted_string(buffer_);
                    transfer_nodes();
                    pre_line_break_state = path_state::expect_dot_or_left_bracket;
                    state_= path_state::cr;
                    break;
                case '\n':
                    apply_unquoted_string(buffer_);
                    transfer_nodes();
                    pre_line_break_state = path_state::expect_dot_or_left_bracket;
                    state_= path_state::lf;
                    break;
                default:
                    buffer_.push_back(*p_);
                    break;
                };
                ++p_;
                ++column_;
                break;
            case path_state::left_bracket_single_quoted_string: 
                switch (*p_)
                {
                case '\'':
                    selectors_.push_back(make_unique_ptr<name_selector>(buffer_,positive_start_));
                    buffer_.clear();
                    state_ = path_state::expect_comma_or_right_bracket;
                    break;
                case '\\':
                    buffer_.push_back(*p_);
                    if (p_+1 < end_input_)
                    {
                        ++p_;
                        ++column_;
                        buffer_.push_back(*p_);
                    }
                    break;
                default:
                    buffer_.push_back(*p_);
                    break;
                };
                ++p_;
                ++column_;
                break;
            case path_state::left_bracket_double_quoted_string: 
                switch (*p_)
                {
                case '\"':
                    selectors_.push_back(make_unique_ptr<name_selector>(buffer_,positive_start_));
                    buffer_.clear();
                    state_ = path_state::expect_comma_or_right_bracket;
                    break;
                case '\\':
                    buffer_.push_back(*p_);
                    if (p_+1 < end_input_)
                    {
                        ++p_;
                        ++column_;
                        buffer_.push_back(*p_);
                    }
                    break;
                default:
                    buffer_.push_back(*p_);
                    break;
                };
                ++p_;
                ++column_;
                break;
            default:
                ++p_;
                ++column_;
                break;
            }
        }
        switch (state_)
        {
        case path_state::unquoted_name: 
            {
                apply_unquoted_string(buffer_);
                transfer_nodes();
            }
            break;
        default:
            break;
        }
    }

    void clear_index()
    {
        buffer_.clear();
        start_ = 0;
        positive_start_ = true;
        end_ = 0;
        positive_end_ = true;
        undefined_end_ = true;
        step_ = 1;
        positive_step_ = true;
    }

    void end_all()
    {
        for (size_t i = 0; i < stack_.back().size(); ++i)
        {
            const auto& path = stack_.back()[i].path;
            pointer p = stack_.back()[i].val_ptr;

            if (p->is_array())
            {
                for (auto it = p->array_range().begin(); it != p->array_range().end(); ++it)
                {
                    nodes_.emplace_back(PathCons()(path,it - p->array_range().begin()),std::addressof(*it));
                }
            }
            else if (p->is_object())
            {
                for (auto it = p->object_range().begin(); it != p->object_range().end(); ++it)
                {
                    nodes_.emplace_back(PathCons()(path,it->key()),std::addressof(it->value()));
                }
            }

        }
        start_ = 0;
    }

    void apply_unquoted_string(const string_view_type& name)
    {
        if (name.length() > 0)
        {
            for (size_t i = 0; i < stack_.back().size(); ++i)
            {
                apply_unquoted_string(stack_.back()[i].path, *(stack_.back()[i].val_ptr), name);
            }
        }
        buffer_.clear();
    }

    void apply_unquoted_string(const string_type& path, reference val, const string_view_type& name)
    {
        if (val.is_object())
        {
            if (val.contains(name))
            {
                nodes_.emplace_back(PathCons()(path,name),std::addressof(val.at(name)));
            }
            if (recursive_descent_)
            {
                for (auto it = val.object_range().begin(); it != val.object_range().end(); ++it)
                {
                    if (it->value().is_object() || it->value().is_array())
                    {
                        apply_unquoted_string(path, it->value(), name);
                    }
                }
            }
        }
        else if (val.is_array())
        {
            size_t pos = 0;
            if (try_string_to_index(name.data(),name.size(),&pos, &positive_start_))
            {
                size_t index = positive_start_ ? pos : val.size() - pos;
                if (index < val.size())
                {
                    nodes_.emplace_back(PathCons()(path,index),std::addressof(val[index]));
                }
            }
            else if (name == length_literal() && val.size() > 0)
            {
                pointer ptr = create_temp(val.size());
                nodes_.emplace_back(PathCons()(path,name),ptr);
            }
            if (recursive_descent_)
            {
                for (auto it = val.array_range().begin(); it != val.array_range().end(); ++it)
                {
                    if (it->is_object() || it->is_array())
                    {
                        apply_unquoted_string(path, *it, name);
                    }
                }
            }
        }
        else if (val.is_string())
        {
            string_view_type sv = val.as_string_view();
            size_t pos = 0;
            if (try_string_to_index(name.data(),name.size(),&pos, &positive_start_))
            {
                auto sequence = unicons::sequence_at(sv.data(), sv.data() + sv.size(), pos);
                if (sequence.length() > 0)
                {
                    pointer ptr = create_temp(sequence.begin(),sequence.length());
                    nodes_.emplace_back(PathCons()(path,pos),ptr);
                }
            }
            else if (name == length_literal() && sv.size() > 0)
            {
                size_t count = unicons::u32_length(sv.begin(),sv.end());
                pointer ptr = create_temp(count);
                nodes_.emplace_back(PathCons()(path,name),ptr);
            }
        }
    }

    void apply_selectors()
    {
        if (selectors_.size() > 0)
        {
            for (size_t i = 0; i < stack_.back().size(); ++i)
            {
                node_type& node = stack_.back()[i];
                apply_selectors(node, node.path, *(node.val_ptr));
            }
            selectors_.clear();
        }
        transfer_nodes();
    }

    void apply_selectors(node_type& node, const string_type& path, reference val)
    {
        for (const auto& selector : selectors_)
        {
            selector->select(*this, node, path, val, nodes_);
        }
        if (recursive_descent_)
        {
            if (val.is_object())
            {
                for (auto& nvp : val.object_range())
                {
                    if (nvp.value().is_object() || nvp.value().is_array())
                    {                        
                        apply_selectors(node,PathCons()(path,nvp.key()),nvp.value());
                    }
                }
            }
            else if (val.is_array())
            {
                for (auto& elem : val.array_range())
                {
                    if (elem.is_object() || elem.is_array())
                    {
                        apply_selectors(node,path, elem);
                    }
                }
            }
        }
    }

    void transfer_nodes()
    {
        stack_.push_back(nodes_);
        nodes_.clear();
        recursive_descent_ = false;
    }

    size_t line_number() const override
    {
        return line_;
    }

    size_t column_number() const override
    {
        return column_;
    }

};

}

}}

#endif
