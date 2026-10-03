#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

#include "DotExport.hpp"
#include "NFA.hpp"
#include "Regex.hpp"

enum class SessionAction { Continue, NewExpression, Exit };

std::string read_line() {
  std::string line;
  if (!std::getline(std::cin, line)) {
    return "";
  }
  return line;
}

std::vector<std::string> split_commands(const std::string &line) {
  std::vector<std::string> commands;
  std::istringstream input(line);
  std::string token;
  while (input >> token) {
    commands.push_back(token);
  }
  return commands;
}

void print_expression_help() {
  std::cout << "Expression commands:\n";
  std::cout << "  to_nfa  - build an NFA from the regular expression\n";
  std::cout << "  help    - show this help\n";
  std::cout << "  exit    - quit the program\n";
}

void print_automaton_help() {
  std::cout << "Automaton commands (space-separated, can be chained):\n";
  std::cout << "  determinize      - determinize the automaton\n";
  std::cout << "  complete         - make the automaton complete\n";
  std::cout << "  minimize         - minimize to a complete DFA\n";
  std::cout << "  epsilon_closure  - eliminate epsilon transitions via epsilon closure\n";
  std::cout << "  complement       - language complement (complete DFA + flip accept)\n";
  std::cout << "  print            - print the automaton in DOT format\n";
  std::cout << "  draw             - save a PNG via Graphviz into output/\n";
  std::cout << "  accept           - check whether a word is in the language\n";
  std::cout << "  help             - show this help\n";
  std::cout << "  new              - discard this automaton and enter another regex\n";
  std::cout << "  exit             - quit the program\n";
  std::cout << "draw requires Graphviz (`dot` in PATH).\n";
  std::cout << "Tip: after you finish with this automaton, type `new` to start over.\n";
}

void prompt_word_and_check(const NFA &automaton) {
  std::cout << "Enter a word (empty line = epsilon): ";
  const std::string word = read_line();
  if (automaton.accepts(word)) {
    std::cout << "yes\n";
  } else {
    std::cout << "no\n";
  }
}

SessionAction apply_automaton_commands(NFA &automaton,
                                       const std::vector<std::string> &commands,
                                       std::size_t &draw_counter) {
  for (const std::string &command : commands) {
    if (command == "determinize") {
      automaton = automaton.determinize();
      std::cout << "Done: determinize\n";
    } else if (command == "complete") {
      automaton = automaton.complete();
      std::cout << "Done: complete\n";
    } else if (command == "minimize") {
      automaton = automaton.minimize();
      std::cout << "Done: minimize\n";
    } else if (command == "epsilon_closure") {
      automaton = automaton.epsilon_closure_nfa();
      std::cout << "Done: epsilon_closure\n";
    } else if (command == "complement") {
      automaton = automaton.complement();
      std::cout << "Done: complement\n";
    } else if (command == "print") {
      std::cout << automaton.to_dot();
    } else if (command == "draw") {
      const std::string path =
          export_automaton_png(automaton, draw_counter++);
      std::cout << "Image saved: " << path << "\n";
    } else if (command == "accept") {
      prompt_word_and_check(automaton);
    } else if (command == "help") {
      print_automaton_help();
    } else if (command == "new") {
      std::cout << "Starting over with a new regular expression.\n";
      return SessionAction::NewExpression;
    } else if (command == "exit") {
      return SessionAction::Exit;
    } else {
      std::cout << "Unknown command: " << command << "\n";
      print_automaton_help();
      return SessionAction::Continue;
    }
  }
  return SessionAction::Continue;
}

SessionAction run_automaton_stage(NFA &automaton, std::size_t &draw_counter) {
  while (true) {
    std::cout << "\nWhat do you want to do with the automaton?\n";
    std::cout << "Options: determinize complete minimize epsilon_closure "
                 "complement print draw accept help new exit\n";
    std::cout << "Use `new` to enter another regular expression.\n";
    std::cout << "> ";

    const std::string line = read_line();
    const std::vector<std::string> commands = split_commands(line);
    if (commands.empty()) {
      continue;
    }

    try {
      const SessionAction action =
          apply_automaton_commands(automaton, commands, draw_counter);
      if (action != SessionAction::Continue) {
        return action;
      }
    } catch (const std::exception &error) {
      std::cout << "Error: " << error.what() << "\n";
    }
  }
}

bool run_expression_stage(NFA &automaton) {
  while (true) {
    std::cout << "Enter a regular expression:\n> ";
    const std::string pattern = read_line();
    if (!std::cin) {
      return false;
    }

    while (true) {
      std::cout << "\nWhat do you want to do with the expression?\n";
      std::cout << "Options: to_nfa help exit\n> ";
      const std::string command_line = read_line();
      if (!std::cin) {
        return false;
      }

      const std::vector<std::string> commands = split_commands(command_line);
      if (commands.empty()) {
        continue;
      }

      const std::string &command = commands.front();
      if (command == "exit") {
        return false;
      }
      if (command == "help") {
        print_expression_help();
        continue;
      }
      if (command != "to_nfa") {
        std::cout << "Unknown command: " << command << "\n";
        print_expression_help();
        continue;
      }

      try {
        automaton = Regex(pattern).to_nfa();
        std::cout << "NFA built.\n";
        return true;
      } catch (const std::exception &error) {
        std::cout << "Error: " << error.what() << "\n";
        break;
      }
    }
  }
}

int main() {
  std::size_t draw_counter = 1;

  while (true) {
    NFA automaton(1, 0, {false}, {}, {{}});

    if (!run_expression_stage(automaton)) {
      break;
    }

    const SessionAction action = run_automaton_stage(automaton, draw_counter);
    if (action == SessionAction::Exit) {
      break;
    }
  }

  std::cout << "Bye.\n";
  return 0;
}
