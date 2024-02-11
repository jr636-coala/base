#pragma once
#include <algorithm>
#include <cstdint>
#include <functional>
#include <iterator>
#include <numeric>
#include <string_view>
#include <unordered_set>
#include <vector>
typedef uint64_t u64; typedef int64_t i64;
typedef uint32_t u32; typedef int32_t i32;
typedef uint16_t u16; typedef int16_t i16;
typedef uint8_t u8;   typedef int8_t i8;

template <>
struct std::hash<std::unordered_set<i32>> {
  std::size_t operator()(const std::unordered_set<i32>& k) const {
    return std::reduce(k.begin(), k.end(), 0, [](auto acc, auto x){ return acc + x; });
  }
};

template <typename T, T T_NULL, T (*TF)(T,T)>
struct RegexLexer {
  using transition_table_t = std::unordered_map<i32, std::unordered_map<char, std::vector<i32>>>;
  using state_t = T;

  constexpr RegexLexer() : start(0), transitions({}), accept({}), states({ T_NULL }), alphabet({}) {}

  constexpr auto addState() { states.push_back(T_NULL); return states.size() - 1; }

  constexpr RegexLexer& concat(char c, const std::optional<T> t = {}) {
    const i32 ne = addState();
    if (t.has_value()) states[ne] = T_NULL;
    std::for_each(accept.begin(), accept.end(), [&](auto n) { transitions[n][c].push_back(ne); });
    if (!accept.size()) transitions[start][c].push_back(ne);
    accept = { ne };
    if (std::find(alphabet.begin(), alphabet.end(), c) == alphabet.end()) alphabet.push_back(c);
    return *this;
  }

  constexpr RegexLexer& concat(const RegexLexer& x) {
    if (!accept.size()) return *this = x;
    const auto maxx = states.size();
    std::for_each(accept.begin(), accept.end(), [&](auto n) { transitions[n]['\0'].push_back(x.start + maxx); });
    std::copy_if(x.alphabet.begin(), x.alphabet.end(), std::back_inserter(alphabet), [&](auto c) {
      return std::find(alphabet.begin(), alphabet.end(), c) == alphabet.end();
    });
    for (const auto& [s1, m] : x.transitions) {
      for (const auto& [c, v] : m) {
        auto& tr = transitions[s1 + maxx][c];
        for (const auto& s2 : v) tr.push_back(s2 + maxx);
      }
    }
    states.insert(states.end(), x.states.begin(), x.states.end());
    accept = x.accept;
    std::transform(accept.begin(), accept.end(), accept.begin(), [&](auto n) { return n + maxx; });
    return *this;
  }


  RegexLexer& concatClass(const std::string_view& str) {
    const i32 ne = addState();
    if (str[0] == '^') {
      bool exclude[256] = {};
      const auto str2 = str.substr(1);
      for (const auto c : str2) exclude[c] = true;
      for (auto c = 1; c < 256; ++c) {
        if (exclude[c]) continue;
        std::for_each(accept.begin(), accept.end(), [&](auto n) { transitions[n][c].push_back(ne); });
        if (!accept.size()) transitions[start][c].push_back(ne);
        if (std::find(alphabet.begin(), alphabet.end(), c) == alphabet.end()) alphabet.push_back(c);
      }
    }
    else {
      for (const auto c : str) {
        std::for_each(accept.begin(), accept.end(), [&](auto n) { transitions[n][c].push_back(ne); });
        if (!accept.size()) transitions[start][c].push_back(ne);
        if (std::find(alphabet.begin(), alphabet.end(), c) == alphabet.end()) alphabet.push_back(c);
      }
    }
    accept = { ne };
    return *this;
  }

  constexpr RegexLexer& alter(const RegexLexer& x) { 
    if (!accept.size()) return *this = x;
    const auto maxx = states.size();
    transitions[start]['\0'].push_back(x.start + maxx);
    std::copy_if(x.alphabet.begin(), x.alphabet.end(), std::back_inserter(alphabet), [&](auto c) {
      return std::find(alphabet.begin(), alphabet.end(), c) == alphabet.end();
    });
    for (const auto& [s1, m] : x.transitions) {
      for (const auto& [c, v] : m) {
        auto& tr = transitions[s1 + maxx][c];
        for (const auto& s2 : v) tr.push_back(s2 + maxx);
      }
    }
    states.insert(states.end(), x.states.begin(), x.states.end());
    std::transform(x.accept.begin(), x.accept.end(), std::back_inserter(accept), [&](auto n) { return n + maxx; });
    return *this; 
  }

  constexpr RegexLexer& close() {
    if (!accept.size()) return *this;
    const i32 ne = addState();
    std::for_each(accept.begin(), accept.end(), [&](auto n) { transitions[n]['\0'].push_back(ne); });
    transitions[start]['\0'].push_back(ne);
    std::for_each(accept.begin(), accept.end(), [&](auto n) { transitions[n]['\0'].push_back(start); });
    accept = { ne };
    return *this;
  }

  constexpr RegexLexer& optional() {
    if (!accept.size()) return *this;
    const i32 ne = addState();
    transitions[start]['\0'].push_back(ne);
    std::for_each(accept.begin(), accept.end(), [&](auto n) { transitions[n]['\0'].push_back(ne); });
    accept = { ne };
    return *this;
  }

  constexpr RegexLexer& concatClose() {
    if (!accept.size()) return *this;
    const i32 ne = addState();
    std::for_each(accept.begin(), accept.end(), [&](auto n) { transitions[n]['\0'].push_back(ne); });
    std::for_each(accept.begin(), accept.end(), [&](auto n) { transitions[n]['\0'].push_back(start); });
    accept = { ne };
    return *this;
  }

