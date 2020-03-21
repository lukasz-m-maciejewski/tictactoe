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

template <typename T>
auto EqualTo(const T& val) {
  return [&val](const auto& x) { return x == val; };
}

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

class Engine;

class Position {
  int row_;
  int col_;

  Position(int row, int col) : row_{row}, col_{col} {}

 public:
  static outcome::result<Position> create_position_for_engine(
      std::tuple<int, int> p, const Engine& e) {
    return create_position_for_engine(std::get<0>(p), std::get<1>(p), e);
  }
  static outcome::result<Position> create_position_for_engine(
      int row, int col, const Engine& engine);

  int row() const { return row_; }
  int col() const { return col_; }
};

using FieldChangeListener = std::function<FieldState(std::tuple<int, int>)>;

class Engine {
  using BoardData = std::vector<FieldState>;
  BoardData fields_;
  int board_size_;
  Player active_player_ = Player::CrossPlayer;
  std::optional<Player> winner_ = std::nullopt;

  Engine(int board_size)
      : fields_(static_cast<BoardData::size_type>(board_size * board_size),
                FieldState::Empty),
        board_size_{board_size} {}

 public:
  static outcome::result<Engine> create_engine(int board_size) {
    if (board_size <= 1) {
      return std::errc::argument_out_of_domain;
    }
    return Engine(board_size);
  }

  std::optional<Player> maybe_winner_for_row(int row_id) {
    if (row_id < 0 or row_id >= board_size_) {
      return std::nullopt;
    }

    const auto row_begin = std::next(fields_.begin(), row_id * board_size_);
    const auto row_end = std::next(fields_.begin(), (row_id + 1) * board_size_);

    if (std::all_of(row_begin, row_end, EqualTo(FieldState::Circle))) {
      return Player::CirclePlayer;
    }
    if (std::all_of(row_begin, row_end, EqualTo(FieldState::Cross))) {
      return Player::CrossPlayer;
    }

    return std::nullopt;
  }

