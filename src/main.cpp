#include <iostream>

#include <spdlog/spdlog.h>
#include <imgui.h>
#include <imgui-SFML.h>
#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/System/Clock.hpp>
#include <SFML/Window/Event.hpp>
#include <SFML/Graphics/CircleShape.hpp>


#include <docopt/docopt.h>


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
      ImGui::Checkbox(fmt::format("{} : {}", index, step.first).c_str(), &step.second);
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
