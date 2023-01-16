#ifndef MY_JSON
#define MY_JSON

#include <fstream>
#include <vector>
#include <string>
#include <unordered_map>
#include <memory>
#include <exception>
#include <sstream>

enum class JsonType : uint8_t {
    NIL,
    STRING,
    NUMBER,
    BOOL,
    ARRAY,
    OBJECT
};

// The parent interface provides default implementations of all access methods that only throw exceptions.
// The interface itself can be used to represent the null type, because nothing can be obtained from this type.
struct JsonNode {
    inline virtual JsonType get_type() {
        return JsonType::NIL;
    }
    inline virtual std::string& get_string() {
        throw(std::runtime_error("not a string"));
    }
    inline virtual double& get_double() {
        throw(std::runtime_error("not a double"));
    }
    inline virtual bool& get_bool() {
        throw(std::runtime_error("not a bool"));
    }
    inline virtual std::vector<std::shared_ptr<JsonNode>>& get_vector() {
        throw(std::runtime_error("not an array"));
    }
    inline virtual std::unordered_map<std::string, std::shared_ptr<JsonNode>>& get_object() {
        throw(std::runtime_error("not an object"));
    }
    inline virtual void write(std::ostream& out, int = 0) {
        out << "null";
    }
    inline void write_to_file(const std::string& filename) {
        std::ofstream out(filename);
        if (!out.good()) 
            throw(std::runtime_error("Could not write to file " + filename));
        this->write(out, 0);
    }

    virtual ~JsonNode() = default;
protected:
    static void write_string(std::ostream& out, const std::string& written) {
        out.put('"');
        for (unsigned int i = 0; i < written.size(); i++) {
            if (written[i] == '"') {
                out.put('\\');
                out.put('"');
            } else if (written[i] == '\n') {
                out.put('\\');
                out.put('n');
            } else if (written[i] == '\\') {
                out.put('\\');
                out.put('\\');
            } else
                out.put(written[i]);
        }
        out.put('"');
    }
    static void indent(std::ostream& out, int depth) {
        for (int i = 0; i < depth; i++)
            out.put('\t');
    }
};

// All types can be represented by similar C++ structures from the C++11 standard:
// null is not represented
// string is represented by std::string
// number is represented by double
// boolean is represented by bool
// object is represented by std::unordered_map<std::string, std::shared_ptr<JSON>>
// array is represented by std::vector<std::shared_ptr<JSON>>

struct JsonString : public JsonNode {
    std::string contents_;
    JsonString(const std::string& from = "") : contents_(from) {}

    inline virtual JsonType get_type() {
        return JsonType::STRING;
    }
    inline virtual std::string& get_string() {
        return contents_;
    }
    inline void write(std::ostream& out, int = 0) {
        write_string(out, contents_);
    }
};
struct JsonDouble : public JsonNode {
    double value_;
    JsonDouble(double from = 0) : value_(from) {}

    inline virtual JsonType get_type() {
        return JsonType::NUMBER;
    }
    inline virtual double& get_double() {
        return value_;
    }
    inline void write(std::ostream& out, int = 0) {
        out << value_;
    }
};
struct JsonBool : public JsonNode {
    bool value_;
    JsonBool(bool from = false) : value_(from) {}

    inline virtual JsonType get_type() {
        return JsonType::BOOL;
    }
    inline virtual bool& get_bool() {
        return value_;
    }
    inline void write(std::ostream& out, int = 0) {
        out << (value_ ? "true" : "false");
    }
};
struct JsonObject : public JsonNode {
    std::unordered_map<std::string, std::shared_ptr<JsonNode>> contents_;
    JsonObject() {}

    inline virtual JsonType get_type() {
        return JsonType::OBJECT;
    }
    inline virtual std::unordered_map<std::string, std::shared_ptr<JsonNode>>& get_object() {
        return contents_;
    }
    inline void write(std::ostream& out, int depth = 0) {
        if (contents_.empty()) {
            out.put('{');
            out.put('}');
            return;
        }
        out.put('{');
        out.put('\n');
        bool first_ele = true;
        for (auto& it : contents_) {
            if (first_ele)
                first_ele = false;
            else {
                out.put(',');
                out.put('\n');
            }
            indent(out, depth + 1);
            write_string(out, it.first);
            out.put(':');
            out.put(' ');
            it.second->write(out, depth + 1);
        }
        out.put('\n');
        indent(out, depth);
        out.put('}');
    }
};
struct JsonArray : public JsonNode {
    std::vector<std::shared_ptr<JsonNode>> contents_;
    JsonArray() {}