  RegexLexer& dfa() {
    std::vector<std::vector<i32>> epsilonCache;
    epsilonCache.reserve(states.size());
    const std::function<std::vector<i32>(std::vector<i32>)> epsilonClosure = [&](auto s) {
      auto s2 = s;
      for (const auto n : s2) {
        if (epsilonCache.size() > n) {
          const auto& t2 = epsilonCache[n];
          std::copy_if(t2.begin(), t2.end(), std::back_inserter(s), [&](auto n) {
            for (const auto x : t2) { if (x == n) return false; }
            return true;
          });
        }
        else {
          std::vector<i32> t2;
          const auto& tr = transitions[n]['\0'];
          t2.insert(t2.end(), tr.begin(), tr.end());
          t2 = epsilonClosure(t2);
          std::copy_if(t2.begin(), t2.end(), std::back_inserter(s), [&](auto n) {
            for (const auto x : t2) { if (x == n) return false; }
            return true;
          });
        }
      }
      return s;
    };
    for (i32 i = 0; i < states.size(); ++i) epsilonCache.push_back(std::move(epsilonClosure({ i })));
    const auto epsilonCached = [&](const std::unordered_set<i32>& s) {
      auto s2 = s;
      for (const auto n : s) {
        const auto& c = epsilonCache[n];
        s2.insert(c.begin(), c.end());
      }
      return s2;
    };

    const auto delta = [&](const auto& s, auto c) {
      std::unordered_set<i32> out;
      for (const auto n : s) {
        const auto& dc = transitions.find(n)->second;
        const auto& dcc = dc.find(c);
        if (dcc != dc.end()) out.insert(dcc->second.begin(), dcc->second.end());
      }
      return out;
    };

    std::vector<std::unordered_set<i32>> ns, work = { epsilonCached({ start }) };
    std::vector<state_t> nstates = { T_NULL };
    start = 0;
    transition_table_t _transitions;

    std::unordered_map<std::unordered_set<i32>, i32> nsc;
    const auto getns = [&](const std::unordered_set<i32>& set) -> size_t {
      const auto& found = nsc.find(set);
      if (found != nsc.end()) return found->second;
      ns.push_back(set);
      const auto n = ns.size() - 1;
      nsc[set] = n;
      _transitions[n];
      return n;
    };

    while (work.size()) {
      auto curr = work.back();
      work.pop_back();
      const auto nn = getns(curr);
      for (const auto c : alphabet) {
        std::unordered_set<i32> s = epsilonCached(delta(curr, c));
        if (!s.size()) continue;
        const auto beforeSize = ns.size();
        const auto to = getns(s);
        _transitions.find(nn)->second[c].push_back(to);
        if (beforeSize != ns.size()) {
          nstates.push_back(std::reduce(s.begin(), s.end(), T_NULL, [&](auto acc, auto n) {
            const auto l = states[n];
            return TF(acc, l);
          }));
          work.push_back(std::move(s));
        }
      }
    }

    std::vector<i32> _accept = accept;
    accept.clear();
    for (auto i = 0; i < ns.size(); ++i) {
      if (std::find_if(_accept.begin(), _accept.end(), [&](auto n) { return ns[i].contains(n); }) != _accept.end())
        accept.push_back(i);
    }
    transitions = std::move(_transitions);
    states = std::move(nstates);
    return *this;
  }

  static RegexLexer parse(const std::string_view& str, T t) {
    auto i = 0;
    auto out = parse(str, i);
    std::for_each(out.accept.begin(), out.accept.end(), [&](auto n) { out.states[n] = t; });
    return out;
  }

  static RegexLexer parse(const std::string_view& str, auto& i) {
    const auto iscontrol = [&](auto c) {
      return c == '(' || c == ')' || c == '*' || c == '|' || c == '\\' || c == '+' || c == '?' || c == '[' || c == ']';
    };
    const auto parse_3 = [&]() {
      RegexLexer out;
      RegexLexer token;
      do {
        if (!iscontrol(str[i])) {
          out.concat(token);
          token = RegexLexer().concat(str[i]);
          continue;
        }
        if (str[i] == '\\') {
          out.concat(token);
          token = RegexLexer().concat(str[++i]);
          continue;
        }
        if (str[i] == '[') {
          out.concat(token);
          const auto start = i + 1;
          while (str[++i] != ']');
          token = RegexLexer().concatClass(str.substr(start, i - start));
          continue;
        }
        if (str[i] == '?') {
          token.optional();
          continue;
        }
        if (str[i] == '+') {
          token.concatClose();
          continue;
        }
        if (str[i] == '*') {
          token.close();
          continue;
        }
        else if (str[i] == '(') {
          ++i;
          out.concat(token);
          token = parse(str, i);
          continue;
        }
        break;
      } while (++i < str.length());
      return out.concat(token);
    };
    const std::function<RegexLexer()> parse_ = [&]() {
      auto lhs = parse_3();
      if (i >= str.length()) return lhs;
      switch (str[i]) {
        case ')': return lhs;
        case '|': ++i; return lhs.alter(parse_());
      }
      return lhs;
    };
    return parse_();
  }
  T match(const std::string_view& str) {
    auto curr = start;
    for (const auto c : str) {
      if (std::find(alphabet.begin(), alphabet.end(), c) == alphabet.end()) return {};
      auto& next = transitions.find(curr)->second[c];
      if (!next.size()) break;
      curr = next[0];
    }
    return states[curr];
  }

  std::vector<state_t> states;
  transition_table_t transitions;
  std::vector<char> alphabet;
  i32 start;
  std::vector<i32> accept;
};
