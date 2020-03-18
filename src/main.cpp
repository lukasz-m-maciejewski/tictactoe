#include <docopt/docopt.h>
#include <imgui-SFML.h>
#include <imgui.h>
#include <spdlog/spdlog.h>

#include <SFML/Graphics.hpp>
#include <SFML/System/Clock.hpp>
#include <SFML/Window/Event.hpp>
#include <algorithm>
#include <functional>
#include <optional>
#include <outcome.hpp>
#include <system_error>
#include <tuple>
#include <vector>

namespace outcome = OUTCOME_V2_NAMESPACE;

namespace tictactoe {
std::size_t pos2idx(int row, int column, int grid_size) {
  return static_cast<std::size_t>(column + row * grid_size);
}

std::tuple<int, int> idx2pos(std::size_t index, int grid_size) {
  const auto x = static_cast<int>(index) % grid_size;
  const auto y = static_cast<int>(index) / grid_size;
  return std::make_tuple(x, y);
}

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

class Position {
  int row_;
  int col_;

  Position(int row, int col) : row_{row}, col_{col} {}

 public:
  static outcome::result<Position> create_position_for_board(
      int row, int col, const Board& board);

  int row() const { return row_; }
  int col() const { return col_; }
};

using FieldChangeListener = std::function<FieldState(std::tuple<int, int>)>;

class Board {
  using BoardData = std::vector<FieldState>;
  BoardData fields_;
  int board_size_;

  Board(int board_size)
      : fields_(static_cast<BoardData::size_type>(board_size * board_size),
                FieldState::Empty),
        board_size_{board_size} {}

 public:
  static outcome::result<Board> create_board(int board_size) {
    if (board_size <= 1) {
      return std::errc::argument_out_of_domain;
    }
    return Board(board_size);
  }

  std::optional<Player> maybe_winner_for_row(int row_id) {
    if (0 < row_id or row_id >= board_size_) {
      return std::nullopt;
    }

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

  std::optional<Player> maybe_winner_for_column(int col_id) {
    if (0 < col_id or col_id >= board_size_) {
      return std::nullopt;
    }

    bool all_eq = true;
    for (int i = 1; i < board_size_; ++i) {
      if (fields_[static_cast<BoardData::size_type>(col_id)] !=
          fields_[static_cast<BoardData::size_type>(col_id +
                                                    i * board_size_)]) {
        all_eq = false;
        break;
      }
    }

    if (!all_eq) {
      return std::nullopt;
    }

    switch (fields_[static_cast<BoardData::size_type>(col_id)]) {
      case FieldState::Circle:
        return Player::CirclePlayer;
      case FieldState::Cross:
        return Player::CrossPlayer;
      default:
        return std::nullopt;
    }
  }

  std::optional<Player> maybe_get_winner() {
    for (int i = 0; i < board_size_; ++i) {
      auto player_opt = maybe_winner_for_row(i);
      if (player_opt) {
        return player_opt;
      }
    }

    for (int i = 0; i < board_size_; ++i) {
      auto player_opt = maybe_winner_for_column(i);
      if (player_opt) {
        return player_opt;
      }
    }

    return std::nullopt;
  }

  int size() const { return board_size_; }

  FieldState get_field_state_at(const Position& pos) {
    return fields_.at(pos2idx(pos.row(), pos.col(), board_size_));
  }

  outcome::result<void> update_field_state_at(const Position& pos,
                                              FieldState state) {
    if (state == FieldState::Empty) {
      return std::errc::argument_out_of_domain;
    }
    fields_.at(pos2idx(pos.row(), pos.col(), board_size_)) = state;
  }
};

outcome::result<Position> Position::create_position_for_board(
    int row, int col, const Board& board) {
  if (row < 0 or row >= board.size()) {
    return std::errc::argument_out_of_domain;
  }
  if (col < 0 or col >= board.size()) {
    return std::errc::argument_out_of_domain;
  }

  return Position(row, col);
}

class Grid {
  int num_boxes_side_;
  int num_boxes_total_;
  std::vector<sf::FloatRect> field_bounds_;
  std::vector<sf::RectangleShape> fields_;
  constexpr static float side_size = 10.0f;
  constexpr static float offset_factor = 0.1f;
  constexpr static float offset = side_size * offset_factor;
  constexpr static float spacing = side_size + offset;

  FieldChangeListener listener_;

 public:
  Grid(int num_boxes)
      : num_boxes_side_{num_boxes}, num_boxes_total_{num_boxes * num_boxes} {
    field_bounds_.reserve(static_cast<std::size_t>(num_boxes_total_));
    fields_.reserve(static_cast<std::size_t>(num_boxes_total_));

    for (int i = 0; i < num_boxes_side_; ++i) {
      for (int j = 0; j < num_boxes_side_; ++j) {
        const auto rect_size = sf::Vector2f{side_size, side_size};
        sf::RectangleShape rect{rect_size};

        const float shift_x = offset + spacing * static_cast<float>(i);
        const float shift_y = offset + spacing * static_cast<float>(j);
        const auto position = sf::Vector2f{shift_x, shift_y};

        rect.setPosition(position);
        rect.setFillColor(sf::Color::Red);

        fields_.emplace_back(std::move(rect));
        field_bounds_.emplace_back(position, rect_size);
      }
    }
  }

