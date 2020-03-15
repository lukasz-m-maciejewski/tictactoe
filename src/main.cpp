#include <vector>
#include <system_error>
#include <optional>

#include <spdlog/spdlog.h>
#include <imgui.h>
#include <imgui-SFML.h>
#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/System/Clock.hpp>
#include <SFML/Window/Event.hpp>
#include <SFML/Graphics/CircleShape.hpp>
#include <outcome.hpp>

#include <docopt/docopt.h>

namespace outcome = OUTCOME_V2_NAMESPACE;

namespace tictactoe {
enum class Player {
  CirclePlayer,
  CrossPlayer,
};

enum class FieldState {
  Empty,
  Circle,
  Cross,
};

class Board;

class Position
{
  int row_;
  int col_;

  Position(int row, int col) : row_{ row }, col_{ col } {}

public:
  static outcome::result<Position>
    create_position_for_board(int row, int col, const Board& board);

  int row() { return row_; }
  int col() { return col_; }
};

class Board
{
  using BoardData = std::vector<FieldState>;
  BoardData fields_;
  int board_size_;

  Board(int board_size)
    : fields_(static_cast<BoardData::size_type>(board_size * board_size),
      FieldState::Empty),
      board_size_{ board_size }
  {}

public:
  static outcome::result<Board> create_board(int board_size)
  {
    if (board_size <= 1) { return std::errc::argument_out_of_domain; }
    return Board(board_size);
  }

  std::optional<Player> maybe_winner_for_row(int row_id)
  {
    if (0 < row_id or row_id >= board_size_) { return std::nullopt; }

    const auto row_begin = std::next(fields_.begin(), row_id * board_size_);
    const auto row_end = std::next(fields_.begin(), (row_id + 1) * board_size_);

    if (std::all_of(row_begin, row_end, [](const auto& field_state) {
          return field_state == FieldState::Circle;
        })) {
      return Player::CirclePlayer;
    }
    if (std::all_of(row_begin, row_end, [](const auto& field_state) {
          return field_state == FieldState::Cross;
        })) {
      return Player::CrossPlayer;
    }

    return std::nullopt;
  }

  std::optional<Player> maybe_winner_for_column(int col_id)
  {
    if (0 < col_id or col_id >= board_size_) { return std::nullopt; }

    bool all_eq = true;
    for (int i = 1; i < board_size_; ++i) {
      if (fields_[static_cast<BoardData::size_type>(col_id)]
          != fields_[static_cast<BoardData::size_type>(
            col_id + i * board_size_)]) {
        all_eq = false;
        break;
      }
    }

    if (!all_eq) { return std::nullopt; }

    switch (fields_[static_cast<BoardData::size_type>(col_id)]) {
    case FieldState::Circle:
      return Player::CirclePlayer;
    case FieldState::Cross:
      return Player::CrossPlayer;
    default:
      return std::nullopt;
    }
  }

  std::optional<Player> maybe_get_winner()
  {
    for (int i = 0; i < board_size_; ++i) {
      auto player_opt = maybe_winner_for_row(i);
      if (player_opt) { return player_opt; }
    }

    for (int i = 0; i < board_size_; ++i) {
      auto player_opt = maybe_winner_for_column(i);
      if (player_opt) { return player_opt; }
    }

    return std::nullopt;
  }

  int size() const { return board_size_; }
};

outcome::result<Position>
  Position::create_position_for_board(int row, int col, const Board& board)
{
  if (row < 0 or row >= board.size()) {
    return std::errc::argument_out_of_domain;
  }
  if (col < 0 or col >= board.size()) {
    return std::errc::argument_out_of_domain;
  }

  return Position(row, col);
}

}// namespace tictactoe

int main([[maybe_unused]] int argc, [[maybe_unused]] const char** argv)
{
  // Use the default logger (stdout, multi-threaded, colored)
  spdlog::info("Hello, {}!", "World");


  sf::RenderWindow window(sf::VideoMode(1024, 768), "ImGui + SFML = <3");
  window.setFramerateLimit(60);
  ImGui::SFML::Init(window);

  constexpr auto scale_factor = 2.0;
  ImGui::GetStyle().ScaleAllSizes(scale_factor);
  ImGui::GetIO().FontGlobalScale = scale_factor;

  std::vector<std::pair<std::string, bool>> states = { {
    { "The Plan", false },
    { "Getting Started", false },
    { "Finding Errors As Soon As Possible", false },
    { "Handling Command Line Parameters", false },
    { "C++ 20 So Far", false },
    { "Reading SFML Input States", false },
    { "Managing Game State", false },
    { "Making Our Game Testable", false },
    { "Making Game State Allocator Aware", false },
    { "Add Logging To Game Engine", false },
    { "Draw A Game Map", false },
    { "Dialog Trees", false },
    { "Porting From SFML To SDL", false },
  } };

  sf::Clock deltaClock;
  while (window.isOpen()) {
    sf::Event event{};
    while (window.pollEvent(event)) {
      ImGui::SFML::ProcessEvent(event);

      if (event.type == sf::Event::Closed) { window.close(); }
    }

    ImGui::SFML::Update(window, deltaClock.restart());


    ImGui::Begin("The Plan");

    int index = 0;
    for (auto& step : states) {
      ImGui::Checkbox(
        fmt::format("{} : {}", index, step.first).c_str(), &step.second);
      ++index;
    }


    ImGui::End();

    window.clear();
    ImGui::SFML::Render(window);
    window.display();
  }

  ImGui::SFML::Shutdown();

  return 0;
}