    inline virtual JsonType get_type() {
        return JsonType::ARRAY;
    }
    inline virtual std::vector<std::shared_ptr<JsonNode>>& get_vector() {
        return contents_;
    }
    inline void write(std::ostream& out, int depth = 0) {
        out.put('[');
        if (contents_.empty()) {
            out.put(']');
            return;
        }
        for (auto& it : contents_) {
            out.put('\n');
            indent(out, depth);
            indent(out, depth);
            it->write(out, depth + 1);
        }
        out.put('\n');
        indent(out, depth);
        out.put(']');
        
    }
};

// A recursive function that parses the input character by character.
static std::shared_ptr<JsonNode> parse_json(std::istream& in) {
    // read until "
    auto read_string = [&in] () -> std::string {
        char letter = in.get();
        std::string collected;
        while (letter != '"') //the end of string
        {
            if (letter == '\\') {
                if (in.get() == '"') collected.push_back('"');
                else if (in.get() == 'n') collected.push_back('\n');
                else if (in.get() == '\\') collected.push_back('\\');
            } else {
                collected.push_back(letter);
            }
            letter = in.get();
        }
        return collected;
    };
    // ignore whitespace and get first char after whitespace
    auto read_whitespace = [&in] () -> char {
        char letter;
        do {
            letter = in.get();
        } while (letter == ' ' || letter == '\t' || letter == '\n' || letter == ',');
        return letter;
    };

    char letter = read_whitespace();
    if (letter == 0 || letter == EOF) return std::make_shared<JsonNode>();
    else if (letter == '"') {
        return std::make_shared<JsonString>(read_string());
    }
    else if (letter == 't') {
        if (in.get() == 'r' && in.get() == 'u' && in.get() == 'e')
            return std::make_shared<JsonBool>(true);
        else
            throw(std::runtime_error("JSON parser found misspelled bool 'true'"));
    }
    else if (letter == 'f') {
        if (in.get() == 'a' && in.get() == 'l' && in.get() == 's' && in.get() == 'e')
            return std::make_shared<JsonBool>(false);
        else
            throw(std::runtime_error("JSON parser found misspelled bool 'false'"));
    }
    else if (letter == 'n') {
        if (in.get() == 'u' && in.get() == 'l' && in.get() == 'l')
            return std::make_shared<JsonNode>();
        else
            throw(std::runtime_error("JSON parser found misspelled bool 'null'"));
    }
    else if (letter == '-' || (letter >= '0' && letter <= '9')) {
        std::string asString;
        asString.push_back(letter);
        do {
            letter = in.get();
            asString.push_back(letter);
        } while (letter == '-' || letter == 'E' || letter == 'e' || letter == ',' || letter == '.' || (letter >= '0' && letter <= '9'));
        in.unget();
        std::stringstream parsing(asString);
        double number;
        parsing >> number;
        return std::make_shared<JsonDouble>(number);
    }
    else if (letter == '{') {
        auto retval = std::make_shared<JsonObject>();
        do {
            letter = read_whitespace();
            if (letter == '"') {
                const std::string& name = read_string();
                letter = read_whitespace();
                if (letter != ':') throw(std::runtime_error("JSON parser expected an additional ':' somewhere"));
                retval->get_object()[name] = parse_json(in);
            } else break;
        } while (letter != '}');
        return retval;
    }
    else if (letter == '[') {
        auto retval = std::make_shared<JsonArray>();
        do {
            letter = read_whitespace();
            if (letter == '{') {
                in.unget();
                retval->get_vector().push_back(parse_json(in));
            } else break;
        } while (letter != ']');
        return retval;
    } else {
        throw(std::runtime_error("JSON parser found unexpected character " + letter));
    }
    return std::make_shared<JsonNode>();
}
// read from file and parse
static std::shared_ptr<JsonNode> parse_json(const std::string& filename) {
    std::ifstream in(filename);
    if (!in.good()) return std::make_shared<JsonNode>();
    return parse_json(in);
}


#endif //MY_JSON