  void draw_on(sf::RenderWindow& window) {
    const float bgnd_size =
        offset + spacing * static_cast<float>(num_boxes_side_);
    sf::RectangleShape background{sf::Vector2f{bgnd_size, bgnd_size}};
    background.setPosition(0.0f, 0.0f);
    background.setFillColor(sf::Color::Blue);
    window.draw(background);

    for (const auto& rect : fields_) {
      window.draw(rect);
    }
  }

  void toggle_color(sf::RectangleShape& s) {
    auto color = s.getFillColor();
    if (color == sf::Color::Red) {
      s.setFillColor(sf::Color::Green);
      return;
    }

    s.setFillColor(sf::Color::Red);
  }

  void set_color_for_state(sf::RectangleShape& s, FieldState state) {
    switch (state) {
      case FieldState::Empty:
        s.setFillColor(sf::Color::Red);
        return;
      case FieldState::Circle:
        s.setFillColor(sf::Color::Green);
        return;
      case FieldState::Cross:
        s.setFillColor(sf::Color::Magenta);
        return;
    }
  }

  void handle_click(const sf::Vector2f& location) {
    auto it = std::find_if(fields_.begin(), fields_.end(),
                           [&](const sf::RectangleShape& r) {
                             return r.getGlobalBounds().contains(location);
                           });
    if (it != fields_.end()) {
      const auto begin = fields_.begin();
      const std::size_t idx =
          static_cast<std::size_t>(std::distance(begin, it));
      if (listener_) {
        set_color_for_state(*it, listener_(idx2pos(idx, num_boxes_side_)));
        return;
      }
      toggle_color(*it);
    }
  }

  void set_clicked_handler(FieldChangeListener listener) {
    listener_ = std::move(listener);
  }

  sf::View get_view() const {
    sf::View v;
    const float num_boxes_f = static_cast<float>(num_boxes_side_);
    const float side_len = offset + spacing * (num_boxes_f);
    const float center_pos = side_len * 0.5f;
    v.setSize(side_len, side_len);
    v.setCenter(center_pos, center_pos);
    return v;
  }
};

sf::FloatRect ComputeAspectPreservingViewport(const sf::Vector2u& screen_size) {
  if (screen_size.x >= screen_size.y) {
    const float dim_ratio_inv =
        static_cast<float>(screen_size.y) / static_cast<float>(screen_size.x);
    const float left_margin = (1.0f - dim_ratio_inv) * 0.5f;
    return sf::FloatRect{left_margin, 0.0f, dim_ratio_inv, 1.0f};
  }

  const float dim_ratio_inv =
      static_cast<float>(screen_size.x) / static_cast<float>(screen_size.y);
  const float top_margin = (1.0f - dim_ratio_inv) * 0.5f;
  return sf::FloatRect{0.0f, top_margin, 1.0f, dim_ratio_inv};
}

}  // namespace tictactoe

outcome::result<void> Main() {
  // Use the default logger (stdout, multi-threaded, colored)
  spdlog::info("Hello, {}!", "World");

  sf::RenderWindow window(sf::VideoMode(1024, 768), "ImGui + SFML = <3");
  window.setFramerateLimit(60);
  ImGui::SFML::Init(window);

  constexpr auto scale_factor = 2.0;
  ImGui::GetStyle().ScaleAllSizes(scale_factor);
  ImGui::GetIO().FontGlobalScale = scale_factor;

  tictactoe::Board board = OUTCOME_TRYX(tictactoe::Board::create_board(3));
  tictactoe::Grid g{3};

  sf::FloatRect viewport_debug{};

  bool show_overlay = false;

  sf::Clock deltaClock;
  while (window.isOpen()) {
    sf::Event event{};
    while (window.pollEvent(event)) {
      if (show_overlay) ImGui::SFML::ProcessEvent(event);

      if (event.type == sf::Event::Closed) {
        window.close();
      }

      // catch the resize events
      if (event.type == sf::Event::Resized) {
        auto viewport =
            tictactoe::ComputeAspectPreservingViewport(window.getSize());
        viewport_debug = viewport;
        auto view = g.get_view();
        view.setViewport(viewport);
        window.setView(view);
      }

      if (event.type == sf::Event::MouseButtonReleased) {
        const sf::Vector2f mouse_pos_world =
            window.mapPixelToCoords(sf::Mouse::getPosition(window));
        g.handle_click(mouse_pos_world);
        spdlog::info("click at ({}, {})", mouse_pos_world.x, mouse_pos_world.y);
      }
    }

    if (show_overlay) {
      const auto window_size = window.getSize();
      const auto window_size_text =
          fmt::format("window size: {}x{}", window_size.x, window_size.y);
      const auto viewport_text = fmt::format(
          "viewport: {} {} {} {}", viewport_debug.left, viewport_debug.top,
          viewport_debug.height, viewport_debug.width);

      ImGui::SFML::Update(window, deltaClock.restart());
      ImGui::Begin("Debug info");
      ImGui::TextUnformatted(window_size_text.c_str());
      ImGui::TextUnformatted(viewport_text.c_str());
      ImGui::End();
    }

    window.clear(sf::Color::Black);
    g.draw_on(window);

    if (show_overlay) ImGui::SFML::Render(window);

    window.display();
  }

  ImGui::SFML::Shutdown();

  return outcome::success();
}

int main([[maybe_unused]] int argc, [[maybe_unused]] const char** argv) {
  auto result = Main();
  return result ? 0 : 1;
}