  std::optional<Player> maybe_winner_for_column(int col_id) {
    if (col_id < 0 or col_id >= board_size_) {
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

  std::optional<Player> maybe_get_winner_for_diagonal() {
    bool all_eq = true;
    for (int i = 1; i < board_size_; ++i) {
      if (fields_[pos2idx(0, 0, board_size_)] !=
          fields_[pos2idx(i, i, board_size_)]) {
        all_eq = false;
        break;
      }
    }
    if (!all_eq) {
      return std::nullopt;
    }

    switch (fields_[pos2idx(0, 0, board_size_)]) {
      case FieldState::Circle:
        return Player::CirclePlayer;
      case FieldState::Cross:
        return Player::CrossPlayer;
      default:
        return std::nullopt;
    }
  }

  std::optional<Player> maybe_get_winner_for_antidiagonal() {
    bool all_eq = true;
    auto last_idx = board_size_ - 1;
    for (int i = 1; i < board_size_; ++i) {
      if (fields_[pos2idx(0, last_idx, board_size_)] !=
          fields_[pos2idx(i, last_idx - i, board_size_)]) {
        all_eq = false;
        break;
      }
    }
    if (!all_eq) {
      return std::nullopt;
    }

    switch (fields_[pos2idx(0, last_idx, board_size_)]) {
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

    auto player_opt = maybe_get_winner_for_diagonal();
    if (player_opt) {
      return player_opt;
    }

    player_opt = maybe_get_winner_for_antidiagonal();
    if (player_opt) {
      return player_opt;
    }

    return std::nullopt;
  }

  int board_size() const { return board_size_; }
  std::optional<Player> maybe_winner() const { return winner_; }

  FieldState get_field_state_at(const Position& pos) {
    return fields_.at(pos2idx(pos.row(), pos.col(), board_size_));
  }

  Player next_player() {
    switch (active_player_) {
      case Player::CrossPlayer:
        return Player::CirclePlayer;
      case Player::CirclePlayer:
      default:
        return Player::CrossPlayer;
    }
  }

  outcome::result<void> update_field_state_at(const Position& pos,
                                              FieldState state) {
    if (state == FieldState::Empty) {
      return std::errc::argument_out_of_domain;
    }
    fields_.at(pos2idx(pos.row(), pos.col(), board_size_)) = state;
    return outcome::success();
  }

  outcome::result<void> handle_field_selected(const Position& pos) {
    if (winner_) {
      return outcome::success();
    }

    FieldState state = get_field_state_at(pos);
    if (state != FieldState::Empty) {
      return outcome::failure(std::errc::invalid_argument);
    }

    FieldState new_state = active_player_ == Player::CrossPlayer
                               ? FieldState::Cross
                               : FieldState::Circle;
    OUTCOME_TRYV(update_field_state_at(pos, new_state));

    active_player_ = next_player();
    winner_ = maybe_get_winner();

    return outcome::success();
  }
};

outcome::result<Position> Position::create_position_for_engine(
    int row, int col, const Engine& engine) {
  if (row < 0 or row >= engine.board_size()) {
    return std::errc::argument_out_of_domain;
  }
  if (col < 0 or col >= engine.board_size()) {
    return std::errc::argument_out_of_domain;
  }

  return Position(row, col);
}

class Grid {
  Engine& engine_;
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
  Grid(Engine& engine)
      : engine_{engine},
        num_boxes_side_{engine.board_size()},
        num_boxes_total_{num_boxes_side_ * num_boxes_side_} {
    field_bounds_.reserve(static_cast<std::size_t>(num_boxes_total_));
    fields_.reserve(static_cast<std::size_t>(num_boxes_total_));

    auto result = update_grid();
    if (!result) {
      spdlog::error("initial grid update failed: {}", result.error().message());
    }
  }

  outcome::result<void> update_grid() {
    for (int i = 0; i < num_boxes_side_; ++i) {
      for (int j = 0; j < num_boxes_side_; ++j) {
        const auto rect_size = sf::Vector2f{side_size, side_size};
        sf::RectangleShape rect{rect_size};

        const float shift_x = offset + spacing * static_cast<float>(i);
        const float shift_y = offset + spacing * static_cast<float>(j);
        const auto position = sf::Vector2f{shift_x, shift_y};

        rect.setPosition(position);
        rect.setFillColor(sf::Color::Red);
        auto pos =
            OUTCOME_TRYX(Position::create_position_for_engine(j, i, engine_));
        set_color_for_state(rect, engine_.get_field_state_at(pos));

        fields_.emplace_back(std::move(rect));
        field_bounds_.emplace_back(position, rect_size);
      }
    }

    return outcome::success();
  }

  void draw_on(sf::RenderWindow& window) {
    const float bgnd_size =
        offset + spacing * static_cast<float>(num_boxes_side_);
    sf::RectangleShape background{sf::Vector2f{bgnd_size, bgnd_size}};
    background.setPosition(0.0f, 0.0f);
    background.setFillColor(sf::Color::Blue);
    if (auto w = engine_.maybe_winner()) {
      switch (*w) {
        case Player::CrossPlayer:
          background.setFillColor(sf::Color::Magenta);
          break;
        case Player::CirclePlayer:
          background.setFillColor(sf::Color::Green);
          break;
      }
    }
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

  outcome::result<void> handle_click(const sf::Vector2f& location) {
    auto it = std::find_if(fields_.begin(), fields_.end(),
                           [&](const sf::RectangleShape& r) {
                             return r.getGlobalBounds().contains(location);
                           });
    if (it != fields_.end()) {
      const auto begin = fields_.begin();
      const std::size_t idx =
          static_cast<std::size_t>(std::distance(begin, it));
      auto pos = OUTCOME_TRYX(Position::create_position_for_engine(
          idx2pos(idx, num_boxes_side_), engine_));

      OUTCOME_TRYV(engine_.handle_field_selected(pos));
      OUTCOME_TRYV(update_grid());
    }
    return outcome::success();
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

  tictactoe::Engine board = OUTCOME_TRYX(tictactoe::Engine::create_engine(3));
  tictactoe::Grid g{board};

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
        auto result = g.handle_click(mouse_pos_world);
        if (!result) {
          spdlog::warn("click failed with: {}", result.error().message());
        }
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
